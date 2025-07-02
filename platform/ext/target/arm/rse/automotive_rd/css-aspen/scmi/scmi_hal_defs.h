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
#define SI_SHARED_MEMORY_SIZE 0x100ULL

#define SI_COMMAND_MEMORY_BASE  SI_SHARED_MEMORY_BASE

/* Doorbell channel for communicating with SI CL0 */
#define SI_MHU_DOORBELL_CHANNEL 0UL

#define SI_MHU_COMMAND_PBX_FLAG  (1UL << 1U) /* 1st bit, value == 2 */
#define SI_MHU_COMMAND_MBX_FLAG  (1UL << 1U)

#define SCMI_MHU_DOORBELL_VALUE_INVALID 0x0UL

#ifdef SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS
#define SI_NOTIFIER_MEMORY_BASE (SI_COMMAND_MEMORY_BASE + SI_SHARED_MEMORY_SIZE)

#define SI_MHU_NOTIFIER_PBX_FLAG (1UL << 2U) /* 2nd bit, value == 4 */
#define SI_MHU_NOTIFIER_MBX_FLAG (1UL << 2U)

#define SCP_SHARED_MEMORY_BASE SI_SHARED_MEMORY_BASE
#define SCP_SHARED_MEMORY_SIZE SI_SHARED_MEMORY_SIZE
#endif /* SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS */

#ifdef __cplusplus
}
#endif

#endif /* __SCMI_HAL_DEFS_H__ */
