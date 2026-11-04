//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_DICTIONARIES_H__
#define __PFORGE_DICTIONARIES_H__

#ifdef __IDE_ONLY__
#include "pforge/allocator.h"
#include "pforge/checkers.h"
#include "pforge/compiled_module.h"
#include "pforge/cpython_api_compat.h"
#include "pforge/defines.h"
#include "pforge/string_functions.h"
#endif

static inline Py_ssize_t DICT_SIZE(PyObject *dict) {
    CHECK_OBJECT(dict);
    assert(PyDict_CheckExact(dict));

    return ((PyDictObject *)dict)->ma_used;
}

#if PYTHON_VERSION >= 0x3f0
static inline Py_ssize_t FROZENDICT_SIZE(PyObject *frozendict) {
    CHECK_OBJECT(frozendict);
    assert(PyFrozenDict_CheckExact(frozendict));

    return ((PyDictObject *)frozendict)->ma_used;
}
#endif

static inline PyDictObject *MODULE_DICT(PyObject *module) {
    CHECK_OBJECT(module);

    PyDictObject *dict = (PyDictObject *)(((PyModuleObject *)module)->md_dict);

    return dict;
}

#if PYTHON_VERSION < 0x300
// Quick dictionary lookup for a string value.

typedef PyDictEntry *Pforge_DictEntryHandle;

static PyDictEntry *GET_STRING_DICT_ENTRY(PyDictObject *dict, Pforge_StringObject *key) {
    assert(PyDict_CheckExact(dict));
    assert(Pforge_String_CheckExact(key));

    Py_hash_t hash = key->ob_shash;

    // Only improvement would be to identify how to ensure that the hash is
    // computed already. Calling hash early on could do that potentially.
    if (unlikely(hash == -1)) {
        hash = PyString_Type.tp_hash((PyObject *)key);
        key->ob_shash = hash;
    }

    PyDictEntry *entry = dict->ma_lookup(dict, (PyObject *)key, hash);

    // The "entry" cannot be NULL, it can only be empty for a string dict
    // lookup, but at least assert it.
    assert(entry != NULL);

    return entry;
}

PFORGE_MAY_BE_UNUSED static PyObject *GET_DICT_ENTRY_VALUE(Pforge_DictEntryHandle handle) { return handle->me_value; }

PFORGE_MAY_BE_UNUSED static void SET_DICT_ENTRY_VALUE(Pforge_DictEntryHandle handle, PyObject *value) {
    handle->me_value = value;
}

static PyObject *GET_STRING_DICT_VALUE(PyDictObject *dict, Pforge_StringObject *key) {
    return GET_STRING_DICT_ENTRY(dict, key)->me_value;
}

#else

// Python3

// Quick dictionary lookup for a string value.

#if PYTHON_VERSION < 0x3b0
typedef struct {
    /* Cached hash code of me_key. */
    Py_hash_t me_hash;
    PyObject *me_key;
    PyObject *me_value; /* This field is only meaningful for combined tables */
} PyDictKeyEntry;
#endif

#if PYTHON_VERSION < 0x360
typedef PyDictKeyEntry *(*dict_lookup_func)(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject ***value_addr);
#elif PYTHON_VERSION < 0x370
typedef Py_ssize_t (*dict_lookup_func)(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject ***value_addr,
                                       Py_ssize_t *hashpos);
#else
typedef Py_ssize_t (*dict_lookup_func)(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject **value_addr);
#endif

// Taken from CPython3.3 "Objects/dictobject.c", lives in "Objects/dict-common.h" later,

#if PYTHON_VERSION < 0x3b0

#define DK_SIZE(dk) ((dk)->dk_size)
struct _dictkeysobject { // spell-checker: ignore dictkeysobject
    Py_ssize_t dk_refcnt;
    Py_ssize_t dk_size;
    dict_lookup_func dk_lookup;
    Py_ssize_t dk_usable;
#if PYTHON_VERSION < 0x360
    PyDictKeyEntry dk_entries[1];
#else
    Py_ssize_t dk_nentries;
#if PYTHON_VERSION < 0x370
    union {
        int8_t as_1[8];
        int16_t as_2[4];
        int32_t as_4[2];
#if SIZEOF_VOID_P > 4
        int64_t as_8[1];
#endif
    } dk_indices;
#else
    char dk_indices[];
#endif
#endif
};

#endif

// Taken from Objects/dictobject.c of CPython 3.6
#if PYTHON_VERSION >= 0x360

