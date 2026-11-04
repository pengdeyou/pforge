//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

/* WARNING, this code is GENERATED. Modify the template HelperDictionarySet.c.j2 instead! */
#pragma once

/* This file is included from another C file, help IDEs to still parse it on its own. */
#ifdef __IDE_ONLY__
#include "pforge/prelude.h"

#include "pforge/helper/dict_internals.h"
#endif

#if PYTHON_VERSION >= 0x300 && PYTHON_VERSION < 0x360
static int _Pforge_DictSetItemCompact(PyThreadState *tstate, PyDictObject *mp, PyObject *key, Py_hash_t hash,
                                      PyObject *value) {
    PyObject **value_addr;

    if (mp->ma_values != NULL && !PyUnicode_CheckExact(key)) {
        if (Pforge_DictResize(tstate, mp, Pforge_DictGrowthRate(mp)) < 0) {
            return -1;
        }
    }

    if (!PyUnicode_CheckExact(key)) {
        // CPython's unicode only lookup switches to the generic lookup for
        // these, and GC traversal and keyword argument validation rely on it.
        mp->ma_keys->dk_lookup = (dict_lookup_func)Pforge_GetGenericDictLookup();
    }

    PyDictKeyEntry *entry = Pforge_DictLookup34(mp, key, hash, &value_addr);

    if (unlikely(entry == NULL)) {
        return -1;
    }

    PFORGE_DICT_MAINTAIN_TRACKING(mp, key, value);

    PyObject *old_value = *value_addr;

    if (old_value != NULL) {
        if (old_value != value) {
            Py_INCREF(value);
            *value_addr = value;
            PFORGE_DICT_NEXT_VERSION(mp);
            Py_DECREF(old_value);
        }
        return 0;
    }

    if (entry->me_key == NULL) {
        if (mp->ma_keys->dk_usable <= 0) {
            if (Pforge_DictResize(tstate, mp, Pforge_DictGrowthRate(mp)) < 0) {
                return -1;
            }

            entry = Pforge_DictFindEmptySlot(mp, hash, &value_addr);
        }

        Py_INCREF(key);
        Py_INCREF(value);

        *value_addr = value;
        mp->ma_keys->dk_usable--;
        mp->ma_used++;

        entry->me_key = key;
        entry->me_hash = hash;

        PFORGE_DICT_NEXT_VERSION(mp);

        return 0;
    }

    Py_INCREF(value);

    *value_addr = value;
    mp->ma_used++;

    if (entry->me_key == Pforge_GetDictDummy()) {
        Py_INCREF(key);

        entry->me_key = key;
        entry->me_hash = hash;
        Py_DECREF(Pforge_GetDictDummy());
    } else {
        assert(mp->ma_values != NULL);
    }

    PFORGE_DICT_NEXT_VERSION(mp);

    return 0;
}
#elif PYTHON_VERSION >= 0x360 && PYTHON_VERSION < 0x3b0
static int _Pforge_DictSetItemCompact(PyThreadState *tstate, PyDictObject *mp, PyObject *key, Py_hash_t hash,
                                      PyObject *value) {
    PyObject *old_value = NULL;
    PyObject **value_addr = NULL;

    if (mp->ma_values != NULL && !PyUnicode_CheckExact(key)) {
        if (Pforge_DictInsertionResize(tstate, mp) < 0) {
            return -1;
        }
    }

    Py_ssize_t ix = Pforge_DictLookupCompact(mp, key, hash, &value_addr);

    if (unlikely(ix == DKIX_ERROR)) {
        return -1;
    }

    if (value_addr != NULL) {
        old_value = *value_addr;
    }

    PFORGE_DICT_MAINTAIN_TRACKING(mp, key, value);

    if (mp->ma_values != NULL && ((ix >= 0 && old_value == NULL && mp->ma_used != ix) ||
                                  (ix == DKIX_EMPTY && mp->ma_used != mp->ma_keys->dk_nentries))) {
        if (Pforge_DictInsertionResize(tstate, mp) < 0) {
            return -1;
        }

        ix = DKIX_EMPTY;
        old_value = NULL;
    }

    if (ix == DKIX_EMPTY) {
        assert(old_value == NULL);

        if (mp->ma_keys->dk_usable <= 0) {
            if (Pforge_DictInsertionResize(tstate, mp) < 0) {
                return -1;
            }
        }

        if (!PyUnicode_CheckExact(key)) {
            mp->ma_keys->dk_lookup = (dict_lookup_func)Pforge_GetGenericDictLookup();
        }

        Py_ssize_t new_hashpos = Pforge_DictFindEmptySlot(mp->ma_keys, hash);
        PyDictKeyEntry *entry = &DK_ENTRIES(mp->ma_keys)[mp->ma_keys->dk_nentries];

        Pforge_DictKeysSetIndex(mp->ma_keys, new_hashpos, mp->ma_keys->dk_nentries);

        Py_INCREF(key);
        Py_INCREF(value);

        entry->me_key = key;
        entry->me_hash = hash;

        if (mp->ma_values != NULL) {
            assert(mp->ma_values[mp->ma_keys->dk_nentries] == NULL);
            mp->ma_values[mp->ma_keys->dk_nentries] = value;
        } else {
            entry->me_value = value;
        }

        mp->ma_used++;
        PFORGE_DICT_NEXT_VERSION(mp);
        mp->ma_keys->dk_usable--;
        mp->ma_keys->dk_nentries++;

        return 0;
    }

    if (old_value != value) {
        Py_INCREF(value);

        if (mp->ma_values != NULL) {
            mp->ma_values[ix] = value;

            if (old_value == NULL) {
                assert(ix == mp->ma_used);
                mp->ma_used++;
            }
        } else {
            assert(old_value != NULL);
            DK_ENTRIES(mp->ma_keys)[ix].me_value = value;
        }

        PFORGE_DICT_NEXT_VERSION(mp);
        Py_XDECREF(old_value);
    }
    return 0;
}
#elif PYTHON_VERSION >= 0x3b0 && PYTHON_VERSION < 0x3d0
static int _Pforge_DictSetItemCompact(PyThreadState *tstate, PyDictObject *mp, PyObject *key, Py_hash_t hash,
                                      PyObject *value) {
    PyObject *old_value = NULL;

    if (DK_IS_UNICODE(mp->ma_keys) && !PyUnicode_CheckExact(key)) {
        if (Pforge_DictInsertionResize(tstate, mp, false) < 0) {
            return -1;
        }

        assert(mp->ma_keys->dk_kind == DICT_KEYS_GENERAL);
    }

    PyObject **value_addr;
    Py_ssize_t ix = Pforge_PyDictLookup(mp, key, hash, &value_addr);

    if (unlikely(ix == DKIX_ERROR)) {
        return -1;
    }

    if (ix >= 0) {
        old_value = *value_addr;
    }

    PFORGE_DICT_MAINTAIN_TRACKING(mp, key, value);

    if (ix == DKIX_EMPTY) {
        assert(old_value == NULL);

        if (mp->ma_keys->dk_usable <= 0) {
            if (Pforge_DictInsertionResize(tstate, mp, true) < 0) {
                return -1;
            }
        }

        mp->ma_keys->dk_version = 0;

        Py_ssize_t hashpos = Pforge_DictFindEmptySlot(mp->ma_keys, hash);

        Pforge_DictKeysSetIndex(mp->ma_keys, hashpos, mp->ma_keys->dk_nentries);

        Py_INCREF(key);
        Py_INCREF(value);

        if (DK_IS_UNICODE(mp->ma_keys)) {
            PyDictUnicodeEntry *entry = &DK_UNICODE_ENTRIES(mp->ma_keys)[mp->ma_keys->dk_nentries];

            entry->me_key = key;

            if (mp->ma_values == NULL) {
                entry->me_value = value;
            }
        } else {
            PyDictKeyEntry *entry = &DK_ENTRIES(mp->ma_keys)[mp->ma_keys->dk_nentries];

            entry->me_key = key;
            entry->me_hash = hash;

            if (mp->ma_values == NULL) {
                entry->me_value = value;
            }
        }

        if (mp->ma_values != NULL) {
            mp->ma_values->values[mp->ma_keys->dk_nentries] = value;
        }

        mp->ma_used++;

        if (mp->ma_values != NULL) {
            Pforge_DictValuesAddToInsertionOrder(mp->ma_values, mp->ma_keys->dk_nentries);
        }

        PFORGE_DICT_NEXT_VERSION(mp);
        mp->ma_keys->dk_usable--;
        mp->ma_keys->dk_nentries++;

        return 0;
    }

    if (old_value != value) {
        Py_INCREF(value);

        if (mp->ma_values != NULL) {
            mp->ma_values->values[ix] = value;

            if (old_value == NULL) {
                Pforge_DictValuesAddToInsertionOrder(mp->ma_values, ix);
                mp->ma_used++;
            }
        } else {
            assert(old_value != NULL);

            if (DK_IS_UNICODE(mp->ma_keys)) {
                DK_UNICODE_ENTRIES(mp->ma_keys)[ix].me_value = value;
            } else {
                DK_ENTRIES(mp->ma_keys)[ix].me_value = value;
            }
        }

        PFORGE_DICT_NEXT_VERSION(mp);
        Py_XDECREF(old_value);
    }
    return 0;
}
#endif

