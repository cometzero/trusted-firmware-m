/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 */

#ifndef __SCMI_HAL_DEFS_H__
#define __SCMI_HAL_DEFS_H__

#include "host_atu_base_address.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Base address and size of shared memory with SI for SCMI transport */
#define SI_SHARED_MEMORY_BASE HOST_RSE_SI_SSRAM_ATU_BASE_S
#define SI_SHARED_MEMORY_SIZE 0x80ULL

/* Doorbell channel for communicating with SI CL0 */
#define SI_MHU_DOORBELL_CHANNEL 0
#define SI_MHU_PBX_FLAG         (1 << 1)   /* bit 1 */
#define SI_MHU_MBX_FLAG         (1 << 1)   /* bit 1 */

#ifdef __cplusplus
}
#endif

#endif /* __SCMI_HAL_DEFS_H__ */
