// Copyright (c) 2026 'Konstantin Makarchev'

// MIT License

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// ------------------------------------------------------------------------------------------------
// # C++ Stress Test (https://github.com/kostya/index-cpp)

// An amalgamated, simplified version of the [LangArena](https://github.com/kostya/LangArena)
// suite for the C++ language.

// LangArena is a collection of 50 real-world-inspired tasks: JSON, Base64, CSV,
// neural networks, compression, maze A*, graph algorithms, sorting, hashing,
// interpreters, parallel matmul, and more.

// The file (index.cpp) serves three purposes at once:

// - **Compiler stress test** - a single ~350 KB translation unit exercises the
//   front end, optimizer, and back end.
// - **Runtime benchmark** - each test runs for roughly a second, so the total
//   runtime is around 52 seconds on my hardware.
// - **Correctness verifier** - every test checks its result against a checksum.
//   If any test fails, the program exits with code 1.

// Fully "all in": no external dependencies beyond the standard libraries.

// All parameters have been tuned for GCC 16 -O2. Timings may differ on other
// hardware.

// ### Build and run:

//     g++ -std=c++17 -w -O3 index.cpp -lpthread -o ./index
//     ./index

// ### Fast run, only test checksums

//     TEST=1 ./index

// ### Source includes:

// * https://github.com/DaveGamble/cJSON
//   MIT License (Copyright (c) 2009-2017 Dave Gamble and cJSON contributors)

// * https://github.com/kokke/tiny-regex-c
//   All material in this repository is in the public domain.

// * https://github.com/wareya/Remimu/
//   Creative Commons Legal Code


#include <iostream>
#include <iomanip>
#include <numeric>
#include <queue>
#include <stack>
#include <utility>
#include <memory>
#include <unordered_map>
#include <optional>
#include <variant>
#include <functional>
#include <array>
#include <algorithm>
#include <cmath>
#include <climits>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <thread>

class Helper {
public:
  static void reset();
  static int32_t next_int(int32_t max);
  static int32_t next_int(int32_t from, int32_t to);
  static double next_float(double max = 1.0);
  static uint32_t checksum(const std::string &v);
  static uint32_t checksum(const std::vector<uint8_t> &v);
  static uint32_t checksum_f64(double v);
  static int64_t config_i64(const std::string &class_name,
                            const std::string &field_name);
  static std::string config_s(const std::string &class_name,
                              const std::string &field_name);

private:
  static const int64_t IM = 139968;
  static const int64_t IA = 3877;
  static const int64_t IC = 29573;

  static thread_local int64_t last;
};

void load_config(const std::string &filename);
bool config_has(const std::string &key);
std::vector<std::string> config_keys();
int64_t config_i64(const std::string &key, const std::string &field);
std::string config_s(const std::string &key, const std::string &field);

double custom_round(double value, int32_t precision);
std::string to_lower(const std::string &str);

class Benchmark {
public:
  virtual ~Benchmark() = default;
  virtual void run(int) = 0;
  virtual uint32_t checksum() = 0;

  virtual void prepare() {}
  virtual std::string name() const = 0;

  int64_t warmup_iterations();

  virtual void warmup();
  void run_all();

  int64_t config_val(const std::string &field_name) const {
    return Helper::config_i64(this->name(), field_name);
  }
  int64_t iterations() const { return config_val("iterations"); }
  int64_t expected_checksum() const { return config_val("checksum"); }

  static void all(const std::string &single_bench);
};


// https://github.com/DaveGamble/cJSON/blob/master/cJSON.h
// ------------------------------------------------------------------------------------------
/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef cJSON__h
#define cJSON__h

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__

/* When compiling for windows, we specify a specific calling convention to avoid issues where we are being called from a project with a different default calling convention.  For windows you have 3 define options:

CJSON_HIDE_SYMBOLS - Define this in the case where you don't want to ever dllexport symbols
CJSON_EXPORT_SYMBOLS - Define this on library build when you want to dllexport symbols (default)
CJSON_IMPORT_SYMBOLS - Define this if you want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the CJSON_API_VISIBILITY flag to "export" the same symbols the way CJSON_EXPORT_SYMBOLS does

*/

#define CJSON_CDECL __cdecl
#define CJSON_STDCALL __stdcall

/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(CJSON_HIDE_SYMBOLS) && !defined(CJSON_IMPORT_SYMBOLS) && !defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_EXPORT_SYMBOLS
#endif

#if defined(CJSON_HIDE_SYMBOLS)
#define CJSON_PUBLIC(type)   type CJSON_STDCALL
#elif defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_PUBLIC(type)   __declspec(dllexport) type CJSON_STDCALL
#elif defined(CJSON_IMPORT_SYMBOLS)
#define CJSON_PUBLIC(type)   __declspec(dllimport) type CJSON_STDCALL
#endif
#else /* !__WINDOWS__ */
#define CJSON_CDECL
#define CJSON_STDCALL

#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(CJSON_API_VISIBILITY)
#define CJSON_PUBLIC(type)   __attribute__((visibility("default"))) type
#else
#define CJSON_PUBLIC(type) type
#endif
#endif

/* project version */
#define CJSON_VERSION_MAJOR 1
#define CJSON_VERSION_MINOR 7
#define CJSON_VERSION_PATCH 19

#include <stddef.h>

/* cJSON Types: */
#define cJSON_Invalid (0)
#define cJSON_False  (1 << 0)
#define cJSON_True   (1 << 1)
#define cJSON_NULL   (1 << 2)
#define cJSON_Number (1 << 3)
#define cJSON_String (1 << 4)
#define cJSON_Array  (1 << 5)
#define cJSON_Object (1 << 6)
#define cJSON_Raw    (1 << 7) /* raw json */

#define cJSON_IsReference 256
#define cJSON_StringIsConst 512

/* The cJSON structure: */
typedef struct cJSON
{
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct cJSON *next;
    struct cJSON *prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    struct cJSON *child;

    /* The type of the item, as above. */
    int type;

    /* The item's string, if type==cJSON_String  and type == cJSON_Raw */
    char *valuestring;
    /* writing to valueint is DEPRECATED, use cJSON_SetNumberValue instead */
    int valueint;
    /* The item's number, if type==cJSON_Number */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;
} cJSON;

typedef struct cJSON_Hooks
{
      /* malloc/free are CDECL on Windows regardless of the default calling convention of the compiler, so ensure the hooks allow passing those functions directly. */
      void *(CJSON_CDECL *malloc_fn)(size_t sz);
      void (CJSON_CDECL *free_fn)(void *ptr);
} cJSON_Hooks;

typedef int cJSON_bool;

/* Limits how deeply nested arrays/objects can be before cJSON rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef CJSON_NESTING_LIMIT
#define CJSON_NESTING_LIMIT 1000
#endif

/* Limits the length of circular references can be before cJSON rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef CJSON_CIRCULAR_LIMIT
#define CJSON_CIRCULAR_LIMIT 10000
#endif

/* returns the version of cJSON as a string */
CJSON_PUBLIC(const char*) cJSON_Version(void);

/* Supply malloc, realloc and free functions to cJSON */
CJSON_PUBLIC(void) cJSON_InitHooks(cJSON_Hooks* hooks);

/* Memory Management: the caller is always responsible to free the results from all variants of cJSON_Parse (with cJSON_Delete) and cJSON_Print (with stdlib free, cJSON_Hooks.free_fn, or cJSON_free as appropriate). The exception is cJSON_PrintPreallocated, where the caller has full responsibility of the buffer. */
/* Supply a block of JSON, and this returns a cJSON object you can interrogate. */
CJSON_PUBLIC(cJSON *) cJSON_Parse(const char *value);
CJSON_PUBLIC(cJSON *) cJSON_ParseWithLength(const char *value, size_t buffer_length);
/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error so will match cJSON_GetErrorPtr(). */
CJSON_PUBLIC(cJSON *) cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated);
CJSON_PUBLIC(cJSON *) cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end, cJSON_bool require_null_terminated);

/* Render a cJSON entity to text for transfer/storage. */
CJSON_PUBLIC(char *) cJSON_Print(const cJSON *item);
/* Render a cJSON entity to text for transfer/storage without any formatting. */
CJSON_PUBLIC(char *) cJSON_PrintUnformatted(const cJSON *item);
/* Render a cJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
CJSON_PUBLIC(char *) cJSON_PrintBuffered(const cJSON *item, int prebuffer, cJSON_bool fmt);
/* Render a cJSON entity to text using a buffer already allocated in memory with given length. Returns 1 on success and 0 on failure. */
/* NOTE: cJSON is not always 100% accurate in estimating how much memory it will use, so to be safe allocate 5 bytes more than you actually need */
CJSON_PUBLIC(cJSON_bool) cJSON_PrintPreallocated(cJSON *item, char *buffer, const int length, const cJSON_bool format);
/* Delete a cJSON entity and all subentities. */
CJSON_PUBLIC(void) cJSON_Delete(cJSON *item);

/* Returns the number of items in an array (or object). */
CJSON_PUBLIC(int) cJSON_GetArraySize(const cJSON *array);
/* Retrieve item number "index" from array "array". Returns NULL if unsuccessful. */
CJSON_PUBLIC(cJSON *) cJSON_GetArrayItem(const cJSON *array, int index);
/* Get item "string" from object. Case insensitive. */
CJSON_PUBLIC(cJSON *) cJSON_GetObjectItem(const cJSON * const object, const char * const string);
CJSON_PUBLIC(cJSON *) cJSON_GetObjectItemCaseSensitive(const cJSON * const object, const char * const string);
CJSON_PUBLIC(cJSON_bool) cJSON_HasObjectItem(const cJSON *object, const char *string);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when cJSON_Parse() returns 0. 0 when cJSON_Parse() succeeds. */
CJSON_PUBLIC(const char *) cJSON_GetErrorPtr(void);

/* Check item type and return its value */
CJSON_PUBLIC(char *) cJSON_GetStringValue(const cJSON * const item);
CJSON_PUBLIC(double) cJSON_GetNumberValue(const cJSON * const item);

/* These functions check the type of an item */
CJSON_PUBLIC(cJSON_bool) cJSON_IsInvalid(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsFalse(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsTrue(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsBool(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsNull(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsNumber(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsString(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsArray(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsObject(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsRaw(const cJSON * const item);

/* These calls create a cJSON item of the appropriate type. */
CJSON_PUBLIC(cJSON *) cJSON_CreateNull(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateTrue(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateFalse(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateBool(cJSON_bool boolean);
CJSON_PUBLIC(cJSON *) cJSON_CreateNumber(double num);
CJSON_PUBLIC(cJSON *) cJSON_CreateString(const char *string);
/* raw json */
CJSON_PUBLIC(cJSON *) cJSON_CreateRaw(const char *raw);
CJSON_PUBLIC(cJSON *) cJSON_CreateArray(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateObject(void);

/* Create a string where valuestring references a string so
 * it will not be freed by cJSON_Delete */
CJSON_PUBLIC(cJSON *) cJSON_CreateStringReference(const char *string);
/* Create an object/array that only references it's elements so
 * they will not be freed by cJSON_Delete */
CJSON_PUBLIC(cJSON *) cJSON_CreateObjectReference(const cJSON *child);
CJSON_PUBLIC(cJSON *) cJSON_CreateArrayReference(const cJSON *child);

/* These utilities create an Array of count items.
 * The parameter count cannot be greater than the number of elements in the number array, otherwise array access will be out of bounds.*/
CJSON_PUBLIC(cJSON *) cJSON_CreateIntArray(const int *numbers, int count);
CJSON_PUBLIC(cJSON *) cJSON_CreateFloatArray(const float *numbers, int count);
CJSON_PUBLIC(cJSON *) cJSON_CreateDoubleArray(const double *numbers, int count);
CJSON_PUBLIC(cJSON *) cJSON_CreateStringArray(const char *const *strings, int count);

/* Append item to the specified array/object. */
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToArray(cJSON *array, cJSON *item);
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the cJSON object.
 * WARNING: When this function was used, make sure to always check that (item->type & cJSON_StringIsConst) is zero before
 * writing to `item->string` */
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing cJSON to a new cJSON, but don't want to corrupt your existing cJSON. */
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item);

/* Remove/Detach items from Arrays/Objects. */
CJSON_PUBLIC(cJSON *) cJSON_DetachItemViaPointer(cJSON *parent, cJSON * const item);
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromArray(cJSON *array, int which);
CJSON_PUBLIC(void) cJSON_DeleteItemFromArray(cJSON *array, int which);
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObject(cJSON *object, const char *string);
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObjectCaseSensitive(cJSON *object, const char *string);
CJSON_PUBLIC(void) cJSON_DeleteItemFromObject(cJSON *object, const char *string);
CJSON_PUBLIC(void) cJSON_DeleteItemFromObjectCaseSensitive(cJSON *object, const char *string);

/* Update array items. */
CJSON_PUBLIC(cJSON_bool) cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem); /* Shifts pre-existing items to the right. */
CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemViaPointer(cJSON * const parent, cJSON * const item, cJSON * replacement);
CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem);
CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem);
CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInObjectCaseSensitive(cJSON *object,const char *string,cJSON *newitem);

/* Duplicate a cJSON item */
CJSON_PUBLIC(cJSON *) cJSON_Duplicate(const cJSON *item, cJSON_bool recurse);
/* Duplicate will create a new, identical cJSON item to the one you pass, in new memory that will
 * need to be released. With recurse!=0, it will duplicate any children connected to the item.
 * The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two cJSON items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
CJSON_PUBLIC(cJSON_bool) cJSON_Compare(const cJSON * const a, const cJSON * const b, const cJSON_bool case_sensitive);

/* Minify a strings, remove blank characters(such as ' ', '\t', '\r', '\n') from strings.
 * The input pointer json cannot point to a read-only address area, such as a string constant, 
 * but should point to a readable and writable address area. */
CJSON_PUBLIC(void) cJSON_Minify(char *json);

/* Helper functions for creating and adding items to an object at the same time.
 * They return the added item or NULL on failure. */
CJSON_PUBLIC(cJSON*) cJSON_AddNullToObject(cJSON * const object, const char * const name);
CJSON_PUBLIC(cJSON*) cJSON_AddTrueToObject(cJSON * const object, const char * const name);
CJSON_PUBLIC(cJSON*) cJSON_AddFalseToObject(cJSON * const object, const char * const name);
CJSON_PUBLIC(cJSON*) cJSON_AddBoolToObject(cJSON * const object, const char * const name, const cJSON_bool boolean);
CJSON_PUBLIC(cJSON*) cJSON_AddNumberToObject(cJSON * const object, const char * const name, const double number);
CJSON_PUBLIC(cJSON*) cJSON_AddStringToObject(cJSON * const object, const char * const name, const char * const string);
CJSON_PUBLIC(cJSON*) cJSON_AddRawToObject(cJSON * const object, const char * const name, const char * const raw);
CJSON_PUBLIC(cJSON*) cJSON_AddObjectToObject(cJSON * const object, const char * const name);
CJSON_PUBLIC(cJSON*) cJSON_AddArrayToObject(cJSON * const object, const char * const name);

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define cJSON_SetIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the cJSON_SetNumberValue macro */
CJSON_PUBLIC(double) cJSON_SetNumberHelper(cJSON *object, double number);
#define cJSON_SetNumberValue(object, number) ((object != NULL) ? cJSON_SetNumberHelper(object, (double)number) : (number))
/* Change the valuestring of a cJSON_String object, only takes effect when type of object is cJSON_String */
CJSON_PUBLIC(char*) cJSON_SetValuestring(cJSON *object, const char *valuestring);

/* If the object is not a boolean type this does nothing and returns cJSON_Invalid else it returns the new type*/
#define cJSON_SetBoolValue(object, boolValue) ( \
    (object != NULL && ((object)->type & (cJSON_False|cJSON_True))) ? \
    (object)->type=((object)->type &(~(cJSON_False|cJSON_True)))|((boolValue)?cJSON_True:cJSON_False) : \
    cJSON_Invalid\
)

/* Macro for iterating over an array or object */
#define cJSON_ArrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

/* malloc/free objects using the malloc/free functions that have been set with cJSON_InitHooks */
CJSON_PUBLIC(void *) cJSON_malloc(size_t size);
CJSON_PUBLIC(void) cJSON_free(void *object);

#ifdef __cplusplus
}
#endif

#endif
// ------------------------------------------------------------------------------------------

// https://github.com/kokke/tiny-regex-c/blob/master/re.h
// ------------------------------------------------------------------------------------------
// This is free and unencumbered software released into the public domain.

// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// For more information, please refer to <http://unlicense.org>

/*
 *
 * Mini regex-module inspired by Rob Pike's regex code described in:
 *
 * http://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html
 *
 *
 *
 * Supports:
 * ---------
 *   '.'        Dot, matches any character
 *   '^'        Start anchor, matches beginning of string
 *   '$'        End anchor, matches end of string
 *   '*'        Asterisk, match zero or more (greedy)
 *   '+'        Plus, match one or more (greedy)
 *   '?'        Question, match zero or one (non-greedy)
 *   '[abc]'    Character class, match if one of {'a', 'b', 'c'}
 *   '[^abc]'   Inverted class, match if NOT one of {'a', 'b', 'c'} -- NOTE: feature is currently broken!
 *   '[a-zA-Z]' Character ranges, the character set of the ranges { a-z | A-Z }
 *   '\s'       Whitespace, \t \f \r \n \v and spaces
 *   '\S'       Non-whitespace
 *   '\w'       Alphanumeric, [a-zA-Z0-9_]
 *   '\W'       Non-alphanumeric
 *   '\d'       Digits, [0-9]
 *   '\D'       Non-digits
 *
 *
 */

#ifndef _TINY_REGEX_C
#define _TINY_REGEX_C


#ifndef RE_DOT_MATCHES_NEWLINE
/* Define to 0 if you DON'T want '.' to match '\r' + '\n' */
#define RE_DOT_MATCHES_NEWLINE 1
#endif

#ifdef __cplusplus
extern "C"{
#endif



/* Typedef'd pointer to get abstract datatype. */
typedef struct regex_t* re_t;


/* Compile regex string pattern to a regex_t-array. */
re_t re_compile(const char* pattern);


/* Find matches of the compiled pattern inside text. */
int re_matchp(re_t pattern, const char* text, int* matchlength);


/* Find matches of the txt pattern inside text (will compile automatically first). */
int re_match(const char* pattern, const char* text, int* matchlength);


#ifdef __cplusplus
}
#endif

#endif /* ifndef _TINY_REGEX_C */

// ------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------


// https://github.com/wareya/Remimu/blob/main/remimu.h
// Creative Commons Legal Code

// CC0 1.0 Universal

//     CREATIVE COMMONS CORPORATION IS NOT A LAW FIRM AND DOES NOT PROVIDE
//     LEGAL SERVICES. DISTRIBUTION OF THIS DOCUMENT DOES NOT CREATE AN
//     ATTORNEY-CLIENT RELATIONSHIP. CREATIVE COMMONS PROVIDES THIS
//     INFORMATION ON AN "AS-IS" BASIS. CREATIVE COMMONS MAKES NO WARRANTIES
//     REGARDING THE USE OF THIS DOCUMENT OR THE INFORMATION OR WORKS
//     PROVIDED HEREUNDER, AND DISCLAIMS LIABILITY FOR DAMAGES RESULTING FROM
//     THE USE OF THIS DOCUMENT OR THE INFORMATION OR WORKS PROVIDED
//     HEREUNDER.

// Statement of Purpose

// The laws of most jurisdictions throughout the world automatically confer
// exclusive Copyright and Related Rights (defined below) upon the creator
// and subsequent owner(s) (each and all, an "owner") of an original work of
// authorship and/or a database (each, a "Work").

// Certain owners wish to permanently relinquish those rights to a Work for
// the purpose of contributing to a commons of creative, cultural and
// scientific works ("Commons") that the public can reliably and without fear
// of later claims of infringement build upon, modify, incorporate in other
// works, reuse and redistribute as freely as possible in any form whatsoever
// and for any purposes, including without limitation commercial purposes.
// These owners may contribute to the Commons to promote the ideal of a free
// culture and the further production of creative, cultural and scientific
// works, or to gain reputation or greater distribution for their Work in
// part through the use and efforts of others.

// For these and/or other purposes and motivations, and without any
// expectation of additional consideration or compensation, the person
// associating CC0 with a Work (the "Affirmer"), to the extent that he or she
// is an owner of Copyright and Related Rights in the Work, voluntarily
// elects to apply CC0 to the Work and publicly distribute the Work under its
// terms, with knowledge of his or her Copyright and Related Rights in the
// Work and the meaning and intended legal effect of CC0 on those rights.

// 1. Copyright and Related Rights. A Work made available under CC0 may be
// protected by copyright and related or neighboring rights ("Copyright and
// Related Rights"). Copyright and Related Rights include, but are not
// limited to, the following:

//   i. the right to reproduce, adapt, distribute, perform, display,
//      communicate, and translate a Work;
//  ii. moral rights retained by the original author(s) and/or performer(s);
// iii. publicity and privacy rights pertaining to a person's image or
//      likeness depicted in a Work;
//  iv. rights protecting against unfair competition in regards to a Work,
//      subject to the limitations in paragraph 4(a), below;
//   v. rights protecting the extraction, dissemination, use and reuse of data
//      in a Work;
//  vi. database rights (such as those arising under Directive 96/9/EC of the
//      European Parliament and of the Council of 11 March 1996 on the legal
//      protection of databases, and under any national implementation
//      thereof, including any amended or successor version of such
//      directive); and
// vii. other similar, equivalent or corresponding rights throughout the
//      world based on applicable law or treaty, and any national
//      implementations thereof.

// 2. Waiver. To the greatest extent permitted by, but not in contravention
// of, applicable law, Affirmer hereby overtly, fully, permanently,
// irrevocably and unconditionally waives, abandons, and surrenders all of
// Affirmer's Copyright and Related Rights and associated claims and causes
// of action, whether now known or unknown (including existing as well as
// future claims and causes of action), in the Work (i) in all territories
// worldwide, (ii) for the maximum duration provided by applicable law or
// treaty (including future time extensions), (iii) in any current or future
// medium and for any number of copies, and (iv) for any purpose whatsoever,
// including without limitation commercial, advertising or promotional
// purposes (the "Waiver"). Affirmer makes the Waiver for the benefit of each
// member of the public at large and to the detriment of Affirmer's heirs and
// successors, fully intending that such Waiver shall not be subject to
// revocation, rescission, cancellation, termination, or any other legal or
// equitable action to disrupt the quiet enjoyment of the Work by the public
// as contemplated by Affirmer's express Statement of Purpose.

// 3. Public License Fallback. Should any part of the Waiver for any reason
// be judged legally invalid or ineffective under applicable law, then the
// Waiver shall be preserved to the maximum extent permitted taking into
// account Affirmer's express Statement of Purpose. In addition, to the
// extent the Waiver is so judged Affirmer hereby grants to each affected
// person a royalty-free, non transferable, non sublicensable, non exclusive,
// irrevocable and unconditional license to exercise Affirmer's Copyright and
// Related Rights in the Work (i) in all territories worldwide, (ii) for the
// maximum duration provided by applicable law or treaty (including future
// time extensions), (iii) in any current or future medium and for any number
// of copies, and (iv) for any purpose whatsoever, including without
// limitation commercial, advertising or promotional purposes (the
// "License"). The License shall be deemed effective as of the date CC0 was
// applied by Affirmer to the Work. Should any part of the License for any
// reason be judged legally invalid or ineffective under applicable law, such
// partial invalidity or ineffectiveness shall not invalidate the remainder
// of the License, and in such case Affirmer hereby affirms that he or she
// will not (i) exercise any of his or her remaining Copyright and Related
// Rights in the Work or (ii) assert any associated claims and causes of
// action with respect to the Work, in either case contrary to Affirmer's
// express Statement of Purpose.

// 4. Limitations and Disclaimers.

//  a. No trademark or patent rights held by Affirmer are waived, abandoned,
//     surrendered, licensed or otherwise affected by this document.
//  b. Affirmer offers the Work as-is and makes no representations or
//     warranties of any kind concerning the Work, express, implied,
//     statutory or otherwise, including without limitation warranties of
//     title, merchantability, fitness for a particular purpose, non
//     infringement, or the absence of latent or other defects, accuracy, or
//     the present or absence of errors, whether or not discoverable, all to
//     the greatest extent permissible under applicable law.
//  c. Affirmer disclaims responsibility for clearing rights of other persons
//     that may apply to the Work or any use thereof, including without
//     limitation any person's Copyright and Related Rights in the Work.
//     Further, Affirmer disclaims responsibility for obtaining any necessary
//     consents, permissions or other rights required for any use of the
//     Work.
//  d. Affirmer understands and acknowledges that Creative Commons is not a
//     party to this document and has no duty or obligation with respect to
//     this CC0 or use of the Work.
// ------------------------------------------------------------------------------------------
#ifndef INCLUDE_REMIMU
#define INCLUDE_REMIMU 1

#ifndef REMIMU_FUNC_VISIBILITY
#define REMIMU_FUNC_VISIBILITY static inline
#endif

#ifndef REMIMU_CONST_VISIBILITY
#define REMIMU_CONST_VISIBILITY static const
#endif

#ifndef REMIMU_LOG_ERROR
#define REMIMU_LOG_ERROR puts
#endif

#ifndef REMIMU_ITERATION_LIMIT
#define REMIMU_ITERATION_LIMIT 0 // Set to non-zero to enable an interation limit
#endif

#ifndef REMIMU_ASSERT
#define REMIMU_ASSERT(x) assert(x)
#endif

/************

    REMIMU: SINGLE HEADER C/C++ REGEX LIBRARY

    Compatible with C99 and C++11 and later standards. Uses backtracking and relatively standard regex syntax.

    #include "remimu.h"

FUNCTIONS

    // Returns 0 on success, or -1 on invalid or unsupported regex, or -2 on not enough tokens given to parse regex.
    int regex_parse(
        const char * pattern,       // Regex pattern to parse.
        RegexToken * tokens,        // Output buffer of token_count regex tokens.
        int16_t * token_count,      // Maximum allowed number of tokens to write
        int32_t flags               // Optional bitflags.
    )

    // Returns match length, or -1 on no match, or -2 on out of memory, or -3 if the regex is invalid.
    int64_t regex_match(
        const RegexToken * tokens,  // Parsed regex to match against text.
        const char * text,          // Text to match against tokens.
        size_t start_i,             // index value to match at.
        uint16_t cap_slots,         // Number of allowed capture info output slots.
        int64_t * cap_pos,          // Capture position info output buffer.
        int64_t * cap_span          // Capture length info output buffer.
    )

    void print_regex_tokens(
        RegexToken * tokens     // Regex tokens to spew to stdout, for debugging.
    )

PERFORMANCE

    On simple cases, Remimu's match speed is similar to PCRE2. Regex parsing/compilation is also much faster (around 4x to 10x), so single-shot regexes are often faster than PCRE2.

    HOWEVER: Remimu is a pure backtracking engine, and has `O(2^x)` complexity on regexes with catastrophic backtracking. It can be much, much, MUCH slower than PCRE2. Beware!

    Remimu uses length-checked fixed memory buffers with no recursion, so memory usage is statically known.

FEATURES

    - Lowest-common-denominator common regex syntax
    - Based on backtracking (slow in the worst case, but fast in the best case)
    - 8-bit only, no utf-16 or utf-32
    - Statically known memory usage (no heap allocation or recursion)
    - Groups with or without capture, and with or without quantifiers
    - Supported escapes:
    - - 2-digit hex: e.g. \x00, \xFF, or lowercase, or mixed case
    - - \r, \n, \t, \v, \f (whitespace characters)
    - - \d, \s, \w, \D, \S, \W (digit, space, and word character classes)
    - - \b, \B word boundary and non-word-boundary anchors (not fully supported in zero-size quantified groups, but even then, usually supported)
    - - Escaped literal characters: {}[]-()|^$*+?:./\
    - - - Escapes work in character classes, except for 'b'
    - Character classes, including disjoint ranges, proper handling of bare [ and trailing -, etc
    - - Dot (.) matches all characters, including newlines, unless REMIMU_FLAG_DOT_NO_NEWLINES is passed as a flag to regex_parse
    - - Dot (.) only matches at most one byte at a time, so matching \r\n requires two dots (and not using REMIMU_FLAG_DOT_NO_NEWLINES)
    - Anchors (^ and $)
    - - Same support caveats as \b, \B apply
    - Basic quantifiers (*, +, ?)
    - - Quantifiers are greedy by default.
    - Explicit quantifiers ({2}, {5}, {5,}, {5,7})
    - Alternation e.g. (asdf|foo)
    - Lazy quantifiers e.g. (asdf)*? or \w+?
    - Possessive greedy quantifiers e.g. (asdf)*+ or \w++
    - - NOTE: Capture groups for and inside of possessive groups return no capture information.
    - Atomic groups e.g. (?>(asdf))
    - - NOTE: Capture groups inside of atomic groups return no capture information.

NOT SUPPORTED

    - Strings with non-terminal null characters
    - Unicode character classes (matching single utf-8 characters works regardless)
    - Exact POSIX regex semantics (posix-style greediness etc)
    - Backreferences
    - Lookbehind/Lookahead
    - Named groups
    - Most other weird flavor-specific regex stuff
    - Capture of or inside of possessive-quantified groups (still take up a capture slot, but no data is returned)

USAGE

    // minimal:

    RegexToken tokens[1024];
    int16_t token_count = 1024;
    int e = regex_parse("[0-9]+\\.[0-9]+", tokens, &token_count, 0);
    assert(!e);

    int64_t match_len = regex_match(tokens, "23.53) ", 0, 0, 0, 0);
    printf("########### return: %zd\n", match_len);

    // with captures:

    RegexToken tokens[256];
    int16_t token_count = sizeof(tokens)/sizeof(tokens[0]);
    int e = regex_parse("((a)|(b))++", tokens, &token_count, 0);
    assert(!e);

    int64_t cap_pos[5];
    int64_t cap_span[5];
    memset(cap_pos, 0xFF, sizeof(cap_pos));
    memset(cap_span, 0xFF, sizeof(cap_span));

    int64_t matchlen = regex_match(tokens, "aaaaaabbbabaqa", 0, 5, cap_pos, cap_span);
    printf("Match length: %zd\n", matchlen);
    for (int i = 0; i < 5; i++)
        printf("Capture %d: %zd plus %zd\n", i, cap_pos[i], cap_span[i]);

    // for debugging
    print_regex_tokens(tokens);

LICENSE

    Creative Commons Zero, public domain.

*/

#include <stddef.h>
#include <string.h>
#include <assert.h>

REMIMU_CONST_VISIBILITY int REMIMU_FLAG_DOT_NO_NEWLINES = 1;

REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_NORMAL      = 0;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_OPEN        = 1;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_NCOPEN      = 2;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_CLOSE       = 3;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_OR          = 4;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_CARET       = 5;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_DOLLAR      = 6;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_BOUND       = 7;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_NBOUND      = 8;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_END         = 9;

REMIMU_CONST_VISIBILITY uint8_t REMIMU_MODE_POSSESSIVE  = 1;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_MODE_LAZY        = 2;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_MODE_INVERTED    = 128; // temporary; gets cleared later

typedef struct _RegexToken {
    uint8_t kind;
    uint8_t mode;
    uint16_t count_lo;
    uint16_t count_hi; // 0 means no limit
    uint16_t mask[16]; // for groups: mask 0 stores group-with-quantifier number (quantifiers are +, *, ?, {n}, {n,}, or {n,m})
    int16_t pair_offset; // from ( or ), offset in token list to matching paren. TODO: move into mask maybe
} RegexToken;

static int remimu_nibble_hex_to_bin(char hex, uint8_t *bin)
{
    if (hex >= '0' && hex <= '9')
    {
        *bin = hex - '0';
        return 0;
    }
    if (hex >= 'A' && hex <= 'F')
    {
        *bin = hex - 'A' + 10;
        return 0;
    }
    if (hex >= 'a' && hex <= 'f')
    {
        *bin = hex - 'a' + 10;
        return 0;
    }
    return -1; // invalid hex digit
}

/// Returns a negative number on failure:
/// -1: Regex string is invalid or using unsupported features or too long.
/// -2: Provided buffer not long enough. Give up, or reallocate with more length and retry.
/// Returns 0 on success.
/// On call, token_count pointer must point to the number of tokens that can be written to the tokens buffer.
/// On successful return, the number of actually used tokens is written to token_count.
/// Sets token_count to zero if a regex is not created but no error happened (e.g. empty pattern).
/// Flags: Not yet used.
/// SAFETY: Pattern must be null-terminated.
/// SAFETY: tokens buffer must have at least the input token_count number of RegexToken objects. They are allowed to be uninitialized.
REMIMU_FUNC_VISIBILITY int regex_parse(const char * pattern, RegexToken * tokens, int16_t * token_count, int32_t flags)
{
    int64_t tokens_len = *token_count;
    uint64_t pattern_len = strlen(pattern);
    if (token_count == 0)
        return -2;

    // 0: normal
    // 1: just saw a backslash
    int esc_state = 0;

    // 0: init
    // 1: normal
    // 2: in char class, initial state
    // 3: in char class, but possibly looking for a range marker
    // 4: in char class, but just saw a range marker
    // 5: immediately after quantifiable token
    // 6: immediately after quantifier

    const int STATE_NORMAL    = 1;
    const int STATE_QUANT     = 2;
    const int STATE_MODE      = 3;
    const int STATE_CC_INIT   = 4;
    const int STATE_CC_NORMAL = 5;
    const int STATE_CC_RANGE  = 6;
    int state = STATE_NORMAL;

    int char_class_mem = -1;

    RegexToken token;

    #define _REGEX_CLEAR_TOKEN() do { \
        memset(&token, 0, sizeof(RegexToken)); \
        token.count_lo = 1; \
        token.count_hi = 2; \
    } while(0)

    _REGEX_CLEAR_TOKEN();

    #define _REGEX_DO_INVERT() do { \
        for (int n = 0; n < 16; n++) \
            token.mask[n] = ~token.mask[n]; \
        token.mode &= ~REMIMU_MODE_INVERTED; \
    } while (0)

    int16_t k = 0;

    #define _REGEX_PUSH_TOKEN() do { \
        if (k == 0 || tokens[k-1].kind != token.kind || (token.kind != REMIMU_KIND_BOUND && token.kind != REMIMU_KIND_NBOUND)) \
        { \
            if (token.mode & REMIMU_MODE_INVERTED) _REGEX_DO_INVERT(); \
            if (k >= tokens_len) \
            { \
                REMIMU_LOG_ERROR("buffer overflow"); \
                return -2; \
            } \
            tokens[k++] = token; \
            _REGEX_CLEAR_TOKEN(); \
        } \
    } while (0)

    #define _REGEX_SET_MASK(byte) do { token.mask[((uint8_t)(byte))>>4] |= 1 << ((uint8_t)(byte) & 0xF); } while (0)
    #define _REGEX_SET_MASK_ALL() do { \
        for (int n = 0; n < 16; n++) \
            token.mask[n] = 0xFFFF; \
    } while (0)

    // start with an invisible group specifier
    // (this allows the matcher to not need to have a special root-level alternation operator case)
    token.kind = REMIMU_KIND_OPEN;
    token.count_lo = 0;
    token.count_hi = 0;

    int paren_count = 0;

    for (uint64_t i = 0; i < pattern_len; i++)
    {
        char c = pattern[i];
        if (state == STATE_QUANT)
        {
            state = STATE_MODE;
            if (c == '?')
            {
                token.count_lo = 0;
                token.count_hi = 2; // first non-allowed amount
                continue;
            }
            else if (c == '+')
            {
                token.count_lo = 1;
                token.count_hi = 0; // unlimited
                continue;
            }
            else if (c == '*')
            {
                token.count_lo = 0;
                token.count_hi = 0; // unlimited
                continue;
            }
            else if (c == '{')
            {
                if (pattern[i+1] == 0 || pattern[i+1] < '0' || pattern[i+1] > '9')
                    state = STATE_NORMAL;
                else
                {
                    i += 1;
                    uint32_t val = 0;
                    while (pattern[i] >= '0' && pattern[i] <= '9')
                    {
                        val *= 10;
                        val += (uint32_t)(pattern[i] - '0');
                        if (val > 0xFFFF)
                        {
                            REMIMU_LOG_ERROR("quantifier range too long");
                            return -1; // unsupported length
                        }
                        i += 1;
                    }
                    token.count_lo = val;
                    token.count_hi = val + 1;
                    if (pattern[i] == ',')
                    {
                        token.count_hi = 0; // unlimited
                        i += 1;

                        if (pattern[i] >= '0' && pattern[i] <= '9')
                        {
                            uint32_t val2 = 0;
                            while (pattern[i] >= '0' && pattern[i] <= '9')
                            {
                                val2 *= 10;
                                val2 += (uint32_t)(pattern[i] - '0');
                                if (val2 > 0xFFFF)
                                {
                                    REMIMU_LOG_ERROR("quantifier range too long");
                                    return -1; // unsupported length
                                }
                                i += 1;
                            }
                            if (val2 < val)
                            {
                                REMIMU_LOG_ERROR("quantifier range is backwards");
                                return -1; // unsupported length
                            }
                            token.count_hi = val2 + 1;
                        }
                    }

                    if (pattern[i] == '}')
                    {
                        // quantifier range parsed successfully
                        continue;
                    }
                    else
                    {
                        REMIMU_LOG_ERROR("quantifier range syntax broken (no terminator)");
                        return -1;
                    }
                }
            }
        }

        if (state == STATE_MODE)
        {
            state = STATE_NORMAL;
            if (c == '?')
            {
                token.mode |= REMIMU_MODE_LAZY;
                continue;
            }
            else if (c == '+')
            {
                token.mode |= REMIMU_MODE_POSSESSIVE;
                continue;
            }
        }

        if (state == STATE_NORMAL)
        {
            if (esc_state == 1)
            {
                esc_state = 0;
                if (c == 'n')
                    _REGEX_SET_MASK('\n');
                else if (c == 'r')
                    _REGEX_SET_MASK('\r');
                else if (c == 't')
                    _REGEX_SET_MASK('\t');
                else if (c == 'v')
                    _REGEX_SET_MASK('\v');
                else if (c == 'f')
                    _REGEX_SET_MASK('\f');
                else if (c == 'x')
                {
                    if (pattern[i+1] == 0 || pattern[i+2] == 0)
                        return -1; // too-short hex pattern
                    uint8_t n0, n1;
                    if (remimu_nibble_hex_to_bin(pattern[i+1], &n0))
                        return -1; // invalid hex
                    if (remimu_nibble_hex_to_bin(pattern[i+2], &n1))
                        return -1; // invalid hex
                    _REGEX_SET_MASK((n0 << 4) | n1);
                    i += 2;
                    state = STATE_QUANT;
                }
                else if (c == '{' || c == '}' ||
                         c == '[' || c == ']' || c == '-' ||
                         c == '(' || c == ')' ||
                         c == '|' || c == '^' || c == '$' ||
                         c == '*' || c == '+' || c == '?' || c == ':' ||
                         c == '.' || c == '/' || c == '\\')
                {
                    _REGEX_SET_MASK(c);
                    state = STATE_QUANT;
                }
                else if (c == 'd' || c == 's' || c == 'w' ||
                         c == 'D' || c == 'S' || c == 'W')
                {
                    uint8_t is_upper = c <= 'Z';

                    uint16_t m[16];
                    memset(m, 0, sizeof(m));

                    if (is_upper)
                        c += 0x20;
                    if (c == 'd' || c == 'w')
                        m[3] |= 0x03FF; // 0~7
                    if (c == 's')
                    {
                        m[0] |= 0x3E00; // \t-\r (includes \n, \v, and \f in the middle. 5 enabled bits.)
                        m[2] |= 1; // ' '
                    }
                    if (c == 'w')
                    {
                        m[4] |= 0xFFFE; // A-O
                        m[5] |= 0x87FF; // P-Z_
                        m[6] |= 0xFFFE; // a-o
                        m[7] |= 0x07FF; // p-z
                    }

                    for (int j = 0; j < 16; j++)
                        token.mask[j] |= is_upper ? ~m[j] : m[j];

                    token.kind = REMIMU_KIND_NORMAL;
                    state = STATE_QUANT;
                }
                else if (c == 'b')
                {
                    token.kind = REMIMU_KIND_BOUND;
                    state = STATE_NORMAL;
                }
                else if (c == 'B')
                {
                    token.kind = REMIMU_KIND_NBOUND;
                    state = STATE_NORMAL;
                }
                else
                {
                    REMIMU_LOG_ERROR("unsupported escape sequence");
                    return -1; // unknown/unsupported escape sequence
                }
            }
            else
            {
                _REGEX_PUSH_TOKEN();
                if (c == '\\')
                {
                    esc_state = 1;
                }
                else if (c == '[')
                {
                    state = STATE_CC_INIT;
                    char_class_mem = -1;
                    token.kind = REMIMU_KIND_NORMAL;
                    if (pattern[i + 1] == '^')
                    {
                        token.mode |= REMIMU_MODE_INVERTED;
                        i += 1;
                    }
                }
                else if (c == '(')
                {
                    paren_count += 1;
                    state = STATE_NORMAL;
                    token.kind = REMIMU_KIND_OPEN;
                    token.count_lo = 0;
                    token.count_hi = 1;
                    if (pattern[i + 1] == '?' && pattern[i + 2] == ':')
                    {
                        token.kind = REMIMU_KIND_NCOPEN;
                        i += 2;
                    }
                    else if (pattern[i + 1] == '?' && pattern[i + 2] == '>')
                    {
                        token.kind = REMIMU_KIND_NCOPEN;
                        _REGEX_PUSH_TOKEN();

                        state = STATE_NORMAL;
                        token.kind = REMIMU_KIND_NCOPEN;
                        token.mode = REMIMU_MODE_POSSESSIVE;
                        token.count_lo = 1;
                        token.count_hi = 2;

                        i += 2;
                    }
                }
                else if (c == ')')
                {
                    paren_count -= 1;
                    if (paren_count < 0 || k == 0)
                        return -1; // unbalanced parens
                    token.kind = REMIMU_KIND_CLOSE;
                    state = STATE_QUANT;

                    int balance = 0;
                    ptrdiff_t found = -1;
                    for (ptrdiff_t l = k - 1; l >= 0; l--)
                    {
                        if (tokens[l].kind == REMIMU_KIND_NCOPEN || tokens[l].kind == REMIMU_KIND_OPEN)
                        {
                            if (balance == 0)
                            {
                                found = l;
                                break;
                            }
                            else
                                balance -= 1;
                        }
                        else if (tokens[l].kind == REMIMU_KIND_CLOSE)
                            balance += 1;
                    }
                    if (found == -1)
                        return -1; // unbalanced parens
                    ptrdiff_t diff = k - found;
                    if (diff > 32767)
                        return -1; // too long
                    token.pair_offset = -diff;
                    tokens[found].pair_offset = diff;
                    // phantom group for atomic group emulation
                    if (tokens[found].mode == REMIMU_MODE_POSSESSIVE)
                    {
                        _REGEX_PUSH_TOKEN();
                        token.kind = REMIMU_KIND_CLOSE;
                        token.mode = REMIMU_MODE_POSSESSIVE;
                        token.pair_offset = -diff - 2;
                        tokens[found - 1].pair_offset = diff + 2;
                    }
                }
                else if (c == '?' || c == '+' || c == '*' || c == '{')
                {
                    REMIMU_LOG_ERROR("quantifier in non-quantifier context");
                    return -1; // quantifier in non-quantifier context
                }
                else if (c == '.')
                {
                    //puts("setting ALL of mask...");
                    _REGEX_SET_MASK_ALL();
                    if (flags & REMIMU_FLAG_DOT_NO_NEWLINES)
                    {
                        token.mask[1] ^= 0x04; // \n
                        token.mask[1] ^= 0x20; // \r
                    }
                    state = STATE_QUANT;
                }
                else if (c == '^')
                {
                    token.kind = REMIMU_KIND_CARET;
                    state = STATE_NORMAL;
                }
                else if (c == '$')
                {
                    token.kind = REMIMU_KIND_DOLLAR;
                    state = STATE_NORMAL;
                }
                else if (c == '|')
                {
                    token.kind = REMIMU_KIND_OR;
                    state = STATE_NORMAL;
                }
                else
                {
                    _REGEX_SET_MASK(c);
                    state = STATE_QUANT;
                }
            }
        }
        else if (state == STATE_CC_INIT || state == STATE_CC_NORMAL || state == STATE_CC_RANGE)
        {
            if (c == '\\' && esc_state == 0)
            {
                esc_state = 1;
                continue;
            }
            uint8_t esc_c = 0;
            if (esc_state == 1)
            {
                esc_state = 0;
                if (c == 'n')
                    esc_c = '\n';
                else if (c == 'r')
                    esc_c = '\r';
                else if (c == 't')
                    esc_c = '\t';
                else if (c == 'v')
                    esc_c = '\v';
                else if (c == 'f')
                    esc_c = '\f';
                else if (c == 'x')
                {
                    if (pattern[i+1] == 0 || pattern[i+2] == 0)
                        return -1; // too-short hex pattern
                    uint8_t n0, n1;
                    if (remimu_nibble_hex_to_bin(pattern[i+1], &n0))
                        return -1; // invalid hex
                    if (remimu_nibble_hex_to_bin(pattern[i+2], &n1))
                        return -1; // invalid hex
                    esc_c = (n0 << 4) | n1;
                    i += 2;
                }
                else if (c == '{' || c == '}' ||
                         c == '[' || c == ']' || c == '-' ||
                         c == '(' || c == ')' ||
                         c == '|' || c == '^' || c == '$' ||
                         c == '*' || c == '+' || c == '?' || c == ':' ||
                         c == '.' || c == '/' || c == '\\')
                {
                    esc_c = c;
                }
                else if (c == 'd' || c == 's' || c == 'w' ||
                         c == 'D' || c == 'S' || c == 'W')
                {
                    if (state == STATE_CC_RANGE)
                    {
                        REMIMU_LOG_ERROR("tried to use a shorthand as part of a range");
                        return -1; // range shorthands can't be part of a range
                    }
                    uint8_t is_upper = c <= 'Z';

                    uint16_t m[16];
                    memset(m, 0, sizeof(m));

                    if (is_upper)
                        c += 0x20;
                    if (c == 'd' || c == 'w')
                        m[3] |= 0x03FF; // 0~7
                    if (c == 's')
                    {
                        m[0] |= 0x3E00; // \t-\r (includes \n, \v, and \f in the middle. 5 enabled bits.)
                        m[2] |= 1; // ' '
                    }
                    if (c == 'w')
                    {
                        m[4] |= 0xFFFE; // A-O
                        m[5] |= 0x87FF; // P-Z_
                        m[6] |= 0xFFFE; // a-o
                        m[7] |= 0x07FF; // p-z
                    }

                    for (int j = 0; j < 16; j++)
                        token.mask[j] |= is_upper ? ~m[j] : m[j];

                    char_class_mem = -1; // range shorthands can't be part of a range
                    continue;
                }
                else
                {
                    printf("unknown/unsupported escape sequence in character class (\\%c)\n", c);
                    return -1; // unknown/unsupported escape sequence
                }
            }
            if (state == STATE_CC_INIT)
            {
                uint8_t val = esc_c ? esc_c : (uint8_t)c;
                char_class_mem = val;
                _REGEX_SET_MASK(val);
                state = STATE_CC_NORMAL;
            }
            else if (state == STATE_CC_NORMAL)
            {
                if (c == ']' && esc_c == 0)
                {
                    char_class_mem = -1;
                    state = STATE_QUANT;
                    continue;
                }
                else if (c == '-' && esc_c == 0 && char_class_mem >= 0)
                {
                    state = STATE_CC_RANGE;
                    continue;
                }
                else
                {
                    uint8_t val = esc_c ? esc_c : (uint8_t)c;
                    char_class_mem = val;
                    _REGEX_SET_MASK(val);
                    state = STATE_CC_NORMAL;
                }
            }
            else if (state == STATE_CC_RANGE)
            {
                if (c == ']' && esc_c == 0)
                {
                    char_class_mem = -1;
                    _REGEX_SET_MASK('-');
                    state = STATE_QUANT;
                    continue;
                }
                else
                {
                    if (char_class_mem == -1)
                    {
                        REMIMU_LOG_ERROR("character class range is broken");
                        return -1; // probably tried to use a character class shorthand as part of a range
                    }
                    uint8_t rhs = esc_c ? esc_c : (uint8_t)c;
                    if (rhs < (uint8_t)char_class_mem)
                    {
                        REMIMU_LOG_ERROR("character class range is misordered");
                        return -1; // range is in wrong order
                    }
                    //printf("enabling char class from %d to %d...\n", char_class_mem, c);
                    for (uint8_t j = rhs; j > (uint8_t)char_class_mem; j--)
                        _REGEX_SET_MASK(j);
                    state = STATE_CC_NORMAL;
                    char_class_mem = -1;
                }
            }
        }
        else
            REMIMU_ASSERT(0);
    }
    if (paren_count > 0)
    {
        REMIMU_LOG_ERROR("(paren_count > 0)");
        return -1; // unbalanced parens
    }
    if (esc_state != 0)
    {
        REMIMU_LOG_ERROR("(esc_state != 0)");
        return -1; // open escape sequence
    }
    if (state >= STATE_CC_INIT)
    {
        REMIMU_LOG_ERROR("(state >= STATE_CC_INIT)");
        return -1; // open character class
    }

    _REGEX_PUSH_TOKEN();

    // add invisible non-capturing group specifier
    token.kind = REMIMU_KIND_CLOSE;
    token.count_lo = 1;
    token.count_hi = 2;
    _REGEX_PUSH_TOKEN();

    // add end token (tells matcher that it's done)
    token.kind = REMIMU_KIND_END;
    _REGEX_PUSH_TOKEN();

    tokens[0].pair_offset = k - 2;
    tokens[k-2].pair_offset = -(k - 2);

    *token_count = k;

    // copy quantifiers from )s to (s (so (s know whether they're optional)
    // also take the opportunity to smuggle "quantified group index" into the mask field for the )
    uint64_t n = 0;
    for (int16_t k2 = 0; k2 < k; k2++)
    {
        if (tokens[k2].kind == REMIMU_KIND_CLOSE)
        {
            tokens[k2].mask[0] = n++;

            int16_t k3 = k2 + tokens[k2].pair_offset;
            tokens[k3].count_lo = tokens[k2].count_lo;
            tokens[k3].count_hi = tokens[k2].count_hi;
            tokens[k3].mask[0] = n++;
            tokens[k3].mode = tokens[k2].mode;

            //if (n > 65535)
            if (n > 1024)
                return -1; // too many quantified groups
        }
        else if (tokens[k2].kind == REMIMU_KIND_OR || tokens[k2].kind == REMIMU_KIND_OPEN || tokens[k2].kind == REMIMU_KIND_NCOPEN)
        {
            // find next | or ) and how far away it is. store in token
            int balance = 0;
            ptrdiff_t found = -1;
            for (ptrdiff_t l = k2 + 1; l < k; l++)
            {
                if (tokens[l].kind == REMIMU_KIND_OR && balance == 0)
                {
                    found = l;
                    break;
                }
                else if (tokens[l].kind == REMIMU_KIND_CLOSE)
                {
                    if (balance == 0)
                    {
                        found = l;
                        break;
                    }
                    else
                        balance -= 1;
                }
                else if (tokens[l].kind == REMIMU_KIND_NCOPEN || tokens[l].kind == REMIMU_KIND_OPEN)
                    balance += 1;
            }
            if (found == -1)
            {
                REMIMU_LOG_ERROR("unbalanced parens...");
                return -1; // unbalanced parens
            }
            ptrdiff_t diff = found - k2;
            if (diff > 32767)
            {
                REMIMU_LOG_ERROR("too long...");
                return -1; // too long
            }

            if (tokens[k2].kind == REMIMU_KIND_OR)
                tokens[k2].pair_offset = diff;
            else
                tokens[k2].mask[15] = diff;
        }
    }

    #undef _REGEX_PUSH_TOKEN
    #undef _REGEX_SET_MASK
    #undef _REGEX_CLEAR_TOKEN

    return 0;
}

typedef struct _RegexMatcherState {
    uint32_t k;
    uint32_t group_state; // quantified group temp state (e.g. number of repetitions)
    uint32_t prev; // for )s, stack index of corresponding previous quantified state
#ifdef REGEX_STACK_SMOL
    uint32_t i;
    uint32_t range_min;
    uint32_t range_max;
#else
    uint64_t i;
    uint64_t range_min;
    uint64_t range_max;
#endif
} RegexMatcherState;

// NOTE: undef'd later
#define _REGEX_CHECK_MASK(K, byte) (!!(tokens[K].mask[((uint8_t)byte)>>4] & (1 << ((uint8_t)byte & 0xF))))

// Returns match length if text starts with a regex match.
// Returns -1 if the text doesn't start with a regex match.
// Returns -2 if the matcher ran out of memory or the regex is too complex.
// Returns -3 if the regex is somehow invalid.
// The first cap_slots capture positions and spans (lengths) will be written to cap_pos and cap_span. If zero, will not be written to.
// SAFETY: The text variable must be null-terminated, and start_i must be the index of a character within the string or its null terminator.
// SAFETY: Tokens array must be terminated by a REMIMU_KIND_END token (done by default by regex_parse).
// SAFETY: Partial capture data may be written even if the match fails.
REMIMU_FUNC_VISIBILITY int64_t regex_match(const RegexToken * tokens, const char * text, size_t start_i, uint16_t cap_slots, int64_t * cap_pos, int64_t * cap_span)
{
    (void)text;

#ifdef REGEX_VERBOSE
    const uint8_t verbose = 1;
#else
    const uint8_t verbose = 0;
#endif

#define IF_VERBOSE(X) { if (verbose) { X } }

#ifdef REGEX_STACK_SMOL
    const uint16_t stack_size_max = 256;
#else
    const uint16_t stack_size_max = 1024;
#endif
    const uint16_t aux_stats_size = 1024;
    if (cap_slots > aux_stats_size)
        cap_slots = aux_stats_size;

    // quantified group state
    uint8_t q_group_accepts_zero[aux_stats_size];
    uint32_t q_group_state[aux_stats_size]; // number of repetitions
    uint32_t q_group_stack[aux_stats_size]; // location of most recent corresponding ) on stack. 0 means nowhere

    uint16_t q_group_cap_index[aux_stats_size];
    memset(q_group_cap_index, 0xFF, sizeof(q_group_cap_index));

    uint64_t tokens_len = 0;
    uint32_t k = 0;
    uint16_t caps = 0;

    while (tokens[k].kind != REMIMU_KIND_END)
    {
        if (tokens[k].kind == REMIMU_KIND_OPEN && caps < cap_slots)
        {
            q_group_cap_index[tokens[k].mask[0]] = caps;
            q_group_cap_index[tokens[k + tokens[k].pair_offset].mask[0]] = caps;
            cap_pos[caps] = -1;
            cap_span[caps] = -1;
            caps += 1;
        }
        k += 1;
        if (tokens[k].kind == REMIMU_KIND_CLOSE || tokens[k].kind == REMIMU_KIND_OPEN || tokens[k].kind == REMIMU_KIND_NCOPEN)
        {
            if (tokens[k].mask[0] >= aux_stats_size)
            {
                REMIMU_LOG_ERROR("too many qualified groups. returning");
                return -2; // OOM: too many quantified groups
            }

            q_group_state[tokens[k].mask[0]] = 0;
            q_group_stack[tokens[k].mask[0]] = 0;
            q_group_accepts_zero[tokens[k].mask[0]] = 0;
        }
    }

    tokens_len = k;

    RegexMatcherState rewind_stack[stack_size_max];
    uint16_t stack_n = 0;

    uint64_t i = start_i;

    uint64_t range_min = 0;
    uint64_t range_max = 0;
    uint8_t just_rewinded = 0;

    #define _P_TEXT_HIGHLIGHTED() do { \
        IF_VERBOSE(printf("\033[91m"); \
        for (uint64_t q = 0; q < i; q++) printf("%c", text[q]); \
        printf("\033[0m"); \
        for (uint64_t q = i; text[q] != 0; q++) printf("%c", text[q]); \
        printf("\n");) \
    } while (0)

    #define _REWIND_DO_SAVE_RAW(K, ISDUMMY) do { \
        if (stack_n >= stack_size_max) \
        { \
            REMIMU_LOG_ERROR("out of backtracking room. returning"); \
            return -2; \
        } \
        RegexMatcherState s; \
        memset(&s, 0, sizeof(RegexMatcherState)); \
        s.i = i; \
        s.k = (K); \
        s.range_min = range_min; \
        s.range_max = range_max; \
        s.prev = 0; \
        if (ISDUMMY) s.prev = 0xFAC7; \
        else if (tokens[s.k].kind == REMIMU_KIND_CLOSE) \
        { \
            s.group_state = q_group_state[tokens[s.k].mask[0]]; \
            s.prev = q_group_stack[tokens[s.k].mask[0]]; \
            q_group_stack[tokens[s.k].mask[0]] = stack_n; \
        } \
        rewind_stack[stack_n++] = s; \
        _P_TEXT_HIGHLIGHTED(); \
        IF_VERBOSE(printf("-- saving rewind state k %u i %zd rmin %zu rmax %zd (line %d) (depth %d prev %d)\n", s.k, i, range_min, range_max, __LINE__, stack_n, s.prev);) \
    } while (0)
    #define _REWIND_DO_SAVE_DUMMY(K) _REWIND_DO_SAVE_RAW(K, 1)
    #define _REWIND_DO_SAVE(K) _REWIND_DO_SAVE_RAW(K, 0)

    #define _REWIND_OR_ABORT() do { \
        if (stack_n == 0) \
            return -1; \
        stack_n -= 1; \
        while (stack_n > 0 && rewind_stack[stack_n].prev == 0xFAC7) stack_n -= 1; \
        just_rewinded = 1; \
        range_min = rewind_stack[stack_n].range_min; \
        range_max = rewind_stack[stack_n].range_max; \
        REMIMU_ASSERT(rewind_stack[stack_n].i <= i); \
        i = rewind_stack[stack_n].i; \
        k = rewind_stack[stack_n].k; \
        if (tokens[k].kind == REMIMU_KIND_CLOSE) \
        { \
            q_group_state[tokens[k].mask[0]] = rewind_stack[stack_n].group_state; \
            q_group_stack[tokens[k].mask[0]] = rewind_stack[stack_n].prev; \
        } \
        _P_TEXT_HIGHLIGHTED(); \
        IF_VERBOSE(printf("-- rewound to k %u i %zd rmin %zu rmax %zd (kind %d prev %d)\n", k, i, range_min, range_max, tokens[k].kind, rewind_stack[stack_n].prev);) \
        k -= 1; \
    } while (0)
    // the -= 1 is because of the k++ in the for loop

    // used in boundary anchor checker
    uint64_t w_mask[16];
    memset(w_mask, 0, sizeof(w_mask));
    w_mask[3] = 0x03FF;
    w_mask[4] = 0xFFFE;
    w_mask[5] = 0x87FF;
    w_mask[6] = 0xFFFE;
    w_mask[7] = 0x07FF;
    #define _REGEX_CHECK_IS_W(byte) (!!(w_mask[((uint8_t)byte)>>4] & (1 << ((uint8_t)byte & 0xF))))

    int limit = REMIMU_ITERATION_LIMIT;
    for (k = 0; k < tokens_len; k++)
    {
        if (REMIMU_ITERATION_LIMIT)
        {
            if (limit-- == 0)
            {
                REMIMU_LOG_ERROR("iteration limit exceeded. returning");
                return -2;
            }
        }
        IF_VERBOSE(printf("k: %u\ti: %zu\tl: %d\tstack_n: %d\n", k, i, limit, stack_n);)
        _P_TEXT_HIGHLIGHTED();
        if (tokens[k].kind == REMIMU_KIND_CARET)
        {
            if (i != 0)
                _REWIND_OR_ABORT();
            continue;
        }
        else if (tokens[k].kind == REMIMU_KIND_DOLLAR)
        {
            if (text[i] != 0)
                _REWIND_OR_ABORT();
            continue;
        }
        else if (tokens[k].kind == REMIMU_KIND_BOUND)
        {
            if (i == 0 && !_REGEX_CHECK_IS_W(text[i]))
                _REWIND_OR_ABORT();
            else if (i != 0 && text[i] == 0 && !_REGEX_CHECK_IS_W(text[i-1]))
                _REWIND_OR_ABORT();
            else if (i != 0 && text[i] != 0 && _REGEX_CHECK_IS_W(text[i-1]) == _REGEX_CHECK_IS_W(text[i]))
                _REWIND_OR_ABORT();
        }
        else if (tokens[k].kind == REMIMU_KIND_NBOUND)
        {
            if (i == 0 && _REGEX_CHECK_IS_W(text[i]))
                _REWIND_OR_ABORT();
            else if (i != 0 && text[i] == 0 && _REGEX_CHECK_IS_W(text[i-1]))
                _REWIND_OR_ABORT();
            else if (i != 0 && text[i] != 0 && _REGEX_CHECK_IS_W(text[i-1]) != _REGEX_CHECK_IS_W(text[i]))
                _REWIND_OR_ABORT();
        }
        else
        {
            // deliberately unmatchable token (e.g. a{0}, a{0,0})
            if (tokens[k].count_hi == 1)
            {
                if (tokens[k].kind == REMIMU_KIND_OPEN || tokens[k].kind == REMIMU_KIND_NCOPEN)
                    k += tokens[k].pair_offset;
                else
                    k += 1;
                continue;
            }

            if (tokens[k].kind == REMIMU_KIND_OPEN || tokens[k].kind == REMIMU_KIND_NCOPEN)
            {
                if (!just_rewinded)
                {
                    IF_VERBOSE(printf("hit OPEN. i is %zd, depth is %d\n", i, stack_n);)
                    // need this to be able to detect and reject zero-size matches
                    //q_group_state[tokens[k].mask[0]] = i;

                    // if we're lazy and the min length is 0, we need to try the non-group case first
                    if ((tokens[k].mode & REMIMU_MODE_LAZY) && (tokens[k].count_lo == 0 || q_group_accepts_zero[tokens[k + tokens[k].pair_offset].mask[0]]))
                    {
                        IF_VERBOSE(puts("trying non-group case first.....");)
                        range_min = 0;
                        range_max = 0;
                        _REWIND_DO_SAVE(k);
                        k += tokens[k].pair_offset; // automatic += 1 will put us past the matching )
                    }
                    else
                    {
                        range_min = 1;
                        range_max = 0;
                        _REWIND_DO_SAVE(k);
                    }
                }
                else
                {
                    IF_VERBOSE(printf("rewinded into OPEN. i is %zd, depth is %d\n", i, stack_n);)
                    just_rewinded = 0;

                    uint64_t orig_k = k;

                    IF_VERBOSE(printf("--- trying to try another alternation, start k is %d, rmin is %zu\n", k, range_min);)

                    if (range_min != 0)
                    {
                        IF_VERBOSE(puts("rangemin is not zero. checking...");)
                        k += range_min;
                        IF_VERBOSE(printf("start kind: %d\n", tokens[k].kind);)
                        IF_VERBOSE(printf("before start kind: %d\n", tokens[k-1].kind);)
                        if (tokens[k-1].kind == REMIMU_KIND_OR)
                            k += tokens[k-1].pair_offset - 1;
                        else if (tokens[k-1].kind == REMIMU_KIND_OPEN || tokens[k-1].kind == REMIMU_KIND_NCOPEN)
                            k += tokens[k-1].mask[15] - 1;

                        IF_VERBOSE(printf("kamakama %d %d\n", k, tokens[k].kind);)

                        if (tokens[k].kind == REMIMU_KIND_END) // unbalanced parens
                            return -3;

                        IF_VERBOSE(printf("---?!?!   %d, %d\n", k, q_group_state[tokens[k].mask[0]]);)
                        if (tokens[k].kind == REMIMU_KIND_CLOSE)
                        {
                            IF_VERBOSE(puts("!!~!~!~~~~!!~~!~   hit CLOSE. rewinding");)
                            // do nothing and continue on if we don't need this group
                            if (tokens[k].count_lo == 0 || q_group_accepts_zero[tokens[k].mask[0]])
                            {
                                IF_VERBOSE(puts("continuing because we don't need this group");)
                                q_group_state[tokens[k].mask[0]] = 0;

                                if (!(tokens[k].mode & REMIMU_MODE_LAZY))
                                    q_group_stack[tokens[k].mask[0]] = 0;

                                continue;
                            }
                            // otherwise go to the last point before the group
                            else
                            {
                                IF_VERBOSE(puts("going to last point before this group");)
                                _REWIND_OR_ABORT();
                                continue;
                            }
                        }

                        REMIMU_ASSERT(tokens[k].kind == REMIMU_KIND_OR);
                    }

                    IF_VERBOSE(printf("--- FOUND ALTERNATION for paren at k %zd at k %d\n", orig_k, k);)

                    ptrdiff_t k_diff = k - orig_k;
                    range_min = k_diff + 1;

                    IF_VERBOSE(puts("(saving in paren after rewinding and looking for next regex token to check)");)
                    IF_VERBOSE(printf("%zd\n", range_min);)
                    _REWIND_DO_SAVE(k - k_diff);
                }
            }
            else if (tokens[k].kind == REMIMU_KIND_CLOSE)
            {
                // unquantified
                if (tokens[k].count_lo == 1 && tokens[k].count_hi == 2)
                {
                    // for captures
                    uint16_t cap_index = q_group_cap_index[tokens[k].mask[0]];
                    if (cap_index != 0xFFFF)
                        _REWIND_DO_SAVE_DUMMY(k);
                }
                // quantified
                else
                {
                    IF_VERBOSE(puts("closer test.....");)
                    if (!just_rewinded)
                    {
                        uint32_t prev = q_group_stack[tokens[k].mask[0]];

                        IF_VERBOSE(printf("qrqrqrqrqrqrqrq-------      k %d, gs %d, gaz %d, i %zd, tklo %d, rmin %zd, tkhi %d, rmax %zd, prev %d, sn %d\n", k, q_group_state[tokens[k].mask[0]], q_group_accepts_zero[tokens[k].mask[0]], i, tokens[k].count_lo, range_min, tokens[k].count_hi, range_max, prev, stack_n);)

                        range_max = tokens[k].count_hi;
                        range_max -= 1;
                        range_min = q_group_accepts_zero[tokens[k].mask[0]] ? 0 : tokens[k].count_lo;
                        //REMIMU_ASSERT(q_group_state[tokens[k + tokens[k].pair_offset].mask[0]] <= i);
                        //if (prev) REMIMU_ASSERT(rewind_stack[prev].i <= i);
                        IF_VERBOSE(printf("qzqzqzqzqzqzqzq-------      rmin %zd, rmax %zd\n", range_min, range_max);)

                        // minimum requirement not yet met
                        if (q_group_state[tokens[k].mask[0]] + 1 < range_min)
                        {
                            IF_VERBOSE(puts("continuing minimum matches for a quantified group");)
                            q_group_state[tokens[k].mask[0]] += 1;
                            _REWIND_DO_SAVE(k);

                            k += tokens[k].pair_offset; // back to start of group
                            k -= 1; // ensure we actually hit the group node next and not the node after it
                            continue;
                        }
                        // maximum allowance exceeded
                        else if (tokens[k].count_hi != 0 && q_group_state[tokens[k].mask[0]] + 1 > range_max)
                        {
                            IF_VERBOSE(printf("hit maximum allowed instances of a quantified group %d %zd\n", q_group_state[tokens[k].mask[0]], range_max);)
                            range_max -= 1;
                            _REWIND_OR_ABORT();
                            continue;
                        }

                        // fallback case to detect zero-length matches when we backtracked into the inside of this group
                        // after an attempted parse of a second copy of itself
                        uint8_t force_zero = 0;
                        if (prev != 0 && rewind_stack[prev].i > i)
                        {
                            // find matching open paren
                            size_t n = stack_n - 1;
                            while (n > 0 && rewind_stack[n].k != k + tokens[k].pair_offset)
                                n -= 1;
                            REMIMU_ASSERT(n > 0);
                            if (rewind_stack[n].i == i)
                                force_zero = 1;
                        }

                        // reject zero-length matches
                        if ((force_zero || (prev != 0 && rewind_stack[prev].i == i))) //  && q_group_state[tokens[k].mask[0]] > 0
                        {
                            IF_VERBOSE(printf("rejecting zero-length match..... %d %zd %zd\n", force_zero, rewind_stack[prev].i, i);)
                            IF_VERBOSE(printf("%d (k: %d)\n", q_group_state[tokens[k].mask[0]], k);)

                            q_group_accepts_zero[tokens[k].mask[0]] = 1;
                            _REWIND_OR_ABORT();
                            //range_max = q_group_state[tokens[k].mask[0]];
                            //range_min = 0;
                        }
                        else if (tokens[k].mode & REMIMU_MODE_LAZY) // lazy
                        {
                            IF_VERBOSE(printf("nidnfasidfnidfndifn-------      %d, %d, %zd\n", q_group_state[tokens[k].mask[0]], tokens[k].count_lo, range_min);)
                            if (prev)
                                IF_VERBOSE(printf("lazy doesn't think it's zero-length. prev i %zd vs i %zd (depth %d)\n", rewind_stack[prev].i, i, stack_n);)
                            // continue on to past the group; group retry is in rewind state
                            q_group_state[tokens[k].mask[0]] += 1;
                            _REWIND_DO_SAVE(k);
                            q_group_state[tokens[k].mask[0]] = 0;
                        }
                        else // greedy
                        {
                            IF_VERBOSE(puts("wahiwahi");)
                            // clear unwanted memory if possessive
                            if ((tokens[k].mode & REMIMU_MODE_POSSESSIVE))
                            {
                                uint32_t k2 = k;

                                // special case for first, only rewind to (, not to )
                                if (q_group_state[tokens[k].mask[0]] == 0)
                                    k2 = k + tokens[k].pair_offset;

                                if (stack_n == 0)
                                    return -1;
                                stack_n -= 1;

                                while (stack_n > 0 && rewind_stack[stack_n].k != k2)
                                    stack_n -= 1;

                                if (stack_n == 0)
                                    return -1;
                            }
                            // continue to next match if sane
                            if ((uint32_t)q_group_state[tokens[k + tokens[k].pair_offset].mask[0]] < (uint32_t)i)
                            {
                                IF_VERBOSE(puts("REWINDING FROM GREEDY NON-REWIND CLOSER");)
                                q_group_state[tokens[k].mask[0]] += 1;
                                _REWIND_DO_SAVE(k);
                                k += tokens[k].pair_offset; // back to start of group
                                k -= 1; // ensure we actually hit the group node next and not the node after it
                            }
                            else
                                IF_VERBOSE(puts("CONTINUING FROM GREEDY NON-REWIND CLOSER");)
                        }
                    }
                    else
                    {
                        IF_VERBOSE(puts("IN CLOSER REWIND!!!");)
                        just_rewinded = 0;

                        if (tokens[k].mode & REMIMU_MODE_LAZY)
                        {
                            // lazy rewind: need to try matching the group again
                            _REWIND_DO_SAVE_DUMMY(k);
                            q_group_stack[tokens[k].mask[0]] = stack_n;
                            k += tokens[k].pair_offset; // back to start of group
                            k -= 1; // ensure we actually hit the group node next and not the node after it
                        }
                        else
                        {
                            // greedy. if we're going to go outside the acceptable range, rewind
                            IF_VERBOSE(printf("kufukufu %d %zd\n", tokens[k].count_lo, range_min);)
                            //uint64_t old_i = i;
                            if (q_group_state[tokens[k].mask[0]] < range_min && !q_group_accepts_zero[tokens[k].mask[0]])
                            {
                                IF_VERBOSE(printf("rewinding from greedy group because we're going to go out of range (%d vs %zd)\n", q_group_state[tokens[k].mask[0]], range_min);)
                                //i = old_i;
                                _REWIND_OR_ABORT();
                            }
                            // otherwise continue on to past the group
                            else
                            {
                                IF_VERBOSE(puts("continuing past greedy group");)
                                q_group_state[tokens[k].mask[0]] = 0;

                                // for captures
                                uint16_t cap_index = q_group_cap_index[tokens[k].mask[0]];
                                if (cap_index != 0xFFFF)
                                    _REWIND_DO_SAVE_DUMMY(k);
                            }
                        }
                    }
                }
            }
            else if (tokens[k].kind == REMIMU_KIND_OR)
            {
                IF_VERBOSE(printf("hit OR at %d. adding %d\n", k, tokens[k].pair_offset);)
                k += tokens[k].pair_offset;
                k -= 1;
            }
            else if (tokens[k].kind == REMIMU_KIND_NORMAL)
            {
                if (!just_rewinded)
                {
                    uint64_t n = 0;
                    // do whatever the obligatory minimum amount of matching is
                    uint64_t old_i = i;
                    while (n < tokens[k].count_lo && text[i] != 0 && _REGEX_CHECK_MASK(k, text[i]))
                    {
                        i += 1;
                        n += 1;
                    }
                    if (n < tokens[k].count_lo)
                    {
                        IF_VERBOSE(printf("non-match A. rewinding (token %d)\n", k);)
                        i = old_i;
                        _REWIND_OR_ABORT();
                        continue;
                    }

                    if (tokens[k].mode & REMIMU_MODE_LAZY)
                    {
                        range_min = n;
                        range_max = tokens[k].count_hi - 1;
                        _REWIND_DO_SAVE(k);
                    }
                    else
                    {
                        uint64_t ilimit = tokens[k].count_hi;
                        if (ilimit == 0)
                            ilimit = ~ilimit;
                        range_min = n;
                        while (text[i] != 0 && _REGEX_CHECK_MASK(k, text[i]) && n + 1 < ilimit)
                        {
                            IF_VERBOSE(printf("match!! (%c)\n", text[i]);)
                            i += 1;
                            n += 1;
                        }
                        range_max = n;
                        IF_VERBOSE(printf("set rmin to %zd and rmax to %zd on entry into normal greedy token with k %d\n", range_min, range_max, k);)
                        if (!(tokens[k].mode & REMIMU_MODE_POSSESSIVE))
                            _REWIND_DO_SAVE(k);
                    }
                }
                else
                {
                    just_rewinded = 0;

                    if (tokens[k].mode & REMIMU_MODE_LAZY)
                    {
                        uint64_t ilimit = range_max;
                        if (ilimit == 0)
                            ilimit = ~ilimit;

                        if (_REGEX_CHECK_MASK(k, text[i]) && text[i] != 0 && range_min < ilimit)
                        {
                            IF_VERBOSE(printf("match2!! (%c) (k: %d)\n", text[i], k);)
                            i += 1;
                            range_min += 1;
                            _REWIND_DO_SAVE(k);
                        }
                        else
                        {
                            IF_VERBOSE(printf("core rewind lazy (k: %d)\n", k);)
                            _REWIND_OR_ABORT();
                        }
                    }
                    else
                    {
                        //IF_VERBOSE(printf("comparing rmin %zd and rmax %zd token with k %d\n", range_min, range_max, k);)
                        if (range_max > range_min)
                        {
                            IF_VERBOSE(printf("greedy normal going back (k: %d)\n", k);)
                            i -= 1;
                            range_max -= 1;
                            _REWIND_DO_SAVE(k);
                        }
                        else
                        {
                            IF_VERBOSE(printf("core rewind greedy (k: %d)\n", k);)
                            _REWIND_OR_ABORT();
                        }
                    }
                }
            }
            else
            {
                fprintf(stderr, "unimplemented token kind %d\n", tokens[k].kind);
                REMIMU_ASSERT(0);
            }
        }
        //printf("k... %d\n", k);
    }

    if (caps != 0)
    {
        //printf("stack_n: %d\n", stack_n);
        fflush(stdout);
        for (size_t n = 0; n < stack_n; n++)
        {
            RegexMatcherState s = rewind_stack[n];
            int kind = tokens[s.k].kind;
            if (kind == REMIMU_KIND_OPEN || kind == REMIMU_KIND_CLOSE)
            {
                uint16_t cap_index = q_group_cap_index[tokens[s.k].mask[0]];
                if (cap_index == 0xFFFF)
                    continue;
                if (tokens[s.k].kind == REMIMU_KIND_OPEN)
                    cap_pos[cap_index] = s.i;
                else if (cap_pos[cap_index] >= 0)
                    cap_span[cap_index] = s.i - cap_pos[cap_index];
            }
        }
        // re-deinitialize capture positions that have no associated capture span
        for (size_t n = 0; n < caps; n++)
        {
            if (cap_span[n] == -1)
                cap_pos[n] = -1;
        }
    }

    #undef _REWIND_DO_SAVE
    #undef _REWIND_OR_ABORT
    #undef _REGEX_CHECK_IS_W
    #undef _P_TEXT_HIGHLIGHTED
    #undef IF_VERBOSE

    return i;
}

REMIMU_FUNC_VISIBILITY void print_regex_tokens(RegexToken * tokens)
{
    const char * kind_to_str[] = {
        "NORMAL",
        "OPEN",
        "NCOPEN",
        "CLOSE",
        "OR",
        "CARET",
        "DOLLAR",
        "BOUND",
        "NBOUND",
        "END",
    };
    const char * mode_to_str[] = {
        "GREEDY",
        "POSSESS",
        "LAZY",
    };
    for (int k = 0;; k++)
    {
        printf("%s\t%s\t", kind_to_str[tokens[k].kind], mode_to_str[tokens[k].mode]);

        int c_old = -1;
        for (int c = 0; c < (tokens[k].kind ? 0 : 256); c++)
        {
            #define _PRINT_C_SMART(c) { \
                if (c >= 0x20 && c <= 0x7E) \
                    printf("%c", c); \
                else \
                    printf("\\x%02x", c); \
            }

            if (_REGEX_CHECK_MASK(k, c))
            {
                if (c_old == -1)
                    c_old = c;
            }
            else if (c_old != -1)
            {
                if (c - 1 == c_old)
                {
                    _PRINT_C_SMART(c_old)
                    c_old = -1;
                }
                else if (c - 2 == c_old)
                {
                    _PRINT_C_SMART(c_old)
                    _PRINT_C_SMART(c_old + 1)
                    c_old = -1;
                }
                else
                {
                    _PRINT_C_SMART(c_old)
                    printf("-");
                    _PRINT_C_SMART(c - 1)
                    c_old = -1;
                }
            }
        }

        /*
        printf("\t");
        for (int i = 0; i < 16; i++)
            printf("%04x", tokens[k].mask[i]);
        */

        printf("\t{%d,%d}\t(%d)\n", tokens[k].count_lo, tokens[k].count_hi - 1, tokens[k].pair_offset);

        if (tokens[k].kind == REMIMU_KIND_END)
            break;
    }
}

#undef _REGEX_CHECK_MASK

#endif //INCLUDE_REMIMU
// ------------------------------------------------------------------------------------------

class Base64Encode : public Benchmark {
private:
  std::string str;
  std::string str2;
  uint32_t result_val;

  static size_t encode_size(size_t size) {
    return (size_t)(size * 4 / 3.0) + 6;
  }

  static size_t b64_encode(char *dst, const char *src, size_t src_size);
  std::string base64_encode_simple(const std::string &input);

public:
  Base64Encode();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};

class Base64Decode : public Benchmark {
private:
  std::string str2;
  std::string str3;
  uint32_t result_val;

  static size_t decode_size(size_t size) {
    return (size_t)(size * 3 / 4.0) + 6;
  }

  static size_t encode_size(size_t size) {
    return (size_t)(size * 4 / 3.0) + 6;
  }

  static size_t b64_decode(char *dst, const char *src, size_t src_size);
  std::string base64_decode_simple(const std::string &input);

public:
  Base64Decode();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};

class BinarytreesObj : public Benchmark {
private:
  struct TreeNode {
    std::unique_ptr<TreeNode> left;
    std::unique_ptr<TreeNode> right;
    int32_t item;

    TreeNode(int32_t item, int depth = 0);

    uint32_t sum() const {
      uint32_t total = static_cast<uint32_t>(item) + 1;
      if (left)
        total += left->sum();
      if (right)
        total += right->sum();
      return total;
    }
  };

  int64_t n;
  uint32_t result_val;

public:
  BinarytreesObj();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};

class BinarytreesArena : public Benchmark {
private:
  struct TreeNode {
    int32_t item;
    int32_t left;
    int32_t right;

    TreeNode(int32_t item) : item(item), left(-1), right(-1) {}
  };

  std::vector<TreeNode> arena;
  int64_t n;
  uint32_t result_val;

  int32_t build_tree(int32_t item, int32_t depth);
  uint32_t sum(int32_t idx) const;

public:
  BinarytreesArena();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};

class BrainfuckArray : public Benchmark {
private:
  class Tape {
  private:
    std::vector<uint8_t> tape;
    size_t pos;

  public:
    Tape() : tape(30000, 0), pos(0) {}

    uint8_t get() const { return tape[pos]; }
    void inc() { tape[pos]++; }
    void dec() { tape[pos]--; }
    void advance() {
      pos++;
      if (pos >= tape.size()) {
        tape.push_back(0);
      }
    }
    void devance() {
      if (pos > 0)
        pos--;
    }
  };

  class Program {
  private:
    std::vector<uint8_t> commands;
    std::vector<size_t> jumps;

  public:
    Program(const std::string &text);
    int64_t run();
  };

  std::string program_text;
  std::string warmup_text;
  uint32_t result_val;

  int64_t _run(const std::string &text);

public:
  BrainfuckArray();

  std::string name() const override;
  void warmup() override;
  void run(int) override;
  uint32_t checksum() override;
};

class BrainfuckRecursion : public Benchmark {
private:
  struct OpInc {};
  struct OpDec {};
  struct OpAdvance {};
  struct OpDevance {};
  struct OpPrint {};
  struct OpLoop;

  using Op = std::variant<OpInc, OpDec, OpAdvance, OpDevance, OpPrint, OpLoop>;

  struct OpLoop {
    std::vector<Op> ops;
  };

  class Tape {
  private:
    std::vector<uint8_t> tape;
    size_t pos = 0;

  public:
    Tape() : tape(30000, 0) {}

    uint8_t get() const { return tape[pos]; }
    void inc() { tape[pos]++; }
    void dec() { tape[pos]--; }
    void advance() {
      pos++;
      if (pos >= tape.size()) {
        tape.push_back(0);
      }
    }
    void devance() {
      if (pos > 0) {
        pos--;
      }
    }
  };

  class Program {
  private:
    std::vector<Op> ops;

    std::vector<Op> parse(std::string::const_iterator &it,
                          const std::string::const_iterator &end);

    struct Visitor {
      Tape &tape;
      int64_t &result;

      void operator()(const OpInc &) const { tape.inc(); }
      void operator()(const OpDec &) const { tape.dec(); }
      void operator()(const OpAdvance &) const { tape.advance(); }
      void operator()(const OpDevance &) const { tape.devance(); }
      void operator()(const OpPrint &) const {
        result = (result << 2) + tape.get();
      }
      void operator()(const OpLoop &loop) const {
        while (tape.get() != 0) {
          for (const auto &op : loop.ops) {
            std::visit(*this, op);
          }
        }
      }
    };

  public:
    explicit Program(const std::string &code);
    int64_t run();
  };

  std::string text;
  uint32_t result_val;

public:
  BrainfuckRecursion();

  std::string name() const override;
  void warmup() override;
  void run(int) override;
  uint32_t checksum() override;
};

class BufferHashBenchmark : public Benchmark {
protected:
  std::vector<uint8_t> data;
  int64_t size_val;
  uint32_t result_val;

  BufferHashBenchmark();

public:
  virtual uint32_t test() = 0;

  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class BufferHashSHA256 : public BufferHashBenchmark {
private:
  struct SimpleSHA256 {
    static std::vector<uint8_t> digest(const std::vector<uint8_t> &data);
  };

public:
  BufferHashSHA256() = default;

  std::string name() const override;
  uint32_t test() override;
};

class BufferHashCRC32 : public BufferHashBenchmark {
private:
  uint32_t crc32(const std::vector<uint8_t> &data);

public:
  BufferHashCRC32() = default;

  std::string name() const override;
  uint32_t test() override;
};

class CacheSimulation : public Benchmark {
private:
  template <typename K, typename V> class LRUCache {
  private:
    struct Node {
      K key;
      V value;
      Node *prev;
      Node *next;

      Node(const K &k, const V &v);
    };

    int capacity_;
    std::unordered_map<K, Node *> cache_;
    Node *head_;
    Node *tail_;
    int size_;

    void move_to_front(Node *node);
    void add_to_front(Node *node);
    void remove_oldest();

  public:
    LRUCache(int capacity);
    ~LRUCache();

    std::optional<V> get(const K &key);
    void put(const K &key, const V &value);
    int size() const { return size_; }
  };

  uint32_t result_val;
  int values_size;
  LRUCache<std::string, std::string> cache;
  int hits;
  int misses;

public:
  CacheSimulation();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};

class CalculatorAst : public Benchmark {
public:
  struct Number {
    int64_t value;
    Number(int64_t v) : value(v) {}
  };

  struct Variable {
    std::string name;
    Variable(const std::string &n) : name(n) {}
  };

  struct BinaryOp;
  struct Assignment;

  struct Node {
    std::variant<Number, Variable, std::unique_ptr<BinaryOp>,
                 std::unique_ptr<Assignment>>
        data;

    Node(Number n) : data(std::move(n)) {}
    Node(Variable v) : data(std::move(v)) {}
    Node(std::unique_ptr<BinaryOp> b) : data(std::move(b)) {}
    Node(std::unique_ptr<Assignment> a) : data(std::move(a)) {}

    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;
    Node(Node &&) = default;
    Node &operator=(Node &&) = default;
  };

  struct BinaryOp {
    char op;
    Node left;
    Node right;

    BinaryOp(char o, Node l, Node r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
  };

  struct Assignment {
    std::string var;
    Node expr;

    Assignment(const std::string &v, Node e) : var(v), expr(std::move(e)) {}
  };

private:
  class Parser {
  private:
    static constexpr char CHAR_EOF = '\0';
    static constexpr char CHAR_PLUS = '+';
    static constexpr char CHAR_MINUS = '-';
    static constexpr char CHAR_STAR = '*';
    static constexpr char CHAR_SLASH = '/';
    static constexpr char CHAR_PERCENT = '%';
    static constexpr char CHAR_LPAREN = '(';
    static constexpr char CHAR_RPAREN = ')';
    static constexpr char CHAR_EQUALS = '=';
    static constexpr char CHAR_ZERO = '0';
    static constexpr char CHAR_NINE = '9';
    static constexpr char CHAR_A_LOWER = 'a';
    static constexpr char CHAR_Z_LOWER = 'z';
    static constexpr char CHAR_A_UPPER = 'A';
    static constexpr char CHAR_Z_UPPER = 'Z';
    static constexpr char CHAR_SPACE = ' ';
    static constexpr char CHAR_TAB = '\t';
    static constexpr char CHAR_NEWLINE = '\n';
    static constexpr char CHAR_CR = '\r';

    const std::string &input;
    size_t pos;
    size_t len;
    char current_char;
    std::vector<Node> expressions;

    void advance();
    void skip_whitespace();
    Node parse_number();
    Node parse_variable();
    Node parse_factor();
    Node parse_term();
    Node parse_expression();

    bool is_digit(char c) const { return c >= CHAR_ZERO && c <= CHAR_NINE; }

    bool is_letter(char c) const {
      return (c >= CHAR_A_LOWER && c <= CHAR_Z_LOWER) ||
             (c >= CHAR_A_UPPER && c <= CHAR_Z_UPPER);
    }

    bool is_whitespace(char c) const {
      return c == CHAR_SPACE || c == CHAR_TAB || c == CHAR_NEWLINE ||
             c == CHAR_CR;
    }

  public:
    Parser(const std::string &input_str);
    std::vector<Node> parse();
  };

  uint32_t result_val;
  std::string text;

  std::string generate_random_program(int64_t n = 1000);

public:
  int64_t n;
  CalculatorAst();

  std::string name() const override;
  std::vector<Node> expressions;

  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;

  std::vector<Node> take_ast() { return std::move(expressions); }
};

class CalculatorInterpreter : public Benchmark {
private:
  class Interpreter {
  private:
    std::unordered_map<std::string, int64_t> variables;

    static int64_t simple_div(int64_t a, int64_t b);
    static int64_t simple_mod(int64_t a, int64_t b);

    struct Evaluator {
      std::unordered_map<std::string, int64_t> &variables;

      Evaluator(std::unordered_map<std::string, int64_t> &vars)
          : variables(vars) {}

      int64_t operator()(const CalculatorAst::Number &n) const;
      int64_t operator()(const CalculatorAst::Variable &v) const;
      int64_t
      operator()(const std::unique_ptr<CalculatorAst::BinaryOp> &binop) const;
      int64_t operator()(
          const std::unique_ptr<CalculatorAst::Assignment> &assign) const;
    };

  public:
    int64_t run(const std::vector<CalculatorAst::Node> &expressions);
    void clear() { variables.clear(); }
  };

  int64_t n;
  uint32_t result_val;
  std::vector<CalculatorAst::Node> ast;

public:
  CalculatorInterpreter();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

std::vector<uint8_t> generate_test_data(int64_t size);

class BWTEncode : public Benchmark {
public:
  struct BWTResult {
    std::vector<uint8_t> transformed;
    int32_t original_idx;

    BWTResult(std::vector<uint8_t> t, int32_t idx)
        : transformed(std::move(t)), original_idx(idx) {}
  };

private:
  BWTResult bwt_transform(const std::vector<uint8_t> &input);

public:
  int64_t size_val;
  std::vector<uint8_t> test_data;
  BWTResult bwt_result;
  uint32_t result_val;

  BWTEncode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class BWTDecode : public Benchmark {
private:
  std::vector<uint8_t> bwt_inverse(const BWTEncode::BWTResult &bwt_result);

public:
  int64_t size_val;
  std::vector<uint8_t> test_data;
  std::vector<uint8_t> inverted;
  BWTEncode::BWTResult bwt_result;
  uint32_t result_val;

  BWTDecode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class HuffEncode : public Benchmark {
public:
  class HuffmanNode {
  public:
    int32_t frequency;
    uint8_t byte_val;
    bool is_leaf;
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;

    HuffmanNode(int32_t freq, uint8_t byte = 0, bool leaf = true)
        : frequency(freq), byte_val(byte), is_leaf(leaf) {}
  };

  struct HuffmanCodes {
    std::vector<int32_t> code_lengths;
    std::vector<int32_t> codes;

    HuffmanCodes() : code_lengths(256, 0), codes(256, 0) {}
  };

  struct EncodedResult {
    std::vector<uint8_t> data;
    int32_t bit_count;
    std::vector<int32_t> frequencies;

    EncodedResult(std::vector<uint8_t> d, int32_t bc, std::vector<int32_t> f)
        : data(std::move(d)), bit_count(bc), frequencies(std::move(f)) {}
  };

  static std::shared_ptr<HuffmanNode>
  build_huffman_tree(const std::vector<int32_t> &frequencies);
  void build_huffman_codes(const std::shared_ptr<HuffmanNode> &node,
                           int32_t code, int32_t length, HuffmanCodes &codes);
  EncodedResult huffman_encode(const std::vector<uint8_t> &data,
                               const HuffmanCodes &codes,
                               const std::vector<int32_t> &frequencies);

public:
  int64_t size_val;
  std::vector<uint8_t> test_data;
  EncodedResult encoded;
  uint32_t result_val;

  HuffEncode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class HuffDecode : public Benchmark {
private:
  std::vector<uint8_t>
  huffman_decode(const std::vector<uint8_t> &encoded,
                 const std::shared_ptr<HuffEncode::HuffmanNode> &root,
                 int32_t bit_count);

public:
  int64_t size_val;
  std::vector<uint8_t> test_data;
  std::vector<uint8_t> decoded;
  HuffEncode::EncodedResult encoded;
  uint32_t result_val;

  HuffDecode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class ArithEncode : public Benchmark {
public:
  struct ArithEncodedResult {
    std::vector<uint8_t> data;
    int32_t bit_count;
    std::vector<int32_t> frequencies;

    ArithEncodedResult() : bit_count(0) {}
    ArithEncodedResult(std::vector<uint8_t> d, int32_t bc,
                       std::vector<int32_t> f)
        : data(std::move(d)), bit_count(bc), frequencies(std::move(f)) {}
  };

  class ArithFreqTable {
  public:
    int32_t total;
    std::vector<int32_t> low;
    std::vector<int32_t> high;

    ArithFreqTable(const std::vector<int32_t> &frequencies);
    ArithFreqTable(int32_t t, std::vector<int32_t> l, std::vector<int32_t> h)
        : total(t), low(std::move(l)), high(std::move(h)) {}
  };

  class BitOutputStream {
  private:
    uint8_t buffer = 0;
    int32_t bit_pos = 0;
    std::vector<uint8_t> bytes;
    int32_t bits_written = 0;

  public:
    void write_bit(int32_t bit);
    std::vector<uint8_t> flush();
    int32_t get_bits_written() const { return bits_written; }
    void clear();
  };

private:
  ArithEncodedResult arith_encode(const std::vector<uint8_t> &data);

public:
  int64_t size_val = 0;
  uint32_t result_val = 0;
  std::vector<uint8_t> test_data;
  ArithEncodedResult encoded;

  ArithEncode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class ArithDecode : public Benchmark {
public:
  class BitInputStream {
  private:
    const std::vector<uint8_t> &bytes;
    size_t byte_pos = 0;
    int32_t bit_pos = 0;
    uint8_t current_byte = 0;

  public:
    BitInputStream(const std::vector<uint8_t> &b);
    int32_t read_bit();
  };

private:
  std::vector<uint8_t>
  arith_decode(const ArithEncode::ArithEncodedResult &encoded);

public:
  int64_t size_val = 0;
  uint32_t result_val = 0;
  std::vector<uint8_t> test_data;
  std::vector<uint8_t> decoded;
  ArithEncode::ArithEncodedResult encoded;

  ArithDecode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class LZWEncode : public Benchmark {
public:
  struct LZWResult {
    std::vector<uint8_t> data;
    int32_t dict_size;

    LZWResult() : dict_size(256) {}
    LZWResult(std::vector<uint8_t> d, int32_t ds)
        : data(std::move(d)), dict_size(ds) {}
  };

private:
  LZWResult lzw_encode(const std::vector<uint8_t> &input);

public:
  int64_t size_val = 0;
  uint32_t result_val = 0;
  std::vector<uint8_t> test_data;
  LZWResult encoded;

  LZWEncode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class LZWDecode : public Benchmark {
private:
  std::vector<uint8_t> lzw_decode(const LZWEncode::LZWResult &encoded);

public:
  int64_t size_val = 0;
  uint32_t result_val = 0;
  std::vector<uint8_t> test_data;
  std::vector<uint8_t> decoded;
  LZWEncode::LZWResult encoded;

  LZWDecode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class CsvParse : public Benchmark {
private:
  struct Point {
    double x, y, z;
    Point(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
  };

  int64_t rows;
  std::string data;
  uint32_t result_val;

public:
  CsvParse();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

std::vector<std::pair<std::string, std::string>>
generate_pair_strings(int64_t n, int64_t m);

class Jaro : public Benchmark {
private:
  int64_t count;
  int64_t size;
  std::vector<std::pair<std::string, std::string>> pairs;
  uint32_t result_val;

  double jaro(const std::string &s1, const std::string &s2);

public:
  Jaro();

  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
  std::string name() const override;
};

class NGram : public Benchmark {
private:
  int64_t count;
  int64_t size;
  std::vector<std::pair<std::string, std::string>> pairs;
  uint32_t result_val;

  double ngram(const std::string &_s1, const std::string &_s2);

public:
  NGram();

  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
  std::string name() const override;
};


class Fannkuchredux : public Benchmark {
private:
  int64_t n;
  uint32_t result_val;

  std::pair<int, int> fannkuchredux(int n);

public:
  Fannkuchredux();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};

class GameOfLife : public Benchmark {
private:
  class Cell {
  private:
    bool alive;
    bool next_state;
    std::vector<Cell *> neighbors;

  public:
    Cell(bool alive = false);
    void add_neighbor(Cell *cell);
    void compute_next_state();
    void update();
    void set_alive(bool state);
    bool is_alive() const;
  };

  class Grid {
  private:
    int width;
    int height;
    std::vector<std::vector<Cell>> cells;

    void link_neighbors();

  public:
    Grid(int w, int h);

    void next_generation();
    std::vector<std::vector<Cell>> &get_cells();
    int count_alive() const;
    uint32_t compute_hash() const;
  };

  int32_t width;
  int32_t height;
  Grid grid;

public:
  GameOfLife();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class GraphPathBenchmark : public Benchmark {
protected:
  class Graph {
  public:
    int vertices;
    int jumps;
    int jump_len;
    std::vector<std::vector<int>> adj;

    Graph(int vertices, int jumps = 3, int jump_len = 100);
    void add_edge(int u, int v);
    void generate_random();
  };

  std::unique_ptr<Graph> graph;
  uint32_t result_val;

public:
  GraphPathBenchmark();

  void prepare() override;
  virtual int64_t test() = 0;
  void run(int) override;
  uint32_t checksum() override;
};

class GraphPathBFS : public GraphPathBenchmark {
private:
  int bfs_shortest_path(int start, int target);

public:
  GraphPathBFS() = default;

  std::string name() const override;
  int64_t test() override;
};

class GraphPathDFS : public GraphPathBenchmark {
private:
  int dfs_shortest_path(int start, int target);

public:
  GraphPathDFS() = default;

  std::string name() const override;
  int64_t test() override;
};

class GraphPathAStar : public GraphPathBenchmark {
private:
  struct Node {
    int vertex;
    int f_score;

    bool operator>(const Node &other) const { return f_score > other.f_score; }
  };

  int heuristic(int v, int target) const { return target - v; }
  int astar_shortest_path(int start, int target);

public:
  GraphPathAStar() = default;

  std::string name() const override;
  int64_t test() override;
};

struct Coordinate {
    double x, y, z;
    std::string name;
    std::unordered_map<std::string, std::pair<int, bool>> opts;
};

class JsonGenerate : public Benchmark {
private:
    std::vector<Coordinate> data;
    uint32_t result;
    int64_t n;
    std::string _result;

    static double custom_round(double v, int digits) {
        double mul = 1.0;
        for (int i = 0; i < digits; ++i) mul *= 10.0;
        return std::round(v * mul) / mul;
    }

public:
    JsonGenerate();

    std::string name() const override;
    void prepare() override;
    void run(int) override;
    uint32_t checksum() override;

    std::string get_result() const { return _result; }
    int64_t& coords_count() { return n; }
};

class JsonParseDom : public Benchmark {
private:
    std::string text;
    uint32_t result_val;

public:
    JsonParseDom();

    std::string name() const override;
    void prepare() override;
    void run(int) override;
    uint32_t checksum() override;
};

class JsonParseMapping : public Benchmark {
private:
    std::string text;
    uint32_t result_val;

public:
    JsonParseMapping();

    std::string name() const override;
    void prepare() override;
    void run(int) override;
    uint32_t checksum() override;
};

class LogParser : public Benchmark {
private:
  static constexpr int PATTERNS_COUNT = 13;
  static constexpr int MAX_TOKENS = 512;

  std::string log;
  size_t log_size{0};
  uint32_t checksum_val{0};
  int lines_count{0};

  RegexToken compiled[PATTERNS_COUNT][MAX_TOKENS];
  int16_t token_counts[PATTERNS_COUNT];
  std::vector<std::string> pattern_names;

  const std::vector<std::string> IPS = [] {
    std::vector<std::string> ips;
    for (int i = 1; i <= 255; i++)
      ips.push_back("192.168.1." + std::to_string(i));
    return ips;
  }();

  const std::vector<std::string> METHODS = {"GET", "POST", "PUT", "DELETE"};
  const std::vector<std::string> PATHS = {"/index.html", "/api/users",
                                          "/admin",      "/images/logo.png",
                                          "/etc/passwd", "/wp-admin/setup.php"};
  const std::vector<int> STATUSES = {200, 201, 301, 302, 400, 401,
                                     403, 404, 500, 502, 503};
  const std::vector<std::string> AGENTS = {"Mozilla/5.0", "Googlebot/2.1",
                                           "curl/7.68.0", "scanner/2.0"};
  const std::vector<std::string> USERS = {"john", "jane", "alex",  "sarah",
                                          "mike", "anna", "david", "elena"};
  const std::vector<std::string> DOMAINS = {"example.com", "gmail.com",
                                            "yahoo.com",   "hotmail.com",
                                            "company.org", "mail.ru"};

  static constexpr std::array<const char *, 13> PATTERNS = {
      " [45][0-9]{2} ",
      "bot|crawler|scanner|spider|indexing|crawl|robot|Googlebot|Mozilla|curl",
      "etc/passwd|wp-admin|\\.\\./",
      "\\d+\\.\\d+\\.\\d+\\.35",
      "/api/[^ \" ]+",
      "POST [^ ]* HTTP",
      "/login|/signin",
      "GET|POST|PUT|DELETE",
      "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}",
      "password=[^&\\s\"]+",
      "token=[^&\\s\"]+|api[_-]?key=[^&\\s\"]+",
      "session[_-]?id=[^&\\s\"]+",
      "\\[\\d+/\\w+/\\d+:1[3-7]:\\d+:\\d+ [+\\-]\\d+\\]"};

  static constexpr std::array<const char *, 13> NAMES = {
      "errors",        "bots",          "suspicious", "ips",    "api_calls",
      "post_requests", "auth_attempts", "methods",    "emails", "passwords",
      "tokens",        "sessions",      "peak_hours"};

  void generate_log_line(std::string &str, int i);

public:
  LogParser();
  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};




class Mandelbrot : public Benchmark {
private:
  static constexpr int ITER = 50;
  static constexpr double LIMIT = 2.0;

  int64_t w, h;
  std::vector<uint8_t> result_bin;

public:
  Mandelbrot();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};

class Matmul1T : public Benchmark {
protected:
  uint32_t result_val;
  std::vector<std::vector<double>> a, b;

  std::vector<std::vector<double>> matgen(int n);
  std::vector<std::vector<double>>
  matmul(int n, const std::vector<std::vector<double>> &a,
         const std::vector<std::vector<double>> &b);

public:
  Matmul1T();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class Matmul4T : public Matmul1T {
protected:
  virtual int get_num_threads() const { return 2; }

  std::vector<std::vector<double>>
  matmul_parallel(int n, const std::vector<std::vector<double>> &a,
                  const std::vector<std::vector<double>> &b);

public:
  Matmul4T() = default;

  std::string name() const override;
  void run(int) override;
};

class Matmul8T : public Matmul4T {
protected:
  int get_num_threads() const override { return 4; }

public:
  Matmul8T() = default;
  std::string name() const override;
};

class Matmul16T : public Matmul4T {
protected:
  int get_num_threads() const override { return 8; }

public:
  Matmul16T() = default;
  std::string name() const override;
};

class MazeGenerator : public Benchmark {
public:
  enum class CellKind : uint8_t {
    Wall = 0,
    Space,
    Start,
    Finish,
    Border,
    Path
  };

  class Cell {
  public:
    CellKind kind;
    std::vector<Cell *> neighbors;
    int x;
    int y;

    Cell(int x, int y);
    bool is_walkable() const;
    void reset();
    uint32_t value() const;
  };

  class Maze {
  private:
    int w;
    int h;
    std::vector<std::vector<Cell>> cells;
    Cell *start;
    Cell *finish;

  public:
    Maze(int width, int height);

    void update_neighbors();
    void reset();
    void dig(Cell *start_cell);
    void ensure_open_finish(Cell *cell);
    void generate();

    Cell *middle_cell();
    Cell *get_start();
    Cell *get_finish();
    Cell *get_cell(int x, int y);

    uint32_t checksum() const;
    void print_to_console() const;
  };

private:
  uint32_t result_val;
  int32_t width;
  int32_t height;
  std::unique_ptr<Maze> maze;

public:
  MazeGenerator();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class MazeBFS : public Benchmark {
private:
  uint32_t result_val;
  int32_t width;
  int32_t height;
  std::unique_ptr<MazeGenerator::Maze> maze;
  std::vector<MazeGenerator::Cell *> path;

  std::vector<MazeGenerator::Cell *> bfs(MazeGenerator::Cell *start,
                                         MazeGenerator::Cell *target);
  uint32_t mid_cell_checksum(const std::vector<MazeGenerator::Cell *> &p);

public:
  MazeBFS();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class MazeAStar : public Benchmark {
private:
  struct Node {
    int f_score;
    int idx;

    bool operator>(const Node &other) const;
  };

  uint32_t result_val;
  int32_t width;
  int32_t height;
  std::unique_ptr<MazeGenerator::Maze> maze;
  std::vector<MazeGenerator::Cell *> path;

  int heuristic(MazeGenerator::Cell *a, MazeGenerator::Cell *b);
  int idx(int y, int x) const;
  std::vector<MazeGenerator::Cell *> astar(MazeGenerator::Cell *start,
                                           MazeGenerator::Cell *target);
  uint32_t mid_cell_checksum(const std::vector<MazeGenerator::Cell *> &p);

public:
  MazeAStar();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class Nbody : public Benchmark {
private:
  static constexpr double SOLAR_MASS = 4 * M_PI * M_PI;
  static constexpr double DAYS_PER_YEAR = 365.24;

  struct Planet {
    double x, y, z;
    double vx, vy, vz;
    double mass;

    Planet(double x, double y, double z, double vx, double vy, double vz,
           double mass)
        : x(x), y(y), z(z), vx(vx * DAYS_PER_YEAR), vy(vy * DAYS_PER_YEAR),
          vz(vz * DAYS_PER_YEAR), mass(mass * SOLAR_MASS) {}

    void move_from_i(std::vector<Planet> &bodies, int nbodies, double dt,
                     int start);
  };

  std::vector<Planet> bodies;
  double v1;

  double energy();
  void offset_momentum();

public:
  Nbody();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class NeuralNet : public Benchmark {
private:
  class Neuron;

  class Synapse {
  public:
    double weight;
    double prev_weight;
    Neuron *source_neuron;
    Neuron *dest_neuron;

    Synapse(Neuron *source, Neuron *dest);
  };

  class Neuron {
  private:
    static constexpr double LEARNING_RATE = 1.0;
    static constexpr double MOMENTUM = 0.3;

    std::vector<Synapse *> synapses_in;
    std::vector<Synapse *> synapses_out;
    double threshold;
    double prev_threshold;
    double error;
    double output;

  public:
    Neuron();

    void calculate_output();
    double derivative() const { return output * (1 - output); }
    void output_train(double rate, double target);
    void hidden_train(double rate);
    void update_weights(double rate);
    void add_synapse_in(Synapse *synapse) { synapses_in.push_back(synapse); }
    void add_synapse_out(Synapse *synapse) { synapses_out.push_back(synapse); }
    void set_output(double val) { output = val; }
    double get_output() const { return output; }
  };

  class NeuralNetwork {
  private:
    std::vector<Neuron> input_layer;
    std::vector<Neuron> hidden_layer;
    std::vector<Neuron> output_layer;
    std::vector<std::unique_ptr<Synapse>> synapses;

  public:
    NeuralNetwork(int inputs, int hidden, int outputs);

    void train(const std::vector<double> &inputs,
               const std::vector<double> &targets);
    void feed_forward(const std::vector<double> &inputs);
    std::vector<double> current_outputs();
  };

  std::unique_ptr<NeuralNetwork> xor_net;

public:
  NeuralNet();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class Sieve : public Benchmark {
private:
  int64_t limit;
  uint32_t checksum_val;

public:
  Sieve();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};

class SortBenchmark : public Benchmark {
protected:
  std::vector<int32_t> data;
  int64_t size_val;
  uint32_t result_val;

  SortBenchmark();

public:
  virtual std::vector<int32_t> test() = 0;

  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class SortQuick : public SortBenchmark {
private:
  void quick_sort(std::vector<int32_t> &arr, int low, int high);

public:
  SortQuick() = default;

  std::string name() const override;
  std::vector<int32_t> test() override;
};

class SortMerge : public SortBenchmark {
private:
  void merge_sort_inplace(std::vector<int32_t> &arr);
  void merge_sort_helper(std::vector<int32_t> &arr, std::vector<int32_t> &temp,
                         int left, int right);
  void merge(std::vector<int32_t> &arr, std::vector<int32_t> &temp, int left,
             int mid, int right);

public:
  SortMerge() = default;

  std::string name() const override;
  std::vector<int32_t> test() override;
};

class SortSelf : public SortBenchmark {
public:
  SortSelf() = default;

  std::string name() const override;
  std::vector<int32_t> test() override;
};

class Spectralnorm : public Benchmark {
private:
  int64_t size_val;
  std::vector<double> u;
  std::vector<double> v;

  double eval_A(int i, int j) {
    return 1.0 / ((i + j) * (i + j + 1.0) / 2.0 + i + 1.0);
  }

  std::vector<double> eval_A_times_u(const std::vector<double> &u);
  std::vector<double> eval_At_times_u(const std::vector<double> &u);
  std::vector<double> eval_AtA_times_u(const std::vector<double> &u);

public:
  Spectralnorm();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};

void generate_template(std::string &text,
                       std::unordered_map<std::string, std::string> &vars,
                       int count);

class TemplateRegex : public Benchmark {
private:
  std::string text;
  std::string rendered;
  uint32_t checksum_val;
  int count;
  std::unordered_map<std::string, std::string> vars;
  re_t regex;

  static constexpr const char *PATTERN = R"(\{\{[^}]*\}\})";

  std::string trim(const std::string &str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    if (start == std::string::npos)
      return "";
    return str.substr(start, end - start + 1);
  }

public:
  TemplateRegex();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class TemplateParse : public Benchmark {
private:
  std::string text;
  std::string rendered;
  uint32_t checksum_val;
  int count;
  std::unordered_map<std::string, std::string> vars;

  std::string trim(const std::string &str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    if (start == std::string::npos)
      return "";
    return str.substr(start, end - start + 1);
  }

public:
  TemplateParse();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class TextRaytracer : public Benchmark {
private:
  struct Vector {
    double x, y, z;

    Vector scale(double s) const { return {x * s, y * s, z * s}; }
    Vector add(const Vector &other) const {
      return {x + other.x, y + other.y, z + other.z};
    }
    Vector sub(const Vector &other) const {
      return {x - other.x, y - other.y, z - other.z};
    }
    double dot(const Vector &other) const {
      return x * other.x + y * other.y + z * other.z;
    }
    double magnitude() const { return std::sqrt(dot(*this)); }
    Vector normalize() const;
  };

  struct Ray {
    Vector orig, dir;
  };

  struct Color {
    double r, g, b;

    Color scale(double s) const { return {r * s, g * s, b * s}; }
    Color add(const Color &other) const {
      return {r + other.r, g + other.g, b + other.b};
    }
  };

  struct Sphere {
    Vector center;
    double radius;
    Color color;

    Vector get_normal(const Vector &pt) const {
      return pt.sub(center).normalize();
    }
  };

  struct Light {
    Vector position;
    Color color;
  };

  static constexpr Color WHITE = {1.0, 1.0, 1.0};
  static constexpr Color RED = {1.0, 0.0, 0.0};
  static constexpr Color GREEN = {0.0, 1.0, 0.0};
  static constexpr Color BLUE = {0.0, 0.0, 1.0};

  static constexpr Light LIGHT1 = {{0.7, -1.0, 1.7}, WHITE};
  static constexpr char LUT[6] = {'.', '-', '+', '*', 'X', 'M'};

  std::vector<Sphere> SCENE = {{{-1.0, 0.0, 3.0}, 0.3, RED},
                               {{0.0, 0.0, 3.0}, 0.8, GREEN},
                               {{1.0, 0.0, 3.0}, 0.4, BLUE}};

  int32_t w, h;
  uint32_t result_val;

  int shade_pixel(const Ray &ray, const Sphere &obj, double tval);
  std::optional<double> intersect_sphere(const Ray &ray, const Vector &center,
                                         double radius);
  double clamp(double x, double a, double b);
  Color diffuse_shading(const Vector &pi, const Sphere &obj,
                        const Light &light);

public:
  TextRaytracer();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};

class Words : public Benchmark {
private:
  int words;
  int word_len;
  std::string text;
  uint32_t checksum_val;

public:
  Words();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

namespace b64 {

static const char *const B64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const int8_t B64_LOOKUP[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1,
    -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1};

inline size_t encode_size(size_t src_size) { return ((src_size + 2) / 3) * 4; }

inline size_t decode_size(size_t src_size) { return (src_size / 4) * 3; }

static size_t encode(char *dst, const char *src, size_t src_size) {
  size_t i = 0, o = 0;
  while (i + 2 < src_size) {
    uint32_t v = (uint8_t(src[i]) << 16) | (uint8_t(src[i + 1]) << 8) |
                 uint8_t(src[i + 2]);
    dst[o++] = B64_CHARS[(v >> 18) & 0x3F];
    dst[o++] = B64_CHARS[(v >> 12) & 0x3F];
    dst[o++] = B64_CHARS[(v >> 6) & 0x3F];
    dst[o++] = B64_CHARS[v & 0x3F];
    i += 3;
  }
  if (i < src_size) {
    uint32_t v = uint8_t(src[i]) << 16;
    bool two = (i + 1 < src_size);
    if (two)
      v |= uint8_t(src[i + 1]) << 8;
    dst[o++] = B64_CHARS[(v >> 18) & 0x3F];
    dst[o++] = B64_CHARS[(v >> 12) & 0x3F];
    dst[o++] = two ? B64_CHARS[(v >> 6) & 0x3F] : '=';
    dst[o++] = '=';
  }
  return o;
}

static size_t decode(char *dst, const char *src, size_t src_size) {
  if (src_size % 4 != 0)
    return 0;
  size_t o = 0;
  for (size_t i = 0; i < src_size; i += 4) {
    int8_t a = B64_LOOKUP[uint8_t(src[i])];
    int8_t b = B64_LOOKUP[uint8_t(src[i + 1])];
    if (a < 0 || b < 0)
      return 0;

    int8_t c = (src[i + 2] == '=') ? -2 : B64_LOOKUP[uint8_t(src[i + 2])];
    int8_t d = (src[i + 3] == '=') ? -2 : B64_LOOKUP[uint8_t(src[i + 3])];
    if (c == -1 || d == -1)
      return 0;

    uint32_t v = (uint32_t(a) << 18) | (uint32_t(b) << 12);
    if (c >= 0)
      v |= uint32_t(c) << 6;
    if (d >= 0)
      v |= uint32_t(d);

    dst[o++] = char((v >> 16) & 0xFF);
    if (c >= 0)
      dst[o++] = char((v >> 8) & 0xFF);
    if (d >= 0)
      dst[o++] = char(v & 0xFF);
  }
  return o;
}

} // namespace b64

Base64Encode::Base64Encode() : result_val(0) {
  int64_t n = config_val("size");
  str = std::string(static_cast<size_t>(n), 'a');
  str2 = base64_encode_simple(str);
}

std::string Base64Encode::name() const { return "Base64::Encode"; }

size_t Base64Encode::b64_encode(char *dst, const char *src, size_t src_size) {
  return b64::encode(dst, src, src_size);
}

std::string Base64Encode::base64_encode_simple(const std::string &input) {
  size_t encoded_len = encode_size(input.size());
  std::string result;
  result.resize(encoded_len);
  size_t actual_len = b64::encode(&result[0], input.data(), input.size());
  result.resize(actual_len);
  return result;
}

void Base64Encode::run(int iteration_id) {
  (void)iteration_id;
  str2 = base64_encode_simple(str);
  result_val += str2.size();
}

uint32_t Base64Encode::checksum() {
  std::ostringstream ss;
  ss << "encode " << (str.size() > 4 ? str.substr(0, 4) + "..." : str) << " to "
     << (str2.size() > 4 ? str2.substr(0, 4) + "..." : str2) << ": "
     << result_val;
  return Helper::checksum(ss.str());
}

Base64Decode::Base64Decode() : result_val(0) {
  int64_t n = config_val("size");
  std::string str = std::string(static_cast<size_t>(n), 'a');

  size_t encoded_size = encode_size(str.size());
  str2.resize(encoded_size);
  size_t actual_encoded = b64::encode(&str2[0], str.data(), str.size());
  str2.resize(actual_encoded);

  str3 = base64_decode_simple(str2);
}

std::string Base64Decode::name() const { return "Base64::Decode"; }

size_t Base64Decode::b64_decode(char *dst, const char *src, size_t src_size) {

  return b64::decode(dst, src, src_size);
}

std::string Base64Decode::base64_decode_simple(const std::string &input) {
  size_t decoded_size = decode_size(input.size());
  std::string result;
  result.resize(decoded_size);
  size_t actual_len = b64::decode(&result[0], input.data(), input.size());
  result.resize(actual_len);
  return result;
}

void Base64Decode::run(int iteration_id) {
  (void)iteration_id;
  str3 = base64_decode_simple(str2);
  result_val += str3.size();
}

uint32_t Base64Decode::checksum() {
  std::ostringstream ss;
  ss << "decode " << (str2.size() > 4 ? str2.substr(0, 4) + "..." : str2)
     << " to " << (str3.size() > 4 ? str3.substr(0, 4) + "..." : str3) << ": "
     << result_val;
  return Helper::checksum(ss.str());
}

int64_t Benchmark::warmup_iterations() {
  if (config_has(name())) {
    int64_t warmup = config_i64(name(), "warmup_iterations");
    if (warmup > 0)
      return warmup;
  }

  int64_t iters = iterations();
  return std::max<int64_t>(static_cast<int64_t>(iters * 0.2), 1LL);
}

void Benchmark::warmup() {
  int64_t prepare_iters = warmup_iterations();
  for (int64_t i = 0; i < prepare_iters; i++) {
    this->run(i);
  }
}

void Benchmark::run_all() {
  int64_t iters = iterations();
  for (int64_t i = 0; i < iters; i++) {
    this->run(i);
  }
}

void Benchmark::all(const std::string &single_bench) {
  double summary_time = 0.0;
  int ok = 0;
  int fails = 0;

  std::unordered_map<std::string, std::function<std::unique_ptr<Benchmark>()>>
      available_benches = {
          {"Binarytrees::Obj",
           []() { return std::make_unique<BinarytreesObj>(); }},
          {"Binarytrees::Arena",
           []() { return std::make_unique<BinarytreesArena>(); }},
          {"Brainfuck::Array",
           []() { return std::make_unique<BrainfuckArray>(); }},
          {"Brainfuck::Recursion",
           []() { return std::make_unique<BrainfuckRecursion>(); }},
          {"CLBG::Fannkuchredux",
           []() { return std::make_unique<Fannkuchredux>(); }},
          {"CLBG::Mandelbrot", []() { return std::make_unique<Mandelbrot>(); }},
          {"Matmul::Single", []() { return std::make_unique<Matmul1T>(); }},
          {"Matmul::T4", []() { return std::make_unique<Matmul4T>(); }},
          {"Matmul::T8", []() { return std::make_unique<Matmul8T>(); }},
          {"Matmul::T16", []() { return std::make_unique<Matmul16T>(); }},
          {"CLBG::Nbody", []() { return std::make_unique<Nbody>(); }},
          {"CLBG::Spectralnorm",
           []() { return std::make_unique<Spectralnorm>(); }},
          {"Base64::Encode", []() { return std::make_unique<Base64Encode>(); }},
          {"Base64::Decode", []() { return std::make_unique<Base64Decode>(); }},
          {"Json::Generate", []() { return std::make_unique<JsonGenerate>(); }},
          {"Json::ParseDom", []() { return std::make_unique<JsonParseDom>(); }},
          {"Json::ParseMapping",
           []() { return std::make_unique<JsonParseMapping>(); }},
          {"Etc::Sieve", []() { return std::make_unique<Sieve>(); }},
          {"Etc::TextRaytracer",
           []() { return std::make_unique<TextRaytracer>(); }},
          {"Etc::NeuralNet", []() { return std::make_unique<NeuralNet>(); }},
          {"Etc::Words", []() { return std::make_unique<Words>(); }},
          {"Sort::Quick", []() { return std::make_unique<SortQuick>(); }},
          {"Sort::Merge", []() { return std::make_unique<SortMerge>(); }},
          {"Sort::Self", []() { return std::make_unique<SortSelf>(); }},
          {"Graph::BFS", []() { return std::make_unique<GraphPathBFS>(); }},
          {"Graph::DFS", []() { return std::make_unique<GraphPathDFS>(); }},
          {"Graph::AStar", []() { return std::make_unique<GraphPathAStar>(); }},
          {"Hash::SHA256",
           []() { return std::make_unique<BufferHashSHA256>(); }},
          {"Hash::CRC32", []() { return std::make_unique<BufferHashCRC32>(); }},
          {"Etc::CacheSimulation",
           []() { return std::make_unique<CacheSimulation>(); }},
          {"Calculator::Ast",
           []() { return std::make_unique<CalculatorAst>(); }},
          {"Calculator::Interpreter",
           []() { return std::make_unique<CalculatorInterpreter>(); }},
          {"Etc::GameOfLife", []() { return std::make_unique<GameOfLife>(); }},
          {"Maze::Generator",
           []() { return std::make_unique<MazeGenerator>(); }},
          {"Maze::BFS", []() { return std::make_unique<MazeBFS>(); }},
          {"Maze::AStar", []() { return std::make_unique<MazeAStar>(); }},
          {"Compress::BWTEncode",
           []() { return std::make_unique<BWTEncode>(); }},
          {"Compress::BWTDecode",
           []() { return std::make_unique<BWTDecode>(); }},
          {"Compress::HuffEncode",
           []() { return std::make_unique<HuffEncode>(); }},
          {"Compress::HuffDecode",
           []() { return std::make_unique<HuffDecode>(); }},
          {"Compress::ArithEncode",
           []() { return std::make_unique<ArithEncode>(); }},
          {"Compress::ArithDecode",
           []() { return std::make_unique<ArithDecode>(); }},
          {"Compress::LZWEncode",
           []() { return std::make_unique<LZWEncode>(); }},
          {"Compress::LZWDecode",
           []() { return std::make_unique<LZWDecode>(); }},
          {"Distance::Jaro", []() { return std::make_unique<Jaro>(); }},
          {"Distance::NGram", []() { return std::make_unique<NGram>(); }},
          {"Etc::LogParser", []() { return std::make_unique<LogParser>(); }},
          {"Template::Regex",
           []() { return std::make_unique<TemplateRegex>(); }},
          {"Template::Parse",
           []() { return std::make_unique<TemplateParse>(); }},
          {"CSV::Parse", []() { return std::make_unique<CsvParse>(); }},
      };

  for (const auto &bench_name : config_keys()) {
    if (!single_bench.empty() &&
        to_lower(bench_name).find(to_lower(single_bench)) ==
            std::string::npos) {
      continue;
    }

    auto it = available_benches.find(bench_name);
    if (it != available_benches.end()) {
      std::cout << bench_name << ": ";
      std::cout.flush();

      auto bench = it->second();
      Helper::reset();
      bench->prepare();
      bench->warmup();
      Helper::reset();

      auto start = std::chrono::steady_clock::now();
      bench->run_all();
      auto end = std::chrono::steady_clock::now();

      std::chrono::duration<double> duration = end - start;

      uint32_t check = bench->checksum();
      uint32_t expect = static_cast<uint32_t>(bench->expected_checksum());
      if (check == expect) {
        std::cout << "OK ";
        ok++;
      } else {
        std::cout << "ERR[actual=" << check << ", expected=" << expect << "] ";
        fails++;
      }

      std::cout << "in " << std::fixed << std::setprecision(3)
                << duration.count() << "s" << std::endl;

      summary_time += duration.count();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } else {
      std::cout << "Warning: Benchmark '" << bench_name
                << "' defined in config but not found in code" << std::endl;
    }
  }

  if (ok + fails > 0) {
    std::cout << "Summary: " << std::fixed << std::setprecision(4)
              << summary_time << "s, " << (ok + fails) << ", " << ok << ", "
              << fails << std::endl;
  }

  if (fails > 0) {
    std::exit(1);
  }
}

BinarytreesObj::TreeNode::TreeNode(int32_t item, int depth) : item(item) {
  if (depth > 0) {
    left = std::make_unique<TreeNode>(item - (1 << (depth - 1)), depth - 1);
    right = std::make_unique<TreeNode>(item + (1 << (depth - 1)), depth - 1);
  }
}

BinarytreesObj::BinarytreesObj() : n(config_val("depth")), result_val(0) {}

std::string BinarytreesObj::name() const { return "Binarytrees::Obj"; }

void BinarytreesObj::run(int iteration_id) {
  (void)iteration_id;
  TreeNode root(0, n);
  result_val += root.sum();
}

uint32_t BinarytreesObj::checksum() { return result_val; }

BinarytreesArena::BinarytreesArena() : n(config_val("depth")), result_val(0) {}

std::string BinarytreesArena::name() const { return "Binarytrees::Arena"; }

int32_t BinarytreesArena::build_tree(int32_t item, int32_t depth) {
  int32_t idx = static_cast<int32_t>(arena.size());
  arena.emplace_back(item);

  if (depth > 0) {
    int32_t left_idx = build_tree(item - (1 << (depth - 1)), depth - 1);
    int32_t right_idx = build_tree(item + (1 << (depth - 1)), depth - 1);
    auto &node = arena[idx];
    node.left = left_idx;
    node.right = right_idx;
  }

  return idx;
}

uint32_t BinarytreesArena::sum(int32_t idx) const {
  const auto &node = arena[idx];
  uint32_t total = static_cast<uint32_t>(node.item) + 1;

  if (node.left >= 0) {
    total += sum(node.left);
  }
  if (node.right >= 0) {
    total += sum(node.right);
  }

  return total;
}

void BinarytreesArena::run(int iteration_id) {
  (void)iteration_id;
  arena = std::vector<TreeNode>();
  build_tree(0, static_cast<int32_t>(n));
  result_val += sum(0);
}

uint32_t BinarytreesArena::checksum() { return result_val; }

BrainfuckArray::Program::Program(const std::string &text) {
  for (char c : text) {
    if (std::string("[]<>+-,.").find(c) != std::string::npos) {
      commands.push_back(static_cast<uint8_t>(c));
    }
  }

  jumps.resize(commands.size(), 0);
  std::vector<size_t> stack;

  for (size_t i = 0; i < commands.size(); ++i) {
    uint8_t cmd = commands[i];
    if (cmd == '[') {
      stack.push_back(i);
    } else if (cmd == ']' && !stack.empty()) {
      size_t start = stack.back();
      stack.pop_back();
      jumps[start] = i;
      jumps[i] = start;
    }
  }
}

int64_t BrainfuckArray::Program::run() {
  int64_t result = 0;
  Tape tape;
  size_t pc = 0;

  while (pc < commands.size()) {
    uint8_t cmd = commands[pc];
    switch (cmd) {
    case '+':
      tape.inc();
      break;
    case '-':
      tape.dec();
      break;
    case '>':
      tape.advance();
      break;
    case '<':
      tape.devance();
      break;
    case '[':
      if (tape.get() == 0) {
        pc = jumps[pc];
        continue;
      }
      break;
    case ']':
      if (tape.get() != 0) {
        pc = jumps[pc];
        continue;
      }
      break;
    case '.':
      result = (result << 2) + static_cast<int64_t>(tape.get());
      break;
    }
    pc++;
  }
  return result;
}

BrainfuckArray::BrainfuckArray() : result_val(0) {
  program_text = Helper::config_s(name(), "program");
  warmup_text = Helper::config_s(name(), "warmup_program");
}

std::string BrainfuckArray::name() const { return "Brainfuck::Array"; }

void BrainfuckArray::warmup() {
  int64_t prepare_iters = warmup_iterations();
  for (int64_t i = 0; i < prepare_iters; i++) {
    _run(warmup_text);
  }
}

void BrainfuckArray::run(int iteration_id) {
  (void)iteration_id;
  int64_t run_result = _run(program_text);
  result_val += static_cast<uint32_t>(run_result);
}

uint32_t BrainfuckArray::checksum() { return result_val; }

int64_t BrainfuckArray::_run(const std::string &text) {
  Program program(text);
  return program.run();
}

BrainfuckRecursion::Program::Program(const std::string &code) {
  auto it = code.begin();
  ops = parse(it, code.end());
}

std::vector<BrainfuckRecursion::Op>
BrainfuckRecursion::Program::parse(std::string::const_iterator &it,
                                   const std::string::const_iterator &end) {
  std::vector<Op> res;
  res.reserve(128);

  while (it != end) {
    char c = *it++;
    switch (c) {
    case '+':
      res.emplace_back(OpInc{});
      break;
    case '-':
      res.emplace_back(OpDec{});
      break;
    case '>':
      res.emplace_back(OpAdvance{});
      break;
    case '<':
      res.emplace_back(OpDevance{});
      break;
    case '.':
      res.emplace_back(OpPrint{});
      break;
    case '[': {
      auto loop_ops = parse(it, end);
      res.emplace_back(OpLoop{std::move(loop_ops)});
      break;
    }
    case ']':
      return res;
    default:
      break;
    }
  }
  return res;
}

int64_t BrainfuckRecursion::Program::run() {
  Tape tape;
  int64_t result = 0;
  Visitor visitor{tape, result};

  for (const auto &op : ops) {
    std::visit(visitor, op);
  }
  return result;
}

BrainfuckRecursion::BrainfuckRecursion() : result_val(0) {
  text = Helper::config_s(name(), "program");
}

std::string BrainfuckRecursion::name() const { return "Brainfuck::Recursion"; }

void BrainfuckRecursion::warmup() {
  int64_t prepare_iters = warmup_iterations();
  std::string warmup_program = Helper::config_s(name(), "warmup_program");
  for (int64_t i = 0; i < prepare_iters; i++) {
    Program(warmup_program).run();
  }
}

void BrainfuckRecursion::run(int iteration_id) {
  (void)iteration_id;
  result_val += Program(text).run();
}

uint32_t BrainfuckRecursion::checksum() { return result_val; }

BufferHashBenchmark::BufferHashBenchmark() : size_val(0), result_val(0) {}

void BufferHashBenchmark::prepare() {
  if (size_val == 0) {
    size_val = config_val("size");
    data.resize(static_cast<size_t>(size_val));
    for (size_t i = 0; i < static_cast<size_t>(size_val); i++) {
      data[i] = static_cast<uint8_t>(Helper::next_int(256));
    }
  }
}

void BufferHashBenchmark::run(int iteration_id) {
  (void)iteration_id;
  uint8_t ch = static_cast<uint8_t>(Helper::next_int(256));
  data[static_cast<size_t>(Helper::next_int(size_val))] = ch;
  result_val += test();
}

uint32_t BufferHashBenchmark::checksum() { return result_val; }

std::string BufferHashSHA256::name() const { return "Hash::SHA256"; }

std::vector<uint8_t>
BufferHashSHA256::SimpleSHA256::digest(const std::vector<uint8_t> &data) {
  std::vector<uint8_t> result(32);

  uint32_t hashes[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

  for (size_t i = 0; i < data.size(); i++) {
    uint32_t hash_idx = i % 8;
    uint32_t &hash = hashes[hash_idx];
    hash = ((hash << 5) + hash) + data[i];
    hash = (hash + (hash << 10)) ^ (hash >> 6);
  }

  for (int i = 0; i < 8; i++) {
    result[i * 4] = static_cast<uint8_t>(hashes[i] >> 24);
    result[i * 4 + 1] = static_cast<uint8_t>(hashes[i] >> 16);
    result[i * 4 + 2] = static_cast<uint8_t>(hashes[i] >> 8);
    result[i * 4 + 3] = static_cast<uint8_t>(hashes[i]);
  }

  return result;
}

uint32_t BufferHashSHA256::test() {
  auto bytes = SimpleSHA256::digest(data);
  return *reinterpret_cast<uint32_t *>(bytes.data());
}

std::string BufferHashCRC32::name() const { return "Hash::CRC32"; }

uint32_t BufferHashCRC32::crc32(const std::vector<uint8_t> &data) {
  uint32_t crc = 0xFFFFFFFFu;

  for (uint8_t byte : data) {
    crc = crc ^ byte;
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xEDB88320u;
      } else {
        crc = crc >> 1;
      }
    }
  }
  return crc ^ 0xFFFFFFFFu;
}

uint32_t BufferHashCRC32::test() { return crc32(data); }

template <typename K, typename V>
CacheSimulation::LRUCache<K, V>::Node::Node(const K &k, const V &v)
    : key(k), value(v), prev(nullptr), next(nullptr) {}

template <typename K, typename V>
CacheSimulation::LRUCache<K, V>::LRUCache(int capacity)
    : capacity_(capacity), head_(nullptr), tail_(nullptr), size_(0) {}

template <typename K, typename V> CacheSimulation::LRUCache<K, V>::~LRUCache() {
  Node *current = head_;
  while (current) {
    Node *next = current->next;
    delete current;
    current = next;
  }
}

template <typename K, typename V>
void CacheSimulation::LRUCache<K, V>::move_to_front(Node *node) {
  if (node == head_)
    return;

  if (node->prev)
    node->prev->next = node->next;
  if (node->next)
    node->next->prev = node->prev;
  if (node == tail_)
    tail_ = node->prev;

  node->prev = nullptr;
  node->next = head_;
  if (head_)
    head_->prev = node;
  head_ = node;
  if (!tail_)
    tail_ = node;
}

template <typename K, typename V>
void CacheSimulation::LRUCache<K, V>::add_to_front(Node *node) {
  node->next = head_;
  if (head_)
    head_->prev = node;
  head_ = node;
  if (!tail_)
    tail_ = node;
}

template <typename K, typename V>
void CacheSimulation::LRUCache<K, V>::remove_oldest() {
  if (!tail_)
    return;

  Node *oldest = tail_;
  cache_.erase(oldest->key);

  if (oldest->prev)
    oldest->prev->next = nullptr;
  tail_ = oldest->prev;
  if (head_ == oldest)
    head_ = nullptr;

  delete oldest;
  size_--;
}

template <typename K, typename V>
std::optional<V> CacheSimulation::LRUCache<K, V>::get(const K &key) {
  auto it = cache_.find(key);
  if (it == cache_.end())
    return std::nullopt;

  Node *node = it->second;
  move_to_front(node);
  return node->value;
}

template <typename K, typename V>
void CacheSimulation::LRUCache<K, V>::put(const K &key, const V &value) {
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    Node *node = it->second;
    node->value = value;
    move_to_front(node);
    return;
  }

  if (size_ >= capacity_)
    remove_oldest();

  Node *node = new Node(key, value);
  cache_[key] = node;
  add_to_front(node);
  size_++;
}

CacheSimulation::CacheSimulation()
    : result_val(5432), values_size(config_val("values")),
      cache(config_val("size")), hits(0), misses(0) {}

std::string CacheSimulation::name() const { return "Etc::CacheSimulation"; }

void CacheSimulation::run(int iteration_id) {
  for (int i = 0; i < 1000; i++) {
    char key_buf[32];
    snprintf(key_buf, sizeof(key_buf), "item_%d",
             Helper::next_int(values_size));
    std::string key(key_buf);

    auto value = cache.get(key);
    if (value.has_value()) {
      hits++;
      char val_buf[32];
      snprintf(val_buf, sizeof(val_buf), "updated_%d", iteration_id);
      cache.put(key, std::string(val_buf));
    } else {
      misses++;
      char val_buf[32];
      snprintf(val_buf, sizeof(val_buf), "new_%d", iteration_id);
      cache.put(key, std::string(val_buf));
    }
  }
}

uint32_t CacheSimulation::checksum() {
  uint32_t result = result_val;
  result = (result << 5) + hits;
  result = (result << 5) + misses;
  result = (result << 5) + static_cast<uint32_t>(cache.size());
  return result;
}

CalculatorAst::Parser::Parser(const std::string &input_str)
    : input(input_str), pos(0), len(input_str.size()) {
  current_char = len > 0 ? input[0] : CHAR_EOF;
}

void CalculatorAst::Parser::advance() {
  pos++;
  if (pos >= len) {
    current_char = CHAR_EOF;
  } else {
    current_char = input[pos];
  }
}

void CalculatorAst::Parser::skip_whitespace() {
  while (is_whitespace(current_char))
    advance();
}

CalculatorAst::Node CalculatorAst::Parser::parse_number() {
  int64_t v = 0;
  while (is_digit(current_char)) {
    v = v * 10 + (current_char - CHAR_ZERO);
    advance();
  }
  return Node(Number{v});
}

CalculatorAst::Node CalculatorAst::Parser::parse_variable() {
  size_t start = pos;
  while (is_letter(current_char) || is_digit(current_char))
    advance();
  std::string var_name = input.substr(start, pos - start);

  skip_whitespace();
  if (current_char == CHAR_EQUALS) {
    advance();
    auto expr = parse_expression();
    return Node(std::make_unique<Assignment>(var_name, std::move(expr)));
  }
  return Node(Variable{var_name});
}

CalculatorAst::Node CalculatorAst::Parser::parse_factor() {
  skip_whitespace();

  if (is_digit(current_char))
    return parse_number();
  if (is_letter(current_char))
    return parse_variable();
  if (current_char == CHAR_LPAREN) {
    advance();
    auto node = parse_expression();
    skip_whitespace();
    if (current_char == CHAR_RPAREN)
      advance();
    return node;
  }
  advance();
  return Node(Number{0});
}

CalculatorAst::Node CalculatorAst::Parser::parse_term() {
  auto node = parse_factor();

  while (true) {
    skip_whitespace();

    if (current_char == CHAR_STAR || current_char == CHAR_SLASH ||
        current_char == CHAR_PERCENT) {
      char op = current_char;
      advance();
      auto right = parse_factor();
      node = Node(
          std::make_unique<BinaryOp>(op, std::move(node), std::move(right)));
    } else
      break;
  }
  return node;
}

CalculatorAst::Node CalculatorAst::Parser::parse_expression() {
  auto node = parse_term();

  while (true) {
    skip_whitespace();

    if (current_char == CHAR_PLUS || current_char == CHAR_MINUS) {
      char op = current_char;
      advance();
      auto right = parse_term();
      node = Node(
          std::make_unique<BinaryOp>(op, std::move(node), std::move(right)));
    } else
      break;
  }
  return node;
}

std::vector<CalculatorAst::Node> CalculatorAst::Parser::parse() {
  expressions.clear();

  while (current_char != CHAR_EOF) {
    skip_whitespace();
    if (current_char == CHAR_EOF)
      break;

    expressions.push_back(parse_expression());

    skip_whitespace();
    while (current_char == CHAR_NEWLINE) {
      advance();
      skip_whitespace();
    }
  }
  return std::move(expressions);
}

CalculatorAst::CalculatorAst() : result_val(0), n(config_val("operations")) {}

std::string CalculatorAst::name() const { return "Calculator::Ast"; }

std::string CalculatorAst::generate_random_program(int64_t n) {
  std::ostringstream os;
  os << "v0 = 1\n";
  for (int i = 0; i < 10; i++) {
    int v = i + 1;
    os << "v" << v << " = v" << (v - 1) << " + " << v << "\n";
  }
  for (int64_t i = 0; i < n; i++) {
    int v = static_cast<int>(i + 10);
    os << "v" << v << " = v" << (v - 1) << " + ";
    switch (Helper::next_int(10)) {
    case 0:
      os << "(v" << (v - 1) << " / 3) * 4 - " << i << " / (3 + (18 - v"
         << (v - 2) << ")) % v" << (v - 3) << " + 2 * ((9 - v" << (v - 6)
         << ") * (v" << (v - 5) << " + 7))";
      break;
    case 1:
      os << "v" << (v - 1) << " + (v" << (v - 2) << " + v" << (v - 3) << ") * v"
         << (v - 4) << " - (v" << (v - 5) << " / v" << (v - 6) << ")";
      break;
    case 2:
      os << "(3789 - (((v" << (v - 7) << ")))) + 1";
      break;
    case 3:
      os << "4/2 * (1-3) + v" << (v - 9) << "/v" << (v - 5);
      break;
    case 4:
      os << "1+2+3+4+5+6+v" << (v - 1);
      break;
    case 5:
      os << "(99999 / v" << (v - 3) << ")";
      break;
    case 6:
      os << "0 + 0 - v" << (v - 8);
      break;
    case 7:
      os << "((((((((((v" << (v - 6) << ")))))))))) * 2";
      break;
    case 8:
      os << i << " * (v" << (v - 1) << "%6)%7";
      break;
    case 9:
      os << "(1)/(0-v" << (v - 5) << ") + (v" << (v - 7) << ")";
      break;
    }
    os << "\n";
  }
  return os.str();
}

void CalculatorAst::prepare() { text = generate_random_program(n); }

void CalculatorAst::run(int iteration_id) {
  (void)iteration_id;
  Parser parser(text);
  expressions = parser.parse();
  result_val += expressions.size();
  if (!expressions.empty() &&
      std::holds_alternative<std::unique_ptr<Assignment>>(
          expressions.back().data)) {
    auto &assign =
        *std::get<std::unique_ptr<Assignment>>(expressions.back().data);
    result_val += Helper::checksum(assign.var);
  }
}

uint32_t CalculatorAst::checksum() { return result_val; }

int64_t CalculatorInterpreter::Interpreter::simple_div(int64_t a, int64_t b) {
  if (b == 0)
    return 0;
  if ((a >= 0 && b > 0) || (a < 0 && b < 0))
    return a / b;
  return -(std::abs(a) / std::abs(b));
}

int64_t CalculatorInterpreter::Interpreter::simple_mod(int64_t a, int64_t b) {
  if (b == 0)
    return 0;
  return a - simple_div(a, b) * b;
}

int64_t CalculatorInterpreter::Interpreter::Evaluator::operator()(
    const CalculatorAst::Number &n) const {
  return n.value;
}

int64_t CalculatorInterpreter::Interpreter::Evaluator::operator()(
    const CalculatorAst::Variable &v) const {
  auto it = variables.find(v.name);
  return (it != variables.end()) ? it->second : 0;
}

int64_t CalculatorInterpreter::Interpreter::Evaluator::operator()(
    const std::unique_ptr<CalculatorAst::BinaryOp> &binop) const {
  int64_t left = std::visit(*this, binop->left.data);
  int64_t right = std::visit(*this, binop->right.data);
  switch (binop->op) {
  case '+':
    return left + right;
  case '-':
    return left - right;
  case '*':
    return left * right;
  case '/':
    return simple_div(left, right);
  case '%':
    return simple_mod(left, right);
  default:
    return 0;
  }
}

int64_t CalculatorInterpreter::Interpreter::Evaluator::operator()(
    const std::unique_ptr<CalculatorAst::Assignment> &assign) const {
  int64_t value = std::visit(*this, assign->expr.data);
  variables[assign->var] = value;
  return value;
}

int64_t CalculatorInterpreter::Interpreter::run(
    const std::vector<CalculatorAst::Node> &expressions) {
  int64_t result = 0;
  Evaluator evaluator(variables);
  for (const auto &expr : expressions)
    result = std::visit(evaluator, expr.data);
  return result;
}

CalculatorInterpreter::CalculatorInterpreter() : result_val(0) {
  n = config_val("operations");
}

std::string CalculatorInterpreter::name() const {
  return "Calculator::Interpreter";
}

void CalculatorInterpreter::prepare() {
  CalculatorAst ca;
  ca.n = n;
  ca.prepare();
  ca.run(0);
  ast = ca.take_ast();
}

void CalculatorInterpreter::run(int iteration_id) {
  (void)iteration_id;
  Interpreter interpreter;
  int64_t result = interpreter.run(ast);
  result_val += result;
}

uint32_t CalculatorInterpreter::checksum() { return result_val; }

std::vector<uint8_t> generate_test_data(int64_t size) {
  const char *pattern = "ABRACADABRA";
  size_t pattern_len = strlen(pattern);
  std::vector<uint8_t> data(static_cast<size_t>(size));

  for (int64_t i = 0; i < size; i++) {
    data[static_cast<size_t>(i)] = pattern[i % pattern_len];
  }

  return data;
}

BWTEncode::BWTEncode()
    : size_val(config_val("size")), bwt_result({}, 0), result_val(0) {}

std::string BWTEncode::name() const { return "Compress::BWTEncode"; }

void BWTEncode::prepare() { test_data = generate_test_data(size_val); }

BWTEncode::BWTResult
BWTEncode::bwt_transform(const std::vector<uint8_t> &input) {
  size_t n = input.size();
  if (n == 0)
    return BWTResult({}, 0);

  int32_t counts[256] = {0};
  for (uint8_t byte : input)
    counts[byte]++;

  int32_t positions[256] = {0};
  int32_t total = 0;
  for (int i = 0; i < 256; i++) {
    positions[i] = total;
    total += counts[i];
  }

  std::vector<size_t> sa(n);
  int32_t temp_counts[256] = {0};
  for (size_t i = 0; i < n; i++) {
    uint8_t byte = input[i];
    size_t pos = positions[byte] + temp_counts[byte];
    sa[pos] = i;
    temp_counts[byte]++;
  }

  if (n > 1) {
    std::vector<int32_t> rank(n, 0);
    int32_t current_rank = 0;
    uint8_t prev_char = input[sa[0]];

    for (size_t i = 0; i < n; i++) {
      size_t idx = sa[i];
      uint8_t curr_char = input[idx];
      if (curr_char != prev_char) {
        current_rank++;
        prev_char = curr_char;
      }
      rank[idx] = current_rank;
    }

    size_t k = 1;
    while (k < n) {
      std::vector<std::pair<int32_t, int32_t>> pairs(n);
      for (size_t i = 0; i < n; i++)
        pairs[i] = {rank[i], rank[(i + k) % n]};

      std::sort(sa.begin(), sa.end(), [&pairs](size_t a, size_t b) {
        const auto &pa = pairs[a];
        const auto &pb = pairs[b];
        return pa.first != pb.first ? pa.first < pb.first
                                    : pa.second < pb.second;
      });

      std::vector<int32_t> new_rank(n, 0);
      new_rank[sa[0]] = 0;
      for (size_t i = 1; i < n; i++)
        new_rank[sa[i]] =
            new_rank[sa[i - 1]] + (pairs[sa[i - 1]] != pairs[sa[i]] ? 1 : 0);

      rank = std::move(new_rank);
      k *= 2;
    }
  }

  std::vector<uint8_t> transformed(n);
  int32_t original_idx = 0;

  for (size_t i = 0; i < n; i++) {
    size_t suffix = sa[i];
    if (suffix == 0) {
      transformed[i] = input[n - 1];
      original_idx = static_cast<int32_t>(i);
    } else {
      transformed[i] = input[suffix - 1];
    }
  }

  return BWTResult(std::move(transformed), original_idx);
}

void BWTEncode::run(int iteration_id) {
  (void)iteration_id;
  bwt_result = bwt_transform(test_data);
  result_val += static_cast<uint32_t>(bwt_result.transformed.size());
}

uint32_t BWTEncode::checksum() { return result_val; }

BWTDecode::BWTDecode()
    : size_val(config_val("size")), bwt_result({}, 0), result_val(0) {}

std::string BWTDecode::name() const { return "Compress::BWTDecode"; }

void BWTDecode::prepare() {
  BWTEncode encoder;
  encoder.size_val = size_val;
  encoder.prepare();
  encoder.run(0);
  test_data = encoder.test_data;
  bwt_result = encoder.bwt_result;
}

std::vector<uint8_t>
BWTDecode::bwt_inverse(const BWTEncode::BWTResult &bwt_result) {
  const auto &bwt = bwt_result.transformed;
  size_t n = bwt.size();
  if (n == 0)
    return {};

  int32_t counts[256] = {0};
  for (uint8_t byte : bwt)
    counts[byte]++;

  int32_t positions[256] = {0};
  int32_t total = 0;
  for (int i = 0; i < 256; i++) {
    positions[i] = total;
    total += counts[i];
  }

  std::vector<size_t> next(n, 0);
  int32_t temp_counts[256] = {0};

  for (size_t i = 0; i < n; i++) {
    uint8_t byte = bwt[i];
    size_t pos = static_cast<size_t>(positions[byte] + temp_counts[byte]);
    next[pos] = i;
    temp_counts[byte]++;
  }

  std::vector<uint8_t> result(n);
  size_t idx = static_cast<size_t>(bwt_result.original_idx);

  for (size_t i = 0; i < n; i++) {
    idx = next[idx];
    result[i] = bwt[idx];
  }

  return result;
}

void BWTDecode::run(int iteration_id) {
  (void)iteration_id;
  inverted = bwt_inverse(bwt_result);
  result_val += static_cast<uint32_t>(inverted.size());
}

uint32_t BWTDecode::checksum() {
  if (inverted == test_data)
    result_val += 100000;
  return result_val;
}

HuffEncode::HuffEncode()
    : size_val(config_val("size")), encoded({}, 0, {}), result_val(0) {}

std::string HuffEncode::name() const { return "Compress::HuffEncode"; }

void HuffEncode::prepare() { test_data = generate_test_data(size_val); }

std::shared_ptr<HuffEncode::HuffmanNode>
HuffEncode::build_huffman_tree(const std::vector<int32_t> &frequencies) {
  std::vector<std::shared_ptr<HuffmanNode>> nodes;
  for (int i = 0; i < 256; i++)
    if (frequencies[i] > 0)
      nodes.push_back(std::make_shared<HuffmanNode>(frequencies[i],
                                                    static_cast<uint8_t>(i)));

  std::sort(nodes.begin(), nodes.end(), [](const auto &a, const auto &b) {
    return a->frequency < b->frequency;
  });

  if (nodes.size() == 1) {
    auto node = nodes[0];
    auto root = std::make_shared<HuffmanNode>(node->frequency, 0, false);
    root->left = node;
    root->right = std::make_shared<HuffmanNode>(0, 0);
    return root;
  }

  while (nodes.size() > 1) {
    auto left = nodes[0], right = nodes[1];
    nodes.erase(nodes.begin());
    nodes.erase(nodes.begin());
    auto parent = std::make_shared<HuffmanNode>(
        left->frequency + right->frequency, 0, false);
    parent->left = left;
    parent->right = right;
    auto pos = std::lower_bound(nodes.begin(), nodes.end(), parent,
                                [](const auto &n, const auto &p) {
                                  return n->frequency < p->frequency;
                                });
    nodes.insert(pos, parent);
  }
  return nodes[0];
}

void HuffEncode::build_huffman_codes(const std::shared_ptr<HuffmanNode> &node,
                                     int32_t code, int32_t length,
                                     HuffmanCodes &codes) {
  if (node->is_leaf) {
    if (length > 0 || node->byte_val != 0) {
      int idx = node->byte_val;
      codes.code_lengths[idx] = length;
      codes.codes[idx] = code;
    }
  } else {
    if (node->left)
      build_huffman_codes(node->left, code << 1, length + 1, codes);
    if (node->right)
      build_huffman_codes(node->right, (code << 1) | 1, length + 1, codes);
  }
}

HuffEncode::EncodedResult
HuffEncode::huffman_encode(const std::vector<uint8_t> &data,
                           const HuffmanCodes &codes,
                           const std::vector<int32_t> &frequencies) {
  std::vector<uint8_t> result;
  result.reserve(data.size() * 2);
  uint8_t current_byte = 0;
  int32_t bit_pos = 0, total_bits = 0;

  for (uint8_t byte : data) {
    int idx = byte;
    int32_t code = codes.codes[idx], length = codes.code_lengths[idx];
    for (int i = length - 1; i >= 0; i--) {
      if ((code & (1 << i)) != 0)
        current_byte |= 1 << (7 - bit_pos);
      bit_pos++;
      total_bits++;
      if (bit_pos == 8) {
        result.push_back(current_byte);
        current_byte = 0;
        bit_pos = 0;
      }
    }
  }
  if (bit_pos > 0)
    result.push_back(current_byte);
  return EncodedResult(std::move(result), total_bits, frequencies);
}

void HuffEncode::run(int iteration_id) {
  (void)iteration_id;
  std::vector<int32_t> frequencies(256, 0);
  for (uint8_t byte : test_data)
    frequencies[byte]++;
  auto tree = build_huffman_tree(frequencies);
  HuffmanCodes codes;
  build_huffman_codes(tree, 0, 0, codes);
  encoded = huffman_encode(test_data, codes, frequencies);
  result_val += static_cast<uint32_t>(encoded.data.size());
}

uint32_t HuffEncode::checksum() { return result_val; }

HuffDecode::HuffDecode()
    : size_val(config_val("size")), encoded({}, 0, {}), result_val(0) {}

std::string HuffDecode::name() const { return "Compress::HuffDecode"; }

void HuffDecode::prepare() {
  test_data = generate_test_data(size_val);
  HuffEncode encoder;
  encoder.size_val = size_val;
  encoder.prepare();
  encoder.run(0);
  encoded = encoder.encoded;
}

std::vector<uint8_t>
HuffDecode::huffman_decode(const std::vector<uint8_t> &encoded,
                           const std::shared_ptr<HuffEncode::HuffmanNode> &root,
                           int32_t bit_count) {
  std::vector<uint8_t> result(bit_count);
  size_t result_size = 0;
  const HuffEncode::HuffmanNode *current_node = root.get();
  int32_t bits_processed = 0;
  size_t byte_index = 0;

  while (bits_processed < bit_count && byte_index < encoded.size()) {
    uint8_t byte_val = encoded[byte_index++];
    for (int bit_pos = 7; bit_pos >= 0 && bits_processed < bit_count;
         bit_pos--) {
      bool bit = ((byte_val >> bit_pos) & 1) == 1;
      current_node = bit ? current_node->right.get() : current_node->left.get();
      bits_processed++;
      if (current_node->is_leaf) {
        result[result_size++] = current_node->byte_val;
        current_node = root.get();
      }
    }
  }
  result.resize(result_size);
  return result;
}

void HuffDecode::run(int iteration_id) {
  (void)iteration_id;
  auto tree = HuffEncode::build_huffman_tree(encoded.frequencies);
  decoded = huffman_decode(encoded.data, tree, encoded.bit_count);
  result_val += static_cast<uint32_t>(decoded.size());
}

uint32_t HuffDecode::checksum() {
  uint32_t res = result_val;
  if (decoded == test_data)
    res += 100000;
  return res;
}

ArithEncode::ArithEncode() { size_val = config_val("size"); }

std::string ArithEncode::name() const { return "Compress::ArithEncode"; }

void ArithEncode::prepare() { test_data = generate_test_data(size_val); }

ArithEncode::ArithFreqTable::ArithFreqTable(
    const std::vector<int32_t> &frequencies)
    : total(0), low(256, 0), high(256, 0) {
  for (int32_t f : frequencies)
    total += f;
  int32_t cum = 0;
  for (int i = 0; i < 256; i++) {
    low[i] = cum;
    cum += frequencies[i];
    high[i] = cum;
  }
}

void ArithEncode::BitOutputStream::write_bit(int32_t bit) {
  buffer = (buffer << 1) | (bit & 1);
  bit_pos++;
  bits_written++;
  if (bit_pos == 8) {
    bytes.push_back(buffer);
    buffer = 0;
    bit_pos = 0;
  }
}

std::vector<uint8_t> ArithEncode::BitOutputStream::flush() {
  if (bit_pos > 0) {
    buffer <<= (8 - bit_pos);
    bytes.push_back(buffer);
  }
  return bytes;
}

void ArithEncode::BitOutputStream::clear() {
  buffer = 0;
  bit_pos = 0;
  bytes.clear();
  bits_written = 0;
}

ArithEncode::ArithEncodedResult
ArithEncode::arith_encode(const std::vector<uint8_t> &data) {
  std::vector<int32_t> frequencies(256, 0);
  for (uint8_t byte : data)
    frequencies[byte]++;
  ArithFreqTable freq_table(frequencies);
  uint64_t low = 0, high = 0xFFFFFFFF;
  int32_t pending = 0;
  BitOutputStream output;

  for (uint8_t byte : data) {
    int32_t idx = byte;
    uint64_t range = high - low + 1;
    high = low + (range * freq_table.high[idx] / freq_table.total) - 1;
    low = low + (range * freq_table.low[idx] / freq_table.total);

    while (true) {
      if (high < 0x80000000) {
        output.write_bit(0);
        for (int i = 0; i < pending; i++)
          output.write_bit(1);
        pending = 0;
      } else if (low >= 0x80000000) {
        output.write_bit(1);
        for (int i = 0; i < pending; i++)
          output.write_bit(0);
        pending = 0;
        low -= 0x80000000;
        high -= 0x80000000;
      } else if (low >= 0x40000000 && high < 0xC0000000) {
        pending++;
        low -= 0x40000000;
        high -= 0x40000000;
      } else
        break;
      low <<= 1;
      high = (high << 1) | 1;
      high &= 0xFFFFFFFF;
    }
  }

  pending++;
  if (low < 0x40000000) {
    output.write_bit(0);
    for (int i = 0; i < pending; i++)
      output.write_bit(1);
  } else {
    output.write_bit(1);
    for (int i = 0; i < pending; i++)
      output.write_bit(0);
  }

  return ArithEncodedResult(output.flush(), output.get_bits_written(),
                            frequencies);
}

void ArithEncode::run(int iteration_id) {
  (void)iteration_id;
  encoded = arith_encode(test_data);
  result_val += static_cast<uint32_t>(encoded.data.size());
}

uint32_t ArithEncode::checksum() { return result_val; }

ArithDecode::ArithDecode() : size_val(config_val("size")), result_val(0) {}

std::string ArithDecode::name() const { return "Compress::ArithDecode"; }

void ArithDecode::prepare() {
  test_data = generate_test_data(size_val);
  ArithEncode encoder;
  encoder.size_val = size_val;
  encoder.prepare();
  encoder.run(0);
  encoded = encoder.encoded;
}

ArithDecode::BitInputStream::BitInputStream(const std::vector<uint8_t> &b)
    : bytes(b) {
  if (!bytes.empty())
    current_byte = bytes[0];
}

int32_t ArithDecode::BitInputStream::read_bit() {
  if (bit_pos == 8) {
    byte_pos++;
    bit_pos = 0;
    current_byte = byte_pos < bytes.size() ? bytes[byte_pos] : 0;
  }
  int32_t bit = (current_byte >> (7 - bit_pos)) & 1;
  bit_pos++;
  return bit;
}

std::vector<uint8_t>
ArithDecode::arith_decode(const ArithEncode::ArithEncodedResult &encoded) {
  const auto &frequencies = encoded.frequencies;
  int32_t total = 0;
  for (int32_t f : frequencies)
    total += f;
  int32_t data_size = total;

  std::array<int32_t, 256> low_table = {0}, high_table = {0};
  int32_t cum = 0;
  for (int i = 0; i < 256; i++) {
    low_table[i] = cum;
    cum += frequencies[i];
    high_table[i] = cum;
  }

  std::vector<uint8_t> result(data_size);
  BitInputStream input(encoded.data);

  uint64_t value = 0;
  for (int i = 0; i < 32; i++)
    value = (value << 1) | input.read_bit();

  uint64_t low = 0, high = 0xFFFFFFFF;

  for (int32_t j = 0; j < data_size; j++) {
    uint64_t range = high - low + 1;
    uint64_t scaled = ((value - low + 1) * total - 1) / range;

    auto it = std::upper_bound(high_table.begin(), high_table.end(), scaled);
    uint8_t symbol = std::distance(high_table.begin(), it);
    result[j] = symbol;

    high = low + (range * high_table[symbol] / total) - 1;
    low = low + (range * low_table[symbol] / total);

    while (true) {
      if (high < 0x80000000) {
      } else if (low >= 0x80000000) {
        value -= 0x80000000;
        low -= 0x80000000;
        high -= 0x80000000;
      } else if (low >= 0x40000000 && high < 0xC0000000) {
        value -= 0x40000000;
        low -= 0x40000000;
        high -= 0x40000000;
      } else
        break;
      low <<= 1;
      high = (high << 1) | 1;
      value = (value << 1) | input.read_bit();
    }
  }
  return result;
}

void ArithDecode::run(int iteration_id) {
  (void)iteration_id;
  decoded = arith_decode(encoded);
  result_val += static_cast<uint32_t>(decoded.size());
}

uint32_t ArithDecode::checksum() {
  if (decoded == test_data)
    result_val += 100000;
  return result_val;
}

LZWEncode::LZWEncode() : size_val(config_val("size")), result_val(0) {}

std::string LZWEncode::name() const { return "Compress::LZWEncode"; }

void LZWEncode::prepare() { test_data = generate_test_data(size_val); }

LZWEncode::LZWResult LZWEncode::lzw_encode(const std::vector<uint8_t> &input) {
  if (input.empty())
    return LZWResult();

  std::unordered_map<std::string, int32_t> dict;
  dict.reserve(4096);
  for (int i = 0; i < 256; i++)
    dict[std::string(1, static_cast<char>(i))] = i;

  int32_t next_code = 256;
  std::vector<uint8_t> result;
  result.reserve(input.size() * 2);
  std::string current(1, static_cast<char>(input[0]));

  for (size_t i = 1; i < input.size(); i++) {
    char next_char(static_cast<char>(input[i]));
    std::string new_str = current + next_char;
    if (dict.find(new_str) != dict.end()) {
      current = new_str;
    } else {
      int32_t code = dict[current];
      result.push_back((code >> 8) & 0xFF);
      result.push_back(code & 0xFF);
      dict[new_str] = next_code++;
      current = std::string(1, next_char);
    }
  }

  int32_t code = dict[current];
  result.push_back((code >> 8) & 0xFF);
  result.push_back(code & 0xFF);
  return LZWResult(result, next_code);
}

void LZWEncode::run(int iteration_id) {
  (void)iteration_id;
  encoded = lzw_encode(test_data);
  result_val += static_cast<uint32_t>(encoded.data.size());
}

uint32_t LZWEncode::checksum() { return result_val; }

LZWDecode::LZWDecode() : size_val(config_val("size")), result_val(0) {}

std::string LZWDecode::name() const { return "Compress::LZWDecode"; }

void LZWDecode::prepare() {
  test_data = generate_test_data(size_val);
  LZWEncode encoder;
  encoder.size_val = size_val;
  encoder.prepare();
  encoder.run(0);
  encoded = encoder.encoded;
}

std::vector<uint8_t>
LZWDecode::lzw_decode(const LZWEncode::LZWResult &encoded) {
  if (encoded.data.empty())
    return {};

  std::vector<std::string> dict;
  dict.reserve(4096);
  for (int i = 0; i < 256; i++)
    dict.emplace_back(1, static_cast<char>(i));

  std::vector<uint8_t> result;
  result.reserve(encoded.data.size() * 2);
  const auto &data = encoded.data;
  size_t pos = 0;

  uint16_t high = data[pos], low = data[pos + 1];
  int32_t old_code = (high << 8) | low;
  pos += 2;

  const std::string &old_str = dict[old_code];
  result.insert(result.end(), old_str.begin(), old_str.end());
  int32_t next_code = 256;

  while (pos < data.size()) {
    high = data[pos];
    low = data[pos + 1];
    int32_t new_code = (high << 8) | low;
    pos += 2;

    std::string new_str;
    if (new_code < static_cast<int32_t>(dict.size())) {
      new_str = dict[new_code];
    } else if (new_code == next_code) {
      new_str = dict[old_code] + dict[old_code][0];
    } else {
      return {};
    }

    result.insert(result.end(), new_str.begin(), new_str.end());
    dict.emplace_back(dict[old_code] + new_str[0]);
    next_code++;
    old_code = new_code;
  }
  return result;
}

void LZWDecode::run(int iteration_id) {
  (void)iteration_id;
  decoded = lzw_decode(encoded);
  result_val += static_cast<uint32_t>(decoded.size());
}

uint32_t LZWDecode::checksum() {
  if (decoded == test_data)
    result_val += 100000;
  return result_val;
}

namespace csvm {

inline std::string_view make_view(std::string_view data, size_t b, size_t e,
                                  bool quoted) {
  (void)quoted;
  return data.substr(b, e - b);
}

static std::string_view trimmed(std::string_view s) {
  size_t b = 0, e = s.size();
  auto is_ws = [](char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
  };
  while (b < e && is_ws(s[b]))
    ++b;
  while (e > b && is_ws(s[e - 1]))
    --e;
  return s.substr(b, e - b);
}

static size_t parse_record(std::string_view data, size_t pos,
                           std::vector<std::string_view> &out, char delim = ',',
                           char quote = '"') {
  out.clear();
  const size_t n = data.size();
  size_t field_begin = pos;
  bool in_quotes = false;
  bool field_quoted = false;

  while (pos < n) {
    char c = data[pos];

    if (in_quotes) {
      if (c == quote) {
        if (pos + 1 < n && data[pos + 1] == quote) {
          pos += 2;
          continue;
        }
        in_quotes = false;
        ++pos;
        continue;
      }
      ++pos;
      continue;
    }

    if (c == quote && pos == field_begin) {
      in_quotes = true;
      field_quoted = true;
      ++pos;
      field_begin = pos;
      continue;
    }
    if (c == delim) {
      out.push_back(make_view(data, field_begin, pos, field_quoted));
      field_quoted = false;
      ++pos;
      field_begin = pos;
      continue;
    }
    if (c == '\n') {
      out.push_back(make_view(data, field_begin, pos, field_quoted));
      return pos + 1;
    }
    if (c == '\r' && pos + 1 < n && data[pos + 1] == '\n') {
      out.push_back(make_view(data, field_begin, pos, field_quoted));
      return pos + 2;
    }
    ++pos;
  }

  out.push_back(make_view(data, field_begin, pos, field_quoted));
  return pos;
}

class Parser {
public:
  Parser(std::string_view data) : data_(data) {}

  struct Row {
    std::vector<std::string_view> fields;

    std::vector<std::string_view> cells(size_t i0, size_t i1, size_t i2) const {
      std::vector<std::string_view> r(3);
      if (i0 < fields.size())
        r[0] = fields[i0];
      if (i1 < fields.size())
        r[1] = fields[i1];
      if (i2 < fields.size())
        r[2] = fields[i2];
      return r;
    }
  };

  class iterator {
  public:
    iterator(const Parser *p, size_t pos) : p_(p), pos_(pos) {
      if (pos_ < p_->data_.size()) {
        pos_ = parse_record(p_->data_, pos_, row_.fields);
        has_ = true;
      } else {
        has_ = false;
      }
    }
    bool operator!=(const iterator &o) const { return has_ != o.has_; }
    const Row &operator*() const { return row_; }
    iterator &operator++() {
      if (pos_ < p_->data_.size()) {
        pos_ = parse_record(p_->data_, pos_, row_.fields);
        has_ = true;
      } else {
        has_ = false;
      }
      return *this;
    }

  private:
    const Parser *p_;
    size_t pos_;
    Row row_;
    bool has_ = false;
  };

  iterator begin() const { return iterator(this, 0); }
  iterator end() const { return iterator(this, data_.size()); }

private:
  std::string_view data_;
};

} // namespace csvm

CsvParse::CsvParse() : rows(config_val("rows")), result_val(0) {}

std::string CsvParse::name() const { return "CSV::Parse"; }

void CsvParse::prepare() {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(10);

  for (int i = 0; i < rows; i++) {
    char c = 'A' + (i % 26);
    double x = Helper::next_float();
    double z = Helper::next_float();
    double y = Helper::next_float();
    ss << '"' << "point " << c << "\\n, \"\"" << (i % 100) << "\"\"\"" << ','
       << x << ',' << ',' << z << ',' << '"' << '['
       << (i % 2 == 0 ? "true" : "false") << "\\n, " << (i % 100) << ']' << '"'
       << ',' << y << '\n';
  }

  data = ss.str();
}

void CsvParse::run(int iteration_id) {
  (void)iteration_id;
  csvm::Parser parser(data);

  std::vector<Point> points;

  for (const auto &row : parser) {
    auto cells = row.cells(1, 3, 5);

    double x = std::stod(std::string(csvm::trimmed(cells[0])));
    double z = std::stod(std::string(csvm::trimmed(cells[2])));
    double y = std::stod(std::string(csvm::trimmed(cells[1])));

    points.emplace_back(x, y, z);
  }

  if (points.empty())
    return;

  double x_sum = 0.0, y_sum = 0.0, z_sum = 0.0;
  for (const auto &p : points) {
    x_sum += p.x;
    y_sum += p.y;
    z_sum += p.z;
  }

  double len = static_cast<double>(points.size());
  double x_avg = x_sum / len;
  double y_avg = y_sum / len;
  double z_avg = z_sum / len;

  result_val += Helper::checksum_f64(x_avg) + Helper::checksum_f64(y_avg) +
                Helper::checksum_f64(z_avg);
}

uint32_t CsvParse::checksum() { return result_val + Helper::checksum(data); }

std::vector<std::pair<std::string, std::string>>
generate_pair_strings(int64_t n, int64_t m) {
  std::vector<std::pair<std::string, std::string>> pairs;
  pairs.reserve(n);

  for (int64_t i = 0; i < n; ++i) {
    int len1 = Helper::next_int(m) + 4;
    int len2 = Helper::next_int(m) + 4;

    std::string str1, str2;
    str1.reserve(len1);
    str2.reserve(len2);

    for (int j = 0; j < len1; ++j)
      str1 += 'a' + Helper::next_int(10);
    for (int j = 0; j < len2; ++j)
      str2 += 'a' + Helper::next_int(10);

    pairs.emplace_back(std::move(str1), std::move(str2));
  }
  return pairs;
}

Jaro::Jaro()
    : count(config_val("count")), size(config_val("size")), result_val(0) {}

void Jaro::prepare() { pairs = generate_pair_strings(count, size); }

double Jaro::jaro(const std::string &s1, const std::string &s2) {
  size_t len1 = s1.size();
  size_t len2 = s2.size();

  if (len1 == 0 || len2 == 0)
    return 0.0;

  size_t match_dist = std::max(len1, len2) / 2;
  if (match_dist > 0)
    match_dist -= 1;

  std::vector<bool> s1_matches(len1, false);
  std::vector<bool> s2_matches(len2, false);

  int matches = 0;
  for (size_t i = 0; i < len1; ++i) {
    size_t start = i > match_dist ? i - match_dist : 0;
    size_t end = std::min<size_t>(len2 - 1, i + match_dist);
    for (size_t j = start; j <= end; ++j) {
      if (!s2_matches[j] && s1[i] == s2[j]) {
        s1_matches[i] = true;
        s2_matches[j] = true;
        matches++;
        break;
      }
    }
  }

  if (matches == 0)
    return 0.0;

  int transpositions = 0;
  size_t k = 0;
  for (size_t i = 0; i < len1; ++i) {
    if (s1_matches[i]) {
      while (k < len2 && !s2_matches[k])
        k++;
      if (k < len2) {
        if (s1[i] != s2[k])
          transpositions++;
        k++;
      }
    }
  }
  transpositions /= 2;

  double m = static_cast<double>(matches);
  volatile double jaro = (m / len1 + m / len2 + (m - transpositions) / m) / 3.0;
  return jaro;
}

void Jaro::run(int iteration_id) {
  (void)iteration_id;
  for (const auto &pair : pairs)
    result_val += static_cast<uint32_t>(jaro(pair.first, pair.second) * 1000);
}

uint32_t Jaro::checksum() { return result_val; }

std::string Jaro::name() const { return "Distance::Jaro"; }

NGram::NGram()
    : count(config_val("count")), size(config_val("size")), result_val(0) {}

void NGram::prepare() { pairs = generate_pair_strings(count, size); }

double NGram::ngram(const std::string &_s1, const std::string &_s2) {
  const auto &s1 = _s1;
  const auto &s2 = _s2;

  std::unordered_map<uint32_t, int> grams1;
  grams1.reserve(s1.size());

  for (size_t i = 0; i <= s1.size() - 4; ++i) {
    uint32_t gram = (static_cast<uint8_t>(s1[i]) << 24) |
                    (static_cast<uint8_t>(s1[i + 1]) << 16) |
                    (static_cast<uint8_t>(s1[i + 2]) << 8) |
                    static_cast<uint8_t>(s1[i + 3]);
    auto [it, inserted] = grams1.try_emplace(gram, 0);
    it->second++;
  }

  std::unordered_map<uint32_t, int> grams2;
  grams2.reserve(s2.size());
  int intersection = 0;

  for (size_t i = 0; i <= s2.size() - 4; ++i) {
    uint32_t gram = (static_cast<uint8_t>(s2[i]) << 24) |
                    (static_cast<uint8_t>(s2[i + 1]) << 16) |
                    (static_cast<uint8_t>(s2[i + 2]) << 8) |
                    static_cast<uint8_t>(s2[i + 3]);
    auto [it2, inserted2] = grams2.try_emplace(gram, 0);
    it2->second++;

    auto it1 = grams1.find(gram);
    if (it1 != grams1.end() && it2->second <= it1->second)
      intersection++;
  }

  size_t total = grams1.size() + grams2.size();
  return total > 0 ? static_cast<double>(intersection) / total : 0.0;
}

void NGram::run(int iteration_id) {
  (void)iteration_id;
  for (const auto &pair : pairs)
    result_val += static_cast<uint32_t>(ngram(pair.first, pair.second) * 1000);
}

uint32_t NGram::checksum() { return result_val; }

std::string NGram::name() const { return "Distance::NGram"; }

Fannkuchredux::Fannkuchredux() : n(config_val("n")), result_val(0) {}

std::string Fannkuchredux::name() const { return "CLBG::Fannkuchredux"; }

std::pair<int, int> Fannkuchredux::fannkuchredux(int n) {
  int perm1[32];
  int perm[32];
  int count[32];

  if (n > 32)
    n = 32;

  std::iota(perm1, perm1 + n, 0);

  int maxFlipsCount = 0, permCount = 0, checksum = 0;
  int r = n;

  while (true) {
    while (r > 1) {
      count[r - 1] = r;
      r--;
    }

    std::copy(perm1, perm1 + n, perm);

    int flipsCount = 0;
    int k = perm[0];

    while (k != 0) {
      std::reverse(perm, perm + k + 1);
      flipsCount++;
      k = perm[0];
    }

    maxFlipsCount = std::max(maxFlipsCount, flipsCount);
    checksum += (permCount % 2 == 0) ? flipsCount : -flipsCount;

    while (true) {
      if (r == n)
        return {checksum, maxFlipsCount};

      std::rotate(perm1, perm1 + 1, perm1 + r + 1);

      count[r]--;
      if (count[r] > 0)
        break;
      r++;
    }
    permCount++;
  }
}

void Fannkuchredux::run(int iteration_id) {
  (void)iteration_id;
  auto [a, b] = fannkuchredux(static_cast<int>(n));
  result_val += a * 100 + b;
}

uint32_t Fannkuchredux::checksum() { return result_val; }

GameOfLife::Cell::Cell(bool alive) : alive(alive), next_state(false) {}

void GameOfLife::Cell::add_neighbor(Cell *cell) { neighbors.push_back(cell); }

void GameOfLife::Cell::compute_next_state() {
  int alive_neighbors = 0;
  for (Cell *neighbor : neighbors)
    if (neighbor->alive)
      alive_neighbors++;

  if (alive)
    next_state = (alive_neighbors == 2 || alive_neighbors == 3);
  else
    next_state = (alive_neighbors == 3);
}

void GameOfLife::Cell::update() { alive = next_state; }

void GameOfLife::Cell::set_alive(bool state) { alive = state; }

bool GameOfLife::Cell::is_alive() const { return alive; }

GameOfLife::Grid::Grid(int w, int h) : width(w), height(h) {
  cells.resize(height);
  for (int y = 0; y < height; ++y) {
    cells[y].reserve(width);
    for (int x = 0; x < width; ++x) {
      cells[y].emplace_back(false);
    }
  }
  link_neighbors();
}

void GameOfLife::Grid::link_neighbors() {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      Cell &cell = cells[y][x];
      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          if (dx == 0 && dy == 0)
            continue;
          int ny = (y + dy + height) % height;
          int nx = (x + dx + width) % width;
          cell.add_neighbor(&cells[ny][nx]);
        }
      }
    }
  }
}

void GameOfLife::Grid::next_generation() {
  for (auto &row : cells)
    for (auto &cell : row)
      cell.compute_next_state();
  for (auto &row : cells)
    for (auto &cell : row)
      cell.update();
}

std::vector<std::vector<GameOfLife::Cell>> &GameOfLife::Grid::get_cells() {
  return cells;
}

int GameOfLife::Grid::count_alive() const {
  int count = 0;
  for (const auto &row : cells)
    for (const auto &cell : row)
      if (cell.is_alive())
        count++;
  return count;
}

uint32_t GameOfLife::Grid::compute_hash() const {
  constexpr uint32_t FNV_OFFSET_BASIS = 2166136261UL;
  constexpr uint32_t FNV_PRIME = 16777619UL;
  uint32_t hash = FNV_OFFSET_BASIS;
  for (const auto &row : cells)
    for (const auto &cell : row) {
      uint32_t alive = cell.is_alive() ? 1U : 0U;
      hash = (hash ^ alive) * FNV_PRIME;
    }
  return hash;
}

GameOfLife::GameOfLife()
    : width(static_cast<int32_t>(config_val("w"))),
      height(static_cast<int32_t>(config_val("h"))), grid(width, height) {}

std::string GameOfLife::name() const { return "Etc::GameOfLife"; }

void GameOfLife::prepare() {
  for (auto &row : grid.get_cells())
    for (auto &cell : row)
      if (Helper::next_float(1.0) < 0.1)
        cell.set_alive(true);
}

void GameOfLife::run(int iteration_id) {
  (void)iteration_id;
  grid.next_generation();
}

uint32_t GameOfLife::checksum() {
  uint32_t alive = static_cast<uint32_t>(grid.count_alive());
  return grid.compute_hash() + alive;
}

GraphPathBenchmark::Graph::Graph(int vertices, int jumps, int jump_len)
    : vertices(vertices), jumps(jumps), jump_len(jump_len), adj(vertices) {}

void GraphPathBenchmark::Graph::add_edge(int u, int v) {
  adj[u].push_back(v);
  adj[v].push_back(u);
}

void GraphPathBenchmark::Graph::generate_random() {
  for (int i = 1; i < vertices; i++) {
    add_edge(i, i - 1);
  }

  for (int v = 0; v < vertices; v++) {
    int num_jumps = Helper::next_int(jumps);
    for (int j = 0; j < num_jumps; j++) {
      int offset = Helper::next_int(jump_len) - jump_len / 2;
      int u = v + offset;

      if (u >= 0 && u < vertices && u != v) {
        add_edge(v, u);
      }
    }
  }
}

GraphPathBenchmark::GraphPathBenchmark() : result_val(0) {}

void GraphPathBenchmark::prepare() {
  int vertices = static_cast<int>(config_val("vertices"));
  int jumps = static_cast<int>(config_val("jumps"));
  int jump_len = static_cast<int>(config_val("jump_len"));
  graph = std::make_unique<Graph>(vertices, jumps, jump_len);
  graph->generate_random();
}

void GraphPathBenchmark::run(int iteration_id) {
  (void)iteration_id;
  result_val += test();
}

uint32_t GraphPathBenchmark::checksum() { return result_val; }

std::string GraphPathBFS::name() const { return "Graph::BFS"; }

int GraphPathBFS::bfs_shortest_path(int start, int target) {
  if (start == target)
    return 0;

  std::vector<uint8_t> visited(graph->vertices, 0);
  std::queue<std::pair<int, int>> queue;

  visited[start] = 1;
  queue.push({start, 0});

  while (!queue.empty()) {
    auto [v, dist] = queue.front();
    queue.pop();

    for (int neighbor : graph->adj[v]) {
      if (neighbor == target)
        return dist + 1;
      if (visited[neighbor] == 0) {
        visited[neighbor] = 1;
        queue.push({neighbor, dist + 1});
      }
    }
  }
  return -1;
}

int64_t GraphPathBFS::test() {
  return bfs_shortest_path(0, graph->vertices - 1);
}

std::string GraphPathDFS::name() const { return "Graph::DFS"; }

int GraphPathDFS::dfs_shortest_path(int start, int target) {
  if (start == target)
    return 0;

  std::vector<uint8_t> visited(graph->vertices, 0);
  std::stack<std::pair<int, int>> stack;
  int best_path = INT_MAX;

  stack.push({start, 0});

  while (!stack.empty()) {
    auto [v, dist] = stack.top();
    stack.pop();

    if (visited[v] == 1 || dist >= best_path)
      continue;
    visited[v] = 1;

    for (int neighbor : graph->adj[v]) {
      if (neighbor == target) {
        if (dist + 1 < best_path)
          best_path = dist + 1;
      } else if (visited[neighbor] == 0) {
        stack.push({neighbor, dist + 1});
      }
    }
  }
  return (best_path == INT_MAX) ? -1 : best_path;
}

int64_t GraphPathDFS::test() {
  return dfs_shortest_path(0, graph->vertices - 1);
}

std::string GraphPathAStar::name() const { return "Graph::AStar"; }

int GraphPathAStar::astar_shortest_path(int start, int target) {
  if (start == target)
    return 0;

  int n = graph->vertices;

  std::vector<int> g_score(n, INT_MAX);
  std::vector<int> best_f(n, INT_MAX);

  g_score[start] = 0;
  int f_start = heuristic(start, target);
  best_f[start] = f_start;

  using QueueType =
      std::priority_queue<Node, std::vector<Node>, std::greater<Node>>;
  QueueType open_set;
  open_set.push({start, f_start});

  while (!open_set.empty()) {
    Node current = open_set.top();
    open_set.pop();

    int current_vertex = current.vertex;

    if (current_vertex == target)
      return g_score[current_vertex];

    for (int neighbor : graph->adj[current_vertex]) {
      int tentative_g = g_score[current_vertex] + 1;

      if (tentative_g < g_score[neighbor]) {
        g_score[neighbor] = tentative_g;
        int f_new = tentative_g + heuristic(neighbor, target);

        if (f_new < best_f[neighbor]) {
          best_f[neighbor] = f_new;
          open_set.push({neighbor, f_new});
        }
      }
    }
  }

  return -1;
}

int64_t GraphPathAStar::test() {
  return astar_shortest_path(0, graph->vertices - 1);
}

static cJSON* config_root = nullptr;

thread_local int64_t Helper::last = 42;

void load_config(const std::string &json_str) {
    if (config_root) {
        cJSON_Delete(config_root);
        config_root = nullptr;
    }

    config_root = cJSON_Parse(json_str.c_str());
    if (!config_root) {
        std::cerr << "Error parsing JSON config: "
                  << (cJSON_GetErrorPtr() ? cJSON_GetErrorPtr() : "unknown")
                  << std::endl;
    }
}

bool config_has(const std::string &key) {
    if (!config_root) return false;

    if (cJSON_IsObject(config_root)) {
        return cJSON_GetObjectItemCaseSensitive(config_root, key.c_str()) != nullptr;
    }

    if (cJSON_IsArray(config_root)) {
        cJSON* item = nullptr;
        cJSON_ArrayForEach(item, config_root) {
            cJSON* name = cJSON_GetObjectItemCaseSensitive(item, "name");
            if (cJSON_IsString(name) && std::strcmp(name->valuestring, key.c_str()) == 0) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::string> config_keys() {
    std::vector<std::string> keys;
    if (!config_root) return keys;

    if (cJSON_IsObject(config_root)) {
        cJSON* item = nullptr;
        cJSON_ArrayForEach(item, config_root) {
            if (item->string) keys.emplace_back(item->string);
        }
    } else if (cJSON_IsArray(config_root)) {
        cJSON* item = nullptr;
        cJSON_ArrayForEach(item, config_root) {
            cJSON* name = cJSON_GetObjectItemCaseSensitive(item, "name");
            if (cJSON_IsString(name)) keys.emplace_back(name->valuestring);
        }
    }
    return keys;
}

int64_t Helper::config_i64(const std::string &class_name,
                           const std::string &field_name) {
    return ::config_i64(class_name, field_name);
}

std::string Helper::config_s(const std::string &class_name,
                             const std::string &field_name) {
    return ::config_s(class_name, field_name);
}

static cJSON* find_in_config(const std::string &key) {
    if (!config_root) return nullptr;

    if (cJSON_IsObject(config_root)) {
        return cJSON_GetObjectItemCaseSensitive(config_root, key.c_str());
    }

    if (cJSON_IsArray(config_root)) {
        cJSON* item = nullptr;
        cJSON_ArrayForEach(item, config_root) {
            cJSON* name = cJSON_GetObjectItemCaseSensitive(item, "name");
            if (cJSON_IsString(name) && std::strcmp(name->valuestring, key.c_str()) == 0) {
                return item;
            }
        }
    }
    return nullptr;
}

int64_t config_i64(const std::string &key, const std::string &field) {
    cJSON* class_obj = find_in_config(key);
    if (!class_obj || !cJSON_IsObject(class_obj)) return 0;

    cJSON* field_elem = cJSON_GetObjectItemCaseSensitive(class_obj, field.c_str());
    if (!field_elem) return 0;

    if (cJSON_IsNumber(field_elem)) {
        // cJSON хранит все числа как double; для int64 используем значение как есть
        return static_cast<int64_t>(field_elem->valuedouble);
    }

    if (cJSON_IsString(field_elem)) {
        try {
            return std::stoll(field_elem->valuestring);
        } catch (...) {
            return 0;
        }
    }

    return 0;
}

std::string config_s(const std::string &key, const std::string &field) {
    cJSON* class_obj = find_in_config(key);
    if (!class_obj || !cJSON_IsObject(class_obj)) {
        std::cerr << "Config not found for " << key << std::endl;
        return "";
    }

    cJSON* field_elem = cJSON_GetObjectItemCaseSensitive(class_obj, field.c_str());
    if (!field_elem) {
        std::cerr << "Field not found: " << field << std::endl;
        return "";
    }

    if (cJSON_IsString(field_elem)) {
        return std::string(field_elem->valuestring);
    }
    return "";
}

void Helper::reset() { last = 42; }

int32_t Helper::next_int(int32_t max) {
    last = (last * IA + IC) % IM;
    return static_cast<int32_t>((last * max) / IM);
}

int32_t Helper::next_int(int32_t from, int32_t to) {
    return next_int(to - from + 1) + from;
}

double Helper::next_float(double max) {
    last = (last * IA + IC) % IM;
    return max * static_cast<double>(last) / IM;
}

uint32_t Helper::checksum(const std::string &v) {
    uint32_t hash = 5381;
    for (char c : v) {
        hash = ((hash << 5) + hash) + static_cast<uint8_t>(c);
    }
    return hash;
}

uint32_t Helper::checksum(const std::vector<uint8_t> &v) {
    uint32_t hash = 5381;
    for (uint8_t byte : v) {
        hash = ((hash << 5) + hash) + byte;
    }
    return hash;
}

uint32_t Helper::checksum_f64(double v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(7) << v;
    return Helper::checksum(oss.str());
}

double custom_round(double value, int32_t precision) {
    if (std::isnan(value) || std::isinf(value)) {
        return value;
    }

    double factor = std::pow(10.0, precision);
    double scaled = value * factor;

    double fraction = scaled - std::floor(scaled);

    if (std::abs(fraction) < 0.5) {
        return std::floor(scaled) / factor;
    } else if (std::abs(fraction) > 0.5) {
        return std::ceil(scaled) / factor;
    } else {
        return (std::round(scaled / 2.0) * 2.0) / factor;
    }
}

std::string to_lower(const std::string &str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

extern "C" {
// https://github.com/DaveGamble/cJSON/blob/master/cJSON.c
// ------------------------------------------------------------------------------------------
/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* cJSON */
/* JSON parser in C. */

/* disable warnings about old C89 functions in MSVC */
#if !defined(_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif
#if defined(_MSC_VER)
#pragma warning (push)
/* disable warning about single line comments in system headers */
#pragma warning (disable : 4001)
#endif

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <float.h>

#ifdef ENABLE_LOCALES
#include <locale.h>
#endif

#if defined(_MSC_VER)
#pragma warning (pop)
#endif
#ifdef __GNUC__
#pragma GCC visibility pop
#endif

/* define our own boolean type */
#ifdef true
#undef true
#endif
#define true ((cJSON_bool)1)

#ifdef false
#undef false
#endif
#define false ((cJSON_bool)0)

/* define isnan and isinf for ANSI C, if in C99 or above, isnan and isinf has been defined in math.h */
#ifndef isinf
#define isinf(d) (isnan((d - d)) && !isnan(d))
#endif
#ifndef isnan
#define isnan(d) (d != d)
#endif

#ifndef NAN
#ifdef _WIN32
#define NAN sqrt(-1.0)
#else
#define NAN 0.0/0.0
#endif
#endif

typedef struct {
    const unsigned char *json;
    size_t position;
} error;
static error global_error = { NULL, 0 };

CJSON_PUBLIC(const char *) cJSON_GetErrorPtr(void)
{
    return (const char*) (global_error.json + global_error.position);
}

CJSON_PUBLIC(char *) cJSON_GetStringValue(const cJSON * const item)
{
    if (!cJSON_IsString(item))
    {
        return NULL;
    }

    return item->valuestring;
}

CJSON_PUBLIC(double) cJSON_GetNumberValue(const cJSON * const item)
{
    if (!cJSON_IsNumber(item))
    {
        return (double) NAN;
    }

    return item->valuedouble;
}

/* This is a safeguard to prevent copy-pasters from using incompatible C and header files */
#if (CJSON_VERSION_MAJOR != 1) || (CJSON_VERSION_MINOR != 7) || (CJSON_VERSION_PATCH != 19)
    #error cJSON.h and cJSON.c have different versions. Make sure that both have the same.
#endif

CJSON_PUBLIC(const char*) cJSON_Version(void)
{
    static char version[15];
    sprintf(version, "%i.%i.%i", CJSON_VERSION_MAJOR, CJSON_VERSION_MINOR, CJSON_VERSION_PATCH);

    return version;
}

/* Case insensitive string comparison, doesn't consider two NULL pointers equal though */
static int case_insensitive_strcmp(const unsigned char *string1, const unsigned char *string2)
{
    if ((string1 == NULL) || (string2 == NULL))
    {
        return 1;
    }

    if (string1 == string2)
    {
        return 0;
    }

    for(; tolower(*string1) == tolower(*string2); (void)string1++, string2++)
    {
        if (*string1 == '\0')
        {
            return 0;
        }
    }

    return tolower(*string1) - tolower(*string2);
}

typedef struct internal_hooks
{
    void *(CJSON_CDECL *allocate)(size_t size);
    void (CJSON_CDECL *deallocate)(void *pointer);
    void *(CJSON_CDECL *reallocate)(void *pointer, size_t size);
} internal_hooks;

#if defined(_MSC_VER)
/* work around MSVC error C2322: '...' address of dllimport '...' is not static */
static void * CJSON_CDECL internal_malloc(size_t size)
{
    return malloc(size);
}
static void CJSON_CDECL internal_free(void *pointer)
{
    free(pointer);
}
static void * CJSON_CDECL internal_realloc(void *pointer, size_t size)
{
    return realloc(pointer, size);
}
#else
#define internal_malloc malloc
#define internal_free free
#define internal_realloc realloc
#endif

/* strlen of character literals resolved at compile time */
#define static_strlen(string_literal) (sizeof(string_literal) - sizeof(""))

static internal_hooks global_hooks = { internal_malloc, internal_free, internal_realloc };

static unsigned char* cJSON_strdup(const unsigned char* string, const internal_hooks * const hooks)
{
    size_t length = 0;
    unsigned char *copy = NULL;

    if (string == NULL)
    {
        return NULL;
    }

    length = strlen((const char*)string) + sizeof("");
    copy = (unsigned char*)hooks->allocate(length);
    if (copy == NULL)
    {
        return NULL;
    }
    memcpy(copy, string, length);

    return copy;
}

CJSON_PUBLIC(void) cJSON_InitHooks(cJSON_Hooks* hooks)
{
    if (hooks == NULL)
    {
        /* Reset hooks */
        global_hooks.allocate = malloc;
        global_hooks.deallocate = free;
        global_hooks.reallocate = realloc;
        return;
    }

    global_hooks.allocate = malloc;
    if (hooks->malloc_fn != NULL)
    {
        global_hooks.allocate = hooks->malloc_fn;
    }

    global_hooks.deallocate = free;
    if (hooks->free_fn != NULL)
    {
        global_hooks.deallocate = hooks->free_fn;
    }

    /* use realloc only if both free and malloc are used */
    global_hooks.reallocate = NULL;
    if ((global_hooks.allocate == malloc) && (global_hooks.deallocate == free))
    {
        global_hooks.reallocate = realloc;
    }
}

/* Internal constructor. */
static cJSON *cJSON_New_Item(const internal_hooks * const hooks)
{
    cJSON* node = (cJSON*)hooks->allocate(sizeof(cJSON));
    if (node)
    {
        memset(node, '\0', sizeof(cJSON));
    }

    return node;
}

/* Delete a cJSON structure. */
CJSON_PUBLIC(void) cJSON_Delete(cJSON *item)
{
    cJSON *next = NULL;
    while (item != NULL)
    {
        next = item->next;
        if (!(item->type & cJSON_IsReference) && (item->child != NULL))
        {
            cJSON_Delete(item->child);
        }
        if (!(item->type & cJSON_IsReference) && (item->valuestring != NULL))
        {
            global_hooks.deallocate(item->valuestring);
            item->valuestring = NULL;
        }
        if (!(item->type & cJSON_StringIsConst) && (item->string != NULL))
        {
            global_hooks.deallocate(item->string);
            item->string = NULL;
        }
        global_hooks.deallocate(item);
        item = next;
    }
}

/* get the decimal point character of the current locale */
static unsigned char get_decimal_point(void)
{
#ifdef ENABLE_LOCALES
    struct lconv *lconv = localeconv();
    return (unsigned char) lconv->decimal_point[0];
#else
    return '.';
#endif
}

typedef struct
{
    const unsigned char *content;
    size_t length;
    size_t offset;
    size_t depth; /* How deeply nested (in arrays/objects) is the input at the current offset. */
    internal_hooks hooks;
} parse_buffer;

/* check if the given size is left to read in a given parse buffer (starting with 1) */
#define can_read(buffer, size) ((buffer != NULL) && (((buffer)->offset + size) <= (buffer)->length))
/* check if the buffer can be accessed at the given index (starting with 0) */
#define can_access_at_index(buffer, index) ((buffer != NULL) && (((buffer)->offset + index) < (buffer)->length))
#define cannot_access_at_index(buffer, index) (!can_access_at_index(buffer, index))
/* get a pointer to the buffer at the position */
#define buffer_at_offset(buffer) ((buffer)->content + (buffer)->offset)

/* Parse the input text to generate a number, and populate the result into item. */
static cJSON_bool parse_number(cJSON * const item, parse_buffer * const input_buffer)
{
    double number = 0;
    unsigned char *after_end = NULL;
    unsigned char *number_c_string;
    unsigned char decimal_point = get_decimal_point();
    size_t i = 0;
    size_t number_string_length = 0;
    cJSON_bool has_decimal_point = false;

    if ((input_buffer == NULL) || (input_buffer->content == NULL))
    {
        return false;
    }

    /* copy the number into a temporary buffer and replace '.' with the decimal point
     * of the current locale (for strtod)
     * This also takes care of '\0' not necessarily being available for marking the end of the input */
    for (i = 0; can_access_at_index(input_buffer, i); i++)
    {
        switch (buffer_at_offset(input_buffer)[i])
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '+':
            case '-':
            case 'e':
            case 'E':
                number_string_length++;
                break;

            case '.':
                number_string_length++;
                has_decimal_point = true;
                break;

            default:
                goto loop_end;
        }
    }
loop_end:
    /* malloc for temporary buffer, add 1 for '\0' */
    number_c_string = (unsigned char *) input_buffer->hooks.allocate(number_string_length + 1);
    if (number_c_string == NULL)
    {
        return false; /* allocation failure */
    }

    memcpy(number_c_string, buffer_at_offset(input_buffer), number_string_length);
    number_c_string[number_string_length] = '\0';

    if (has_decimal_point)
    {
        for (i = 0; i < number_string_length; i++)
        {
            if (number_c_string[i] == '.')
            {
                /* replace '.' with the decimal point of the current locale (for strtod) */
                number_c_string[i] = decimal_point;
            }
        }
    }

    number = strtod((const char*)number_c_string, (char**)&after_end);
    if (number_c_string == after_end)
    {
        /* free the temporary buffer */
        input_buffer->hooks.deallocate(number_c_string);
        return false; /* parse_error */
    }

    item->valuedouble = number;

    /* use saturation in case of overflow */
    if (number >= INT_MAX)
    {
        item->valueint = INT_MAX;
    }
    else if (number <= (double)INT_MIN)
    {
        item->valueint = INT_MIN;
    }
    else
    {
        item->valueint = (int)number;
    }

    item->type = cJSON_Number;

    input_buffer->offset += (size_t)(after_end - number_c_string);
    /* free the temporary buffer */
    input_buffer->hooks.deallocate(number_c_string);
    return true;
}

/* don't ask me, but the original cJSON_SetNumberValue returns an integer or double */
CJSON_PUBLIC(double) cJSON_SetNumberHelper(cJSON *object, double number)
{
    if (object == NULL)
    {
        return (double)NAN;
    }

    if (number >= INT_MAX)
    {
        object->valueint = INT_MAX;
    }
    else if (number <= (double)INT_MIN)
    {
        object->valueint = INT_MIN;
    }
    else
    {
        object->valueint = (int)number;
    }

    return object->valuedouble = number;
}

/* Note: when passing a NULL valuestring, cJSON_SetValuestring treats this as an error and return NULL */
CJSON_PUBLIC(char*) cJSON_SetValuestring(cJSON *object, const char *valuestring)
{
    char *copy = NULL;
    size_t v1_len;
    size_t v2_len;
    /* if object's type is not cJSON_String or is cJSON_IsReference, it should not set valuestring */
    if ((object == NULL) || !(object->type & cJSON_String) || (object->type & cJSON_IsReference))
    {
        return NULL;
    }
    /* return NULL if the object is corrupted or valuestring is NULL */
    if (object->valuestring == NULL || valuestring == NULL)
    {
        return NULL;
    }

    v1_len = strlen(valuestring);
    v2_len = strlen(object->valuestring);

    if (v1_len <= v2_len)
    {
        /* strcpy does not handle overlapping string: [X1, X2] [Y1, Y2] => X2 < Y1 or Y2 < X1 */
        if (!( valuestring + v1_len < object->valuestring || object->valuestring + v2_len < valuestring ))
        {
            return NULL;
        }
        strcpy(object->valuestring, valuestring);
        return object->valuestring;
    }
    copy = (char*) cJSON_strdup((const unsigned char*)valuestring, &global_hooks);
    if (copy == NULL)
    {
        return NULL;
    }
    if (object->valuestring != NULL)
    {
        cJSON_free(object->valuestring);
    }
    object->valuestring = copy;

    return copy;
}

typedef struct
{
    unsigned char *buffer;
    size_t length;
    size_t offset;
    size_t depth; /* current nesting depth (for formatted printing) */
    cJSON_bool noalloc;
    cJSON_bool format; /* is this print a formatted print */
    internal_hooks hooks;
} printbuffer;

/* realloc printbuffer if necessary to have at least "needed" bytes more */
static unsigned char* ensure(printbuffer * const p, size_t needed)
{
    unsigned char *newbuffer = NULL;
    size_t newsize = 0;

    if ((p == NULL) || (p->buffer == NULL))
    {
        return NULL;
    }

    if ((p->length > 0) && (p->offset >= p->length))
    {
        /* make sure that offset is valid */
        return NULL;
    }

    if (needed > INT_MAX)
    {
        /* sizes bigger than INT_MAX are currently not supported */
        return NULL;
    }

    needed += p->offset + 1;
    if (needed <= p->length)
    {
        return p->buffer + p->offset;
    }

    if (p->noalloc) {
        return NULL;
    }

    /* calculate new buffer size */
    if (needed > (INT_MAX / 2))
    {
        /* overflow of int, use INT_MAX if possible */
        if (needed <= INT_MAX)
        {
            newsize = INT_MAX;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        newsize = needed * 2;
    }

    if (p->hooks.reallocate != NULL)
    {
        /* reallocate with realloc if available */
        newbuffer = (unsigned char*)p->hooks.reallocate(p->buffer, newsize);
        if (newbuffer == NULL)
        {
            p->hooks.deallocate(p->buffer);
            p->length = 0;
            p->buffer = NULL;

            return NULL;
        }
    }
    else
    {
        /* otherwise reallocate manually */
        newbuffer = (unsigned char*)p->hooks.allocate(newsize);
        if (!newbuffer)
        {
            p->hooks.deallocate(p->buffer);
            p->length = 0;
            p->buffer = NULL;

            return NULL;
        }

        memcpy(newbuffer, p->buffer, p->offset + 1);
        p->hooks.deallocate(p->buffer);
    }
    p->length = newsize;
    p->buffer = newbuffer;

    return newbuffer + p->offset;
}

/* calculate the new length of the string in a printbuffer and update the offset */
static void update_offset(printbuffer * const buffer)
{
    const unsigned char *buffer_pointer = NULL;
    if ((buffer == NULL) || (buffer->buffer == NULL))
    {
        return;
    }
    buffer_pointer = buffer->buffer + buffer->offset;

    buffer->offset += strlen((const char*)buffer_pointer);
}

/* securely comparison of floating-point variables */
static cJSON_bool compare_double(double a, double b)
{
    double maxVal = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return (fabs(a - b) <= maxVal * DBL_EPSILON);
}

/* Render the number nicely from the given item into a string. */
static cJSON_bool print_number(const cJSON * const item, printbuffer * const output_buffer)
{
    unsigned char *output_pointer = NULL;
    double d = item->valuedouble;
    int length = 0;
    size_t i = 0;
    unsigned char number_buffer[26] = {0}; /* temporary buffer to print the number into */
    unsigned char decimal_point = get_decimal_point();
    double test = 0.0;

    if (output_buffer == NULL)
    {
        return false;
    }

    /* This checks for NaN and Infinity */
    if (isnan(d) || isinf(d))
    {
        length = sprintf((char*)number_buffer, "null");
    }
    else if(d == (double)item->valueint)
    {
        length = sprintf((char*)number_buffer, "%d", item->valueint);
    }
    else
    {
        /* Try 15 decimal places of precision to avoid nonsignificant nonzero digits */
        length = sprintf((char*)number_buffer, "%1.15g", d);

        /* Check whether the original double can be recovered */
        if ((sscanf((char*)number_buffer, "%lg", &test) != 1) || !compare_double((double)test, d))
        {
            /* If not, print with 17 decimal places of precision */
            length = sprintf((char*)number_buffer, "%1.17g", d);
        }
    }

    /* sprintf failed or buffer overrun occurred */
    if ((length < 0) || (length > (int)(sizeof(number_buffer) - 1)))
    {
        return false;
    }

    /* reserve appropriate space in the output */
    output_pointer = ensure(output_buffer, (size_t)length + sizeof(""));
    if (output_pointer == NULL)
    {
        return false;
    }

    /* copy the printed number to the output and replace locale
     * dependent decimal point with '.' */
    for (i = 0; i < ((size_t)length); i++)
    {
        if (number_buffer[i] == decimal_point)
        {
            output_pointer[i] = '.';
            continue;
        }

        output_pointer[i] = number_buffer[i];
    }
    output_pointer[i] = '\0';

    output_buffer->offset += (size_t)length;

    return true;
}

/* parse 4 digit hexadecimal number */
static unsigned parse_hex4(const unsigned char * const input)
{
    unsigned int h = 0;
    size_t i = 0;

    for (i = 0; i < 4; i++)
    {
        /* parse digit */
        if ((input[i] >= '0') && (input[i] <= '9'))
        {
            h += (unsigned int) input[i] - '0';
        }
        else if ((input[i] >= 'A') && (input[i] <= 'F'))
        {
            h += (unsigned int) 10 + input[i] - 'A';
        }
        else if ((input[i] >= 'a') && (input[i] <= 'f'))
        {
            h += (unsigned int) 10 + input[i] - 'a';
        }
        else /* invalid */
        {
            return 0;
        }

        if (i < 3)
        {
            /* shift left to make place for the next nibble */
            h = h << 4;
        }
    }

    return h;
}

/* converts a UTF-16 literal to UTF-8
 * A literal can be one or two sequences of the form \uXXXX */
static unsigned char utf16_literal_to_utf8(const unsigned char * const input_pointer, const unsigned char * const input_end, unsigned char **output_pointer)
{
    long unsigned int codepoint = 0;
    unsigned int first_code = 0;
    const unsigned char *first_sequence = input_pointer;
    unsigned char utf8_length = 0;
    unsigned char utf8_position = 0;
    unsigned char sequence_length = 0;
    unsigned char first_byte_mark = 0;

    if ((input_end - first_sequence) < 6)
    {
        /* input ends unexpectedly */
        goto fail;
    }

    /* get the first utf16 sequence */
    first_code = parse_hex4(first_sequence + 2);

    /* check that the code is valid */
    if (((first_code >= 0xDC00) && (first_code <= 0xDFFF)))
    {
        goto fail;
    }

    /* UTF16 surrogate pair */
    if ((first_code >= 0xD800) && (first_code <= 0xDBFF))
    {
        const unsigned char *second_sequence = first_sequence + 6;
        unsigned int second_code = 0;
        sequence_length = 12; /* \uXXXX\uXXXX */

        if ((input_end - second_sequence) < 6)
        {
            /* input ends unexpectedly */
            goto fail;
        }

        if ((second_sequence[0] != '\\') || (second_sequence[1] != 'u'))
        {
            /* missing second half of the surrogate pair */
            goto fail;
        }

        /* get the second utf16 sequence */
        second_code = parse_hex4(second_sequence + 2);
        /* check that the code is valid */
        if ((second_code < 0xDC00) || (second_code > 0xDFFF))
        {
            /* invalid second half of the surrogate pair */
            goto fail;
        }


        /* calculate the unicode codepoint from the surrogate pair */
        codepoint = 0x10000 + (((first_code & 0x3FF) << 10) | (second_code & 0x3FF));
    }
    else
    {
        sequence_length = 6; /* \uXXXX */
        codepoint = first_code;
    }

    /* encode as UTF-8
     * takes at maximum 4 bytes to encode:
     * 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    if (codepoint < 0x80)
    {
        /* normal ascii, encoding 0xxxxxxx */
        utf8_length = 1;
    }
    else if (codepoint < 0x800)
    {
        /* two bytes, encoding 110xxxxx 10xxxxxx */
        utf8_length = 2;
        first_byte_mark = 0xC0; /* 11000000 */
    }
    else if (codepoint < 0x10000)
    {
        /* three bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx */
        utf8_length = 3;
        first_byte_mark = 0xE0; /* 11100000 */
    }
    else if (codepoint <= 0x10FFFF)
    {
        /* four bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        utf8_length = 4;
        first_byte_mark = 0xF0; /* 11110000 */
    }
    else
    {
        /* invalid unicode codepoint */
        goto fail;
    }

    /* encode as utf8 */
    for (utf8_position = (unsigned char)(utf8_length - 1); utf8_position > 0; utf8_position--)
    {
        /* 10xxxxxx */
        (*output_pointer)[utf8_position] = (unsigned char)((codepoint | 0x80) & 0xBF);
        codepoint >>= 6;
    }
    /* encode first byte */
    if (utf8_length > 1)
    {
        (*output_pointer)[0] = (unsigned char)((codepoint | first_byte_mark) & 0xFF);
    }
    else
    {
        (*output_pointer)[0] = (unsigned char)(codepoint & 0x7F);
    }

    *output_pointer += utf8_length;

    return sequence_length;

fail:
    return 0;
}

/* Parse the input text into an unescaped cinput, and populate item. */
static cJSON_bool parse_string(cJSON * const item, parse_buffer * const input_buffer)
{
    const unsigned char *input_pointer = buffer_at_offset(input_buffer) + 1;
    const unsigned char *input_end = buffer_at_offset(input_buffer) + 1;
    unsigned char *output_pointer = NULL;
    unsigned char *output = NULL;

    /* not a string */
    if (buffer_at_offset(input_buffer)[0] != '\"')
    {
        goto fail;
    }

    {
        /* calculate approximate size of the output (overestimate) */
        size_t allocation_length = 0;
        size_t skipped_bytes = 0;
        while (((size_t)(input_end - input_buffer->content) < input_buffer->length) && (*input_end != '\"'))
        {
            /* is escape sequence */
            if (input_end[0] == '\\')
            {
                if ((size_t)(input_end + 1 - input_buffer->content) >= input_buffer->length)
                {
                    /* prevent buffer overflow when last input character is a backslash */
                    goto fail;
                }
                skipped_bytes++;
                input_end++;
            }
            input_end++;
        }
        if (((size_t)(input_end - input_buffer->content) >= input_buffer->length) || (*input_end != '\"'))
        {
            goto fail; /* string ended unexpectedly */
        }

        /* This is at most how much we need for the output */
        allocation_length = (size_t) (input_end - buffer_at_offset(input_buffer)) - skipped_bytes;
        output = (unsigned char*)input_buffer->hooks.allocate(allocation_length + sizeof(""));
        if (output == NULL)
        {
            goto fail; /* allocation failure */
        }
    }

    output_pointer = output;
    /* loop through the string literal */
    while (input_pointer < input_end)
    {
        if (*input_pointer != '\\')
        {
            *output_pointer++ = *input_pointer++;
        }
        /* escape sequence */
        else
        {
            unsigned char sequence_length = 2;
            if ((input_end - input_pointer) < 1)
            {
                goto fail;
            }

            switch (input_pointer[1])
            {
                case 'b':
                    *output_pointer++ = '\b';
                    break;
                case 'f':
                    *output_pointer++ = '\f';
                    break;
                case 'n':
                    *output_pointer++ = '\n';
                    break;
                case 'r':
                    *output_pointer++ = '\r';
                    break;
                case 't':
                    *output_pointer++ = '\t';
                    break;
                case '\"':
                case '\\':
                case '/':
                    *output_pointer++ = input_pointer[1];
                    break;

                /* UTF-16 literal */
                case 'u':
                    sequence_length = utf16_literal_to_utf8(input_pointer, input_end, &output_pointer);
                    if (sequence_length == 0)
                    {
                        /* failed to convert UTF16-literal to UTF-8 */
                        goto fail;
                    }
                    break;

                default:
                    goto fail;
            }
            input_pointer += sequence_length;
        }
    }

    /* zero terminate the output */
    *output_pointer = '\0';

    item->type = cJSON_String;
    item->valuestring = (char*)output;

    input_buffer->offset = (size_t) (input_end - input_buffer->content);
    input_buffer->offset++;

    return true;

fail:
    if (output != NULL)
    {
        input_buffer->hooks.deallocate(output);
        output = NULL;
    }

    if (input_pointer != NULL)
    {
        input_buffer->offset = (size_t)(input_pointer - input_buffer->content);
    }

    return false;
}

/* Render the cstring provided to an escaped version that can be printed. */
static cJSON_bool print_string_ptr(const unsigned char * const input, printbuffer * const output_buffer)
{
    const unsigned char *input_pointer = NULL;
    unsigned char *output = NULL;
    unsigned char *output_pointer = NULL;
    size_t output_length = 0;
    /* numbers of additional characters needed for escaping */
    size_t escape_characters = 0;

    if (output_buffer == NULL)
    {
        return false;
    }

    /* empty string */
    if (input == NULL)
    {
        output = ensure(output_buffer, sizeof("\"\""));
        if (output == NULL)
        {
            return false;
        }
        strcpy((char*)output, "\"\"");

        return true;
    }

    /* set "flag" to 1 if something needs to be escaped */
    for (input_pointer = input; *input_pointer; input_pointer++)
    {
        switch (*input_pointer)
        {
            case '\"':
            case '\\':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                /* one character escape sequence */
                escape_characters++;
                break;
            default:
                if (*input_pointer < 32)
                {
                    /* UTF-16 escape sequence uXXXX */
                    escape_characters += 5;
                }
                break;
        }
    }
    output_length = (size_t)(input_pointer - input) + escape_characters;

    output = ensure(output_buffer, output_length + sizeof("\"\""));
    if (output == NULL)
    {
        return false;
    }

    /* no characters have to be escaped */
    if (escape_characters == 0)
    {
        output[0] = '\"';
        memcpy(output + 1, input, output_length);
        output[output_length + 1] = '\"';
        output[output_length + 2] = '\0';

        return true;
    }

    output[0] = '\"';
    output_pointer = output + 1;
    /* copy the string */
    for (input_pointer = input; *input_pointer != '\0'; (void)input_pointer++, output_pointer++)
    {
        if ((*input_pointer > 31) && (*input_pointer != '\"') && (*input_pointer != '\\'))
        {
            /* normal character, copy */
            *output_pointer = *input_pointer;
        }
        else
        {
            /* character needs to be escaped */
            *output_pointer++ = '\\';
            switch (*input_pointer)
            {
                case '\\':
                    *output_pointer = '\\';
                    break;
                case '\"':
                    *output_pointer = '\"';
                    break;
                case '\b':
                    *output_pointer = 'b';
                    break;
                case '\f':
                    *output_pointer = 'f';
                    break;
                case '\n':
                    *output_pointer = 'n';
                    break;
                case '\r':
                    *output_pointer = 'r';
                    break;
                case '\t':
                    *output_pointer = 't';
                    break;
                default:
                    /* escape and print as unicode codepoint */
                    sprintf((char*)output_pointer, "u%04x", *input_pointer);
                    output_pointer += 4;
                    break;
            }
        }
    }
    output[output_length + 1] = '\"';
    output[output_length + 2] = '\0';

    return true;
}

/* Invoke print_string_ptr (which is useful) on an item. */
static cJSON_bool print_string(const cJSON * const item, printbuffer * const p)
{
    return print_string_ptr((unsigned char*)item->valuestring, p);
}

/* Predeclare these prototypes. */
static cJSON_bool parse_value(cJSON * const item, parse_buffer * const input_buffer);
static cJSON_bool print_value(const cJSON * const item, printbuffer * const output_buffer);
static cJSON_bool parse_array(cJSON * const item, parse_buffer * const input_buffer);
static cJSON_bool print_array(const cJSON * const item, printbuffer * const output_buffer);
static cJSON_bool parse_object(cJSON * const item, parse_buffer * const input_buffer);
static cJSON_bool print_object(const cJSON * const item, printbuffer * const output_buffer);

/* Utility to jump whitespace and cr/lf */
static parse_buffer *buffer_skip_whitespace(parse_buffer * const buffer)
{
    if ((buffer == NULL) || (buffer->content == NULL))
    {
        return NULL;
    }

    if (cannot_access_at_index(buffer, 0))
    {
        return buffer;
    }

    while (can_access_at_index(buffer, 0) && (buffer_at_offset(buffer)[0] <= 32))
    {
       buffer->offset++;
    }

    if (buffer->offset == buffer->length)
    {
        buffer->offset--;
    }

    return buffer;
}

/* skip the UTF-8 BOM (byte order mark) if it is at the beginning of a buffer */
static parse_buffer *skip_utf8_bom(parse_buffer * const buffer)
{
    if ((buffer == NULL) || (buffer->content == NULL) || (buffer->offset != 0))
    {
        return NULL;
    }

    if (can_access_at_index(buffer, 4) && (strncmp((const char*)buffer_at_offset(buffer), "\xEF\xBB\xBF", 3) == 0))
    {
        buffer->offset += 3;
    }

    return buffer;
}

CJSON_PUBLIC(cJSON *) cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated)
{
    size_t buffer_length;

    if (NULL == value)
    {
        return NULL;
    }

    /* Adding null character size due to require_null_terminated. */
    buffer_length = strlen(value) + sizeof("");

    return cJSON_ParseWithLengthOpts(value, buffer_length, return_parse_end, require_null_terminated);
}

/* Parse an object - create a new root, and populate. */
CJSON_PUBLIC(cJSON *) cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end, cJSON_bool require_null_terminated)
{
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0, 0 } };
    cJSON *item = NULL;

    /* reset error position */
    global_error.json = NULL;
    global_error.position = 0;

    if (value == NULL || 0 == buffer_length)
    {
        goto fail;
    }

    buffer.content = (const unsigned char*)value;
    buffer.length = buffer_length;
    buffer.offset = 0;
    buffer.hooks = global_hooks;

    item = cJSON_New_Item(&global_hooks);
    if (item == NULL) /* memory fail */
    {
        goto fail;
    }

    if (!parse_value(item, buffer_skip_whitespace(skip_utf8_bom(&buffer))))
    {
        /* parse failure. ep is set. */
        goto fail;
    }

    /* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
    if (require_null_terminated)
    {
        buffer_skip_whitespace(&buffer);
        if ((buffer.offset >= buffer.length) || buffer_at_offset(&buffer)[0] != '\0')
        {
            goto fail;
        }
    }
    if (return_parse_end)
    {
        *return_parse_end = (const char*)buffer_at_offset(&buffer);
    }

    return item;

fail:
    if (item != NULL)
    {
        cJSON_Delete(item);
    }

    if (value != NULL)
    {
        error local_error;
        local_error.json = (const unsigned char*)value;
        local_error.position = 0;

        if (buffer.offset < buffer.length)
        {
            local_error.position = buffer.offset;
        }
        else if (buffer.length > 0)
        {
            local_error.position = buffer.length - 1;
        }

        if (return_parse_end != NULL)
        {
            *return_parse_end = (const char*)local_error.json + local_error.position;
        }

        global_error = local_error;
    }

    return NULL;
}

/* Default options for cJSON_Parse */
CJSON_PUBLIC(cJSON *) cJSON_Parse(const char *value)
{
    return cJSON_ParseWithOpts(value, 0, 0);
}

CJSON_PUBLIC(cJSON *) cJSON_ParseWithLength(const char *value, size_t buffer_length)
{
    return cJSON_ParseWithLengthOpts(value, buffer_length, 0, 0);
}

#define cjson_min(a, b) (((a) < (b)) ? (a) : (b))

static unsigned char *print(const cJSON * const item, cJSON_bool format, const internal_hooks * const hooks)
{
    static const size_t default_buffer_size = 256;
    printbuffer buffer[1];
    unsigned char *printed = NULL;

    memset(buffer, 0, sizeof(buffer));

    /* create buffer */
    buffer->buffer = (unsigned char*) hooks->allocate(default_buffer_size);
    buffer->length = default_buffer_size;
    buffer->format = format;
    buffer->hooks = *hooks;
    if (buffer->buffer == NULL)
    {
        goto fail;
    }

    /* print the value */
    if (!print_value(item, buffer))
    {
        goto fail;
    }
    update_offset(buffer);

    /* check if reallocate is available */
    if (hooks->reallocate != NULL)
    {
        printed = (unsigned char*) hooks->reallocate(buffer->buffer, buffer->offset + 1);
        if (printed == NULL) {
            goto fail;
        }
        buffer->buffer = NULL;
    }
    else /* otherwise copy the JSON over to a new buffer */
    {
        printed = (unsigned char*) hooks->allocate(buffer->offset + 1);
        if (printed == NULL)
        {
            goto fail;
        }
        memcpy(printed, buffer->buffer, cjson_min(buffer->length, buffer->offset + 1));
        printed[buffer->offset] = '\0'; /* just to be sure */

        /* free the buffer */
        hooks->deallocate(buffer->buffer);
        buffer->buffer = NULL;
    }

    return printed;

fail:
    if (buffer->buffer != NULL)
    {
        hooks->deallocate(buffer->buffer);
        buffer->buffer = NULL;
    }

    if (printed != NULL)
    {
        hooks->deallocate(printed);
        printed = NULL;
    }

    return NULL;
}

/* Render a cJSON item/entity/structure to text. */
CJSON_PUBLIC(char *) cJSON_Print(const cJSON *item)
{
    return (char*)print(item, true, &global_hooks);
}

CJSON_PUBLIC(char *) cJSON_PrintUnformatted(const cJSON *item)
{
    return (char*)print(item, false, &global_hooks);
}

CJSON_PUBLIC(char *) cJSON_PrintBuffered(const cJSON *item, int prebuffer, cJSON_bool fmt)
{
    printbuffer p = { 0, 0, 0, 0, 0, 0, { 0, 0, 0 } };

    if (prebuffer < 0)
    {
        return NULL;
    }

    p.buffer = (unsigned char*)global_hooks.allocate((size_t)prebuffer);
    if (!p.buffer)
    {
        return NULL;
    }

    p.length = (size_t)prebuffer;
    p.offset = 0;
    p.noalloc = false;
    p.format = fmt;
    p.hooks = global_hooks;

    if (!print_value(item, &p))
    {
        global_hooks.deallocate(p.buffer);
        p.buffer = NULL;
        return NULL;
    }

    return (char*)p.buffer;
}

CJSON_PUBLIC(cJSON_bool) cJSON_PrintPreallocated(cJSON *item, char *buffer, const int length, const cJSON_bool format)
{
    printbuffer p = { 0, 0, 0, 0, 0, 0, { 0, 0, 0 } };

    if ((length < 0) || (buffer == NULL))
    {
        return false;
    }

    p.buffer = (unsigned char*)buffer;
    p.length = (size_t)length;
    p.offset = 0;
    p.noalloc = true;
    p.format = format;
    p.hooks = global_hooks;

    return print_value(item, &p);
}

/* Parser core - when encountering text, process appropriately. */
static cJSON_bool parse_value(cJSON * const item, parse_buffer * const input_buffer)
{
    if ((input_buffer == NULL) || (input_buffer->content == NULL))
    {
        return false; /* no input */
    }

    /* parse the different types of values */
    /* null */
    if (can_read(input_buffer, 4) && (strncmp((const char*)buffer_at_offset(input_buffer), "null", 4) == 0))
    {
        item->type = cJSON_NULL;
        input_buffer->offset += 4;
        return true;
    }
    /* false */
    if (can_read(input_buffer, 5) && (strncmp((const char*)buffer_at_offset(input_buffer), "false", 5) == 0))
    {
        item->type = cJSON_False;
        input_buffer->offset += 5;
        return true;
    }
    /* true */
    if (can_read(input_buffer, 4) && (strncmp((const char*)buffer_at_offset(input_buffer), "true", 4) == 0))
    {
        item->type = cJSON_True;
        item->valueint = 1;
        input_buffer->offset += 4;
        return true;
    }
    /* string */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '\"'))
    {
        return parse_string(item, input_buffer);
    }
    /* number */
    if (can_access_at_index(input_buffer, 0) && ((buffer_at_offset(input_buffer)[0] == '-') || ((buffer_at_offset(input_buffer)[0] >= '0') && (buffer_at_offset(input_buffer)[0] <= '9'))))
    {
        return parse_number(item, input_buffer);
    }
    /* array */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '['))
    {
        return parse_array(item, input_buffer);
    }
    /* object */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '{'))
    {
        return parse_object(item, input_buffer);
    }

    return false;
}

/* Render a value to text. */
static cJSON_bool print_value(const cJSON * const item, printbuffer * const output_buffer)
{
    unsigned char *output = NULL;

    if ((item == NULL) || (output_buffer == NULL))
    {
        return false;
    }

    switch ((item->type) & 0xFF)
    {
        case cJSON_NULL:
            output = ensure(output_buffer, 5);
            if (output == NULL)
            {
                return false;
            }
            strcpy((char*)output, "null");
            return true;

        case cJSON_False:
            output = ensure(output_buffer, 6);
            if (output == NULL)
            {
                return false;
            }
            strcpy((char*)output, "false");
            return true;

        case cJSON_True:
            output = ensure(output_buffer, 5);
            if (output == NULL)
            {
                return false;
            }
            strcpy((char*)output, "true");
            return true;

        case cJSON_Number:
            return print_number(item, output_buffer);

        case cJSON_Raw:
        {
            size_t raw_length = 0;
            if (item->valuestring == NULL)
            {
                return false;
            }

            raw_length = strlen(item->valuestring) + sizeof("");
            output = ensure(output_buffer, raw_length);
            if (output == NULL)
            {
                return false;
            }
            memcpy(output, item->valuestring, raw_length);
            return true;
        }

        case cJSON_String:
            return print_string(item, output_buffer);

        case cJSON_Array:
            return print_array(item, output_buffer);

        case cJSON_Object:
            return print_object(item, output_buffer);

        default:
            return false;
    }
}

/* Build an array from input text. */
static cJSON_bool parse_array(cJSON * const item, parse_buffer * const input_buffer)
{
    cJSON *head = NULL; /* head of the linked list */
    cJSON *current_item = NULL;

    if (input_buffer->depth >= CJSON_NESTING_LIMIT)
    {
        return false; /* to deeply nested */
    }
    input_buffer->depth++;

    if (buffer_at_offset(input_buffer)[0] != '[')
    {
        /* not an array */
        goto fail;
    }

    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ']'))
    {
        /* empty array */
        goto success;
    }

    /* check if we skipped to the end of the buffer */
    if (cannot_access_at_index(input_buffer, 0))
    {
        input_buffer->offset--;
        goto fail;
    }

    /* step back to character in front of the first element */
    input_buffer->offset--;
    /* loop through the comma separated array elements */
    do
    {
        /* allocate next item */
        cJSON *new_item = cJSON_New_Item(&(input_buffer->hooks));
        if (new_item == NULL)
        {
            goto fail; /* allocation failure */
        }

        /* attach next item to list */
        if (head == NULL)
        {
            /* start the linked list */
            current_item = head = new_item;
        }
        else
        {
            /* add to the end and advance */
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        /* parse next value */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_value(current_item, input_buffer))
        {
            goto fail; /* failed to parse value */
        }
        buffer_skip_whitespace(input_buffer);
    }
    while (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ','));

    if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != ']')
    {
        goto fail; /* expected end of array */
    }

success:
    input_buffer->depth--;

    if (head != NULL) {
        head->prev = current_item;
    }

    item->type = cJSON_Array;
    item->child = head;

    input_buffer->offset++;

    return true;

fail:
    if (head != NULL)
    {
        cJSON_Delete(head);
    }

    return false;
}

/* Render an array to text */
static cJSON_bool print_array(const cJSON * const item, printbuffer * const output_buffer)
{
    unsigned char *output_pointer = NULL;
    size_t length = 0;
    cJSON *current_element = item->child;

    if (output_buffer == NULL)
    {
        return false;
    }

    if (output_buffer->depth >= CJSON_NESTING_LIMIT)
    {
        return false; /* nesting is too deep */
    }

    /* Compose the output array. */
    /* opening square bracket */
    output_pointer = ensure(output_buffer, 1);
    if (output_pointer == NULL)
    {
        return false;
    }

    *output_pointer = '[';
    output_buffer->offset++;
    output_buffer->depth++;

    while (current_element != NULL)
    {
        if (!print_value(current_element, output_buffer))
        {
            return false;
        }
        update_offset(output_buffer);
        if (current_element->next)
        {
            length = (size_t) (output_buffer->format ? 2 : 1);
            output_pointer = ensure(output_buffer, length + 1);
            if (output_pointer == NULL)
            {
                return false;
            }
            *output_pointer++ = ',';
            if(output_buffer->format)
            {
                *output_pointer++ = ' ';
            }
            *output_pointer = '\0';
            output_buffer->offset += length;
        }
        current_element = current_element->next;
    }

    output_pointer = ensure(output_buffer, 2);
    if (output_pointer == NULL)
    {
        return false;
    }
    *output_pointer++ = ']';
    *output_pointer = '\0';
    output_buffer->depth--;

    return true;
}

/* Build an object from the text. */
static cJSON_bool parse_object(cJSON * const item, parse_buffer * const input_buffer)
{
    cJSON *head = NULL; /* linked list head */
    cJSON *current_item = NULL;

    if (input_buffer->depth >= CJSON_NESTING_LIMIT)
    {
        return false; /* to deeply nested */
    }
    input_buffer->depth++;

    if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != '{'))
    {
        goto fail; /* not an object */
    }

    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '}'))
    {
        goto success; /* empty object */
    }

    /* check if we skipped to the end of the buffer */
    if (cannot_access_at_index(input_buffer, 0))
    {
        input_buffer->offset--;
        goto fail;
    }

    /* step back to character in front of the first element */
    input_buffer->offset--;
    /* loop through the comma separated array elements */
    do
    {
        /* allocate next item */
        cJSON *new_item = cJSON_New_Item(&(input_buffer->hooks));
        if (new_item == NULL)
        {
            goto fail; /* allocation failure */
        }

        /* attach next item to list */
        if (head == NULL)
        {
            /* start the linked list */
            current_item = head = new_item;
        }
        else
        {
            /* add to the end and advance */
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        if (cannot_access_at_index(input_buffer, 1))
        {
            goto fail; /* nothing comes after the comma */
        }

        /* parse the name of the child */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_string(current_item, input_buffer))
        {
            goto fail; /* failed to parse name */
        }
        buffer_skip_whitespace(input_buffer);

        /* swap valuestring and string, because we parsed the name */
        current_item->string = current_item->valuestring;
        current_item->valuestring = NULL;

        if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != ':'))
        {
            goto fail; /* invalid object */
        }

        /* parse the value */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_value(current_item, input_buffer))
        {
            goto fail; /* failed to parse value */
        }
        buffer_skip_whitespace(input_buffer);
    }
    while (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ','));

    if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != '}'))
    {
        goto fail; /* expected end of object */
    }

success:
    input_buffer->depth--;

    if (head != NULL) {
        head->prev = current_item;
    }

    item->type = cJSON_Object;
    item->child = head;

    input_buffer->offset++;
    return true;

fail:
    if (head != NULL)
    {
        cJSON_Delete(head);
    }

    return false;
}

/* Render an object to text. */
static cJSON_bool print_object(const cJSON * const item, printbuffer * const output_buffer)
{
    unsigned char *output_pointer = NULL;
    size_t length = 0;
    cJSON *current_item = item->child;

    if (output_buffer == NULL)
    {
        return false;
    }

    if (output_buffer->depth >= CJSON_NESTING_LIMIT)
    {
        return false; /* nesting is too deep */
    }

    /* Compose the output: */
    length = (size_t) (output_buffer->format ? 2 : 1); /* fmt: {\n */
    output_pointer = ensure(output_buffer, length + 1);
    if (output_pointer == NULL)
    {
        return false;
    }

    *output_pointer++ = '{';
    output_buffer->depth++;
    if (output_buffer->format)
    {
        *output_pointer++ = '\n';
    }
    output_buffer->offset += length;

    while (current_item)
    {
        if (output_buffer->format)
        {
            size_t i;
            output_pointer = ensure(output_buffer, output_buffer->depth);
            if (output_pointer == NULL)
            {
                return false;
            }
            for (i = 0; i < output_buffer->depth; i++)
            {
                *output_pointer++ = '\t';
            }
            output_buffer->offset += output_buffer->depth;
        }

        /* print key */
        if (!print_string_ptr((unsigned char*)current_item->string, output_buffer))
        {
            return false;
        }
        update_offset(output_buffer);

        length = (size_t) (output_buffer->format ? 2 : 1);
        output_pointer = ensure(output_buffer, length);
        if (output_pointer == NULL)
        {
            return false;
        }
        *output_pointer++ = ':';
        if (output_buffer->format)
        {
            *output_pointer++ = '\t';
        }
        output_buffer->offset += length;

        /* print value */
        if (!print_value(current_item, output_buffer))
        {
            return false;
        }
        update_offset(output_buffer);

        /* print comma if not last */
        length = ((size_t)(output_buffer->format ? 1 : 0) + (size_t)(current_item->next ? 1 : 0));
        output_pointer = ensure(output_buffer, length + 1);
        if (output_pointer == NULL)
        {
            return false;
        }
        if (current_item->next)
        {
            *output_pointer++ = ',';
        }

        if (output_buffer->format)
        {
            *output_pointer++ = '\n';
        }
        *output_pointer = '\0';
        output_buffer->offset += length;

        current_item = current_item->next;
    }

    output_pointer = ensure(output_buffer, output_buffer->format ? (output_buffer->depth + 1) : 2);
    if (output_pointer == NULL)
    {
        return false;
    }
    if (output_buffer->format)
    {
        size_t i;
        for (i = 0; i < (output_buffer->depth - 1); i++)
        {
            *output_pointer++ = '\t';
        }
    }
    *output_pointer++ = '}';
    *output_pointer = '\0';
    output_buffer->depth--;

    return true;
}

/* Get Array size/item / object item. */
CJSON_PUBLIC(int) cJSON_GetArraySize(const cJSON *array)
{
    cJSON *child = NULL;
    size_t size = 0;

    if (array == NULL)
    {
        return 0;
    }

    child = array->child;

    while(child != NULL)
    {
        size++;
        child = child->next;
    }

    /* FIXME: Can overflow here. Cannot be fixed without breaking the API */

    return (int)size;
}

static cJSON* get_array_item(const cJSON *array, size_t index)
{
    cJSON *current_child = NULL;

    if (array == NULL)
    {
        return NULL;
    }

    current_child = array->child;
    while ((current_child != NULL) && (index > 0))
    {
        index--;
        current_child = current_child->next;
    }

    return current_child;
}

CJSON_PUBLIC(cJSON *) cJSON_GetArrayItem(const cJSON *array, int index)
{
    if (index < 0)
    {
        return NULL;
    }

    return get_array_item(array, (size_t)index);
}

static cJSON *get_object_item(const cJSON * const object, const char * const name, const cJSON_bool case_sensitive)
{
    cJSON *current_element = NULL;

    if ((object == NULL) || (name == NULL))
    {
        return NULL;
    }

    current_element = object->child;
    if (case_sensitive)
    {
        while ((current_element != NULL) && (current_element->string != NULL) && (strcmp(name, current_element->string) != 0))
        {
            current_element = current_element->next;
        }
    }
    else
    {
        while ((current_element != NULL) && (case_insensitive_strcmp((const unsigned char*)name, (const unsigned char*)(current_element->string)) != 0))
        {
            current_element = current_element->next;
        }
    }

    if ((current_element == NULL) || (current_element->string == NULL)) {
        return NULL;
    }

    return current_element;
}

CJSON_PUBLIC(cJSON *) cJSON_GetObjectItem(const cJSON * const object, const char * const string)
{
    return get_object_item(object, string, false);
}

CJSON_PUBLIC(cJSON *) cJSON_GetObjectItemCaseSensitive(const cJSON * const object, const char * const string)
{
    return get_object_item(object, string, true);
}

CJSON_PUBLIC(cJSON_bool) cJSON_HasObjectItem(const cJSON *object, const char *string)
{
    return cJSON_GetObjectItem(object, string) ? 1 : 0;
}

/* Utility for array list handling. */
static void suffix_object(cJSON *prev, cJSON *item)
{
    prev->next = item;
    item->prev = prev;
}

/* Utility for handling references. */
static cJSON *create_reference(const cJSON *item, const internal_hooks * const hooks)
{
    cJSON *reference = NULL;
    if (item == NULL)
    {
        return NULL;
    }

    reference = cJSON_New_Item(hooks);
    if (reference == NULL)
    {
        return NULL;
    }

    memcpy(reference, item, sizeof(cJSON));
    reference->string = NULL;
    reference->type |= cJSON_IsReference;
    reference->next = reference->prev = NULL;
    return reference;
}

static cJSON_bool add_item_to_array(cJSON *array, cJSON *item)
{
    cJSON *child = NULL;

    if ((item == NULL) || (array == NULL) || (array == item))
    {
        return false;
    }

    child = array->child;
    /*
     * To find the last item in array quickly, we use prev in array
     */
    if (child == NULL)
    {
        /* list is empty, start new one */
        array->child = item;
        item->prev = item;
        item->next = NULL;
    }
    else
    {
        /* append to the end */
        if (child->prev)
        {
            suffix_object(child->prev, item);
            array->child->prev = item;
        }
    }

    return true;
}

/* Add item to array/object. */
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToArray(cJSON *array, cJSON *item)
{
    return add_item_to_array(array, item);
}

#if defined(__clang__) || (defined(__GNUC__)  && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
/* helper function to cast away const */
static void* cast_away_const(const void* string)
{
    return (void*)string;
}
#if defined(__clang__) || (defined(__GNUC__)  && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic pop
#endif


static cJSON_bool add_item_to_object(cJSON * const object, const char * const string, cJSON * const item, const internal_hooks * const hooks, const cJSON_bool constant_key)
{
    char *new_key = NULL;
    int new_type = cJSON_Invalid;

    if ((object == NULL) || (string == NULL) || (item == NULL) || (object == item))
    {
        return false;
    }

    if (constant_key)
    {
        new_key = (char*)cast_away_const(string);
        new_type = item->type | cJSON_StringIsConst;
    }
    else
    {
        new_key = (char*)cJSON_strdup((const unsigned char*)string, hooks);
        if (new_key == NULL)
        {
            return false;
        }

        new_type = item->type & ~cJSON_StringIsConst;
    }

    if (!(item->type & cJSON_StringIsConst) && (item->string != NULL))
    {
        hooks->deallocate(item->string);
    }

    item->string = new_key;
    item->type = new_type;

    return add_item_to_array(object, item);
}

CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item)
{
    return add_item_to_object(object, string, item, &global_hooks, false);
}

/* Add an item to an object with constant string as key */
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item)
{
    return add_item_to_object(object, string, item, &global_hooks, true);
}

CJSON_PUBLIC(cJSON_bool) cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item)
{
    if (array == NULL)
    {
        return false;
    }

    return add_item_to_array(array, create_reference(item, &global_hooks));
}

CJSON_PUBLIC(cJSON_bool) cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item)
{
    if ((object == NULL) || (string == NULL))
    {
        return false;
    }

    return add_item_to_object(object, string, create_reference(item, &global_hooks), &global_hooks, false);
}

CJSON_PUBLIC(cJSON*) cJSON_AddNullToObject(cJSON * const object, const char * const name)
{
    cJSON *null = cJSON_CreateNull();
    if (add_item_to_object(object, name, null, &global_hooks, false))
    {
        return null;
    }

    cJSON_Delete(null);
    return NULL;
}

CJSON_PUBLIC(cJSON*) cJSON_AddTrueToObject(cJSON * const object, const char * const name)
{
    cJSON *true_item = cJSON_CreateTrue();
    if (add_item_to_object(object, name, true_item, &global_hooks, false))
    {
        return true_item;
    }

    cJSON_Delete(true_item);
    return NULL;
}

CJSON_PUBLIC(cJSON*) cJSON_AddFalseToObject(cJSON * const object, const char * const name)
{
    cJSON *false_item = cJSON_CreateFalse();
    if (add_item_to_object(object, name, false_item, &global_hooks, false))
    {
        return false_item;
    }

    cJSON_Delete(false_item);
    return NULL;
}

CJSON_PUBLIC(cJSON*) cJSON_AddBoolToObject(cJSON * const object, const char * const name, const cJSON_bool boolean)
{
    cJSON *bool_item = cJSON_CreateBool(boolean);
    if (add_item_to_object(object, name, bool_item, &global_hooks, false))
    {
        return bool_item;
    }

    cJSON_Delete(bool_item);
    return NULL;
}

CJSON_PUBLIC(cJSON*) cJSON_AddNumberToObject(cJSON * const object, const char * const name, const double number)
{
    cJSON *number_item = cJSON_CreateNumber(number);
    if (add_item_to_object(object, name, number_item, &global_hooks, false))
    {
        return number_item;
    }

    cJSON_Delete(number_item);
    return NULL;
}

CJSON_PUBLIC(cJSON*) cJSON_AddStringToObject(cJSON * const object, const char * const name, const char * const string)
{
    cJSON *string_item = cJSON_CreateString(string);
    if (add_item_to_object(object, name, string_item, &global_hooks, false))
    {
        return string_item;
    }

    cJSON_Delete(string_item);
    return NULL;
}

CJSON_PUBLIC(cJSON*) cJSON_AddRawToObject(cJSON * const object, const char * const name, const char * const raw)
{
    cJSON *raw_item = cJSON_CreateRaw(raw);
    if (add_item_to_object(object, name, raw_item, &global_hooks, false))
    {
        return raw_item;
    }

    cJSON_Delete(raw_item);
    return NULL;
}

CJSON_PUBLIC(cJSON*) cJSON_AddObjectToObject(cJSON * const object, const char * const name)
{
    cJSON *object_item = cJSON_CreateObject();
    if (add_item_to_object(object, name, object_item, &global_hooks, false))
    {
        return object_item;
    }

    cJSON_Delete(object_item);
    return NULL;
}

CJSON_PUBLIC(cJSON*) cJSON_AddArrayToObject(cJSON * const object, const char * const name)
{
    cJSON *array = cJSON_CreateArray();
    if (add_item_to_object(object, name, array, &global_hooks, false))
    {
        return array;
    }

    cJSON_Delete(array);
    return NULL;
}

CJSON_PUBLIC(cJSON *) cJSON_DetachItemViaPointer(cJSON *parent, cJSON * const item)
{
    if ((parent == NULL) || (item == NULL) || (item != parent->child && item->prev == NULL))
    {
        return NULL;
    }

    if (item != parent->child)
    {
        /* not the first element */
        item->prev->next = item->next;
    }
    if (item->next != NULL)
    {
        /* not the last element */
        item->next->prev = item->prev;
    }

    if (item == parent->child)
    {
        /* first element */
        parent->child = item->next;
    }
    else if (item->next == NULL)
    {
        /* last element */
        parent->child->prev = item->prev;
    }

    /* make sure the detached item doesn't point anywhere anymore */
    item->prev = NULL;
    item->next = NULL;

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromArray(cJSON *array, int which)
{
    if (which < 0)
    {
        return NULL;
    }

    return cJSON_DetachItemViaPointer(array, get_array_item(array, (size_t)which));
}

CJSON_PUBLIC(void) cJSON_DeleteItemFromArray(cJSON *array, int which)
{
    cJSON_Delete(cJSON_DetachItemFromArray(array, which));
}

CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObject(cJSON *object, const char *string)
{
    cJSON *to_detach = cJSON_GetObjectItem(object, string);

    return cJSON_DetachItemViaPointer(object, to_detach);
}

CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObjectCaseSensitive(cJSON *object, const char *string)
{
    cJSON *to_detach = cJSON_GetObjectItemCaseSensitive(object, string);

    return cJSON_DetachItemViaPointer(object, to_detach);
}

CJSON_PUBLIC(void) cJSON_DeleteItemFromObject(cJSON *object, const char *string)
{
    cJSON_Delete(cJSON_DetachItemFromObject(object, string));
}

CJSON_PUBLIC(void) cJSON_DeleteItemFromObjectCaseSensitive(cJSON *object, const char *string)
{
    cJSON_Delete(cJSON_DetachItemFromObjectCaseSensitive(object, string));
}

/* Replace array/object items with new ones. */
CJSON_PUBLIC(cJSON_bool) cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem)
{
    cJSON *after_inserted = NULL;

    if (which < 0 || newitem == NULL)
    {
        return false;
    }

    after_inserted = get_array_item(array, (size_t)which);
    if (after_inserted == NULL)
    {
        return add_item_to_array(array, newitem);
    }

    if (after_inserted != array->child && after_inserted->prev == NULL) {
        /* return false if after_inserted is a corrupted array item */
        return false;
    }

    newitem->next = after_inserted;
    newitem->prev = after_inserted->prev;
    after_inserted->prev = newitem;
    if (after_inserted == array->child)
    {
        array->child = newitem;
    }
    else
    {
        newitem->prev->next = newitem;
    }
    return true;
}

CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemViaPointer(cJSON * const parent, cJSON * const item, cJSON * replacement)
{
    if ((parent == NULL) || (parent->child == NULL) || (replacement == NULL) || (item == NULL))
    {
        return false;
    }

    if (replacement == item)
    {
        return true;
    }

    replacement->next = item->next;
    replacement->prev = item->prev;

    if (replacement->next != NULL)
    {
        replacement->next->prev = replacement;
    }
    if (parent->child == item)
    {
        if (parent->child->prev == parent->child)
        {
            replacement->prev = replacement;
        }
        parent->child = replacement;
    }
    else
    {   /*
         * To find the last item in array quickly, we use prev in array.
         * We can't modify the last item's next pointer where this item was the parent's child
         */
        if (replacement->prev != NULL)
        {
            replacement->prev->next = replacement;
        }
        if (replacement->next == NULL)
        {
            parent->child->prev = replacement;
        }
    }

    item->next = NULL;
    item->prev = NULL;
    cJSON_Delete(item);

    return true;
}

CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem)
{
    if (which < 0)
    {
        return false;
    }

    return cJSON_ReplaceItemViaPointer(array, get_array_item(array, (size_t)which), newitem);
}

static cJSON_bool replace_item_in_object(cJSON *object, const char *string, cJSON *replacement, cJSON_bool case_sensitive)
{
    if ((replacement == NULL) || (string == NULL))
    {
        return false;
    }

    /* replace the name in the replacement */
    if (!(replacement->type & cJSON_StringIsConst) && (replacement->string != NULL))
    {
        cJSON_free(replacement->string);
    }
    replacement->string = (char*)cJSON_strdup((const unsigned char*)string, &global_hooks);
    if (replacement->string == NULL)
    {
        return false;
    }

    replacement->type &= ~cJSON_StringIsConst;

    return cJSON_ReplaceItemViaPointer(object, get_object_item(object, string, case_sensitive), replacement);
}

CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem)
{
    return replace_item_in_object(object, string, newitem, false);
}

CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInObjectCaseSensitive(cJSON *object, const char *string, cJSON *newitem)
{
    return replace_item_in_object(object, string, newitem, true);
}

/* Create basic types: */
CJSON_PUBLIC(cJSON *) cJSON_CreateNull(void)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = cJSON_NULL;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateTrue(void)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = cJSON_True;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateFalse(void)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = cJSON_False;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateBool(cJSON_bool boolean)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = boolean ? cJSON_True : cJSON_False;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateNumber(double num)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = cJSON_Number;
        item->valuedouble = num;

        /* use saturation in case of overflow */
        if (num >= INT_MAX)
        {
            item->valueint = INT_MAX;
        }
        else if (num <= (double)INT_MIN)
        {
            item->valueint = INT_MIN;
        }
        else
        {
            item->valueint = (int)num;
        }
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateString(const char *string)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = cJSON_String;
        item->valuestring = (char*)cJSON_strdup((const unsigned char*)string, &global_hooks);
        if(!item->valuestring)
        {
            cJSON_Delete(item);
            return NULL;
        }
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateStringReference(const char *string)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item != NULL)
    {
        item->type = cJSON_String | cJSON_IsReference;
        item->valuestring = (char*)cast_away_const(string);
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateObjectReference(const cJSON *child)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item != NULL) {
        item->type = cJSON_Object | cJSON_IsReference;
        item->child = (cJSON*)cast_away_const(child);
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateArrayReference(const cJSON *child) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item != NULL) {
        item->type = cJSON_Array | cJSON_IsReference;
        item->child = (cJSON*)cast_away_const(child);
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateRaw(const char *raw)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type = cJSON_Raw;
        item->valuestring = (char*)cJSON_strdup((const unsigned char*)raw, &global_hooks);
        if(!item->valuestring)
        {
            cJSON_Delete(item);
            return NULL;
        }
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateArray(void)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if(item)
    {
        item->type=cJSON_Array;
    }

    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateObject(void)
{
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item)
    {
        item->type = cJSON_Object;
    }

    return item;
}

/* Create Arrays: */
CJSON_PUBLIC(cJSON *) cJSON_CreateIntArray(const int *numbers, int count)
{
    size_t i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = NULL;

    if ((count < 0) || (numbers == NULL))
    {
        return NULL;
    }

    a = cJSON_CreateArray();

    for(i = 0; a && (i < (size_t)count); i++)
    {
        n = cJSON_CreateNumber(numbers[i]);
        if (!n)
        {
            cJSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateFloatArray(const float *numbers, int count)
{
    size_t i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = NULL;

    if ((count < 0) || (numbers == NULL))
    {
        return NULL;
    }

    a = cJSON_CreateArray();

    for(i = 0; a && (i < (size_t)count); i++)
    {
        n = cJSON_CreateNumber((double)numbers[i]);
        if(!n)
        {
            cJSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateDoubleArray(const double *numbers, int count)
{
    size_t i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = NULL;

    if ((count < 0) || (numbers == NULL))
    {
        return NULL;
    }

    a = cJSON_CreateArray();

    for(i = 0; a && (i < (size_t)count); i++)
    {
        n = cJSON_CreateNumber(numbers[i]);
        if(!n)
        {
            cJSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateStringArray(const char *const *strings, int count)
{
    size_t i = 0;
    cJSON *n = NULL;
    cJSON *p = NULL;
    cJSON *a = NULL;

    if ((count < 0) || (strings == NULL))
    {
        return NULL;
    }

    a = cJSON_CreateArray();

    for (i = 0; a && (i < (size_t)count); i++)
    {
        n = cJSON_CreateString(strings[i]);
        if(!n)
        {
            cJSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p,n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

/* Duplication */
cJSON * cJSON_Duplicate_rec(const cJSON *item, size_t depth, cJSON_bool recurse);

CJSON_PUBLIC(cJSON *) cJSON_Duplicate(const cJSON *item, cJSON_bool recurse)
{
    return cJSON_Duplicate_rec(item, 0, recurse );
}

cJSON * cJSON_Duplicate_rec(const cJSON *item, size_t depth, cJSON_bool recurse)
{
    cJSON *newitem = NULL;
    cJSON *child = NULL;
    cJSON *next = NULL;
    cJSON *newchild = NULL;

    /* Bail on bad ptr */
    if (!item)
    {
        goto fail;
    }
    /* Create new item */
    newitem = cJSON_New_Item(&global_hooks);
    if (!newitem)
    {
        goto fail;
    }
    /* Copy over all vars */
    newitem->type = item->type & (~cJSON_IsReference);
    newitem->valueint = item->valueint;
    newitem->valuedouble = item->valuedouble;
    if (item->valuestring)
    {
        newitem->valuestring = (char*)cJSON_strdup((unsigned char*)item->valuestring, &global_hooks);
        if (!newitem->valuestring)
        {
            goto fail;
        }
    }
    if (item->string)
    {
        newitem->string = (item->type&cJSON_StringIsConst) ? item->string : (char*)cJSON_strdup((unsigned char*)item->string, &global_hooks);
        if (!newitem->string)
        {
            goto fail;
        }
    }
    /* If non-recursive, then we're done! */
    if (!recurse)
    {
        return newitem;
    }
    /* Walk the ->next chain for the child. */
    child = item->child;
    while (child != NULL)
    {
        if(depth >= CJSON_CIRCULAR_LIMIT) {
            goto fail;
        }
        newchild = cJSON_Duplicate_rec(child, depth + 1, true); /* Duplicate (with recurse) each item in the ->next chain */
        if (!newchild)
        {
            goto fail;
        }
        if (next != NULL)
        {
            /* If newitem->child already set, then crosswire ->prev and ->next and move on */
            next->next = newchild;
            newchild->prev = next;
            next = newchild;
        }
        else
        {
            /* Set newitem->child and move to it */
            newitem->child = newchild;
            next = newchild;
        }
        child = child->next;
    }
    if (newitem && newitem->child)
    {
        newitem->child->prev = newchild;
    }

    return newitem;

fail:
    if (newitem != NULL)
    {
        cJSON_Delete(newitem);
    }

    return NULL;
}

static void skip_oneline_comment(char **input)
{
    *input += static_strlen("//");

    for (; (*input)[0] != '\0'; ++(*input))
    {
        if ((*input)[0] == '\n') {
            *input += static_strlen("\n");
            return;
        }
    }
}

static void skip_multiline_comment(char **input)
{
    *input += static_strlen("/*");

    for (; (*input)[0] != '\0'; ++(*input))
    {
        if (((*input)[0] == '*') && ((*input)[1] == '/'))
        {
            *input += static_strlen("*/");
            return;
        }
    }
}

static void minify_string(char **input, char **output) {
    (*output)[0] = (*input)[0];
    *input += static_strlen("\"");
    *output += static_strlen("\"");


    for (; (*input)[0] != '\0'; (void)++(*input), ++(*output)) {
        (*output)[0] = (*input)[0];

        if ((*input)[0] == '\"') {
            (*output)[0] = '\"';
            *input += static_strlen("\"");
            *output += static_strlen("\"");
            return;
        } else if (((*input)[0] == '\\') && ((*input)[1] == '\"')) {
            (*output)[1] = (*input)[1];
            *input += static_strlen("\"");
            *output += static_strlen("\"");
        }
    }
}

CJSON_PUBLIC(void) cJSON_Minify(char *json)
{
    char *into = json;

    if (json == NULL)
    {
        return;
    }

    while (json[0] != '\0')
    {
        switch (json[0])
        {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                json++;
                break;

            case '/':
                if (json[1] == '/')
                {
                    skip_oneline_comment(&json);
                }
                else if (json[1] == '*')
                {
                    skip_multiline_comment(&json);
                } else {
                    json++;
                }
                break;

            case '\"':
                minify_string(&json, (char**)&into);
                break;

            default:
                into[0] = json[0];
                json++;
                into++;
        }
    }

    /* and null-terminate. */
    *into = '\0';
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsInvalid(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Invalid;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsFalse(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_False;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsTrue(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xff) == cJSON_True;
}


CJSON_PUBLIC(cJSON_bool) cJSON_IsBool(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & (cJSON_True | cJSON_False)) != 0;
}
CJSON_PUBLIC(cJSON_bool) cJSON_IsNull(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_NULL;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsNumber(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Number;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsString(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_String;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsArray(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Array;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsObject(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Object;
}

CJSON_PUBLIC(cJSON_bool) cJSON_IsRaw(const cJSON * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Raw;
}

CJSON_PUBLIC(cJSON_bool) cJSON_Compare(const cJSON * const a, const cJSON * const b, const cJSON_bool case_sensitive)
{
    if ((a == NULL) || (b == NULL) || ((a->type & 0xFF) != (b->type & 0xFF)))
    {
        return false;
    }

    /* check if type is valid */
    switch (a->type & 0xFF)
    {
        case cJSON_False:
        case cJSON_True:
        case cJSON_NULL:
        case cJSON_Number:
        case cJSON_String:
        case cJSON_Raw:
        case cJSON_Array:
        case cJSON_Object:
            break;

        default:
            return false;
    }

    /* identical objects are equal */
    if (a == b)
    {
        return true;
    }

    switch (a->type & 0xFF)
    {
        /* in these cases and equal type is enough */
        case cJSON_False:
        case cJSON_True:
        case cJSON_NULL:
            return true;

        case cJSON_Number:
            if (compare_double(a->valuedouble, b->valuedouble))
            {
                return true;
            }
            return false;

        case cJSON_String:
        case cJSON_Raw:
            if ((a->valuestring == NULL) || (b->valuestring == NULL))
            {
                return false;
            }
            if (strcmp(a->valuestring, b->valuestring) == 0)
            {
                return true;
            }

            return false;

        case cJSON_Array:
        {
            cJSON *a_element = a->child;
            cJSON *b_element = b->child;

            for (; (a_element != NULL) && (b_element != NULL);)
            {
                if (!cJSON_Compare(a_element, b_element, case_sensitive))
                {
                    return false;
                }

                a_element = a_element->next;
                b_element = b_element->next;
            }

            /* one of the arrays is longer than the other */
            if (a_element != b_element) {
                return false;
            }

            return true;
        }

        case cJSON_Object:
        {
            cJSON *a_element = NULL;
            cJSON *b_element = NULL;
            cJSON_ArrayForEach(a_element, a)
            {
                /* TODO This has O(n^2) runtime, which is horrible! */
                b_element = get_object_item(b, a_element->string, case_sensitive);
                if (b_element == NULL)
                {
                    return false;
                }

                if (!cJSON_Compare(a_element, b_element, case_sensitive))
                {
                    return false;
                }
            }

            /* doing this twice, once on a and b to prevent true comparison if a subset of b
             * TODO: Do this the proper way, this is just a fix for now */
            cJSON_ArrayForEach(b_element, b)
            {
                a_element = get_object_item(a, b_element->string, case_sensitive);
                if (a_element == NULL)
                {
                    return false;
                }

                if (!cJSON_Compare(b_element, a_element, case_sensitive))
                {
                    return false;
                }
            }

            return true;
        }

        default:
            return false;
    }
}

CJSON_PUBLIC(void *) cJSON_malloc(size_t size)
{
    return global_hooks.allocate(size);
}

CJSON_PUBLIC(void) cJSON_free(void *object)
{
    global_hooks.deallocate(object);
    object = NULL;
}
// ------------------------------------------------------------------------------------------
}

JsonGenerate::JsonGenerate() : result(0), n(config_val("coords")) {
    data.reserve(static_cast<size_t>(n));
}

std::string JsonGenerate::name() const { return "Json::Generate"; }

void JsonGenerate::prepare() {
    for (int64_t i = 0; i < n; i++) {
        double x = custom_round(Helper::next_float(), 8);
        double y = custom_round(Helper::next_float(), 8);
        double z = custom_round(Helper::next_float(), 8);

        std::ostringstream name_ss;
        name_ss << std::fixed << std::setprecision(7) << Helper::next_float()
                << " " << Helper::next_int(10000);

        std::unordered_map<std::string, std::pair<int, bool>> opts = {
            {"1", {1, true}}};

        data.emplace_back(Coordinate{x, y, z, name_ss.str(), opts});
    }
}

void JsonGenerate::run(int iteration_id) {
    (void)iteration_id;

    cJSON* root = cJSON_CreateObject();
    if (!root) return;

    cJSON* coordinates = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "coordinates", coordinates);

    for (const auto& c : data) {
        cJSON* coord = cJSON_CreateObject();

        cJSON_AddNumberToObject(coord, "x", c.x);
        cJSON_AddNumberToObject(coord, "y", c.y);
        cJSON_AddNumberToObject(coord, "z", c.z);
        cJSON_AddStringToObject(coord, "name", c.name.c_str());

        cJSON* opts = cJSON_CreateObject();
        for (const auto& [key, value] : c.opts) {
            cJSON* arr = cJSON_CreateArray();
            cJSON_AddItemToArray(arr, cJSON_CreateNumber(value.first));
            cJSON_AddItemToArray(arr, cJSON_CreateBool(value.second));
            cJSON_AddItemToObject(opts, key.c_str(), arr);
        }
        cJSON_AddItemToObject(coord, "opts", opts);

        cJSON_AddItemToArray(coordinates, coord);
    }

    cJSON_AddStringToObject(root, "info", "some info");

    char* printed = cJSON_PrintUnformatted(root);
    if (printed) {
        _result.assign(printed);
        cJSON_free(printed);
    } else {
        _result.clear();
    }

    if (_result.size() >= 15 &&
        _result.compare(0, 15, "{\"coordinates\":") == 0) {
        result++;
    }

    cJSON_Delete(root);
}

uint32_t JsonGenerate::checksum() { return result; }

JsonParseDom::JsonParseDom() : result_val(0) {}

std::string JsonParseDom::name() const { return "Json::ParseDom"; }

void JsonParseDom::prepare() {
    JsonGenerate jg;
    jg.coords_count() = config_val("coords");
    jg.prepare();
    jg.run(0);
    text = jg.get_result();
}

void JsonParseDom::run(int iteration_id) {
    (void)iteration_id;

    cJSON* root = cJSON_Parse(text.c_str());
    if (!root) return;

    cJSON* coordinates = cJSON_GetObjectItem(root, "coordinates");
    if (!coordinates || !cJSON_IsArray(coordinates)) {
        cJSON_Delete(root);
        return;
    }

    double x_sum = 0.0, y_sum = 0.0, z_sum = 0.0;
    int64_t len = 0;

    cJSON* coord_item = nullptr;
    cJSON_ArrayForEach(coord_item, coordinates) {
        cJSON* x_item = cJSON_GetObjectItem(coord_item, "x");
        cJSON* y_item = cJSON_GetObjectItem(coord_item, "y");
        cJSON* z_item = cJSON_GetObjectItem(coord_item, "z");

        if (x_item && y_item && z_item &&
            cJSON_IsNumber(x_item) && cJSON_IsNumber(y_item) &&
            cJSON_IsNumber(z_item)) {
            x_sum += x_item->valuedouble;
            y_sum += y_item->valuedouble;
            z_sum += z_item->valuedouble;
            len++;
        }
    }

    cJSON_Delete(root);

    if (len > 0) {
        double x_avg = x_sum / len;
        double y_avg = y_sum / len;
        double z_avg = z_sum / len;

        result_val += Helper::checksum_f64(x_avg) +
                      Helper::checksum_f64(y_avg) +
                      Helper::checksum_f64(z_avg);
    }
}

uint32_t JsonParseDom::checksum() { return result_val; }

JsonParseMapping::JsonParseMapping() : result_val(0) {}

std::string JsonParseMapping::name() const { return "Json::ParseMapping"; }

void JsonParseMapping::prepare() {
    JsonGenerate jg;
    jg.coords_count() = config_val("coords");
    jg.prepare();
    jg.run(0);
    text = jg.get_result();
}

void JsonParseMapping::run(int iteration_id) {
    (void)iteration_id;

    cJSON* root = cJSON_Parse(text.c_str());
    if (!root) return;

    cJSON* coordinates = cJSON_GetObjectItem(root, "coordinates");
    if (!coordinates || !cJSON_IsArray(coordinates)) {
        cJSON_Delete(root);
        return;
    }

    double x_sum = 0.0, y_sum = 0.0, z_sum = 0.0;
    int64_t len = 0;

    cJSON* coord_item = nullptr;
    cJSON_ArrayForEach(coord_item, coordinates) {
        cJSON* x_item = cJSON_GetObjectItem(coord_item, "x");
        cJSON* y_item = cJSON_GetObjectItem(coord_item, "y");
        cJSON* z_item = cJSON_GetObjectItem(coord_item, "z");

        if (x_item && y_item && z_item) {
            x_sum += x_item->valuedouble;
            y_sum += y_item->valuedouble;
            z_sum += z_item->valuedouble;
            len++;
        }
    }

    cJSON_Delete(root);

    if (len > 0) {
        double x_avg = x_sum / len;
        double y_avg = y_sum / len;
        double z_avg = z_sum / len;

        result_val += Helper::checksum_f64(x_avg) +
                      Helper::checksum_f64(y_avg) +
                      Helper::checksum_f64(z_avg);
    }
}

uint32_t JsonParseMapping::checksum() { return result_val; }

void LogParser::generate_log_line(std::string &str, int i) {

  str += IPS[i % IPS.size()] + " - - [" + std::to_string(i % 31) +
         "/Oct/2023:" + std::to_string(i % 60) + ":55:36 +0000] \"" +
         METHODS[i % METHODS.size()] + " ";

  if (i % 3 == 0)
    str += "/login?email=" + USERS[i % USERS.size()] + std::to_string(i % 100) +
           "@" + DOMAINS[i % DOMAINS.size()] + "&password=secret" +
           std::to_string(i % 10000);
  else if (i % 5 == 0) {
    str += "/api/data?token=";
    for (int j = 0; j < (i % 3) + 1; j++)
      str += "abcdef123456";
  } else if (i % 7 == 0) {
    char hex[16];
    snprintf(hex, sizeof(hex), "%x", i * 12345);
    str += std::string("/user/profile?session_id=sess_") + hex;
  } else
    str += PATHS[i % PATHS.size()];

  str += " HTTP/1.1\" " + std::to_string(STATUSES[i % STATUSES.size()]) +
         " 2326 \"http://" + DOMAINS[i % DOMAINS.size()] + "\" \"" +
         AGENTS[i % AGENTS.size()] + "\"\n";
}

LogParser::LogParser() {
  for (int i = 0; i < PATTERNS_COUNT; ++i)
    token_counts[i] = 0;
}

std::string LogParser::name() const { return "Etc::LogParser"; }

void LogParser::prepare() {
  lines_count = config_val("lines_count");
  std::string log_builder;
  log_builder.reserve(static_cast<size_t>(lines_count) * 256);
  for (int i = 0; i < lines_count; i++)
    generate_log_line(log_builder, i);
  log = std::move(log_builder);
  log_size = log.size();

  pattern_names.clear();
  pattern_names.reserve(PATTERNS_COUNT);

  for (int i = 0; i < PATTERNS_COUNT; ++i) {
    token_counts[i] = MAX_TOKENS;
    int e = regex_parse(PATTERNS[i], compiled[i], &token_counts[i], 0);
    if (e) {
      fprintf(stderr,
              "[LogParser] regex_parse failed for pattern %d (\"%s\"): %d\n", i,
              PATTERNS[i], e);
      token_counts[i] = 0;
    }
    pattern_names.emplace_back(NAMES[i]);
  }
}

void LogParser::run(int) {
  uint32_t total = 0;

  for (int i = 0; i < PATTERNS_COUNT; ++i) {
    if (token_counts[i] <= 0)
      continue;

    int matches = 0;
    size_t offset = 0;

    while (offset < log_size) {
      const char *start = log.c_str() + offset;
      int64_t end = regex_match(compiled[i], start, 0, 0, nullptr, nullptr);

      if (end < 0) {
        offset++;
      } else {
        matches++;
        if (end > 0) {
          offset += static_cast<size_t>(end);
        } else {
          offset++;
        }
      }
    }
    total += static_cast<uint32_t>(matches);
  }

  checksum_val += total;
}

uint32_t LogParser::checksum() { return checksum_val; }

Mandelbrot::Mandelbrot() {
  w = config_val("w");
  h = config_val("h");
}

std::string Mandelbrot::name() const { return "CLBG::Mandelbrot"; }

void Mandelbrot::run(int iteration_id) {
  (void)iteration_id;
  std::ostringstream header;
  header << "P4\n" << w << " " << h << "\n";
  std::string header_str = header.str();
  result_bin.insert(result_bin.end(), header_str.begin(), header_str.end());

  int bit_num = 0;
  uint8_t byte_acc = 0;

  for (int y = 0; y < h; y++) {
    double ci = 2.0 * static_cast<double>(y) / static_cast<double>(h) - 1.0;
    for (int x = 0; x < w; x++) {
      double cr = 2.0 * static_cast<double>(x) / static_cast<double>(w) - 1.5;

      double zr = 0.0, zi = 0.0;
      double tr = 0.0, ti = 0.0;

      int i = 0;
      while (i < ITER && tr + ti <= LIMIT * LIMIT) {
        zi = 2.0 * zr * zi + ci;
        zr = tr - ti + cr;
        tr = zr * zr;
        ti = zi * zi;
        i++;
      }

      byte_acc <<= 1;
      if (tr + ti <= LIMIT * LIMIT) {
        byte_acc |= 0x01;
      }
      bit_num++;

      if (bit_num == 8) {
        result_bin.push_back(byte_acc);
        byte_acc = 0;
        bit_num = 0;
      } else if (x == w - 1) {
        byte_acc <<= (8 - (w % 8));
        result_bin.push_back(byte_acc);
        byte_acc = 0;
        bit_num = 0;
      }
    }
  }
}

uint32_t Mandelbrot::checksum() { return Helper::checksum(result_bin); }

Matmul1T::Matmul1T() : result_val(0) {}

std::string Matmul1T::name() const { return "Matmul::Single"; }

std::vector<std::vector<double>> Matmul1T::matgen(int n) {
  double tmp = 1.0 / n / n;
  std::vector<std::vector<double>> a(n, std::vector<double>(n));
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      a[i][j] = tmp * (i - j) * (i + j);
    }
  }
  return a;
}

std::vector<std::vector<double>>
Matmul1T::matmul(int n, const std::vector<std::vector<double>> &a,
                 const std::vector<std::vector<double>> &b) {

  std::vector<std::vector<double>> b_t(n, std::vector<double>(n));
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      b_t[j][i] = b[i][j];
    }
  }

  std::vector<std::vector<double>> c(n, std::vector<double>(n));
  for (int i = 0; i < n; i++) {
    const auto &ai = a[i];
    auto &ci = c[i];
    for (int j = 0; j < n; j++) {
      double s = 0.0;
      const auto &b_tj = b_t[j];
      for (int k = 0; k < n; k++) {
        s += ai[k] * b_tj[k];
      }
      ci[j] = s;
    }
  }
  return c;
}

void Matmul1T::prepare() {
  int n = static_cast<int>(config_val("n"));
  a = matgen(n);
  b = matgen(n);
}

void Matmul1T::run(int) {
  int n = static_cast<int>(a.size());
  auto c = matmul(n, a, b);
  result_val += Helper::checksum_f64(c[n >> 1][n >> 1]);
}

uint32_t Matmul1T::checksum() { return result_val; }

std::string Matmul4T::name() const { return "Matmul::T4"; }

std::vector<std::vector<double>>
Matmul4T::matmul_parallel(int n, const std::vector<std::vector<double>> &a,
                          const std::vector<std::vector<double>> &b) {
  int num_threads = get_num_threads();

  std::vector<std::vector<double>> b_t(n, std::vector<double>(n));
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      b_t[j][i] = b[i][j];
    }
  }

  std::vector<std::vector<double>> c(n, std::vector<double>(n));
  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  int rows_per_thread = (n + num_threads - 1) / num_threads;

  for (int t = 0; t < num_threads; t++) {
    int start = t * rows_per_thread;
    int end = std::min(start + rows_per_thread, n);

    threads.emplace_back([&, start, end]() {
      for (int i = start; i < end; i++) {
        const auto &ai = a[i];
        auto &ci = c[i];
        for (int j = 0; j < n; j++) {
          double sum = 0.0;
          const auto &b_tj = b_t[j];
          for (int k = 0; k < n; k++) {
            sum += ai[k] * b_tj[k];
          }
          ci[j] = sum;
        }
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }
  return c;
}

void Matmul4T::run(int) {
  int n = static_cast<int>(a.size());
  auto c = matmul_parallel(n, a, b);
  result_val += Helper::checksum_f64(c[n >> 1][n >> 1]);
}

std::string Matmul8T::name() const { return "Matmul::T8"; }

std::string Matmul16T::name() const { return "Matmul::T16"; }

MazeGenerator::Cell::Cell(int x, int y) : kind(CellKind::Wall), x(x), y(y) {
  neighbors.reserve(4);
}

bool MazeGenerator::Cell::is_walkable() const {
  return kind == CellKind::Space || kind == CellKind::Start ||
         kind == CellKind::Finish;
}

void MazeGenerator::Cell::reset() {
  if (kind == CellKind::Space)
    kind = CellKind::Wall;
}

uint32_t MazeGenerator::Cell::value() const {
  return static_cast<uint32_t>(kind);
}

MazeGenerator::Maze::Maze(int width, int height) : w(width), h(height) {
  cells.reserve(h);
  for (int y = 0; y < h; ++y) {
    auto &row = cells.emplace_back();
    row.reserve(w);
    for (int x = 0; x < w; ++x) {
      row.emplace_back(x, y);
    }
  }

  start = &cells[1][1];
  finish = &cells[h - 2][w - 2];
  start->kind = CellKind::Start;
  finish->kind = CellKind::Finish;

  update_neighbors();
}

void MazeGenerator::Maze::update_neighbors() {
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      auto &cell = cells[y][x];

      if (x > 0 && y > 0 && x < w - 1 && y < h - 1) {
        cell.neighbors = {&cells[y - 1][x], &cells[y + 1][x], &cells[y][x + 1],
                          &cells[y][x - 1]};

        for (int t = 0; t < 4; ++t) {
          int i = Helper::next_int(4);
          int j = Helper::next_int(4);
          if (i != j)
            std::swap(cell.neighbors[i], cell.neighbors[j]);
        }
      } else {
        cell.kind = CellKind::Border;
      }
    }
  }
}

void MazeGenerator::Maze::reset() {
  for (auto &row : cells)
    for (auto &cell : row)
      cell.reset();
  start->kind = CellKind::Start;
  finish->kind = CellKind::Finish;
}

void MazeGenerator::Maze::dig(Cell *start_cell) {
  if (!start_cell)
    return;
  std::vector<Cell *> stack;
  stack.reserve(static_cast<size_t>(w) * static_cast<size_t>(h));
  stack.push_back(start_cell);

  while (!stack.empty()) {
    auto *cell = stack.back();
    stack.pop_back();

    int walkable = 0;
    for (auto *n : cell->neighbors)
      if (n->is_walkable())
        ++walkable;
    if (walkable != 1)
      continue;

    cell->kind = CellKind::Space;
    for (auto *n : cell->neighbors)
      if (n->kind == CellKind::Wall)
        stack.push_back(n);
  }
}

void MazeGenerator::Maze::ensure_open_finish(Cell *cell) {
  if (!cell)
    return;
  cell->kind = CellKind::Space;

  int walkable = 0;
  for (auto *n : cell->neighbors)
    if (n->is_walkable())
      ++walkable;

  if (walkable > 1)
    return;

  for (auto *n : cell->neighbors)
    if (n->kind == CellKind::Wall)
      ensure_open_finish(n);
}

void MazeGenerator::Maze::generate() {
  for (auto *n : start->neighbors)
    if (n->kind == CellKind::Wall)
      dig(n);
  for (auto *n : finish->neighbors)
    if (n->kind == CellKind::Wall)
      ensure_open_finish(n);
}

MazeGenerator::Cell *MazeGenerator::Maze::middle_cell() {
  return &cells[h / 2][w / 2];
}
MazeGenerator::Cell *MazeGenerator::Maze::get_start() { return start; }
MazeGenerator::Cell *MazeGenerator::Maze::get_finish() { return finish; }

MazeGenerator::Cell *MazeGenerator::Maze::get_cell(int x, int y) {
  if (x >= 0 && x < w && y >= 0 && y < h)
    return &cells[y][x];
  return nullptr;
}

uint32_t MazeGenerator::Maze::checksum() const {
  uint32_t hasher = 2166136261UL;
  uint32_t prime = 16777619UL;
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
      if (cells[y][x].kind == CellKind::Space) {
        uint32_t val = static_cast<uint32_t>(x * y);
        hasher = (hasher ^ val) * prime;
      }
  return hasher;
}

void MazeGenerator::Maze::print_to_console() const {
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      switch (cells[y][x].kind) {
      case CellKind::Space:
        std::cout << ' ';
        break;
      case CellKind::Wall:
        std::cout << "\033[34m#\033[0m";
        break;
      case CellKind::Border:
        std::cout << "\033[31mO\033[0m";
        break;
      case CellKind::Start:
        std::cout << "\033[32m>\033[0m";
        break;
      case CellKind::Finish:
        std::cout << "\033[32m<\033[0m";
        break;
      case CellKind::Path:
        std::cout << "\033[33m.\033[0m";
        break;
      }
    }
    std::cout << '\n';
  }
  std::cout << '\n';
}

MazeGenerator::MazeGenerator() : result_val(0) {
  width = static_cast<int32_t>(config_val("w"));
  height = static_cast<int32_t>(config_val("h"));
  if (width < 5)
    width = 5;
  if (height < 5)
    height = 5;
  maze = std::make_unique<Maze>(width, height);
}

std::string MazeGenerator::name() const { return "Maze::Generator"; }

void MazeGenerator::prepare() {}

void MazeGenerator::run(int) {
  maze->reset();
  maze->generate();
  result_val += maze->middle_cell()->value();
}

uint32_t MazeGenerator::checksum() { return result_val + maze->checksum(); }

MazeBFS::MazeBFS() : result_val(0) {
  width = static_cast<int32_t>(config_val("w"));
  height = static_cast<int32_t>(config_val("h"));
  if (width < 5)
    width = 5;
  if (height < 5)
    height = 5;
  maze = std::make_unique<MazeGenerator::Maze>(width, height);
}

std::string MazeBFS::name() const { return "Maze::BFS"; }

void MazeBFS::prepare() { maze->generate(); }

std::vector<MazeGenerator::Cell *> MazeBFS::bfs(MazeGenerator::Cell *start,
                                                MazeGenerator::Cell *target) {
  if (start == target)
    return {start};

  struct PathNode {
    MazeGenerator::Cell *cell;
    int parent;
  };
  std::deque<int> queue;
  std::vector<std::vector<bool>> visited(height,
                                         std::vector<bool>(width, false));
  std::vector<PathNode> path_nodes;

  visited[start->y][start->x] = true;
  path_nodes.push_back({start, -1});
  queue.push_back(0);

  while (!queue.empty()) {
    int path_id = queue.front();
    queue.pop_front();
    auto *cell = path_nodes[path_id].cell;

    for (auto *neighbor : cell->neighbors) {
      if (neighbor == target) {
        std::vector<MazeGenerator::Cell *> result = {target};
        int current = path_id;
        while (current >= 0) {
          result.push_back(path_nodes[current].cell);
          current = path_nodes[current].parent;
        }
        std::reverse(result.begin(), result.end());
        return result;
      }
      if (neighbor->is_walkable() && !visited[neighbor->y][neighbor->x]) {
        visited[neighbor->y][neighbor->x] = true;
        path_nodes.push_back({neighbor, path_id});
        queue.push_back(path_nodes.size() - 1);
      }
    }
  }
  return {};
}

uint32_t
MazeBFS::mid_cell_checksum(const std::vector<MazeGenerator::Cell *> &p) {
  if (p.empty())
    return 0;
  size_t mid = p.size() / 2;
  auto *cell = p[mid];
  return static_cast<uint32_t>(cell->x * cell->y);
}

void MazeBFS::run(int) {
  path = bfs(maze->get_start(), maze->get_finish());
  result_val += static_cast<uint32_t>(path.size());
}

uint32_t MazeBFS::checksum() { return result_val + mid_cell_checksum(path); }

bool MazeAStar::Node::operator>(const Node &other) const {
  if (f_score != other.f_score)
    return f_score > other.f_score;
  return idx > other.idx;
}

MazeAStar::MazeAStar() : result_val(0) {
  width = static_cast<int32_t>(config_val("w"));
  height = static_cast<int32_t>(config_val("h"));
  if (width < 5)
    width = 5;
  if (height < 5)
    height = 5;
  maze = std::make_unique<MazeGenerator::Maze>(width, height);
}

std::string MazeAStar::name() const { return "Maze::AStar"; }

void MazeAStar::prepare() { maze->generate(); }

int MazeAStar::heuristic(MazeGenerator::Cell *a, MazeGenerator::Cell *b) {
  return std::abs(a->x - b->x) + std::abs(a->y - b->y);
}

int MazeAStar::idx(int y, int x) const { return y * width + x; }

std::vector<MazeGenerator::Cell *>
MazeAStar::astar(MazeGenerator::Cell *start, MazeGenerator::Cell *target) {
  if (start == target)
    return {start};

  int size = width * height;
  std::vector<int> came_from(size, -1);
  std::vector<int> g_score(size, std::numeric_limits<int>::max());
  std::vector<int> best_f(size, std::numeric_limits<int>::max());

  int start_idx = idx(start->y, start->x);
  int target_idx = idx(target->y, target->x);

  std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open_set;

  g_score[start_idx] = 0;
  int f_start = heuristic(start, target);
  open_set.push({f_start, start_idx});
  best_f[start_idx] = f_start;

  while (!open_set.empty()) {
    auto [f_val, current_idx] = open_set.top();
    open_set.pop();

    if (f_val != best_f[current_idx])
      continue;

    if (current_idx == target_idx) {
      std::vector<MazeGenerator::Cell *> result;
      int cur = current_idx;
      while (cur != -1) {
        int y = cur / width, x = cur % width;
        result.push_back(maze->get_cell(x, y));
        cur = came_from[cur];
      }
      std::reverse(result.begin(), result.end());
      return result;
    }

    int current_y = current_idx / width, current_x = current_idx % width;
    auto *current = maze->get_cell(current_x, current_y);
    int current_g = g_score[current_idx];

    for (auto *neighbor : current->neighbors) {
      if (!neighbor->is_walkable())
        continue;
      int neighbor_idx = idx(neighbor->y, neighbor->x);
      int tentative_g = current_g + 1;

      if (tentative_g < g_score[neighbor_idx]) {
        came_from[neighbor_idx] = current_idx;
        g_score[neighbor_idx] = tentative_g;
        int f_new = tentative_g + heuristic(neighbor, target);

        if (f_new < best_f[neighbor_idx]) {
          best_f[neighbor_idx] = f_new;
          open_set.push({f_new, neighbor_idx});
        }
      }
    }
  }
  return {};
}

uint32_t
MazeAStar::mid_cell_checksum(const std::vector<MazeGenerator::Cell *> &p) {
  if (p.empty())
    return 0;
  size_t mid = p.size() / 2;
  auto *cell = p[mid];
  return static_cast<uint32_t>(cell->x * cell->y);
}

void MazeAStar::run(int) {
  path = astar(maze->get_start(), maze->get_finish());
  result_val += static_cast<uint32_t>(path.size());
}

uint32_t MazeAStar::checksum() { return result_val + mid_cell_checksum(path); }

void Nbody::Planet::move_from_i(std::vector<Planet> &bodies, int nbodies,
                                double dt, int start) {
  for (int i = start; i < nbodies; i++) {
    Planet &b2 = bodies[i];
    double dx = x - b2.x;
    double dy = y - b2.y;
    double dz = z - b2.z;

    double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    double mag = dt / (distance * distance * distance);
    double b_mass_mag = mass * mag;
    double b2_mass_mag = b2.mass * mag;

    vx -= dx * b2_mass_mag;
    vy -= dy * b2_mass_mag;
    vz -= dz * b2_mass_mag;
    b2.vx += dx * b_mass_mag;
    b2.vy += dy * b_mass_mag;
    b2.vz += dz * b_mass_mag;
  }

  x += dt * vx;
  y += dt * vy;
  z += dt * vz;
}

Nbody::Nbody() : v1(0.0) {
  bodies = {Planet(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0),
            Planet(4.84143144246472090e+00, -1.16032004402742839e+00,
                   -1.03622044471123109e-01, 1.66007664274403694e-03,
                   7.69901118419740425e-03, -6.90460016972063023e-05,
                   9.54791938424326609e-04),
            Planet(8.34336671824457987e+00, 4.12479856412430479e+00,
                   -4.03523417114321381e-01, -2.76742510726862411e-03,
                   4.99852801234917238e-03, 2.30417297573763929e-05,
                   2.85885980666130812e-04),
            Planet(1.28943695621391310e+01, -1.51111514016986312e+01,
                   -2.23307578892655734e-01, 2.96460137564761618e-03,
                   2.37847173959480950e-03, -2.96589568540237556e-05,
                   4.36624404335156298e-05),
            Planet(1.53796971148509165e+01, -2.59193146099879641e+01,
                   1.79258772950371181e-01, 2.68067772490389322e-03,
                   1.62824170038242295e-03, -9.51592254519715870e-05,
                   5.15138902046611451e-05)};
}

std::string Nbody::name() const { return "CLBG::Nbody"; }

double Nbody::energy() {
  double e = 0.0;
  int nbodies = static_cast<int>(bodies.size());

  for (int i = 0; i < nbodies; i++) {
    Planet &b = bodies[i];
    e += 0.5 * b.mass * (b.vx * b.vx + b.vy * b.vy + b.vz * b.vz);
    for (int j = i + 1; j < nbodies; j++) {
      Planet &b2 = bodies[j];
      double dx = b.x - b2.x;
      double dy = b.y - b2.y;
      double dz = b.z - b2.z;
      double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
      e -= (b.mass * b2.mass) / distance;
    }
  }
  return e;
}

void Nbody::offset_momentum() {
  double px = 0.0, py = 0.0, pz = 0.0;

  for (auto &b : bodies) {
    px += b.vx * b.mass;
    py += b.vy * b.mass;
    pz += b.vz * b.mass;
  }

  Planet &b = bodies[0];
  b.vx = -px / SOLAR_MASS;
  b.vy = -py / SOLAR_MASS;
  b.vz = -pz / SOLAR_MASS;
}

void Nbody::prepare() {
  offset_momentum();
  v1 = energy();
}

void Nbody::run(int iteration_id) {
  (void)iteration_id;
  int nbodies = static_cast<int>(bodies.size());
  double dt = 0.01;

  for (int n = 0; n < 1000; n++) {
    int i = 0;
    while (i < nbodies) {
      Planet &b = bodies[i];
      b.move_from_i(bodies, nbodies, dt, i + 1);
      i++;
    }
  }
}

uint32_t Nbody::checksum() {
  double v2 = energy();
  return (Helper::checksum_f64(v1) << 5) & Helper::checksum_f64(v2);
}

NeuralNet::Synapse::Synapse(Neuron *source, Neuron *dest)
    : source_neuron(source), dest_neuron(dest) {
  weight = prev_weight = Helper::next_float() * 2 - 1;
}

NeuralNet::Neuron::Neuron() {
  threshold = prev_threshold = Helper::next_float() * 2 - 1;
  output = 0.0;
  error = 0.0;
}

void NeuralNet::Neuron::calculate_output() {
  double activation = 0.0;
  for (auto synapse : synapses_in) {
    activation += synapse->weight * synapse->source_neuron->output;
  }
  activation -= threshold;
  output = 1.0 / (1.0 + std::exp(-activation));
}

void NeuralNet::Neuron::output_train(double rate, double target) {
  error = (target - output) * derivative();
  update_weights(rate);
}

void NeuralNet::Neuron::hidden_train(double rate) {
  double sum = 0.0;
  for (auto synapse : synapses_out) {
    sum += synapse->prev_weight * synapse->dest_neuron->error;
  }
  error = sum * derivative();
  update_weights(rate);
}

void NeuralNet::Neuron::update_weights(double rate) {
  for (auto synapse : synapses_in) {
    double temp_weight = synapse->weight;
    synapse->weight +=
        (rate * LEARNING_RATE * error * synapse->source_neuron->output) +
        (MOMENTUM * (synapse->weight - synapse->prev_weight));
    synapse->prev_weight = temp_weight;
  }

  double temp_threshold = threshold;
  threshold += (rate * LEARNING_RATE * error * -1) +
               (MOMENTUM * (threshold - prev_threshold));
  prev_threshold = temp_threshold;
}

NeuralNet::NeuralNetwork::NeuralNetwork(int inputs, int hidden, int outputs)
    : input_layer(inputs), hidden_layer(hidden), output_layer(outputs) {

  for (auto &source : input_layer) {
    for (auto &dest : hidden_layer) {
      auto synapse = std::make_unique<Synapse>(&source, &dest);
      source.add_synapse_out(synapse.get());
      dest.add_synapse_in(synapse.get());
      synapses.push_back(std::move(synapse));
    }
  }

  for (auto &source : hidden_layer) {
    for (auto &dest : output_layer) {
      auto synapse = std::make_unique<Synapse>(&source, &dest);
      source.add_synapse_out(synapse.get());
      dest.add_synapse_in(synapse.get());
      synapses.push_back(std::move(synapse));
    }
  }
}

void NeuralNet::NeuralNetwork::train(const std::vector<double> &inputs,
                                     const std::vector<double> &targets) {
  feed_forward(inputs);

  for (size_t i = 0; i < output_layer.size(); i++) {
    output_layer[i].output_train(0.3, targets[i]);
  }

  for (auto &neuron : hidden_layer) {
    neuron.hidden_train(0.3);
  }
}

void NeuralNet::NeuralNetwork::feed_forward(const std::vector<double> &inputs) {
  for (size_t i = 0; i < input_layer.size(); i++) {
    input_layer[i].set_output(inputs[i]);
  }

  for (auto &neuron : hidden_layer) {
    neuron.calculate_output();
  }

  for (auto &neuron : output_layer) {
    neuron.calculate_output();
  }
}

std::vector<double> NeuralNet::NeuralNetwork::current_outputs() {
  std::vector<double> outputs;
  outputs.reserve(output_layer.size());
  for (const auto &neuron : output_layer) {
    outputs.push_back(neuron.get_output());
  }
  return outputs;
}

NeuralNet::NeuralNet() { xor_net = std::make_unique<NeuralNetwork>(0, 0, 0); }

std::string NeuralNet::name() const { return "Etc::NeuralNet"; }

void NeuralNet::prepare() {
  xor_net = std::make_unique<NeuralNetwork>(2, 10, 1);
}

void NeuralNet::run(int iteration_id) {
  (void)iteration_id;
  NeuralNetwork &xor_ref = *xor_net;

  static const std::vector<double> INPUT_00 = {0, 0};
  static const std::vector<double> INPUT_01 = {0, 1};
  static const std::vector<double> INPUT_10 = {1, 0};
  static const std::vector<double> INPUT_11 = {1, 1};
  static const std::vector<double> TARGET_0 = {0};
  static const std::vector<double> TARGET_1 = {1};

  for (int iter = 0; iter < 1000; iter++) {
    xor_ref.train(INPUT_00, TARGET_0);
    xor_ref.train(INPUT_10, TARGET_1);
    xor_ref.train(INPUT_01, TARGET_1);
    xor_ref.train(INPUT_11, TARGET_0);
  }
}

uint32_t NeuralNet::checksum() {
  xor_net->feed_forward({0, 0});
  auto outputs1 = xor_net->current_outputs();

  xor_net->feed_forward({0, 1});
  auto outputs2 = xor_net->current_outputs();

  xor_net->feed_forward({1, 0});
  auto outputs3 = xor_net->current_outputs();

  xor_net->feed_forward({1, 1});
  auto outputs4 = xor_net->current_outputs();

  std::vector<double> all_outputs;
  all_outputs.insert(all_outputs.end(), outputs1.begin(), outputs1.end());
  all_outputs.insert(all_outputs.end(), outputs2.begin(), outputs2.end());
  all_outputs.insert(all_outputs.end(), outputs3.begin(), outputs3.end());
  all_outputs.insert(all_outputs.end(), outputs4.begin(), outputs4.end());

  double sum = 0.0;
  for (double v : all_outputs)
    sum += v;
  return Helper::checksum_f64(sum);
}

Sieve::Sieve() : limit(config_val("limit")), checksum_val(0) {}

std::string Sieve::name() const { return "Etc::Sieve"; }

void Sieve::run(int iteration_id) {
  (void)iteration_id;
  size_t sz = static_cast<size_t>(limit);
  std::vector<uint8_t> primes(sz + 1, 1);
  primes[0] = 0;
  primes[1] = 0;

  size_t sqrt_limit =
      static_cast<size_t>(std::sqrt(static_cast<double>(limit)));

  for (size_t p = 2; p <= sqrt_limit; ++p) {
    if (primes[p] == 1) {
      for (size_t multiple = p * p; multiple <= sz; multiple += p) {
        primes[multiple] = 0;
      }
    }
  }

  int last_prime = 2;
  int count = 1;

  for (size_t n = 3; n <= sz; n += 2) {
    if (primes[n] == 1) {
      last_prime = static_cast<int>(n);
      count++;
    }
  }

  checksum_val += static_cast<uint32_t>(last_prime + count);
}

uint32_t Sieve::checksum() { return checksum_val; }

SortBenchmark::SortBenchmark() : size_val(0), result_val(0) {}

void SortBenchmark::prepare() {
  if (size_val == 0) {
    size_val = config_val("size");
    data.reserve(static_cast<size_t>(size_val));
    for (int64_t i = 0; i < size_val; i++) {
      data.push_back(Helper::next_int(1'000'000));
    }
  }
}

void SortBenchmark::run(int iteration_id) {
  (void)iteration_id;
  result_val += data[Helper::next_int(static_cast<int32_t>(size_val))];
  std::vector<int32_t> t = test();
  result_val += t[Helper::next_int(static_cast<int32_t>(size_val))];
}

uint32_t SortBenchmark::checksum() { return result_val; }

std::string SortQuick::name() const { return "Sort::Quick"; }

void SortQuick::quick_sort(std::vector<int32_t> &arr, int low, int high) {
  if (low >= high)
    return;

  int pivot = arr[(low + high) / 2];
  int i = low, j = high;

  while (i <= j) {
    while (arr[i] < pivot)
      i++;
    while (arr[j] > pivot)
      j--;
    if (i <= j) {
      std::swap(arr[i], arr[j]);
      i++;
      j--;
    }
  }

  quick_sort(arr, low, j);
  quick_sort(arr, i, high);
}

std::vector<int32_t> SortQuick::test() {
  std::vector<int32_t> arr = data;
  quick_sort(arr, 0, static_cast<int>(arr.size() - 1));
  return arr;
}

std::string SortMerge::name() const { return "Sort::Merge"; }

void SortMerge::merge_sort_inplace(std::vector<int32_t> &arr) {
  std::vector<int32_t> temp(arr.size());
  merge_sort_helper(arr, temp, 0, static_cast<int>(arr.size() - 1));
}

void SortMerge::merge_sort_helper(std::vector<int32_t> &arr,
                                  std::vector<int32_t> &temp, int left,
                                  int right) {
  if (left >= right)
    return;

  int mid = (left + right) / 2;
  merge_sort_helper(arr, temp, left, mid);
  merge_sort_helper(arr, temp, mid + 1, right);
  merge(arr, temp, left, mid, right);
}

void SortMerge::merge(std::vector<int32_t> &arr, std::vector<int32_t> &temp,
                      int left, int mid, int right) {
  for (int i = left; i <= right; i++) {
    temp[i] = arr[i];
  }

  int i = left;
  int j = mid + 1;
  int k = left;

  while (i <= mid && j <= right) {
    if (temp[i] <= temp[j]) {
      arr[k] = temp[i];
      i++;
    } else {
      arr[k] = temp[j];
      j++;
    }
    k++;
  }

  while (i <= mid) {
    arr[k] = temp[i];
    i++;
    k++;
  }
}

std::vector<int32_t> SortMerge::test() {
  std::vector<int32_t> arr = data;
  merge_sort_inplace(arr);
  return arr;
}

std::string SortSelf::name() const { return "Sort::Self"; }

std::vector<int32_t> SortSelf::test() {
  std::vector<int32_t> arr = data;
  std::sort(arr.begin(), arr.end());
  return arr;
}

Spectralnorm::Spectralnorm() {
  size_val = config_val("size");
  u = std::vector<double>(size_val, 1.0);
  v = std::vector<double>(size_val, 1.0);
}

std::string Spectralnorm::name() const { return "CLBG::Spectralnorm"; }

std::vector<double> Spectralnorm::eval_A_times_u(const std::vector<double> &u) {
  std::vector<double> v(u.size());
  for (size_t i = 0; i < u.size(); i++) {
    double sum = 0.0;
    for (size_t j = 0; j < u.size(); j++) {
      sum += eval_A(static_cast<int>(i), static_cast<int>(j)) * u[j];
    }
    v[i] = sum;
  }
  return v;
}

std::vector<double>
Spectralnorm::eval_At_times_u(const std::vector<double> &u) {
  std::vector<double> v(u.size());
  for (size_t i = 0; i < u.size(); i++) {
    double sum = 0.0;
    for (size_t j = 0; j < u.size(); j++) {
      sum += eval_A(static_cast<int>(j), static_cast<int>(i)) * u[j];
    }
    v[i] = sum;
  }
  return v;
}

std::vector<double>
Spectralnorm::eval_AtA_times_u(const std::vector<double> &u) {
  return eval_At_times_u(eval_A_times_u(u));
}

void Spectralnorm::run(int iteration_id) {
  (void)iteration_id;
  v = eval_AtA_times_u(u);
  u = eval_AtA_times_u(v);
}

uint32_t Spectralnorm::checksum() {
  double vBv = 0.0, vv = 0.0;
  for (int i = 0; i < size_val; i++) {
    vBv += u[i] * v[i];
    vv += v[i] * v[i];
  }
  return Helper::checksum_f64(sqrt(vBv / vv));
}

// https://github.com/kokke/tiny-regex-c/blob/master/re.c
// ------------------------------------------------------------------------------------------
// This is free and unencumbered software released into the public domain.

// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// For more information, please refer to <http://unlicense.org>

/*
 *
 * Mini regex-module inspired by Rob Pike's regex code described in:
 *
 * http://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html
 *
 *
 *
 * Supports:
 * ---------
 *   '.'        Dot, matches any character
 *   '^'        Start anchor, matches beginning of string
 *   '$'        End anchor, matches end of string
 *   '*'        Asterisk, match zero or more (greedy)
 *   '+'        Plus, match one or more (greedy)
 *   '?'        Question, match zero or one (non-greedy)
 *   '[abc]'    Character class, match if one of {'a', 'b', 'c'}
 *   '[^abc]'   Inverted class, match if NOT one of {'a', 'b', 'c'} -- NOTE: feature is currently broken!
 *   '[a-zA-Z]' Character ranges, the character set of the ranges { a-z | A-Z }
 *   '\s'       Whitespace, \t \f \r \n \v and spaces
 *   '\S'       Non-whitespace
 *   '\w'       Alphanumeric, [a-zA-Z0-9_]
 *   '\W'       Non-alphanumeric
 *   '\d'       Digits, [0-9]
 *   '\D'       Non-digits
 *
 *
 */


/* Definitions: */

#define MAX_REGEXP_OBJECTS      30    /* Max number of regex symbols in expression. */
#define MAX_CHAR_CLASS_LEN      40    /* Max length of character-class buffer in.   */


enum { UNUSED, DOT, BEGIN, END, QUESTIONMARK, STAR, PLUS, CHAR, CHAR_CLASS, INV_CHAR_CLASS, DIGIT, NOT_DIGIT, ALPHA, NOT_ALPHA, WHITESPACE, NOT_WHITESPACE, /* BRANCH */ };

typedef struct regex_t
{
  unsigned char  type;   /* CHAR, STAR, etc.                      */
  union
  {
    unsigned char  ch;   /*      the character itself             */
    unsigned char* ccl;  /*  OR  a pointer to characters in class */
  } u;
} regex_t;



/* Private function declarations: */
static int matchpattern(regex_t* pattern, const char* text, int* matchlength);
static int matchcharclass(char c, const char* str);
static int matchstar(regex_t p, regex_t* pattern, const char* text, int* matchlength);
static int matchplus(regex_t p, regex_t* pattern, const char* text, int* matchlength);
static int matchone(regex_t p, char c);
static int matchdigit(char c);
static int matchalpha(char c);
static int matchwhitespace(char c);
static int matchmetachar(char c, const char* str);
static int matchrange(char c, const char* str);
static int matchdot(char c);
static int ismetachar(char c);



/* Public functions: */
int re_match(const char* pattern, const char* text, int* matchlength)
{
  return re_matchp(re_compile(pattern), text, matchlength);
}

int re_matchp(re_t pattern, const char* text, int* matchlength)
{
  *matchlength = 0;
  if (pattern != 0)
  {
    if (pattern[0].type == BEGIN)
    {
      return ((matchpattern(&pattern[1], text, matchlength)) ? 0 : -1);
    }
    else
    {
      int idx = -1;

      do
      {
        idx += 1;

        if (matchpattern(pattern, text, matchlength))
        {
          if (text[0] == '\0')
            return -1;

          return idx;
        }
      }
      while (*text++ != '\0');
    }
  }
  return -1;
}

re_t re_compile(const char* pattern)
{
  /* The sizes of the two static arrays below substantiates the static RAM usage of this module.
     MAX_REGEXP_OBJECTS is the max number of symbols in the expression.
     MAX_CHAR_CLASS_LEN determines the size of buffer for chars in all char-classes in the expression. */
  static regex_t re_compiled[MAX_REGEXP_OBJECTS];
  static unsigned char ccl_buf[MAX_CHAR_CLASS_LEN];
  int ccl_bufidx = 1;

  char c;     /* current char in pattern   */
  int i = 0;  /* index into pattern        */
  int j = 0;  /* index into re_compiled    */

  while (pattern[i] != '\0' && (j+1 < MAX_REGEXP_OBJECTS))
  {
    c = pattern[i];

    switch (c)
    {
      /* Meta-characters: */
      case '^': {    re_compiled[j].type = BEGIN;           } break;
      case '$': {    re_compiled[j].type = END;             } break;
      case '.': {    re_compiled[j].type = DOT;             } break;
      case '*': {    re_compiled[j].type = STAR;            } break;
      case '+': {    re_compiled[j].type = PLUS;            } break;
      case '?': {    re_compiled[j].type = QUESTIONMARK;    } break;
/*    case '|': {    re_compiled[j].type = BRANCH;          } break; <-- not working properly */

      /* Escaped character-classes (\s \w ...): */
      case '\\':
      {
        if (pattern[i+1] != '\0')
        {
          /* Skip the escape-char '\\' */
          i += 1;
          /* ... and check the next */
          switch (pattern[i])
          {
            /* Meta-character: */
            case 'd': {    re_compiled[j].type = DIGIT;            } break;
            case 'D': {    re_compiled[j].type = NOT_DIGIT;        } break;
            case 'w': {    re_compiled[j].type = ALPHA;            } break;
            case 'W': {    re_compiled[j].type = NOT_ALPHA;        } break;
            case 's': {    re_compiled[j].type = WHITESPACE;       } break;
            case 'S': {    re_compiled[j].type = NOT_WHITESPACE;   } break;

            /* Escaped character, e.g. '.' or '$' */
            default:
            {
              re_compiled[j].type = CHAR;
              re_compiled[j].u.ch = pattern[i];
            } break;
          }
        }
        /* '\\' as last char in pattern -> invalid regular expression. */
/*
        else
        {
          re_compiled[j].type = CHAR;
          re_compiled[j].ch = pattern[i];
        }
*/
      } break;

      /* Character class: */
      case '[':
      {
        /* Remember where the char-buffer starts. */
        int buf_begin = ccl_bufidx;

        /* Look-ahead to determine if negated */
        if (pattern[i+1] == '^')
        {
          re_compiled[j].type = INV_CHAR_CLASS;
          i += 1; /* Increment i to avoid including '^' in the char-buffer */
          if (pattern[i+1] == 0) /* incomplete pattern, missing non-zero char after '^' */
          {
            return 0;
          }
        }
        else
        {
          re_compiled[j].type = CHAR_CLASS;
        }

        /* Copy characters inside [..] to buffer */
        while (    (pattern[++i] != ']')
                && (pattern[i]   != '\0')) /* Missing ] */
        {
          if (pattern[i] == '\\')
          {
            if (ccl_bufidx >= MAX_CHAR_CLASS_LEN - 1)
            {
              //fputs("exceeded internal buffer!\n", stderr);
              return 0;
            }
            if (pattern[i+1] == 0) /* incomplete pattern, missing non-zero char after '\\' */
            {
              return 0;
            }
            ccl_buf[ccl_bufidx++] = pattern[i++];
          }
          else if (ccl_bufidx >= MAX_CHAR_CLASS_LEN)
          {
              //fputs("exceeded internal buffer!\n", stderr);
              return 0;
          }
          ccl_buf[ccl_bufidx++] = pattern[i];
        }
        if (ccl_bufidx >= MAX_CHAR_CLASS_LEN)
        {
            /* Catches cases such as [00000000000000000000000000000000000000][ */
            //fputs("exceeded internal buffer!\n", stderr);
            return 0;
        }
        /* Null-terminate string end */
        ccl_buf[ccl_bufidx++] = 0;
        re_compiled[j].u.ccl = &ccl_buf[buf_begin];
      } break;

      /* Other characters: */
      default:
      {
        re_compiled[j].type = CHAR;
        re_compiled[j].u.ch = c;
      } break;
    }
    /* no buffer-out-of-bounds access on invalid patterns - see https://github.com/kokke/tiny-regex-c/commit/1a279e04014b70b0695fba559a7c05d55e6ee90b */
    if (pattern[i] == 0)
    {
      return 0;
    }

    i += 1;
    j += 1;
  }
  /* 'UNUSED' is a sentinel used to indicate end-of-pattern */
  re_compiled[j].type = UNUSED;

  return (re_t) re_compiled;
}

void re_print(regex_t* pattern)
{
  const char* types[] = { "UNUSED", "DOT", "BEGIN", "END", "QUESTIONMARK", "STAR", "PLUS", "CHAR", "CHAR_CLASS", "INV_CHAR_CLASS", "DIGIT", "NOT_DIGIT", "ALPHA", "NOT_ALPHA", "WHITESPACE", "NOT_WHITESPACE", "BRANCH" };

  int i;
  int j;
  char c;
  for (i = 0; i < MAX_REGEXP_OBJECTS; ++i)
  {
    if (pattern[i].type == UNUSED)
    {
      break;
    }

    printf("type: %s", types[pattern[i].type]);
    if (pattern[i].type == CHAR_CLASS || pattern[i].type == INV_CHAR_CLASS)
    {
      printf(" [");
      for (j = 0; j < MAX_CHAR_CLASS_LEN; ++j)
      {
        c = pattern[i].u.ccl[j];
        if ((c == '\0') || (c == ']'))
        {
          break;
        }
        printf("%c", c);
      }
      printf("]");
    }
    else if (pattern[i].type == CHAR)
    {
      printf(" '%c'", pattern[i].u.ch);
    }
    printf("\n");
  }
}



/* Private functions: */
static int matchdigit(char c)
{
  return isdigit(c);
}
static int matchalpha(char c)
{
  return isalpha(c);
}
static int matchwhitespace(char c)
{
  return isspace(c);
}
static int matchalphanum(char c)
{
  return ((c == '_') || matchalpha(c) || matchdigit(c));
}
static int matchrange(char c, const char* str)
{
  return (    (c != '-')
           && (str[0] != '\0')
           && (str[0] != '-')
           && (str[1] == '-')
           && (str[2] != '\0')
           && (    (c >= str[0])
                && (c <= str[2])));
}
static int matchdot(char c)
{
#if defined(RE_DOT_MATCHES_NEWLINE) && (RE_DOT_MATCHES_NEWLINE == 1)
  (void)c;
  return 1;
#else
  return c != '\n' && c != '\r';
#endif
}
static int ismetachar(char c)
{
  return ((c == 's') || (c == 'S') || (c == 'w') || (c == 'W') || (c == 'd') || (c == 'D'));
}

static int matchmetachar(char c, const char* str)
{
  switch (str[0])
  {
    case 'd': return  matchdigit(c);
    case 'D': return !matchdigit(c);
    case 'w': return  matchalphanum(c);
    case 'W': return !matchalphanum(c);
    case 's': return  matchwhitespace(c);
    case 'S': return !matchwhitespace(c);
    default:  return (c == str[0]);
  }
}

static int matchcharclass(char c, const char* str)
{
  do
  {
    if (matchrange(c, str))
    {
      return 1;
    }
    else if (str[0] == '\\')
    {
      /* Escape-char: increment str-ptr and match on next char */
      str += 1;
      if (matchmetachar(c, str))
      {
        return 1;
      }
      else if ((c == str[0]) && !ismetachar(c))
      {
        return 1;
      }
    }
    else if (c == str[0])
    {
      if (c == '-')
      {
        return ((str[-1] == '\0') || (str[1] == '\0'));
      }
      else
      {
        return 1;
      }
    }
  }
  while (*str++ != '\0');

  return 0;
}

static int matchone(regex_t p, char c)
{
  switch (p.type)
  {
    case DOT:            return matchdot(c);
    case CHAR_CLASS:     return  matchcharclass(c, (const char*)p.u.ccl);
    case INV_CHAR_CLASS: return !matchcharclass(c, (const char*)p.u.ccl);
    case DIGIT:          return  matchdigit(c);
    case NOT_DIGIT:      return !matchdigit(c);
    case ALPHA:          return  matchalphanum(c);
    case NOT_ALPHA:      return !matchalphanum(c);
    case WHITESPACE:     return  matchwhitespace(c);
    case NOT_WHITESPACE: return !matchwhitespace(c);
    default:             return  (p.u.ch == c);
  }
}

static int matchstar(regex_t p, regex_t* pattern, const char* text, int* matchlength)
{
  int prelen = *matchlength;
  const char* prepoint = text;
  while ((text[0] != '\0') && matchone(p, *text))
  {
    text++;
    (*matchlength)++;
  }
  while (text >= prepoint)
  {
    if (matchpattern(pattern, text--, matchlength))
      return 1;
    (*matchlength)--;
  }

  *matchlength = prelen;
  return 0;
}

static int matchplus(regex_t p, regex_t* pattern, const char* text, int* matchlength)
{
  const char* prepoint = text;
  while ((text[0] != '\0') && matchone(p, *text))
  {
    text++;
    (*matchlength)++;
  }
  while (text > prepoint)
  {
    if (matchpattern(pattern, text--, matchlength))
      return 1;
    (*matchlength)--;
  }

  return 0;
}

static int matchquestion(regex_t p, regex_t* pattern, const char* text, int* matchlength)
{
  if (p.type == UNUSED)
    return 1;
  if (matchpattern(pattern, text, matchlength))
      return 1;
  if (*text && matchone(p, *text++))
  {
    if (matchpattern(pattern, text, matchlength))
    {
      (*matchlength)++;
      return 1;
    }
  }
  return 0;
}


#if 0

/* Recursive matching */
static int matchpattern(regex_t* pattern, const char* text, int *matchlength)
{
  int pre = *matchlength;
  if ((pattern[0].type == UNUSED) || (pattern[1].type == QUESTIONMARK))
  {
    return matchquestion(pattern[1], &pattern[2], text, matchlength);
  }
  else if (pattern[1].type == STAR)
  {
    return matchstar(pattern[0], &pattern[2], text, matchlength);
  }
  else if (pattern[1].type == PLUS)
  {
    return matchplus(pattern[0], &pattern[2], text, matchlength);
  }
  else if ((pattern[0].type == END) && pattern[1].type == UNUSED)
  {
    return text[0] == '\0';
  }
  else if ((text[0] != '\0') && matchone(pattern[0], text[0]))
  {
    (*matchlength)++;
    return matchpattern(&pattern[1], text+1);
  }
  else
  {
    *matchlength = pre;
    return 0;
  }
}

#else

/* Iterative matching */
static int matchpattern(regex_t* pattern, const char* text, int* matchlength)
{
  int pre = *matchlength;
  do
  {
    if ((pattern[0].type == UNUSED) || (pattern[1].type == QUESTIONMARK))
    {
      return matchquestion(pattern[0], &pattern[2], text, matchlength);
    }
    else if (pattern[1].type == STAR)
    {
      return matchstar(pattern[0], &pattern[2], text, matchlength);
    }
    else if (pattern[1].type == PLUS)
    {
      return matchplus(pattern[0], &pattern[2], text, matchlength);
    }
    else if ((pattern[0].type == END) && pattern[1].type == UNUSED)
    {
      return (text[0] == '\0');
    }
/*  Branching is not working properly
    else if (pattern[1].type == BRANCH)
    {
      return (matchpattern(pattern, text) || matchpattern(&pattern[2], text));
    }
*/
  (*matchlength)++;
  }
  while ((text[0] != '\0') && matchone(*pattern++, *text++));

  *matchlength = pre;
  return 0;
}

#endif

// ------------------------------------------------------------------------------------------


static const std::vector<std::string> FIRST_NAMES = {
    "John", "Jane", "Bob", "Alice", "Charlie", "Diana", "Sarah", "Mike"};
static const std::vector<std::string> LAST_NAMES = {
    "Smith",  "Johnson", "Brown",  "Taylor",
    "Wilson", "Davis",   "Miller", "Jones"};
static const std::vector<std::string> CITIES = {"New York", "Los Angeles",
                                                "Chicago",  "Houston",
                                                "Phoenix",  "San Francisco"};
static const std::string LOREM =
    "Lorem {ipsum} dolor {sit} amet, consectetur adipiscing elit. Sed do "
    "eiusmod tempor incididunt ut labore {et} dolore magna aliqua. ";

void generate_template(std::string &text,
                       std::unordered_map<std::string, std::string> &vars,
                       int count) {
  vars.clear();
  std::string builder;
  builder.reserve(static_cast<size_t>(count) * 200);

  builder += "<html><body>";
  builder += "<h1>{{TITLE}}</h1>";
  vars["TITLE"] = "Template title";
  builder += "<p>";
  builder += LOREM;
  builder += "</p>";
  builder += "<table>";

  for (int i = 0; i < count; i++) {
    if (i % 3 == 0) {
      builder += "<!-- {comment} -->";
    }
    builder += "<tr>";
    builder += "<td>{{ FIRST_NAME" + std::to_string(i) + " }}</td>";
    builder += "<td>{{LAST_NAME" + std::to_string(i) + "}}</td>";
    builder += "<td>{{  CITY" + std::to_string(i) + "  }}</td>";

    vars["FIRST_NAME" + std::to_string(i)] =
        FIRST_NAMES[i % FIRST_NAMES.size()];
    vars["LAST_NAME" + std::to_string(i)] = LAST_NAMES[i % LAST_NAMES.size()];
    vars["CITY" + std::to_string(i)] = CITIES[i % CITIES.size()];

    builder += "<td>{balance: " + std::to_string(i % 100) + "}</td>";
    builder += "</tr>\n";
  }

  builder += "</table>";
  builder += "</body></html>";
  text = std::move(builder);
}

TemplateRegex::TemplateRegex() : checksum_val(0), count(0), regex(nullptr) {
  count = config_val("count");
  regex = re_compile(PATTERN);
}

std::string TemplateRegex::name() const { return "Template::Regex"; }

void TemplateRegex::prepare() { generate_template(text, vars, count); }

void TemplateRegex::run(int iteration_id) {
  (void)iteration_id;

  if (!regex || text.empty())
    return;

  const size_t text_len = text.size();
  const char  *text_c   = text.c_str();

  std::string result;
  result.reserve(text_len * 2);

  size_t last_pos  = 0;
  int    match_len = 0;

  int match_idx = re_matchp(regex, text_c + last_pos, &match_len);

  while (match_idx != -1) {
    match_idx += static_cast<int>(last_pos);

    if (static_cast<size_t>(match_idx) > last_pos) {
      result.append(text_c + last_pos, match_idx - last_pos);
    }

    size_t key_start = static_cast<size_t>(match_idx) + 2;
    size_t key_end   = static_cast<size_t>(match_idx + match_len) - 2;

    if (key_end > key_start) {
      std::string key = text.substr(key_start, key_end - key_start);
      key = trim(key);

      auto it = vars.find(key);
      if (it != vars.end()) {
        result += it->second;
      }
    }

    last_pos = static_cast<size_t>(match_idx + match_len);

    if (last_pos >= text_len)
      break;

    match_idx = re_matchp(regex, text_c + last_pos, &match_len);
  }

  if (last_pos < text_len) {
    result.append(text_c + last_pos, text_len - last_pos);
  }

  rendered = std::move(result);
  checksum_val += static_cast<uint32_t>(rendered.size());
}

uint32_t TemplateRegex::checksum() {
  return checksum_val + Helper::checksum(rendered);
}

TemplateParse::TemplateParse() : checksum_val(0), count(0) {
  count = config_val("count");
}

std::string TemplateParse::name() const { return "Template::Parse"; }

void TemplateParse::prepare() { generate_template(text, vars, count); }

void TemplateParse::run(int iteration_id) {
  (void)iteration_id;
  const size_t len = text.size();
  std::string  result;
  result.reserve(static_cast<size_t>(len * 1.5));

  size_t i = 0;
  while (i < len) {
    if (i + 1 < len && text[i] == '{' && text[i + 1] == '{') {
      size_t j = i + 2;
      while (j + 1 < len) {
        if (text[j] == '}' && text[j + 1] == '}') {
          break;
        }
        j++;
      }

      if (j + 1 < len) {
        std::string key = text.substr(i + 2, j - i - 2);
        key = trim(key);

        auto it = vars.find(key);
        if (it != vars.end()) {
          result += it->second;
        }
        i = j + 2;
        continue;
      }
    }

    result += text[i];
    i++;
  }

  rendered = std::move(result);
  checksum_val += static_cast<uint32_t>(rendered.size());
}

uint32_t TemplateParse::checksum() {
  return checksum_val + Helper::checksum(rendered);
}

TextRaytracer::Vector TextRaytracer::Vector::normalize() const {
  double mag = magnitude();
  if (mag == 0.0)
    return {0, 0, 0};
  return scale(1.0 / mag);
}

TextRaytracer::TextRaytracer() : result_val(0) {
  w = static_cast<int32_t>(config_val("w"));
  h = static_cast<int32_t>(config_val("h"));
}

std::string TextRaytracer::name() const { return "Etc::TextRaytracer"; }

int TextRaytracer::shade_pixel(const Ray &ray, const Sphere &obj, double tval) {
  Vector pi = ray.orig.add(ray.dir.scale(tval));
  Color color = diffuse_shading(pi, obj, LIGHT1);
  double col = (color.r + color.g + color.b) / 3.0;
  int idx = static_cast<int>(col * 6.0);
  if (idx < 0)
    idx = 0;
  if (idx >= 6)
    idx = 5;
  return idx;
}

std::optional<double> TextRaytracer::intersect_sphere(const Ray &ray,
                                                      const Vector &center,
                                                      double radius) {
  Vector l = center.sub(ray.orig);
  double tca = l.dot(ray.dir);
  if (tca < 0.0)
    return std::nullopt;

  double d2 = l.dot(l) - tca * tca;
  double r2 = radius * radius;
  if (d2 > r2)
    return std::nullopt;

  double thc = std::sqrt(r2 - d2);
  double t0 = tca - thc;
  if (t0 > 10000.0)
    return std::nullopt;

  return t0;
}

double TextRaytracer::clamp(double x, double a, double b) {
  if (x < a)
    return a;
  if (x > b)
    return b;
  return x;
}

TextRaytracer::Color TextRaytracer::diffuse_shading(const Vector &pi,
                                                    const Sphere &obj,
                                                    const Light &light) {
  Vector n = obj.get_normal(pi);
  Vector light_dir = light.position.sub(pi).normalize();
  double lam1 = light_dir.dot(n);
  double lam2 = clamp(lam1, 0.0, 1.0);
  return light.color.scale(lam2 * 0.5).add(obj.color.scale(0.3));
}

void TextRaytracer::run(int iteration_id) {
  (void)iteration_id;
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      double fw = w, fi = i, fj = j, fh = h;

      Ray ray{
          {0.0, 0.0, 0.0},
          Vector{(fi - fw / 2.0) / fw, (fj - fh / 2.0) / fh, 1.0}.normalize()};

      std::optional<double> tval;
      const Sphere *hit_obj = nullptr;

      for (const auto &obj : SCENE) {
        auto intersect = intersect_sphere(ray, obj.center, obj.radius);
        if (intersect) {
          tval = intersect;
          hit_obj = &obj;
          break;
        }
      }

      char pixel = ' ';
      if (hit_obj && tval) {
        pixel = LUT[shade_pixel(ray, *hit_obj, *tval)];
      }
      result_val += static_cast<uint8_t>(pixel);
    }
  }
}

uint32_t TextRaytracer::checksum() { return result_val; }

Words::Words() : checksum_val(0) {
  words = config_val("words");
  word_len = config_val("word_len");
}

std::string Words::name() const { return "Etc::Words"; }

void Words::prepare() {
  const char chars[] = "abcdefghijklmnopqrstuvwxyz";
  const int char_count = 26;

  std::string result;
  result.reserve(words * (word_len + 1));

  for (int i = 0; i < words; ++i) {
    int len = Helper::next_int(word_len) + Helper::next_int(3) + 3;
    for (int j = 0; j < len; ++j) {
      result.push_back(chars[Helper::next_int(char_count)]);
    }
    if (i != words - 1) {
      result.push_back(' ');
    }
  }

  text = std::move(result);
}

void Words::run(int iteration_id) {
  (void)iteration_id;
  std::unordered_map<std::string, int> frequencies;
  std::istringstream iss(text);
  std::string word;

  while (iss >> word) {
    if (auto [it, inserted] = frequencies.try_emplace(word, 1); !inserted) {
      it->second++;
    }
  }

  std::string max_word;
  int max_count = 0;

  for (const auto &pair : frequencies) {
    if (pair.second > max_count) {
      max_count = pair.second;
      max_word = pair.first;
    }
  }

  checksum_val += static_cast<uint32_t>(max_count) +
                  Helper::checksum(max_word) +
                  static_cast<uint32_t>(frequencies.size());
}

uint32_t Words::checksum() { return checksum_val; }

int main(int argc, char *argv[]) {
  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();
  std::cout << "start: " << now << std::endl;

  int test = 0;
  const char *test_str = getenv("TEST");
  if (test_str != NULL && strcmp(test_str, "1") == 0) test = 1;

  static std::string config =
    "[\n"
    "  {\n"
    "    \"name\": \"Binarytrees::Obj\",\n"
    "    \"checksum\": 41943020,\n"
    "    \"depth\": 20,\n"
    "    \"iterations\": 17\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Binarytrees::Arena\",\n"
    "    \"checksum\": 201326568,\n"
    "    \"depth\": 22,\n"
    "    \"iterations\": 20\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Brainfuck::Array\",\n"
    "    \"checksum\": 954437102,\n"
    "    \"program\": \">++[<+++++++++++++>-]<[[>+>+<<-]>[<+>-]++++++++[>++++++++<-]>.[-]<<>++++++++++[>++++++++++[>++++++++++[>++++++++++[>++++++++++[>++++++++++[>++++++++++[-]<-]<-]<-]<-]<-]<-]<-]++++++++++.\",\n"
    "    \"warmup_program\": \"++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.\",\n"
    "    \"warmup_iterations\": 1000,\n"
    "    \"iterations\": 1\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Brainfuck::Recursion\",\n"
    "    \"checksum\": 954437102,\n"
    "    \"program\": \">++[<+++++++++++++>-]<[[>+>+<<-]>[<+>-]++++++++[>++++++++<-]>.[-]<<>++++++++++[>++++++++++[>++++++++++[>++++++++++[>++++++++++[>++++++++++[>++++++++++[-]<-]<-]<-]<-]<-]<-]<-]++++++++++.\",\n"
    "    \"warmup_program\": \"++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.\",\n"
    "    \"warmup_iterations\": 1000,\n"
    "    \"iterations\": 1\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Matmul::Single\",\n"
    "    \"checksum\": 3579146284,\n"
    "    \"n\": 538,\n"
    "    \"iterations\": 10\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Matmul::T4\",\n"
    "    \"checksum\": 1831243914,\n"
    "    \"n\": 840,\n"
    "    \"iterations\": 5\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Matmul::T8\",\n"
    "    \"checksum\": 3632192078,\n"
    "    \"n\": 1045,\n"
    "    \"iterations\": 5\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Matmul::T16\",\n"
    "    \"checksum\": 2405782988,\n"
    "    \"n\": 1284,\n"
    "    \"iterations\": 5\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Base64::Encode\",\n"
    "    \"checksum\": 862346474,\n"
    "    \"size\": 11000000,\n"
    "    \"iterations\": 180\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Base64::Decode\",\n"
    "    \"checksum\": 2188118664,\n"
    "    \"size\": 11000000,\n"
    "    \"iterations\": 110\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Json::Generate\",\n"
    "    \"checksum\": 156,\n"
    "    \"coords\": 4050,\n"
    "    \"iterations\": 130\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Json::ParseDom\",\n"
    "    \"checksum\": 859349936,\n"
    "    \"coords\": 7150,\n"
    "    \"iterations\": 147\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Json::ParseMapping\",\n"
    "    \"checksum\": 3873451916,\n"
    "    \"coords\": 7250,\n"
    "    \"iterations\": 138\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"CSV::Parse\",\n"
    "    \"checksum\": 4121601653,\n"
    "    \"rows\": 42000,\n"
    "    \"iterations\": 66\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::Sieve\",\n"
    "    \"checksum\": 1257617640,\n"
    "    \"limit\": 3800000,\n"
    "    \"iterations\": 258\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::TextRaytracer\",\n"
    "    \"checksum\": 2019483270,\n"
    "    \"w\": 790,\n"
    "    \"h\": 790,\n"
    "    \"iterations\": 75\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::NeuralNet\",\n"
    "    \"checksum\": 144809111,\n"
    "    \"iterations\": 1630\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::CacheSimulation\",\n"
    "    \"checksum\": 3120168072,\n"
    "    \"values\": 5500,\n"
    "    \"size\": 2600,\n"
    "    \"iterations\": 5050\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::GameOfLife\",\n"
    "    \"checksum\": 1434080561,\n"
    "    \"w\": 440,\n"
    "    \"h\": 440,\n"
    "    \"iterations\": 1150\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::Words\",\n"
    "    \"checksum\": 4195702752,\n"
    "    \"words\": 50000,\n"
    "    \"word_len\": 4,\n"
    "    \"iterations\": 240\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::LogParser\",\n"
    "    \"checksum\": 40256,\n"
    "    \"iterations\": 14,\n"
    "    \"lines_count\": 540\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Template::Regex\",\n"
    "    \"checksum\": 3411727785,\n"
    "    \"iterations\": 22,\n"
    "    \"count\": 50000\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Template::Parse\",\n"
    "    \"checksum\": 3193018079,\n"
    "    \"iterations\": 40,\n"
    "    \"count\": 56000\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Sort::Quick\",\n"
    "    \"checksum\": 55512467,\n"
    "    \"size\": 370000,\n"
    "    \"iterations\": 48\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Sort::Merge\",\n"
    "    \"checksum\": 41625861,\n"
    "    \"size\": 395000,\n"
    "    \"iterations\": 35\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Sort::Self\",\n"
    "    \"checksum\": 41820346,\n"
    "    \"size\": 540000,\n"
    "    \"iterations\": 37\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Graph::BFS\",\n"
    "    \"checksum\": 38850,\n"
    "    \"vertices\": 500000,\n"
    "    \"jumps\": 20,\n"
    "    \"jump_len\": 1000,\n"
    "    \"iterations\": 31\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Graph::DFS\",\n"
    "    \"checksum\": 17925,\n"
    "    \"vertices\": 500500,\n"
    "    \"jumps\": 20,\n"
    "    \"jump_len\": 1000,\n"
    "    \"iterations\": 13\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Graph::AStar\",\n"
    "    \"checksum\": 2202288,\n"
    "    \"vertices\": 1200500,\n"
    "    \"jumps\": 20,\n"
    "    \"jump_len\": 27,\n"
    "    \"iterations\": 19\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Hash::SHA256\",\n"
    "    \"checksum\": 346753887,\n"
    "    \"size\": 1100000,\n"
    "    \"iterations\": 1200\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Hash::CRC32\",\n"
    "    \"checksum\": 2412799940,\n"
    "    \"size\": 1200000,\n"
    "    \"iterations\": 98\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Calculator::Ast\",\n"
    "    \"checksum\": 1349845004,\n"
    "    \"operations\": 45500,\n"
    "    \"iterations\": 37\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Calculator::Interpreter\",\n"
    "    \"checksum\": 2933918360,\n"
    "    \"operations\": 70000,\n"
    "    \"iterations\": 70\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Maze::Generator\",\n"
    "    \"checksum\": 2632623298,\n"
    "    \"w\": 1200,\n"
    "    \"h\": 1200,\n"
    "    \"iterations\": 18\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Maze::BFS\",\n"
    "    \"checksum\": 3550680,\n"
    "    \"w\": 1400,\n"
    "    \"h\": 1400,\n"
    "    \"iterations\": 88\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Maze::AStar\",\n"
    "    \"checksum\": 2293579,\n"
    "    \"w\": 1400,\n"
    "    \"h\": 1400,\n"
    "    \"iterations\": 54\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"CLBG::Fannkuchredux\",\n"
    "    \"checksum\": 96973200,\n"
    "    \"n\": 8,\n"
    "    \"iterations\": 500\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"CLBG::Mandelbrot\",\n"
    "    \"checksum\": 2142473783,\n"
    "    \"w\": 400,\n"
    "    \"h\": 400,\n"
    "    \"iterations\": 115\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"CLBG::Nbody\",\n"
    "    \"checksum\": 338797504,\n"
    "    \"iterations\": 14000\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"CLBG::Spectralnorm\",\n"
    "    \"checksum\": 1052198219,\n"
    "    \"size\": 1200,\n"
    "    \"iterations\": 188\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::BWTEncode\",\n"
    "    \"checksum\": 3200000,\n"
    "    \"size\": 100000,\n"
    "    \"iterations\": 27\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::BWTDecode\",\n"
    "    \"checksum\": 258100258,\n"
    "    \"size\": 1000001,\n"
    "    \"iterations\": 215\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::HuffEncode\",\n"
    "    \"checksum\": 78409500,\n"
    "    \"size\": 1000002,\n"
    "    \"iterations\": 250\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::HuffDecode\",\n"
    "    \"checksum\": 439101317,\n"
    "    \"size\": 1000003,\n"
    "    \"iterations\": 366\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::ArithEncode\",\n"
    "    \"checksum\": 13467168,\n"
    "    \"size\": 100004,\n"
    "    \"iterations\": 440\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::ArithDecode\",\n"
    "    \"checksum\": 42702130,\n"
    "    \"size\": 100005,\n"
    "    \"iterations\": 355\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::LZWEncode\",\n"
    "    \"checksum\": 450792,\n"
    "    \"size\": 200006,\n"
    "    \"iterations\": 90\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::LZWDecode\",\n"
    "    \"checksum\": 1843300000,\n"
    "    \"size\": 1200000,\n"
    "    \"iterations\": 1280\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Distance::Jaro\",\n"
    "    \"checksum\": 587526372,\n"
    "    \"iterations\": 810,\n"
    "    \"count\": 1000,\n"
    "    \"size\": 49\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Distance::NGram\",\n"
    "    \"checksum\": 450632,\n"
    "    \"iterations\": 152,\n"
    "    \"count\": 10,\n"
    "    \"size\": 15000\n"
    "  }\n"
    "]\n"
  ;

  static std::string test_config =
    "[\n"
    "  {\n"
    "    \"name\": \"Binarytrees::Obj\",\n"
    "    \"checksum\": 124,\n"
    "    \"depth\": 4,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Binarytrees::Arena\",\n"
    "    \"checksum\": 124,\n"
    "    \"depth\": 4,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Brainfuck::Array\",\n"
    "    \"checksum\": 3562897308,\n"
    "    \"program\": \"++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.\",\n"
    "    \"warmup_program\": \"++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.\",\n"
    "    \"warmup_iterations\": 1,\n"
    "    \"iterations\": 2\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Brainfuck::Recursion\",\n"
    "    \"checksum\": 3562897308,\n"
    "    \"program\": \"++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.\",\n"
    "    \"warmup_program\": \"++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.\",\n"
    "    \"warmup_iterations\": 1,\n"
    "    \"iterations\": 2\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Matmul::Single\",\n"
    "    \"checksum\": 720656960,\n"
    "    \"n\": 40,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Matmul::T4\",\n"
    "    \"checksum\": 2420334760,\n"
    "    \"n\": 42,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Matmul::T8\",\n"
    "    \"checksum\": 4014223764,\n"
    "    \"n\": 44,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Matmul::T16\",\n"
    "    \"checksum\": 418055336,\n"
    "    \"n\": 46,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Base64::Encode\",\n"
    "    \"checksum\": 3894061047,\n"
    "    \"size\": 30,\n"
    "    \"iterations\": 8192\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Base64::Decode\",\n"
    "    \"checksum\": 732348015,\n"
    "    \"size\": 23,\n"
    "    \"iterations\": 8192\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Json::Generate\",\n"
    "    \"checksum\": 4,\n"
    "    \"coords\": 9,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Json::ParseDom\",\n"
    "    \"checksum\": 2918383076,\n"
    "    \"coords\": 10,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Json::ParseMapping\",\n"
    "    \"checksum\": 1969907348,\n"
    "    \"coords\": 11,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"CSV::Parse\",\n"
    "    \"checksum\": 3563398641,\n"
    "    \"rows\": 11,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::Sieve\",\n"
    "    \"checksum\": 4660,\n"
    "    \"limit\": 1000,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::TextRaytracer\",\n"
    "    \"checksum\": 1438660,\n"
    "    \"w\": 100,\n"
    "    \"h\": 100,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::NeuralNet\",\n"
    "    \"checksum\": 1586674334,\n"
    "    \"iterations\": 2\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::CacheSimulation\",\n"
    "    \"checksum\": 179773224,\n"
    "    \"values\": 2000,\n"
    "    \"size\": 1000,\n"
    "    \"iterations\": 2\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::GameOfLife\",\n"
    "    \"checksum\": 4006235749,\n"
    "    \"w\": 20,\n"
    "    \"h\": 20,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::Words\",\n"
    "    \"checksum\": 580539813,\n"
    "    \"words\": 600,\n"
    "    \"word_len\": 1,\n"
    "    \"iterations\": 2\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Etc::LogParser\",\n"
    "    \"checksum\": 138,\n"
    "    \"iterations\": 2,\n"
    "    \"lines_count\": 10\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Template::Regex\",\n"
    "    \"checksum\": 1554169905,\n"
    "    \"iterations\": 2,\n"
    "    \"count\": 12\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Template::Parse\",\n"
    "    \"checksum\": 917360148,\n"
    "    \"iterations\": 2,\n"
    "    \"count\": 10\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Sort::Quick\",\n"
    "    \"checksum\": 2548773,\n"
    "    \"size\": 100000,\n"
    "    \"iterations\": 2\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Sort::Merge\",\n"
    "    \"checksum\": 2548773,\n"
    "    \"size\": 100000,\n"
    "    \"iterations\": 2\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Sort::Self\",\n"
    "    \"checksum\": 2548773,\n"
    "    \"size\": 100000,\n"
    "    \"iterations\": 2\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Graph::BFS\",\n"
    "    \"checksum\": 20,\n"
    "    \"vertices\": 100,\n"
    "    \"jumps\": 3,\n"
    "    \"jump_len\": 100,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Graph::DFS\",\n"
    "    \"checksum\": 28,\n"
    "    \"vertices\": 100,\n"
    "    \"jumps\": 3,\n"
    "    \"jump_len\": 100,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Graph::AStar\",\n"
    "    \"checksum\": 36,\n"
    "    \"vertices\": 100,\n"
    "    \"jumps\": 3,\n"
    "    \"jump_len\": 100,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Hash::SHA256\",\n"
    "    \"checksum\": 1036619072,\n"
    "    \"size\": 1000,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Hash::CRC32\",\n"
    "    \"checksum\": 1626481306,\n"
    "    \"size\": 1000,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Calculator::Ast\",\n"
    "    \"checksum\": 774028720,\n"
    "    \"operations\": 3,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Calculator::Interpreter\",\n"
    "    \"checksum\": 3208680,\n"
    "    \"operations\": 10,\n"
    "    \"iterations\": 100\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Maze::Generator\",\n"
    "    \"checksum\": 2309652669,\n"
    "    \"w\": 200,\n"
    "    \"h\": 30,\n"
    "    \"iterations\": 2\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Maze::BFS\",\n"
    "    \"checksum\": 6771,\n"
    "    \"w\": 200,\n"
    "    \"h\": 30,\n"
    "    \"iterations\": 2\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Maze::AStar\",\n"
    "    \"checksum\": 6771,\n"
    "    \"w\": 200,\n"
    "    \"h\": 30,\n"
    "    \"iterations\": 2\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"CLBG::Fannkuchredux\",\n"
    "    \"checksum\": 4428,\n"
    "    \"n\": 5,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"CLBG::Mandelbrot\",\n"
    "    \"checksum\": 2236342953,\n"
    "    \"w\": 30,\n"
    "    \"h\": 30,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"CLBG::Nbody\",\n"
    "    \"checksum\": 338796608,\n"
    "    \"iterations\": 2\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"CLBG::Spectralnorm\",\n"
    "    \"checksum\": 1052168784,\n"
    "    \"size\": 45,\n"
    "    \"iterations\": 10\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::BWTEncode\",\n"
    "    \"checksum\": 200,\n"
    "    \"size\": 50,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::BWTDecode\",\n"
    "    \"checksum\": 100204,\n"
    "    \"size\": 51,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::HuffEncode\",\n"
    "    \"checksum\": 56,\n"
    "    \"size\": 52,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::HuffDecode\",\n"
    "    \"checksum\": 100212,\n"
    "    \"size\": 53,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::ArithEncode\",\n"
    "    \"checksum\": 56,\n"
    "    \"size\": 54,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::ArithDecode\",\n"
    "    \"checksum\": 100220,\n"
    "    \"size\": 55,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::LZWEncode\",\n"
    "    \"checksum\": 208,\n"
    "    \"size\": 60,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Compress::LZWDecode\",\n"
    "    \"checksum\": 100244,\n"
    "    \"size\": 61,\n"
    "    \"iterations\": 3\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Distance::Jaro\",\n"
    "    \"checksum\": 7020,\n"
    "    \"iterations\": 1,\n"
    "    \"count\": 10,\n"
    "    \"size\": 5\n"
    "  },\n"
    "  {\n"
    "    \"name\": \"Distance::NGram\",\n"
    "    \"checksum\": 42,\n"
    "    \"iterations\": 1,\n"
    "    \"count\": 2,\n"
    "    \"size\": 500\n"
    "  }\n"
    "]\n";
 
  load_config(test ? test_config : config);

  if (argc > 1) {
    Benchmark::all(argv[1]);
  } else {
    Benchmark::all("");
  }

  return 0;
}
