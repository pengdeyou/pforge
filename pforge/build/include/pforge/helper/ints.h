//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_HELPER_INTS_H__
#define __PFORGE_HELPER_INTS_H__

#ifdef __IDE_ONLY__
#include "pforge/cpython_api_compat.h"

#include "internal/pycore_long.h"
#include "pforge/checkers.h"
#include "pforge/defines.h"
#include "pforge/printing.h"
#endif

// Our "PyLong_FromLong" replacement.
extern PyObject *Pforge_PyLong_FromLong(long ival);
extern long Pforge_PyLong_AsLongAndOverflow(PyObject *value, int *overflow);
extern Py_ssize_t Pforge_PyLong_AsSsize_tAndOverflow(PyObject *value, int *overflow);
extern Py_ssize_t Pforge_PyLong_AsSsize_t(PyThreadState *tstate, PyObject *value);

// Our "PyInt_FromLong" replacement, not done (yet?).
#if PYTHON_VERSION >= 0x300
#define Pforge_PyInt_FromLong(ival) Pforge_PyLong_FromLong(ival)
#else
#define Pforge_PyInt_FromLong(ival) PyInt_FromLong(ival)
#endif

// We are using this mixed type for both Python2 and Python3, since then we
// avoid the complexity of overflowed integers for Python2 to switch over.

typedef enum {
    PFORGE_ILONG_UNASSIGNED = 0,
    PFORGE_ILONG_OBJECT_VALID = 1,
    PFORGE_ILONG_CLONG_VALID = 2,
    PFORGE_ILONG_BOTH_VALID = 3,
    PFORGE_ILONG_EXCEPTION = 4
} pforge_ilong_validity;

typedef struct {
    pforge_ilong_validity validity;

    PyObject *python_value;
    long c_value;
} pforge_ilong;

// An unassigned "pforge_ilong" value, usable in assignments (unlike the brace
// initializer form, which is only valid in a declaration).
PFORGE_MAY_BE_UNUSED static const pforge_ilong PFORGE_ILONG_UNASSIGNED_VALUE = {PFORGE_ILONG_UNASSIGNED, NULL, 0};

#define IS_NILONG_OBJECT_VALUE_VALID(value) (((value)->validity & PFORGE_ILONG_OBJECT_VALID) != 0)
#define IS_NILONG_C_VALUE_VALID(value) (((value)->validity & PFORGE_ILONG_CLONG_VALID) != 0)

PFORGE_MAY_BE_UNUSED static void SET_NILONG_OBJECT_VALUE(pforge_ilong *dual_value, PyObject *python_value) {
    dual_value->validity = PFORGE_ILONG_OBJECT_VALID;
    dual_value->python_value = python_value;
}

PFORGE_MAY_BE_UNUSED static void SET_NILONG_C_VALUE(pforge_ilong *dual_value, long c_value) {
    dual_value->validity = PFORGE_ILONG_CLONG_VALID;
    dual_value->c_value = c_value;
}

PFORGE_MAY_BE_UNUSED static void SET_NILONG_OBJECT_AND_C_VALUE(pforge_ilong *dual_value, PyObject *python_value,
                                                               long c_value) {
    dual_value->validity = PFORGE_ILONG_BOTH_VALID;
    dual_value->python_value = python_value;
    dual_value->c_value = c_value;
}

PFORGE_MAY_BE_UNUSED static void RELEASE_NILONG_VALUE(pforge_ilong *dual_value) {
    if (IS_NILONG_OBJECT_VALUE_VALID(dual_value)) {
        CHECK_OBJECT(dual_value);
        Py_DECREF(dual_value->python_value);
    }

    dual_value->validity = PFORGE_ILONG_UNASSIGNED;
}

PFORGE_MAY_BE_UNUSED static void INCREF_NILONG_VALUE(pforge_ilong *dual_value) {
    if (IS_NILONG_OBJECT_VALUE_VALID(dual_value)) {
        CHECK_OBJECT(dual_value);
        Py_INCREF(dual_value->python_value);
    }
}

