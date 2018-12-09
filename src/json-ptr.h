/*
 * CentiJSON
 * <http://github.com/mity/centijson>
 *
 * Copyright (c) 2018 Martin Mitas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef JSON_PTR_H
#define JSON_PTR_H

#include "value.h"

#ifdef __cplusplus
extern "C" {
#endif


/* This implements RFC-6901: JSON Pointer
 * (see https://tools.ietf.org/html/rfc6901)
 *
 * Limitations:
 *
 *  -- These functions cannot deal with object keys which contain U+0000.
 *
 *  -- To prevent surprising results, the pointer parts which, according to the
 *     RFC-6901, can specify an array index (numbers, and also "-") are ALWAYS
 *     understood as an array index.
 *
 *     I.e. if the given subpart of the pointer refers actually to anything but
 *     array (e.g. an object, even if the object has key which looks as the
 *     given array index) it shall lead to a failure (NULL shall be returned).
 *
 * Extension:
 *
 *  -- We allow negative array indexes, counted in the reverse manner.
 *     E.g. consider "/foo/-0" where "-0" refers to the last element of
 *     an array keyed in the root object as "foo". Similarly "/foo/-1" is
 *     the array element just before it.
 */


/* Get a value on the given pointer; or NULL of no such value exists.
 *
 * Unlike the other functions, this function never modifies the VALUE
 * hierarchy.
 */
VALUE* json_ptr_get(const VALUE* root, const char* pointer);

/* Add a new value on the given pointer. The new value is initialized to
 * VALUE_NULL with the new flag set. Caller is supposed to re-initialize the
 * new value to reset the flag.
 *
 * Returns NULL in case of a failure. Note that that includes the situation if
 * the value specified with the pointer already exists.
 *
 * If some parent array or object does not exist (as expected from the pointer
 * specification), they are automatically created on the fly. Note in case of
 * a failure some of these parents may or may not be created, depending on the
 * nature of the failure.
 */
VALUE* json_ptr_add(VALUE* root, const char* pointer);

/* Get a value on the given pointer. If the value does not exist, it is added.
 * In such case the new value is initialized to VALUE_NULL with the new flag
 * set (test for it with value_is_new()) and caller is supposed to re-initialize
 * the new value to reset the flag.
 *
 * Returns NULL in case of a failure.
 *
 * If some parent array or object does not exist (as expected from the pointer
 * specification), they are automatically created on the fly. Note in case of
 * a failure some of these parents may or may not be created, depending on the
 * nature of the failure.
 */
VALUE* json_ptr_get_or_add(VALUE* root, const char* pointer);


#ifdef __cplusplus
}  /* extern "C" { */
#endif

#endif  /* JSON_PTR_H */
