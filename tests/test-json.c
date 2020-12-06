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

#include "acutest.h"
#include "json.h"
#include "json-dom.h"
#include "json-ptr.h"
#include "value.h"


/***************
 *** Helpers ***
 ***************/

/* For testing purposes, we process every single JSON snippet twice:
 * Once as a single block of input, and 2nd time by tiny one-byte buffers.
 * Then we compare the results of both and check they are exactly the same.
 */

static void deep_value_cmp(const VALUE* v1, const VALUE* v2);

static void
string_cmp(const VALUE* v1, const VALUE* v2)
{
    if(!TEST_CHECK(value_string_length(v1) == value_string_length(v2)))
        return;
    TEST_CHECK(memcmp(value_string(v1), value_string(v2), value_string_length(v1)) == 0);
}

static void
deep_array_cmp(const VALUE* v1, const VALUE* v2)
{
    size_t i;
    size_t size;
    const VALUE* data1;
    const VALUE* data2;

    if(!TEST_CHECK(value_array_size(v1) == value_array_size(v2)))
        return;
    size = value_array_size(v1);

    data1 = value_array_get_all(v1);
    data2 = value_array_get_all(v1);

    for(i = 0; i < size; i++)
        deep_value_cmp(&data1[i], &data2[i]);
}

static void
deep_dict_cmp(const VALUE* v1, const VALUE* v2)
{
    size_t i;
    size_t size;
    const VALUE** keys1;
    const VALUE** keys2;
    VALUE* value1;
    VALUE* value2;

    if(!TEST_CHECK(value_dict_size(v1) == value_dict_size(v2)))
        return;
    size = value_dict_size(v1);

    keys1 = malloc(size * sizeof(const VALUE*));
    keys2 = malloc(size * sizeof(const VALUE*));
    value_dict_keys_sorted(v1, keys1, size);
    value_dict_keys_sorted(v2, keys2, size);

    for(i = 0; i < size; i++) {
        string_cmp(keys1[i], keys2[i]);

        value1 = value_dict_get_(v1, value_string(keys1[i]), value_string_length(keys1[i]));
        value2 = value_dict_get_(v2, value_string(keys2[i]), value_string_length(keys2[i]));
        deep_value_cmp(value1, value2);
    }

    free((void*) keys1);
    free((void*) keys2);
}

static int
parse_byte_by_byte(const char* input, size_t size, const JSON_CONFIG* config,
                   unsigned dom_flags, VALUE* p_root, JSON_INPUT_POS* p_pos)
{
    JSON_DOM_PARSER dom_parser;
    size_t off;
    int ret;

    ret = json_dom_init(&dom_parser, config, dom_flags);
    if(ret != 0)
        return ret;

    for(off = 0; off < size; off++) {
        /* We rely on propagation of any error code into json_fini(). */
        if(json_dom_feed(&dom_parser, input+off, 1) != 0)
            break;
    }

    return json_dom_fini(&dom_parser, p_root, p_pos);
}

static void
deep_value_cmp(const VALUE* v1, const VALUE* v2)
{
    VALUE_TYPE type;

    if(v1 == NULL  ||  v2 == NULL) {
        TEST_CHECK(v1 == v2);
        return;
    }

    if(!TEST_CHECK(value_is_new(v1) == value_is_new(v2)))
        return;
    if(!TEST_CHECK(value_type(v1) == value_type(v2)))
        return;
    type = value_type(v1);

    switch(type) {
        case VALUE_NULL:
        case VALUE_BOOL:    TEST_CHECK(value_bool(v1) == value_bool(v2)); break;
        case VALUE_INT32:   TEST_CHECK(value_int32(v1) == value_int32(v2)); break;
        case VALUE_UINT32:  TEST_CHECK(value_uint32(v1) == value_uint32(v2)); break;
        case VALUE_INT64:   TEST_CHECK(value_int64(v1) == value_int64(v2)); break;
        case VALUE_UINT64:  TEST_CHECK(value_uint64(v1) == value_uint64(v2)); break;
        case VALUE_FLOAT:   TEST_CHECK(value_float(v1) == value_float(v2)); break;
        case VALUE_DOUBLE:  TEST_CHECK(value_double(v1) == value_double(v2)); break;
        case VALUE_STRING:  string_cmp(v1, v2); break;
        case VALUE_ARRAY:   deep_array_cmp(v1, v2); break;
        case VALUE_DICT:    deep_dict_cmp(v1, v2); break;
    }
}

static int
parse_(const char* input, size_t size, const JSON_CONFIG* config,
       unsigned dom_flags, VALUE* p_root, JSON_INPUT_POS* p_pos)
{
    VALUE root1 = VALUE_NULL_INITIALIZER;
    int err1;
    JSON_INPUT_POS pos1;

    VALUE root2 = VALUE_NULL_INITIALIZER;
    int err2;
    JSON_INPUT_POS pos2;

    /* Config to disable all limitations. */
    static const JSON_CONFIG config_no_limit = {
        0,  /* max_total_len */
        0,  /* max_total_values */
        0,  /* max_number_len */
        0,  /* max_string_len */
        0,  /* max_key_len */
        0,  /* max_nesting_level */
        0   /* flags */
    };

    if(config == NULL)
        config = &config_no_limit;

    err1 = json_dom_parse(input, size, config, dom_flags, &root1, &pos1);
    err2 = parse_byte_by_byte(input, size, config, dom_flags, &root2, &pos2);

    if(!TEST_CHECK(err1 == err2)) {
        TEST_MSG("parse whole input:  %d", err1);
        TEST_MSG("parse byte by byte: %d", err2);
    }
    if(!TEST_CHECK(pos1.offset == pos2.offset)) {
        TEST_MSG("parse whole input:  %u", (unsigned) pos1.offset);
        TEST_MSG("parse byte by byte: %u", (unsigned) pos2.offset);
    }
    if(!TEST_CHECK(pos1.line_number == pos2.line_number)) {
        TEST_MSG("parse whole input:  %u", pos1.line_number);
        TEST_MSG("parse byte by byte: %u", pos2.line_number);
    }
    if(!TEST_CHECK(pos1.column_number == pos2.column_number)) {
        TEST_MSG("parse whole input:  %u", pos1.column_number);
        TEST_MSG("parse byte by byte: %u", pos2.column_number);
    }
    deep_value_cmp(&root1, &root2);

    if(p_root != NULL)
        memcpy(p_root, &root1, sizeof(VALUE));
    else
        value_fini(&root1);
    value_fini(&root2);

    if(p_pos != NULL)
        memcpy(p_pos, &pos1, sizeof(JSON_INPUT_POS));

    return err1;
}

static int
parse(const char* input, const JSON_CONFIG* config, unsigned dom_flags,
      VALUE* p_root, JSON_INPUT_POS* p_pos)
{
    return parse_(input, strlen(input), config, dom_flags, p_root, p_pos);
}


/******************
 *** Unit Tests ***
 ******************/

