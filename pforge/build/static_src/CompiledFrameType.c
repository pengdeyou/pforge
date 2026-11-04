//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#ifdef __IDE_ONLY__
#include "pforge/prelude.h"

#include "pforge/compiled_types_common.h"
#include "pforge/freelists.h"
#include <structmember.h>
#endif

#if _PFORGE_FRAME_LOCALS_PROXY
#include "CompiledFrameLocalsProxyType.c"
#endif

// For reporting about reference counts per type.
#if _DEBUG_REFCOUNTS
int count_active_Pforge_Frame_Type = 0;
int count_allocated_Pforge_Frame_Type = 0;
int count_released_Pforge_Frame_Type = 0;
#endif

// For reporting about frame cache usage
#if _DEBUG_REFCOUNTS
int count_active_frame_cache_instances = 0;
int count_allocated_frame_cache_instances = 0;
int count_released_frame_cache_instances = 0;
int count_hit_frame_cache_instances = 0;
#endif

#if PYTHON_VERSION < 0x3b0
static PyMemberDef Pforge_Frame_members[] = {
    {(char *)"f_back", T_OBJECT, offsetof(PyFrameObject, f_back), READONLY | RESTRICTED},
    {(char *)"f_code", T_OBJECT, offsetof(PyFrameObject, f_code), READONLY | RESTRICTED},
    {(char *)"f_builtins", T_OBJECT, offsetof(PyFrameObject, f_builtins), READONLY | RESTRICTED},
    {(char *)"f_globals", T_OBJECT, offsetof(PyFrameObject, f_globals), READONLY | RESTRICTED},
    {(char *)"f_lasti", T_INT, offsetof(PyFrameObject, f_lasti), READONLY | RESTRICTED},
    {NULL}};

#else
#define Pforge_Frame_members 0
#endif

#if PYTHON_VERSION < 0x300

static PyObject *_Pforge_Frame_get_exc_traceback(PyObject *self, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    struct Pforge_FrameObject *frame = (struct Pforge_FrameObject *)self;
    PyObject *result = frame->m_frame.f_exc_traceback;

    if (result == NULL) {
        result = Py_None;
    }

    Py_INCREF(result);
    return result;
}

static int _Pforge_Frame_set_exc_traceback(PyObject *self, PyObject *traceback, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    struct Pforge_FrameObject *frame = (struct Pforge_FrameObject *)self;
    Py_XDECREF(frame->m_frame.f_exc_traceback);

    if (traceback == Py_None) {
        traceback = NULL;
    }

    frame->m_frame.f_exc_traceback = traceback;
    Py_XINCREF(traceback);

    return 0;
}

static PyObject *_Pforge_Frame_get_exc_type(PyObject *self, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    PyObject *result;
    struct Pforge_FrameObject *frame = (struct Pforge_FrameObject *)self;

    if (frame->m_frame.f_exc_type != NULL) {
        result = frame->m_frame.f_exc_type;
    } else {
        result = Py_None;
    }

    Py_INCREF(result);
    return result;
}

static int _Pforge_Frame_set_exc_type(PyObject *self, PyObject *exception_type, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    struct Pforge_FrameObject *frame = (struct Pforge_FrameObject *)self;
    PyObject *old = frame->m_frame.f_exc_type;

    if (exception_type == Py_None) {
        exception_type = NULL;
    }

    frame->m_frame.f_exc_type = exception_type;
    Py_XINCREF(frame->m_frame.f_exc_type);

    Py_XDECREF(old);

    return 0;
}

static PyObject *_Pforge_Frame_get_exc_value(PyObject *self, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    PyObject *result;
    struct Pforge_FrameObject *frame = (struct Pforge_FrameObject *)self;

    if (frame->m_frame.f_exc_value != NULL) {
        result = frame->m_frame.f_exc_value;
    } else {
        result = Py_None;
    }

    Py_INCREF(result);
    return result;
}

static int _Pforge_Frame_set_exc_value(PyObject *self, PyObject *exception_value, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    struct Pforge_FrameObject *frame = (struct Pforge_FrameObject *)self;
    PyObject *old = frame->m_frame.f_exc_value;

    if (exception_value == Py_None) {
        exception_value = NULL;
    }

    frame->m_frame.f_exc_value = exception_value;
    Py_XINCREF(exception_value);
    Py_XDECREF(old);

    return 0;
}

static PyObject *_Pforge_Frame_get_restricted(PyObject *self, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    Py_INCREF_IMMORTAL(Py_False);
    return Py_False;
}

#endif

// The number of entries in a frame variable description, which matches the
// variable names of the code object. For Python before 3.11, "co_nlocals" is
// deliberately zero to keep CPython's "PyFrame_FastToLocals" from walking the
// frame's "f_localsplus", which has no object pointer layout, so the variable
// names tuple is used for the count.
PFORGE_MAY_BE_UNUSED static Py_ssize_t Pforge_GetCodeLocalsCount(PyCodeObject *code_object) {
#if PYTHON_VERSION < 0x3b0
    return PyTuple_GET_SIZE(code_object->co_varnames);
#else
    return code_object->co_nlocals;
#endif
}

static PyObject *_Pforge_Frame_GetLocalsDict(struct Pforge_FrameObject *pforge_frame) {
    PFORGE_MAY_BE_UNUSED PyThreadState *tstate = PyThreadState_GET();

    PyObject *result = MAKE_DICT_EMPTY(tstate);

    // The published pointer is the authoritative storage: the stack struct
    // while the frame executes, or the frame's copy after the exception
    // detach. It is NULL once the frame exited normally.
    char const *storage = (char *)pforge_frame->m_locals_ptr;

    if (storage == NULL) {
        return result;
    }

    PyObject **var_names = Pforge_GetCodeVarNames(Pforge_GetFrameCodeObject(pforge_frame));

    PyCodeObject *co = Pforge_GetFrameCodeObject(pforge_frame);
    unsigned char const *w = (unsigned char const *)pforge_frame->m_type_description;
    size_t offset = 0;

    Py_ssize_t locals_count = Pforge_GetCodeLocalsCount(co);
    assert(w[locals_count] == 0);

    for (Py_ssize_t i = 0; i < locals_count; i++) {
        int type = w[i];
        offset = Pforge_FrameLocals_AlignUp(offset, Pforge_FrameLocals_Align(type));
        char const *t = storage + offset;

        switch (type) {
        case PFORGE_TYPE_DESCRIPTION_OBJECT:
        case PFORGE_TYPE_DESCRIPTION_OBJECT_PTR: {
            PyObject *value = *(PyObject **)t;
            CHECK_OBJECT_X(value);

            if (value != NULL) {
                DICT_SET_ITEM(result, *var_names, value);
            }

            break;
        }
        case PFORGE_TYPE_DESCRIPTION_CELL: {
            struct Pforge_CellObject *value = *(struct Pforge_CellObject **)t;

            if (value != NULL) {
                assert(Pforge_CellOrPyCell_Check((PyObject *)value));
                CHECK_OBJECT(value);

                PyObject *cell_value = Pforge_CellOrPyCell_GET((PyObject *)value);

                if (cell_value != NULL) {
                    DICT_SET_ITEM(result, *var_names, cell_value);
                }
            }

            break;
        }
        case PFORGE_TYPE_DESCRIPTION_NULL: {
            break;
        }
        case PFORGE_TYPE_DESCRIPTION_BOOL: {
            pforge_bool value = *(pforge_bool *)t;

            switch (value) {
            case PFORGE_BOOL_TRUE: {
                DICT_SET_ITEM(result, *var_names, Py_True);
                break;
            }
            case PFORGE_BOOL_FALSE: {
                DICT_SET_ITEM(result, *var_names, Py_False);
                break;
            }
            default:
                break;
            }
            break;
        }
        case PFORGE_TYPE_DESCRIPTION_NILONG: {
            pforge_ilong const *value = (pforge_ilong const *)t;

            if (IS_NILONG_OBJECT_VALUE_VALID(value)) {
                DICT_SET_ITEM(result, *var_names, value->python_value);
            } else if (IS_NILONG_C_VALUE_VALID(value)) {
                PyObject *boxed = Pforge_PyLong_FromLong(value->c_value);

                if (boxed != NULL) {
                    DICT_SET_ITEM1(result, *var_names, boxed);
                }
            }

            break;
        }
        default:
            assert(false);
        }

        offset += Pforge_FrameLocals_Size(type);
        var_names += 1;
    }

    return result;
}

