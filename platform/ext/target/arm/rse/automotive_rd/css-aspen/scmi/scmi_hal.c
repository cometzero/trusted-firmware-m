/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 */

#include "bootutil/bootutil_log.h"
#include "device_definition.h"
#include "host_atu_base_address.h"
#include "scmi_hal.h"
#include "scmi_protocol.h"


scmi_comms_err_t scmi_hal_shared_memory_init(void)
{
    enum atu_error_t err;

    err = atu_rse_initialize_region(&ATU_DEV_S,
                                    RSE_ATU_SI_SSRAM_ID,
                                    HOST_RSE_SI_SSRAM_ATU_BASE_S,
                                    HOST_RSE_SI_SSRAM_ATU_PHYS_BASE,
                                    HOST_RSE_SI_SSRAM_ATU_SIZE);
    if (err != ATU_ERR_NONE) {
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t scmi_hal_doorbell_init(void)
{
    enum mhu_v3_x_error_t mhuv3_err;
    uint8_t ch;
    uint8_t num_ch;
    uint8_t i;

    /* Init sender */
    mhuv3_err = mhu_v3_x_driver_init(&MHU_RSE_TO_SI_CL0_DEV);
    if (mhuv3_err != MHU_V_3_X_ERR_NONE) {
        BOOT_LOG_ERR("SCMI: RSE to SI CL0 MHU driver init failed: %d",
                     (int)mhuv3_err);
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    /* Read the number of doorbell channels implemented in the MHU Sender */
    mhuv3_err = mhu_v3_x_get_num_channel_implemented(
                &MHU_RSE_TO_SI_CL0_DEV, MHU_V3_X_CHANNEL_TYPE_DBCH, &num_ch);
    if (mhuv3_err != MHU_V_3_X_ERR_NONE) {
        BOOT_LOG_ERR("SCMI: RSE to SI CL0 MHU get channels failed: %d",
                     (int)mhuv3_err);
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    /* Init receiver */
    mhuv3_err = mhu_v3_x_driver_init(&MHU_SI_CL0_TO_RSE_DEV);
    if (mhuv3_err != MHU_V_3_X_ERR_NONE) {
        BOOT_LOG_ERR("SCMI: SI CL0 to RSE MHU driver init failed: %d",
                     (int)mhuv3_err);
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    /* Read the number of doorbell channels implemented in the MHU Receiver */
    mhuv3_err = mhu_v3_x_get_num_channel_implemented(
                &MHU_SI_CL0_TO_RSE_DEV, MHU_V3_X_CHANNEL_TYPE_DBCH, &num_ch);
    if (mhuv3_err != MHU_V_3_X_ERR_NONE) {
        BOOT_LOG_ERR("SCMI: SI CL0 to RSE MHU get channels failed: %d",
                     (int)mhuv3_err);
        return SCMI_COMMS_HARDWARE_ERROR;
    }
    /* Mask all channels of receiver because we are wroking in polling mode */
    for (i = 0; i < num_ch; i++) {
        mhuv3_err = mhu_v3_x_doorbell_mask_set(&MHU_SI_CL0_TO_RSE_DEV, i, UINT32_MAX);
        if (mhuv3_err != MHU_V_3_X_ERR_NONE) {
            return SCMI_COMMS_HARDWARE_ERROR;
        }
    }

    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t scmi_hal_doorbell_ring(void)
{
    enum mhu_v3_x_error_t err;

    err = mhu_v3_x_doorbell_write(&MHU_RSE_TO_SI_CL0_DEV,
                                SI_MHU_DOORBELL_CHANNEL, SI_MHU_PBX_FLAG);
    if (err != MHU_V_3_X_ERR_NONE) {
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t scmi_hal_doorbell_clear(void)
{
    enum mhu_v3_x_error_t err;

    err = mhu_v3_x_doorbell_clear(&MHU_SI_CL0_TO_RSE_DEV,
                                SI_MHU_DOORBELL_CHANNEL, SI_MHU_MBX_FLAG);
    if (err != MHU_V_3_X_ERR_NONE) {
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
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    return SCMI_COMMS_SUCCESS;
}

void scmi_hal_wait(uint32_t cycles)
{
    while (cycles--) {
        __asm__("nop");
    }
}