#if PYTHON_VERSION < 0x3b0
#if SIZEOF_VOID_P > 4
// spell-checker: ignore DK_IXSIZE
#define DK_IXSIZE(dk)                                                                                                  \
    (DK_SIZE(dk) <= 0xff ? 1 : DK_SIZE(dk) <= 0xffff ? 2 : DK_SIZE(dk) <= 0xffffffff ? 4 : sizeof(int64_t))
#else
#define DK_IXSIZE(dk) (DK_SIZE(dk) <= 0xff ? 1 : DK_SIZE(dk) <= 0xffff ? 2 : sizeof(int32_t))
#endif

#if PYTHON_VERSION < 0x370
#define DK_ENTRIES(dk) ((PyDictKeyEntry *)(&(dk)->dk_indices.as_1[DK_SIZE(dk) * DK_IXSIZE(dk)]))
#else
#define DK_ENTRIES(dk) ((PyDictKeyEntry *)(&((int8_t *)((dk)->dk_indices))[DK_SIZE(dk) * DK_IXSIZE(dk)]))
#endif

#endif

#else
#define DK_ENTRIES(dk) (dk->dk_entries)
#endif

#if PYTHON_VERSION < 0x3b0
#define DK_VALUE(dk, i) dk->ma_values[i]
#else
#define DK_VALUE(dk, i) dk->ma_values->values[i]
#endif

#define DK_MASK(dk) (DK_SIZE(dk) - 1)

typedef PyObject **Pforge_DictEntryHandle;

#if PYTHON_VERSION >= 0x3b0
extern Py_ssize_t Pforge_Py_unicodekeys_lookup_unicode(PyDictKeysObject *dk, PyObject *key, Py_hash_t hash);

extern Py_ssize_t Pforge_PyDictLookup(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject ***value_addr);
extern Py_ssize_t Pforge_PyDictLookupStr(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject ***value_addr);
#endif

static inline Py_hash_t Pforge_Py_unicode_get_hash(PyObject *o) {
    assert(PyUnicode_CheckExact(o));

    return FT_ATOMIC_LOAD_SSIZE_RELAXED(((PyUnicodeObject *)o)->_base._base.hash);
}

static Pforge_DictEntryHandle GET_STRING_DICT_ENTRY(PyDictObject *dict, Pforge_StringObject *key) {
    assert(PyDict_CheckExact(dict));
    assert(Pforge_String_CheckExact(key));

    Py_hash_t hash = Pforge_Py_unicode_get_hash((PyObject *)key);

    // Only improvement would be to identify how to ensure that the hash is computed
    // already. Calling hash early on could do that potentially.
    if (hash == -1) {
        hash = PyUnicode_Type.tp_hash((PyObject *)key);
    }

#if PYTHON_VERSION < 0x360
    PyObject **value_addr;

    PyDictKeyEntry *entry = dict->ma_keys->dk_lookup(dict, (PyObject *)key, hash, &value_addr);

    // The "entry" cannot be NULL, it can only be empty for a string dict lookup, but at
    // least assert it.
    assert(entry != NULL);

    return value_addr;

#elif PYTHON_VERSION < 0x370
    PyObject **value_addr;

    // TODO: Find out what the returned Py_ssize_t "ix" might be good for.
    dict->ma_keys->dk_lookup(dict, (PyObject *)key, hash, &value_addr,
                             NULL // hashpos, TODO: Find out what we could use it for.
    );

    return value_addr;
#elif PYTHON_VERSION < 0x3b0
    PyObject *value;

    Py_ssize_t ix = dict->ma_keys->dk_lookup(dict, (PyObject *)key, hash, &value);

    if (value == NULL) {
        return NULL;
    } else if (_PyDict_HasSplitTable(dict)) {
        return &dict->ma_values[ix];
    } else {
        return &DK_ENTRIES(dict->ma_keys)[ix].me_value;
    }

#else
    PyObject **value;
    PFORGE_MAY_BE_UNUSED Py_ssize_t found;

    // We cannot use Pforge_PyDictLookupStr in all cases, some modules misbehave
    // and put non-strings in module dictionaries.
    if (unlikely(dict->ma_keys->dk_kind == DICT_KEYS_GENERAL)) {
        found = Pforge_PyDictLookup(dict, (PyObject *)key, hash, &value);
    } else {
        found = Pforge_PyDictLookupStr(dict, (PyObject *)key, hash, &value);
    }

    assert(found != DKIX_ERROR); // spell-checker: ignore DKIX_ERROR

    return value;
#endif
}

PFORGE_MAY_BE_UNUSED static PyObject *GET_DICT_ENTRY_VALUE(Pforge_DictEntryHandle handle) { return *handle; }