static PyObject *_Pforge_Frame_get_locals(PyObject *self, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    PFORGE_MAY_BE_UNUSED PyThreadState *tstate = PyThreadState_GET();

    struct Pforge_FrameObject *pforge_frame = (struct Pforge_FrameObject *)self;
    if (pforge_frame->m_type_description == NULL) {
#if PYTHON_VERSION < 0x3b0
        PyFrameObject *locals_owner = &pforge_frame->m_frame;
#else
        _PyInterpreterFrame *locals_owner = &pforge_frame->m_interpreter_frame;
#endif

        if (locals_owner->f_locals == NULL) {
            locals_owner->f_locals = MAKE_DICT_EMPTY(tstate);
        }

        Py_INCREF(locals_owner->f_locals);
        return locals_owner->f_locals;
    } else {
#if PYTHON_VERSION >= 0x3d0 && _PFORGE_FRAME_LOCALS_PROXY
        if (pforge_frame->m_locals_writable) {
            // Return a FrameLocalsProxy.
            return _Pforge_FrameLocalsProxy_New(pforge_frame);
        }
#endif

        // Snapshot dict: pre-3.13, or when writable locals are disabled and
        // the locals are only a copy made at exception time.
        return _Pforge_Frame_GetLocalsDict(pforge_frame);
    }
}

// Attach an exposed locals mapping to a compiled frame, with Python2 class fallback.
void Pforge_Frame_AssignLocals(struct Pforge_FrameObject *frame_object, PyObject *locals_value) {
    assert(Pforge_Frame_CheckExact((PyObject *)frame_object));
    CHECK_OBJECT((PyObject *)frame_object);
    CHECK_OBJECT(locals_value);

#if PYTHON_VERSION < 0x300
    PFORGE_MAY_BE_UNUSED PyThreadState *tstate = PyThreadState_GET();
#endif

#if PYTHON_VERSION < 0x3b0
    PyFrameObject *locals_owner = &frame_object->m_frame;
#else
    _PyInterpreterFrame *locals_owner = &frame_object->m_interpreter_frame;
#endif

    assert(locals_owner->f_locals == NULL);

#if PYTHON_VERSION < 0x300
    if (PyModule_Check(locals_value)) {
        // Class frames in Python2 expose a minimal locals dict with "__module__".
        PyObject *module_name = MODULE_NAME1(tstate, locals_value);
        PyObject *kw_pairs[2] = {const_str_plain___module__, module_name};

        locals_owner->f_locals = MAKE_DICT(kw_pairs, 1);
        Py_DECREF(module_name);
        return;
    }
#endif

    locals_owner->f_locals = locals_value;
    Py_INCREF(locals_owner->f_locals);
}

// Drop an exposed locals mapping from a compiled frame.
void Pforge_Frame_ClearLocals(struct Pforge_FrameObject *frame_object) {
    assert(Pforge_Frame_CheckExact((PyObject *)frame_object));
    CHECK_OBJECT((PyObject *)frame_object);

#if PYTHON_VERSION < 0x3b0
    PyFrameObject *locals_owner = &frame_object->m_frame;
#else
    _PyInterpreterFrame *locals_owner = &frame_object->m_interpreter_frame;
#endif

    Py_CLEAR(locals_owner->f_locals);
}

static PyObject *_Pforge_Frame_get_lineno(PyObject *self, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    struct Pforge_FrameObject *frame = (struct Pforge_FrameObject *)self;
    return Pforge_PyInt_FromLong(frame->m_frame.f_lineno);
}

static PyObject *_Pforge_Frame_get_trace(PyObject *self, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    struct Pforge_FrameObject *frame = (struct Pforge_FrameObject *)self;
    PyObject *result = frame->m_frame.f_trace;
    Py_INCREF(result);
    return result;
}

static int _Pforge_Frame_set_trace(PyObject *self, PyObject *value, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));
#if !defined(_PFORGE_DEPLOYMENT_MODE) && !defined(_PFORGE_NO_DEPLOYMENT_FRAME_USELESS_SET_TRACE)
    if (value == Py_None) {
        return 0;
    } else {
        PyThreadState *tstate = PyThreadState_GET();

        SET_CURRENT_EXCEPTION_TYPE0_STR(
            tstate, PyExc_RuntimeError,
            "f_trace is not writable in Pforge, ignore with '--no-deployment-flag=frame-useless-set-trace'");
        return -1;
    }
#else
    return 0;
#endif
}

#if PYTHON_VERSION >= 0x370
static PyObject *_Pforge_Frame_get_trace_lines(PyObject *self, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    PyObject *result = Py_False;
    Py_INCREF_IMMORTAL(result);
    return result;
}

static int _Pforge_Frame_set_trace_lines(PyObject *self, PyObject *value, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    PyThreadState *tstate = PyThreadState_GET();

    SET_CURRENT_EXCEPTION_TYPE0_STR(tstate, PyExc_RuntimeError, "f_trace_lines is not writable in Pforge");
    return -1;
}

static PyObject *_Pforge_Frame_get_trace_opcodes(PyObject *self, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    PyObject *result = Py_False;
    Py_INCREF_IMMORTAL(result);
    return result;
}

static int _Pforge_Frame_set_trace_opcodes(PyObject *self, PyObject *value, void *data) {
    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT(self);
    assert(_PyObject_GC_IS_TRACKED(self));

    PyThreadState *tstate = PyThreadState_GET();

    SET_CURRENT_EXCEPTION_TYPE0_STR(tstate, PyExc_RuntimeError, "f_trace_opcodes is not writable in Pforge");
    return -1;
}
#endif

static PyGetSetDef Pforge_Frame_tp_getset[] = {
    {(char *)"f_locals", _Pforge_Frame_get_locals, NULL, NULL},
    {(char *)"f_lineno", _Pforge_Frame_get_lineno, NULL, NULL},
    {(char *)"f_trace", _Pforge_Frame_get_trace, _Pforge_Frame_set_trace, NULL},
#if PYTHON_VERSION < 0x300
    {(char *)"f_restricted", _Pforge_Frame_get_restricted, NULL, NULL},
    {(char *)"f_exc_traceback", _Pforge_Frame_get_exc_traceback, _Pforge_Frame_set_exc_traceback, NULL},
    {(char *)"f_exc_type", _Pforge_Frame_get_exc_type, _Pforge_Frame_set_exc_type, NULL},
    {(char *)"f_exc_value", _Pforge_Frame_get_exc_value, _Pforge_Frame_set_exc_value, NULL},
#endif
#if PYTHON_VERSION >= 0x370
    {(char *)"f_trace_lines", _Pforge_Frame_get_trace_lines, _Pforge_Frame_set_trace_lines, NULL},
    {(char *)"f_trace_opcodes", _Pforge_Frame_get_trace_opcodes, _Pforge_Frame_set_trace_opcodes, NULL},
#endif
    {NULL}};

// tp_repr slot, decide how a function shall be output
static PyObject *Pforge_Frame_tp_repr(PyObject *self) {
    struct Pforge_FrameObject *pforge_frame = (struct Pforge_FrameObject *)self;

    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT((PyObject *)pforge_frame);
    assert(_PyObject_GC_IS_TRACKED(pforge_frame));

#if PYTHON_VERSION >= 0x370
    PyCodeObject *code_object = Pforge_GetFrameCodeObject(pforge_frame);
    return Pforge_String_FromFormat("<compiled_frame at %p, file %R, line %d, code %S>", pforge_frame,
                                    code_object->co_filename, Pforge_GetFrameLineNumber(pforge_frame),
                                    code_object->co_name);
#elif _DEBUG_FRAME || _DEBUG_REFRAME || _DEBUG_EXCEPTIONS
    PyCodeObject *code_object = Pforge_GetFrameCodeObject(pforge_frame);
    return Pforge_String_FromFormat("<compiled_frame object for %s at %p>",
                                    Pforge_String_AsString(code_object->co_name), pforge_frame);
#else
    return Pforge_String_FromFormat("<compiled_frame object at %p>", pforge_frame);
#endif
}

// Release the values in a locals storage given an explicit type description.
// Used by frame clear/dealloc.
static void _Pforge_ReleaseLocalsWithDescription(char const *type_description, void *storage, int locals_count);

