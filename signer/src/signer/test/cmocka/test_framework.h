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
#ifndef TEST_MOCKS_H
#define TEST_MOCKS_H


// cmocka required includes
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>


#include "daemon/worker.h"
#include "signer/zone.h"
#include "mock_logging.h"
#include "mock_inputs.h"


#define MOCK_ZONE_NAME "mockzone."


typedef struct e2e_test_state_struct {
    worker_type *worker;
    hsm_ctx_t   *hsm_ctx;
} e2e_test_state_type;


typedef struct test_keys_struct {
    ldns_key *  ldns_key;
    ldns_rr *   ldns_rr;
    char *      locator;
    hsm_key_t * hsm_key;
} test_keys_type;


#define MOCK_ASSERT(expression)  mock_assert((int)(expression), #expression, __FILE__, __LINE__);
#define MOCK_POINTER             (void*)0xDEADBEEF


test_keys_type* e2e_get_mock_keys(void);
int             e2e_get_mock_key_count(void);
void            e2e_init_mock_key(const char *locator, const zone_type *zone, bool is_ksk, bool is_zsk);
int             e2e_setup(void** state);
int             e2e_teardown(e2e_test_state_type** state);
void            e2e_configure_mocks(const e2e_test_state_type* state, const task_id task_id, const char *input_zone);


#endif