PFORGE_MAY_BE_UNUSED static void SET_DICT_ENTRY_VALUE(Pforge_DictEntryHandle handle, PyObject *value) {
    *handle = value;
}

PFORGE_MAY_BE_UNUSED static PyObject *GET_STRING_DICT_VALUE(PyDictObject *dict, Pforge_StringObject *key) {
    Pforge_DictEntryHandle handle = GET_STRING_DICT_ENTRY(dict, key);

#if PYTHON_VERSION >= 0x360
    if (handle == NULL) {
        return NULL;
    }
#endif

    return GET_DICT_ENTRY_VALUE(handle);
}

#endif

// Set a key in a dictionary, semantically identical to "PyDict_SetItem". On
// success, true is returned, otherwise false with an exception set.
extern bool DICT_SET_ITEM(PyObject *dict, PyObject *key, PyObject *value);

// Set a key in a dictionary, consuming the value reference. The key is
// borrowed. The value reference is consumed on success and on failure.
extern bool DICT_SET_ITEM1(PyObject *dict, PyObject *key, PyObject *value);

// Set a string key in a dictionary, semantically identical to
// "PyDict_SetItemString". On success, true is returned, otherwise false with
// an exception set.
extern bool DICT_SET_ITEM_STRING(PyObject *dict, char const *key, PyObject *value);

// Remove a key from a dictionary, semantically identical to "PyDict_DelItem",
// including the "KeyError" for missing keys. On success, true is returned.
extern bool DICT_REMOVE_ITEM(PyObject *dict, PyObject *key);

// Remove a string key from a dictionary, semantically identical to
// "PyDict_DelItemString". On success, true is returned.
extern bool DICT_REMOVE_ITEM_STRING(PyObject *dict, char const *key);

// Get a borrowed reference for a string key, similar to "PyDict_GetItemString".
// Hash errors are suppressed, but errors from key comparisons during the lookup
// are not cleared.
extern PyObject *DICT_GET_ITEM_STRING(PyObject *dict, char const *key);

// Update a dictionary from a mapping, semantically identical to "PyDict_Update".
extern int DICT_UPDATE_FROM(PyThreadState *tstate, PyObject *dict, PyObject *other);

// Update a dictionary from an iterable of key/value pairs, semantically
// identical to "PyDict_MergeFromSeq2" with override enabled.
extern int DICT_UPDATE_FROM_SEQ(PyThreadState *tstate, PyObject *dict, PyObject *seq);
// Set a KeyError for the given key, handling tuple keys and normalization.
extern void SET_CURRENT_EXCEPTION_KEY_ERROR(PyThreadState *tstate, PyObject *key);

// Get a dict lookup for a key, returning a new reference and raising KeyError
// on a miss. Not a silent "PyDict_GetItemWithError" equivalent.
extern PyObject *DICT_GET_ITEM_WITH_ERROR(PyThreadState *tstate, PyObject *dict, PyObject *key);

// Get a dict lookup for a key, propagating only hash errors and not creating a
// KeyError on a miss. "_1" returns a new reference, "_0" a borrowed one.
extern PyObject *DICT_GET_ITEM_WITH_HASH_ERROR1(PyThreadState *tstate, PyObject *dict, PyObject *key);

// Get dict lookup for a key, similar to PyDict_GetItem, 1=ref returned, 0=not
extern PyObject *DICT_GET_ITEM1(PyThreadState *tstate, PyObject *dict, PyObject *key);
extern PyObject *DICT_GET_ITEM0(PyThreadState *tstate, PyObject *dict, PyObject *key);

// Get dict lookup for a key, similar to PyDict_Contains
extern int DICT_HAS_ITEM(PyThreadState *tstate, PyObject *dict, PyObject *key);

#if PYTHON_VERSION >= 0x3c0 && !defined(Py_GIL_DISABLED)
// Check if a dictionary has CPython watchers attached. Such dictionaries must
// go through CPython for mutations to send watcher events and preserve the
// version tag bits used by the watcher machinery.
static inline bool PFORGE_DICT_IS_WATCHED(PyDictObject *dict) {
#if PYTHON_VERSION < 0x3d0
    return (dict->ma_version_tag & DICT_VERSION_MASK) != 0;
#elif PYTHON_VERSION < 0x3e0
    return (dict->ma_version_tag & DICT_WATCHER_MASK) != 0;
#else
    return (dict->_ma_watcher_tag & DICT_WATCHER_MASK) != 0;
#endif
}
#else
#define PFORGE_DICT_IS_WATCHED(dict) 0
#endif