static void _Pforge_Frame_tp_clear(struct Pforge_FrameObject *frame) {
    if (frame->m_locals_ptr != NULL && frame->m_type_description != NULL) {
        int locals_count = (int)Pforge_GetCodeLocalsCount(Pforge_GetFrameCodeObject(frame));

        _Pforge_ReleaseLocalsWithDescription(frame->m_type_description, frame->m_locals_ptr, locals_count);
    }

    frame->m_locals_ptr = NULL;
    frame->m_type_description = NULL;
    frame->m_locals_writable = 0;

#if PYTHON_VERSION >= 0x3d0
    Py_CLEAR(frame->m_frame.f_extra_locals);
    Py_CLEAR(frame->m_frame.f_locals_cache);
#endif
#if PYTHON_VERSION >= 0x3e0
    Py_CLEAR(frame->m_frame.f_overwritten_fast_locals);
#endif
}

static int Pforge_Frame_tp_clear(PyObject *frame_obj) {
    _Pforge_Frame_tp_clear((struct Pforge_FrameObject *)frame_obj);
    return 0;
}

// Freelist setup
#define MAX_FRAME_FREE_LIST_COUNT 100
static struct Pforge_FrameObject *free_list_frames = NULL;
static int free_list_frames_count = 0;
PFORGE_FREELIST_DECLARE_LOCK(free_list_frames)

static void Pforge_Frame_tp_dealloc(PyObject *self) {
    struct Pforge_FrameObject *pforge_frame = (struct Pforge_FrameObject *)self;

#if _DEBUG_REFCOUNTS
    count_active_Pforge_Frame_Type -= 1;
    count_released_Pforge_Frame_Type += 1;
#endif

#ifndef __PFORGE_NO_ASSERT__
    // Save the current exception, if any, we must to not corrupt it.
    PyThreadState *tstate = PyThreadState_GET();

    struct Pforge_ExceptionPreservationItem saved_exception_state1;
    FETCH_ERROR_OCCURRED_STATE(tstate, &saved_exception_state1);
    RESTORE_ERROR_OCCURRED_STATE(tstate, &saved_exception_state1);
#endif

    if (_PyObject_GC_IS_TRACKED(pforge_frame)) {
        Pforge_GC_UnTrack(pforge_frame);
    }

    PyFrameObject *frame = &pforge_frame->m_frame;
#if PYTHON_VERSION < 0x3b0
    PyFrameObject *locals_owner = frame;
#else
    _PyInterpreterFrame *locals_owner = &pforge_frame->m_interpreter_frame;
#endif

    assert(Pforge_GC_IS_TRACKED_X((PyObject *)frame->f_back));
    Py_XDECREF(frame->f_back);
    Py_DECREF(locals_owner->f_builtins);
    Py_DECREF(locals_owner->f_globals);
    Py_XDECREF(locals_owner->f_locals);

    _Pforge_Frame_tp_clear(pforge_frame);

#if PYTHON_VERSION >= 0x3e0
    PyStackRef_CLEAR(locals_owner->f_executable);
#endif

#if PYTHON_VERSION < 0x370
    Py_XDECREF(frame->f_exc_type);
    Py_XDECREF(frame->f_exc_value);
    Py_XDECREF(frame->f_exc_traceback);
#endif

    if (Py_REFCNT(pforge_frame) > 0) {
        Py_SET_REFCNT(pforge_frame, Py_REFCNT(pforge_frame) - 1);
        if (Py_REFCNT(pforge_frame) >= 1) {
            // TODO: Allow this in debug mode, for now we would like to reproduce it.
            assert(false);
            return;
        }
    }

#if PYTHON_VERSION >= 0x3b0
    // Restore from backup, see header comment for the field "m_ob_size" to get
    // it.
    Py_SET_SIZE(pforge_frame, pforge_frame->m_ob_size);
#endif

    assert(pforge_frame->m_frame_cache == NULL);

    releaseToFreeList(free_list_frames, pforge_frame, MAX_FRAME_FREE_LIST_COUNT);

#ifndef __PFORGE_NO_ASSERT__
    struct Pforge_ExceptionPreservationItem saved_exception_state2;
    FETCH_ERROR_OCCURRED_STATE(tstate, &saved_exception_state2);
    RESTORE_ERROR_OCCURRED_STATE(tstate, &saved_exception_state2);

    ASSERT_SAME_EXCEPTION_STATE(&saved_exception_state1, &saved_exception_state2);
#endif
}

static int Pforge_Frame_tp_traverse(PyObject *self, visitproc visit, void *arg) {
    struct Pforge_FrameObject *frame = (struct Pforge_FrameObject *)self;

    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT((PyObject *)frame);
    assert(_PyObject_GC_IS_TRACKED(frame));

    Py_VISIT(frame->m_frame.f_back);

#if PYTHON_VERSION < 0x3b0
    PyFrameObject *locals_owner = &frame->m_frame;
#else
    _PyInterpreterFrame *locals_owner = &frame->m_interpreter_frame;
#endif

    Py_VISIT(locals_owner->f_builtins);
    Py_VISIT(locals_owner->f_globals);
    Py_VISIT(locals_owner->f_locals);

#if PYTHON_VERSION >= 0x3d0
    Py_VISIT(frame->m_frame.f_extra_locals);
    Py_VISIT(frame->m_frame.f_locals_cache);
#endif
#if PYTHON_VERSION >= 0x3e0
    Py_VISIT(frame->m_frame.f_overwritten_fast_locals);
#endif

#if PYTHON_VERSION < 0x370
    Py_VISIT(frame->m_frame.f_exc_type);
    Py_VISIT(frame->m_frame.f_exc_value);
    Py_VISIT(frame->m_frame.f_exc_traceback);
#endif

    // Traverse attached locals too.
    if (frame->m_type_description != NULL && frame->m_locals_ptr != NULL) {
        PyCodeObject *co = Pforge_GetFrameCodeObject(frame);
        unsigned char const *w = (unsigned char const *)frame->m_type_description;
        size_t offset = 0;

        Py_ssize_t locals_count = Pforge_GetCodeLocalsCount(co);
        assert(w[locals_count] == 0);

        for (Py_ssize_t i = 0; i < locals_count; i++) {
            int type = w[i];
            offset = Pforge_FrameLocals_AlignUp(offset, Pforge_FrameLocals_Align(type));
            void *slot = (char *)frame->m_locals_ptr + offset;
            switch (type) {
            case PFORGE_TYPE_DESCRIPTION_OBJECT:
            case PFORGE_TYPE_DESCRIPTION_OBJECT_PTR: {
                PyObject *value = *(PyObject **)slot;
                CHECK_OBJECT_X(value);
                Py_VISIT(value);
                break;
            }
            case PFORGE_TYPE_DESCRIPTION_CELL: {
                struct Pforge_CellObject *value = *(struct Pforge_CellObject **)slot;
                if (value != NULL) {
                    assert(Pforge_CellOrPyCell_Check((PyObject *)value));
                    CHECK_OBJECT(value);
                    Py_VISIT(value);
                }
                break;
            }
            case PFORGE_TYPE_DESCRIPTION_NILONG: {
                pforge_ilong *value = (pforge_ilong *)slot;
                if (IS_NILONG_OBJECT_VALUE_VALID(value)) {
                    CHECK_OBJECT(value->python_value);
                    Py_VISIT(value->python_value);
                }
                break;
            }
            case PFORGE_TYPE_DESCRIPTION_BOOL:
            case PFORGE_TYPE_DESCRIPTION_NULL:
                break;
            default:
                PFORGE_CANNOT_GET_HERE("invalid type in type description");
            }
            offset += Pforge_FrameLocals_Size(type);
        }
    }

    return 0;
}

#if PYTHON_VERSION >= 0x300

static PyObject *Pforge_GetFrameGenerator(struct Pforge_FrameObject *pforge_frame) {
#if PYTHON_VERSION < 0x3b0
    return pforge_frame->m_frame.f_gen;
#else
    return pforge_frame->m_generator;
#endif
}

