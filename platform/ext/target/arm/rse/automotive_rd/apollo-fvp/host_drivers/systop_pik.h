/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 */

#ifndef __SYSTOP_PIK_H__
#define __SYSTOP_PIK_H__

#include <stdint.h>

#include "tfm_hal_device_header.h"

/* CLKFORCE_SET bit positions */
#define SISYSCLK_FORCE_SET_SHIFT  27U

typedef struct systop_pik_type {
    const uint8_t reserved0[0x800];      /* 0x000 */
    __IOM uint32_t core_ctrl[16];        /* 0x800-0x83C */
    __IOM uint32_t sysclk_ctrl;          /* 0x840 */
    const uint8_t reserved1[12];         /* 0x844 */
    __IOM uint32_t dsu_periphclk_ctrl;   /* 0x850 */
    __IOM uint32_t dsu_atclk_ctrl;       /* 0x854 */
    __IOM uint32_t dsu_gicclk_ctrl;      /* 0x858 */
    __IOM uint32_t dsu_pclk_ctrl;        /* 0x85C */
    __IOM uint32_t dsu_ppuclk_ctrl;      /* 0x860 */
    const uint8_t reserved2[12];         /* 0x864 */
    __IOM uint32_t periphclk_aclk_ctrl;  /* 0x870 */
    __IOM uint32_t periphclk_pclk_ctrl;  /* 0x874 */
    const uint8_t reserved3[8];          /* 0x878 */
    __IOM uint32_t gicclk_ctrl;          /* 0x880 */
    const uint8_t reserved4[12];         /* 0x884 */
    __IOM uint32_t ioclk_ctrl;           /* 0x890 */
    const uint8_t reserved5[12];         /* 0x894 */
    __IOM uint32_t rseclk_ctrl;          /* 0x8A0 */
    const uint8_t reserved6[12];         /* 0x8A4 */
    __IOM uint32_t sisysclk_ctrl;        /* 0x8B0 */
    const uint8_t reserved7[12];         /* 0x8B4 */
    __IOM uint32_t sicpu_ctrl;           /* 0x8C0 */
    const uint8_t reserved8[12];         /* 0x8C4 */
    __IOM uint32_t smdperi_aclk_ctrl;    /* 0x8D0 */
    __IOM uint32_t smdperi_pclk_ctrl;    /* 0x8D4 */
    const uint8_t reserved9[0x128];      /* 0x8D8 */
    __IOM uint32_t clkforce_status;      /* 0xA00 */
    __IOM uint32_t clkforce_set;         /* 0xA04 */
    __IOM uint32_t clkforce_clr;         /* 0xA08 */
    const uint8_t reserved10[0x1F4];     /* 0xA0C */
    __IM  uint32_t sys_pwr_req_st;       /* 0xC00 */
    __IOM uint32_t sys_pwr_ack;          /* 0xC04 */
    __IM  uint32_t sys_rst_req_st;       /* 0xC08 */
    __IOM uint32_t sys_rst_ack;          /* 0xC0C */
    const uint8_t reserved11[0x1F0];     /* 0xC10 */
    __IOM uint32_t tri_redt_intr;        /* 0xE00 */
    const uint8_t reserved12[0x1BC];     /* 0xE04 */
    __IOM uint32_t pik_config;           /* 0xFC0 */
    const uint8_t reserved13[12];        /* 0xFC4 */
    __IM  uint32_t sid_pid_4;            /* 0xFD0 */
    const uint8_t reserved14[12];        /* 0xFD4 */
    __IM  uint32_t sid_pid_0;            /* 0xFE0 */
    __IM  uint32_t sid_pid_1;            /* 0xFE4 */
    __IM  uint32_t sid_pid_2;            /* 0xFE8 */
    __IM  uint32_t sid_pid_3;            /* 0xFEC */
    __IM  uint32_t compid0;              /* 0xFF0 */
    __IM  uint32_t compid1;              /* 0xFF4 */
    __IM  uint32_t compid2;              /* 0xFF8 */
    __IM  uint32_t compid3;              /* 0xFFC */
} systop_pik_t;

struct systop_pik_dev_t {
    /* Base address of SYSTOP PIK registers */
    volatile uintptr_t *pik_base;
};

#endif /* __SYSTOP_PIK_H__ */