PFORGE_MAY_BE_UNUSED static long GET_NILONG_C_VALUE(pforge_ilong const *dual_value) {
    assert(IS_NILONG_C_VALUE_VALID(dual_value));
    return dual_value->c_value;
}

PFORGE_MAY_BE_UNUSED static PyObject *GET_NILONG_OBJECT_VALUE(pforge_ilong const *dual_value) {
    assert(IS_NILONG_OBJECT_VALUE_VALID(dual_value));
    return dual_value->python_value;
}

PFORGE_MAY_BE_UNUSED static void ENFORCE_NILONG_OBJECT_VALUE(pforge_ilong *dual_value) {
    assert(dual_value->validity != PFORGE_ILONG_UNASSIGNED);

    if (!IS_NILONG_OBJECT_VALUE_VALID(dual_value)) {
        dual_value->python_value = Pforge_PyLong_FromLong(dual_value->c_value);

        assert(dual_value->python_value != NULL);

        dual_value->validity = PFORGE_ILONG_BOTH_VALID;
    }
}

PFORGE_MAY_BE_UNUSED static void CHECK_NILONG_OBJECT(pforge_ilong const *dual_value) {
    assert(dual_value->validity != PFORGE_ILONG_UNASSIGNED);

    if (IS_NILONG_OBJECT_VALUE_VALID(dual_value)) {
        CHECK_OBJECT(dual_value);
    }
}

PFORGE_MAY_BE_UNUSED static void PRINT_NILONG(pforge_ilong const *dual_value) {
    PRINT_FORMAT("NILONG: %d", dual_value->validity);
    if (IS_NILONG_C_VALUE_VALID(dual_value)) {
        PRINT_FORMAT("C=%d", dual_value->c_value);
    }
    if (IS_NILONG_OBJECT_VALUE_VALID(dual_value)) {
        PRINT_STRING("Python=");
        PRINT_ITEM(dual_value->python_value);
    }
}

#if PYTHON_VERSION < 0x3c0
// Convert single digit to sdigit (int32_t),
// spell-checker: ignore sdigit,stwodigits
typedef long medium_result_value_t;
#define MEDIUM_VALUE(x)                                                                                                \
    (Py_SIZE(x) < 0 ? -(sdigit)((PyLongObject *)(x))->ob_digit[0]                                                      \
                    : (Py_SIZE(x) == 0 ? (sdigit)0 : (sdigit)((PyLongObject *)(x))->ob_digit[0]))

#else
typedef stwodigits medium_result_value_t;
#define MEDIUM_VALUE(x) ((stwodigits)_PyLong_CompactValue((PyLongObject *)x))

#endif

// TODO: Use this from header files, although they have changed.
#define PFORGE_STATIC_SMALLINT_VALUE_MIN -5
#define PFORGE_STATIC_SMALLINT_VALUE_MAX 257

#define PFORGE_TO_SMALL_VALUE_OFFSET(value) (value - PFORGE_STATIC_SMALLINT_VALUE_MIN)

#if PYTHON_VERSION < 0x3b0

#if PYTHON_VERSION >= 0x300

#if PYTHON_VERSION >= 0x390
extern PyObject **Pforge_Long_SmallValues;
#else
extern PyObject *Pforge_Long_SmallValues[PFORGE_STATIC_SMALLINT_VALUE_MAX - PFORGE_STATIC_SMALLINT_VALUE_MIN + 1];
#endif

PFORGE_MAY_BE_UNUSED static inline PyObject *Pforge_Long_GetSmallValue(int ival) {
    return Pforge_Long_SmallValues[PFORGE_TO_SMALL_VALUE_OFFSET(ival)];
}

#endif

#else
PFORGE_MAY_BE_UNUSED static inline PyObject *Pforge_Long_GetSmallValue(medium_result_value_t ival) {
    return (PyObject *)&_PyLong_SMALL_INTS[PFORGE_TO_SMALL_VALUE_OFFSET(ival)];
}
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
