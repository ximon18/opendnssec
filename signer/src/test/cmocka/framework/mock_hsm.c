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
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <stdio.h>
#include "libhsm.h"
#include "libhsmdns.h"
#include "mock_logging.h"
#include "mock_hsm.h"
#include "mock_worker.h"
#include "test_framework.h"


const char *MOCK_HSM_MODULE_NAME = "MOCK_MODULE";


void configure_mock_hsm(const e2e_test_state_type* state)
{
    will_return_always(__wrap_hsm_create_context, state->hsm_ctx);
}


// ----------------------------------------------------------------------------
// Plain mocks.
// ----------------------------------------------------------------------------
CK_RV mock_C_DigestInit(unsigned long session, CK_MECHANISM_PTR m) {
    MOCK_ANNOUNCE();
    return CKR_OK;
}
CK_RV mock_C_Digest(unsigned long session, unsigned char* data,
    unsigned long data_len, unsigned char* digest, unsigned long *digest_len) {
    static unsigned char MOCK_DIGEST[] = { 0xD, 0xE, 0xA, 0xD, 0xB, 0xE, 0xE, 0xF };
    MOCK_ANNOUNCE();
    memcpy(digest, MOCK_DIGEST, sizeof(MOCK_DIGEST));
    *digest_len = sizeof(MOCK_DIGEST);
    return CKR_OK;
}
CK_RV mock_C_SignInit(unsigned long session, CK_MECHANISM_PTR m, unsigned long pk) {
    MOCK_ANNOUNCE();
    return CKR_OK;
}
CK_RV mock_C_Sign(unsigned long session, unsigned char* data,
    unsigned long data_len, unsigned char* signature, unsigned long *signature_len) {
    static unsigned char MOCK_SIGNATURE[] = { 0xD, 0xE, 0xA, 0xD, 0xB, 0xE, 0xE, 0xF };
    MOCK_ANNOUNCE();
    function_called();
    memcpy(signature, MOCK_SIGNATURE, sizeof(MOCK_SIGNATURE));
    *signature_len = sizeof(MOCK_SIGNATURE);
    return CKR_OK;
}


// ----------------------------------------------------------------------------
// monkey patches:
// these require compilation with -Wl,--wrap=ods_log_debug,--wrap=xxx etc to 
// cause these wrapper implementations to replace the originals.
// ----------------------------------------------------------------------------
const libhsm_key_t* __wrap_keycache_lookup(hsm_ctx_t* ctx, const char* locator) {
    MOCK_ANNOUNCE();
    test_keys_type *keys = e2e_get_mock_keys();
    for (int i = 0; i < e2e_get_mock_key_count(); i++) {
        if (0 == strcmp(keys[i].locator, locator)) {
            return keys[i].hsm_key;
        }
    }
    fail_msg("No key with locator %s found\n", locator);
}
int __wrap_hsm_open2(hsm_repository_t* rlist, char *(pin_callback)(unsigned int, const char *, unsigned int))
{
    MOCK_ANNOUNCE();
    return HSM_OK;
}
void __wrap_hsm_close()
{
    MOCK_ANNOUNCE();
}
hsm_ctx_t * __wrap_hsm_create_context()
{
    MOCK_ANNOUNCE();
    return mock();
}
int __wrap_hsm_check_context()
{
    MOCK_ANNOUNCE();
    return HSM_OK;
}
void __wrap_hsm_destroy_context(hsm_ctx_t *ctx)
{
    MOCK_ANNOUNCE();
    // Destruction is handled by the test teardown function.
}
ldns_rr * __wrap_hsm_get_dnskey(hsm_ctx_t *ctx, const libhsm_key_t *key, const hsm_sign_params_t *sign_params)
{
    MOCK_ANNOUNCE();
    test_keys_type *keys = e2e_get_mock_keys();

    // abuse the private key field for test purposes
    int mock_key_idx = key->private_key;
    int mock_key_count = e2e_get_mock_key_count();
    if (mock_key_idx >= mock_key_count) {
        fail_msg("Mock key index %d out of range, only %d mock keys exist!\n", mock_key_idx, mock_key_count);
    } else {
        return keys[mock_key_idx].ldns_rr;
    }
}
char * __wrap_hsm_get_error(hsm_ctx_t *gctx)
{
    return NULL;
}


hsm_ctx_t *setup_mock_hsm()
{
    // define a mock pkcs#11 function symbol table for use with the HSM context
    // built below.
    CK_FUNCTION_LIST_PTR mock_sym_table = calloc(1, sizeof(CK_FUNCTION_LIST));
    mock_sym_table->C_DigestInit = mock_C_DigestInit;
    mock_sym_table->C_Digest = mock_C_Digest;
    mock_sym_table->C_SignInit = mock_C_SignInit;
    mock_sym_table->C_Sign = mock_C_Sign;

    // prepare a mock HSM context so that we can use libhsm functionality
    // without a real/soft HSM.
    hsm_ctx_t *ctx = calloc(1, sizeof(hsm_ctx_t));
    ctx->session_count = 1;
    ctx->session[0] = calloc(1, sizeof(hsm_session_t));
    ctx->session[0]->session = 0;
    ctx->session[0]->module = calloc(1, sizeof(hsm_module_t));
    ctx->session[0]->module->name = strdup(MOCK_HSM_MODULE_NAME);
    ctx->session[0]->module->sym = mock_sym_table;

    return ctx;
}

void teardown_mock_hsm(const hsm_ctx_t *ctx)
{
    assert_non_null(ctx);
    free(ctx->session[0]->module->sym);
    free(ctx->session[0]->module->name);
    free(ctx->session[0]->module);
    free(ctx->session[0]);
    free(ctx);
}