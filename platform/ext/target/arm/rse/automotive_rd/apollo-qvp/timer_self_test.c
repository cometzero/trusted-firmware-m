/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "timer_self_test.h"

#include "platform_base_address.h"
#include "systimer_armv8-m_reg_map.h"
#include "system_core_init.h"
#include "tfm_hal_device_header.h"
#include "tfm_log.h"

#include <stdbool.h>
#include <stdint.h>

#define APOLLO_TIMER_COUNTER_HZ  UINT32_C(125000000)
#define APOLLO_TIMER_TEST_TICKS  UINT32_C(1250)
#define APOLLO_TIMER_TEST_SPINS  UINT32_C(1000000)
#define APOLLO_SYSTICK_CORE_HZ   UINT32_C(100000000)

#define APOLLO_TIMER_CTL_ENABLE  (1U << 0)
#define APOLLO_TIMER_CTL_STATUS  (1U << 2)

struct apollo_timer_desc {
    const char *name;
    uintptr_t base;
    uint32_t irq;
};

static const struct apollo_timer_desc timers[] = {
    {"TIMER0", SYSTIMER0_ARMV8_M_BASE_S, TIMER0_IRQn},
    {"TIMER1", SYSTIMER1_ARMV8_M_BASE_S, TIMER1_IRQn},
    {"TIMER2", SYSTIMER2_ARMV8_M_BASE_S, TIMER2_IRQn},
    {"TIMER3", SYSTIMER3_ARMV8_M_BASE_S, TIMER3_AON_IRQn},
};

static uint32_t timer_counter_low(struct cnt_base_reg_map_t *regs)
{
    return regs->cntpct_low;
}

static bool test_systick(const char *stage)
{
    uint32_t start;
    uint32_t end = 0U;
    uint32_t spins;
    bool counted = false;
    bool expired = false;
    bool passed;

    SysTick->CTRL = 0U;
    SysTick->LOAD = 999U;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    start = SysTick->VAL;

    for (spins = 0U; spins < APOLLO_TIMER_TEST_SPINS; ++spins) {
        end = SysTick->VAL;
        counted = counted || (end != start);
        if ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0U) {
            expired = true;
            break;
        }
    }

    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;
    passed = counted && expired &&
             (SystemCoreClock == APOLLO_SYSTICK_CORE_HZ);

    INFO("APOLLO_TIMER_TEST stage=%s timer=SysTick result=%s "
         "start=%lu end=%lu load=999 core_hz=%lu spins=%lu\r\n",
         stage, passed ? "PASS" : "FAIL", (unsigned long)start,
         (unsigned long)end, (unsigned long)SystemCoreClock,
         (unsigned long)spins);

    return passed;
}

static bool test_timer(const char *stage,
                       const struct apollo_timer_desc *timer)
{
    struct cnt_base_reg_map_t *regs =
        (struct cnt_base_reg_map_t *)timer->base;
    const uint32_t irq_mask = 1U << timer->irq;
    const bool was_enabled = (NVIC->ISER[0] & irq_mask) != 0U;
    const uint32_t saved_frequency = regs->cntfrq;
    uint32_t start;
    uint32_t end;
    uint32_t observed_frequency;
    uint32_t spins;
    bool expired = false;
    bool pending = false;
    bool passed;

    NVIC->ICER[0] = irq_mask;
    NVIC->ICPR[0] = irq_mask;
    regs->cntp_ctl = 0U;
    regs->cntfrq = APOLLO_TIMER_COUNTER_HZ;
    start = timer_counter_low(regs);
    regs->cntp_tval = APOLLO_TIMER_TEST_TICKS;
    regs->cntp_ctl = APOLLO_TIMER_CTL_ENABLE;

    for (spins = 0U; spins < APOLLO_TIMER_TEST_SPINS; ++spins) {
        expired = (regs->cntp_ctl & APOLLO_TIMER_CTL_STATUS) != 0U;
        pending = (NVIC->ISPR[0] & irq_mask) != 0U;
        if (expired && pending) {
            break;
        }
    }

    end = timer_counter_low(regs);
    observed_frequency = regs->cntfrq;
    passed = expired && pending && (end != start) &&
             (observed_frequency == APOLLO_TIMER_COUNTER_HZ);

    regs->cntp_ctl = 0U;
    regs->cntp_cval_low = 0U;
    regs->cntp_cval_high = 0U;
    regs->cntfrq = saved_frequency;
    NVIC->ICPR[0] = irq_mask;
    if (was_enabled) {
        NVIC->ISER[0] = irq_mask;
    }

    INFO("APOLLO_TIMER_TEST stage=%s timer=%s result=%s "
         "start=%lu end=%lu freq=%lu pending=%lu spins=%lu\r\n",
         stage, timer->name, passed ? "PASS" : "FAIL",
         (unsigned long)start, (unsigned long)end,
         (unsigned long)observed_frequency, (unsigned long)pending,
         (unsigned long)spins);

    return passed;
}

int apollo_timer_self_test(const char *stage)
{
    bool passed = test_systick(stage);
    uint32_t idx;

    for (idx = 0U; idx < (sizeof(timers) / sizeof(timers[0])); ++idx) {
        passed = test_timer(stage, &timers[idx]) && passed;
    }

    return passed ? 0 : -1;
}
