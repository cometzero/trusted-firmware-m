/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "tfm_hal_device_header.h"
#include "common_target_cfg.h"
#include "tfm_hal_platform.h"
#include "tfm_peripherals_def.h"
#include "uart_stdout.h"
#include "device_definition.h"
#include "rse_clocks.h"
#include "rse_sam_config.h"
#include "atu_config.h"
#include "atu_rse_lib.h"

#ifdef RSE_GPIO_SELF_TEST
#include "gpio_pl061_drv.h"
#include "tfm_log.h"
#include <stdbool.h>

#define RSE_GPIO_SELF_TEST_MASK       (1U << 7)
#define RSE_GPIO_SELF_TEST_PERIPH_ID  0x00041061U
#define RSE_GPIO_SELF_TEST_CELL_ID    0xB105F00DU

static bool rse_gpio_self_test_device(pl061_regblk_t *gpio)
{
    bool passed;

    if ((pl061_get_perifid(gpio) != RSE_GPIO_SELF_TEST_PERIPH_ID) ||
        (pl061_get_cellid(gpio) != RSE_GPIO_SELF_TEST_CELL_ID)) {
        return false;
    }

    pl061_set_output(gpio, RSE_GPIO_SELF_TEST_MASK);
    pl061_set_gpio(gpio, RSE_GPIO_SELF_TEST_MASK, RSE_GPIO_SELF_TEST_MASK);
    passed = pl061_get_gpio(gpio, RSE_GPIO_SELF_TEST_MASK) ==
             RSE_GPIO_SELF_TEST_MASK;

    gpio_set_low(gpio, RSE_GPIO_SELF_TEST_MASK);
    passed = passed &&
             (pl061_get_gpio(gpio, RSE_GPIO_SELF_TEST_MASK) == 0U);
    pl061_set_input(gpio, RSE_GPIO_SELF_TEST_MASK);

    return passed;
}
#endif

#ifdef TFM_PARTITION_PROTECTED_STORAGE
#define RSE_ATU_REGION_PS_SLOT  1
#endif /* TFM_PARTITION_PROTECTED_STORAGE */

extern const struct memory_region_limits memory_regions;

enum tfm_hal_status_t tfm_hal_platform_init(void)
{
    enum atu_error_t err;
    enum tfm_plat_err_t plat_err = TFM_PLAT_ERR_SYSTEM_ERR;

    if (rse_clock_config() != 0) {
        return TFM_HAL_ERROR_GENERIC;
    }
    SystemCoreClockUpdate();

    plat_err = enable_fault_handlers();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        return TFM_HAL_ERROR_GENERIC;
    }

    plat_err = system_reset_cfg();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        return TFM_HAL_ERROR_GENERIC;
    }

    plat_err = init_debug();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        return TFM_HAL_ERROR_GENERIC;
    }

    err = atu_rse_drv_init(&ATU_LIB_S, &ATU_DEV_S, ATU_DOMAIN_ROOT,
                           atu_regions_static, atu_stat_count);
    if (err != ATU_ERR_NONE) {
        return err;
    }

    __enable_irq();
    stdio_init();

#ifdef RSE_GPIO_SELF_TEST
    if (!rse_gpio_self_test_device(GPIO0_DEV_S) ||
        !rse_gpio_self_test_device(GPIO1_DEV_S)) {
        ERROR("RSE GPIO self-test... FAILED\r\n");
        return TFM_HAL_ERROR_GENERIC;
    }
    INFO("RSE GPIO self-test... PASSED\r\n");
#endif

    plat_err = rse_sam_init(RSE_SAM_INIT_SETUP_HANDLERS_ONLY);
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        return TFM_HAL_ERROR_GENERIC;
    }

    plat_err = nvic_interrupt_target_state_cfg();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        return TFM_HAL_ERROR_GENERIC;
    }

    plat_err = nvic_interrupt_enable();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        return TFM_HAL_ERROR_GENERIC;
    }

    plat_err = dma_init_cfg();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        return TFM_HAL_ERROR_GENERIC;
    }

    return TFM_HAL_SUCCESS;
}

uint32_t tfm_hal_get_ns_VTOR(void)
{
#ifndef TFM_LOAD_NS_IMAGE
    /* If an NS image hasn't been set up, then just return 0 */
    return 0;
#endif

    return memory_regions.non_secure_code_start;
}

uint32_t tfm_hal_get_ns_MSP(void)
{
#ifndef TFM_LOAD_NS_IMAGE
    /* If an NS image hasn't been set up, then just return 0 */
    return 0;
#endif

    return *((uint32_t *)memory_regions.non_secure_code_start);
}

uint32_t tfm_hal_get_ns_entry_point(void)
{
#ifndef TFM_LOAD_NS_IMAGE
    /* If an NS image hasn't been set up, then just return 0 */
    return 0;
#endif

    return *((uint32_t *)(memory_regions.non_secure_code_start + 4));
}
