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
#include "scmi_power_domain.h"
#include "scmi_hal.h"


/**
 * \brief Create a SCMI power domain protocol version message.
 *
 * \param[out] msg  SCMI message
 */
static void scmi_message_power_domain_protocol_version(struct scmi_message_t *msg)
{
    /* Validate input */
    if (msg == NULL) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }

    msg->header =
        scmi_message_header(SCMI_MESSAGE_ID_SYS_POWER_DOMAIN_PROTOCOL_VERSION,
                            SCMI_MESSAGE_TYPE_COMMAND,
                            SCMI_PROTOCOL_ID_POWER_DOMAIN,
                            0);

    msg->payload_len = 0; /* this message has no payload*/
}

/**
 * \brief Handle a SCMI power domain protocol version response message.
 *
 * \param[in] msg  SCMI message
 * \param[out] version protocol verion
 * \return Error value as defined by scmi_comms_err_t.
 */
static scmi_comms_err_t scmi_handle_power_domain_protocol_version(
    struct scmi_message_t *msg, uint32_t *version)
{
    scmi_comms_err_t err;

    if (msg->payload_len !=
        sizeof(struct scmi_pd_protocol_version_response_t)) {
        msg->payload_len = 0;
        err = SCMI_STATUS_COMMS_ERROR;
        return err;
    }

    struct scmi_pd_protocol_version_response_t *response =
        (struct scmi_pd_protocol_version_response_t *)msg->payload;

    if (response->status != SCMI_STATUS_SUCCESS) {
        err = response->status;
        return err;
    }
    *version = response->version;

    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t scmi_comm_get_power_domain_version(uint32_t *version)
{
    if (version == NULL) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }
    scmi_comms_err_t err;
    struct scmi_message_t msg;

    /* Ensure message is zero-initialized to avoid garbage values */
    memset(&msg, 0, sizeof(msg));

    scmi_message_power_domain_protocol_version(&msg);

    err = transport_send(&msg);
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    /* wait a while and check the mhu mbx flag */
    scmi_hal_wait(SCMI_HAL_WAIT_TIME);
    err = transport_wait();
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    err = transport_receive(&msg);
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    return scmi_handle_power_domain_protocol_version(&msg, version);
}

/**
 * \brief Create a SCMI power domain state set message.
 *
 * \param[out] msg  SCMI message
 * \param[in] flags 0: sync ; 1: async
 * \param[in] domain_id Identifier for the power domain
 * \param[in] power_domain_state power domain state to set
 */
static void scmi_message_power_domain_state_set(struct scmi_message_t *msg,
    uint32_t flags, uint32_t domain_id, uint32_t power_domain_state)
{
    msg->header =
        scmi_message_header(SCMI_MESSAGE_ID_PD_POWER_STATE_SET,
                            SCMI_MESSAGE_TYPE_COMMAND,
                            SCMI_PROTOCOL_ID_POWER_DOMAIN,
                            0);

    assert(sizeof(struct scmi_pd_power_state_set_t) <= sizeof(msg->payload));

    memcpy(msg->payload,
           &(struct scmi_pd_power_state_set_t) {
                .flags = flags,
                .domain_id = domain_id,
                .power_state = power_domain_state},
           sizeof(struct scmi_pd_power_state_set_t));

    msg->payload_len = sizeof(struct scmi_pd_power_state_set_t);
}

/**
 * \brief Handle a SCMI system power set response message.
 *
 * \param[in,out] msg  SCMI message
 * \return Error value as defined by scmi_comms_err_t.
 */
static scmi_comms_err_t scmi_handle_power_domain_state_set(
    struct scmi_message_t *msg)
{
    scmi_comms_err_t err;

    if (msg->payload_len !=
        sizeof(struct scmi_pd_power_state_set_response_t)) {
        msg->payload_len = 0;
        err = SCMI_STATUS_COMMS_ERROR;
        return err;
    }

    struct scmi_pd_power_state_set_response_t *response =
        (struct scmi_pd_power_state_set_response_t *)msg->payload;

    if (response->status != SCMI_STATUS_SUCCESS) {
        err = response->status;
        return err;
    }

    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t scmi_comm_power_domain_state_set(uint32_t domain_id,
    uint32_t power_domain_state)
{
    scmi_comms_err_t err;
    struct scmi_message_t msg;

    scmi_message_power_domain_state_set(&msg, 0, domain_id, power_domain_state);

    err = transport_send(&msg);
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    /* wait a while and check the mhu mbx flag */
    scmi_hal_wait(SCMI_HAL_WAIT_TIME);
    err = transport_wait();
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    err = transport_receive(&msg);
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    return (scmi_handle_power_domain_state_set(&msg) ==
        SCMI_STATUS_SUCCESS) ? SCMI_COMMS_SUCCESS : SCMI_COMMS_GENERIC_ERROR;
}

/**
 * \brief Create a message of request power domain attributes
 *
 * \param[out] msg  SCMI message
 * \param[in,out] domain_id Identifier for the power domain
 * \return scmi_comms_err_t
 */
static scmi_comms_err_t scmi_message_power_domain_attributes(
    struct scmi_message_t *msg, uint32_t domain_id)
{
    /* Validate input */
    if (msg == NULL) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }

