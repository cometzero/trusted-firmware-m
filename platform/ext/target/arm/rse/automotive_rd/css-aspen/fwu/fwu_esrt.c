/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <string.h>
#include "bl2_image_id.h"
#include "bootutil/image.h"
#include "fip_parser.h"
#include "flash_layout.h"
#include "fwu_esrt.h"
#include "fwu_flash.h"
#include "fwu_private_metadata.h"
#include "image_layout_bl1_2.h"
#include "platform_error_codes.h"
#include "psa/error.h"
#include "psa/update.h"
#include "service_api.h"
#include "tfm_boot_status.h"

#define MAX_IMAGE_INFO_LENGTH   (MCUBOOT_IMAGE_NUMBER * \
                                (sizeof(struct image_version) + \
                                 SHARED_DATA_ENTRY_HEADER_SIZE))

/* Convert the image version into uint32_t type. */
#define CONVERT_FWU_VERSION(major, minor, revision)    \
                            ((uint32_t)((((uint32_t)(major) & 0xFF) << 24) | \
                                        (((uint32_t)(minor) & 0xFF) << 16) | \
                                        (revision & 0xFFFF)))

/* Contains the received boot status information from bootloader */
typedef struct fwu_image_info_data_s {
    struct shared_data_tlv_header header;
    uint8_t data[MAX_IMAGE_INFO_LENGTH];
} fwu_image_info_data_t;

static fwu_image_info_data_t __attribute__((aligned(4))) boot_shared_data;

static psa_status_t get_tfm_bl2_image_version(uint8_t bank_index,
                                              uint8_t *major, uint8_t *minor,
                                              uint16_t *revision)
{
    const struct fwu_image_location *image_bank;
    struct tfm_bl1_image_version_t image_ver;
    psa_status_t status;
    uint32_t offset;

    /* Offset of BL2 image version in BL1_2 image structure */
    size_t ver_offset = offsetof(struct bl1_2_image_t, protected_values.version);

    /* Read the offset of BL2 image in the selected bank. */
    image_bank = fwu_get_image_location(FWU_COMPONENT_INDEX_BL2);
    offset = image_bank->partition_offset[bank_index];

    /* Read out the BL2 image version */
    status = fwu_flash_read(image_bank->flash, offset + ver_offset,
                            (void *)&image_ver, sizeof(image_ver));
    if (status != PSA_SUCCESS) {
        return status;
    }

    *major = image_ver.major;
    *minor = image_ver.minor;
    *revision = image_ver.revision;

    return PSA_SUCCESS;
}

static psa_status_t get_tfm_bl2_update_image_version(
                                    const struct fwu_private_metadata *mdata,
                                    uint32_t *version)
{
    psa_status_t status;
    uint8_t major, minor;
    uint16_t revision;

    status = get_tfm_bl2_image_version(mdata->boot_index ^ 1, &major,
                                       &minor, &revision);
    if (status != PSA_SUCCESS) {
        return status;
    }

    *version = CONVERT_FWU_VERSION(major, minor, revision);

    return PSA_SUCCESS;
}

