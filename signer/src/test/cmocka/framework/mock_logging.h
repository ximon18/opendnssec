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
#ifndef TEST_MOCK_LOGGING_H
#define TEST_MOCK_LOGGING_H


#include <stdio.h>
#include <stdbool.h>


#define MOCK_ANNOUNCE()          TEST_LOG("mock") "mock: %s()\n", __func__);
#define TEST_LOG_NOCHECK(level)  fprintf(stderr, "test: log[" level "] "
#define TEST_LOG(level)          if (log_level_enabled(level)) TEST_LOG_NOCHECK(level)


bool log_level_enabled(const char *log_level);
void set_logging_level(const char *log_level);
void set_filtered_tests(const char *filter);


void expect_ods_log_warning(const char* partial_msg);
void expect_ods_log_error(const char* partial_msg);
void expect_ods_log_crit(const char* partial_msg);
void expect_ods_fatal_exit(const char* partial_msg);


#endif