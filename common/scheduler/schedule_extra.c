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

/**
 * Task scheduling extra.
 *
 * This module separates functions out of schedule.c to avoid internal
 * references within the same compilation module thereby enabling ld --wrap
 * for mocking.
 * 
 * See: https://sourceware.org/binutils/docs/ld/Options.html#index-_002d_002dwrap_003dsymbol
 */

#include "schedule.h"


// Externalized to enable schedule_task() to be "wrapped", otherwise the call
// by this function to schedule_task() would not be "wrappable".
void
schedule_scheduletask(schedule_type* schedule, task_id type, const char* owner, void* userdata, pthread_mutex_t* resource, time_t when)
{
    task_type* task;
    struct schedule_handler* handler;
    handler = schedule_findregisteredhandler(schedule, type);
    if (handler) {
        task = task_create(strdup(owner), handler->class, type, handler->callback, userdata, NULL, when);
        task->lock = resource;
        schedule_task(schedule, task, 0, 0);
    }
}