#if PYTHON_VERSION >= 0x300 && PYTHON_VERSION < 0x360
static int _Pforge_DictSetItemCompact1(PyThreadState *tstate, PyDictObject *mp, PyObject *key, Py_hash_t hash,
                                       PyObject *value) {
    PyObject **value_addr;

    if (mp->ma_values != NULL && !PyUnicode_CheckExact(key)) {
        if (Pforge_DictResize(tstate, mp, Pforge_DictGrowthRate(mp)) < 0) {
            return -1;
        }
    }

    if (!PyUnicode_CheckExact(key)) {
        // CPython's unicode only lookup switches to the generic lookup for
        // these, and GC traversal and keyword argument validation rely on it.
        mp->ma_keys->dk_lookup = (dict_lookup_func)Pforge_GetGenericDictLookup();
    }

    PyDictKeyEntry *entry = Pforge_DictLookup34(mp, key, hash, &value_addr);

    if (unlikely(entry == NULL)) {
        return -1;
    }

    PFORGE_DICT_MAINTAIN_TRACKING(mp, key, value);

    PyObject *old_value = *value_addr;

    if (old_value != NULL) {
        if (old_value != value) {
            *value_addr = value;
            PFORGE_DICT_NEXT_VERSION(mp);
            Py_DECREF(old_value);
        } else {
            Py_DECREF(value);
        }
        return 0;
    }

    if (entry->me_key == NULL) {
        if (mp->ma_keys->dk_usable <= 0) {
            if (Pforge_DictResize(tstate, mp, Pforge_DictGrowthRate(mp)) < 0) {
                return -1;
            }

            entry = Pforge_DictFindEmptySlot(mp, hash, &value_addr);
        }

        Py_INCREF(key);

        *value_addr = value;
        mp->ma_keys->dk_usable--;
        mp->ma_used++;

        entry->me_key = key;
        entry->me_hash = hash;

        PFORGE_DICT_NEXT_VERSION(mp);

        return 0;
    }

    *value_addr = value;
    mp->ma_used++;

    if (entry->me_key == Pforge_GetDictDummy()) {
        Py_INCREF(key);

        entry->me_key = key;
        entry->me_hash = hash;
        Py_DECREF(Pforge_GetDictDummy());
    } else {
        assert(mp->ma_values != NULL);
    }

    PFORGE_DICT_NEXT_VERSION(mp);

    return 0;
}
#elif PYTHON_VERSION >= 0x360 && PYTHON_VERSION < 0x3b0
static int _Pforge_DictSetItemCompact1(PyThreadState *tstate, PyDictObject *mp, PyObject *key, Py_hash_t hash,
                                       PyObject *value) {
    PyObject *old_value = NULL;
    PyObject **value_addr = NULL;

    if (mp->ma_values != NULL && !PyUnicode_CheckExact(key)) {
        if (Pforge_DictInsertionResize(tstate, mp) < 0) {
            return -1;
        }
    }

    Py_ssize_t ix = Pforge_DictLookupCompact(mp, key, hash, &value_addr);

    if (unlikely(ix == DKIX_ERROR)) {
        return -1;
    }

    if (value_addr != NULL) {
        old_value = *value_addr;
    }

    PFORGE_DICT_MAINTAIN_TRACKING(mp, key, value);

    if (mp->ma_values != NULL && ((ix >= 0 && old_value == NULL && mp->ma_used != ix) ||
                                  (ix == DKIX_EMPTY && mp->ma_used != mp->ma_keys->dk_nentries))) {
        if (Pforge_DictInsertionResize(tstate, mp) < 0) {
            return -1;
        }

        ix = DKIX_EMPTY;
        old_value = NULL;
    }

    if (ix == DKIX_EMPTY) {
        assert(old_value == NULL);

        if (mp->ma_keys->dk_usable <= 0) {
            if (Pforge_DictInsertionResize(tstate, mp) < 0) {
                return -1;
            }
        }

        if (!PyUnicode_CheckExact(key)) {
            mp->ma_keys->dk_lookup = (dict_lookup_func)Pforge_GetGenericDictLookup();
        }

        Py_ssize_t new_hashpos = Pforge_DictFindEmptySlot(mp->ma_keys, hash);
        PyDictKeyEntry *entry = &DK_ENTRIES(mp->ma_keys)[mp->ma_keys->dk_nentries];

        Pforge_DictKeysSetIndex(mp->ma_keys, new_hashpos, mp->ma_keys->dk_nentries);

        Py_INCREF(key);

        entry->me_key = key;
        entry->me_hash = hash;

        if (mp->ma_values != NULL) {
            assert(mp->ma_values[mp->ma_keys->dk_nentries] == NULL);
            mp->ma_values[mp->ma_keys->dk_nentries] = value;
        } else {
            entry->me_value = value;
        }

        mp->ma_used++;
        PFORGE_DICT_NEXT_VERSION(mp);
        mp->ma_keys->dk_usable--;
        mp->ma_keys->dk_nentries++;

        return 0;
    }

    if (old_value != value) {

        if (mp->ma_values != NULL) {
            mp->ma_values[ix] = value;

            if (old_value == NULL) {
                assert(ix == mp->ma_used);
                mp->ma_used++;
            }
        } else {
            assert(old_value != NULL);
            DK_ENTRIES(mp->ma_keys)[ix].me_value = value;
        }

        PFORGE_DICT_NEXT_VERSION(mp);
        Py_XDECREF(old_value);
    } else {
        Py_DECREF(value);
    }
    return 0;
}
#elif PYTHON_VERSION >= 0x3b0 && PYTHON_VERSION < 0x3d0
static int _Pforge_DictSetItemCompact1(PyThreadState *tstate, PyDictObject *mp, PyObject *key, Py_hash_t hash,
                                       PyObject *value) {
    PyObject *old_value = NULL;

    if (DK_IS_UNICODE(mp->ma_keys) && !PyUnicode_CheckExact(key)) {
        if (Pforge_DictInsertionResize(tstate, mp, false) < 0) {
            return -1;
        }

        assert(mp->ma_keys->dk_kind == DICT_KEYS_GENERAL);
    }

    PyObject **value_addr;
    Py_ssize_t ix = Pforge_PyDictLookup(mp, key, hash, &value_addr);

    if (unlikely(ix == DKIX_ERROR)) {
        return -1;
    }

    if (ix >= 0) {
        old_value = *value_addr;
    }

    PFORGE_DICT_MAINTAIN_TRACKING(mp, key, value);

    if (ix == DKIX_EMPTY) {
        assert(old_value == NULL);

        if (mp->ma_keys->dk_usable <= 0) {
            if (Pforge_DictInsertionResize(tstate, mp, true) < 0) {
                return -1;
            }
        }

        mp->ma_keys->dk_version = 0;

        Py_ssize_t hashpos = Pforge_DictFindEmptySlot(mp->ma_keys, hash);

        Pforge_DictKeysSetIndex(mp->ma_keys, hashpos, mp->ma_keys->dk_nentries);

        Py_INCREF(key);

        if (DK_IS_UNICODE(mp->ma_keys)) {
            PyDictUnicodeEntry *entry = &DK_UNICODE_ENTRIES(mp->ma_keys)[mp->ma_keys->dk_nentries];

            entry->me_key = key;

            if (mp->ma_values == NULL) {
                entry->me_value = value;
            }
        } else {
            PyDictKeyEntry *entry = &DK_ENTRIES(mp->ma_keys)[mp->ma_keys->dk_nentries];

            entry->me_key = key;
            entry->me_hash = hash;

            if (mp->ma_values == NULL) {
                entry->me_value = value;
            }
        }

        if (mp->ma_values != NULL) {
            mp->ma_values->values[mp->ma_keys->dk_nentries] = value;
        }

        mp->ma_used++;

        if (mp->ma_values != NULL) {
            Pforge_DictValuesAddToInsertionOrder(mp->ma_values, mp->ma_keys->dk_nentries);
        }

        PFORGE_DICT_NEXT_VERSION(mp);
        mp->ma_keys->dk_usable--;
        mp->ma_keys->dk_nentries++;

        return 0;
    }

    if (old_value != value) {

        if (mp->ma_values != NULL) {
            mp->ma_values->values[ix] = value;

            if (old_value == NULL) {
                Pforge_DictValuesAddToInsertionOrder(mp->ma_values, ix);
                mp->ma_used++;
            }
        } else {
            assert(old_value != NULL);

            if (DK_IS_UNICODE(mp->ma_keys)) {
                DK_UNICODE_ENTRIES(mp->ma_keys)[ix].me_value = value;
            } else {
                DK_ENTRIES(mp->ma_keys)[ix].me_value = value;
            }
        }

        PFORGE_DICT_NEXT_VERSION(mp);
        Py_XDECREF(old_value);
    } else {
        Py_DECREF(value);
    }
    return 0;
}
#endif

