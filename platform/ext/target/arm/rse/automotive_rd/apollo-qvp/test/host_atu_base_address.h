/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdint.h>
extern uint64_t test_mailbox[32];
#define HOST_RSE_SI_SSRAM_ATU_BASE_S ((uintptr_t)test_mailbox)
