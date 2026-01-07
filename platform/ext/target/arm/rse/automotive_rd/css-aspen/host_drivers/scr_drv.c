/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 */

#include "scr_drv.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tfm_hal_device_header.h"

/* Check if CL1 is present */
bool scr_sid_is_cl1_present(const struct scr_dev_t *dev)
{
    volatile scr_t* scr = (volatile scr_t*)(dev->scr_base);
    return ((scr->sid_system_cfg.bits.cl1_present) != 0U);
}

/* Halts cores in CL0 and CL1(if cl1 is present) */
void scr_cfg_cpuhalt(const struct scr_dev_t *dev, bool halt_val)
{
    volatile scr_t* scr = (volatile scr_t*)(dev->scr_base);

    /* configure cl0 cores */
    scr->cpuhalt.bits.cl0_c0_cpuhalt = halt_val;

    /* configure cl1 cores only if cl1 is present */
    if((scr->sid_system_cfg.bits.cl1_present) != 0U)
    {
        scr->cpuhalt.bits.cl1_c0_cpuhalt = halt_val;
        scr->cpuhalt.bits.cl1_c1_cpuhalt = halt_val;
        scr->cpuhalt.bits.cl1_c2_cpuhalt = halt_val;
        scr->cpuhalt.bits.cl1_c3_cpuhalt = halt_val;
    }
}
