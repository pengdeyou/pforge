//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_MAPPINGS_H__
#define __PFORGE_MAPPINGS_H__

#ifdef __IDE_ONLY__
#include "pforge/checkers.h"
#include "pforge/cpython_api_compat.h"
#include "pforge/defines.h"
#include "pforge/exceptions.h"
#endif

#include "pforge/helper/calling_generated.h"

extern Py_ssize_t Pforge_PyMapping_Size(PyObject *mapping);

PFORGE_MAY_BE_UNUSED static PyObject *Pforge_Mapping_Keys(PyThreadState *tstate, PyObject *mapping) {
    CHECK_OBJECT(mapping);

#if PYTHON_VERSION < 0x300
    // The "PyMapping_Keys" macro passes a string literal to a non-const argument, which the C++
    // fallback compilation for old compilers warns about.
    return CALL_METHOD_NO_ARGS(tstate, mapping, const_str_plain_keys);
#else
    // TODO: Make this not a Python C-API call, emulating the list conversion that
    // "PyMapping_Keys" does for non-exact dictionaries.
    return PyMapping_Keys(mapping);
#endif
}

PFORGE_MAY_BE_UNUSED static int MAPPING_HAS_ITEM(PyThreadState *tstate, PyObject *mapping, PyObject *key) {
    PyObject *result = PyObject_GetItem(mapping, key);

    if (result == NULL) {
        bool had_key_error = CHECK_AND_CLEAR_KEY_ERROR_OCCURRED(tstate);

        if (had_key_error) {
            return 0;
        } else {
            return -1;
        }
    } else {
        Py_DECREF(result);
        return 1;
    }
}

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
