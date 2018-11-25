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

#include "json.h"
#include "json-dom.h"
#include "json-err.h"

#include <stdio.h>


void
json_err(int errcode, JSON_INPUT_POS* pos)
{
    switch(errcode) {
        /* Branches for errors not related to the given location use return instead of break. */
        case JSON_ERR_SUCCESS:          fprintf(stderr, "Success.\n"); return;
        case JSON_ERR_OUTOFMEMORY:      fprintf(stderr, "Out of memory.\n"); return;

        /* For these, position should provide reasonable pointer to the input. */
        case JSON_ERR_INTERNAL:         fprintf(stderr, "Internal error.\n"); break;
        case JSON_ERR_SYNTAX:           fprintf(stderr, "Syntax error.\n"); break;
        case JSON_ERR_BADCLOSER:        fprintf(stderr, "Object/array closer mismatch.\n"); break;
        case JSON_ERR_BADROOTTYPE:      fprintf(stderr, "Prohibited root value type.\n"); break;
        case JSON_ERR_EXPECTEDVALUE:    fprintf(stderr, "Value expected.\n"); break;
        case JSON_ERR_EXPECTEDKEY:      fprintf(stderr, "Key expected.\n"); break;
        case JSON_ERR_EXPECTEDVALUEORCLOSER: fprintf(stderr, "Value or closer expected.\n"); break;
        case JSON_ERR_EXPECTEDKEYORCLOSER: fprintf(stderr, "Key or closer expected.\n"); break;
        case JSON_ERR_EXPECTEDCOLON:    fprintf(stderr, "Colon ':' expected.\n"); break;
        case JSON_ERR_EXPECTEDCOMMAORCLOSER: fprintf(stderr, "Comma ',' or closer expected.\n"); break;
        case JSON_ERR_EXPECTEDEOF:      fprintf(stderr, "End of file expected.\n"); break;
        case JSON_ERR_MAXTOTALLEN:      fprintf(stderr, "Input file too long.\n"); break;
        case JSON_ERR_MAXTOTALVALUES:   fprintf(stderr, "Too many data records.\n"); break;
        case JSON_ERR_MAXNESTINGLEVEL:  fprintf(stderr, "Too deep object/array nesting.\n"); break;
        case JSON_ERR_MAXNUMBERLEN:     fprintf(stderr, "Too long number.\n"); break;
        case JSON_ERR_MAXSTRINGLEN:     fprintf(stderr, "Too long string.\n"); break;
        case JSON_ERR_MAXKEYLEN:        fprintf(stderr, "Too long key.\n"); break;
        case JSON_ERR_UNCLOSEDSTRING:   fprintf(stderr, "Unclosed string.\n"); break;
        case JSON_ERR_UNESCAPEDCONTROL: fprintf(stderr, "Unescaped control character.\n"); break;
        case JSON_ERR_INVALIDESCAPE:    fprintf(stderr, "Invalid escape sequence.\n"); break;
        case JSON_ERR_INVALIDUTF8:      fprintf(stderr, "Ill formed UTF-8.\n"); break;
        default:                        fprintf(stderr, "Unknown parsing error.\n"); break;
    }

    if(pos != NULL) {
        fprintf(stderr, "Offset: %lu\n", (unsigned long) pos->offset);
        fprintf(stderr, "Line:   %u\n", pos->line_number);
        fprintf(stderr, "Column: %u\n", pos->column_number);
    }
}
