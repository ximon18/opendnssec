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


// ----------------------------------------------------------------------------
// useful constants
// ----------------------------------------------------------------------------
const int SERIAL_BITS = 32;
const uint64_t TWO_POW_32 = 4294967296;         // 2^(SERIAL_BITS)
const uint32_t TWO_POW_31 = 2147483648;         // 2^(SERIAL_BITS - 1)
const uint32_t MAX_SERIAL_ADD = TWO_POW_31 - 1; // 2^(SERIAL_BITS - 1) - 1
const uint32_t MOCK_UNIXTIME_NOW = 1234;


// ----------------------------------------------------------------------------
// helper macros and functions used by the tests.
// ----------------------------------------------------------------------------
#define namedb_update_serial_test_common() \
    {0}; \
    will_return(__wrap_time_now, 0);

static uint32_t rfc1982_serial_increment(uint32_t s, uint32_t n)
{
    // RFC-1982 section "3.1. Addition":
    // Serial numbers may be incremented by the addition of a positive
    // integer n, where n is taken from the range of integers
    // [0 .. (2^(SERIAL_BITS - 1) - 1)].  For a sequence number s, the
    // result of such an addition, s', is defined as
    // 
    //               s' = (s + n) modulo (2 ^ SERIAL_BITS)
    //
    // Addition of a value outside the range
    // [0 .. (2^(SERIAL_BITS - 1) - 1)] is undefined.
    assert_true(n <= MAX_SERIAL_ADD);
    return (uint32_t) (((uint64_t)s + n) % TWO_POW_32);
}

static int rfc1982_serial_comparison(uint32_t i1_32, uint32_t i2_32)
{
    // RFC-1982 section "3.2. Comparison":
    // For the purposes of this definition, consider two integers, i1 and
    // i2, from the unbounded set of non-negative integers, such that i1 and
    // s1 have the same numeric value, as do i2 and s2.  Arithmetic and
    // comparisons applied to i1 and i2 use ordinary unbounded integer
    // arithmetic.
    //
    // Then, s1 is said to be equal to s2 if and only if i1 is equal to i2,
    // in all other cases, s1 is not equal to s2.
    //
    // s1 is said to be less than s2 if, and only if, s1 is not equal to s2,
    // and
    //     (i1 < i2 and i2 - i1 < 2^(SERIAL_BITS - 1)) or
    //     (i1 > i2 and i1 - i2 > 2^(SERIAL_BITS - 1))
    //
    // s1 is said to be greater than s2 if, and only if, s1 is not equal to
    // s2, and
    //     (i1 < i2 and i2 - i1 > 2^(SERIAL_BITS - 1)) or
    //     (i1 > i2 and i1 - i2 < 2^(SERIAL_BITS - 1))
    int64_t i1 = i1_32;
    int64_t i2 = i2_32;

    if (i1 == i2) {
        // s1 equal s2
        return 0;
    } else if ((i1 < i2 && i2 - i1 < TWO_POW_31) ||
               (i1 > i2 && i1 - i2 > TWO_POW_31)) {
        // s1 less than s2 
        return -1;
    } else if ((i1 < i2 && i2 - i1 > TWO_POW_31) ||
               (i1 > i2 && i1 - i2 < TWO_POW_31)) {
        // s1 greater than s2
        return 1;
    } else {
        // RFC-1982 section "3.2. Comparison":
        // Note that there are some pairs of values s1 and s2 for which s1 is
        // not equal to s2, but for which s1 is neither greater than, nor less
        // than, s2.  An attempt to use these ordering operators on such pairs
        // of values produces an undefined result.
        // ...
        // Thus the problem case is left undefined, implementations are free to
        // return either result, or to flag an error, and users must take care
        // not to depend on any particular outcome.  Usually this will mean
        // avoiding allowing those particular pairs of numbers to co-exist.
        //
        // This will happen where (pseudo code): abs(i1 - i2) == 2^31
        // Raise a mock assertion that can be cmocka expected in tests below.
        MOCK_ASSERT(false);
    }
}

static int rfc1982_serial_gt(uint32_t i1, uint32_t i2)
{
    return rfc1982_serial_comparison(i1, i2) > 0;
}

static void test_one_serial_gt(uint32_t i1, uint32_t i2)
{
    assert_int_equal(util_serial_gt(i1, i2), rfc1982_serial_gt(i1, i2));
}


// ----------------------------------------------------------------------------
// the tests:
// TODO: test all the possible permutations of things like have_serial,
// arg_inbound_serial, inbserial, 'format' and outbserial.
// ----------------------------------------------------------------------------
UNIT_TEST_BEGIN(soa_serial_counter)
    namedb_type db = namedb_update_serial_test_common();

    // given a previously seen inbound serial number
    db.have_serial = 1;
    db.intserial = 0;

    // and an inbound serial number that is equal to the previous serial
    uint32_t inbound_serial = db.intserial;

    // when updating the serial number succeeds
    assert_int_equal(
        ODS_STATUS_OK,
        namedb_update_serial(&db, "azone", "counter", inbound_serial));

    // verify that the inbound serial number was incremented by one
    assert_int_equal(db.intserial, inbound_serial + 1);
UNIT_TEST_END

UNIT_TEST_BEGIN(soa_serial_keep_okay)
    namedb_type db = namedb_update_serial_test_common();

    // given a previously seen inbound serial number
    db.have_serial = 1;
    db.intserial = 0;

    // and an inbound serial number that is higher than the previous serial
    uint32_t inbound_serial = db.intserial + 1;

    // when updating the serial number succeeds
    assert_int_equal(
        ODS_STATUS_OK,
        namedb_update_serial(&db, "azone", "keep", inbound_serial));

    // verify that the specified inbound serial number was "kept"
    assert_int_equal(db.intserial, inbound_serial);
