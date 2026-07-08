/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mbedtls/nist_kw.h"
#include "psa/crypto.h"

#include <string.h>

psa_status_t mbedcrypto__psa_unwrap_key(const psa_key_attributes_t *attributes,
                                         psa_key_id_t wrapping_key,
                                         psa_algorithm_t alg,
                                         const uint8_t *data,
                                         size_t data_length,
                                         psa_key_id_t *key)
{
    mbedtls_svc_key_id_t wrapping_svc_key;
    mbedtls_svc_key_id_t imported_key;
    uint8_t unwrapped_key[PSA_CIPHER_MAX_KEY_LENGTH];
    size_t unwrapped_key_length = 0;
    psa_status_t status;

    (void)alg;

    if (attributes == NULL || data == NULL || key == NULL) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    wrapping_svc_key = mbedtls_svc_key_id_make(0, wrapping_key);

    status = mbedtls_nist_kw_unwrap(wrapping_svc_key, MBEDTLS_KW_MODE_KW,
                                    data, data_length, unwrapped_key,
                                    sizeof(unwrapped_key),
                                    &unwrapped_key_length);
    if (status != PSA_SUCCESS) {
        memset(unwrapped_key, 0, sizeof(unwrapped_key));
        return status;
    }

    imported_key = MBEDTLS_SVC_KEY_ID_INIT;
    status = psa_import_key(attributes, unwrapped_key,
                            unwrapped_key_length, &imported_key);
    memset(unwrapped_key, 0, sizeof(unwrapped_key));
    if (status != PSA_SUCCESS) {
        return status;
    }

    *key = MBEDTLS_SVC_KEY_ID_GET_KEY_ID(imported_key);

    return PSA_SUCCESS;
}
