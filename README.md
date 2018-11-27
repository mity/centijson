[![Build status (travis-ci.com)](https://img.shields.io/travis/mity/centijson/master.svg?label=linux%20build)](https://travis-ci.org/mity/centijson)
[![Build status (appveyor.com)](https://img.shields.io/appveyor/ci/mity/centijson/master.svg?label=windows%20build)](https://ci.appveyor.com/project/mity/centijson/branch/master)
[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/mity-centijson.svg?label=coverity%20scan)](https://scan.coverity.com/projects/mity-centijson)
[![Codecov](https://img.shields.io/codecov/c/github/mity/centijson/master.svg?label=code%20coverage)](https://codecov.io/github/mity/centijson)

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


## Why Yet Another JSON Parser?

Indeed, there are already hundreds (if not thousands) JSON parsers written in
C out there. But as far as I know, they mostly fall into one of two categories.

In the 1st category, they are very small and simple (and quite often they also
take pride in it). But then they usually have one or more shortcomings from
this list:

* They usually expect full in-memory document to parse and do not allow parsing
  block by block;

* They usually allow no or very minimal configuration;

* They in almost all cases use an array or linked list for storing the children
  of JSON arrays as well as of JSON objects (and sometimes even for **all** the
  data in the JSON document), so searching the object by the key is operation
  of linear complexity.

  That may be good enough if you really **know** that all the input will be
  always small. But allow any black hat feed it with some bigger beast and you
  have Denial of Service faster then you spell it.

* Those really small ones usually often lack possibility of modifying the tree
  of the data, like adding new items into arrays or dictionaries, or removing
  them from there.

* They often perform minimal or no UTF-8 encoding validation, do not perform
  full escape sequence resolution, or fall into troubles if any string contains
  U+0000 (`"foo\u0000bar"`).

In the 2nd category, there are far less numerous, but very huge beasts which
provide many scores of functions, which provide very complicated data
abstraction layers and baroque interfaces, and they are simply too big and
complicated for my taste or needs or will to incorporate them in my projects.

CentiJSON aims to reside somewhere in the no man's land, between the two
categories.


## Main features:

* **Size:** The code and memory footprint is quite small.

* **Standard compliance** High emphasis is put on correctness and compliance
  with the JSON standards [ECMA-404] and [RFC-8259]. That includes:

  * **Full input validation:** During the parsing, CentiJSON verifies that the
    input forms valid JSON.

  * **String validation:** CentiJSON verifies that all strings are valid UTF-8
    (including surrogate pair correctness). All JSON escape sequences are
    automatically translated to their respective Unicode counterparts.

  * **Diagnostics:** In the case of invalid input, you get more then just some
    failure flag, but full information about nature of the issue, and also an
    information where in the document it has happened (offset as well as the
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
  need full DOM parser and representation? Take it all, it's still just three
  reasonably-sized C files (and three headers).

  * **SAX-like parser:** Take just `json.h` + `json.c` and you have complete
    SAX-like parser. It's smart enough to verify JSON correctness, validate
    UTF-8, resolve escape  sequences and all the boring stuff. You can easily
    build DOM structure which fits your special needs or process the data on
    the fly.

  * **Full DOM parser:** `json-dom.h` + `json-dom.c` implements such DOM
    builder on top of the SAX-like parser which populates the data storage.

  * **Data storage:** The data storage module, `value.h` + `value.c` from
    [C-reusables](http://github.com/mity/c-reusables) is very versatile and
    it is not bound to the JSON parser implementation in any way, so you can
    reuse it for other purposes.

* **Streaming:** Ability to feed the parser with JSON input step by step, in
  blocks of size the application chooses.

* **Performance:** The data storage provides great performance.

  * Arrays offer `O(1)` to access its elements and `O(1)` to add them to
    the end of the array. (Inserting or removing elements at the beginning or
    in the middle of the array is only `O(N)` but, naturally, arrays are
    populated from the beginning to the end during the JSON parsing.)

  * Objects (or dictionaries, in terms of `value.h`) are implemented internally
    as red-black trees and have `O(log N)` complexity for all the operations
    like accessing, adding or removing their items.


## FAQ

**Q: Why `value.h` does not provide any API for objects?**

**A:** That module is not designed to be JSON-specific, and the term "object"
as JSON uses it somewhat misleading outside of that context, so `value.h` uses
the term "dictionary" instead.

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

**A:** It's true that the untyped notion of number as JSON specifies it is
little bit complicated for languages like C.

On the SAX-like parser level, the syntax of numbers is verified accordingly
to the JSON standards and provided to the callback as verbatim string.

The provided DOM builder (`json-dom.h`) tries to guess most appropriate C type
how to store the number to mitigate any data loss, by following the following
rules (in this order; 1st one suitable is used.)
 * If there is no fraction and no exponent part and the integer fits into
   `VALUE_INT32`, then it shall be `VALUE_INT32`.
 * If there is no minus sign, no fraction or exponent part and the integer fits
   into `VALUE_UINT32`, then it shall be `VALUE_UINT32`.
 * If there is no fraction and no exponent part and the integer fits into
   `VALUE_INT64`, then it shall be `VALUE_INT64`.
 * If there is no minus sign, no fraction or exponent part and the integer fits
   into `VALUE_UINT64`, then it shall be `VALUE_UINT64`.
 * In all other cases, it shall be `VALUE_DOUBLE`.

That said, note that whatever numeric type is used for actually storing the
value, the getter functions of all those numeric values are capable to convert
the value into another C type. Naturally, the conversion may exhibit similar
limitations as C casting, including data loss or rounding errors.

See the comments in the header `value.h` for more details.

**Q: Are there any hard-coded limits?**

**A:** No. There are only soft-limits, configurable in run time by the
application and intended to be used as mitigation against Denial-of-Service
attacks.

Application can instruct the parser to use no limits by providing appropriately
filled `JSON_CONFIG` structure to `json_init()`. The only limitation are then
properties of your machine and OS.

**Q: Is CentiJSON thread-safe?**

**A:** Yes. You may parse as many documents in parallel as you like or your
machine is capable of. There is no global state and no need to synchronize
as long as each thread uses different parser instance.

(Of course, do not try parallelizing parsing of single document. That makes
no sense, given the nature of JSON format.)

**Q: CentiJSON? Why such a horrible name?**

**A:** First, because I am poor in naming things. Second, because CentiJSON is
bigger then all those picojsons, nanojsons or microjsons, but it's still quite
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