#if PYTHON_VERSION >= 0x3d0 && !defined(Py_GIL_DISABLED)
static void _Pforge_DictInsertSplitValue(PyDictObject *mp, PyObject *value, Py_ssize_t ix) {
    PyObject *old_value = mp->ma_values->values[ix];

    if (old_value == NULL) {
        mp->ma_values->values[ix] = Py_NewRef(value);
        Pforge_DictValuesAddToInsertionOrder(mp->ma_values, ix);
        mp->ma_used++;
    } else if (old_value != value) {
        mp->ma_values->values[ix] = Py_NewRef(value);
        Py_DECREF(old_value);
    }
}
#endif

#if PYTHON_VERSION >= 0x3d0 && !defined(Py_GIL_DISABLED)
static void _Pforge_DictInsertSplitValue1(PyDictObject *mp, PyObject *value, Py_ssize_t ix) {
    PyObject *old_value = mp->ma_values->values[ix];

    if (old_value == NULL) {
        mp->ma_values->values[ix] = value;
        Pforge_DictValuesAddToInsertionOrder(mp->ma_values, ix);
        mp->ma_used++;
    } else if (old_value != value) {
        mp->ma_values->values[ix] = value;
        Py_DECREF(old_value);
    } else {
        Py_DECREF(value);
    }
}
#endif

