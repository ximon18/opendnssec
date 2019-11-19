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
#include "test_framework.h"
#include "mock_outputs.h"
#include "signer/zone.h"
#include <alloca.h>
#include <stdlib.h>


const char *ZONE_TEMPLATE = R"(
$TTL	86400 ; 24 hours could have been written as 24h or 1d
; $TTL used for all RRs without explicit TTL value
$ORIGIN <ZONE_NAME> 
@  1D  IN  SOA ns1.example.com. hostmaster.example.com. (
                <ZONE_SERIAL>; serial
                3H ; refresh
                15 ; retry
                1w ; expire
                3h ; nxdomain ttl
                )
    IN  NS     ns1.example.com. ; in the domain
    IN  NS     ns2.smokeyjoe.com. ; external to domain
    IN  MX  10 mail.another.com. ; external mail provider
; server host definitions
ns1    IN  A      192.168.0.1  ;name server definition     
www    IN  A      192.168.0.2  ;web server definition
ftp    IN  CNAME  www.example.com.  ;ftp server definition
; non server domain hosts
bill   IN  A      192.168.0.3
fred   IN  A      192.168.0.4
)";


// Define our own assert_int_equal() because with CMocka() the printed error
// message just indicates the failing line of code, not what the line says and
// thus nothing about what actually failed, at least not at first glance, you
// have to go and read that line of code to understand what was being tested.
void assert_uint32_ptr_equal(const char* field_name, uint32_t *actual, uint32_t *expected)
{
    if (!actual && !expected) return;
    if ((!actual && expected) || (actual && !expected) || (*actual != *expected)) {
        char actual_str[128], expected_str[128];
        if (actual)   snprintf(actual_str, 128, "%d", *actual);
        if (expected) snprintf(expected_str, 128, "%d", *expected);
        fail_msg("%s value %s != %s", field_name,
            actual ? actual_str : "NULL",
            expected ? expected_str : "NULL");
    }
}

// Define a compound checker, because with CMocka it's not possible to register
// multiple checkers which in sequence check the same parameter (in this case
// fields of the same struct parameter ) to the same function.
#define ASSERT_UINT32_PTR_EQUAL(actual, expected) \
    assert_uint32_ptr_equal(#actual, actual, expected)

int check_zone_serials(zone_type *actual, zone_type *expected)
{
    assert_string_equal(actual->name, expected->name);
    ASSERT_UINT32_PTR_EQUAL(actual->nextserial,     expected->nextserial);
    ASSERT_UINT32_PTR_EQUAL(actual->inboundserial,  expected->inboundserial);
    // ASSERT_UINT32_PTR_EQUAL(actual->outboundserial, expected->outboundserial);
}

// DEFINE_SIGNED_ZONE_CHECK_RETURNS_ZERO(expect_our_zone_is_output, strcmp(zone->name, MOCK_ZONE_NAME));
// DEFINE_SIGNED_ZONE_CHECK_WITH_OP(expect_next_serial, zone->nextserial ? *(zone->nextserial) : fail()), !=);
// DEFINE_SIGNED_ZONE_CHECK_WITH_OP(expect_inbound_serial, zone->inboundserial ? *(zone->inboundserial) : fail()), !=);
// DEFINE_SIGNED_ZONE_CHECK_WITH_OP(expect_outbound_serial, zone->outboundserial ? *(zone->outboundserial) : fail()), !=);
DEFINE_SIGNED_ZONE_CHECK_WITH_FUNC(expect_output_zone_serials, check_zone_serials);


// This cannot be used at present with the develop branch because sig_count is
// never updated. This is a regression compared to the 2.1.x branch.
// DEFINE_SIGNED_ZONE_CHECK(expect_sig_count, zone->stats->sig_count, !=);


#define DUP_SERIAL(serial) dup_serial(serial, alloca(sizeof(uint32_t)))
uint32_t *dup_serial(uint32_t serial, uint32_t *new_mem) {
    *new_mem = serial;
    return new_mem;
}


E2E_TEST_BEGIN(load_zone_file)
    const char *zone_soa_serial = "2019111801";
    uint32_t zone_soa_serial_num = atol(zone_soa_serial);
    const char *replacements[] = { MOCK_ZONE_NAME, zone_soa_serial };
    const char *test_zone = from_template(ZONE_TEMPLATE, replacements);

    // Given an unsigned input zone
    zone_type *zone = e2e_configure_mocks(state, test_zone, E2E_VIEWSTATE_W_OKAY);

    // When it is signed using the 'datecounter' soa serial algorithm
    zone->signconf->soa_serial = strdup("datecounter");
    zone->signconf->last_modified = 1;
    will_return_always(__wrap_time_now, __real_time_now());

    // Expect the serial number to be
    expect_output_zone_serials(&((zone_type) {
        .name           = zone->name,
        .nextserial     = NULL,
        .inboundserial  = DUP_SERIAL(zone_soa_serial_num),
        .outboundserial = DUP_SERIAL(zone_soa_serial_num+1)
    }));

    // Go!
    e2e_go(state, TASK_READ, TASK_SIGN, TASK_WRITE, TASK_STOP);
E2E_TEST_END


E2E_TEST_BEGIN(load_zone_file2)
    const char *zone_soa_serial = "2019111801";
    uint32_t zone_soa_serial_num = atol(zone_soa_serial);
    const char *replacements[] = { MOCK_ZONE_NAME, zone_soa_serial };
    const char *test_zone = from_template(ZONE_TEMPLATE, replacements);

    // sleeping >0.5 seconds and then loading state which contains an expiry
    // time for a record in the zone to be signed (in this case bill.MOCKZONE.)
    // causes an assert:
    // src/views/recordset.c:623: names_recordsetexpiry: Assertion `record->expiry == NULL' failed.
    usleep(600000);

    // Given an unsigned input zone and prior state
    zone_type *zone = e2e_configure_mocks(state, test_zone, E2E_VIEWSTATE_R_OKAY);

    // When it is signed using the 'datecounter' soa serial algorithm
    zone->signconf->soa_serial = strdup("datecounter");
    zone->signconf->last_modified = 1;
    will_return_always(__wrap_time_now, __real_time_now());

    // Expect the serial number to be
    expect_output_zone_serials(&((zone_type) {
        .name           = zone->name,
        .nextserial     = NULL,
        .inboundserial  = DUP_SERIAL(zone_soa_serial_num),
        .outboundserial = DUP_SERIAL(zone_soa_serial_num+1)
    }));

    // Go!
    e2e_go(state, TASK_READ, TASK_SIGN, TASK_WRITE, TASK_STOP);
E2E_TEST_END


E2E_TEST_BEGIN(cleanup_view_state_tmp_file)
    unlink(MOCK_VIEW_STATE_PATH);
E2E_TEST_END