static void
test_pos_tracking(void)
{
    JSON_INPUT_POS pos;

    parse("", NULL, 0, NULL, &pos);
    TEST_CHECK(pos.offset == 0);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 1);

    parse(" ", NULL, 0, NULL, &pos);
    TEST_CHECK(pos.offset == 1);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 2);

    parse("   ", NULL, 0, NULL, &pos);
    TEST_CHECK(pos.offset == 3);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 4);

    parse("\n", NULL, 0, NULL, &pos);
    TEST_CHECK(pos.offset == 1);
    TEST_CHECK(pos.line_number == 2);
    TEST_CHECK(pos.column_number == 1);

    parse("\r", NULL, 0, NULL, &pos);
    TEST_CHECK(pos.offset == 1);
    TEST_CHECK(pos.line_number == 2);
    TEST_CHECK(pos.column_number == 1);

    parse("\r\n", NULL, 0, NULL, &pos);
    TEST_CHECK(pos.offset == 2);
    TEST_CHECK(pos.line_number == 2);
    TEST_CHECK(pos.column_number == 1);

    parse("\n\n", NULL, 0, NULL, &pos);
    TEST_CHECK(pos.offset == 2);
    TEST_CHECK(pos.line_number == 3);
    TEST_CHECK(pos.column_number == 1);

    parse("\n\r\n\r", NULL, 0, NULL, &pos);
    TEST_CHECK(pos.offset == 4);
    TEST_CHECK(pos.line_number == 4);
    TEST_CHECK(pos.column_number == 1);

    parse("   \n   \r\n   ", NULL, 0, NULL, &pos);
    TEST_CHECK(pos.offset == 12);
    TEST_CHECK(pos.line_number == 3);
    TEST_CHECK(pos.column_number == 4);
}

static void
test_null(void)
{
    VALUE root;
    int err;

    err = parse("null", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_NULL);
    value_fini(&root);

    err = parse("  null\r\n", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_NULL);
    value_fini(&root);
}

static void
test_bool(void)
{
    VALUE root;
    int err;

    err = parse("true", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_BOOL);
    TEST_CHECK(value_bool(&root) != 0);
    value_fini(&root);

    err = parse("  false \r\n", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_BOOL);
    TEST_CHECK(value_bool(&root) == 0);
    value_fini(&root);
}

static void
test_number(void)
{
    VALUE root;
    int err;

    /* Simple cases */
    err = parse("0", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_INT32);
    TEST_CHECK(value_int32(&root) == 0);
    value_fini(&root);

    err = parse("123", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_INT32);
    TEST_CHECK(value_int32(&root) == 123);
    value_fini(&root);

    err = parse("-123", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_INT32);
    TEST_CHECK(value_int32(&root) == -123);
    value_fini(&root);

    /* Boundaries */
    err = parse("-2147483648", NULL, 0, &root, NULL);   /* INT32_MIN */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_INT32);
    TEST_CHECK(value_int32(&root) == INT32_MIN);
    value_fini(&root);

    err = parse("2147483647", NULL, 0, &root, NULL);    /* INT32_MAX */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_INT32);
    TEST_CHECK(value_int32(&root) == INT32_MAX);
    value_fini(&root);

    err = parse("2147483648", NULL, 0, &root, NULL);    /* INT32_MAX + 1 */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_UINT32);
    TEST_CHECK(value_int32(&root) == (uint32_t)INT32_MAX + 1);
    value_fini(&root);

    err = parse("4294967295", NULL, 0, &root, NULL);    /* UINT32_MAX */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_UINT32);
    TEST_CHECK(value_int32(&root) == UINT32_MAX);
    value_fini(&root);

    err = parse("-2147483649", NULL, 0, &root, NULL);    /* INT32_MIN - 1 */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_INT64);
    TEST_CHECK(value_int64(&root) == (int64_t)INT32_MIN - 1);
    value_fini(&root);

    err = parse("4294967296", NULL, 0, &root, NULL);    /* UINT32_MAX + 1 */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_INT64);
    TEST_CHECK(value_int64(&root) == (int64_t)UINT32_MAX + 1);
    value_fini(&root);

    err = parse("-9223372036854775808", NULL, 0, &root, NULL);    /* INT64_MIN */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_INT64);
    TEST_CHECK(value_int64(&root) == INT64_MIN);
    value_fini(&root);

    err = parse("9223372036854775807", NULL, 0, &root, NULL);    /* INT64_MAX */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_INT64);
    TEST_CHECK(value_int64(&root) == INT64_MAX);
    value_fini(&root);

    err = parse("9223372036854775808", NULL, 0, &root, NULL);    /* INT64_MAX + 1 */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_UINT64);
    TEST_CHECK(value_int64(&root) == (uint64_t)INT64_MAX + 1);
    value_fini(&root);

    err = parse("18446744073709551615", NULL, 0, &root, NULL);    /* UINT64_MAX */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_UINT64);
    TEST_CHECK(value_int64(&root) == UINT64_MAX);
    value_fini(&root);

    err = parse("18446744073709551616", NULL, 0, &root, NULL);    /* UINT64_MAX + 1 */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DOUBLE);
    TEST_CHECK(value_double(&root) > 0.9999 * (double)(UINT64_MAX));   /* so high, double is not precise enough... */
    TEST_CHECK(value_double(&root) < 1.0001 * (double)(UINT64_MAX));
    value_fini(&root);

    /* Fractions */
    err = parse("0.0", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DOUBLE);
    TEST_CHECK(value_double(&root) == 0.0);
    value_fini(&root);

    err = parse("-0.0", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DOUBLE);
    TEST_CHECK(value_double(&root) == -0.0);
    value_fini(&root);

    err = parse("0.5", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DOUBLE);
    TEST_CHECK(value_double(&root) == 0.5);
    value_fini(&root);

    err = parse("-0.5", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DOUBLE);
    TEST_CHECK(value_double(&root) == -0.5);
    value_fini(&root);

    err = parse("3.14159265", NULL, 0, &root, NULL);    /* Pi */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DOUBLE);
    TEST_CHECK(value_double(&root) > 0.9999 * 3.14159265);
    TEST_CHECK(value_double(&root) < 1.0001 * 3.14159265);
    value_fini(&root);

    /* Scientific notation */
    err = parse("1e2", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DOUBLE);
    TEST_CHECK(value_double(&root) == 100.0);
    value_fini(&root);

    err = parse("1e-2", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DOUBLE);
    TEST_CHECK(value_double(&root) == 0.01);
    value_fini(&root);

    err = parse("-1e-2", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DOUBLE);
    TEST_CHECK(value_double(&root) == -0.01);
    value_fini(&root);

    err = parse("6.626E-34", NULL, 0, &root, NULL);     /* Planck constant */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DOUBLE);
    TEST_CHECK(value_double(&root) > 0.9999 * 6.626E-34);
    TEST_CHECK(value_double(&root) < 1.0001 * 6.626E-34);
    value_fini(&root);

    err = parse("3.28e80", NULL, 0, &root, NULL);       /* Estimated count of particles in the observable universe */
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DOUBLE);
    TEST_CHECK(value_double(&root) > 0.9999 * 3.28e80);
    TEST_CHECK(value_double(&root) < 1.0001 * 3.28e80);
    value_fini(&root);
}

