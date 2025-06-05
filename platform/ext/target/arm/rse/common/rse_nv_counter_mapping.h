/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "tfm_plat_otp.h"

#ifndef __RSE_NV_COUNTER_MAPPING_H__
#define __RSE_NV_COUNTER_MAPPING_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum rse_nv_counter_bank_0_mapping_t {
    BANK_0_COUNTER_BL1 = 0,

    BANK_0_COUNTER_BL2,
    BANK_0_COUNTER_BL2_MAX = BANK_0_COUNTER_BL2 + RSE_NV_COUNTER_BL2_AMOUNT,

    BANK_0_COUNTER_PS = BANK_0_COUNTER_BL2_MAX,
    /*
     * PS NV counters are longer to support more writes.
     * Each PS NV counter occupies multiple NV counter slots.
     */
    BANK_0_COUNTER_PS_MAX = BANK_0_COUNTER_PS + \
                            RSE_NV_COUNTER_PS_AMOUNT * RSE_NV_COUNTER_PS_LENGTH_MULTIPLIER,

    BANK_0_COUNTER_HOST = BANK_0_COUNTER_PS_MAX,
    BANK_0_COUNTER_HOST_MAX = BANK_0_COUNTER_HOST + RSE_NV_COUNTER_HOST_AMOUNT,

    BANK_0_COUNTER_SUBPLATFORM = BANK_0_COUNTER_HOST_MAX,
    BANK_0_COUNTER_SUBPLATFORM_MAX = BANK_0_COUNTER_SUBPLATFORM + RSE_NV_COUNTER_SUBPLATFORM_AMOUNT,

    BANK_0_COUNTER_MAX = BANK_0_COUNTER_SUBPLATFORM_MAX,
};

enum rse_nv_counter_bank_1_mapping_t {
    BANK_1_COUNTER_MAX,
};

enum rse_nv_counter_bank_2_mapping_t {
    BANK_2_COUNTER_MAX,
};

enum rse_nv_counter_bank_3_mapping_t {
    BANK_3_COUNTER_MAX,
};

enum tfm_otp_element_id_t rse_get_bl1_counter(uint32_t image_id);
enum tfm_otp_element_id_t rse_get_bl2_counter(uint32_t image_id);
enum tfm_otp_element_id_t rse_get_ps_counter(uint32_t image_id);
enum tfm_otp_element_id_t rse_get_host_counter(uint32_t host_counter_id);
enum tfm_otp_element_id_t rse_get_subplatform_counter(uint32_t host_counter_id);

#ifdef __cplusplus
}
#endif

#endif /* __RSE_NV_COUNTER_MAPPING_H__ */