#if PYTHON_VERSION >= 0x3d0 && !defined(Py_GIL_DISABLED)
static int _Pforge_DictSetItem3x(PyThreadState *tstate, PyDictObject *mp, PyObject *key, Py_hash_t hash,
                                 PyObject *value) {
    PyObject *old_value = NULL;
    Py_ssize_t ix;

    PFORGE_DICT_MAINTAIN_TRACKING(mp, key, value);

    if (mp->ma_values != NULL && PyUnicode_CheckExact(key)) {
        ix = Pforge_DictInsertSplitKey(mp->ma_keys, key, hash);

        if (ix != DKIX_EMPTY) {
            _Pforge_DictInsertSplitValue(mp, value, ix);
            return 0;
        }
    } else {
        PyObject **value_addr;
        ix = Pforge_PyDictLookup(mp, key, hash, &value_addr);

        if (unlikely(ix == DKIX_ERROR)) {
            return -1;
        }

        if (ix >= 0) {
            old_value = *value_addr;
        }
    }

    if (old_value == NULL) {
        if (DK_IS_UNICODE(mp->ma_keys) && !PyUnicode_CheckExact(key)) {
            if (Pforge_DictInsertionResize(tstate, mp, false) < 0) {
                return -1;
            }

            assert(mp->ma_keys->dk_kind == DICT_KEYS_GENERAL);
        } else if (mp->ma_values != NULL) {
            if (Pforge_DictInsertionResize(tstate, mp, true) < 0) {
                return -1;
            }
        }

        if (mp->ma_keys->dk_usable <= 0) {
            if (Pforge_DictInsertionResize(tstate, mp, true) < 0) {
                return -1;
            }
        }

        mp->ma_keys->dk_version = 0;

        Py_ssize_t new_ix = mp->ma_keys->dk_nentries;
        Py_ssize_t hashpos = Pforge_DictFindEmptySlot(mp->ma_keys, hash);
        Pforge_DictKeysSetIndex(mp->ma_keys, hashpos, new_ix);

        Py_INCREF(key);
        Py_INCREF(value);

        if (DK_IS_UNICODE(mp->ma_keys)) {
            PyDictUnicodeEntry *entry = &DK_UNICODE_ENTRIES(mp->ma_keys)[new_ix];

            entry->me_key = key;

            if (mp->ma_values != NULL) {
                mp->ma_values->values[new_ix] = value;
                Pforge_DictValuesAddToInsertionOrder(mp->ma_values, new_ix);
            } else {
                entry->me_value = value;
            }
        } else {
            PyDictKeyEntry *entry = &DK_ENTRIES(mp->ma_keys)[new_ix];

            entry->me_key = key;
            entry->me_hash = hash;
            entry->me_value = value;
        }

        mp->ma_used++;
        PFORGE_DICT_NEXT_VERSION(mp);
        mp->ma_keys->dk_usable--;
        mp->ma_keys->dk_nentries++;

        return 0;
    }

    if (old_value != value) {
        Py_INCREF(value);

        if (mp->ma_values != NULL) {
            mp->ma_values->values[ix] = value;
        } else if (DK_IS_UNICODE(mp->ma_keys)) {
            DK_UNICODE_ENTRIES(mp->ma_keys)[ix].me_value = value;
        } else {
            DK_ENTRIES(mp->ma_keys)[ix].me_value = value;
        }

        PFORGE_DICT_NEXT_VERSION(mp);
        Py_DECREF(old_value);
    }
    return 0;
}
#endif