static void
test_string(void)
{
    VALUE root;
    int err;

    err = parse("\"\"", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_STRING);
    TEST_CHECK(strcmp(value_string(&root), "") == 0);
    value_fini(&root);

    err = parse("\"foo\"", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_STRING);
    TEST_CHECK(strcmp(value_string(&root), "foo") == 0);
    value_fini(&root);

    err = parse("\"foo\\nbar\"", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_STRING);
    TEST_CHECK(strcmp(value_string(&root), "foo\nbar") == 0);
    value_fini(&root);
}

static void
test_string_c_escape(void)
{
    static const struct {
        const char* name;
        const char* input;
        const char* output;
    } vector[] = {
        { "quotation mark", "\"\\\"\"", "\"" },
        { "reverse solidus", "\"\\\\\"", "\\" },
        { "solidus", "\"\\/\"", "/" },
        { "backspace", "\"\\b\"", "\b" },
        { "formfeed", "\"\\f\"", "\f" },
        { "newline", "\"\\n\"", "\n" },
        { "carriage return", "\"\\r\"", "\r" },
        { "horizontal tab", "\"\\t\"", "\t" },
        { "unicode", "\"\\u0001\"", "\x01" },
        { "unknown escape", "\"\\X\"", NULL },
        { "broken unicode 1", "\"\\uABC\"", NULL },
        { "broken unicode 2", "\"\\uAxBC\"", NULL },
        { 0 }
    };

    int i;
    int err;
    VALUE root;

    for(i = 0; vector[i].input != NULL; i++) {
        TEST_CASE(vector[i].name);
        err = parse(vector[i].input, NULL, 0, &root, NULL);
        if(vector[i].output != NULL) {
            TEST_CHECK(err == JSON_ERR_SUCCESS);
            TEST_CHECK(strcmp(value_string(&root), vector[i].output) == 0);
        } else {
            TEST_CHECK(err == JSON_ERR_INVALIDESCAPE);
        }
        value_fini(&root);
    }
}

static void
test_string_utf8(void)
{
    static const struct {
        const char* name;
        const char* input;
        const char* output;     /* NULL for ill-formed input. */
        const char* output_ignore_ill_formed;
        const char* output_fix_ill_formed;
    } vector[] = {
        /* Trivial text. */
        { "trivial", "\"foo\"", "foo", "foo", "foo" },

        /* Correct UTF-8 text */
        { "kosme",
          "\"\xce\xba\xe1\xbd\xb9\xcf\x83\xce\xbc\xce\xb5\"",   /* Greek word 'kosme' */
          "\xce\xba\xe1\xbd\xb9\xcf\x83\xce\xbc\xce\xb5",
          "\xce\xba\xe1\xbd\xb9\xcf\x83\xce\xbc\xce\xb5",
          "\xce\xba\xe1\xbd\xb9\xcf\x83\xce\xbc\xce\xb5" },

        /* The smallest possible UTF-8 sequences of given length
         * (except for 1 byte where we cannot use control characters) */
        { "smallest 1", "\"\x20\"", "\x20", "\x20", "\x20" },
        { "smallest 2", "\"\xc2\x80\"", "\xc2\x80", "\xc2\x80", "\xc2\x80" },
        { "smallest 3","\"\xe0\xa0\x80\"", "\xe0\xa0\x80", "\xe0\xa0\x80", "\xe0\xa0\x80" },
        { "smallest 4","\"\xf0\x90\x80\x80\"", "\xf0\x90\x80\x80", "\xf0\x90\x80\x80", "\xf0\x90\x80\x80" },

        /* The largest possible UTF-8 sequences of given length */
        { "largest 1", "\"\x7f\"", "\x7f", "\x7f", "\x7f" },
        { "largest 2", "\"\xdf\xbf\"", "\xdf\xbf", "\xdf\xbf", "\xdf\xbf" },
        { "largest 3", "\"\xef\xbf\xbf\"", "\xef\xbf\xbf", "\xef\xbf\xbf", "\xef\xbf\xbf" },
        { "largest 4", "\"\xf4\x8f\xbf\xbf\"", "\xf4\x8f\xbf\xbf", "\xf4\x8f\xbf\xbf", "\xf4\x8f\xbf\xbf" },

        /* Other boundary conditions */
        { "boundary 1", "\"\xed\x9f\xbf\"", "\xed\x9f\xbf", "\xed\x9f\xbf", "\xed\x9f\xbf" },
        { "boundary 2", "\"\xee\x80\x80\"", "\xee\x80\x80", "\xee\x80\x80", "\xee\x80\x80" },
        { "boundary 3", "\"\xef\xbf\xbd\"", "\xef\xbf\xbd", "\xef\xbf\xbd", "\xef\xbf\xbd" },

        /* Orphan trailing byte(s) */
        { "orphan 1", "\"\x80\"", NULL, "\x80", "\xef\xbf\xbd" },
        { "orphan 2", "\"\xbf\"", NULL, "\xbf", "\xef\xbf\xbd" },
        { "orphan 3", "\"\x80\x80\"", NULL, "\x80\x80", "\xef\xbf\xbd\xef\xbf\xbd" },

        /* Incomplete UTF-8 sequences */
        { "incomplete 1.1", "\"\xc2\"", NULL, "\xc2", "\xef\xbf\xbd" },
        { "incomplete 1.2", "\"\xe0\"", NULL, "\xe0", "\xef\xbf\xbd" },
        { "incomplete 1.3", "\"\xe0\xa0\"", NULL, "\xe0\xa0", "\xef\xbf\xbd" },
        { "incomplete 1.4", "\"\xf0\"", NULL, "\xf0", "\xef\xbf\xbd" },
        { "incomplete 1.5", "\"\xf0\x90\"", NULL, "\xf0\x90", "\xef\xbf\xbd" },
        { "incomplete 1.6", "\"\xf0\x90\x80\"", NULL, "\xf0\x90\x80", "\xef\xbf\xbd" },

        /* Incomplete UTF-8 sequences in the middle of text */
        { "incomplete 2.1", "\"foo\xc2""bar\"", NULL, "foo\xc2""bar", "foo\xef\xbf\xbd""bar" },
        { "incomplete 2.2", "\"foo\xe0""bar\"", NULL, "foo\xe0""bar", "foo\xef\xbf\xbd""bar" },
        { "incomplete 2.3", "\"foo\xe0\xa0""bar\"", NULL, "foo\xe0\xa0""bar", "foo\xef\xbf\xbd""bar" },
        { "incomplete 2.4", "\"foo\xf0""bar\"", NULL, "foo\xf0""bar", "foo\xef\xbf\xbd""bar" },
        { "incomplete 2.5", "\"foo\xf0\x90""bar\"", NULL, "foo\xf0\x90""bar", "foo\xef\xbf\xbd""bar" },
        { "incomplete 2.6", "\"foo\xf0\x90\x80""bar\"", NULL, "foo\xf0\x90\x80""bar", "foo\xef\xbf\xbd""bar" },

        /* 3 concatenated incomplete UTF-8 sequences */
        { "incomplete 3.1", "\"\xc2\xf0\x90\x80\xe0\"", NULL, "\xc2\xf0\x90\x80\xe0", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },

        /* Over-long UTF-8 sequences. */
        { "overlong 1", "\"\xc0\xaf\"", NULL, "\xc0\xaf", "\xef\xbf\xbd\xef\xbf\xbd" },
        { "overlong 2", "\"\xc0\x80\xaf\"", NULL, "\xc0\x80\xaf", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },

        /* Codepoints reserved for UTF-16 surrogates. */
        { "high surrogate 1", "\"\xed\xa0\x80\"", NULL, "\xed\xa0\x80", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },   /* U+d800 */
        { "high surrogate 2", "\"\xed\xaf\xbf\"", NULL, "\xed\xaf\xbf", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },   /* U+dbff */
        { "low surrogate 1", "\"\xed\xb0\x80\"", NULL, "\xed\xb0\x80", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },    /* U+dc00 */
        { "low surrogate 2", "\"\xed\xbf\xbf\"", NULL, "\xed\xbf\xbf", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },    /* U+dfff */

        { 0 }
    };

    int i;
    int err;
    JSON_CONFIG cfg_default;
    JSON_CONFIG cfg_ignore_ill_formed;
    JSON_CONFIG cfg_fix_ill_formed;
    VALUE root;

    json_default_config(&cfg_default);
    json_default_config(&cfg_ignore_ill_formed);
    cfg_ignore_ill_formed.flags |= JSON_IGNOREILLUTF8VALUE;
    json_default_config(&cfg_fix_ill_formed);
    cfg_fix_ill_formed.flags |= JSON_FIXILLUTF8VALUE;

    for(i = 0; vector[i].input != NULL; i++) {
        TEST_CASE(vector[i].name);

        err = parse(vector[i].input, &cfg_default, 0, &root, NULL);
        if(vector[i].output != NULL) {
            TEST_CHECK(err == JSON_ERR_SUCCESS);
            TEST_CHECK(strcmp(value_string(&root), vector[i].output) == 0);
        } else {
            TEST_CHECK(err == JSON_ERR_INVALIDUTF8);
        }
        value_fini(&root);

        err = parse(vector[i].input, &cfg_ignore_ill_formed, 0, &root, NULL);
        TEST_CHECK(vector[i].output_ignore_ill_formed != NULL);
        TEST_CHECK(err == JSON_ERR_SUCCESS);
        TEST_CHECK(strcmp(value_string(&root), vector[i].output_ignore_ill_formed) == 0);
        value_fini(&root);

        err = parse(vector[i].input, &cfg_fix_ill_formed, 0, &root, NULL);
        TEST_CHECK(vector[i].output_fix_ill_formed != NULL);
        TEST_CHECK(err == JSON_ERR_SUCCESS);
        TEST_CHECK(strcmp(value_string(&root), vector[i].output_fix_ill_formed) == 0);
        value_fini(&root);
    }
}

