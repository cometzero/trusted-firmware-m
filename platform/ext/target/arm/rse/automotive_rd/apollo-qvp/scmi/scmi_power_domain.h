/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 */

#ifndef __SCMI_POWER_DOMAIN_H__
#define __SCMI_POWER_DOMAIN_H__

#include <stdint.h>

#include "scmi_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SCMI power domain protocol IDs
 */
#define SCMI_PROTOCOL_ID_POWER_DOMAIN    UINT8_C(0x11)

/**
 * SCMI power domaim message IDs
 */
#define SCMI_MESSAGE_ID_SYS_POWER_DOMAIN_PROTOCOL_VERSION      UINT8_C(0x0)
#define SCMI_MESSAGE_ID_PD_POWER_DOMAIN_ATTRIBUTES             UINT8_C(0x3)
#define SCMI_MESSAGE_ID_PD_POWER_STATE_SET                     UINT8_C(0x4)
#define SCMI_MESSAGE_ID_PD_POWER_STATE_GET                     UINT8_C(0x5)
#define SCMI_MESSAGE_ID_PD_POWER_STATE_NOTIFY                  UINT8_C(0x6)
#define SCMI_MESSAGE_ID_PD_POWER_STATE_CHANGE_REQUESTED_NOTIFY UINT8_C(0x7)

/**
 * SCMI power domain state messages
 */
#define SCMI_PD_DEVICE_STATE_ID_OFF  0U
#define SCMI_PD_DEVICE_STATE_ID_ON   0U
#define SCMI_PD_DEVICE_STATE_ID      0U
#define SCMI_PD_DEVICE_STATE_ID_MASK 0xFFFFFFFU
#define SCMI_PD_DEVICE_STATE_TYPE    (1U << 30)

/**
 * \brief power domain state protocol version response payload
 */
struct scmi_pd_protocol_version_response_t {
    int32_t  status;
    uint32_t version;
};

/*
 * PROTOCOL_ATTRIBUTES
 */
struct scmi_pd_protocol_attributes_response_t {
    int32_t status;
    uint32_t attributes;
    uint32_t statistics_address_low;
    uint32_t statistics_address_high;
    uint32_t statistics_len;
};

/*
 * POWER_DOMAIN_ATTRIBUTES
 */
struct scmi_pd_power_domain_attributes_t {
    uint32_t domain_id;
};

#define SCMI_PD_POWER_STATE_CHANGE_NOTIFICATIONS (1UL << 31)
#define SCMI_PD_POWER_STATE_SET_ASYNC            (1U << 30)
#define SCMI_PD_POWER_STATE_SET_SYNC             (1U << 29)

#define SCMI_POWER_DOMAIN_NAME_LEN               (16)

struct scmi_pd_power_domain_attributes_response_t {
    int32_t status;
    uint32_t attributes;
    uint8_t name[SCMI_POWER_DOMAIN_NAME_LEN];
};

/*
 * POWER_STATE_SET
 */

#define SCMI_PD_POWER_STATE_SET_ASYNC_FLAG_MASK  (1U << 0)
#define SCMI_PD_POWER_STATE_SET_FLAGS_MASK       (1U << 0)
#define SCMI_PD_POWER_STATE_SET_POWER_STATE_MASK UINT32_C(0x4FFFFFFF)

struct scmi_pd_power_state_set_t {
    uint32_t flags;
    uint32_t domain_id;
    uint32_t power_state;
};

struct scmi_pd_power_state_set_response_t {
    int32_t status;
};

/*
 * POWER_STATE_GET
 */

struct scmi_pd_power_state_get_t {
    uint32_t domain_id;
};

struct scmi_pd_power_state_get_response_t {
    int32_t status;
    uint32_t power_state;
};

/*
 * POWER_STATE_NOTIFY
 */
#define SCMI_PD_NOTIFY_ENABLE_MASK UINT32_C(0x1)

struct scmi_pd_power_state_notify_t {
    uint32_t domain_id;
    uint32_t notify_enable;
};

struct scmi_pd_power_state_notify_response_t {
    int32_t status;
};

struct scmi_pd_power_state_notification_t {
    uint32_t agent_id;
    uint32_t domain_id;
    uint32_t power_state;
};

/*!
 * \brief Identifiers of the power domain states. The other states are defined
 *      by the platform code for more flexibility. The power states defined by
 *      the platform must be ordered from the shallowest to the deepest state.
 */

/*! \c OFF power state */
#define MOD_PD_STATE_OFF 0

/*! \c ON power state */
#define MOD_PD_STATE_ON 1

