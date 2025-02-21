/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 */

#ifndef __SCMI_COMMS_H__
#define __SCMI_COMMS_H__

#include <stdint.h>
#include "scmi_hal_defs.h"
#include "scmi_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCMI_MESSAGE_HEADER_MESSAGE_ID_POS   0
#define SCMI_MESSAGE_HEADER_MESSAGE_ID_MASK \
    (UINT32_C(0xFF) << SCMI_MESSAGE_HEADER_MESSAGE_ID_POS)

#define SCMI_MESSAGE_HEADER_MESSAGE_TYPE_POS 8
#define SCMI_MESSAGE_HEADER_MESSAGE_TYPE_MASK \
    (UINT32_C(0x3) << SCMI_MESSAGE_HEADER_MESSAGE_TYPE_POS)

#define SCMI_MESSAGE_HEADER_PROTOCOL_ID_POS  10
#define SCMI_MESSAGE_HEADER_PROTOCOL_ID_MASK \
    (UINT32_C(0xFF) << SCMI_MESSAGE_HEADER_PROTOCOL_ID_POS)

#define SCMI_MESSAGE_HEADER_TOKEN_POS        18
#define SCMI_MESSAGE_HEADER_TOKEN_MASK \
    (UINT32_C(0x3FF) << SCMI_MESSAGE_HEADER_TOKEN_POS)

#define TRANSPORT_BUFFER_MAX_LENGTH \
  (SI_SHARED_MEMORY_SIZE - offsetof(struct transport_buffer_t, message_header))

/**
 * \brief Shared memory area layout used for sending & receiving messages
 */
struct transport_buffer_t {
    uint32_t reserved0; /**< Reserved, must be zero */

    volatile uint32_t status; /**< Channel status */
    uint64_t reserved1; /**< Implementation defined field */
    uint32_t flags; /**< Channel flags */
    volatile uint32_t length; /**< Length(bytes) of message header+payload */
    uint32_t message_header; /**< Message header */
    uint32_t message_payload[]; /**< Message payload */
};

#define SCMI_MESSAGE_PAYLOAD_MAX_WORDS \
    ((TRANSPORT_BUFFER_MAX_LENGTH - sizeof(uint32_t)) / sizeof(uint32_t))

typedef enum {
    SCMI_COMMS_SUCCESS = 0,
    SCMI_COMMS_GENERIC_ERROR,
    SCMI_COMMS_INVALID_ARGUMENT,
    SCMI_COMMS_HARDWARE_ERROR,
} scmi_comms_err_t;

/**
 * \brief Structure representing an SCMI message.
 */
struct scmi_message_t {
    uint32_t header;
    uint32_t payload[SCMI_MESSAGE_PAYLOAD_MAX_WORDS];
    uint32_t payload_len;
};

/**
 * @brief Constructs an SCMI message header.
 *
 * @param[in] message_id Identifier for the SCMI message.
 * @param[in] message_type Type of SCMI message.
 * @param[in] protocol_id Protocol ID associated with the message.
 * @param[in] token Transaction token for message tracking.
 * @return uint32_t Encoded SCMI message header.
 */
uint32_t scmi_message_header(uint8_t message_id, uint8_t message_type,
                                    uint8_t protocol_id, uint8_t token);

/**
 * \brief Init SCMI comm context.
 * \return Error value as defined by scmi_comms_err_t.
 */
scmi_comms_err_t scmi_comm_init(void);

/**
 * \brief Abort the last scmi comm transaction. It clears the internal
 * status/flags of the transport layers.
 */
void scmi_comm_abort(void);

/**
 * \brief Write a message from the local buffer to the shared memory and wait
 *        for a peer end acknowledge to read the message.
 *
 * \param[in] msg  SCMI message
 *
 * \return Error value as defined by scmi_comms_err_t.
 */
int32_t transport_send(const struct scmi_message_t *msg);

/**
 * \brief Read a message from the shared memory to the local buffer.
 *
 * \param[out] msg  SCMI message
 *
 * \return Error value as defined by scmi_comms_err_t.
 */
scmi_comms_err_t transport_receive(struct scmi_message_t *msg);

/**
 * \brief Wait for the SCMI platform response by reading doorbell register
 *  expected flag is set.
 * \return Error value as defined by scmi_comms_err_t.
 */
scmi_comms_err_t transport_wait(void);

#ifdef __cplusplus
}
#endif

#endif /* __SCMI_COMMS_H__ */
