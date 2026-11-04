//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_COMPILED_CELL_H__
#define __PFORGE_COMPILED_CELL_H__

/* This is a clone of the normal PyCell structure. We should keep it binary
 * compatible, just in case somebody crazy insists on it.
 */

/* This file is included from another C file, help IDEs to still parse it on its own. */
#ifdef __IDE_ONLY__
#include "pforge/prelude.h"
#endif

extern PyTypeObject Pforge_Cell_Type;

static inline bool Pforge_Cell_Check(PyObject *obj) { return Py_TYPE(obj) == &Pforge_Cell_Type; }
static inline bool Pforge_CellOrPyCell_Check(PyObject *obj) { return Pforge_Cell_Check(obj) || PyCell_Check(obj); }

struct Pforge_CellObject {
    /* Python object folklore: */
    PyObject_HEAD

        /* Content of the cell or NULL when empty */
        PyObject *ob_ref;
};

// Create cell with out value, and with or without reference given.
extern struct Pforge_CellObject *Pforge_Cell_NewEmpty(void);
extern struct Pforge_CellObject *Pforge_Cell_New0(PyObject *value);
extern struct Pforge_CellObject *Pforge_Cell_New1(PyObject *value);

// Check stuff while accessing a compile cell in debug mode.
#ifdef __PFORGE_NO_ASSERT__
#define Pforge_Cell_GET(cell) (((struct Pforge_CellObject *)(cell))->ob_ref)
#else
#define Pforge_Cell_GET(cell)                                                                                          \
    (CHECK_OBJECT(cell), assert(Pforge_Cell_Check((PyObject *)cell)), (((struct Pforge_CellObject *)(cell))->ob_ref))
#endif

PFORGE_MAY_BE_UNUSED static inline PyObject *Pforge_CellOrPyCell_GET(PyObject *cell) {
    CHECK_OBJECT(cell);
    assert(Pforge_CellOrPyCell_Check(cell));

    return ((struct Pforge_CellObject *)cell)->ob_ref;
}

#if _DEBUG_REFCOUNTS
extern int count_active_Pforge_Cell_Type;
extern int count_allocated_Pforge_Cell_Type;
extern int count_released_Pforge_Cell_Type;
#endif

PFORGE_MAY_BE_UNUSED static inline void Pforge_Cell_SET(struct Pforge_CellObject *cell_object, PyObject *value) {
    CHECK_OBJECT_X(value);
    CHECK_OBJECT(cell_object);

    assert(Pforge_Cell_Check((PyObject *)cell_object));
    cell_object->ob_ref = value;
}

PFORGE_MAY_BE_UNUSED static inline void Pforge_CellOrPyCell_SET(PyObject *cell, PyObject *value) {
    CHECK_OBJECT_X(value);
    CHECK_OBJECT(cell);

    assert(Pforge_CellOrPyCell_Check(cell));
    ((struct Pforge_CellObject *)cell)->ob_ref = value;
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
