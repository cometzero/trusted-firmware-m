/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 */

#ifndef __PPU_DRV_H__
#define __PPU_DRV_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ppu_error_t {
    /* No Error */
    PPU_ERR_NONE,
    /* Invalid parameter */
    PPU_ERR_INVALID_PARAM,
    /* Error accessing PPU */
    PPU_ERR_ACCESS,
    /* General error with driver */
    PPU_ERR_GENERAL,
};

typedef enum ppu_power_mode_policy_type {
    /* Power mode off */
    PPU_PWR_POLICY_OFF,
    /* Power mode FULL_RET */
    PPU_PWR_POLICY_FULL_RET,
    /* Power mode on */
    PPU_PWR_POLICY_ON,
}ppu_power_mode_policy_t;

struct ppu_dev_t {
    /* Base address of the PPU registers */
    const uintptr_t ppu_base;

    /* Base address of the Cluster safety register */
    const uintptr_t cluster_safety_base;
};

enum ppu_error_t ppu_drv_cfg_power_policy(const struct ppu_dev_t *dev,
                                          ppu_power_mode_policy_t pwr_policy);

#ifdef __cplusplus
}
#endif

#endif /* __PPU_DRV_H__ */
