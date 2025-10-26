/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <fip_parser.h>
#include <platform_error_codes.h>
#include <string.h>
#include "bootutil/image.h"
#include "flash_layout.h"
#include "fwu_esrt.h"
#include "fwu_flash.h"
#include "fwu_metadata_v2.h"
#include "fwu_private_metadata.h"
#include "psa/error.h"
#include "psa/update.h"
#include "tfm_boot_status.h"
#include "tfm_bootloader_fwu_abstraction.h"
#include "tfm_log.h"

#define LOG_TAG "FWU_SHIM: "

static struct fwu_private_metadata private_mdata = { 0 };
static struct fwu_metadata_v2 fwu_mdata = {0};

/* Read and update the component state */
static psa_status_t fwu_update_component_state(psa_fwu_component_t component,
                                               uint8_t next_state,
                                               bool switch_bank_index)
{
    psa_status_t status;

    private_mdata.fwu_image_state[component] = next_state;

    /* Handle boot_index update for FWU PSA_FWU_REJECTED state */
    if (switch_bank_index) {
        private_mdata.boot_index ^= 1;
    }

    return fwu_private_metadata_write(&private_mdata);
}

/* Updates components state */
static psa_status_t fwu_update_components_state(const psa_fwu_component_t *components,
                                                int number,
                                                uint8_t next_state)
{
    assert(components != NULL);

    for (int idx = 0; idx < number; idx++) {
        private_mdata.fwu_image_state[components[idx]] = next_state;
    }

    return PSA_SUCCESS;
}

psa_status_t fwu_bootloader_init(void)
{
    psa_status_t status;

    status = fwu_private_metadata_init();
    if (status != PSA_SUCCESS) {
        ERROR(LOG_TAG "RSE flash initialization failed\n");
        return status;
    }

    status = fwu_private_metadata_read(&private_mdata);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = fwu_metadata_init();
    if (status != PSA_SUCCESS) {
        ERROR(LOG_TAG "AP flash initialization failed\n");
        return status;
    }

    status = fwu_metadata_read(&fwu_mdata);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = esrt_init_boot_data();

    return status;
}

psa_status_t fwu_bootloader_staging_area_init(psa_fwu_component_t component,
                                              const void *manifest,
                                              size_t manifest_size)
{
    (void)manifest;
    (void)manifest_size;

    return fwu_update_component_state(component, PSA_FWU_WRITING_CANDIDATE,
                                      false);
}

