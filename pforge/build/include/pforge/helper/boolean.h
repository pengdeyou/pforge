//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_HELPER_BOOLEAN_H__
#define __PFORGE_HELPER_BOOLEAN_H__

#ifdef __IDE_ONLY__
#include "pforge/checkers.h"
#include "pforge/cpython_api_compat.h"
#include "pforge/defines.h"
#endif

// The slot in Python3 got renamed, compensate it like this.
#if PYTHON_VERSION >= 0x300
#define nb_nonzero nb_bool
#endif

PFORGE_MAY_BE_UNUSED static int CHECK_IF_TRUE(PyObject *obj) {
    CHECK_OBJECT(obj);

    if (obj == Py_True) {
        return 1;
    } else if (obj == Py_False || obj == Py_None) {
        return 0;
    } else {
        Py_ssize_t result;

        if (Py_TYPE(obj)->tp_as_number != NULL && Py_TYPE(obj)->tp_as_number->nb_nonzero != NULL) {
            result = (*Py_TYPE(obj)->tp_as_number->nb_nonzero)(obj);
        } else if (Py_TYPE(obj)->tp_as_mapping != NULL && Py_TYPE(obj)->tp_as_mapping->mp_length != NULL) {
            result = (*Py_TYPE(obj)->tp_as_mapping->mp_length)(obj);
        } else if (Py_TYPE(obj)->tp_as_sequence != NULL && Py_TYPE(obj)->tp_as_sequence->sq_length != NULL) {
            result = (*Py_TYPE(obj)->tp_as_sequence->sq_length)(obj);
        } else {
            return 1;
        }

        if (result > 0) {
            return 1;
        } else if (result == 0) {
            return 0;
        } else {
            return -1;
        }
    }
}

PFORGE_MAY_BE_UNUSED static int CHECK_IF_FALSE(PyObject *obj) {
    int result = CHECK_IF_TRUE(obj);

    if (result == 0) {
        return 1;
    }
    if (result == 1) {
        return 0;
    }
    return -1;
}

PFORGE_MAY_BE_UNUSED static inline PyObject *BOOL_FROM(bool value) {
    CHECK_OBJECT(Py_True);
    CHECK_OBJECT(Py_False);

    return value ? Py_True : Py_False;
}

#undef nb_nonzero

typedef enum {
    PFORGE_BOOL_FALSE = 0,
    PFORGE_BOOL_TRUE = 1,
    PFORGE_BOOL_UNASSIGNED = 2,
    PFORGE_BOOL_EXCEPTION = -1
} pforge_bool;

// Convert a truth check to "pforge_bool", preserving the exception indicator.
PFORGE_MAY_BE_UNUSED static pforge_bool CHECK_IF_TRUE_NBOOL(PyObject *obj) {
    int result = CHECK_IF_TRUE(obj);

    if (result == -1) {
        return PFORGE_BOOL_EXCEPTION;
    }

    return result == 0 ? PFORGE_BOOL_FALSE : PFORGE_BOOL_TRUE;
}

typedef enum { PFORGE_VOID_OK = 0, PFORGE_VOID_EXCEPTION = 1 } pforge_void;

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
