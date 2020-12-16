[![Build status (travis-ci.com)](https://img.shields.io/travis/mity/centijson/master.svg?logo=linux&label=linux%20build)](https://travis-ci.com/mity/centijson)
[![Build status (appveyor.com)](https://img.shields.io/appveyor/ci/mity/centijson/master.svg?logo=windows&label=windows%20build)](https://ci.appveyor.com/project/mity/centijson/branch/master)
[![Codecov](https://img.shields.io/codecov/c/github/mity/centijson/master.svg?logo=codecov&label=code%20coverage)](https://codecov.io/github/mity/centijson)
[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/mity-centijson.svg?label=coverity%20scan)](https://scan.coverity.com/projects/mity-centijson)

# CentiJSON Readme

* Home: http://github.com/mity/centijson


## What is JSON

From http://json.org:

> JSON (JavaScript Object Notation) is a lightweight data-interchange format.
> It is easy for humans to read and write. It is easy for machines to parse
> and generate. It is based on a subset of the JavaScript Programming Language,
> Standard ECMA-262 3rd Edition - December 1999. JSON is a text format that is
> completely language independent but uses conventions that are familiar to
> programmers of the C-family of languages, including C, C++, C#, Java,
> JavaScript, Perl, Python, and many others. These properties make JSON an ideal
> data-interchange language.


## Main features:

* **Size:** The code size and memory footprint is relatively small.

* **Standard compliance** High emphasis is put on correctness and compliance
  with the JSON standards [ECMA-404], [RFC-8259] and [RFC-6901]. That includes:

  * **Full input validation:** During the parsing, CentiJSON verifies that the
    input forms valid JSON.

  * **String validation:** CentiJSON verifies that all strings are valid UTF-8
    (including corner cases like two Unicode escapes forming surrogate pairs).
    All JSON escape sequences are automatically translated to their respective
    Unicode counterparts.

* **Diagnostics:** In the case of an invalid input, you get more than just some
  failure flag, but also an information about nature of the issue and about its
  location in the document where it has been detected (offset as well as the
  line and column numbers are provided).

* **Security:** CentiJSON is intended to be usable even in situations where
  your application reads JSON from an untrusted source. That includes:

  * **Thorough testing and high code quality:** Having a lot of tests and
    maintaining their high code coverage, static code analysis and fuzz testing
    are all tools used commonly during the development and maintenance of
    CentiJSON.

  * **DoS mitigation:** The API allows good flexibility in imposing limits on
    the parsed input, including but not limited to, total length of the input,
    count of all the data records, maximal length of object keys or string
    values, maximal level of array/object nesting etc. This provides high degree
    of flexibility how to define policies for mitigation of Denial-of-Service
    attacks.

* **Modularity:** Do you need just SAX-like parser? Take just that. Do you
  need full DOM parser and AST representation? Take it all, it's still just
  few reasonably-sized C files (and corresponding headers).

  * **SAX-like parser:** Take just `json.h` + `json.c` and you have complete
    SAX-like parser. It's smart enough to verify JSON correctness, validate
    UTF-8, resolve escape  sequences and all the boring stuff. You can easily
    build DOM structure which fits your special needs or process the data on
    the fly.

  * **Full DOM parser:** `json-dom.h` + `json-dom.c` implements such a DOM
    builder on top of the SAX-like parser which populates the data storage
    implemented in `value.h` + `value.c`.

  * **Data storage:** The data storage module, `value.h` + `value.c` from
    [C Reusables](http://github.com/mity/c-reusables) is very versatile and
    it is not bound to the JSON parser implementation in any way, so you can
    reuse it for other purposes.

  * **JSON pointer:** JSON pointer module, `json-ptr.h` + `json-ptr.c`, which
    allows to query the data storage (`value.h` + `value.c`) as specified by
    [RFC-6901].

* **Streaming:** Ability to feed the parser with JSON input block by block
  (the blocks can be of an arbitrary size).

* **Serialization:**

  * **Low-level serialization:** `json.h` provides functions for outputting the
    non-trivial stuff like strings or numbers from C numeric types.

  * **High-level:** `json-dom.h` provides function `json_dom_dump()` which is
    capable to serialize whole DOM hierarchy.


## Performance

To be honest, we more focus on correctness and guaranteeing reasonable parsing
times for crazy input (the worst case) rather than for a simple uncomplicated
input. We should therefore be usable even for application reading the JSON from
an untrusted sources.

That for example means the objects in the DOM hierarchy are implemented as a
red-black tree and we can provide reasonable member lookup times (`log(n)`) no
matter how heavily populated the objects are.

Of course, building the RB-trees takes some CPU time and this may show in some
benchmarks, especially if they measure just the parsing and never perform any
lookup in heavily populated objects.

Also the support for the parsing block by block, in the streaming fashion,
means we cannot have as tight loops as some parsers which do not support this,
and this gives us a smaller space for some optimizations.

But even so, some preliminary tests we have done so far seem to indicate that
we are quite competitive.

(We will likely publish some real data on this in some foreseeable future.)


## Why Yet Another JSON Parser?

Indeed, there are already hundreds (if not thousands) JSON parsers written in
C out there. But as far as I know, they mostly fall into one of two categories.

The parsers in the 1st category are very small and simple (and quite often they
also take pride in it). They then usually have one or more shortcomings from
the following list:

* They usually expect full in-memory document to parse and do not allow parsing
  block by block;

* They usually allow no or very minimal configuration;

* They in almost all cases use an array or linked list for storing the children
  of JSON arrays as well as of JSON objects (and sometimes even for **all** the
  data in the JSON document), so searching the object by the key is operation
  of linear complexity.

  (That may be good enough if you really **know** that all the input will be
  always small. But allow any black hat feed it with some bigger beast and you
  have Denial of Service faster than you can spell it.)

* They often lack any possibility of modifying the tree of the data, like e.g.
  adding a new item into an array or an object, or removing an item from there.

* They often perform minimal or no UTF-8 encoding validation, do not perform
  full escape sequence resolution, or fall into troubles if any string contains
  U+0000 (`"foo\u0000bar"`).

The parsers in the 2nd category are far less numerous. They are usually very
huge beasts which provide many scores of functions, complicated abstraction
layers and/or baroque interfaces, and they are simply too big and complicated
for my taste or needs or will to incorporate them in my projects.

CentiJSON aims to reside somewhere in the no man's land, between the two
categories.


## Using CentiJSON

### SAX-like Parser

(Disclaimer: If you don't know what "SAX-like parser" means, you likely want
to see the section below about the DOM parser and ignore this section.)

If you want to use just the SAX-like parser, follow these steps:

1. Incorporate `src/json.h` and `src/json.c` into your project.

2. Use `#include "json.h"` in all relevant sources of your projects where
   you deal with JSON parsing.

3. Implement callback function, which is called anytime a scalar value (`null`,
   `false`, `true`, number or string) are encountered; or whenever a begin
   or end of a container (array or object) are encountered.

   To help with the implementation of the callback, you may call some utility
   functions to e.g. analyze a number found in the JSON input or to convert
   it to particular C types (see functions like e.g. `json_number_to_int32()`).

4. To parse a JSON input part by part (e.g. if you read the input by some
   blocks from a file), use `json_init()` + `json_feed()` + `json_fini()`.
   Or alternatively, if you have whole input in a single buffer, you may use
   `json_parse()` which wraps the three functions.

Note that CentiJSON fully verifies correctness of the input. But it is done on
the fly. Hence, if you feed the parser with broken JSON file, your callback
function can see e.g. a beginning of an array but not its end, if in the mean
time the parser aborts due to an error.

Hence, if the parsing as a whole fails (`json_fini()` or `json_parse()` returns
non-zero), you may still likely need to release any resources you allocated so
far as the callback has been called through out the process; and the code
dealing with that has to be ready the parsing is aborted at any point between
the calls of the callback.

See comments in `src/json.h` for more details about the API.

### DOM Parser

To use just the DOM parser, follow these steps:

1. Incorporate the sources `json.h`, `json.c`, `json-dom.h`, `json-dom.c`,
   `value.h` and `value.c` in the `src` directory into your project.

2. Use `#include "json-dom.h"` in all relevant sources of your projects where
   you deal with JSON parsing, and `#include "value.h"` in all sources where
   you query the parsed data.

3. To parse a JSON input part by part (e.g. if you read the input by some
   blocks from a file), use `json_dom_init()` + `json_dom_feed()` +
   `json_dom_fini()`. Or alternatively, if you have whole input in a single
   buffer, you may use `json_dom_parse()` which wraps the three functions.

4. If the parsing succeeds, the result (document object model or DOM) forms
   tree hierarchy of `VALUE` structures. Use all the power of the API in
   `value.h` to query (or modify) the data stored in it.

See comments in `src/json-dom.h` and `src/value.h` for more details about the
API.

### JSON Pointer

The JSON pointer module is an optional module on top of the DOM parser. To use
it, follow the instructions for the DOM parser, and add also the sources
`json-ptr.h` and `json-ptr.c` into your project.

### Outputting JSON

If you also need to output JSON, you may use low-level helper utilities
in `src/json.h` which are capable to output JSON numbers from C numeric types,
or JSON strings from C strings, handling all the hard stuff of the JSON syntax
like escaping of problematic characters. Writing the simple stuff like array or
object brackets, value delimiters etc. is kept on the application's shoulders.

Or, if you have DOM model represented by the `VALUE` structure hierarchy (as
provided by the DOM parser or crafted manually), call just `json_dom_dump()`.
This function, provided in `src/json-dom.h`, dumps the complete data hierarchy
into a JSON stream. It supports few options for formatting the output in the
desired way: E.g. it can indent the output to reflect the nesting of objects
and arrays; and it can also minimize the output by skipping any non-meaningful
whitespace altogether.

In either cases, you have to implement a writer callback, which is capable to
simply write down some sequence of bytes. This way, the application may save
the output into a file; send it over a network or whatever it wishes to do
with the stream.


## FAQ

**Q: Why `value.h` does not provide any API for objects?**

**A:** That module is not designed to be JSON-specific. The term "object", as
used in JSON context, is somewhat misleading outside of the context. Therefore
`value.h` instead uses more descriptive term "dictionary".

The following table shows how are JSON types translated to their counterparts
in `value.h`:

| JSON type | `json.h` type                       | `value.h` type        |
|-----------|-------------------------------------|-----------------------|
| null      | `JSON_NULL`                         | `VALUE_NULL`          |
| false     | `JSON_FALSE`                        | `VALUE_BOOL`          |
| true      | `JSON_TRUE`                         | `VALUE_BOOL`          |
| number    | `JSON_NUMBER`                       | see the next question |
| string    | `JSON_STRING`                       | `VALUE_STRING`        |
| array     | `JSON_ARRAY_BEG`+`JSON_ARRAY_END`   | `VALUE_ARRAY`         |
| object    | `JSON_OBJECT_BEG`+`JSON_OBJECT_END` | `VALUE_DICT`          |

**Q: How does CentiJSON deal with numbers?**

**A:** It's true that the untyped notion of the number type, as specified by
JSON standards, is a little bit complicated to deal with for languages like C.

On the SAX-like parser level, the syntax of numbers is verified accordingly
to the JSON standards and provided to the callback as a verbatim string.

The provided DOM builder (`json-dom.h`) tries to guess the most appropriate C
type how to store the number to mitigate any data loss by applying the rules
(the first applicable rule is used):
1. If there is no fraction and no exponent part and the integer fits into
   `VALUE_INT32`, then it shall be `VALUE_INT32`.
2. If there is no minus sign, no fraction or exponent part and the integer fits
   into `VALUE_UINT32`, then it shall be `VALUE_UINT32`.
3. If there is no fraction and no exponent part and the integer fits into
   `VALUE_INT64`, then it shall be `VALUE_INT64`.
4. If there is no minus sign, no fraction or exponent part and the integer fits
   into `VALUE_UINT64`, then it shall be `VALUE_UINT64`.
5. In all other cases, it shall be `VALUE_DOUBLE`.

That said, note that whatever numeric type is actually used for storing the
value, the getter functions of all those numeric values are capable to convert
the value into another C numeric types.

For example, you may use getter function `value_get_int32()` not only for values
of the type `VALUE_INT32`, but also for the other numeric values, e.g.
`VALUE_INT64` or `VALUE_DOUBLE`.

Naturally, the conversion may exhibit similar limitations as C casting,
including data loss (e.g. in the overflow situation) or rounding errors (e.g.
in double to integer conversion).

See the comments in the header `value.h` for more details.

**Q: Are there any hard-coded limits?**

**A:** No. There are only soft limits, configurable in run time by the
application and intended to be used as mitigation against Denial-of-Service
attacks.

Application can instruct the parser to use no limits by appropriate setup of
the structure `JSON_CONFIG` passed to `json_init()`. The only limitations are
then imposed by properties of your machine and OS.

**Q: Is CentiJSON thread-safe?**

**A:** Yes. You may parse as many documents in parallel as you like or your
machine is capable of. There is no global state and no need to synchronize
as long as each thread uses different parser instance.

(Of course, do not try parallelizing parsing of a single document. That makes
no sense, given the nature of JSON format.)

**Q: CentiJSON? Why such a horrible name?**

**A:** First, because I am poor in naming things. Second, because CentiJSON is
bigger than all those picojsons, nanojsons or microjsons; yet it's still quite
small, as the prefix suggests. Third, because it begins with the letter 'C',
and that refers to the C language. Forth, because the name reminds centipedes
and centipedes belong to Arthropods. The characteristic feature of this group
is their segmented body; similarly I see the modularity of CentiJSON as an
important competitive advantage in its niche. And last but not least, because
it seems it's not yet used for any other JSON implementation.


## License

CentiJSON is covered with MIT license, see the file `LICENSE.md`.


## Reporting Bugs

If you encounter any bug, please be so kind and report it. Unheard bugs cannot
get fixed. You can submit bug reports here:

* http://github.com/mity/centijson/issues



[ECMA-404]: http://www.ecma-international.org/publications/files/ECMA-ST/ECMA-404.pdf
[RFC-8259]: https://tools.ietf.org/html/rfc8259
[RFC-6901]: https://tools.ietf.org/html/rfc6901

