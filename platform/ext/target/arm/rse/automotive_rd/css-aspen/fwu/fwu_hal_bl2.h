/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __FWU_HAL_BL2_H_
#define __FWU_HAL_BL2_H_

#include "flash_layout.h"

#include <stdint.h>

#ifdef TFM_PARTITION_FIRMWARE_UPDATE

/* Handles the Firmware update state transitions for TF-Runtime, SCP firmware
 * AP BL2 and handles MCUBoot image slots as per FWU
 */
int32_t fwu_hal_bl2_update_state_and_flashmap(void);

/* Returns the active boot index from private metadata */
uint8_t get_active_boot_index(void);
#else /* TFM_PARTITION_FIRMWARE_UPDATE */
static int32_t fwu_hal_bl2_update_state_and_flashmap(void)
{
    return 0;
}

static uint8_t get_active_boot_index(void)
{
    return FWU_BANK_0;
}
#endif /* TFM_PARTITION_FIRMWARE_UPDATE */
#endif /* __FWU_HAL_BL2_H_ */
