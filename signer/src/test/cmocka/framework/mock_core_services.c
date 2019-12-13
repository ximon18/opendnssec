/*
 * Copyright (c) 2009 NLNet Labs. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    notice, this list
  of conditions and the following disclaimer.
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
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>


#include "test_framework.h"


#define IOBUF_DEF_SIZE          2048
static const char *MODE_R     = "r";
static const char *MODE_RPLUS = "r+";
static const char *MODE_W     = "w";
static const char *MODE_WPLUS = "w+";
static const char *MODE_A     = "a";
static const char *MODE_APLUS = "a+";


typedef struct  {
    char *filename;
    char *buf;
    size_t size;
} mock_io_buf;

static mock_io_buf* mock_io_buffers = NULL;
static int mock_io_buffers_count = 0;


// ----------------------------------------------------------------------------
static mock_io_buf * get_mock_io_buffer(const char* filename);
static mock_io_buf * get_or_add_mock_io_buffer(const char *filename, size_t size);
static FILE *        get_file_ptr_by_mode(const char *filename, const char* modes);
static const char *  open_flag_2_mode(int flag);


// ----------------------------------------------------------------------------
// public functions:
// ----------------------------------------------------------------------------

void set_mock_time_now_value(time_t t)
{
    char str[21];
    struct tm *tmp;
    tmp = localtime(&t);
    if (tmp && strftime(str, sizeof(str), "%Y-%m-%d %H:%M:%S", tmp) > 0) {
        TEST_LOG("mock") "Advancing time to %d (%s)\n", t, str);
        // Change every time_now() value returned, as the caller doesn't know
        // how many times time_now() will be called. To change the current time
        // during a test use modified CMocka which allows will_return_always()
        // to be called again to update the always return value. See:
        //   https://gitlab.com/cmocka/cmocka/merge_requests/21
        will_return_always(__wrap_time_now, t);
    } else {
        fail();
    }
}

void set_mock_time_now_from_str(char *str)
{
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    strptime(str, "%Y-%m-%d %H:%M:%S", &tm);
    set_mock_time_now_value(mktime(&tm));
}

void cleanup_mock_io_buffers(void)
{
    for (int i = 0; i < mock_io_buffers_count; i++) {
        mock_io_buf *buf = &mock_io_buffers[i];
        free(buf->filename);
        free(buf->buf);
        buf->size = 0;
    }
    free(mock_io_buffers);
}

// filename and contents will be copied so the caller remains responsible for
// free()'ing their copies of these buffers.
void set_mock_io_buffer(const char* filename, const char* contents)
{
    size_t size = strlen(contents);
    mock_io_buf *buf = get_or_add_mock_io_buffer(filename, size);

    if (!buf) fail();

    if (buf->size != size) {
        buf->buf = realloc(buf->buf, size);
        buf->size = size;
    }

    strncpy(buf->buf, contents, size); // does NOT copy the NULL terminator
}

// ----------------------------------------------------------------------------
// internal functions:
// ----------------------------------------------------------------------------

static mock_io_buf * get_mock_io_buffer(const char* filename)
{
    for (int i = 0; i < mock_io_buffers_count; i++) {
        if (0 == strcmp(mock_io_buffers[i].filename, filename)) {
            return &mock_io_buffers[i];
        }
    }
    return NULL;
}

static mock_io_buf * get_or_add_mock_io_buffer(const char *filename, size_t size)
{
    mock_io_buf *buf = get_mock_io_buffer(filename);
    if (!buf) {
        mock_io_buffers = reallocarray(mock_io_buffers, mock_io_buffers_count+1, sizeof(mock_io_buf));
        if (!mock_io_buffers) fail();
        buf = &mock_io_buffers[mock_io_buffers_count++];
        buf->size = size;
        buf->filename = strdup(filename); if (!buf->filename) fail();
        buf->buf = calloc(1, size); if (!buf->buf) fail();
    }
    return buf;
}

static FILE * get_file_ptr_by_mode(const char *filename, const char* modes)
{
    // in read mode the buffer must already exist, otherwise it is okay to
    // create an empty one if it doesn't already exist.
    mock_io_buf *buf = (0 == strcmp(modes, "r") ?
        get_mock_io_buffer(filename) : 
        get_or_add_mock_io_buffer(filename, IOBUF_DEF_SIZE));
    return buf ? fmemopen(buf->buf, buf->size, modes) : NULL;
}

static const char * open_flag_2_mode(int flag)
{
    // mapped per rules described in man fopen
    if ((flag & (O_WRONLY|O_CREAT|O_TRUNC)) == (O_WRONLY|O_CREAT|O_TRUNC))        return MODE_W;
    else if ((flag & (O_WRONLY|O_CREAT|O_APPEND)) == (O_WRONLY|O_CREAT|O_APPEND)) return MODE_A;
    else if ((flag & (O_RDWR|O_CREAT|O_TRUNC)) == (O_RDWR|O_CREAT|O_TRUNC))       return MODE_WPLUS;
    else if ((flag & (O_RDWR|O_CREAT|O_APPEND)) == (O_RDWR|O_CREAT|O_APPEND))     return MODE_APLUS;
    else if ((flag & O_RDWR) == O_RDWR)                                           return MODE_RPLUS;
    else                                                                          return MODE_R;
}


// ----------------------------------------------------------------------------
// monkey patches:
// these require compilation with -Wl,--wrap=ods_log_debug,--wrap=xxx etc to 
// cause these wrapper implementations to replace the originals.
// ----------------------------------------------------------------------------

int __wrap_pthread_cond_init (pthread_cond_t *__restrict __cond,
			      const pthread_condattr_t *__restrict __cond_attr)
{
    return 0;
}
int __wrap_pthread_mutex_init (pthread_mutex_t *__mutex,
			       const pthread_mutexattr_t *__mutexattr)
{
    return 0;
}
int __wrap_pthread_mutex_lock (pthread_mutex_t *__mutex)
{
    return 0;
}
int __wrap_pthread_mutex_unlock (pthread_mutex_t *__mutex)
{
    return 0;
}
int __wrap_pthread_cond_broadcast (pthread_cond_t *__cond)
{
    return 0;
}
time_t __wrap_time_now(void)
{
    MOCK_ANNOUNCE();
    return mock();
}
FILE* __wrap_ods_fopen(const char* file, const char* dir, const char* mode)
{
    MOCK_ANNOUNCE();
    return get_file_ptr_by_mode(file, mode);
}
ods_status __wrap_privdrop(const char *username, const char *groupname, const char *newroot, uid_t* puid, gid_t* pgid)
{
    MOCK_ANNOUNCE();
    return ODS_STATUS_OK;
}
void __wrap_ods_chown(const char* file, uid_t uid, gid_t gid, int getdir)
{
    MOCK_ANNOUNCE();
}
int __wrap_util_write_pidfile(const char* pidfile, pid_t pid)
{
    MOCK_ANNOUNCE();
    return 0;
}
ods_status __wrap_parse_file_check(const char* cfgfile, const char* rngfile)
{
    MOCK_ANNOUNCE()
    return ODS_STATUS_OK;
}

// Wrap direct I/O calls to enable interception of zoneoutput.c file access.
FILE* __wrap_fopen(const char *__filename, const char * __modes)
{
    MOCK_ANNOUNCE();
    return get_file_ptr_by_mode(__filename, __modes);
}

int __wrap_rename(const char *__old, const char *__new)
{
    MOCK_ANNOUNCE();
    // pretend to have done the rename
    return 0;
}

// Wrap direct I/O calls to enable interception of views.c and marshalling.c
// direct .state(.tmp) file access.
int __wrap_open(const char *__path, int __oflag, ...)
{
    MOCK_ANNOUNCE()
    const char *file_ext = strchr(__path, '.');
    const char *mock_target = NULL;
    char *mock_action = NULL;
    e2e_view_state_mode mode = mock();

    if ((__oflag & O_RDWR) && 0 == strcmp(file_ext, ".state")) {
        mock_action = "READ";
        if (mode & E2E_VIEWSTATE_R_OKAY) {
            // Allow ODS to read view state
            mock_target = MOCK_VIEW_STATE_PATH;
        }
    }
    else if ((__oflag & O_CREAT) && 0 == strcmp(file_ext, ".state.tmp")) {
        mock_action = "WRITE";
        if (mode & E2E_VIEWSTATE_W_OKAY) {
            // Allow ODS to write view state
            mock_target = MOCK_VIEW_STATE_PATH;
        } else {
            // Prevent ODS from writing view state, but pretend to allow it
            // otherwise it gets upset
            mock_target = MOCK_FD;
        }
    }

    if (MOCK_FD == mock_target) {
        TEST_LOG("mock") "Preventing open(%s) view state by pretending to open() '%s'\n", mock_action, __path);
        return MOCK_FD;
    } else if (mock_target) {
        TEST_LOG("mock") "Redirecting open(%s) '%s' to open '%s' instead\n", mock_action, __path, mock_target);
        // We don't know how many arguments were passed but in this specific
        // case just for restoring a state file there are no additional
        // arguments. The calling code at the time of writing sets mode 0666
        // and we fail to read the file in a later test if we don't do the
        // same.
        return __real_open(mock_target, __oflag, 0666);
    } else {
        TEST_LOG("mock") "Deliberately failing open() '%s'\n", __path);
        return -1;
    }
}
int __wrap_openat(int __fd, const char *__path, int __oflag, ...)
{
    MOCK_ANNOUNCE()
    TEST_LOG("mock") "Deliberately failing to openat() '%s'\n", __path);
    return -1;
}
ssize_t __wrap_read(int __fd, void *__buf, size_t __nbytes)
{
    MOCK_ANNOUNCE()
    return __real_read(__fd, __buf, __nbytes);
}
ssize_t __wrap_write(int __fd, const void *__buf, size_t __n)
{
    // don't announce -- too verbose
    if (__fd == MOCK_FD) {
        // pretend to have successfully written the given number of bytes
        return __n;
    } else {
        return __real_write(__fd, __buf, __n);
    }
}
int __wrap_close(int __fd)
{
    MOCK_ANNOUNCE()
    return __fd == MOCK_FD ? 0 : __real_close(__fd);
}
int __wrap_renameat(
    int __oldfd, const char *__old, int __newfd, const char *__new)
{
    MOCK_ANNOUNCE();
    // pretend to have done the rename
    return 0;
}