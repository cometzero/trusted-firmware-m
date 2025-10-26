/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_FWU_IMPL_INFO_H__
#define __TFM_FWU_IMPL_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief The implementation-specific data in the component information
 *        structure.
 */
typedef struct __attribute__((__packed__)) {
    uint32_t fw_type;
    uint32_t lowest_supported_fw_version;
    uint32_t last_attempt_version;
    uint32_t last_attempt_status;
} psa_fwu_impl_info_t;

#ifdef __cplusplus
}
#endif

#endif /* TFM_FWU_IMPL_INFO_H */
