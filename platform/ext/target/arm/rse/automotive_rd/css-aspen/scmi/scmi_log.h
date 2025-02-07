/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 */

#ifndef __SCMI_LOG_H__
#define __SCMI_LOG_H__

#ifdef SCMI_COMMS_FOR_BL2_POLLING_MODE

#include "bootutil/bootutil_log.h"

#define BL2_LOG_PREFIX "BL2: "

#define SCMI_LOG_INF(...) BOOT_LOG_INF(BL2_LOG_PREFIX __VA_ARGS__)
#define SCMI_LOG_DBG(...) BOOT_LOG_DBG(BL2_LOG_PREFIX __VA_ARGS__)
#define SCMI_LOG_ERR(...) BOOT_LOG_ERR(BL2_LOG_PREFIX __VA_ARGS__)

#endif
#ifdef SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS

#include "tfm_log_unpriv.h"

#define SYS_LOG_PREFIX "[SYS][SCMI] "
#define INF_LOG_PREFIX "[INF][SCMI] "
#define DBG_LOG_PREFIX "[DBG][SCMI] "
#define ERR_LOG_PREFIX "[ERR][SCMI] "
#define NEWLINE_SUFFIX "\r\n"

#define SCMI_LOG_SYS(fmt, ...) printf(SYS_LOG_PREFIX fmt NEWLINE_SUFFIX, ##__VA_ARGS__)
#define SCMI_LOG_INF(fmt, ...) INFO_UNPRIV_RAW(INF_LOG_PREFIX fmt NEWLINE_SUFFIX, ##__VA_ARGS__)
#define SCMI_LOG_DBG(fmt, ...) VERBOSE_UNPRIV_RAW(DBG_LOG_PREFIX fmt NEWLINE_SUFFIX, ##__VA_ARGS__)
#define SCMI_LOG_ERR(fmt, ...) ERROR_UNPRIV_RAW(ERR_LOG_PREFIX fmt NEWLINE_SUFFIX, ##__VA_ARGS__)

#endif

#endif /* __SCMI_LOG_H__ */
