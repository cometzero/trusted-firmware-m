/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "bl2_image_id.h"
#include "Driver_Flash.h"
#include "fih.h"
#include "flash_layout.h"
#include "flash_map/flash_map.h"
#include "fwu_flash.h"
#include "fwu_private_metadata.h"
#include "fwu_hal_bl2.h"
#include "psa/error.h"
#include "psa/update.h"

#include <stdint.h>

extern struct flash_area flash_map[];
extern const int flash_map_entry_num;

int32_t fwu_hal_bl2_update_state_and_flashmap(void)
{
    struct fwu_private_metadata private_metadata = {0};
    uint32_t secure_image_offset;
    uint32_t secure_image_size;
    uint32_t scp_image_offset;
    uint32_t scp_image_size;
    uint32_t si_cl1_image_offset;
    uint32_t si_cl1_image_size;
    bool transition = false;

    if (fwu_private_metadata_read(&private_metadata) != PSA_SUCCESS) {
        return 1;
    }

    for (psa_fwu_component_t component = FWU_COMPONENT_INDEX_RSE_RUNTIME;
         component < FWU_COMPONENT_NUMBER; component++) {
        if (private_metadata.fwu_image_state[component] == PSA_FWU_STAGED) {
            /*
             * Transition to PSA_FWU_TRIAL state
             * only if current state is PSA_FWU_STAGED
             */
            private_metadata.fwu_image_state[component] = PSA_FWU_TRIAL;
            transition = true;
        }
    }

    if (transition &&
        (fwu_private_metadata_write(&private_metadata) != PSA_SUCCESS)) {
        return 1;
    }

    if (private_metadata.boot_index == FWU_BANK_0) {
        /* For TF-M Runtime and Safety Island images, set flash_map[] both
         * slot offsets to Primary Slot offset
         */
        secure_image_offset = FLASH_AREA_2_OFFSET;
        secure_image_size = FLASH_AREA_2_SIZE;
        scp_image_offset = FLASH_AREA_4_OFFSET;
        scp_image_size = FLASH_AREA_4_SIZE;
        si_cl1_image_offset = FLASH_AREA_6_OFFSET;
        si_cl1_image_size = FLASH_AREA_6_SIZE;
    } else {
        /* For TF-M Runtime and Safety Island images, set flash_map[]
         * both slot offsets to Secondary Slot offset
         */
        secure_image_offset = FLASH_AREA_3_OFFSET;
        secure_image_size = FLASH_AREA_3_SIZE;
        scp_image_offset = FLASH_AREA_5_OFFSET;
        scp_image_size = FLASH_AREA_5_SIZE;
        si_cl1_image_offset = FLASH_AREA_7_OFFSET;
        si_cl1_image_size = FLASH_AREA_7_SIZE;
    }

    /*
     * Workaround MCUboot image slots loading
     * Note: MCUboot doesn't export an interface to platform port to select
     * the active bank. Instead, MCUboot always reads both Primary slot and
     * Secondary slot and selects the newer image
     */
    for (uint8_t idx = 0; idx < flash_map_entry_num; idx++) {
        switch (flash_map[idx].fa_id) {
            case FLASH_AREA_IMAGE_PRIMARY(RSE_FIRMWARE_SECURE_ID):
            case FLASH_AREA_IMAGE_SECONDARY(RSE_FIRMWARE_SECURE_ID):
                flash_map[idx].fa_off = secure_image_offset;
                flash_map[idx].fa_size = secure_image_size;
                break;
            case FLASH_AREA_IMAGE_PRIMARY(RSE_FIRMWARE_SI_CL0_ID):
            case FLASH_AREA_IMAGE_SECONDARY(RSE_FIRMWARE_SI_CL0_ID):
                flash_map[idx].fa_off = scp_image_offset;
                flash_map[idx].fa_size = scp_image_size;
                break;
            case FLASH_AREA_IMAGE_PRIMARY(RSE_FIRMWARE_SI_CL1_ID):
            case FLASH_AREA_IMAGE_SECONDARY(RSE_FIRMWARE_SI_CL1_ID):
                flash_map[idx].fa_off = si_cl1_image_offset;
                flash_map[idx].fa_size =si_cl1_image_size;
                break;
        }
    }

    return 0;
}

uint8_t get_active_boot_index(void)
{
    struct fwu_private_metadata private_metadata = {0};

    if (fwu_private_metadata_read(&private_metadata) != PSA_SUCCESS) {
        FIH_PANIC;
    }

    return private_metadata.boot_index;
}
