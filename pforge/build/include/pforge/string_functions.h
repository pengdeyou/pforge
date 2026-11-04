//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_STRING_FUNCTIONS_H__
#define __PFORGE_STRING_FUNCTIONS_H__

#ifdef __IDE_ONLY__
#include "pforge/cpython_api_compat.h"
#include "pforge/defines.h"
#endif

/* String handling that uses the proper version of strings for Python3 or not,
 * which makes it easier to write portable code.
 */
#if PYTHON_VERSION < 0x300
#define PyUnicode_GET_LENGTH(x) (PyUnicode_GET_SIZE(x))
#define Pforge_String_AsString PyString_AsString
#define Pforge_String_AsString_Unchecked PyString_AS_STRING
#define Pforge_String_Check PyString_Check
#define Pforge_String_CheckExact PyString_CheckExact
PFORGE_MAY_BE_UNUSED static inline bool Pforge_StringOrUnicode_CheckExact(PyObject *value) {
    return PyString_CheckExact(value) || PyUnicode_CheckExact(value);
}
#define Pforge_StringObject PyStringObject
#define Pforge_String_FromString PyString_FromString
#define Pforge_String_FromStringAndSize PyString_FromStringAndSize
#define Pforge_String_FromFormat PyString_FromFormat
#define PyUnicode_CHECK_INTERNED (0)
PFORGE_MAY_BE_UNUSED static Py_UNICODE *Pforge_UnicodeAsWideString(PyObject *str, Py_ssize_t *size) {
    PyObject *unicode;

    if (!PyUnicode_Check(str)) {
        // Leaking memory, but for usages its acceptable to
        // achieve that the pointer remains valid.
        unicode = PyObject_Unicode(str);
    } else {
        unicode = str;
    }

    if (size != NULL) {
        *size = (Py_ssize_t)PyUnicode_GET_LENGTH(unicode);
    }

    return PyUnicode_AsUnicode(unicode);
}
#else
#define Pforge_String_AsString _PyUnicode_AsString

// Note: This private stuff from file "Objects/unicodeobject.c"
// spell-checker: ignore unicodeobject
#define _PyUnicode_UTF8(op) (((PyCompactUnicodeObject *)(op))->utf8)
#define PyUnicode_UTF8(op)                                                                                             \
    (assert(PyUnicode_IS_READY(op)),                                                                                   \
     PyUnicode_IS_COMPACT_ASCII(op) ? ((char *)((PyASCIIObject *)(op) + 1)) : _PyUnicode_UTF8(op))
#ifdef __PFORGE_NO_ASSERT__
#define Pforge_String_AsString_Unchecked PyUnicode_UTF8
#else
PFORGE_MAY_BE_UNUSED static char const *Pforge_String_AsString_Unchecked(PyObject *obj) {
    char const *result = PyUnicode_UTF8(obj);
    assert(result != NULL);
    return result;
}
#endif
#define Pforge_String_Check PyUnicode_Check
#define Pforge_String_CheckExact PyUnicode_CheckExact
#define Pforge_StringOrUnicode_CheckExact PyUnicode_CheckExact
#define Pforge_StringObject PyUnicodeObject
#define Pforge_String_FromString PyUnicode_FromString
#define Pforge_String_FromStringAndSize PyUnicode_FromStringAndSize
#define Pforge_String_FromFormat PyUnicode_FromFormat
#define Pforge_UnicodeAsWideString PyUnicode_AsWideCharString
#endif

// Before 3.7, it's only available in debug Python, so we need to guard this.
#if PYTHON_VERSION < 0x370
#define Pforge_PyUnicode_CheckConsistency(op, check) 1
#else
#define Pforge_PyUnicode_CheckConsistency(op, check) (_PyUnicode_CheckConsistency(op, check))
#endif

#endif

//     Part of "Pforge", an optimizing Python compiler that is compatible and
//     integrates with CPython, but also works on its own.
//
//     Licensed under the GNU Affero General Public License, Version 3 (the "License");
//     you may not use this file except in compliance with the License.
//     You may obtain a copy of the License at
//
//        https://www.gnu.org/licenses/agpl-3.0.txt
//
//     See also: "Pforge Runtime Library Exception, Version 1.0" in file
//     "LICENSE-RUNTIME.txt" for additional permissions granted under Section 7.
//
//     Unless required by applicable law or agreed to in writing, software
//     distributed under the License is distributed on an "AS IS" BASIS,
//     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//     See the License for the specific language governing permissions and
//     limitations under the License.