psa_status_t fwu_bootloader_load_image(psa_fwu_component_t component,
                                       size_t block_offset,
                                       const void *block, size_t block_size)
{
    const struct fwu_image_location *image_bank;
    uint32_t update_offset;
    uint8_t current_state;
    psa_status_t status;
    uint8_t update_bank;

    if (block == NULL) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    image_bank = fwu_get_image_location(component);
    if (component != image_bank->component) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    /* validate block_size and block_offset */
    if (block_offset > (UINT32_MAX - block_size)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }
    if ((block_size + block_offset) > image_bank->partition_size) {
        ERROR(LOG_TAG "received image size : 0x%x, maximum allowed size : 0x%x\n",
              (block_offset + block_size), image_bank->partition_size);
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    current_state = private_mdata.fwu_image_state[component];
    if (current_state != PSA_FWU_WRITING_CANDIDATE) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    update_bank = (fwu_mdata.active_index == FWU_BANK_0) ? FWU_BANK_1 : FWU_BANK_0;
    update_offset = image_bank->partition_offset[update_bank] + block_offset;

    status = fwu_flash_write(image_bank->flash, update_offset, block, block_size);
    return status;
}

psa_status_t fwu_bootloader_install_image(const psa_fwu_component_t *candidates,
                                          uint8_t number)
{
    uint8_t update_bank, idx;
    psa_status_t status;

    if (candidates == NULL) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    for (idx = 0; idx < number; idx++) {
        status = esrt_update_last_attempt(&private_mdata, candidates[idx]);
        if (status != PSA_SUCCESS) {
            return status;
        }
    }

    status = fwu_update_components_state(candidates, number, PSA_FWU_STAGED);
    if (status != PSA_SUCCESS) {
        return status;
    }

    /* switch boot_index in private metadata */
    private_mdata.boot_index ^= 1;

    status = fwu_private_metadata_write(&private_mdata);
    if (status != PSA_SUCCESS) {
        return status;
    }

    update_bank = (fwu_mdata.active_index == FWU_BANK_0) ? FWU_BANK_1 : FWU_BANK_0;
    /* Update active index, previous index and bank state in Metadata v2 */
    fwu_mdata.previous_active_index = fwu_mdata.active_index;
    fwu_mdata.active_index = update_bank;
    fwu_mdata.bank_state[update_bank] = FWU_BANK_STATE_VALID;

    status = fwu_metadata_write(&fwu_mdata);
    if (status != PSA_SUCCESS) {
        return status;
    }

    return PSA_SUCCESS_REBOOT;
}

psa_status_t fwu_bootloader_mark_image_accepted(const psa_fwu_component_t *trials,
                                                uint8_t number)
{
    psa_status_t status;
    uint8_t active_index;

    if (trials == NULL) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    status = fwu_update_components_state(trials, number, PSA_FWU_UPDATED);
    if (status != PSA_SUCCESS) {
        return status;
    }

    active_index = fwu_mdata.active_index;
    for (int idx = 0; idx < number; idx++) {
        fwu_mdata.fw_desc.img_entry[trials[idx]].img_props[active_index].accepted =
                FWU_IMAGE_ACCEPTED;
        private_mdata.esrt_entries[trials[idx]].last_attempt_status = LAST_ATTEMPT_STATUS_SUCCESS;

        /* lowest_supported_fw_version should be updated when the
         * security counter increases. For now we are relying on
         * last_attempt_version. */
        private_mdata.esrt_entries[trials[idx]].lowest_supported_fw_version =
            private_mdata.esrt_entries[trials[idx]].last_attempt_version;
    }
    fwu_mdata.bank_state[active_index] = FWU_BANK_STATE_ACCEPTED;

    status = fwu_private_metadata_write(&private_mdata);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = fwu_metadata_write(&fwu_mdata);
    return status;
}

/* Reject the staged image. */
psa_status_t fwu_bootloader_reject_staged_image(psa_fwu_component_t component)
{
    psa_status_t status;

    /* Set the component state to PSA_FWU_FAILED and rollback the boot_index
     * in private metadata
     */
    status = fwu_update_component_state(component, PSA_FWU_FAILED, true);
    if (status != PSA_SUCCESS) {
        return status;
    }

    fwu_mdata.bank_state[fwu_mdata.active_index] = FWU_BANK_STATE_INVALID;
    fwu_mdata.fw_desc.img_entry[component].img_props[fwu_mdata.active_index].accepted =
            FWU_IMAGE_NOT_ACCEPTED;

    /* Swap the active and previous active image indices
     * Note: Both indices can only be 0 or 1
     */
    fwu_mdata.previous_active_index ^= 1;
    fwu_mdata.active_index ^= 1;

    return fwu_metadata_write(&fwu_mdata);
}

/* Reject the running image in trial state.
 * The image will be reverted if it is not accepted explicitly.
 */
psa_status_t fwu_bootloader_reject_trial_image(psa_fwu_component_t component)
{
    psa_status_t status;

    /* Set the component state to PSA_FWU_REJECTED and rollback the boot_index
     * in private metadata
     */
    status = fwu_update_component_state(component, PSA_FWU_REJECTED, true);
    if (status != PSA_SUCCESS)
        return status;

    fwu_mdata.bank_state[fwu_mdata.active_index] = FWU_BANK_STATE_INVALID;
    fwu_mdata.fw_desc.img_entry[component].img_props[fwu_mdata.active_index].accepted =
            FWU_IMAGE_NOT_ACCEPTED;

    /* Swap the active and previous active image indices
     * Note: Both indices can only be 0 or 1
     */
    fwu_mdata.previous_active_index ^= 1;
    fwu_mdata.active_index ^= 1;

    status = fwu_metadata_write(&fwu_mdata);
    if (status != PSA_SUCCESS) {
        return status;
    }

    return PSA_SUCCESS_REBOOT;
}

psa_status_t fwu_bootloader_abort(psa_fwu_component_t component)
{
    /* Functionality unsupported */
    return PSA_ERROR_NOT_SUPPORTED;
}

static void construct_impl_info(psa_fwu_component_t component,
                                psa_fwu_impl_info_t *impl_info)
{
    impl_info->lowest_supported_fw_version =
            private_mdata.esrt_entries[component].lowest_supported_fw_version;
    impl_info->last_attempt_version =
                    private_mdata.esrt_entries[component].last_attempt_version;
    impl_info->last_attempt_status =
                    private_mdata.esrt_entries[component].last_attempt_status;
    impl_info->fw_type = ESRT_FW_TYPE_SYSTEMFIRMWARE;
}

psa_status_t fwu_bootloader_get_image_info(psa_fwu_component_t component, bool query_state,
                                           bool query_impl_info, psa_fwu_component_info_t *info)
{
    const struct fwu_image_location *image_bank;
    psa_status_t status;
    struct psa_fwu_image_version_t fwu_version;
    uint32_t version = 0;

    if (info == NULL) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (query_state) {
        info->state = private_mdata.fwu_image_state[component];
    }

    image_bank = fwu_get_image_location(component);
    info->max_size = image_bank->partition_size;
    info->location = image_bank->partition_offset[fwu_mdata.active_index];

    status = esrt_get_active_image_version(&private_mdata, component, &fwu_version);
    if (status != PSA_SUCCESS) {
        return status;
    }
    info->version = fwu_version;

    if (query_impl_info) {
        construct_impl_info(component, &info->impl);
    }

    return PSA_SUCCESS;
}

psa_status_t fwu_bootloader_clean_component(psa_fwu_component_t component)
{
    psa_status_t status;

    status = fwu_update_component_state(component, PSA_FWU_READY, false);
    /* TODO: try and find */
    return status;
}