static PyObject *Pforge_Frame_clear(PyObject *self, PyObject *unused) {
    struct Pforge_FrameObject *frame = (struct Pforge_FrameObject *)self;

    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT((PyObject *)frame);
    assert(_PyObject_GC_IS_TRACKED(frame));

    PyThreadState *tstate = PyThreadState_GET();

    if (Pforge_Frame_IsExecuting(frame)) {
        SET_CURRENT_EXCEPTION_TYPE0_STR(tstate, PyExc_RuntimeError, "cannot clear an executing frame");

        return NULL;
    }

#if PYTHON_VERSION >= 0x3d0
    // CPython 3.13+ only rejects clearing a generated-owned suspended frame.
    // Frame-object-owned frames can always be cleared, matching CPython's
    // frame_clear logic.
    PyObject *frame_gen = Pforge_GetFrameGenerator(frame);
    if ((frame_gen != NULL) && Pforge_Frame_IsSuspended(frame)) {
        SET_CURRENT_EXCEPTION_TYPE0_STR(tstate, PyExc_RuntimeError, "cannot clear a suspended frame");

        return NULL;
    }
#endif

#if PYTHON_VERSION >= 0x3b0
    if (frame->m_frame_state == FRAME_COMPLETED) {
        _Pforge_Frame_tp_clear(frame);

        Py_RETURN_NONE;
    }
#endif

#if PYTHON_VERSION >= 0x300
    // For frames that are closed, we also need to close the generator.
    PyObject *f_gen = Pforge_GetFrameGenerator(frame);
    if (f_gen != NULL) {
        CHECK_OBJECT(f_gen);

        Py_INCREF(frame);

        bool close_exception;

        if (Pforge_Generator_Check(f_gen)) {
            struct Pforge_GeneratorObject *generator = (struct Pforge_GeneratorObject *)f_gen;
            Pforge_SetFrameGenerator(frame, NULL);

            close_exception = !_Pforge_Generator_close(tstate, generator);
        }
#if PYTHON_VERSION >= 0x350
        else if (Pforge_Coroutine_Check(f_gen)) {
            struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)f_gen;
            Pforge_SetFrameGenerator(frame, NULL);

            close_exception = !_Pforge_Coroutine_close(tstate, coroutine);
        }
#endif
#if PYTHON_VERSION >= 0x360
        else if (Pforge_Asyncgen_Check(f_gen)) {
            struct Pforge_AsyncgenObject *asyncgen = (struct Pforge_AsyncgenObject *)f_gen;
            Pforge_SetFrameGenerator(frame, NULL);

            close_exception = !_Pforge_Asyncgen_close(tstate, asyncgen);
        }
#endif
        else {
            // Compiled frames should only have our types, so this ought to not happen.
            assert(false);

            Pforge_SetFrameGenerator(frame, NULL);

            close_exception = false;
        }

        if (unlikely(close_exception)) {
            PyErr_WriteUnraisable(f_gen);
        }

        Py_DECREF(frame);
    }
#endif

    _Pforge_Frame_tp_clear(frame);

    Py_RETURN_NONE;
}

#endif

static inline Py_ssize_t Pforge_Frame_GetSize(struct Pforge_FrameObject *frame) {
    assert(Pforge_Frame_CheckExact((PyObject *)frame));
    CHECK_OBJECT((PyObject *)frame);
    assert(_PyObject_GC_IS_TRACKED(frame));

#if PYTHON_VERSION < 0x3b0
    return Py_SIZE(frame);
#else
    return frame->m_ob_size;
#endif
}

// Bytes available in the frame's locals storage, which is located after the
// CPython localsplus slots starting with Python 3.11.
static inline size_t Pforge_Frame_GetStorageSize(struct Pforge_FrameObject *frame) {
#if PYTHON_VERSION >= 0x3b0
    return (size_t)Pforge_Frame_GetSize(frame) - Pforge_GetFrameCodeObject(frame)->co_nlocalsplus * sizeof(PyObject *);
#else
    return (size_t)Pforge_Frame_GetSize(frame);
#endif
}

static PyObject *Pforge_Frame_sizeof(PyObject *self, PyObject *unused) {
    struct Pforge_FrameObject *frame = (struct Pforge_FrameObject *)self;

    assert(Pforge_Frame_CheckExact(self));
    CHECK_OBJECT((PyObject *)frame);
    assert(_PyObject_GC_IS_TRACKED(frame));

    return PyInt_FromSsize_t(sizeof(struct Pforge_FrameObject) + Py_SIZE(frame));
}

static PyMethodDef Pforge_Frame_methods[] = {
#if PYTHON_VERSION >= 0x300
    {"clear", (PyCFunction)Pforge_Frame_clear, METH_NOARGS, "F.clear(): clear most references held by the frame"},
#endif
    {"__sizeof__", (PyCFunction)Pforge_Frame_sizeof, METH_NOARGS, "F.__sizeof__() -> size of F in memory, in bytes"},
    {NULL, NULL}};

PyTypeObject Pforge_Frame_Type = {
    PyVarObject_HEAD_INIT(NULL, 0) "compiled_frame",
    sizeof(struct Pforge_FrameObject),
    1,
    (destructor)Pforge_Frame_tp_dealloc,     // tp_dealloc
    0,                                       // tp_print
    0,                                       // tp_getattr
    0,                                       // tp_setattr
    0,                                       // tp_compare
    (reprfunc)Pforge_Frame_tp_repr,          // tp_repr
    0,                                       // tp_as_number
    0,                                       // tp_as_sequence
    0,                                       // tp_as_mapping
    0,                                       // tp_hash
    0,                                       // tp_call
    0,                                       // tp_str
    0,                                       // tp_getattro (PyObject_GenericGetAttr)
    0,                                       // tp_setattro (PyObject_GenericSetAttr)
    0,                                       // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, // tp_flags
    0,                                       // tp_doc
    (traverseproc)Pforge_Frame_tp_traverse,  // tp_traverse
    (inquiry)Pforge_Frame_tp_clear,          // tp_clear
    0,                                       // tp_richcompare
    0,                                       // tp_weaklistoffset
    0,                                       // tp_iter
    0,                                       // tp_iternext
    Pforge_Frame_methods,                    // tp_methods
    Pforge_Frame_members,                    // tp_members
    Pforge_Frame_tp_getset,                  // tp_getset
    0,                                       // tp_base
    0,                                       // tp_dict
};

void _initCompiledFrameType(void) {
    assert(Pforge_Frame_Type.tp_doc != PyFrame_Type.tp_doc || PyFrame_Type.tp_doc == NULL);
    assert(Pforge_Frame_Type.tp_traverse != PyFrame_Type.tp_traverse);
    assert(Pforge_Frame_Type.tp_clear != PyFrame_Type.tp_clear || PyFrame_Type.tp_clear == NULL);
    assert(Pforge_Frame_Type.tp_richcompare != PyFrame_Type.tp_richcompare || PyFrame_Type.tp_richcompare == NULL);
    assert(Pforge_Frame_Type.tp_weaklistoffset != PyFrame_Type.tp_weaklistoffset ||
           PyFrame_Type.tp_weaklistoffset == 0);
    assert(Pforge_Frame_Type.tp_iter != PyFrame_Type.tp_iter || PyFrame_Type.tp_iter == NULL);
    assert(Pforge_Frame_Type.tp_iternext != PyFrame_Type.tp_iternext || PyFrame_Type.tp_iternext == NULL);
    assert(Pforge_Frame_Type.tp_methods != PyFrame_Type.tp_methods);
    assert(Pforge_Frame_Type.tp_members != PyFrame_Type.tp_members);
    assert(Pforge_Frame_Type.tp_getset != PyFrame_Type.tp_getset);

    assert(Pforge_Frame_Type.tp_descr_get != PyFrame_Type.tp_descr_get || PyFrame_Type.tp_descr_get == NULL);

    assert(Pforge_Frame_Type.tp_descr_set != PyFrame_Type.tp_descr_set || PyFrame_Type.tp_descr_set == NULL);
    assert(Pforge_Frame_Type.tp_dictoffset != PyFrame_Type.tp_dictoffset || PyFrame_Type.tp_dictoffset == 0);
    // TODO: These get changed and into the same thing, not sure what to compare against, project something
    // assert(Pforge_Frame_Type.tp_dict != PyFrame_Type.tp_dict);
    // assert(Pforge_Frame_Type.tp_init != PyFrame_Type.tp_init || PyFrame_Type.tp_init == NULL);
    // assert(Pforge_Frame_Type.tp_alloc != PyFrame_Type.tp_alloc || PyFrame_Type.tp_alloc == NULL);
    // assert(Pforge_Frame_Type.tp_new != PyFrame_Type.tp_new || PyFrame_Type.tp_new == NULL);
    // assert(Pforge_Frame_Type.tp_free != PyFrame_Type.tp_free || PyFrame_Type.tp_free == NULL);
    // assert(Pforge_Frame_Type.tp_weaklist != PyFrame_Type.tp_weaklist);
    assert(Pforge_Frame_Type.tp_bases != PyFrame_Type.tp_bases);
    assert(Pforge_Frame_Type.tp_mro != PyFrame_Type.tp_mro);
    assert(Pforge_Frame_Type.tp_cache != PyFrame_Type.tp_cache || PyFrame_Type.tp_cache == NULL);
    assert(Pforge_Frame_Type.tp_subclasses != PyFrame_Type.tp_subclasses || PyFrame_Type.tp_cache == NULL);
    assert(Pforge_Frame_Type.tp_del != PyFrame_Type.tp_del || PyFrame_Type.tp_del == NULL);
#if PYTHON_VERSION >= 0x300
    assert(Pforge_Frame_Type.tp_finalize != PyFrame_Type.tp_finalize || PyFrame_Type.tp_finalize == NULL);
#endif
    Pforge_PyType_Ready(&Pforge_Frame_Type, // type
                        &PyFrame_Type,      // base
                        true,               // generic_get_attr
                        true,               // generic_set_attr
                        false,              // self_iter
                        false,              // await_self_iter
                        false               // await_self_aiter
    );

#if PYTHON_VERSION >= 0x3d0 && _PFORGE_FRAME_LOCALS_PROXY
    _initCompiledFrameLocalsProxyType();
#endif

    // These are to be used interchangeably. Make sure that's true.
    STATIC_ASSERT(offsetof(struct Pforge_FrameObject, m_frame) == 0, "m_frame must be at offset 0");
}

