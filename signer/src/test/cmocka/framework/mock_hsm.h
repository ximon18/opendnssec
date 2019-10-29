/*
 * Copyright (c) 2009 NLNet Labs. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THI
 * S SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef TEST_MOCK_HSM_H
#define TEST_MOCK_HSM_H


#include <pkcs11.h>
#include "libhsm.h"
#include "test_framework.h"


extern const char *MOCK_HSM_MODULE_NAME;


hsm_ctx_t * setup_mock_hsm();
void        teardown_mock_hsm(const hsm_ctx_t *hsm_ctx);
void        configure_mock_hsm(const e2e_test_state_type* state);
hsm_ctx_t * get_mock_hsm_context();


CK_RV mock_C_DigestInit(unsigned long session, CK_MECHANISM_PTR m);
CK_RV mock_C_Digest(unsigned long session, unsigned char* data,
    unsigned long data_len, unsigned char* digest, unsigned long *digest_len);
CK_RV mock_C_SignInit(unsigned long session, CK_MECHANISM_PTR m, unsigned long pk);
CK_RV mock_C_Sign(unsigned long session, unsigned char* data,
    unsigned long data_len, unsigned char* signature, unsigned long *signature_len);


#endif