/*! \c SLEEP power state */
#define MOD_PD_STATE_SLEEP 2

/*! \c OFF0 power state */
#define MOD_PD_STATE_OFF_0 3

/*! \c OFF1 power state */
#define MOD_PD_STATE_OFF_1 4

/*! \c OFF2 power state */
#define MOD_PD_STATE_OFF_2 5

/*!
 * \brief Identifiers for the power levels.
 */

/*! Level 0. */
#define MOD_PD_LEVEL_0 0

/*! Level 1. */
#define MOD_PD_LEVEL_1 1

/*! Level 2. */
#define MOD_PD_LEVEL_2 2

/*! Level 3. */
#define MOD_PD_LEVEL_3 3

/*! Number of power domain levels. */
#define MOD_PD_LEVEL_COUNT 4

/*!
 * \brief Number of bits for each level state in a composite power state.
 */
#define MOD_PD_CS_STATE_BITS_PER_LEVEL 4

/*!
 * \brief Shifts for the states and child policies fields in a composite
 *        power state.
 */
#define MOD_PD_CS_LEVEL_0_STATE_SHIFT (MOD_PD_LEVEL_0 * MOD_PD_CS_STATE_BITS_PER_LEVEL)
#define MOD_PD_CS_LEVEL_1_STATE_SHIFT (MOD_PD_LEVEL_1 * MOD_PD_CS_STATE_BITS_PER_LEVEL)
#define MOD_PD_CS_LEVEL_2_STATE_SHIFT (MOD_PD_LEVEL_2 * MOD_PD_CS_STATE_BITS_PER_LEVEL)
#define MOD_PD_CS_LEVEL_3_STATE_SHIFT (MOD_PD_LEVEL_3 * MOD_PD_CS_STATE_BITS_PER_LEVEL)
#define MOD_PD_CS_LEVEL_SHIFT (MOD_PD_LEVEL_COUNT * MOD_PD_CS_STATE_BITS_PER_LEVEL)

/*!
 * \brief Compute a composite power domain state.
 */
#define MOD_PD_COMPOSITE_STATE(HIGHEST_LEVEL, LEVEL_3_STATE, LEVEL_2_STATE, \
                               LEVEL_1_STATE, LEVEL_0_STATE)                \
    (((HIGHEST_LEVEL) << MOD_PD_CS_LEVEL_SHIFT)         | \
     ((LEVEL_3_STATE) << MOD_PD_CS_LEVEL_3_STATE_SHIFT) | \
     ((LEVEL_2_STATE) << MOD_PD_CS_LEVEL_2_STATE_SHIFT) | \
     ((LEVEL_1_STATE) << MOD_PD_CS_LEVEL_1_STATE_SHIFT) | \
     ((LEVEL_0_STATE) << MOD_PD_CS_LEVEL_0_STATE_SHIFT))

#ifdef __cplusplus
}
#endif

/**
 * \brief SCMI agent get power domain protocol version from platform.
 *
 * \param[out] version  protocol version return from platform.
 * \return Error value as defined by scmi_comms_err_t.
 */
scmi_comms_err_t scmi_comm_get_power_domain_version(uint32_t *version);

/**
 * \brief SCMI agent set a power domain state to platform.
 *
 * \param[in] domain_id Identifier for the power domain
 * \param[in] power_domain_state SCMI_PD_DEVICE_STATE_<x> in scmi_power_domain.h
 * \return scmi_comm_err_t Error value as defined by scmi_comms_err_t.
 */
scmi_comms_err_t scmi_comm_power_domain_state_set(uint32_t domain_id,
    uint32_t power_domain_state);

/**
 * \brief SCMI agent get the attributes of a power domain
 *
 * \param[in] domain_id Identifier for the power domain
 * \param[out] domain_attributes Attribute of the power domain
 * \param[out] domain_name Null-terminated ASCII string of up to 16 bytes in
 *             length describing the power domain name
 * \return scmi_comms_err_t
 */
scmi_comms_err_t scmi_comm_power_domain_attributes(uint32_t domain_id,
    uint32_t *domain_attributes, uint8_t *domain_name);

/**
 * \brief SCMI agent get a power domain state from platform.
 *
 * \param[in] domain_id Identifier for the power domain
 * \param[out] power_domain_state SCMI_PD_DEVICE_STATE_<x> in scmi_power_domain.h
 * \return scmi_comm_err_t Error value as defined by scmi_comms_err_t.
 */
scmi_comms_err_t scmi_comm_power_domain_state_get(uint32_t domain_id,
    uint32_t *power_domain_state);

#endif /* __SCMI_POWER_DOMAIN_H__ */