    msg->header =
        scmi_message_header(SCMI_MESSAGE_ID_PD_POWER_DOMAIN_ATTRIBUTES,
                            SCMI_MESSAGE_TYPE_COMMAND,
                            SCMI_PROTOCOL_ID_POWER_DOMAIN,
                            0);

    assert(sizeof(struct scmi_pd_power_domain_attributes_t)
        <= sizeof(msg->payload));

    memcpy(msg->payload,
           &(struct scmi_pd_power_domain_attributes_t) {
                .domain_id = domain_id},
           sizeof(struct scmi_pd_power_domain_attributes_t));

    msg->payload_len = sizeof(struct scmi_pd_power_domain_attributes_t);
}

/**
 * \brief Handle a SCMI power domain attributes response message.
 *
 * \param[in,out] msg  SCMI message
 * \return Error value as defined by scmi_comms_err_t.
 */
static scmi_comms_err_t scmi_handle_power_domain_attributes(
    struct scmi_message_t *msg, uint32_t *domain_attributes,
    uint8_t *domain_name)
{
    scmi_comms_err_t err;

    if (msg->payload_len !=
        sizeof(struct scmi_pd_power_domain_attributes_response_t)) {
        msg->payload_len = 0;
        err = SCMI_STATUS_COMMS_ERROR;
        return err;
    }

    struct scmi_pd_power_domain_attributes_response_t *response =
        (struct scmi_pd_power_domain_attributes_response_t *)msg->payload;

    if (response->status != SCMI_STATUS_SUCCESS) {
        err = response->status;
        return err;
    }
    *domain_attributes = response->attributes;
    memcpy(domain_name, response->name, SCMI_POWER_DOMAIN_NAME_LEN - 1);
    domain_name[SCMI_POWER_DOMAIN_NAME_LEN-1] = 0;

    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t scmi_comm_power_domain_attributes(uint32_t domain_id,
    uint32_t *domain_attributes, uint8_t *domain_name)
{
    if (domain_attributes == NULL || domain_name == NULL) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }

    scmi_comms_err_t err;
    struct scmi_message_t msg;

    scmi_message_power_domain_attributes(&msg, domain_id);

    err = transport_send(&msg);
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    /* wait a while and check the mhu mbx flag */
    scmi_hal_wait(SCMI_HAL_WAIT_TIME);
    err = transport_wait();
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    err = transport_receive(&msg);
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    err = scmi_handle_power_domain_attributes(&msg,
            domain_attributes,
            domain_name);

    return (err == SCMI_STATUS_SUCCESS) ? SCMI_COMMS_SUCCESS
        : SCMI_COMMS_GENERIC_ERROR;
}

/**
 * \brief Create a message of get a power domain state
 *
 * \param[out] msg SCMI message
 * \param[in,out] domain_id Identifier for the power domain
 * \return scmi_comms_err_t
 */
static scmi_comms_err_t scmi_message_power_domain_state_get(
    struct scmi_message_t *msg, uint32_t domain_id)
{
    /* Validate input */
    if (msg == NULL) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }

    msg->header =
        scmi_message_header(SCMI_MESSAGE_ID_PD_POWER_STATE_GET,
                            SCMI_MESSAGE_TYPE_COMMAND,
                            SCMI_PROTOCOL_ID_POWER_DOMAIN,
                            0);

    assert(sizeof(struct scmi_pd_power_state_get_t)
        <= sizeof(msg->payload));

    memcpy(msg->payload,
           &(struct scmi_pd_power_state_get_t) {
                .domain_id = domain_id},
           sizeof(struct scmi_pd_power_state_get_t));

    msg->payload_len = sizeof(struct scmi_pd_power_state_get_t);
}

/**
 * \brief Handle a power domain state get response message
 *
 * \param[in,out] msg SCMI message
 * \param[in,out] power_domain_state return the power domain state
 * \return scmi_comms_err_t
 */
static scmi_comms_err_t scmi_handle_power_domain_state_get(
    struct scmi_message_t *msg, uint32_t *power_domain_state)
{
    scmi_comms_err_t err;

    if (msg->payload_len !=
        sizeof(struct scmi_pd_power_state_get_response_t)) {
        msg->payload_len = 0;
        err = SCMI_STATUS_COMMS_ERROR;
        return err;
    }

    struct scmi_pd_power_state_get_response_t *response =
        (struct scmi_pd_power_state_get_response_t *)msg->payload;

    if (response->status != SCMI_STATUS_SUCCESS) {
        err = response->status;
        return err;
    }
    *power_domain_state = response->power_state;
    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t scmi_comm_power_domain_state_get(uint32_t domain_id,
    uint32_t *power_domain_state)
{
    if (power_domain_state == NULL) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }

    scmi_comms_err_t err;
    struct scmi_message_t msg;

    scmi_message_power_domain_state_get(&msg, domain_id);

    err = transport_send(&msg);
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    /* wait a while and check the mhu mbx flag */
    scmi_hal_wait(SCMI_HAL_WAIT_TIME);
    err = transport_wait();
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    err = transport_receive(&msg);
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    return scmi_handle_power_domain_state_get(&msg, power_domain_state);
}