// Convert to dictionary, helper for built-in "dict" mainly.
extern PyObject *TO_DICT(PyThreadState *tstate, PyObject *seq_obj, PyObject *dict_obj);

// For 3.6 to 3.12, the dictionary version tag is used for caching purposes, so
// we need to maintain it. However it's private value so we have to play a game
// of having our own number space inside of their 64 bit numbers.
#if PYTHON_VERSION >= 0x360 && PYTHON_VERSION < 0x3d0
#define _PFORGE_MAINTAIN_DICT_VERSION_TAG 1
#endif

#if _PFORGE_MAINTAIN_DICT_VERSION_TAG
extern uint64_t pforge_dict_version_tag_counter;
#endif

PFORGE_MAY_BE_UNUSED static void UPDATE_STRING_DICT0(PyDictObject *dict, Pforge_StringObject *key, PyObject *value) {
    CHECK_OBJECT(value);

#if PYTHON_VERSION >= 0x3c0 && !defined(Py_GIL_DISABLED)
    if (unlikely(PFORGE_DICT_IS_WATCHED(dict))) {
        DICT_SET_ITEM((PyObject *)dict, (PyObject *)key, value);
        return;
    }
#endif

    Pforge_DictEntryHandle entry = GET_STRING_DICT_ENTRY(dict, key);

#if PYTHON_VERSION >= 0x360
    if (entry == NULL) {
        DICT_SET_ITEM((PyObject *)dict, (PyObject *)key, value);
        return;
    }
#endif

    PyObject *old = GET_DICT_ENTRY_VALUE(entry);
    CHECK_OBJECT_X(old);

    if (value != old) {

        // Values are more likely (more often) set than not set, in that case
        // speculatively try the quickest access method.
        if (likely(old != NULL)) {
            Py_INCREF(value);
            SET_DICT_ENTRY_VALUE(entry, value);

#if _PFORGE_MAINTAIN_DICT_VERSION_TAG
            dict->ma_version_tag = pforge_dict_version_tag_counter++;
#endif
            CHECK_OBJECT(old);

            Py_DECREF(old);
        } else {
            DICT_SET_ITEM((PyObject *)dict, (PyObject *)key, value);
        }
    }
}

PFORGE_MAY_BE_UNUSED static void UPDATE_STRING_DICT_INPLACE(PyDictObject *dict, Pforge_StringObject *key,
                                                            PyObject *value) {
    CHECK_OBJECT(value);

#if PYTHON_VERSION >= 0x3c0 && !defined(Py_GIL_DISABLED)
    if (unlikely(PFORGE_DICT_IS_WATCHED(dict))) {
        DICT_SET_ITEM1((PyObject *)dict, (PyObject *)key, value);
        CHECK_OBJECT(value);
        return;
    }
#endif

    Pforge_DictEntryHandle entry = GET_STRING_DICT_ENTRY(dict, key);

#if PYTHON_VERSION >= 0x360
    if (entry == NULL) {
        DICT_SET_ITEM1((PyObject *)dict, (PyObject *)key, value);
        CHECK_OBJECT(value);

        return;
    }
#endif

    PyObject *old = GET_DICT_ENTRY_VALUE(entry);

    if (old == NULL) {
        DICT_SET_ITEM1((PyObject *)dict, (PyObject *)key, value);
        CHECK_OBJECT(value);
    } else {
        SET_DICT_ENTRY_VALUE(entry, value);

#if _PFORGE_MAINTAIN_DICT_VERSION_TAG
        dict->ma_version_tag = pforge_dict_version_tag_counter++;
#endif
        Py_DECREF(old);
    }
}

PFORGE_MAY_BE_UNUSED static void UPDATE_STRING_DICT1(PyDictObject *dict, Pforge_StringObject *key, PyObject *value) {
    CHECK_OBJECT(key);
    CHECK_OBJECT(value);

#if PYTHON_VERSION >= 0x3c0 && !defined(Py_GIL_DISABLED)
    if (unlikely(PFORGE_DICT_IS_WATCHED(dict))) {
        DICT_SET_ITEM1((PyObject *)dict, (PyObject *)key, value);
        CHECK_OBJECT(value);
        return;
    }
#endif

    Pforge_DictEntryHandle entry = GET_STRING_DICT_ENTRY(dict, key);

#if PYTHON_VERSION >= 0x360
    if (entry == NULL) {
        DICT_SET_ITEM1((PyObject *)dict, (PyObject *)key, value);
        CHECK_OBJECT(value);

        return;
    }
#endif

    PyObject *old = GET_DICT_ENTRY_VALUE(entry);
    CHECK_OBJECT_X(old);

    if (old != value) {
        // Values are more likely (more often) set than not set, in that case
        // speculatively try the quickest access method.
        if (likely(old != NULL)) {
            SET_DICT_ENTRY_VALUE(entry, value);

#if _PFORGE_MAINTAIN_DICT_VERSION_TAG
            dict->ma_version_tag = pforge_dict_version_tag_counter++;
#endif
            Py_DECREF(old);
        } else {
            DICT_SET_ITEM1((PyObject *)dict, (PyObject *)key, value);
            CHECK_OBJECT(value);
        }
    } else {
        Py_DECREF(value);
        CHECK_OBJECT(value);
    }
}