static void
test_string_unicode_escape(void)
{
    static const struct {
        const char* name;
        const char* input;
        const char* output;         /* NULL for ill-formed input. */
        const char* output_ignore_ill_formed;
        const char* output_fix_ill_formed;
    } vector[] = {
        /* Simple Plane 0 (BMP) codepoints. */
        { "U+0001", "\"\\u0001\"", "\x01", "\x01", "\x01" },
        { "U+abcd", "\"\\uabcd\"", "\xea\xaf\x8d", "\xea\xaf\x8d", "\xea\xaf\x8d" },
        { "U+ABCD", "\"\\uABCD\"", "\xea\xaf\x8d", "\xea\xaf\x8d", "\xea\xaf\x8d" },
        { "U+AbCd", "\"\\uAbCd\"", "\xea\xaf\x8d", "\xea\xaf\x8d", "\xea\xaf\x8d" },
        { "U+ABCD U+abcd", "\"\\uABCD\\uabcd\"", "\xea\xaf\x8d\xea\xaf\x8d", "\xea\xaf\x8d\xea\xaf\x8d", "\xea\xaf\x8d\xea\xaf\x8d" },
        { "U+ffff", "\"\\uffff\"", "\xef\xbf\xbf", "\xef\xbf\xbf", "\xef\xbf\xbf" },

        /* Surrogate pairs. */
        { "U+d800 U+dc00", "\"\\ud800\\udc00\"", "\xf0\x90\x80\x80", "\xf0\x90\x80\x80", "\xf0\x90\x80\x80" },
        { "U+dbff U+dfff", "\"\\udbff\\udfff\"", "\xf4\x8f\xbf\xbf", "\xf4\x8f\xbf\xbf", "\xf4\x8f\xbf\xbf" },

        /* Orphan surrogates. */
        { "U+d800", "\"\\ud800\"", NULL, "\xed\xa0\x80", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
        { "U+dbff", "\"\\udbff\"", NULL, "\xed\xaf\xbf", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
        { "U+dc00", "\"\\udc00\"", NULL, "\xed\xb0\x80", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
        { "U+dfff", "\"\\udfff\"", NULL, "\xed\xbf\xbf", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },

        { 0 }
    };

    int i;
    int err;
    JSON_CONFIG cfg_default;
    JSON_CONFIG cfg_ignore_ill_formed;
    JSON_CONFIG cfg_fix_ill_formed;
    VALUE root;

    json_default_config(&cfg_default);
    json_default_config(&cfg_ignore_ill_formed);
    cfg_ignore_ill_formed.flags |= JSON_IGNOREILLUTF8VALUE;
    json_default_config(&cfg_fix_ill_formed);
    cfg_fix_ill_formed.flags |= JSON_FIXILLUTF8VALUE;

    for(i = 0; vector[i].input != NULL; i++) {
        TEST_CASE(vector[i].name);

        err = parse(vector[i].input, &cfg_default, 0, &root, NULL);
        if(vector[i].output != NULL) {
            TEST_CHECK(err == JSON_ERR_SUCCESS);
            TEST_CHECK(strcmp(value_string(&root), vector[i].output) == 0);
        } else {
            TEST_CHECK(err == JSON_ERR_INVALIDUTF8);
        }
        value_fini(&root);

        err = parse(vector[i].input, &cfg_ignore_ill_formed, 0, &root, NULL);
        TEST_CHECK(vector[i].output_ignore_ill_formed != NULL);
        TEST_CHECK(err == JSON_ERR_SUCCESS);
        TEST_CHECK(strcmp(value_string(&root), vector[i].output_ignore_ill_formed) == 0);
        TEST_MSG("expected[%d]: %s", i, vector[i].output_ignore_ill_formed);
        TEST_MSG("received[%d]: %s", i, value_string(&root));
        value_fini(&root);

        err = parse(vector[i].input, &cfg_fix_ill_formed, 0, &root, NULL);
        TEST_CHECK(vector[i].output_fix_ill_formed != NULL);
        TEST_CHECK(err == JSON_ERR_SUCCESS);
        TEST_CHECK(strcmp(value_string(&root), vector[i].output_fix_ill_formed) == 0);
        value_fini(&root);
    }
}

static void
test_array(void)
{
    VALUE root;
    int err;

    err = parse("[]", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_ARRAY);
    TEST_CHECK(value_array_size(&root) == 0);
    value_fini(&root);

    err = parse("[null]", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_ARRAY);
    TEST_CHECK(value_array_size(&root) == 1);
    TEST_CHECK(value_type(value_array_get(&root, 0)) == VALUE_NULL);
    value_fini(&root);

    err = parse("[null,false,true]", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_ARRAY);
    TEST_CHECK(value_array_size(&root) == 3);
    TEST_CHECK(value_type(value_array_get(&root, 0)) == VALUE_NULL);
    TEST_CHECK(value_bool(value_array_get(&root, 1)) == 0);
    TEST_CHECK(value_bool(value_array_get(&root, 2)) == !0);
    value_fini(&root);

    err = parse("[[],[[\"foo\"]]]", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_ARRAY);
    TEST_CHECK(value_array_size(&root) == 2);
    TEST_CHECK(value_type(value_array_get(&root, 0)) == VALUE_ARRAY);
    TEST_CHECK(value_type(value_array_get(&root, 1)) == VALUE_ARRAY);
    TEST_CHECK(strcmp(value_string(value_path(&root, "[1]/[0]/[0]")), "foo") == 0);
    value_fini(&root);
}

static void
test_object(void)
{
    VALUE root;
    int err;

    err = parse("{}", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DICT);
    TEST_CHECK(value_dict_size(&root) == 0);
    value_fini(&root);

    err = parse("{ \"name\": \"John Doe\" }", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DICT);
    TEST_CHECK(value_dict_size(&root) == 1);
    TEST_CHECK(strcmp(value_string(value_dict_get(&root, "name")), "John Doe") == 0);
    value_fini(&root);
}

static void
test_combined(void)
{
    VALUE root;
    int err;

    err = parse(
        "[\n"
            "{\n"
                "\"name\": \"Alice\",\n"
                "\"age\": 17\n"
            "},\n"
            "{\n"
                "\"name\": \"Bob\",\n"
                "\"age\": 19\n"
            "}\n"
        "]\n",
        NULL, 0, &root, NULL
    );
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(strcmp(value_string(value_path(&root, "[0]/name")), "Alice") == 0);
    TEST_CHECK(value_int32(value_path(&root, "[0]/age")) == 17);
    TEST_CHECK(strcmp(value_string(value_path(&root, "[1]/name")), "Bob") == 0);
    TEST_CHECK(value_int32(value_path(&root, "[1]/age")) == 19);
    value_fini(&root);
}

static void
test_limit_max_total_len(void)
{
    JSON_CONFIG config;
    JSON_INPUT_POS pos;
    int err;

    json_default_config(&config);
    config.max_total_len = 3;
    err = parse("123", &config, 0, NULL, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);

    err = parse("1234", &config, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_MAXTOTALLEN);
    TEST_CHECK(pos.offset == 3);
}

static void
test_limit_max_total_values(void)
{
    JSON_CONFIG config;
    JSON_INPUT_POS pos;
    int err;

    json_default_config(&config);
    config.max_total_values = 3;
    err = parse("[1, 2]", &config, 0, NULL, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);

    err = parse("[1, 2, 3]", &config, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_MAXTOTALVALUES);
    TEST_CHECK(pos.offset == 7);
}

static void
test_limit_max_nesting_level(void)
{
    JSON_CONFIG config;
    JSON_INPUT_POS pos;
    int err;

    json_default_config(&config);
    config.max_nesting_level = 3;
    err = parse("[[[]]]", &config, 0, NULL, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);

    err = parse("[[[[]]]]", &config, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_MAXNESTINGLEVEL);
    TEST_CHECK(pos.offset == 3);
}

static void
test_limit_max_number_len(void)
{
    JSON_CONFIG config;
    JSON_INPUT_POS pos;
    int err;

    json_default_config(&config);
    config.max_number_len = 3;
    err = parse("  123  ", &config, 0, NULL, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);

    err = parse("  1234  ", &config, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_MAXNUMBERLEN);
    TEST_CHECK(pos.offset == 2);
}

static void
test_limit_max_string_len(void)
{
    JSON_CONFIG config;
    JSON_INPUT_POS pos;
    int err;

    json_default_config(&config);
    config.max_string_len = 3;
    err = parse("[ \"Max\", \"Anna\" ]", &config, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_MAXSTRINGLEN);
    TEST_CHECK(pos.offset == 9);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 10);
}

static void
test_limit_max_key_len(void)
{
    JSON_CONFIG config;
    JSON_INPUT_POS pos;
    int err;

    json_default_config(&config);
    config.max_key_len = 3;
    err = parse("{ \"age\": 12,\n  \"name\": \"Daisy\" }", &config, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_MAXKEYLEN);
    TEST_CHECK(pos.offset == 15);
    TEST_CHECK(pos.line_number == 2);
    TEST_CHECK(pos.column_number == 3);
}

static void
test_err_common(void)
{
    VALUE root;
    int err;

    /* Check that, even on error, ROOT gets initialized (to VALUE_NULL). */
    memset(&root, 0xff, sizeof(VALUE));
    err = parse("foo", NULL, 0, &root, NULL);
    TEST_CHECK(err != JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_NULL);
    value_fini(&root);
}

static void
test_err_bad_closer(void)
{
    JSON_INPUT_POS pos;
    int err;

    err = parse("{ ]", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_BADCLOSER);
    TEST_CHECK(pos.offset == 2);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 3);

    err = parse("[\n}", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_BADCLOSER);
    TEST_CHECK(pos.offset == 2);
    TEST_CHECK(pos.line_number == 2);
    TEST_CHECK(pos.column_number == 1);

    err = parse("[[[ ]}]", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_BADCLOSER);
    TEST_CHECK(pos.offset == 5);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 6);
}