static struct Pforge_FrameObject *_MAKE_COMPILED_FRAME(PyCodeObject *code, PyObject *module, PyObject *f_locals,
                                                       Py_ssize_t locals_size, char const *type_description) {
    CHECK_CODE_OBJECT(code);
    CHECK_OBJECT(module);

#if _DEBUG_REFCOUNTS
    count_active_Pforge_Frame_Type += 1;
    count_allocated_Pforge_Frame_Type += 1;
#endif

    PyObject *globals = ((PyModuleObject *)module)->md_dict;
    CHECK_OBJECT(globals);

    assert(PyDict_Check(globals));

    struct Pforge_FrameObject *result;

#if PYTHON_VERSION >= 0x3b0
    Py_ssize_t total_locals_size = (code->co_nlocalsplus * sizeof(PyObject *)) + locals_size;
#else
    Py_ssize_t total_locals_size = locals_size;
#endif

    // Macro to assign result memory from GC or free list.
    allocateFromFreeList(free_list_frames, struct Pforge_FrameObject, Pforge_Frame_Type, total_locals_size);

    result->m_type_description = type_description;
    result->m_frame_cache = NULL;
#if PYTHON_VERSION >= 0x3b0
    result->m_locals_ptr = &result->m_interpreter_frame.localsplus[code->co_nlocalsplus];
#else
    result->m_locals_ptr = result->m_locals_storage;
#endif
    result->m_locals_writable = type_description != NULL;

#if PYTHON_VERSION >= 0x3b0
    // Zero the full locals region: CPython's co_nlocalsplus slots followed by
    // Pforge's heap buffer / detach target.
    memset(&result->m_interpreter_frame.localsplus[0], 0, total_locals_size);
#endif

    PyFrameObject *frame = &result->m_frame;
    // Globals and locals are stored differently before Python 3.11
#if PYTHON_VERSION < 0x3b0
    PyFrameObject *locals_owner = frame;
#else
    _PyInterpreterFrame *locals_owner = &result->m_interpreter_frame;
#endif

#if PYTHON_VERSION < 0x3d0
    locals_owner->f_code = code;
#elif PYTHON_VERSION < 0x3e0
    locals_owner->f_executable = (PyObject *)code;
#else
    // TODO: Why is our code object not immortal?
    locals_owner->f_executable = PyStackRef_FromPyObjectNew((PyObject *)code);
#endif

    frame->f_trace = Py_None;

#if PYTHON_VERSION < 0x370
    frame->f_exc_type = NULL;
    frame->f_exc_value = NULL;
    frame->f_exc_traceback = NULL;
#else
    frame->f_trace_lines = 0;
    frame->f_trace_opcodes = 0;
#endif

#if PYTHON_VERSION >= 0x3d0
    frame->f_extra_locals = NULL;
    frame->f_locals_cache = NULL;
#endif
#if PYTHON_VERSION >= 0x3e0
    frame->f_overwritten_fast_locals = NULL;
#endif

#if PYTHON_VERSION >= 0x3b0
    result->m_ob_size = Py_SIZE(result);
#endif
    frame->f_back = NULL;

    Py_INCREF(dict_builtin);
    locals_owner->f_builtins = (PyObject *)dict_builtin;

    Py_INCREF(globals);
    locals_owner->f_globals = globals;

    // Note: Reference taking happens in calling function.
    CHECK_OBJECT_X(f_locals);
    locals_owner->f_locals = f_locals;

#if PYTHON_VERSION < 0x300
    frame->f_tstate = PyThreadState_GET();
#endif

#if PYTHON_VERSION < 0x3b0
    frame->f_lasti = -1;
    frame->f_iblock = 0; // spell-checker: ignore iblock
#endif

    frame->f_lineno = code->co_firstlineno;

#if PYTHON_VERSION >= 0x300
    Pforge_SetFrameGenerator(result, NULL);

    Pforge_Frame_MarkAsNotExecuting(result);
#endif

#if PYTHON_VERSION >= 0x3b0
    result->m_interpreter_frame.frame_obj = &result->m_frame;

#if PYTHON_VERSION >= 0x3c0 && defined(Py_GIL_DISABLED)
    result->m_interpreter_frame.owner = FRAME_OWNED_BY_FRAME_OBJECT;
#else
    result->m_interpreter_frame.owner = FRAME_OWNED_BY_GENERATOR;
#endif

#if PYTHON_VERSION >= 0x3e0
    result->m_interpreter_frame.f_funcobj = PyStackRef_NULL; // spell-checker: ignore funcobj
    locals_owner->stackpointer = &locals_owner->localsplus[0];
#elif PYTHON_VERSION >= 0x3c0
    result->m_interpreter_frame.f_funcobj = NULL; // spell-checker: ignore funcobj
#else
    result->m_interpreter_frame.f_func = NULL;
#endif

#if PYTHON_VERSION >= 0x3d0
    result->m_interpreter_frame.instr_ptr = _PyCode_CODE(code) + code->_co_firsttraceable + 1;
#else
    result->m_interpreter_frame.prev_instr =
        _PyCode_CODE(code) + code->_co_firsttraceable; // spell-checker: ignore firsttraceable
#endif
    result->m_frame.f_frame = &result->m_interpreter_frame;

    assert(!_PyFrame_IsIncomplete(&result->m_interpreter_frame));
#endif

    Pforge_GC_Track(result);
    return result;
}

struct Pforge_FrameObject *MAKE_MODULE_FRAME(PyCodeObject *code, PyObject *module) {
    PyObject *f_locals = ((PyModuleObject *)module)->md_dict;
    Py_INCREF(f_locals);

    return _MAKE_COMPILED_FRAME(code, module, f_locals, 0, NULL);
}

struct Pforge_FrameObject *MAKE_FUNCTION_FRAME(PyThreadState *tstate, PyCodeObject *code, PyObject *module,
                                               Py_ssize_t locals_size, char const *type_description) {
    PyObject *f_locals;

    if (likely((code->co_flags & CO_OPTIMIZED) == CO_OPTIMIZED)) {
        f_locals = NULL;
    } else {
        PyObject *module_name = MODULE_NAME1(tstate, module);
        PyObject *kw_pairs[2] = {const_str_plain___module__, module_name};
        f_locals = MAKE_DICT(kw_pairs, 1);
        Py_DECREF(module_name);
    }

    return _MAKE_COMPILED_FRAME(code, module, f_locals, locals_size, type_description);
}

struct Pforge_FrameObject *MAKE_CLASS_FRAME(PyThreadState *tstate, PyCodeObject *code, PyObject *module,
                                            PyObject *f_locals, Py_ssize_t locals_size, char const *type_description) {
    // The frame template sets f_locals on usage itself.
    if (f_locals != NULL) {
        Py_INCREF(f_locals);
    }

    return _MAKE_COMPILED_FRAME(code, module, f_locals, locals_size, type_description);
}

