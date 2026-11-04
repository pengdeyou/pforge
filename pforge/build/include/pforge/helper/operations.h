//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_OPERATIONS_H__
#define __PFORGE_OPERATIONS_H__

#ifdef __IDE_ONLY__
#include "pforge/checkers.h"
#include "pforge/cpython_api_compat.h"
#include "pforge/defines.h"
#include "pforge/helper/boolean.h"
#include "pforge/helper/ints.h"
#endif

#if PYTHON_VERSION >= 0x300
extern PyObject *UNICODE_CONCAT(PyThreadState *tstate, PyObject *left, PyObject *right);
extern bool UNICODE_APPEND(PyThreadState *tstate, PyObject **p_left, PyObject *right);
#else
// TODO: Specialize for Python2 too.
PFORGE_MAY_BE_UNUSED static PyObject *UNICODE_CONCAT(PyThreadState *tstate, PyObject *left, PyObject *right) {
    return PyUnicode_Concat(left, right);
}
#endif

// This macro is necessary for Python2 to determine if the "coerce" slot
// will have to be considered or not, and if in-place operations are
// allowed to be there or not.
#if PYTHON_VERSION < 0x300
#define NEW_STYLE_NUMBER(o) PyType_HasFeature(Py_TYPE(o), Py_TPFLAGS_CHECKTYPES)
#define NEW_STYLE_NUMBER_TYPE(t) PyType_HasFeature(t, Py_TPFLAGS_CHECKTYPES)
#define CAN_HAVE_INPLACE(t) PyType_HasFeature(t, Py_TPFLAGS_HAVE_INPLACEOPS)
#else
#define NEW_STYLE_NUMBER(o) (true)
#define NEW_STYLE_NUMBER_TYPE(t) (true)
#define CAN_HAVE_INPLACE(t) (true)
#endif

typedef PyObject *(unary_api)(PyObject *);

PFORGE_MAY_BE_UNUSED static PyObject *UNARY_OPERATION(unary_api api, PyObject *operand) {
    CHECK_OBJECT(operand);
    PyObject *result = api(operand);

    if (unlikely(result == NULL)) {
        return NULL;
    }

    CHECK_OBJECT(result);

    return result;
}

// Generated helpers to execute operations on fully or partially known types.
#include "pforge/helper/operations_binary_add.h"
#include "pforge/helper/operations_binary_bitand.h"
#include "pforge/helper/operations_binary_bitor.h"
#include "pforge/helper/operations_binary_bitxor.h"
#include "pforge/helper/operations_binary_divmod.h"
#include "pforge/helper/operations_binary_floordiv.h"
#include "pforge/helper/operations_binary_lshift.h"
#include "pforge/helper/operations_binary_mod.h"
#include "pforge/helper/operations_binary_mult.h"
#include "pforge/helper/operations_binary_pow.h"
#include "pforge/helper/operations_binary_rshift.h"
#include "pforge/helper/operations_binary_sub.h"
#include "pforge/helper/operations_binary_truediv.h"

// Generated helpers to execute operations on dual types.
#include "pforge/helper/operations_binary_dual_add.h"

#include "pforge/helper/operations_inplace_add.h"
#include "pforge/helper/operations_inplace_bitand.h"
#include "pforge/helper/operations_inplace_bitor.h"
#include "pforge/helper/operations_inplace_bitxor.h"
#include "pforge/helper/operations_inplace_floordiv.h"
#include "pforge/helper/operations_inplace_lshift.h"
#include "pforge/helper/operations_inplace_mod.h"
#include "pforge/helper/operations_inplace_mult.h"
#include "pforge/helper/operations_inplace_pow.h"
#include "pforge/helper/operations_inplace_rshift.h"
#include "pforge/helper/operations_inplace_sub.h"
#include "pforge/helper/operations_inplace_truediv.h"

#if PYTHON_VERSION < 0x300
// Classical division is Python2 only.
#include "pforge/helper/operations_binary_olddiv.h"
#include "pforge/helper/operations_inplace_olddiv.h"
#endif
#if PYTHON_VERSION >= 0x350
// Matrix multiplication is Python3.5 or higher only.
#include "pforge/helper/operations_binary_matmult.h"
#include "pforge/helper/operations_inplace_matmult.h"
#endif

// TODO: Get rid of the need for these, we should inline the abstract
// algorithms, for now we have them for easier templating.
#define PyNumber_InPlaceSub PyNumber_InPlaceSubtract
#define PyNumber_InPlaceMult PyNumber_InPlaceMultiply
#define PyNumber_InPlaceOlddiv PyNumber_InPlaceDivide
#define PyNumber_InPlacePow(a, b) PyNumber_InPlacePower(a, b, Py_None)
#define PyNumber_InPlaceMod PyNumber_InPlaceRemainder
#define PyNumber_InPlaceBitor PyNumber_InPlaceOr
#define PyNumber_InPlaceBitxor PyNumber_InPlaceXor
#define PyNumber_InPlaceBitand PyNumber_InPlaceAnd
#define PyNumber_InPlaceTruediv PyNumber_InPlaceTrueDivide
#define PyNumber_InPlaceFloordiv PyNumber_InPlaceFloorDivide
#define PyNumber_InPlaceMatmult PyNumber_InPlaceMatrixMultiply
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
