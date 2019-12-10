/*
 * Copyright (c) 2009-2018 NLNet Labs.
 * All rights reserved.
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
 */

/**
 *
 * Durations: code split out to avoid internal function calls within the same
 * file thereby preventing "monkey patchin" via -Wl,--wrap=ods_log_debug,--wrap=xxx.
 */
#include "duration.h"

static const char* duration_str = "duration2";

/**
 * copycode: This code is based on the EXAMPLE in the strftime manual.
 *
 */
uint32_t
time_datestamp(time_t tt, const char* format, char** str)
{
    time_t t;
    struct tm datetime;
    struct tm *tmp;
    uint32_t ut = 0;
    char outstr[32];

    if (tt) {
        t = tt;
    } else {
        t = time_now();
    }

    tmp = localtime_r(&t,&datetime);
    if (tmp == NULL) {
        ods_log_error("[%s] time_datestamp: localtime_r() failed", duration_str);
        return 0;
    }

    if (strftime(outstr, sizeof(outstr), format, tmp) == 0) {
        ods_log_error("[%s] time_datestamp: strftime() failed", duration_str);
        return 0;
    }

    ut = (uint32_t) strtoul(outstr, NULL, 10);
    if (str) {
        *str = strdup(outstr);
    }
    return ut;
}