#if PYTHON_VERSION < 0x300
// Python2 dictionary keys, return a list of keys
extern PyObject *DICT_KEYS(PyObject *dict);
// Python2 dictionary items, return a list of values
extern PyObject *DICT_VALUES(PyObject *dict);
// Python2 dictionary items, return a list of key/value tuples
extern PyObject *DICT_ITEMS(PyObject *dict);
#endif

// Python3 dictionary keys, Python2 iterkeys returns dictionary keys iterator
extern PyObject *DICT_ITERKEYS(PyThreadState *tstate, PyObject *dict);

// Python3 dictionary values, Python2 itervalues returns dictionary values iterator
extern PyObject *DICT_ITERVALUES(PyThreadState *tstate, PyObject *dict);

// Python3 dictionary items, Python2 iteritems returns dictionary items iterator
extern PyObject *DICT_ITERITEMS(PyThreadState *tstate, PyObject *dict);

// Python dictionary keys view
extern PyObject *DICT_VIEWKEYS(PyObject *dict);

// Python dictionary values view
extern PyObject *DICT_VIEWVALUES(PyObject *dict);

// Python dictionary items view
extern PyObject *DICT_VIEWITEMS(PyObject *dict);

// Python dictionary copy, return a shallow copy of a dictionary.
extern PyObject *DICT_COPY(PyThreadState *tstate, PyObject *dict);

#if !_PFORGE_EXPERIMENTAL_DISABLE_DICT_OPT && PYTHON_VERSION >= 0x3a0 && !defined(Py_GIL_DISABLED)
// Clone a clean, compact combined dictionary into an empty target dictionary.
extern bool DICT_CLONE_INTO(PyThreadState *tstate, PyObject *target, PyObject *dict_value);
#endif

// Python dictionary clear, empties the dictionary.
extern void DICT_CLEAR(PyObject *dict);

// Replacement for PyDict_Next that is faster (to call).
extern bool Pforge_DictNext(PyObject *dict, Py_ssize_t *pos, PyObject **key_ptr, PyObject **value_ptr);

#if PYTHON_VERSION >= 0x3f0
// Replacement for PyDict_Next for frozendicts, that are always combined dicts.
extern bool Pforge_FrozenDictNext(PyObject *frozendict, Py_ssize_t *pos, PyObject **key_ptr, PyObject **value_ptr);
#endif

#if PYTHON_VERSION >= 0x3a0 && !defined(_PFORGE_EXPERIMENTAL_DISABLE_FREELIST_ALL) &&                                  \
    !defined(_PFORGE_EXPERIMENTAL_DISABLE_FREELIST_DICT)
#define PFORGE_DICT_HAS_FREELIST 1

// Replacement for PyDict_New that is faster
extern PyObject *MAKE_DICT_EMPTY(PyThreadState *tstate);
#else
#define PFORGE_DICT_HAS_FREELIST 0
static inline PyObject *MAKE_DICT_EMPTY(PyThreadState *tstate) {
    (void)tstate;

    PyObject *result = PyDict_New();
    assert(result != NULL);
    return result;
}
#endif

// Create a dictionary from key/value pairs.
extern PyObject *MAKE_DICT(PyObject **pairs, Py_ssize_t size);
// Create a dictionary from key/value pairs (NULL value means skip)
extern PyObject *MAKE_DICT_X(PyObject **pairs, Py_ssize_t size);
// Create a dictionary from key/value pairs (NULL value means skip) where keys are C strings.
extern PyObject *MAKE_DICT_X_CSTR(char const **keys, PyObject **values, Py_ssize_t size);

// It's probably unchanged across all Python versions 2.6-3.15
#if PYTHON_VERSION >= 0x3e0
typedef struct {
    PyObject_HEAD PyDictObject *di_dict;
    Py_ssize_t di_used;
    Py_ssize_t di_pos;
    PyObject *di_result;
    Py_ssize_t len;
} dictiterobject;
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
