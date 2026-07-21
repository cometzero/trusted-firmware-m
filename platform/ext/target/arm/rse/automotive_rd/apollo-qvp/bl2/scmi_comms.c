/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 */

#include "bootutil/bootutil_log.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "scmi_comms.h"
#include "scmi_hal.h"

uint32_t scmi_message_header(uint8_t message_id, uint8_t message_type,
                                    uint8_t protocol_id, uint8_t token)
{
    return (((uint32_t)message_id << SCMI_MESSAGE_HEADER_MESSAGE_ID_POS) &
            SCMI_MESSAGE_HEADER_MESSAGE_ID_MASK) |
           (((uint32_t)message_type << SCMI_MESSAGE_HEADER_MESSAGE_TYPE_POS) &
            SCMI_MESSAGE_HEADER_MESSAGE_TYPE_MASK) |
           (((uint32_t)protocol_id << SCMI_MESSAGE_HEADER_PROTOCOL_ID_POS) &
            SCMI_MESSAGE_HEADER_PROTOCOL_ID_MASK) |
           (((uint32_t)token << SCMI_MESSAGE_HEADER_TOKEN_POS) &
            SCMI_MESSAGE_HEADER_TOKEN_MASK);
}

static struct transport_buffer_t *const shared_memory =
    (struct transport_buffer_t *)SI_SHARED_MEMORY_BASE;

/**
 * \brief Initialize the SCMI transport layer.
 *
 * \return Error value as defined by scmi_comms_err_t.
 */
static scmi_comms_err_t transport_init(void)
{
    scmi_comms_err_t err;

    err = scmi_hal_doorbell_init();
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    err = scmi_hal_shared_memory_init();
    if (err != SCMI_COMMS_SUCCESS) {
        BOOT_LOG_INF("SCMI: SCMI shared memory init failure.");
        return err;
    }

    shared_memory->flags = 0;
    shared_memory->length = 0;
    shared_memory->status = TRANSPORT_BUFFER_STATUS_FREE_MASK;

    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t transport_receive(struct scmi_message_t *msg)
{
    /* Validate input */
    if (msg == NULL) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }
    scmi_comms_err_t err = scmi_hal_doorbell_clear();

    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    uint32_t length = shared_memory->length;

    /* Validate received message length */
    if ((length < sizeof(shared_memory->message_header)) ||
        (length > TRANSPORT_BUFFER_MAX_LENGTH)) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }

    /* Copy the received message from shared memory */
    memcpy(msg, &shared_memory->message_header, length);
    /* Correct payload length calculation */
    msg->payload_len = length - sizeof(msg->header);

    return SCMI_COMMS_SUCCESS;
}

int32_t transport_send(const struct scmi_message_t *msg)
{
    scmi_comms_err_t err;
    uint32_t length = msg->payload_len + sizeof(msg->header);

    /* Validate message size before sending */
    if (length > TRANSPORT_BUFFER_MAX_LENGTH) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }

    /* Wait for channel to be free */
    scmi_hal_wait(SCMI_HAL_WAIT_TIME);
    if (!(shared_memory->status & TRANSPORT_BUFFER_STATUS_FREE_MASK)) {
        return SCMI_STATUS_GENERIC_ERROR;
    }

    /* Populate shared memory with the message */
    memcpy(&shared_memory->message_header, msg, length);
    shared_memory->length = length;

    /* Require the response set a bit of channel*/
    shared_memory->flags |= TRANSPORT_BUFFER_FLAGS_INTERRUPT_MASK;

    /* Mark channel as busy */
    shared_memory->status &= ~TRANSPORT_BUFFER_STATUS_FREE_MASK;

    /* Ring doorbell to notify receiver */
    err = scmi_hal_doorbell_ring();
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }
    return SCMI_COMMS_SUCCESS;
}

scmi_comms_err_t transport_wait(void)
{
    scmi_comms_err_t err;
    uint32_t value = 0;

    err = scmi_hal_doorbell_read(&value);
    if (err != SCMI_COMMS_SUCCESS) {
        return SCMI_COMMS_HARDWARE_ERROR;
    }

    if (!(value & SI_MHU_COMMAND_MBX_FLAG)) {
        return SCMI_STATUS_GENERIC_ERROR;
    }

    return SCMI_COMMS_SUCCESS;
}

/**
 * \brief Abort the last SCMI transport.
 *
 * \return Error value as defined by scmi_comms_err_t.
 */
static scmi_comms_err_t transport_abort(void)
{
    shared_memory->flags = 0;
    shared_memory->length = 0;
    shared_memory->status = TRANSPORT_BUFFER_STATUS_FREE_MASK;
    return scmi_hal_doorbell_clear();
}

scmi_comms_err_t scmi_comm_init(void)
{
    scmi_comms_err_t err;

    err = transport_init();
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    err = scmi_hal_doorbell_clear();
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }

    return SCMI_COMMS_SUCCESS;
}

void scmi_comm_abort(void)
{
    transport_abort();
}
