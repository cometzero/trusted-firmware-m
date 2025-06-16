/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 */

#include "device_definition.h"
#include "host_atu_base_address.h"
#include "scmi_hal.h"
#include "scmi_protocol.h"
#include "scmi_log.h"

#ifdef SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS
#include "config_tfm_target.h"
#include "device_definition.h"
#include "mhu_v3_x.h"
#include "tfm_hal_spm_logdev.h"
#include "tfm_hal_device_header.h"
#include "tfm_hal_platform.h"

#include <stdbool.h>
#include <string.h>
#endif

#ifdef SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS
static struct scmi_sram_page_config sram_page_table[] = {
    [SCMI_COMMAND_SRAM_PAGE] = {
        .base_addr = SI_COMMAND_MEMORY_BASE,
        .pbx_db_flag_bit = SI_MHU_COMMAND_PBX_FLAG,
        .mbx_db_flag_bit = SI_MHU_COMMAND_MBX_FLAG,
    },
    [SCMI_NOTIFIER_SRAM_PAGE] = {
        .base_addr = SI_NOTIFIER_MEMORY_BASE,
        .pbx_db_flag_bit = SI_MHU_NOTIFIER_PBX_FLAG,
        .mbx_db_flag_bit = SI_MHU_NOTIFIER_MBX_FLAG,
    }
};
#endif