UNIT_TEST_END

UNIT_TEST_BEGIN(soa_serial_keep_unusable)
    namedb_type db = namedb_update_serial_test_common();

    // given a previously seen inbound serial number
    db.have_serial = 1;
    db.intserial = 0;

    // and an inbound serial number that is unchanged from the previous serial
    uint32_t inbound_serial = db.intserial;

    // expect an error to be logged
    expect_ods_log_error("cannot keep SOA SERIAL from input zone");

    // when failing to update the serial number
    assert_int_equal(
        ODS_STATUS_CONFLICT_ERR,
        namedb_update_serial(&db, "azone", "keep", inbound_serial));
UNIT_TEST_END

UNIT_TEST_BEGIN(soa_serial_unixtime_gt_now)
    namedb_type db = namedb_update_serial_test_common();

    // given a previously seen inbound serial number
    db.have_serial = 1;
    db.intserial = 0;

    // given that the time now is controlled by us
    set_mock_time_now_value(MOCK_UNIXTIME_NOW);

    // and that the inbound serial number is newer than the time now
    uint32_t inbound_serial = MOCK_UNIXTIME_NOW + 1;

    // when updating the serial number succeeds
    assert_int_equal(
        ODS_STATUS_OK,
        namedb_update_serial(&db, "azone", "unixtime", inbound_serial)),

    // verify that the chosen serial number is greater than both the previous
    // serial and the inbound serial
    assert_int_equal(db.intserial, inbound_serial + 1);
UNIT_TEST_END

UNIT_TEST_BEGIN(soa_serial_unixtime_eq_now)
    namedb_type db = namedb_update_serial_test_common();

    // given a previously seen inbound serial number
    db.have_serial = 1;
    db.intserial = 0;

    // given that the time now is controlled by us
    set_mock_time_now_value(MOCK_UNIXTIME_NOW);

    // and that the inbound serial number is the same as the time now
    uint32_t inbound_serial = MOCK_UNIXTIME_NOW;

    // when updating the serial number succeeds
    assert_int_equal(
        ODS_STATUS_OK,
        namedb_update_serial(&db, "azone", "unixtime", inbound_serial)),

    // verify that the newly chosen serial number has been forced to be one
    // higher than the unusable inbound serial number
    assert_int_equal(db.intserial, inbound_serial + 1);
UNIT_TEST_END

UNIT_TEST_BEGIN(soa_serial_unixtime_lt_now)
    namedb_type db = namedb_update_serial_test_common();

    // given a previously seen inbound serial number
    db.have_serial = 1;
    db.intserial = 0;

    // given that the time now is controlled by us
    set_mock_time_now_value(MOCK_UNIXTIME_NOW);

    // and that the inbound serial number is older than the time now
    uint32_t inbound_serial = MOCK_UNIXTIME_NOW - 1;

    // when updating the serial number succeeds
    assert_int_equal(
        ODS_STATUS_OK,
        namedb_update_serial(&db, "azone", "unixtime", inbound_serial));

    // verify that the new serial is set to the time now
    assert_int_equal(db.intserial, MOCK_UNIXTIME_NOW);
UNIT_TEST_END

UNIT_TEST_BEGIN(serial_gt)
    // loosely based on RFC 1982 section "5.2. A slightly larger example".
    test_one_serial_gt(1, 0);
    test_one_serial_gt(44, 0);
    test_one_serial_gt(100, 0);
    test_one_serial_gt(100, 44);
    test_one_serial_gt(200, 100);
    test_one_serial_gt(TWO_POW_31, TWO_POW_31-55);
    test_one_serial_gt(100, TWO_POW_31);
    test_one_serial_gt(0, TWO_POW_31-55);
    test_one_serial_gt(44, TWO_POW_31-55);
UNIT_TEST_END

UNIT_TEST_BEGIN(serial_gt_incomparable)
    // According to RFC-1982 sections "3.2. Comparison", section "5.1. A
    // trivial example" and "5.2. A slightly larger example" comparison of
    // some serial numbers, specifically those exactly half the serial range
    // apart, is undefined, and that " implementations are free to return
    // either result, or to flag an error". This test documents what ODS does
    // in such cases. Using rfc1982_serial_gt instead of util_serial_gt will
    // fail as it refuses to compare such numbers.
    uint32_t i1s[] = { TWO_POW_31, TWO_POW_31+1, TWO_POW_32-2, TWO_POW_32-1 };
    uint32_t i2s[] = { 0,          1,            TWO_POW_31-2, TWO_POW_31-1 };
    uint32_t idx;

    for (idx = 0; idx < sizeof(i1s)/sizeof(uint32_t); idx++) {
        uint32_t i1 = i1s[idx];
        uint32_t i2 = i2s[idx];
        assert_false(util_serial_gt(i1, i2));
        expect_assert_failure(rfc1982_serial_gt(i1, i2));
    }

    // note that the reverse comparisons give the same result
    for (idx = 0; idx < sizeof(i1s)/sizeof(uint32_t); idx++) {
        uint32_t i1 = i1s[idx];
        uint32_t i2 = i2s[idx];
        assert_false(util_serial_gt(i2, i1));
        expect_assert_failure(rfc1982_serial_gt(i2, i1));
    }
UNIT_TEST_END