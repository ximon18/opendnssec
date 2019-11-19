#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

// For each character in 'accept' (e.g. "<>") invoke the given yield function
// when matched in 's' with:
//   - n          : the zero-based count of the number of matches so far
//   - accumulator: a user defined void* value, e.g. for accumulating some
//                  value over the found matches. When:
//                    n == 0: accumulator = accumulator_init
//                    n  > 0: accumulator = value returned last by yield_func
//   - s          : When:
//                    n == 0: s = s as passed to with_splits()
//                    n  > 0: s = match_ptr passed last to yield_func
//   - match_ptr  : Pointer to the matched 'accept' character in s
static void * with_splits(const char *s, const char* accept,
    void * (*yield_func)(int n, void *acc, const char *s, const char* match_ptr, void *data),
    void *data, void *acc_init)
{
    char *start = s, *match_ptr = s;
    void *acc = acc_init;
    int n = 0;
    while (match_ptr = strpbrk(start = match_ptr+1, accept)) {
        acc = yield_func(n++, acc, start, match_ptr, data);
    }
    return yield_func(n, acc, start, NULL, data);
}

// See: from_template().
// To be passed as 'yield_func' to with_splits(accept="<>"). Accumulates the
// number of bytes required to store the search string with <placeholder>
// values replaced by strings taken sequentially from the 'replacements' array.
// The 'replacements' array should be passed to with_splits() as 'data'.
static int * cb_accumulate_required_size(
    int n, int *acc, const char *s, const char *match_ptr, char **replacements)
{
    if (NULL == match_ptr) {
        // no match, the search hit the end of the string.
        // measure how many characters remain that we would have to copy from
        // the template into the final string, n should be even
        assert(n % 2 == 0);
        (*acc) += strlen(s);
    } else if ('<' == *match_ptr) {
        // start of replacement placeholder, n should be even
        // we need space for the text until here
        assert(n % 2 == 0);
        (*acc) += (match_ptr - s);
    } else if ('>' == *match_ptr) {
        // end of replacement placeholder, n should be odd
        assert(n % 2 == 1);
        (*acc) += strlen(replacements[n>>1]);
    } else {
        assert(false);
    }

    return acc;
}

// See: from_template().
// Appends fragments from the search string into 'dst', unchanged except for
// <placeholder>s being replaced by strings taken sequentially from the from
// the 'replacements' array. // The 'replacements' array should be passed to
// with_splits() as 'data'.
static char * cb_replace_placeholders(
    int n, char *dst, const char *s, const char *match_ptr, char **replacements)
{
    if (NULL == match_ptr) {
        // no match, copy the rest of the string
        strcpy(dst, s);
    } else if ('<' == *match_ptr) {
        // start of replacement placeholder, append text until here
        // and the placeholder replacement string
        int template_fragment_len = match_ptr - s;
        strncpy(dst, s, template_fragment_len);
        dst += template_fragment_len;
        strcpy(dst, replacements[n>>1]);
        dst += strlen(replacements[n>>1]);
    }
    return dst;
}

// Given a template string copy it into a new string (to be free()'d by the
// caller) but replace <placeholder> occurerences by strings taken sequently
// from the 'replacements' array. If there are fewer replacements than
// <placeholders> the result is undefined.
char* from_template(const char* template, char **replacements)
{
    // 1st pass: determine the required size of the destination buffer
    int bytes_needed = 0;
    with_splits(template, "<>", cb_accumulate_required_size, replacements, (void*)&bytes_needed);

    // 2nd pass: allocate and populate the destination buffer
    char *dst = calloc(bytes_needed + 1, sizeof(char));
    with_splits(template, "<>", cb_replace_placeholders, replacements, (void*)dst);

    return dst;
}