#if PYTHON_VERSION >= 0x3d0 && !defined(Py_GIL_DISABLED)
static int _Pforge_DictSetItem3x1(PyThreadState *tstate, PyDictObject *mp, PyObject *key, Py_hash_t hash,
                                  PyObject *value) {
    PyObject *old_value = NULL;
    Py_ssize_t ix;

    PFORGE_DICT_MAINTAIN_TRACKING(mp, key, value);

    if (mp->ma_values != NULL && PyUnicode_CheckExact(key)) {
        ix = Pforge_DictInsertSplitKey(mp->ma_keys, key, hash);

        if (ix != DKIX_EMPTY) {
            _Pforge_DictInsertSplitValue1(mp, value, ix);
            return 0;
        }
    } else {
        PyObject **value_addr;
        ix = Pforge_PyDictLookup(mp, key, hash, &value_addr);

        if (unlikely(ix == DKIX_ERROR)) {
            return -1;
        }

        if (ix >= 0) {
            old_value = *value_addr;
        }
    }

    if (old_value == NULL) {
        if (DK_IS_UNICODE(mp->ma_keys) && !PyUnicode_CheckExact(key)) {
            if (Pforge_DictInsertionResize(tstate, mp, false) < 0) {
                return -1;
            }

            assert(mp->ma_keys->dk_kind == DICT_KEYS_GENERAL);
        } else if (mp->ma_values != NULL) {
            if (Pforge_DictInsertionResize(tstate, mp, true) < 0) {
                return -1;
            }
        }

        if (mp->ma_keys->dk_usable <= 0) {
            if (Pforge_DictInsertionResize(tstate, mp, true) < 0) {
                return -1;
            }
        }

        mp->ma_keys->dk_version = 0;

        Py_ssize_t new_ix = mp->ma_keys->dk_nentries;
        Py_ssize_t hashpos = Pforge_DictFindEmptySlot(mp->ma_keys, hash);
        Pforge_DictKeysSetIndex(mp->ma_keys, hashpos, new_ix);

        Py_INCREF(key);

        if (DK_IS_UNICODE(mp->ma_keys)) {
            PyDictUnicodeEntry *entry = &DK_UNICODE_ENTRIES(mp->ma_keys)[new_ix];

            entry->me_key = key;

            if (mp->ma_values != NULL) {
                mp->ma_values->values[new_ix] = value;
                Pforge_DictValuesAddToInsertionOrder(mp->ma_values, new_ix);
            } else {
                entry->me_value = value;
            }
        } else {
            PyDictKeyEntry *entry = &DK_ENTRIES(mp->ma_keys)[new_ix];

            entry->me_key = key;
            entry->me_hash = hash;
            entry->me_value = value;
        }

        mp->ma_used++;
        PFORGE_DICT_NEXT_VERSION(mp);
        mp->ma_keys->dk_usable--;
        mp->ma_keys->dk_nentries++;

        return 0;
    }

    if (old_value != value) {

        if (mp->ma_values != NULL) {
            mp->ma_values->values[ix] = value;
        } else if (DK_IS_UNICODE(mp->ma_keys)) {
            DK_UNICODE_ENTRIES(mp->ma_keys)[ix].me_value = value;
        } else {
            DK_ENTRIES(mp->ma_keys)[ix].me_value = value;
        }

        PFORGE_DICT_NEXT_VERSION(mp);
        Py_DECREF(old_value);
    } else {
        Py_DECREF(value);
    }
    return 0;
}
#endif