scmi_comms_err_t scmi_hal_shared_memory_init(void)
{
#ifdef SCMI_COMMS_FOR_BL2_POLLING_MODE
    /*
     * ATU initalizes the shared memory region during boot-time. This
     * is only necessary for BL2.
     */
    enum atu_error_t err;

    err = atu_rse_initialize_region(&ATU_DEV_S,
                                    RSE_ATU_SI_SSRAM_ID,
                                    HOST_RSE_SI_SSRAM_ATU_BASE_S,
                                    HOST_RSE_SI_SSRAM_ATU_PHYS_BASE,
                                    HOST_RSE_SI_SSRAM_ATU_SIZE);
    if (err != ATU_ERR_NONE) {
        return SCMI_COMMS_HARDWARE_ERROR;
    }
#endif /* SCMI_COMMS_FOR_BL2_POLLING_MODE */

#ifdef SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS
    /*
     * Shared memory is divided into pages. Each page contains a
     * transport buffer struct initialized at runtime.
     */
    for (int i = 0; i < SCMI_SRAM_PAGE_COUNT; ++i) {
        sram_page_table[i].base_addr->flags = 0;
        sram_page_table[i].base_addr->length = 0;
        sram_page_table[i].base_addr->status = TRANSPORT_BUFFER_STATUS_FREE_MASK;
    }
#endif /* SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS */

    SCMI_LOG_INF("Shared memory initialized");

    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t scmi_hal_doorbell_init(void)
{
    enum mhu_v3_x_error_t mhuv3_err;
    uint8_t ch;
    uint8_t num_ch;

    /* Init sender */
    mhuv3_err = mhu_v3_x_driver_init(&MHU_RSE_TO_SI_CL0_DEV);
    if (mhuv3_err != MHU_V_3_X_ERR_NONE) {
        SCMI_LOG_ERR("RSE to SI CL0 MHU driver init failed: %d",
                     (int)mhuv3_err);
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    /* Read the number of doorbell channels implemented in the MHU Sender */
    mhuv3_err = mhu_v3_x_get_num_channel_implemented(&MHU_RSE_TO_SI_CL0_DEV,
                                                     MHU_V3_X_CHANNEL_TYPE_DBCH,
                                                     &num_ch);
    if (mhuv3_err != MHU_V_3_X_ERR_NONE) {
        SCMI_LOG_ERR("RSE to SI CL0 MHU get channels failed: %d",
                     (int)mhuv3_err);
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    /* Init receiver */
    mhuv3_err = mhu_v3_x_driver_init(&MHU_SI_CL0_TO_RSE_DEV);
    if (mhuv3_err != MHU_V_3_X_ERR_NONE) {
        SCMI_LOG_ERR("SI CL0 to RSE MHU driver init failed: %d",
                     (int)mhuv3_err);
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    /* Read the number of doorbell channels implemented in the MHU Receiver */
    mhuv3_err = mhu_v3_x_get_num_channel_implemented(&MHU_SI_CL0_TO_RSE_DEV,
                                                     MHU_V3_X_CHANNEL_TYPE_DBCH,
                                                     &num_ch);
    if (mhuv3_err != MHU_V_3_X_ERR_NONE) {
        SCMI_LOG_ERR("SI CL0 to RSE MHU get channels failed: %d",
                     (int)mhuv3_err);
        return SCMI_COMMS_HARDWARE_ERROR;
    }

#ifdef SCMI_COMMS_FOR_BL2_POLLING_MODE
    /* Mask all channels of receiver because we are wroking in polling mode */
    for (ch = 0; ch < num_ch; ch++) {
        mhuv3_err = mhu_v3_x_doorbell_mask_set(&MHU_SI_CL0_TO_RSE_DEV, ch, UINT32_MAX);
        if (mhuv3_err != MHU_V_3_X_ERR_NONE) {
            return SCMI_COMMS_HARDWARE_ERROR;
        }
    }
#endif /* SCMI_COMMS_FOR_BL2_POLLING_MODE */

#ifdef SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS
    /* Clear the mask for notifying doorbell channel */
    mhuv3_err = mhu_v3_x_doorbell_mask_clear(&MHU_SI_CL0_TO_RSE_DEV,
                                             SI_MHU_DOORBELL_CHANNEL,
                                             UINT32_MAX);
    if (mhuv3_err != MHU_V_3_X_ERR_NONE) {
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    /* Set masks for other doorbell channels */
    for (ch = 0; ch < num_ch; ch++) {
        if (ch == SI_MHU_DOORBELL_CHANNEL) {
            continue;
        }
        mhuv3_err = mhu_v3_x_doorbell_mask_set(&MHU_SI_CL0_TO_RSE_DEV,
                                               ch,
                                               UINT32_MAX);
        if (mhuv3_err != MHU_V_3_X_ERR_NONE) {
            return SCMI_COMMS_HARDWARE_ERROR;
        }
    }

    /* Enable channel interrupt for doorbell */
    mhuv3_err = mhu_v3_x_channel_interrupt_enable(&MHU_SI_CL0_TO_RSE_DEV,
                                                  SI_MHU_DOORBELL_CHANNEL,
                                                  MHU_V3_X_CHANNEL_TYPE_DBCH);
    if (mhuv3_err != MHU_V_3_X_ERR_NONE) {
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    SCMI_LOG_INF("SI CL0 MHU doorbell channel interrupt enabled");
#endif /* SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS */

    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t scmi_hal_doorbell_ring(void)
{
    enum mhu_v3_x_error_t err;

    err = mhu_v3_x_doorbell_write(&MHU_RSE_TO_SI_CL0_DEV,
                                SI_MHU_DOORBELL_CHANNEL, SI_MHU_COMMAND_PBX_FLAG);
    if (err != MHU_V_3_X_ERR_NONE) {
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t scmi_hal_doorbell_clear(void)
{
    enum mhu_v3_x_error_t err;
    uint32_t ch_val = 0;
#ifdef SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS
    uint32_t valid_signals = (SI_MHU_COMMAND_MBX_FLAG | SI_MHU_NOTIFIER_MBX_FLAG);
#else
    uint32_t valid_signals = (SI_MHU_COMMAND_MBX_FLAG);
#endif

    err = mhu_v3_x_doorbell_read(&MHU_SI_CL0_TO_RSE_DEV,
                                SI_MHU_DOORBELL_CHANNEL, &ch_val);
    if (err != MHU_V_3_X_ERR_NONE) {
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    err = mhu_v3_x_doorbell_clear(&MHU_SI_CL0_TO_RSE_DEV,
                                SI_MHU_DOORBELL_CHANNEL, ch_val);
    if (err != MHU_V_3_X_ERR_NONE) {
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    if ((ch_val & ~valid_signals) != 0) {
        SCMI_LOG_ERR("Unknown doorbell flag detected");
        return SCMI_COMMS_HARDWARE_ERROR;
    }
    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t scmi_hal_doorbell_read(uint32_t *value)
{
    enum mhu_v3_x_error_t err;

    err = mhu_v3_x_doorbell_read(&MHU_SI_CL0_TO_RSE_DEV,
                                SI_MHU_DOORBELL_CHANNEL, value);

    if (err != MHU_V_3_X_ERR_NONE) {
        SCMI_LOG_ERR("Failed to read MHU doorbell register");
        return SCMI_COMMS_HARDWARE_ERROR;
    }

#ifdef SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS
    if (*value == SCMI_MHU_DOORBELL_VALUE_INVALID) {
        SCMI_LOG_ERR("Received undefined doorbell value 0x0");
        return SCMI_COMMS_INVALID_ARGUMENT;
    }
#endif

    return SCMI_COMMS_SUCCESS;
}

#ifdef SCMI_COMMS_FOR_BL2_POLLING_MODE
void scmi_hal_wait(uint32_t cycles)
{
    while (cycles--) {
        __asm__("nop");
    }
}
#endif /* SCMI_COMMS_FOR_BL2_POLLING_MODE */

#ifdef SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS
scmi_comms_err_t scmi_hal_shared_memory_read(uint32_t db_flag_bit, struct scmi_message_t *msg)
{
    if (msg == NULL) {
        SCMI_LOG_ERR("Message buffer error");
        return SCMI_COMMS_INVALID_ARGUMENT;
    }

    uint32_t page_table_idx = 0;
    for (int i = 0; i < SCMI_SRAM_PAGE_COUNT; ++i) {
        if ((sram_page_table[i].mbx_db_flag_bit & db_flag_bit) == sram_page_table[i].mbx_db_flag_bit) {
            page_table_idx = i;
        }
    }

    struct transport_buffer_t *const page_buffer = sram_page_table[page_table_idx].base_addr;
    uint32_t length = page_buffer->length;

    if ((length < sizeof(page_buffer->message_header)) ||
        (length > TRANSPORT_BUFFER_MAX_LENGTH)) {
        SCMI_LOG_ERR("Shared memory read failed. Incorrect length.");
        return SCMI_COMMS_INVALID_ARGUMENT;
    }

    memcpy(msg, &page_buffer->message_header, length);
    msg->payload_len = length - sizeof(msg->header);

    SCMI_LOG_INF("Shared memory read complete.");
    return SCMI_COMMS_SUCCESS;
}

int32_t scmi_hal_sys_power_state(uint32_t agent_id, uint32_t flags,
                                 uint32_t system_state)
{
    SCMI_LOG_INF("Power state notification received");

    /* Check if flags parameter is a valid value. */
    if (flags > SCMI_SYS_POWER_STATE_FLAGS_GRACEFUL_MASK) {
        SCMI_LOG_ERR("Incorrect flag value");
        return SCMI_STATUS_INVALID_PARAMETERS;
    }

    /* Check if a graceful power state transition is requested
     * using flags value. */
    if ((flags & SCMI_SYS_POWER_STATE_FLAGS_GRACEFUL_MASK)) {
        SCMI_LOG_INF("Graceful system power state transition requested");
    } else {
        SCMI_LOG_INF("Forceful system power state transition requested");
    }

    /* Check system state change has occured.
     * Note: Power up state is included for coverage but is treated
     * as an unsupported command in RSE runtime. */
    switch (system_state) {
    case SCMI_SYS_POWER_STATE_SHUTDOWN:
        SCMI_LOG_NOT("System shutdown complete");
        while (1) {
            __WFI();
        }
        break;
    case SCMI_SYS_POWER_STATE_COLD_RESET:
    case SCMI_SYS_POWER_STATE_WARM_RESET:
        SCMI_LOG_NOT("Resetting system");
        tfm_hal_system_reset(TFM_PLAT_SWSYN_DEFAULT);
        break;
    case SCMI_SYS_POWER_STATE_POWER_UP:
    default:
        SCMI_LOG_ERR("Unsupported command");
        return SCMI_STATUS_NOT_SUPPORTED;
    }

    return SCMI_STATUS_SUCCESS;
}
#endif /* SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS */
