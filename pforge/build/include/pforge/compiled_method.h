//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_COMPILED_METHOD_H__
#define __PFORGE_COMPILED_METHOD_H__

// Compiled function and compile generator types may be referenced.
#include "compiled_function.h"
#include "compiled_generator.h"

// The backbone of the integration into CPython. Try to behave as well as normal
// method objects, or even better.

// The Pforge_MethodObject is the storage associated with a compiled method
// instance of which there can be many for each code.

struct Pforge_MethodObject {
    /* Python object folklore: */
    PyObject_HEAD

        struct Pforge_FunctionObject *m_function;

    PyObject *m_weakrefs;

    PyObject *m_object;
    PyObject *m_class;

#if PYTHON_VERSION >= 0x380
    vectorcallfunc m_vectorcall;
#endif
};

extern PyTypeObject Pforge_Method_Type;

// Make a method out of a function.
extern PyObject *Pforge_Method_New(struct Pforge_FunctionObject *function, PyObject *obj, PyObject *class_object);

static inline bool Pforge_Method_Check(PyObject *obj) { return Py_TYPE(obj) == &Pforge_Method_Type; }

#if _DEBUG_REFCOUNTS
extern int count_active_Pforge_Method_Type;
extern int count_allocated_Pforge_Method_Type;
extern int count_released_Pforge_Method_Type;
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
