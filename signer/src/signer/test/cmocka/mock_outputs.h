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
#ifndef TEST_MOCK_OUTPUTS_H
#define TEST_MOCK_OUTPUTS_H


/* CMocka expect_check custom parameter checking function for checking the
   state of the zone post signing (when adapter_write() is invoked by worker
   task TASK_WRITE, the task that immediately and automatically follows task
   TASK_SIGN).

   Example:

   DEFINE_SIGNED_ZONE_CHECK(expect_sig_count, zone->stats->sig_count, !=);

   void some_test(...) {
       expect_sig_count(14);
       worker_perform_task(state->worker);
   }

   When worker_perform_task() invokes adapter_write for TASK_WRITE, post
   TASK_SIGN, field zone->stats->sig_count will be compared to value 15 and the
   test will fail if they are not "not equal".

   On failure the log will contain:

       test: log[crit] expect_sig_count: check zone->stats->sig_count != expected failed because 15 != 14
*/

#define DEFINE_SIGNED_ZONE_CHECK(func_name, test_field, test_op) \
    int func_name ## _expect_check_custom_parameter_checking_func( \
        const LargestIntegralType value, const LargestIntegralType expected) \
    { \
        zone_type *zone = (zone_type *) value; \
        if (test_field test_op expected) { \
            TEST_LOG("crit") #func_name ": check " \
            #test_field " " #test_op " expected " \
            "failed because %d != %d\n", test_field, expected); \
            return 0; \
        } else { \
            return 1; \
        } \
    } \
    void func_name(const LargestIntegralType expected) \
    { \
        expect_check( \
            __wrap_adapter_write, \
            zone, \
            func_name ## _expect_check_custom_parameter_checking_func, \
            expected); \
    }


#endif