static void
test_err_bad_root_type(void)
{
    JSON_CONFIG config;
    JSON_INPUT_POS pos;
    int err;

    /* Check null as root. */
    json_default_config(&config);
    config.flags |= JSON_NONULLASROOT;
    err = parse("null", &config, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_BADROOTTYPE);
    TEST_CHECK(pos.offset == 0);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 1);

    /* Check bool as root. */
    json_default_config(&config);
    config.flags |= JSON_NOBOOLASROOT;
    err = parse("true", &config, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_BADROOTTYPE);

    /* Check number as root. */
    json_default_config(&config);
    config.flags |= JSON_NONUMBERASROOT;
    err = parse("42", &config, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_BADROOTTYPE);

    /* Check string as root. */
    json_default_config(&config);
    config.flags |= JSON_NOSTRINGASROOT;
    err = parse("\"foo\"", &config, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_BADROOTTYPE);

    /* Check array as root. */
    json_default_config(&config);
    config.flags |= JSON_NOARRAYASROOT;
    err = parse("[ ]", &config, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_BADROOTTYPE);

    /* Check object as root. */
    json_default_config(&config);
    config.flags |= JSON_NOOBJECTASROOT;
    err = parse("{ }", &config, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_BADROOTTYPE);
}

static void
test_err_syntax(void)
{
    JSON_INPUT_POS pos;
    int err;

    err = parse("xxx", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_SYNTAX);
    TEST_CHECK(pos.offset == 0);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 1);

    err = parse("nullx", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_SYNTAX);
    TEST_CHECK(pos.offset == 0);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 1);

    err = parse("12xx", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_SYNTAX);
    TEST_CHECK(pos.offset == 0);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 1);

    err = parse("\"foo", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_UNCLOSEDSTRING);
    TEST_CHECK(pos.offset == 0);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 1);

    err = parse("\"foo\n", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_UNCLOSEDSTRING);
    TEST_CHECK(pos.offset == 0);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 1);

    err = parse("\"foo\\X\"", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_INVALIDESCAPE);
    TEST_CHECK(pos.offset == 5);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 6);

    err = parse("\"foo\b\"", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_UNESCAPEDCONTROL);
    TEST_CHECK(pos.offset == 4);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 5);

    err = parse("", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_EXPECTEDVALUE);
    TEST_CHECK(pos.offset == 0);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 1);

    err = parse("[,]", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_EXPECTEDVALUEORCLOSER);
    TEST_CHECK(pos.offset == 1);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 2);

    err = parse("{,}", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_EXPECTEDKEYORCLOSER);
    TEST_CHECK(pos.offset == 1);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 2);

    err = parse("{ \"key\" }", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_EXPECTEDCOLON);
    TEST_CHECK(pos.offset == 8);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 9);

    err = parse("{ \"key\": \"value\" , }", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_EXPECTEDKEY);
    TEST_CHECK(pos.offset == 19);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 20);

    err = parse("1, 2, 3", NULL, 0, NULL, &pos);
    TEST_CHECK(err == JSON_ERR_EXPECTEDEOF);
    TEST_CHECK(pos.offset == 1);
    TEST_CHECK(pos.line_number == 1);
    TEST_CHECK(pos.column_number == 2);
}

