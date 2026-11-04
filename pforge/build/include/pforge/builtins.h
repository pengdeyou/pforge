//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_BUILTINS_H__
#define __PFORGE_BUILTINS_H__

/* This file is included from another C file, help IDEs to still parse it on its own. */
#ifdef __IDE_ONLY__
#include "pforge/prelude.h"
#endif

extern PyModuleObject *builtin_module;
extern PyDictObject *dict_builtin;

#include "pforge/calling.h"

PFORGE_MAY_BE_UNUSED static PyObject *LOOKUP_BUILTIN(PyObject *name) {
    CHECK_OBJECT(dict_builtin);
    CHECK_OBJECT(name);
    assert(Pforge_String_CheckExact(name));

    PyObject *result = GET_STRING_DICT_VALUE(dict_builtin, (Pforge_StringObject *)name);

    // This is assumed to not fail, abort if it does.
    if (unlikely(result == NULL)) {
        PyErr_PrintEx(0);
        Py_Exit(1);
    }

    CHECK_OBJECT(result);

    return result;
}

// Returns a reference.
PFORGE_MAY_BE_UNUSED static PyObject *LOOKUP_BUILTIN_STR(char const *name) {
    CHECK_OBJECT(dict_builtin);

    PyObject *result = DICT_GET_ITEM_STRING((PyObject *)dict_builtin, name);

    // This is assumed to not fail, abort if it does.
    if (unlikely(result == NULL)) {
        PyErr_PrintEx(0);
        Py_Exit(1);
    }

    CHECK_OBJECT(result);

    Py_INCREF(result);
    return result;
}

extern void _initBuiltinModule(PyThreadState *tstate);

#define PFORGE_DECLARE_BUILTIN(name) extern PyObject *_python_original_builtin_value_##name;
#define PFORGE_DEFINE_BUILTIN(name) PyObject *_python_original_builtin_value_##name = NULL;
#define PFORGE_ASSIGN_BUILTIN(name)                                                                                    \
    if (_python_original_builtin_value_##name == NULL)                                                                 \
        _python_original_builtin_value_##name = LOOKUP_BUILTIN_STR(#name);
#define PFORGE_UPDATE_BUILTIN(name, value) _python_original_builtin_value_##name = value;
#define PFORGE_ACCESS_BUILTIN(name) (_python_original_builtin_value_##name)

#if !_PFORGE_MODULE_MODE
// Original builtin values, currently only used for assertions.
PFORGE_DECLARE_BUILTIN(type);
PFORGE_DECLARE_BUILTIN(len);
PFORGE_DECLARE_BUILTIN(range);
PFORGE_DECLARE_BUILTIN(repr);
PFORGE_DECLARE_BUILTIN(int);
PFORGE_DECLARE_BUILTIN(iter);
#if PYTHON_VERSION < 0x300
PFORGE_DECLARE_BUILTIN(long);
#endif

extern void _initBuiltinOriginalValues(void);
#endif

// Avoid the casts needed for older Python, as it's easily forgotten and
// potentially have our own better implementation later. Gives no reference.
// TODO: Can do it ourselves once DICT_GET_ITEM_WITH_ERROR becomes available.
PFORGE_MAY_BE_UNUSED static PyObject *Pforge_SysGetObject(char const *name) { return PySys_GetObject((char *)name); }

PFORGE_MAY_BE_UNUSED static void Pforge_SysSetObject(char const *name, PyObject *value) {
    // TODO: Check error in debug mode at least.
    PySys_SetObject((char *)name, value);
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
