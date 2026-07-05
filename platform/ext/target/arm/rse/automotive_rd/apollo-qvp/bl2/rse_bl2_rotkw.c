/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "tfm_plat_otp.h"
#include "platform_otp_ids.h"
#include <bootutil/sign_key.h>
#include <bootutil/bootutil_log.h>
#include "rse_kmu_slot_ids.h"
#include "cc3xx_opaque_keys.h"
#include "bl2_image_id.h"

uint32_t get_enc_key_id_for_image(uint32_t image_id)
{
    uint32_t res;

    switch (image_id) {
        case RSE_FIRMWARE_SECURE_ID:
        case RSE_FIRMWARE_AP_BL2_ID:
        case RSE_FIRMWARE_SI_CL0_ID:
        case RSE_FIRMWARE_SI_CL1_ID:
            res = cc3xx_get_opaque_key(RSE_KMU_SLOT_SECURE_ENCRYPTION_KEY);
            break;
        default:
            res = CC3XX_OPAQUE_KEY_ID_INVALID;
            break;
    }

    return res;
}
