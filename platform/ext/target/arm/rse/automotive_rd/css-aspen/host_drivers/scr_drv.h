/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 */

#ifndef __SCR_DRV_H__
#define __SCR_DRV_H__

#include <stdbool.h>
#include <stdint.h>

#include "tfm_hal_device_header.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MEMPROTCTLR bit positions */
#define MEMPROTCTLR_LM0_MEMPROTEN_BIT   0U
#define MEMPROTCTLR_LM1_MEMPROTEN_BIT   1U
#define MEMPROTCTLR_SH1_MEMPROTEN_BIT   4U
#define MEMPROTCTLR_SH2_MEMPROTEN_BIT   5U

typedef union system_cfg_type{
    struct{
        uint32_t cl1_present: 1;
        uint32_t reserved0: 3;
        uint32_t lm0_size: 4;
        uint32_t lm1_size: 4;
        uint32_t reserved1: 20;
    }bits;
    uint32_t byte;
}system_cfg_t;

typedef union cpuhalt_type{
    struct{
        uint32_t cl0_c0_cpuhalt: 1;
        uint32_t reserved0: 7;
        uint32_t cl1_c0_cpuhalt: 1;
        uint32_t cl1_c1_cpuhalt: 1;
        uint32_t cl1_c2_cpuhalt: 1;
        uint32_t cl1_c3_cpuhalt: 1;
        uint32_t reserved1: 20;
    }bits;
    uint32_t byte;
}cpuhalt_t;

typedef struct scr_type {
    __IOM uint32_t cl0_config_0;        /* 0x0 */
    __IOM uint32_t cl0_config_1;        /* 0x4 */
    __IOM uint32_t cl0_config_2;        /* 0x8 */
    const uint8_t reserved0[4];         /* reserved 4 bytes */
    __IOM uint32_t cl0_c0_config_0;     /* 0x10 */
    __IOM uint32_t cl0_c0_config_1;     /* 0x14 */
    __IOM uint32_t cl0_c0_config_2;     /* 0x18 */
    __IOM uint32_t cl0_c0_config_3;     /* 0x1C */
    const uint8_t reserved1[32];        /* reserved 32 bytes */
    __IM  uint32_t sid_system_id;       /* 0x40 */
    const uint8_t reserved2[12];        /* reserved 12 bytes */
    __IM  uint32_t sid_soc_id;          /* 0x50 */
    const uint8_t reserved3[12];        /* reserved 12 bytes */
    __IM  uint32_t sid_chip_id;         /* 0x60 */
    const uint8_t reserved4[12];        /* reserved 12 bytes */
    __IM  system_cfg_t sid_system_cfg;  /* 0x70 */
    const uint8_t reserved5[652];       /* reserved 652 bytes */
    __IOM cpuhalt_t cpuhalt;            /* 0x300 */
    const uint8_t reserved6[508];       /* reserved 508 bytes */
    __IOM uint32_t memprotctlr;         /* 0x500 */
    const uint8_t reserved7[252];       /* reserved 252 bytes */
    __IOM uint32_t safectlr;            /* 0x600 */
    const uint8_t reserved8[2508];      /* reserved 2508 bytes */
    __IM  uint32_t sid_pidr4;           /* 0xFD0 */
    const uint8_t reserved9[12];        /* reserved 12 bytes */
    __IM  uint32_t sid_pidr0;           /* 0xFE0 */
    __IM  uint32_t sid_pidr1;           /* 0xFE4 */
    __IM  uint32_t sid_pidr2;           /* 0xFE8 */
    __IM  uint32_t sid_pidr3;           /* 0xFEC */
    __IM  uint32_t sid_cidr0;           /* 0xFF0 */
    __IM  uint32_t sid_cidr1;           /* 0xFF4 */
    __IM  uint32_t sid_cidr2;           /* 0xFF8 */
    __IM  uint32_t sid_cidr3;           /* 0xFFC */
}scr_t;

/* SCR device definition */
struct scr_dev_t {
    /* Base address of SCR registers */
    const uintptr_t scr_base;
};

bool scr_sid_is_cl1_present(const struct scr_dev_t *dev);

void scr_cfg_cpuhalt(const struct scr_dev_t *dev, bool halt_val);

#ifdef __cplusplus
}
#endif

#endif /* __SCR_DRV_H__ */
