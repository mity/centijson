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

#include "json-ptr.h"


typedef enum JSON_PTR_OP {
    JSON_PTR_GET,
    JSON_PTR_ADD,
    JSON_PTR_GET_OR_ADD
} JSON_PTR_OP;


#define IS_DIGIT(ch)        (('0') <= (ch)  &&  (ch) <= '9')


static int
json_ptr_is_index(const char* tok, const char* end, int* p_neg, size_t* p_index)
{
    if(tok < end && *tok == '-') {
        *p_neg = 1;
        tok++;
    } else {
        *p_neg = 0;
    }

    if(tok == end)
        return 0;

    /* The 1st digit can be zero only if no other digit follows. */
    *p_index = 0;
    if(*tok == '0'  &&  tok+1 < end)
        return 0;

    while(tok < end) {
        if(!IS_DIGIT(*tok))
            return 0;
        *p_index = (*p_index * 10) + (*tok - '0');
        tok++;
    }
    return 1;
}

static VALUE*
json_ptr_impl(VALUE* root, const char* pointer, JSON_PTR_OP op)
{
    const char* tok_beg = pointer;
    const char* tok_end;
    VALUE* v = root;
    int is_new = 0;
    int is_index;
    size_t index;

    if(*tok_beg == '\0')
        return (op != JSON_PTR_ADD) ? root : NULL;

    if(*tok_beg== '/')
        tok_beg++;

    while(v != NULL) {
        tok_end = tok_beg;
        while(*tok_end != '\0'  &&  *tok_end != '/')
            tok_end++;

        /* Determine if the token is array index. */
        if(tok_end - tok_beg == 1  &&  *tok_beg == '-') {
            is_index = 1;
            index = value_array_size(v);
        } else {
            int is_neg;

            is_index = json_ptr_is_index(tok_beg, tok_end, &is_neg, &index);
            if(is_index && is_neg) {
                size_t size = value_array_size(v);
                if(index <= size)
                    index = size - index;
                else
                    return NULL;
            }
        }

        if(is_index) {
            VALUE* tmp;

            if(is_new)
                value_init_array(v);
            if(value_type(v) != VALUE_ARRAY)
                return NULL;

            tmp = value_array_get(v, index);
            if(tmp == NULL  &&  op != JSON_PTR_GET  &&  index == value_array_size(v)) {
                tmp = value_array_append(v);
                is_new = 1;
            } else {
                is_new = 0;
            }

            v = tmp;
        } else {
            int has_escape;
            char* key;
            const char* tok_ptr;
            char* key_ptr;
            size_t len;

            if(is_new)
                value_init_dict(v);
            if(value_type(v) != VALUE_DICT)
                return NULL;

            /* The RFC-6901 allows some escape sequences:
             *   -- "~0" means '~'
             *   -- "~1" means '/'
             */
            has_escape = 0;
            for(tok_ptr = tok_beg; tok_ptr < tok_end; tok_ptr++) {
                if(*tok_ptr == '~') {
                    if(tok_ptr+1 == tok_end  ||  (*(tok_ptr+1) != '0' && *(tok_ptr+1) != '1')) {
                        /* invalid escape. */
                        return NULL;
                    }

                    has_escape = 1;
                    break;
                }
            }

            if(has_escape) {
                key = (char*) malloc(tok_end - tok_beg);
                if(key == NULL)
                    return NULL;

                tok_ptr = tok_beg;
                key_ptr = key;
                while(tok_ptr < tok_end) {
                    if(*tok_ptr == '~') {
                        *key_ptr = (*(tok_ptr+1) == '0' ? '~' : '/');
                        tok_ptr += 2;
                    } else {
                        *key_ptr = *tok_ptr;
                        tok_ptr += 1;
                    }
                    key_ptr++;
                }
                len = key_ptr - key;
            } else {
                key = (char*) tok_beg;
                len = tok_end - tok_beg;
            }

            if(op == JSON_PTR_GET) {
                v = value_dict_get_(v, key, len);
                is_new = 0;
            } else {
                v = value_dict_get_or_add_(v, key, len);
                is_new = value_is_new(v);
            }

            if(has_escape)
                free(key);
        }

        if(*tok_end == '\0')
            break;

        tok_beg = tok_end+1;
    }

    if(op == JSON_PTR_ADD  &&  !is_new) {
        /* The caller wanted to add a new value. */
        v = NULL;
    }

    return v;
}

VALUE*
json_ptr_get(const VALUE* root, const char* pointer)
{
    return json_ptr_impl((VALUE*) root, pointer, JSON_PTR_GET);
}

VALUE*
json_ptr_add(VALUE* root, const char* pointer)
{
    return json_ptr_impl((VALUE*) root, pointer, JSON_PTR_ADD);
}

VALUE*
json_ptr_get_or_add(VALUE* root, const char* pointer)
{
    return json_ptr_impl((VALUE*) root, pointer, JSON_PTR_GET_OR_ADD);
}