static void
test_json_checker(void)
{
    /* These tests are from http://www.json.org/JSON_checker/ */

    static const char pass1[] =
        "[\n"
        "    \"JSON Test Pattern pass1\",\n"
        "    {\"object with 1 member\":[\"array with 1 element\"]},\n"
        "    {},\n"
        "    [],\n"
        "    -42,\n"
        "    true,\n"
        "    false,\n"
        "    null,\n"
        "    {\n"
        "        \"integer\": 1234567890,\n"
        "        \"real\": -9876.543210,\n"
        "        \"e\": 0.123456789e-12,\n"
        "        \"E\": 1.234567890E+34,\n"
        "        \"\":  23456789012E66,\n"
        "        \"zero\": 0,\n"
        "        \"one\": 1,\n"
        "        \"space\": \" \",\n"
        "        \"quote\": \"\\\"\",\n"
        "        \"backslash\": \"\\\\\",\n"
        "        \"controls\": \"\\b\\f\\n\\r\\t\",\n"
        "        \"slash\": \"/ & \\/\",\n"
        "        \"alpha\": \"abcdefghijklmnopqrstuvwyz\",\n"
        "        \"ALPHA\": \"ABCDEFGHIJKLMNOPQRSTUVWYZ\",\n"
        "        \"digit\": \"0123456789\",\n"
        "        \"0123456789\": \"digit\",\n"
        "        \"special\": \"`1~!@#$%^&*()_+-={':[,]}|;.</>?\",\n"
        "        \"hex\": \"\\u0123\\u4567\\u89AB\\uCDEF\\uabcd\\uef4A\",\n"
        "        \"true\": true,\n"
        "        \"false\": false,\n"
        "        \"null\": null,\n"
        "        \"array\":[  ],\n"
        "        \"object\":{  },\n"
        "        \"address\": \"50 St. James Street\",\n"
        "        \"url\": \"http://www.JSON.org/\",\n"
        "        \"comment\": \"// /* <!-- --\",\n"
        "        \"# -- --> */\": \" \",\n"
        "        \" s p a c e d \" :[1,2 , 3\n"
        "\n"
        ",\n"
        "\n"
        "4 , 5        ,          6           ,7        ],\"compact\":[1,2,3,4,5,6,7],\n"
        "        \"jsontext\": \"{\\\"object with 1 member\\\":[\\\"array with 1 element\\\"]}\",\n"
        "        \"quotes\": \"&#34; \\u0022 %22 0x22 034 &#x22;\",\n"
        "        \"\\/\\\\\\\"\\uCAFE\\uBABE\\uAB98\\uFCDE\\ubcda\\uef4A\\b\\f\\n\\r\\t`1~!@#$%^&*()_+-=[]{}|;:',./<>?\"\n"
        ": \"A key can be any string\"\n"
        "    },\n"
        "    0.5 ,98.6\n"
        ",\n"
        "99.44\n"
        ",\n"
        "\n"
        "1066,\n"
        "1e1,\n"
        "0.1e1,\n"
        "1e-1,\n"
        "1e00,2e+00,2e-00\n"
        ",\"rosebud\"]";

    static const char pass2[] =
        "[[[[[[[[[[[[[[[[[[[\"Not too deep\"]]]]]]]]]]]]]]]]]]]";

    static const char pass3[] =
        "{\n"
        "    \"JSON Test Pattern pass3\": {\n"
        "        \"The outermost value\": \"must be an object or array.\",\n"
        "        \"In this test\": \"It is an object.\"\n"
        "    }\n"
        "}\n";

    static const struct {
        const char* name;
        const char* input;
    } pass[] = {
        { "pass 1", pass1 },
        { "pass 2", pass2 },
        { "pass 3", pass3 },
        { 0 }
    };

    static const struct {
        const char* name;
        const char* input;
    } fail[] = {
#if 0
        /* Disabled, because this case is not really true.
         *
         * From RFC-8259 (https://tools.ietf.org/html/rfc8259#page-14):
         *
         * > A JSON text is a serialized value.  Note that certain previous
         * > specifications of JSON constrained a JSON text to be an object or
         * > an array.  Implementations that generate only objects or arrays
         * > where a JSON text is called for will be interoperable in the sense
         * > that all implementations will accept these as conforming JSON
         * > texts.
         *
         * In Centijson, we allow by default any root, but caller may configure
         * this via flags of json_init().
         *
         * We also test that quite thoroughly in test_err_bad_root_type().
         */
        { "fail 1",  "\"A JSON payload should be an object or array, not a string.\"" },
#endif
        { "fail 2",  "[\"Unclosed array\"" },
        { "fail 3",  "{unquoted_key: \"keys must be quoted\"}" },
        { "fail 4",  "[\"extra comma\",]" },
        { "fail 5",  "[\"double extra comma\",,]" },
        { "fail 6",  "[   , \"<-- missing value\"]" },
        { "fail 7",  "[\"Comma after the close\"]," },
        { "fail 8",  "[\"Extra close\"]]" },
        { "fail 9",  "{\"Extra comma\": true,}" },
        { "fail 10", "{\"Extra value after close\": true} \"misplaced quoted value\"" },
        { "fail 11", "{\"Illegal expression\": 1 + 2}" },
        { "fail 12", "{\"Illegal invocation\": alert()}" },
        { "fail 13", "{\"Numbers cannot have leading zeroes\": 013}" },
        { "fail 14", "{\"Numbers cannot be hex\": 0x14}" },
        { "fail 15", "[\"Illegal backslash escape: \x15\"]" },
        { "fail 16", "[\\naked]" },
        { "fail 17", "[\"Illegal backslash escape: \\017\"]" },
#if 0
        /* This is plain wrong. No JSON standard limits maximal nesting. And we
         * allow caller limit it as he wishes in json_init().
         */
        { "fail 18", "[[[[[[[[[[[[[[[[[[[[\"Too deep\"]]]]]]]]]]]]]]]]]]]]" },
#endif
        { "fail 19", "{\"Missing colon\" null}" },
        { "fail 20", "{\"Double colon\":: null}" },
        { "fail 21", "{\"Comma instead of colon\", null}" },
        { "fail 22", "[\"Colon instead of comma\": false]" },
        { "fail 23", "[\"Bad value\", truth]" },
        { "fail 24", "['single quote']" },
        { "fail 25", "[\"\ttab character\tin\tstring\t\"]" },
        { "fail 26", "[\"tab\\\tcharacter\\\tin\\\tstring\\\t\"]" },
        { "fail 27", "[\"line\nbreak\"]" },
        { "fail 28", "[\"line\\\nbreak\"]" },
        { "fail 29", "[0e]" },
        { "fail 30", "[0e+]" },
        { "fail 31", "[0e+-1]" },
        { "fail 32", "{\"Comma instead if closing brace\": true," },
        { "fail 33", "[\"mismatch\"}" },
        { 0 }
    };

    int i;

    for(i = 0; pass[i].name != NULL; i++) {
        TEST_CASE(pass[i].name);
        TEST_CHECK(parse(pass[i].input, NULL, 0, NULL, NULL) == 0);
    }

    for(i = 0; fail[i].name != NULL; i++) {
        TEST_CASE(fail[i].name);
        TEST_CHECK(parse(fail[i].input, NULL, 0, NULL, NULL) != 0);
    }
}


