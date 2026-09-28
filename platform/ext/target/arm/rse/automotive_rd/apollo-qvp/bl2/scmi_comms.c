/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 */

#include "bootutil/bootutil_log.h"

#include <assert.h>
#include <stdbool.h>
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

static scmi_comms_err_t transport_try_send(const struct scmi_message_t *msg,
                                         bool *submitted)
{
    scmi_comms_err_t err;
    uint32_t length = msg->payload_len + sizeof(msg->header);

    *submitted = false;
    /* Validate message size before sending */
    if (length > TRANSPORT_BUFFER_MAX_LENGTH) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }

    /* BUSY is not a failed RPC: no command has been submitted yet. */
    if (!(shared_memory->status & TRANSPORT_BUFFER_STATUS_FREE_MASK)) {
        return SCMI_COMMS_SUCCESS;
    }

    /* Populate shared memory with the message */
    __sync_synchronize();
    memcpy(&shared_memory->message_header, msg, length);
    shared_memory->length = length;

    /* Require the response set a bit of channel*/
    shared_memory->flags |= TRANSPORT_BUFFER_FLAGS_INTERRUPT_MASK;

    /* Mark channel as busy */
    __sync_synchronize();
    shared_memory->status &= ~TRANSPORT_BUFFER_STATUS_FREE_MASK;

    /* Ring doorbell to notify receiver */
    __sync_synchronize();
    *submitted = true;
    err = scmi_hal_doorbell_ring();
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }
    return SCMI_COMMS_SUCCESS;
}

int32_t transport_send(const struct scmi_message_t *msg)
{
    scmi_comms_err_t err;
    bool submitted;

    if (msg->payload_len + sizeof(msg->header) > TRANSPORT_BUFFER_MAX_LENGTH) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }
    /* Preserve the wait and error ABI of the other boot-time RPCs. */
    scmi_hal_wait(SCMI_HAL_WAIT_TIME);
    err = transport_try_send(msg, &submitted);
    if (err != SCMI_COMMS_SUCCESS) {
        return err;
    }
    return submitted ? SCMI_COMMS_SUCCESS : SCMI_STATUS_GENERIC_ERROR;
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

scmi_comms_err_t transport_exchange(struct scmi_message_t *msg,
                                   uint32_t timeout_s)
{
    uint32_t frequency = scmi_hal_counter_frequency();
    uint64_t start = scmi_hal_counter_ticks();
    uint64_t budget = (uint64_t)frequency * timeout_s;
    uint32_t expected_header;
    uint32_t doorbell;
    scmi_comms_err_t err;
    int sent = 0;

    if (msg == NULL || frequency == 0 || timeout_s == 0) {
        return SCMI_COMMS_INVALID_ARGUMENT;
    }
    expected_header = msg->header;

    /* Elapsed subtraction also handles a wrapping 64-bit counter. */
    while (scmi_hal_counter_ticks() - start < budget) {
        err = scmi_hal_doorbell_read(&doorbell);
        if (err != SCMI_COMMS_SUCCESS) {
            return err;
        }
        if (sent && (doorbell & SI_MHU_COMMAND_MBX_FLAG)) {
            if (scmi_hal_counter_ticks() - start >= budget) {
                return SCMI_COMMS_GENERIC_ERROR;
            }
            if (!(shared_memory->status & TRANSPORT_BUFFER_STATUS_FREE_MASK)) {
                return SCMI_COMMS_GENERIC_ERROR;
            }
            if (shared_memory->status & TRANSPORT_BUFFER_STATUS_ERROR_MASK) {
                return SCMI_COMMS_GENERIC_ERROR;
            }
            __sync_synchronize();
            err = transport_receive(msg);
            if (err != SCMI_COMMS_SUCCESS) {
                return err;
            }
            return msg->header == expected_header ? SCMI_COMMS_SUCCESS :
                                                   SCMI_COMMS_INVALID_ARGUMENT;
        }

        /* SCP startup may initialize its mailbox after an early request.
         * Only FREE + empty proves that initialization discarded the request.
         * FREE + response data can precede the response doorbell: keep waiting
         * and never overwrite that response or a BUSY outstanding command.
         */
        if (!(shared_memory->status & TRANSPORT_BUFFER_STATUS_FREE_MASK)) {
            continue;
        }
        __sync_synchronize();
        if (!sent || shared_memory->length == 0) {
            bool submitted;

            if (scmi_hal_counter_ticks() - start >= budget) {
                return SCMI_COMMS_GENERIC_ERROR;
            }
            err = transport_try_send(msg, &submitted);
            if (err != SCMI_COMMS_SUCCESS) {
                return err;
            }
            /* A concurrent completer initialization can make the mailbox
             * BUSY after the outer check. Retry without resetting deadline.
             */
            if (submitted) {
                sent = 1;
            }
        }
    }
    return SCMI_COMMS_GENERIC_ERROR;
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
