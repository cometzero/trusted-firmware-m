/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 */

#include "ppu_drv.h"

#include <stddef.h>
#include <stdint.h>

#include "tfm_hal_device_header.h"

typedef union ppu_pwpr_type{
    struct{
        uint32_t pwrpolicy: 4;
        uint32_t reserved: 4;
        uint32_t pwr_dyn_enable: 1;
        uint32_t reserved1: 3;
        uint32_t lock_en: 1;
        uint32_t reserved2: 19;
    }bits;
    uint32_t byte;
}ppu_pwpr_t;

typedef union ppu_pmer_type{
    struct{
        uint32_t emu_enable: 1;
        uint32_t reserved: 31;
    }bits;
    uint32_t byte;
}ppu_pmer_t;

typedef union ppu_pwsr_type{
    struct{
        uint32_t pwr_status: 4;
        uint32_t reserved: 4;
        uint32_t pwr_dyn_enable_status: 1;
        uint32_t reserved1: 3;
        uint32_t lock_en_status: 1;
        uint32_t reserved2: 19;
    }bits;
    uint32_t byte;
}ppu_pwsr_t;

struct _ppu {
    __IOM ppu_pwpr_t ppu_pwpr; /* 0x0 */
    __IOM ppu_pmer_t ppu_pmer; /* 0x4 */
    __IM  ppu_pwsr_t ppu_pwsr; /* 0x8 */
    const uint8_t reserved0[0x1000 - 0xC];
};

struct _cluster_safety {
    const uint8_t reserved0[0x60];
    __OM uint32_t cluster_safety_key;
};

#define PPU_PWPR_PWR_POLICY_ON              (0x8)
#define PPU_PWPR_PWR_POLICY_OFF             (0x0)
#define PPU_PWPR_PWR_POLICY_FULL_RET        (0x5)

#define PPU_PWSR_PWR_STATUS_ON              (0x8)
#define PPU_PWSR_PWR_STATUS_OFF             (0x0)
#define PPU_PWSR_PWR_STATUS_FULL_RET        (0x5)

#define CLUSTER_SAFETY_KEY_VALUE     (0x000000BA)

enum ppu_error_t ppu_drv_cfg_power_policy(const struct ppu_dev_t *dev,
                                          ppu_power_mode_policy_t pwr_policy)
{
    if (dev == NULL) {
        return PPU_ERR_INVALID_PARAM;
    }

    struct _ppu *ppu = (struct _ppu *)dev->ppu_base;
    struct _cluster_safety *cluster_safety = (struct _cluster_safety *)dev->cluster_safety_base;

    if (cluster_safety) {
        /* Unlock PPU registers write operation. */
        cluster_safety->cluster_safety_key = CLUSTER_SAFETY_KEY_VALUE;
    }

    switch(pwr_policy){
    case PPU_PWR_POLICY_ON:
        ppu->ppu_pwpr.bits.pwrpolicy = PPU_PWPR_PWR_POLICY_ON;
        while (ppu->ppu_pwsr.bits.pwr_status != PPU_PWSR_PWR_STATUS_ON) {};
        break;
    case PPU_PWR_POLICY_OFF:
        ppu->ppu_pwpr.bits.pwrpolicy = PPU_PWPR_PWR_POLICY_OFF;
        while (ppu->ppu_pwsr.bits.pwr_status != PPU_PWSR_PWR_STATUS_OFF) {};
        break;
    case PPU_PWR_POLICY_FULL_RET:
        ppu->ppu_pwpr.bits.pwrpolicy = PPU_PWPR_PWR_POLICY_FULL_RET;
        while (ppu->ppu_pwsr.bits.pwr_status !=
               PPU_PWPR_PWR_POLICY_FULL_RET) {};
        break;
    default:
        break;
    }

    return PPU_ERR_NONE;
}