static char dump_buffer[16 * 256];

static int
test_dump_callback(const char* data, size_t size, void* userdata)
{
    size_t* n = (size_t*) userdata;

    if(!TEST_CHECK(*n + size <= sizeof(dump_buffer)))
        return -1;

    memcpy(dump_buffer + *n, data, size);
    *n += size;
    return 0;
}

static void
test_dump(void)
{
    static const char expected[] = {
        "[\n"
        "\t{\n"
        "\t\t\"name\": \"Alice\",\n"
        "\t\t\"age\": 23,\n"
        "\t\t\"height\": 168.5\n"
        "\t},\n"
        "\t{\n"
        "\t\t\"name\": \"Bob\",\n"
        "\t\t\"age\": 54,\n"
        "\t\t\"height\": 182.0\n"
        "\t}\n"
        "]"
    };

    size_t n = 0;
    VALUE root;
    VALUE* alice;
    VALUE* bob;
    VALUE* name;
    VALUE* age;
    VALUE* height;
    int err;

    value_init_array(&root);

    alice = value_array_append(&root);
    value_init_dict_ex(alice, NULL, VALUE_DICT_MAINTAINORDER);
    name = value_dict_add(alice, "name");
    value_init_string(name, "Alice");
    age = value_dict_add(alice, "age");
    value_init_uint32(age, 23);
    height = value_dict_add(alice, "height");
    value_init_float(height, 168.5);

    bob = value_array_append(&root);
    value_init_dict_ex(bob, NULL, VALUE_DICT_MAINTAINORDER);
    name = value_dict_add(bob, "name");
    value_init_string(name, "Bob");
    age = value_dict_add(bob, "age");
    value_init_uint32(age, 54);
    height = value_dict_add(bob, "height");
    value_init_float(height, 182.0);

    err = json_dom_dump(&root, test_dump_callback, (void*) &n, 0, JSON_DOM_DUMP_PREFERDICTORDER);
    if(TEST_CHECK(err == 0)) {
        /* Parse it back and compare it with what we expect. */
        VALUE a, b;
        TEST_CHECK(json_dom_parse(dump_buffer, n, NULL, JSON_DOM_MAINTAINDICTORDER, &a, NULL) == 0);
        TEST_CHECK(json_dom_parse(expected, strlen(expected), NULL, JSON_DOM_MAINTAINDICTORDER, &b, NULL) == 0);
        deep_value_cmp(&a, &b);
        value_fini(&a);
        value_fini(&b);
    }

    value_fini(&root);
}

