/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __FWU_PRIVATE_METADATA_H_
#define __FWU_PRIVATE_METADATA_H_

#include "psa/error.h"
#include "psa/update.h"

#include <stdint.h>

/*
 * Represents both PSA_FWU_WRITING and PSA_FWU_CANDIDATE states
 * as combined state for Shim layer
 */
#define PSA_FWU_WRITING_CANDIDATE   (8u)

/**
 * Aspen specific FWU Private metadata is stored in RSE FLASH
 */
struct fwu_private_metadata {
    /* The bank from which system is booted from */
    uint8_t boot_index;

    /* FWU state of image component */
    uint8_t fwu_image_state[FWU_COMPONENT_NUMBER];
};

psa_status_t fwu_private_metadata_init(void);
psa_status_t fwu_private_metadata_deinit(void);
psa_status_t fwu_private_metadata_read(struct fwu_private_metadata *metadata);
psa_status_t fwu_private_metadata_write(const struct fwu_private_metadata *metadata);

#endif /* __FWU_PRIVATE_METADATA_H_ */
