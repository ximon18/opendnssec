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
#include "signer/zone.h"


const char *test_zone = R"(
$TTL	86400 ; 24 hours could have been written as 24h or 1d
; $TTL used for all RRs without explicit TTL value
$ORIGIN )" MOCK_ZONE_NAME R"( 
@  1D  IN  SOA ns1.example.com. hostmaster.example.com. (
			      2002022401 ; serial
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



static int check_sig_count(
    const LargestIntegralType value, const LargestIntegralType check_value_data)
{
    zone_type *zone = (zone_type *) value;
    int expected_sig_count = (int) check_value_data;
    if (zone->stats->sig_count == expected_sig_count) {
        return 1;
    } else {
        TEST_LOG("crit") "check_sig_count: %d != %d\n", zone->stats->sig_count, expected_sig_count);
        return 0;
    }
}


void e2e_test_load_zone_file(e2e_test_state_type** cmocka_state)
{
    e2e_test_state_type *state = *cmocka_state;

    // Given an unsigned input zone
    e2e_configure_mocks(state, TASK_READ, test_zone);
    zone_type *zone = state->worker->task->zone;
    zone->signconf->soa_serial = strdup("datecounter");
    zone->signconf->last_modified = 1;
    will_return_always(__wrap_time_now, __real_time_now());

    assert_int_equal(0, zone->stats->sig_count);

    // After signing expect: 15 RRSIGs and no errors
    expect_check(__wrap_adapter_write, zone, check_sig_count, 15);

    // Go!
    worker_perform_task(state->worker);
}