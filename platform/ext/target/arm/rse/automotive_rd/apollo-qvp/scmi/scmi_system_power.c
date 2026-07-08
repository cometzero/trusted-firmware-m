/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "scmi_comms.h"
#include "scmi_system_power.h"
#include "scmi_hal.h"

/**
 * \brief Create an SCMI system power state protocol version message.
 *
 * \param[out] msg  SCMI message
 */
static void scmi_message_sys_power_protocol_version(struct scmi_message_t *msg)
{
    /* Validate input */
    if (msg == NULL) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }

    msg->header =
        scmi_message_header(SCMI_MESSAGE_ID_SYS_POWER_PROTOCOL_VERSION,
                            SCMI_MESSAGE_TYPE_COMMAND,
                            SCMI_PROTOCOL_ID_SYS_POWER_STATE,
                            0);

    msg->payload_len = 0; /* this message has no payload*/
}

/**
 * \brief Handle an SCMI system power protocol version response message.
 *
 * \param[in] msg  SCMI message
 * \param[out] version protocol verion
 * \return Error value as defined by scmi_comms_err_t.
 */
static scmi_comms_err_t scmi_handle_sys_power_protocol_version(
    struct scmi_message_t *msg, uint32_t *version)
{
    /* Validate input pointers to prevent null dereference */
    if (msg == NULL || version == NULL) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }

    /* Check if the payload length matches the expected response size */
    if (msg->payload_len !=
        sizeof(struct scmi_sys_power_protocol_version_response_t)) {
        return SCMI_STATUS_COMMS_ERROR;
    }

    /* Cast the payload to the expected response structure */
    struct scmi_sys_power_protocol_version_response_t *response =
        (struct scmi_sys_power_protocol_version_response_t *)msg->payload;

    /* Check the response status field */
    if (response->status != SCMI_STATUS_SUCCESS) {
        return (scmi_comms_err_t)response->status;
    }

    /* Extract the protocol version and store it in the provided pointer */
    *version = response->version;

    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t scmi_comm_get_sys_power_version(uint32_t *version)
{
    if (version == NULL) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }
    scmi_comms_err_t err;
    struct scmi_message_t msg;

    /* Ensure message is zero-initialized to avoid garbage values */
    memset(&msg, 0, sizeof(msg));

    /* Populate message with system power protocol version request */
    scmi_message_sys_power_protocol_version(&msg);

    err = transport_send(&msg);
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    /*wait a while and check the mhu mbx flag*/
    scmi_hal_wait(SCMI_HAL_WAIT_TIME);
    err = transport_wait();
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    err = transport_receive(&msg);
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }
    /* Extract version from the response message */
    return (scmi_handle_sys_power_protocol_version(&msg, version));
}

/**
 * \brief Create an SCMI system power state set message.
 *
 * \param[out] msg  SCMI message
 * \param[in] flags 0: forceful ; 1: graceful
 * \param[in] system_state SCMI_SYS_POWER_STATE_XXX defined in scmi_protocol.h
 */
static void scmi_message_sys_power_state_set(struct scmi_message_t *msg,
    uint32_t flags, uint32_t system_sate)
{
    msg->header =
        scmi_message_header(SCMI_MESSAGE_ID_SYS_POWER_STATE_SET,
                            SCMI_MESSAGE_TYPE_COMMAND,
                            SCMI_PROTOCOL_ID_SYS_POWER_STATE,
                            0);

    assert(sizeof(struct scmi_sys_power_state_set_t) <= sizeof(msg->payload));

    /* Copy the payload to the message structure */
    memcpy(msg->payload,
           &(struct scmi_sys_power_state_set_t) {
                .flags = flags, .system_state = system_sate},
           sizeof(struct scmi_sys_power_state_set_t));

    /* Set the payload length */
    msg->payload_len = sizeof(struct scmi_sys_power_state_set_t);
}

/**
 * \brief Handle an SCMI system power set response message.
 *
 * \param[in] msg  SCMI message
 * \return Error value as defined by scmi_comms_err_t.
 */
static scmi_comms_err_t scmi_handle_sys_power_set(
    struct scmi_message_t *msg)
{
    scmi_comms_err_t err;

    if (msg->payload_len !=
        sizeof(struct scmi_sys_power_state_set_response_t)) {
        msg->payload_len = 0;
        err = SCMI_STATUS_COMMS_ERROR;
        return err;
    }

    /* Cast the payload to the expected response structure */
    struct scmi_sys_power_state_set_response_t *response =
        (struct scmi_sys_power_state_set_response_t *)msg->payload;

    /* Check the response status field */
    if (response->status != SCMI_STATUS_SUCCESS) {
        err = response->status;
        return err;
    }

    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t scmi_comm_sys_power_state_set(uint32_t sys_power_state)
{
    scmi_comms_err_t err;
    struct scmi_message_t msg;

    scmi_message_sys_power_state_set(&msg, 0, sys_power_state);

    err = transport_send(&msg);
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    /*wait a while and check the mhu mbx flag*/
    scmi_hal_wait(SCMI_HAL_WAIT_TIME);
    err = transport_wait();
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    err = transport_receive(&msg);
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    return scmi_handle_sys_power_set(&msg);
}