static void
test_pointer(void)
{
    static const char input[] =
            "{\n"
                "\"foo\": [\"bar\", \"baz\"],\n"
                "\"\": 0,\n"
                "\"a/b\": 1,\n"
                "\"c%d\": 2,\n"
                "\"e^f\": 3,\n"
                "\"g|h\": 4,\n"
                "\"i\\\\j\": 5,\n"
                "\"k\\\"l\": 6,\n"
                "\" \": 7,\n"
                "\"m~n\": 8\n"
            "}\n";

    VALUE root;
    VALUE* v;

    TEST_CHECK(json_dom_parse(input, strlen(input), NULL, 0, &root, NULL) == 0);

    TEST_CHECK(json_ptr_get(&root, "") == &root);

    v = json_ptr_get(&root, "/foo");
    TEST_CHECK(value_type(v) == VALUE_ARRAY);
    TEST_CHECK(value_array_size(v) == 2);
    TEST_CHECK(strcmp(value_string(value_array_get(v, 0)), "bar") == 0);
    TEST_CHECK(strcmp(value_string(value_array_get(v, 1)), "baz") == 0);

    v = json_ptr_get(&root, "/foo/0");
    TEST_CHECK(strcmp(value_string(v), "bar") == 0);

    v = json_ptr_get(&root, "/foo/1");
    TEST_CHECK(strcmp(value_string(v), "baz") == 0);

    v = json_ptr_get(&root, "/foo/-2");
    TEST_CHECK(strcmp(value_string(v), "bar") == 0);

    v = json_ptr_get(&root, "/foo/-1");
    TEST_CHECK(strcmp(value_string(v), "baz") == 0);

    v = json_ptr_get(&root, "/");
    TEST_CHECK(value_int32(v) == 0);

    v = json_ptr_get(&root, "/a~1b");
    TEST_CHECK(value_int32(v) == 1);

    v = json_ptr_get(&root, "/c%d");
    TEST_CHECK(value_int32(v) == 2);

    v = json_ptr_get(&root, "/e^f");
    TEST_CHECK(value_int32(v) == 3);

    v = json_ptr_get(&root, "/g|h");
    TEST_CHECK(value_int32(v) == 4);

    v = json_ptr_get(&root, "/i\\j");
    TEST_CHECK(value_int32(v) == 5);

    v = json_ptr_get(&root, "/k\"l");
    TEST_CHECK(value_int32(v) == 6);

    v = json_ptr_get(&root, "/ ");
    TEST_CHECK(value_int32(v) == 7);

    v = json_ptr_get(&root, "/m~0n");
    TEST_CHECK(value_int32(v) == 8);

    v = json_ptr_get(&root, "/xyz");
    TEST_CHECK(v == NULL);

    v = json_ptr_add(&root, "/xyz");
    TEST_CHECK(v != NULL);
    TEST_CHECK(value_is_new(v));
    value_init_int32(v, 42);

    v = json_ptr_get_or_add(&root, "xyz");
    TEST_CHECK(value_int32(v) == 42);

    v = json_ptr_get_or_add(&root, "xyz2");
    TEST_CHECK(v != NULL);
    TEST_CHECK(value_is_new(v));

    json_ptr_add(&root, "/foo/-");
    json_ptr_add(&root, "/foo/-");
    v = json_ptr_get(&root, "/foo");
    TEST_CHECK(value_type(v) == VALUE_ARRAY);
    TEST_CHECK(value_array_size(v) == 4);
    TEST_CHECK(value_is_new(value_array_get(v, 2)));
    TEST_CHECK(value_is_new(value_array_get(v, 3)));

    v = json_ptr_add(&root, "");    /* Cannot add root. */
    TEST_CHECK(v == NULL);

    v = json_ptr_get_or_add(&root, "");
    TEST_CHECK(v == &root);

    value_fini(&root);
}


static void
test_crazy_double(void)
{
    /* These two are taken from https://github.com/miloyip/nativejson-benchmark
     * tests (it has hit us.) */
    char buffer[512];
    int i, n;
    VALUE root;
    int err;

    /* Very long number ('1' followed by 308 zeros). */
    n = 0;
    buffer[n++] = '1';
    for(i = 0; i < 308; i++)
        buffer[n++] = '0';
    buffer[n++] = '\0';
    err = parse(buffer, NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DOUBLE);
    TEST_CHECK(value_double(&root) > 0.9999 * 1e308);
    TEST_CHECK(value_double(&root) < 1.0001 * 1e308);

    /* Trimming. */
    err = parse(
        "2.22507385850720113605740979670913197593481954635164564802342610972482222202107694551652952390813508"
        "7914149158913039621106870086438694594645527657207407820621743379988141063267329253552286881372149012"
        "9811224514518898490572223072852551331557550159143974763979834118019993239625482890171070818506906306"
        "6665599493827577257201576306269066333264756530000924588831643303777979186961204949739037782970490505"
        "1080609940730262937128958950003583799967207254304360284078895771796150945516748243471030702609144621"
        "5722898802581825451803257070188608721131280795122334262883686223215037756666225039825343359745688844"
        "2390026549819838548794829220689472168983109969836584681402285424333066033985088644580400103493397042"
        "7567186443383770486037861622771738545623065874679014086723327636718751234567890123456789012345678901"
        "e-308", NULL, 0, &root, NULL);
    TEST_CHECK(err == JSON_ERR_SUCCESS);
    TEST_CHECK(value_type(&root) == VALUE_DOUBLE);
    TEST_CHECK(value_double(&root) > 0.9999 * 2.2250738585072014e-308);
    TEST_CHECK(value_double(&root) < 1.0001 * 2.2250738585072014e-308);
}

static void
test_issue2(void)
{
    static const char expected[] = "[0]";

    VALUE root;
    VALUE* zero;
    size_t n = 0;
    int err;

    value_init_array(&root);
    zero = value_array_append(&root);
    value_init_int32(zero, 0);

    err = json_dom_dump(&root, test_dump_callback, (void*) &n, 0, 0);

    if(TEST_CHECK(err == 0)) {
        /* Parse it back and compare it with what we expect. */
        VALUE a, b;
        TEST_CHECK(json_dom_parse(dump_buffer, n, NULL, 0, &a, NULL) == 0);
        TEST_CHECK(json_dom_parse(expected, strlen(expected), NULL, 0, &b, NULL) == 0);
        deep_value_cmp(&a, &b);
        value_fini(&a);
        value_fini(&b);
    }

    value_fini(&root);
}

static void
test_issue3(void)
{
    static const char expected[] = "[-1]";

    VALUE root;
    VALUE* minusone;
    size_t n = 0;
    int err;

    value_init_array(&root);
    minusone = value_array_append(&root);
    value_init_int32(minusone, -1);

    err = json_dom_dump(&root, test_dump_callback, (void*) &n, 0, 0);

    if(TEST_CHECK(err == 0)) {
        /* Parse it back and compare it with what we expect. */
        VALUE a, b;
        TEST_CHECK(json_dom_parse(dump_buffer, n, NULL, 0, &a, NULL) == 0);
        TEST_CHECK(json_dom_parse(expected, strlen(expected), NULL, 0, &b, NULL) == 0);
        deep_value_cmp(&a, &b);
        value_fini(&a);
        value_fini(&b);
    }

    value_fini(&root);
}


TEST_LIST = {
    { "pos-tracking",               test_pos_tracking },
    { "null",                       test_null },
    { "bool",                       test_bool },
    { "number",                     test_number },
    { "string",                     test_string },
    { "string-c-escape",            test_string_c_escape },
    { "string-utf8",                test_string_utf8 },
    { "string-unicode-escape",      test_string_unicode_escape },
    { "array",                      test_array },
    { "object",                     test_object },
    { "combined",                   test_combined },
    { "limit-max-total-len",        test_limit_max_total_len },
    { "limit-max-total-values",     test_limit_max_total_values },
    { "limit-max-nesting-level",    test_limit_max_nesting_level },
    { "limit-max-number-len",       test_limit_max_number_len },
    { "limit-max-string-len",       test_limit_max_string_len },
    { "limit-max-key-len",          test_limit_max_key_len },
    { "err-common",                 test_err_common },
    { "err-bad-closer",             test_err_bad_closer },
    { "err-bad-root-type",          test_err_bad_root_type },
    { "err-syntax",                 test_err_syntax },
    { "json-checker",               test_json_checker },
    { "dump",                       test_dump },
    { "pointer",                    test_pointer },
    { "crazy-double",               test_crazy_double },
    { "bug-issue2",                 test_issue2 },
    { "bug-issue3",                 test_issue3 },
    { 0 }
};