psa_status_t esrt_get_active_image_version(struct fwu_private_metadata *mdata,
                                           psa_fwu_component_t component,
                                           psa_fwu_image_version_t *fwu_version)
{
    struct image_version image_ver;
    struct shared_data_tlv_entry tlv_entry;
    uint8_t *tlv_end;
    uint8_t *tlv_curr;
    uint16_t image_id;
    psa_status_t status;

    switch (component) {
    case FWU_COMPONENT_INDEX_BL2:
        fwu_version->build = 0;
        return get_tfm_bl2_image_version(mdata->boot_index, &fwu_version->major,
                                         &fwu_version->minor,
                                         &fwu_version->patch);

    case FWU_COMPONENT_INDEX_RSE_RUNTIME:
        image_id = RSE_FIRMWARE_SECURE_ID;
        break;

    case FWU_COMPONENT_INDEX_SI_CL0:
        image_id = RSE_FIRMWARE_SI_CL0_ID;
        break;

    case FWU_COMPONENT_INDEX_AP_FIP_IMAGE:
        image_id = RSE_FIRMWARE_AP_BL2_ID;
        break;

    case FWU_COMPONENT_INDEX_SI_CL1:
        image_id = RSE_FIRMWARE_SI_CL1_ID;
        break;

    default:
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    /* The bootloader writes the image version information into the memory which
     * is shared between MCUboot and TF-M. Read the shared memory.
     */
    if (boot_shared_data.header.tlv_magic != SHARED_DATA_TLV_INFO_MAGIC) {
        return PSA_ERROR_DATA_CORRUPT;
    }

    tlv_end = (uint8_t *)&boot_shared_data +
                boot_shared_data.header.tlv_tot_len;
    tlv_curr = boot_shared_data.data;

    while (tlv_curr < tlv_end) {
        (void)memcpy(&tlv_entry, tlv_curr, SHARED_DATA_ENTRY_HEADER_SIZE);

        if ((GET_FWU_CLAIM(tlv_entry.tlv_type) == SW_VERSION) &&
            (GET_FWU_MODULE(tlv_entry.tlv_type) == image_id)) {

            if (tlv_entry.tlv_len != sizeof(struct image_version)) {
                return PSA_ERROR_DATA_CORRUPT;
            }

            memcpy(&image_ver, tlv_curr + SHARED_DATA_ENTRY_HEADER_SIZE,
                   tlv_entry.tlv_len);
            fwu_version->major = image_ver.iv_major;
            fwu_version->minor = image_ver.iv_minor;
            fwu_version->patch = image_ver.iv_revision;
            fwu_version->build = 0;

            return PSA_SUCCESS;
        }

        tlv_curr += SHARED_DATA_ENTRY_HEADER_SIZE + tlv_entry.tlv_len;
    }

    return PSA_ERROR_DATA_CORRUPT;
}

static psa_status_t retrieve_bl2_img_header_ver(const struct image_header *header,
                                                uint32_t *version)
{
    if (header->ih_magic != IMAGE_MAGIC) {
        return PSA_ERROR_DATA_CORRUPT;
    }

    *version = CONVERT_FWU_VERSION(header->ih_ver.iv_major,
                                   header->ih_ver.iv_minor,
                                   header->ih_ver.iv_revision);

    return PSA_SUCCESS;
}

static psa_status_t get_update_image_version(const struct fwu_private_metadata *mdata,
                                             psa_fwu_component_t component,
                                             uint32_t *version)
{
    const struct fwu_image_location *image_bank;
    struct image_header header;
    psa_status_t status;
    uintptr_t offset;

    /*
     * Read the offset of images in the update bank.
     * This part could be optimized based on GPT parsing later.
    */
    image_bank = fwu_get_image_location(component);

    offset = image_bank->partition_offset[mdata->boot_index ^ 1];

    status = fwu_flash_read(image_bank->flash, offset, (void *)&header,
                            sizeof(header));
    if (status != PSA_SUCCESS) {
        return status;
    }

    return retrieve_bl2_img_header_ver(&header, version);
}

static psa_status_t get_ap_fip_update_image_version(
                                       const struct fwu_private_metadata *mdata,
                                       uint32_t *version)
{
    const struct fwu_image_location *image_bank;
    struct image_header header;
    enum tfm_plat_err_t result;
    psa_status_t status;
    uint32_t fip_offset = 0;
    uint64_t tfa_bl2_offset;
    size_t tfa_bl2_size = 0;

    image_bank = fwu_get_image_location(FWU_COMPONENT_INDEX_AP_FIP_IMAGE);

    /* Fetch the offset of update bank */
    if (mdata->boot_index == FWU_BANK_0) {
        fip_offset = AP_FLASH_AREA_1_OFFSET;
    } else {
        fip_offset = AP_FLASH_AREA_0_OFFSET;
    }

    INIT_ATU_REGION_FOR_AP_FLASH();

    /* Locate the AP BL2 inside AP FIP */
    result = fip_get_entry_by_uuid(image_bank->flash,
                                   fip_offset, AP_FLASH_FIP_SIZE,
                                   UUID_TRUSTED_BOOT_FIRMWARE_BL2,
                                   &tfa_bl2_offset, &tfa_bl2_size);

    DEINIT_ATU_REGION_FOR_AP_FLASH();

    if (result != TFM_PLAT_ERR_SUCCESS) {
        return PSA_ERROR_DOES_NOT_EXIST;
    }

    status = fwu_flash_read(image_bank->flash, tfa_bl2_offset + fip_offset,
                            (void *)&header, sizeof(header));
    if (status != PSA_SUCCESS) {
        return status;
    }

    return retrieve_bl2_img_header_ver(&header, version);
}

psa_status_t esrt_update_last_attempt(struct fwu_private_metadata *mdata,
                                      psa_fwu_component_t component)

{
    psa_status_t status = PSA_SUCCESS;
    uint32_t version;

    switch (component) {
    case FWU_COMPONENT_INDEX_BL2:
        status = get_tfm_bl2_update_image_version(mdata, &version);
        break;

    case FWU_COMPONENT_INDEX_RSE_RUNTIME:
    case FWU_COMPONENT_INDEX_SI_CL0:
    case FWU_COMPONENT_INDEX_SI_CL1:
        status = get_update_image_version(mdata, component, &version);
        break;

    case FWU_COMPONENT_INDEX_AP_FIP_IMAGE:
        status = get_ap_fip_update_image_version(mdata, &version);
        break;

    default:
        status = PSA_ERROR_INVALID_ARGUMENT;
        break;
    }

    if (status != PSA_SUCCESS) {
        return status;
    }

    mdata->esrt_entries[component].last_attempt_version = version;

    if (version < mdata->esrt_entries[component].lowest_supported_fw_version) {
        mdata->esrt_entries[component].last_attempt_status =
                                    LAST_ATTEMPT_STATUS_ERROR_INCORRECT_VERSION;
        return PSA_ERROR_GENERIC_ERROR;
    }

    mdata->esrt_entries[component].last_attempt_status =
                                        LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;

    return PSA_SUCCESS;
}

/* Get shared bootloader data */
psa_status_t esrt_init_boot_data(void)
{
    return tfm_core_get_boot_data(TLV_MAJOR_FWU,
                                  (struct tfm_boot_data *)&boot_shared_data,
                                  sizeof(boot_shared_data));
}