#if PYTHON_VERSION < 0x300
static int _Pforge_DictSetItemLegacy(PyThreadState *tstate, PyDictObject *mp, PyObject *key, Py_hash_t hash,
                                     PyObject *value) {
    assert(key != Pforge_GetDictDummy());

    PyDictEntry *entry = Pforge_DictLookupLegacy(mp, key, hash);

    if (unlikely(entry == NULL)) {
        return -1;
    }

    PFORGE_DICT_MAINTAIN_TRACKING(mp, key, value);

    if (entry->me_value != NULL) {
        PyObject *old_value = entry->me_value;

        if (old_value != value) {
            Py_INCREF(value);
            entry->me_value = value;
            Py_DECREF(old_value);
        }
        return 0;
    }

    Py_INCREF(key);
    Py_INCREF(value);

    if (entry->me_key == NULL) {
        mp->ma_fill++;
    } else {
        assert(entry->me_key == Pforge_GetDictDummy());
        Py_DECREF(Pforge_GetDictDummy());
    }

    entry->me_key = key;
    entry->me_hash = (Py_ssize_t)hash;
    entry->me_value = value;
    mp->ma_used++;

    if (mp->ma_fill * 3 >= (mp->ma_mask + 1) * 2) {
        if (Pforge_DictResizeLegacy(mp, (mp->ma_used > 50000 ? 2 : 4) * mp->ma_used) < 0) {
            return -1;
        }
    }

    return 0;
}

