//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_HELPER_LONG_HELPERS_H__
#define __PFORGE_HELPER_LONG_HELPERS_H__

/* Shared helpers for the "long" digit level operations.
 *
 * The implementations are in "HelpersOperationBinaryAddUtils.c", which is
 * chained into the same compilation unit, and therefore the declarations
 * are "static" to it.
 *
 * This file is included from another C file, help IDEs to still parse it on its own.
 */
#ifdef __IDE_ONLY__
#include "pforge/prelude.h"
#endif

#if PYTHON_VERSION < 0x3c0
#define MAX_LONG_DIGITS ((PY_SSIZE_T_MAX - offsetof(PyLongObject, ob_digit)) / sizeof(digit))
#define Pforge_LongGetDigitPointer(value) (&(((PyLongObject *)value)->ob_digit[0]))
#define Pforge_LongGetDigitSize(value) (Py_ABS(Py_SIZE(value)))
#define Pforge_LongGetSignedDigitSize(value) (Py_SIZE(value))
#define Pforge_LongIsNegative(value) (Py_SIZE(value) < 0)
#define Pforge_LongSetSignNegative(value) Py_SET_SIZE(value, -Py_ABS(Py_SIZE(value)))
#define Pforge_LongSetSign(value, positive) Py_SET_SIZE(value, (((positive) ? 1 : -1) * Py_ABS(Py_SIZE(value))))
#define Pforge_LongFlipSign(value) Py_SET_SIZE(value, -Py_SIZE(value))
#define Pforge_LongSetDigitSizeAndNegative(value, count, negative) Py_SET_SIZE(value, negative ? -count : count)
#else
#define MAX_LONG_DIGITS ((PY_SSIZE_T_MAX - offsetof(PyLongObject, long_value.ob_digit)) / sizeof(digit))

#define Pforge_LongGetDigitPointer(value) (&(((PyLongObject *)value)->long_value.ob_digit[0]))
#define Pforge_LongGetDigitSize(value) (_PyLong_DigitCount((PyLongObject const *)(value)))
#define Pforge_LongGetSignedDigitSize(value) (_PyLong_SignedDigitCount((PyLongObject const *)(value)))
#define Pforge_LongIsNegative(value) (((PyLongObject *)value)->long_value.lv_tag & SIGN_NEGATIVE)
#define Pforge_LongSetSignNegative(value)                                                                              \
    ((PyLongObject *)value)->long_value.lv_tag = ((PyLongObject *)value)->long_value.lv_tag | SIGN_NEGATIVE;
#define Pforge_LongSetSignPositive(value)                                                                              \
    ((PyLongObject *)value)->long_value.lv_tag = ((PyLongObject *)value)->long_value.lv_tag & ~(SIGN_NEGATIVE);
#define Pforge_LongSetSign(value, positive)                                                                            \
    if (positive) {                                                                                                    \
        Pforge_LongSetSignPositive(value);                                                                             \
    } else {                                                                                                           \
        Pforge_LongSetSignNegative(value);                                                                             \
    }
#define Pforge_LongSetDigitSizeAndNegative(value, count, negative)                                                     \
    _PyLong_SetSignAndDigitCount(value, negative ? -1 : 1, count)
#define Pforge_LongFlipSign(value) _PyLong_FlipSign(value)
#endif

static PyLongObject *Pforge_LongNew(Py_ssize_t size);
static PyObject *Pforge_LongRealloc(PyObject *value, Py_ssize_t size);
static PyObject *Pforge_LongFromCLong(long ival);
static void Pforge_LongUpdateFromCLong(PyObject **value, long ival);
static PyLongObject *_Pforge_LongAddDigits(digit const *a, Py_ssize_t size_a, digit const *b, Py_ssize_t size_b);
static PyObject *_Pforge_LongAddInplaceDigits(PyObject *left, digit const *b, Py_ssize_t size_b);
static PyLongObject *_Pforge_LongSubDigits(digit const *a, Py_ssize_t size_a, digit const *b, Py_ssize_t size_b);
static PyObject *_Pforge_LongSubInplaceDigits(PyObject *left, digit const *b, Py_ssize_t size_b, int sign);

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
