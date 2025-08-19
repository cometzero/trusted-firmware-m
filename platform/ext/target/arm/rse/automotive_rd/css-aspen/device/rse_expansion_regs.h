/*
* Copyright (c) 2025, Arm Limited. All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*
*/

#ifndef __RSE_EXPANSION_REGS_H__
#define __RSE_EXPANSION_REGS_H__

#include "tfm_hal_device_header.h"

#include <stdint.h>

__PACKED_STRUCT rse_integ_t {
    __IOM uint32_t rsecoreclk_ctrl; /* 0x000 */
    __IOM uint32_t rsecoreclk_div;  /* 0x004 */
    const uint32_t reserved0[2];
    __IOM uint32_t rse_integration; /* 0x010*/
    __IOM uint32_t atu_ap; /* 0x014 */
    const uint32_t reserved1[1005];
    __IM  uint32_t pidr4; /* 0xFD0 */
    const uint32_t reserved2[3];
    __IM  uint32_t pidr0; /* 0xFE0 */
    __IM  uint32_t pidr1; /* 0xFE4 */
    __IM  uint32_t pidr2; /* 0xFE8 */
    __IM  uint32_t pidr3; /* 0xFEC */
    __IM  uint32_t cidr0; /* 0xFF0 */
    __IM  uint32_t cidr1; /* 0xFF4 */
    __IM  uint32_t cidr2; /* 0xFF8 */
    __IM  uint32_t cidr3; /* 0xFFC */
};

/* Field definitions for RSECORECLK_CTRL register */
#define RSE_INTEG_CLKCTRL_SEL_POS      (0U)
#define RSE_INTEG_CLKCTRL_SEL_MSK      (0xFFUL << RSE_INTEG_CLKCTRL_SEL_POS)
#define RSE_INTEG_CLKCTRL_SEL_REF      (0x1UL << RSE_INTEG_CLKCTRL_SEL_POS)
#define RSE_INTEG_CLKCTRL_SEL_SPLL     (0x2UL << RSE_INTEG_CLKCTRL_SEL_POS)
#define RSE_INTEG_CLKCTRL_SEL_CUR_POS  (8U)
#define RSE_INTEG_CLKCTRL_SEL_CUR_MSK  (0xFFUL << RSE_INTEG_CLKCTRL_SEL_CUR_POS)
#define RSE_INTEG_CLKCTRL_SEL_CUR_REF  (0x1UL << RSE_INTEG_CLKCTRL_SEL_CUR_POS)
#define RSE_INTEG_CLKCTRL_SEL_CUR_SPLL (0x2UL << RSE_INTEG_CLKCTRL_SEL_CUR_POS)

/* Field definitions for RSECORECLK_DIV register */
#define RSE_INTEG_CLKDIV_DIV_POS       (0U)
#define RSE_INTEG_CLKDIV_DIV_MSK       (0xFUL << RSE_INTEG_CLKDIV_DIV_POS)
#define RSE_INTEG_CLKDIV_DIV_CUR_POS   (16U)
#define RSE_INTEG_CLKDIV_DIV_CUR_MSK   (0x4UL << RSE_INTEG_CLKDIV_DIV_CUR_POS)

#endif /* __RSE_EXPANSION_REGS_H__ */
