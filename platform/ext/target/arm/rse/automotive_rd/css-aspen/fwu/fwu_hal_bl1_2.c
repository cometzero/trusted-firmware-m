/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "fih.h"
#include "flash_layout.h"
#include "fwu_private_metadata.h"
#include "psa/error.h"
#include "psa/update.h"

#include <stdint.h>

#define IMAGE_ID_INVALID    (0xFFFF)

uint32_t bl1_2_select_image(void)
{
    struct fwu_private_metadata private_metadata = {0};
    psa_status_t status;

    status = fwu_private_metadata_read(&private_metadata);
    if (status != PSA_SUCCESS) {
        FIH_PANIC;
    }

    /*
     * Transition TF-M BL2 to PSA_FWU_TRIAL
     * only if the current state is PSA_FWU_STAGED
     */
    if (private_metadata.fwu_image_state[FWU_COMPONENT_INDEX_BL2] == PSA_FWU_STAGED) {
        private_metadata.fwu_image_state[FWU_COMPONENT_INDEX_BL2] = PSA_FWU_TRIAL;
        status = fwu_private_metadata_write(&private_metadata);
        if (status != PSA_SUCCESS) {
            FIH_PANIC;
        }
    }

    return private_metadata.boot_index;
}

uint32_t bl1_2_rollback_image(void)
{
    struct fwu_private_metadata private_metadata = {0};
    psa_status_t status;

    status = fwu_private_metadata_read(&private_metadata);
    if (status != PSA_SUCCESS) {
        FIH_PANIC;
    }

    /* Roll back only if TF-M BL2 is in PSA_FWU_READY state */
    private_metadata.boot_index ^= 1;
    status = fwu_private_metadata_write(&private_metadata);
    if (status != PSA_SUCCESS) {
        FIH_PANIC;
    }

    return private_metadata.boot_index;
}