#endif

#if PYTHON_VERSION < 0x300
static int _Pforge_DictSetItemLegacy1(PyThreadState *tstate, PyDictObject *mp, PyObject *key, Py_hash_t hash,
                                      PyObject *value) {
    assert(key != Pforge_GetDictDummy());

    PyDictEntry *entry = Pforge_DictLookupLegacy(mp, key, hash);

    if (unlikely(entry == NULL)) {
        return -1;
    }

    PFORGE_DICT_MAINTAIN_TRACKING(mp, key, value);

    if (entry->me_value != NULL) {
        PyObject *old_value = entry->me_value;

        if (old_value != value) {
            entry->me_value = value;
            Py_DECREF(old_value);
        } else {
            Py_DECREF(value);
        }
        return 0;
    }

    Py_INCREF(key);

    if (entry->me_key == NULL) {
        mp->ma_fill++;
    } else {
        assert(entry->me_key == Pforge_GetDictDummy());
        Py_DECREF(Pforge_GetDictDummy());
    }

    entry->me_key = key;
    entry->me_hash = (Py_ssize_t)hash;
    entry->me_value = value;
    mp->ma_used++;

    if (mp->ma_fill * 3 >= (mp->ma_mask + 1) * 2) {
        if (Pforge_DictResizeLegacy(mp, (mp->ma_used > 50000 ? 2 : 4) * mp->ma_used) < 0) {
            return -1;
        }
    }

    return 0;
}

#endif

