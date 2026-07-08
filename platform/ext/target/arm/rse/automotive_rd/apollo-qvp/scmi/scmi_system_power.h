/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 */

#ifndef __SCMI_SYSTEM_POWER_H__
#define __SCMI_SYSTEM_POWER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SCMI system power protocol IDs
 */
#define SCMI_PROTOCOL_ID_SYS_POWER_STATE UINT8_C(0x12)

/**
 * SCMI system power message IDs
 */
#define SCMI_MESSAGE_ID_SYS_POWER_PROTOCOL_VERSION         UINT8_C(0x0)
#define SCMI_MESSAGE_ID_SYS_POWER_STATE_SET                UINT8_C(0x3)
#define SCMI_MESSAGE_ID_SYS_POWER_STATE_NOTIFY             UINT8_C(0x5)
#define SCMI_MESSAGE_ID_SYS_POWER_STATE_NOTIFIER           UINT8_C(0x0)

/**
 * SCMI system power state messages
 */
#define SCMI_SYS_POWER_STATE_FLAGS_GRACEFUL_POS 0
#define SCMI_SYS_POWER_STATE_FLAGS_GRACEFUL_MASK \
    (UINT32_C(0x1) << SCMI_SYS_POWER_STATE_FLAGS_GRACEFUL_POS)

#define SCMI_SYS_POWER_STATE_SHUTDOWN   UINT32_C(0)
#define SCMI_SYS_POWER_STATE_COLD_RESET UINT32_C(1)
#define SCMI_SYS_POWER_STATE_WARM_RESET UINT32_C(2)
#define SCMI_SYS_POWER_STATE_POWER_UP   UINT32_C(3)
#define SCMI_SYS_POWER_STATE_SUSPEND    UINT32_C(4)

/**
 * \brief System power state protocol version response payload
 */
struct scmi_sys_power_protocol_version_response_t {
    int32_t  status;
    uint32_t version;
};

/**
 * \brief System power state set message payload.
 */
struct scmi_sys_power_state_set_t {
    uint32_t flags;
    uint32_t system_state;
};

/**
 * \brief System power state set response payload.
 */
struct scmi_sys_power_state_set_response_t {
    int32_t status;
};

/**
 * \brief System power state notification subscription message payload.
 */
struct scmi_sys_power_state_notify_t {
    /**< Enable scmi_sys_power_state_notifier_t notifications */
    uint32_t notify_enable;
};

/**
 * \brief System power state notification subscription response payload.
 */
struct scmi_sys_power_state_notify_response_t {
    int32_t status;
};

/**
 * \brief System power state notification message payload.
 */
struct scmi_sys_power_state_notifier_t {
    /**< ID of the agent that caused the power state transition */
    uint32_t agent_id;
    uint32_t flags;
    uint32_t system_state;
};

#ifdef __cplusplus
}
#endif

/**
 * \brief SCMI agent set system power state to platform.
 *
 * \param[in] sys_power_state SCMI_SYS_POWER_STATE_<x> in scmi_system_ppower.h
 * \return Error value as defined by scmi_comms_err_t.
 */
scmi_comms_err_t scmi_comm_sys_power_state_set(uint32_t sys_power_state);

/**
 * \brief SCMI agent get system power protocol version from platform.
 *
 * \param[out] version  protocol version return from platform.
 * \return Error value as defined by scmi_comms_err_t.
 */
scmi_comms_err_t scmi_comm_get_sys_power_version(uint32_t *version);

#endif /* __SCMI_SYSTEM_POWER_H__ */
