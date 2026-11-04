//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_HELPER_DICT_INTERNALS_H__
#define __PFORGE_HELPER_DICT_INTERNALS_H__

/* Shared helpers for the dictionary internals.
 *
 * The implementations are in "HelpersDictionaries.c", which is chained into
 * the same compilation unit, and therefore the declarations are "static" to
 * it. The generated dictionary helpers use them too.
 *
 * This file is included from another C file, help IDEs to still parse it on its own.
 */
#ifdef __IDE_ONLY__
#include "pforge/prelude.h"
#endif

// Usable fraction of keys.
#define DK_USABLE_FRACTION(n) (((n) << 1) / 3)

#if PYTHON_VERSION < 0x3b0
typedef PyObject *PyDictValues;
#endif

#if PYTHON_VERSION < 0x360
#define DK_ENTRIES_SIZE(keys) (keys->dk_size)
#elif PYTHON_VERSION < 0x3b0
#define DK_ENTRIES_SIZE(keys) DK_USABLE_FRACTION(DK_SIZE(keys))
#else
#define DK_ENTRIES_SIZE(keys) (keys->dk_nentries)
#endif

// More than 2/3 of the keys are used, i.e. no space is wasted.
#if PYTHON_VERSION < 0x360
#define IS_COMPACT(dict_mp) (dict_mp->ma_used >= (dict_mp->ma_keys->dk_size * 2) / 3)
#else
#define IS_COMPACT(dict_mp) (dict_mp->ma_used >= (dict_mp->ma_keys->dk_nentries * 2) / 3)
#endif

#if _PFORGE_MAINTAIN_DICT_VERSION_TAG
#define PFORGE_DICT_NEXT_VERSION(dict_mp) ((dict_mp)->ma_version_tag = pforge_dict_version_tag_counter++)
#else
#define PFORGE_DICT_NEXT_VERSION(dict_mp)
#endif

#define PFORGE_DICT_MAINTAIN_TRACKING(dict_mp, key, value)                                                             \
    do {                                                                                                               \
        if (!_PyObject_GC_IS_TRACKED(dict_mp) &&                                                                       \
            (_PyObject_GC_MAY_BE_TRACKED(key) || _PyObject_GC_MAY_BE_TRACKED(value))) {                                \
            Pforge_GC_Track(dict_mp);                                                                                  \
        }                                                                                                              \
    } while (0)

// Declarations of the helpers that the chained generated dictionary code
// uses, they are implemented in "HelpersDictionaries.c".
PFORGE_MAY_BE_UNUSED static bool Pforge_DictGetHash(PyThreadState *tstate, PyObject *key, Py_hash_t *hash_ptr);

#if PYTHON_VERSION < 0x3b0
static void *Pforge_GetGenericDictLookup(void);
#endif

#if PYTHON_VERSION < 0x360
static PyObject *Pforge_GetDictDummy(void);
#endif

#if PYTHON_VERSION < 0x300
static PyDictEntry *Pforge_DictLookupLegacy(PyDictObject *mp, PyObject *key, Py_hash_t hash);
static int Pforge_DictResizeLegacy(PyDictObject *mp, Py_ssize_t minused);
#elif PYTHON_VERSION < 0x360
static PyDictKeyEntry *Pforge_DictLookup34(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject ***value_addr);
static PyDictKeyEntry *Pforge_DictFindEmptySlot(PyDictObject *mp, Py_hash_t hash, PyObject ***value_addr);
static int Pforge_DictResize(PyThreadState *tstate, PyDictObject *mp, Py_ssize_t minused);
static Py_ssize_t Pforge_DictGrowthRate(PyDictObject *mp);
#elif PYTHON_VERSION < 0x3b0
static Py_ssize_t Pforge_DictLookupCompact(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject ***value_addr);
static Py_ssize_t Pforge_DictLookupCompactResult(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject **result);
static void Pforge_DictKeysSetIndex(PyDictKeysObject *keys, Py_ssize_t i, Py_ssize_t ix);
static Py_ssize_t Pforge_DictFindEmptySlot(PyDictKeysObject *keys, Py_hash_t hash);
static int Pforge_DictResize(PyThreadState *tstate, PyDictObject *mp, Py_ssize_t new_size);
static int Pforge_DictInsertionResize(PyThreadState *tstate, PyDictObject *mp);
#elif !defined(Py_GIL_DISABLED)
static void Pforge_DictKeysSetIndex(PyDictKeysObject *keys, Py_ssize_t i, Py_ssize_t ix);
static void Pforge_DictValuesAddToInsertionOrder(PyDictValues *values, Py_ssize_t ix);
static Py_ssize_t Pforge_DictFindEmptySlot(PyDictKeysObject *keys, Py_hash_t hash);
static int Pforge_DictResize(PyThreadState *tstate, PyDictObject *mp, uint8_t log2_new_size, bool unicode);
static int Pforge_DictInsertionResize(PyThreadState *tstate, PyDictObject *mp, bool unicode);
#if PYTHON_VERSION >= 0x3d0
static Py_ssize_t Pforge_DictInsertSplitKey(PyDictKeysObject *keys, PyObject *key, Py_hash_t hash);
#endif
#endif

#if PYTHON_VERSION >= 0x300 && (PFORGE_DICT_HAS_FREELIST || !_PFORGE_EXPERIMENTAL_DISABLE_DICT_OPT)
static PyDictObject *_Pforge_AllocatePyDictObject(PyThreadState *tstate);
#endif
#if PYTHON_VERSION >= 0x360
static PyDictKeysObject *_Pforge_AllocatePyDictKeysObject(PyThreadState *tstate, Py_ssize_t keys_size);
static Py_ssize_t _Pforge_Py_PyDict_KeysSize(PyDictKeysObject *keys);
#endif
static PyDictValues *_Pforge_PyDict_new_values(Py_ssize_t size);
#if PYTHON_VERSION >= 0x3d0
static PyDictValues *_Pforge_PyDict_copy_values(PyDictValues *values);
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