bool DICT_SET_ITEM(PyObject *dict, PyObject *key, PyObject *value) {
    CHECK_OBJECT(dict);
    assert(PyDict_Check(dict));
    CHECK_OBJECT(key);
    CHECK_OBJECT(value);

#if !defined(Py_GIL_DISABLED) && PYTHON_VERSION >= 0x3c0
    if (unlikely(PFORGE_DICT_IS_WATCHED((PyDictObject *)dict))) {
        int status = PyDict_Type.tp_as_mapping->mp_ass_subscript(dict, key, value);

        return status == 0;
    }
#endif

#if _PFORGE_EXPERIMENTAL_DISABLE_DICT_OPT || _PFORGE_EXPERIMENTAL_DISABLE_DICT_ACCESS_OPT
    int status = PyDict_SetItem(dict, key, value);

    return status == 0;
#elif PYTHON_VERSION >= 0x300 && !defined(Py_GIL_DISABLED)
    PyThreadState *tstate = PyThreadState_GET();
    Py_hash_t hash;

    if (unlikely(Pforge_DictGetHash(tstate, key, &hash) == false)) {
        return false;
    }

    bool result;

#if PYTHON_VERSION >= 0x3d0
    result = _Pforge_DictSetItem3x(tstate, (PyDictObject *)dict, key, hash, value) == 0;
#elif PYTHON_VERSION >= 0x300
    result = _Pforge_DictSetItemCompact(tstate, (PyDictObject *)dict, key, hash, value) == 0;
#else
    result = _Pforge_DictSetItemLegacy(tstate, (PyDictObject *)dict, key, hash, value) == 0;
#endif

    return result;
#elif PYTHON_VERSION < 0x300
    PyThreadState *tstate = PyThreadState_GET();
    Py_hash_t hash;

    if (unlikely(Pforge_DictGetHash(tstate, key, &hash) == false)) {
        return false;
    }

    bool result = _Pforge_DictSetItemLegacy(tstate, (PyDictObject *)dict, key, hash, value) == 0;

    return result;
#else
    int status = PyDict_Type.tp_as_mapping->mp_ass_subscript(dict, key, value);

    return status == 0;
#endif
}
bool DICT_SET_ITEM1(PyObject *dict, PyObject *key, PyObject *value) {
    CHECK_OBJECT(dict);
    assert(PyDict_Check(dict));
    CHECK_OBJECT(key);
    CHECK_OBJECT(value);

#if !defined(Py_GIL_DISABLED) && PYTHON_VERSION >= 0x3c0
    if (unlikely(PFORGE_DICT_IS_WATCHED((PyDictObject *)dict))) {
        int status = PyDict_Type.tp_as_mapping->mp_ass_subscript(dict, key, value);

        Py_DECREF(value);
        return status == 0;
    }
#endif

#if _PFORGE_EXPERIMENTAL_DISABLE_DICT_OPT || _PFORGE_EXPERIMENTAL_DISABLE_DICT_ACCESS_OPT
    int status = PyDict_SetItem(dict, key, value);

    Py_DECREF(value);
    return status == 0;
#elif PYTHON_VERSION >= 0x300 && !defined(Py_GIL_DISABLED)
    PyThreadState *tstate = PyThreadState_GET();
    Py_hash_t hash;

    if (unlikely(Pforge_DictGetHash(tstate, key, &hash) == false)) {
        Py_DECREF(value);
        return false;
    }

    bool result;

#if PYTHON_VERSION >= 0x3d0
    result = _Pforge_DictSetItem3x1(tstate, (PyDictObject *)dict, key, hash, value) == 0;
#elif PYTHON_VERSION >= 0x300
    result = _Pforge_DictSetItemCompact1(tstate, (PyDictObject *)dict, key, hash, value) == 0;
#else
    result = _Pforge_DictSetItemLegacy1(tstate, (PyDictObject *)dict, key, hash, value) == 0;
#endif

    if (!result) {
        Py_DECREF(value);
    }

    return result;
#elif PYTHON_VERSION < 0x300
    PyThreadState *tstate = PyThreadState_GET();
    Py_hash_t hash;

    if (unlikely(Pforge_DictGetHash(tstate, key, &hash) == false)) {
        Py_DECREF(value);
        return false;
    }

    bool result = _Pforge_DictSetItemLegacy1(tstate, (PyDictObject *)dict, key, hash, value) == 0;

    if (!result) {
        Py_DECREF(value);
    }

    return result;
#else
    int status = PyDict_Type.tp_as_mapping->mp_ass_subscript(dict, key, value);

    Py_DECREF(value);
    return status == 0;
#endif
}

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