// This is the backend of MAKE_CODE_OBJECT macro.
PyCodeObject *makeCodeObject(PyObject *filename, int line, int flags, PyObject *function_name,
#if PYTHON_VERSION >= 0x3b0
                             PyObject *function_qualname,
#endif
                             PyObject *arg_names, PyObject *free_vars, int arg_count
#if PYTHON_VERSION >= 0x300
                             ,
                             int kw_only_count
#endif
#if PYTHON_VERSION >= 0x380
                             ,
                             int pos_only_count
#endif
) {

    if (filename == Py_None) {
        filename = const_str_empty;
    }

    // TODO: A real filename is no longer needed here, code objects are
    // created incomplete and get theirs from "USE_CODE_OBJECT" on first use.
    CHECK_OBJECT(filename);
    assert(Pforge_StringOrUnicode_CheckExact(filename));

    CHECK_OBJECT(function_name);
    assert(Pforge_String_CheckExact(function_name));

#if PYTHON_VERSION >= 0x3b0
    if (function_qualname) {
        CHECK_OBJECT(function_qualname);
        assert(PyUnicode_CheckExact(function_qualname));
    }
#endif

#if PYTHON_VERSION >= 0x300
    assert(kw_only_count >= 0);
#endif
#if PYTHON_VERSION >= 0x380
    assert(pos_only_count >= 0);
#endif
    assert(flags >= 0);
    assert(arg_count >= 0);

// TODO: On macOS with zig, libpython asserts against values failing, but only if
// debug mode is not on, these check without using the disabled assert.
#if 0
    if (pos_only_count < 0) abort();
    if (kw_only_count < 0) abort();
    if (flags < 0) abort();
    if (arg_count < 0) abort();
#endif

#if PYTHON_VERSION < 0x300
    PyObject *filename_str = NULL;

    // TODO: Memory leak of filename, it might be intended transferred to the
    // code object by using code.
    if (PyUnicode_CheckExact(filename)) {
        filename_str = PyUnicode_AsUTF8String(filename);
        filename = filename_str;
    } else {
        filename_str = filename;
        Py_INCREF(filename);
    }
#endif

    if (arg_names == NULL || arg_names == Py_None) {
        arg_names = const_tuple_empty;
    }
    CHECK_OBJECT(arg_names);
    assert(PyTuple_Check(arg_names));

    if (free_vars == NULL || free_vars == Py_None) {
        free_vars = const_tuple_empty;
    }
    CHECK_OBJECT(free_vars);
    assert(PyTuple_Check(free_vars));

    // The PyCode_New has funny code that interns, mutating the tuple that owns
    // it. Really serious non-immutable shit. We have triggered that changes
    // behind our back in the past.
#ifndef __PFORGE_NO_ASSERT__
    // TODO: Reactivate once code object creation becomes un-streaming driven
    // and we can pass the extra args with no worries.

    // Py_hash_t hash = DEEP_HASH(arg_names);
#endif

    // spell-checker: ignore lnotab
#if PYTHON_VERSION < 0x300
    PyObject *code = const_str_empty;
    PyObject *lnotab = const_str_empty;
    PyObject *consts = const_tuple_empty;
    PyObject *names = const_tuple_empty;
    int stacksize = 0;
#elif PYTHON_VERSION < 0x3b0
    PyObject *code = const_bytes_empty;
    PyObject *lnotab = const_bytes_empty;
    PyObject *consts = const_tuple_empty;
    PyObject *names = const_tuple_empty;
    int stacksize = 0;
#else
    // Our code object needs to be recognizable, and Python doesn't store the
    // length anymore, so we need a non-empty one.
    static PyObject *empty_code = NULL;
    static PyObject *lnotab = NULL;
    static PyObject *consts = NULL;
    static PyObject *names = NULL;
    // TODO: Seems removable.
    static PyObject *exception_table = NULL;
    static int stacksize = 0;

    if (empty_code == NULL) {
        // Only needed once here.
        PyThreadState *tstate = PyThreadState_GET();

        PyObject *empty_code_module_object = Py_CompileString(
            "def empty(): raise RuntimeError('Compiled function bytecode used')", "<exec>", Py_file_input);
        PyObject *module = PyImport_ExecCodeModule("pforge_empty_function", empty_code_module_object);
        CHECK_OBJECT(module);

        PyObject *empty_function = PyObject_GetAttrString(module, "empty");
        CHECK_OBJECT(empty_function);
        PyObject *empty_code_object = PyObject_GetAttrString(empty_function, "__code__");
        CHECK_OBJECT(empty_code_object);

        PFORGE_MAY_BE_UNUSED bool bool_res = Pforge_DelModuleString(tstate, "pforge_empty_function");
        assert(bool_res != false);

        empty_code = PyObject_GetAttrString(empty_code_object, "co_code");
        CHECK_OBJECT(empty_code);
#if PYTHON_VERSION >= 0x3c0
        lnotab = ((PyCodeObject *)empty_code_object)->co_linetable; // spell-checker: ignore linetable
#else
        lnotab = PyObject_GetAttrString(empty_code_object, "co_lnotab");
        CHECK_OBJECT(lnotab);
#endif
        consts = PyObject_GetAttrString(empty_code_object, "co_consts");
        CHECK_OBJECT(consts);
        names = PyObject_GetAttrString(empty_code_object, "co_names");
        CHECK_OBJECT(names);
        exception_table = PyObject_GetAttrString(empty_code_object, "co_exceptiontable");
        CHECK_OBJECT(exception_table);

        stacksize = (int)PyLong_AsLong(PyObject_GetAttrString(empty_code_object, "co_stacksize"));
    }

    PyObject *code = empty_code;
    CHECK_OBJECT(empty_code);
    assert(PyBytes_Check(code));
    CHECK_OBJECT(lnotab);
    assert(PyBytes_Check(lnotab));
    CHECK_OBJECT(consts);
    assert(PyTuple_Check(consts));
    CHECK_OBJECT(names);
    assert(PyTuple_Check(names));
    CHECK_OBJECT(exception_table);
    assert(PyBytes_Check(exception_table));
    assert(stacksize >= 0);
#endif

    // Before Python 3.11, having "co_nlocals" match the variable names makes
    // CPython's "PyFrame_FastToLocals" walk the frame's "f_localsplus", which
    // does not have the object pointer layout it expects, so keep it zero, and
    // use the variable names tuple to learn the count instead.
#if PYTHON_VERSION >= 0x3b0
    int nlocals = (int)PyTuple_GET_SIZE(arg_names);
#else
    int nlocals = 0;
#endif

    // Not using PyCode_NewEmpty, it doesn't given us much beyond this
    // and is not available for Python2.

#if PYTHON_VERSION >= 0x380
    PyCodeObject *result = PyCode_NewWithPosOnlyArgs(arg_count, // arg_count
#else
    PyCodeObject *result = PyCode_New(arg_count, // arg_count
#endif

#if PYTHON_VERSION >= 0x300
#if PYTHON_VERSION >= 0x380
                                                     pos_only_count, // pos-only count
#endif
                                                     kw_only_count, // kw-only count
#endif
                                                     nlocals,           // nlocals
                                                     stacksize,         // stacksize
                                                     flags,             // flags
                                                     code,              // code (bytecode)
                                                     consts,            // consts (we are not going to be compatible)
                                                     names,             // names (we are not going to be compatible)
                                                     arg_names,         // var_names (we are not going to be compatible)
                                                     free_vars,         // free_vars
                                                     const_tuple_empty, // cell_vars (we are not going to be compatible)
                                                     filename,          // filename
                                                     function_name,     // name
#if PYTHON_VERSION >= 0x3b0
                                                     function_qualname, // qualname
#endif
                                                     line,  // first_lineno (offset of the code object)
                                                     lnotab // lnotab (table to translate code object)
#if PYTHON_VERSION >= 0x3b0
                                                     ,
                                                     exception_table // exception_table
#endif
    );

    // assert(DEEP_HASH(tstate, arg_names) == hash);

#if PYTHON_VERSION < 0x300
    Py_DECREF(filename_str);
#endif

    if (result == NULL) {
        PyErr_PrintEx(0);
        PFORGE_CANNOT_GET_HERE("Failed to create code object");
    }

    return result;
}

PyCodeObject *USE_CODE_OBJECT(PyThreadState *tstate, PyObject *code_object, PyObject *module_filename_obj) {
    assert(PyCode_Check(code_object));
    CHECK_OBJECT(module_filename_obj);

    PyCodeObject *co = (PyCodeObject *)code_object;
    PyObject *old = co->co_filename;

    if (old == const_str_empty) {
        // Set the filename, ignore the loss of a reference to empty string,
        // that's our singleton and immortal at least practically.
        co->co_filename = Py_NewRef(module_filename_obj);

#if PYTHON_VERSION >= 0x3b0
        // Also, make sure the qualname is completed from the partial
        // name.
        if (co->co_qualname != co->co_name) {
            PyObject *w = UNICODE_CONCAT(tstate, co->co_qualname, const_str_dot);
            co->co_qualname = UNICODE_CONCAT(tstate, w, co->co_name);
            Py_DECREF(w);
        }
#endif
    }

    return co;
}

void Pforge_Frame_AttachLocals(struct Pforge_FrameObject *frame_object) {
    assert(Pforge_Frame_CheckExact((PyObject *)frame_object));
    CHECK_OBJECT((PyObject *)frame_object);
    assert(_PyObject_GC_IS_TRACKED(frame_object));
    assertFrameObject(frame_object);

#if _DEBUG_FRAME
    PRINT_FORMAT("Attaching to frame 0x%lx %s\n", frame_object,
                 Pforge_String_AsString(PyObject_Repr((PyObject *)Pforge_Frame_GetCodeObject(&frame_object->m_frame))));
#endif

    char const *type_description = frame_object->m_type_description;
    if (type_description == NULL) {
        return;
    }

    unsigned char const *w = (unsigned char const *)type_description;
    void *src = frame_object->m_locals_ptr;
    assert(src != NULL);
    char *dst = PFORGE_FRAME_LOCALS_STORAGE(frame_object);
    size_t offset = 0;

    Py_ssize_t locals_count = Pforge_GetCodeLocalsCount(Pforge_GetFrameCodeObject(frame_object));
    assert(w[locals_count] == 0);

    for (Py_ssize_t i = 0; i < locals_count; i++) {
        int type = w[i];
        offset = Pforge_FrameLocals_AlignUp(offset, Pforge_FrameLocals_Align(type));
        void *src_slot = (char *)src + offset;
        char *dst_slot = dst + offset;

        switch (type) {
        case PFORGE_TYPE_DESCRIPTION_OBJECT:
        case PFORGE_TYPE_DESCRIPTION_OBJECT_PTR: {
            PyObject *value = *(PyObject **)src_slot;
            memcpy(dst_slot, &value, sizeof(PyObject *));
            Py_XINCREF(value);
            break;
        }
        case PFORGE_TYPE_DESCRIPTION_CELL: {
            struct Pforge_CellObject *value = *(struct Pforge_CellObject **)src_slot;
            memcpy(dst_slot, &value, sizeof(struct Pforge_CellObject *));
            if (value != NULL) {
                Py_INCREF(value);
            }
            break;
        }
        case PFORGE_TYPE_DESCRIPTION_NULL:
            break;
        case PFORGE_TYPE_DESCRIPTION_BOOL: {
            pforge_bool value = *(pforge_bool *)src_slot;
            memcpy(dst_slot, &value, sizeof(pforge_bool));
            break;
        }
        case PFORGE_TYPE_DESCRIPTION_NILONG: {
            pforge_ilong value = *(pforge_ilong *)src_slot;
            memcpy(dst_slot, &value, sizeof(pforge_ilong));
            INCREF_NILONG_VALUE((pforge_ilong *)dst_slot);
            break;
        }
        default:
            PFORGE_CANNOT_GET_HERE("invalid type description");
        }

        offset += Pforge_FrameLocals_Size(type);
    }

    frame_object->m_locals_ptr = PFORGE_FRAME_LOCALS_STORAGE(frame_object);

    assert(offset <= Pforge_Frame_GetStorageSize(frame_object));
}

// Mark the locals storage of a frame as attached, after the caller copied the
// locals struct into it by value. The values are borrowed, so take references
// and record the type description for reading and releasing.
void Pforge_Frame_AttachLocalsCopied(struct Pforge_FrameObject *frame_object, char const *type_description) {
    assert(Pforge_Frame_CheckExact((PyObject *)frame_object));
    CHECK_OBJECT((PyObject *)frame_object);
    assert(_PyObject_GC_IS_TRACKED(frame_object));
    assertFrameObject(frame_object);

    assert(frame_object->m_type_description == NULL);
    assert(type_description != NULL);
    assert(type_description[0] != 0);

    frame_object->m_type_description = type_description;

    unsigned char const *w = (unsigned char const *)type_description;
    char *storage = (char *)PFORGE_FRAME_LOCALS_STORAGE(frame_object);
    size_t offset = 0;

    Py_ssize_t locals_count = Pforge_GetCodeLocalsCount(Pforge_GetFrameCodeObject(frame_object));
    assert(w[locals_count] == 0);

    for (Py_ssize_t i = 0; i < locals_count; i++) {
        int type = w[i];
        offset = Pforge_FrameLocals_AlignUp(offset, Pforge_FrameLocals_Align(type));
        char *t = storage + offset;

        switch (type) {
        case PFORGE_TYPE_DESCRIPTION_OBJECT:
        case PFORGE_TYPE_DESCRIPTION_OBJECT_PTR: {
            Py_XINCREF(*(PyObject **)t);

            break;
        }
        case PFORGE_TYPE_DESCRIPTION_CELL: {
            Py_XINCREF(*(struct Pforge_CellObject **)t);

            break;
        }
        case PFORGE_TYPE_DESCRIPTION_BOOL: {
            break;
        }
        case PFORGE_TYPE_DESCRIPTION_NILONG: {
            INCREF_NILONG_VALUE((pforge_ilong *)t);

            break;
        }
        case PFORGE_TYPE_DESCRIPTION_NULL:
            break;
        default:
            PFORGE_CANNOT_GET_HERE("invalid type description");
        }

        offset += Pforge_FrameLocals_Size(type);
    }

    frame_object->m_locals_ptr = storage;

    assert(offset <= Pforge_Frame_GetStorageSize(frame_object));
}

// Chain a newly set exception, either to the keeper exception state, or to the
// current exception, as the generated code used to do separately.
static void _Pforge_Frame_ChainException(PyThreadState *tstate,
                                         struct Pforge_ExceptionPreservationItem *exception_state,
                                         struct Pforge_ExceptionPreservationItem *keeper_exception_state) {
#if PYTHON_VERSION >= 0x300
    if (keeper_exception_state != NULL) {
        ADD_EXCEPTION_CONTEXT(tstate, keeper_exception_state);
    } else {
#if PYTHON_VERSION < 0x3c0
        NORMALIZE_EXCEPTION_STATE(tstate, exception_state);
#endif
        CHAIN_EXCEPTION(tstate, exception_state->exception_value);
    }
#else
    (void)keeper_exception_state;
#endif
}

// Format an UnboundLocalError for the given variable name, chaining the
// exception as needed. This is used for frame-less outline code, where no
// frame variable index is available.
void Pforge_FormatUnboundLocalError(PyThreadState *tstate, struct Pforge_ExceptionPreservationItem *exception_state,
                                    struct Pforge_ExceptionPreservationItem *keeper_exception_state,
                                    PyObject *variable_name) {
    CHECK_OBJECT(variable_name);

#if PYTHON_VERSION < 0x300
    PyObject *exception_value = Pforge_String_FromFormat("local variable '%s' referenced before assignment",
                                                         Pforge_String_AsString_Unchecked(variable_name));
#elif PYTHON_VERSION < 0x3b0
    PyObject *exception_value =
        Pforge_String_FromFormat("local variable '%U' referenced before assignment", variable_name);
#else
    PyObject *exception_value = Pforge_String_FromFormat(
        "cannot access local variable '%U' where it is not associated with a value", variable_name);
#endif

    CHECK_OBJECT(exception_value);
    SET_EXCEPTION_PRESERVATION_STATE_FROM_TYPE0_VALUE1(tstate, exception_state, PyExc_UnboundLocalError,
                                                       exception_value);

    _Pforge_Frame_ChainException(tstate, exception_state, keeper_exception_state);
}

// Format an UnboundLocalError for the local variable at the given index of the
// frame code object's variable names. The name is taken from the code object,
// so no string constant is needed at the call site.
void Pforge_Frame_FormatUnboundLocalError(PyThreadState *tstate,
                                          struct Pforge_ExceptionPreservationItem *exception_state,
                                          struct Pforge_ExceptionPreservationItem *keeper_exception_state,
                                          struct Pforge_FrameObject *frame_object, int variable_index) {
    CHECK_OBJECT((PyObject *)frame_object);
    assertFrameObject(frame_object);

    PyCodeObject *code_object = Pforge_GetFrameCodeObject(frame_object);

    assert(variable_index >= 0);
    assert(variable_index < Pforge_GetCodeLocalsCount(code_object));

    PyObject *variable_name = Pforge_GetCodeVarNames(code_object)[variable_index];

    Pforge_FormatUnboundLocalError(tstate, exception_state, keeper_exception_state, variable_name);
}

// Format an NameError for the free variable at the given index of the frame
// code object's free variables. The name is taken from the code object, so no
// string constant is needed at the call site.
void Pforge_Frame_FormatUnboundClosureError(PyThreadState *tstate,
                                            struct Pforge_ExceptionPreservationItem *exception_state,
                                            struct Pforge_ExceptionPreservationItem *keeper_exception_state,
                                            struct Pforge_FrameObject *frame_object, int variable_index) {
    CHECK_OBJECT((PyObject *)frame_object);
    assertFrameObject(frame_object);

    PyCodeObject *code_object = Pforge_GetFrameCodeObject(frame_object);

    assert(variable_index >= 0);
#if PYTHON_VERSION >= 0x3b0
    assert(variable_index < code_object->co_nfreevars);

    // Cells are empty, so free variables are after the locals.
    PyObject *variable_name = Pforge_GetCodeVarNames(code_object)[code_object->co_nlocals + variable_index];
#else
    assert(variable_index < (int)PyTuple_GET_SIZE(code_object->co_freevars));

    PyObject *variable_name = PyTuple_GET_ITEM(code_object->co_freevars, variable_index);
#endif

#if PYTHON_VERSION < 0x3b0
    char const *message = "free variable '%s' referenced before assignment in enclosing scope";
#else
    char const *message = "cannot access free variable '%s' where it is not associated with a value in enclosing scope";
#endif

    PyObject *exception_value = Pforge_String_FromFormat(message, Pforge_String_AsString_Unchecked(variable_name));
    CHECK_OBJECT(exception_value);
    SET_EXCEPTION_PRESERVATION_STATE_FROM_TYPE0_VALUE1(tstate, exception_state, PyExc_NameError, exception_value);

    _Pforge_Frame_ChainException(tstate, exception_state, keeper_exception_state);
}

static void _Pforge_ReleaseLocalsWithDescription(char const *type_description, void *storage, int locals_count) {
    assert(type_description != NULL);
    assert(type_description[0] != 0);
    assert(locals_count != 0);

    unsigned char const *w = (unsigned char const *)type_description;
    size_t offset = 0;

    assert(w[locals_count] == 0);

    for (int i = 0; i < locals_count; i++) {
        int type = w[i];
        offset = Pforge_FrameLocals_AlignUp(offset, Pforge_FrameLocals_Align(type));
        void *slot = (char *)storage + offset;
        switch (type) {
        case PFORGE_TYPE_DESCRIPTION_OBJECT:
        case PFORGE_TYPE_DESCRIPTION_OBJECT_PTR: {
            PyObject *value = *(PyObject **)slot;
            Py_XDECREF(value);
            break;
        }
        case PFORGE_TYPE_DESCRIPTION_CELL: {
            struct Pforge_CellObject *value = *(struct Pforge_CellObject **)slot;
            if (value != NULL) {
                Py_DECREF(value);
            }
            break;
        }
        case PFORGE_TYPE_DESCRIPTION_NILONG:
            RELEASE_NILONG_VALUE((pforge_ilong *)slot);
            break;
        case PFORGE_TYPE_DESCRIPTION_BOOL:
        case PFORGE_TYPE_DESCRIPTION_NULL:
            break;
        default:
            PFORGE_CANNOT_GET_HERE("invalid type in type description");
        }
        offset += Pforge_FrameLocals_Size(type);
    }
}

// Make a dump of the active frame stack. For debugging purposes only.
#if _DEBUG_FRAME
void dumpFrameStack(void) {
    PyThreadState *tstate = PyThreadState_GET();

    struct Pforge_ExceptionPreservationItem saved_exception_state;
    FETCH_ERROR_OCCURRED_STATE(tstate, &saved_exception_state);

    int total = 0;

#if PYTHON_VERSION < 0x3b0
    PyFrameObject *current = PyThreadState_GET()->frame;
    while (current != NULL) {
        total++;
        current = current->f_back;
    }

    current = tstate->frame;
#else
    _PyCFrame *current = tstate->cframe;
    while (current != NULL) {
        total++;
        current = current->previous;
    }

    current = tstate->cframe;
#endif

    PRINT_STRING(">--------->\n");

    while (current) {
#if PYTHON_VERSION < 0x3b0
        PyObject *current_repr = PyObject_Str((PyObject *)current);
        PyObject *code_repr = PyObject_Str((PyObject *)current->f_code);
#else
        PyObject *current_repr = NULL;
        if (current->current_frame->frame_obj != NULL) {
            current_repr = PyObject_Str((PyObject *)current->current_frame->frame_obj);
        } else {
            current_repr = const_str_empty;
            Py_INCREF(const_str_empty);
        }
        PyObject *code_repr = PyObject_Str((PyObject *)Pforge_InterpreterFrame_GetCodeObject(current->current_frame));
#endif

        PRINT_FORMAT("Frame stack %d: %s %d %s\n", total--, Pforge_String_AsString(current_repr), Py_REFCNT(current),
                     Pforge_String_AsString(code_repr));

        Py_DECREF(current_repr);
        Py_DECREF(code_repr);

#if PYTHON_VERSION < 0x3b0
        current = current->f_back;
#else
        current = current->previous;
#endif
    }

    PRINT_STRING(">---------<\n");

    RESTORE_ERROR_OCCURRED_STATE(tstate, &saved_exception_state);
}

static void PRINT_UNCOMPILED_FRAME(char const *prefix, PyFrameObject *frame) {
    PRINT_STRING(prefix);
    PRINT_STRING(" ");

    if (frame) {
        PyObject *frame_str = PyObject_Str((PyObject *)frame);
        PRINT_ITEM(frame_str);
        Py_DECREF(frame_str);

        PyObject *code_object_str = PyObject_Repr((PyObject *)Pforge_Frame_GetCodeObject(frame));
        PRINT_ITEM(code_object_str);
        Py_DECREF(code_object_str);

        PRINT_REFCOUNT((PyObject *)frame);
    } else {
        PRINT_STRING("<NULL> no frame");
    }

    PRINT_NEW_LINE();
}

void PRINT_COMPILED_FRAME(char const *prefix, struct Pforge_FrameObject *frame) {
    if (frame == NULL) {
        PRINT_STRING(prefix);
        PRINT_STRING(" <NULL> no frame");
        PRINT_NEW_LINE();
        return;
    }

    if (!_PyObject_GC_IS_TRACKED(frame)) {
        PRINT_STRING(prefix);
        PRINT_STRING(" <untracked compiled_frame>");
        PRINT_NEW_LINE();
        return;
    }

    return PRINT_UNCOMPILED_FRAME(prefix, &frame->m_frame);
}

void PRINT_INTERPRETER_FRAME(char const *prefix, Pforge_ThreadStateFrameType *frame) {
#if PYTHON_VERSION < 0x3b0
    PRINT_UNCOMPILED_FRAME(prefix, frame);
#else
    PRINT_STRING(prefix);
    PRINT_STRING(" ");

    if (frame) {
        PRINT_FORMAT("0x%lx ", frame);

        PyObject *code_object_str = PyObject_Repr((PyObject *)Pforge_InterpreterFrame_GetCodeObject(frame));
        PRINT_ITEM(code_object_str);
        Py_DECREF(code_object_str);
    } else {
        PRINT_STRING("<NULL> no frame");
    }

    PRINT_NEW_LINE();
#endif
}

void PRINT_TOP_FRAME(char const *prefix) {
    PyThreadState *tstate = PyThreadState_GET();

#if PYTHON_VERSION < 0x3b0
    PRINT_UNCOMPILED_FRAME(prefix, tstate->frame);
#else
    PRINT_INTERPRETER_FRAME(prefix, CURRENT_TSTATE_INTERPRETER_FRAME(tstate));
#endif
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
