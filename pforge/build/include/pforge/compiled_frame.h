//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_COMPILED_FRAME_H__
#define __PFORGE_COMPILED_FRAME_H__

/* This file is included from another C file, help IDEs to still parse it on its own. */
#ifdef __IDE_ONLY__
#include "pforge/allocator.h"
#include "pforge/cpython_api_compat.h"

#include "internal/pycore_frame.h"
#include "internal/pycore_object.h"

#if PYTHON_VERSION >= 0x3e0
#include "internal/pycore_interpframe.h"
#endif

#include "pforge/checkers.h"
#include "pforge/defines.h"
#include "pforge/exceptions.h"
#include "pforge/helper/boolean.h"
#include "pforge/helper/ints.h"
#endif

#include "pforge/constants_blob_spec.h"

// Removed flag in 3.11, but we keep code compatible for now. We do not use old
// value, but 0 because it might get reused. TODO: Probably better to #ifdef
// usages of it away.
#if PYTHON_VERSION >= 0x3b0
#define CO_NOFREE 0
#endif

// With Python 3.11 or higher, a lightweight object needs to be put into thread
// state, rather than the full blown frame, that is more similar to current
// Pforge frames.
#if PYTHON_VERSION < 0x3b0
typedef PyFrameObject Pforge_ThreadStateFrameType;
#else
typedef _PyInterpreterFrame Pforge_ThreadStateFrameType;
#endif

// Print a description of given frame objects in frame debug mode
#if _DEBUG_FRAME
extern void PRINT_TOP_FRAME(char const *prefix);
extern void PRINT_PYTHON_FRAME(char const *prefix, PyFrameObject *frame);
extern void PRINT_COMPILED_FRAME(char const *prefix, struct Pforge_FrameObject *frame);
extern void PRINT_INTERPRETER_FRAME(char const *prefix, Pforge_ThreadStateFrameType *frame);
#else
#define PRINT_TOP_FRAME(prefix)
#define PRINT_PYTHON_FRAME(prefix, frame)
#define PRINT_COMPILED_FRAME(prefix, frame)
#define PRINT_INTERPRETER_FRAME(prefix, frame)
#endif

// Create a frame object for the given code object, frame or module.
extern struct Pforge_FrameObject *MAKE_MODULE_FRAME(PyCodeObject *code, PyObject *module);
extern struct Pforge_FrameObject *MAKE_FUNCTION_FRAME(PyThreadState *tstate, PyCodeObject *code, PyObject *module,
                                                      Py_ssize_t locals_size, char const *type_description);
extern struct Pforge_FrameObject *MAKE_CLASS_FRAME(PyThreadState *tstate, PyCodeObject *code, PyObject *module,
                                                   PyObject *f_locals, Py_ssize_t locals_size,
                                                   char const *type_description);
extern void Pforge_Frame_AssignLocals(struct Pforge_FrameObject *frame_object, PyObject *locals_value);
extern void Pforge_Frame_ClearLocals(struct Pforge_FrameObject *frame_object);
extern void Pforge_Frame_AttachLocalsCopied(struct Pforge_FrameObject *frame_object, char const *type_description);

// Format an UnboundLocalError for the local variable at the given index of the
// frame code object's variable names, and chain the exception as needed.
extern void Pforge_Frame_FormatUnboundLocalError(PyThreadState *tstate,
                                                 struct Pforge_ExceptionPreservationItem *exception_state,
                                                 struct Pforge_ExceptionPreservationItem *keeper_exception_state,
                                                 struct Pforge_FrameObject *frame_object, int variable_index);

// Format an UnboundLocalError for the given variable name, used by frame-less
// outline code, and chain the exception as needed.
extern void Pforge_FormatUnboundLocalError(PyThreadState *tstate,
                                           struct Pforge_ExceptionPreservationItem *exception_state,
                                           struct Pforge_ExceptionPreservationItem *keeper_exception_state,
                                           PyObject *variable_name);

// Format a NameError for the free variable at the given index of the frame code
// object's free variables, and chain the exception as needed.
extern void Pforge_Frame_FormatUnboundClosureError(PyThreadState *tstate,
                                                   struct Pforge_ExceptionPreservationItem *exception_state,
                                                   struct Pforge_ExceptionPreservationItem *keeper_exception_state,
                                                   struct Pforge_FrameObject *frame_object, int variable_index);

// Create a code object for the given filename and function name

#if PYTHON_VERSION < 0x300
#define MAKE_CODE_OBJECT(filename, line, flags, function_name, function_qualname, arg_names, free_vars, arg_count,     \
                         kw_only_count, pos_only_count)                                                                \
    makeCodeObject(filename, line, flags, function_name, arg_names, free_vars, arg_count)
extern PyCodeObject *makeCodeObject(PyObject *filename, int line, int flags, PyObject *function_name,
                                    PyObject *arg_names, PyObject *free_vars, int arg_count);
#elif PYTHON_VERSION < 0x380
#define MAKE_CODE_OBJECT(filename, line, flags, function_name, function_qualname, arg_names, free_vars, arg_count,     \
                         kw_only_count, pos_only_count)                                                                \
    makeCodeObject(filename, line, flags, function_name, arg_names, free_vars, arg_count, kw_only_count)
extern PyCodeObject *makeCodeObject(PyObject *filename, int line, int flags, PyObject *function_name,
                                    PyObject *arg_names, PyObject *free_vars, int arg_count, int kw_only_count);
#elif PYTHON_VERSION < 0x3b0
#define MAKE_CODE_OBJECT(filename, line, flags, function_name, function_qualname, arg_names, free_vars, arg_count,     \
                         kw_only_count, pos_only_count)                                                                \
    makeCodeObject(filename, line, flags, function_name, arg_names, free_vars, arg_count, kw_only_count, pos_only_count)
extern PyCodeObject *makeCodeObject(PyObject *filename, int line, int flags, PyObject *function_name,
                                    PyObject *arg_names, PyObject *free_vars, int arg_count, int kw_only_count,
                                    int pos_only_count);
#else
#define MAKE_CODE_OBJECT(filename, line, flags, function_name, function_qualname, arg_names, free_vars, arg_count,     \
                         kw_only_count, pos_only_count)                                                                \
    makeCodeObject(filename, line, flags, function_name, function_qualname, arg_names, free_vars, arg_count,           \
                   kw_only_count, pos_only_count)
extern PyCodeObject *makeCodeObject(PyObject *filename, int line, int flags, PyObject *function_name,
                                    PyObject *function_qualname, PyObject *arg_names, PyObject *free_vars,
                                    int arg_count, int kw_only_count, int pos_only_count);
#endif

PFORGE_MAY_BE_UNUSED static inline bool isFakeCodeObject(PyCodeObject *code) {
#if PYTHON_VERSION < 0x300
    return code->co_code == const_str_empty;
#elif PYTHON_VERSION < 0x3b0
    return code->co_code == const_bytes_empty;
#else
    // Starting for Python3.11, we just proper bytecode that raises
    // "RuntimeError" itself, so this function is only used to
    // optimize checks away.
    return false;
#endif
}

// Prepare code object for use, patching up its filename.
extern PyCodeObject *USE_CODE_OBJECT(PyThreadState *tstate, PyObject *code_object, PyObject *module_filename_obj);

extern PyTypeObject Pforge_Frame_Type;

static inline bool Pforge_Frame_CheckExact(PyObject *obj) {
    CHECK_OBJECT(obj);
    return Py_TYPE(obj) == &Pforge_Frame_Type;
}

static inline bool Pforge_Frame_Check(PyObject *obj) {
    assert(obj);

    if (!_PyObject_GC_IS_TRACKED(obj)) {
        return false;
    }

    CHECK_OBJECT(obj);

    if (Pforge_Frame_CheckExact(obj)) {
        return true;
    }

    return strcmp(Py_TYPE(obj)->tp_name, "compiled_frame") == 0;
}

struct Pforge_FrameObject;

struct Pforge_FrameCacheHandle {
    struct Pforge_FrameObject *frame;
    uint8_t in_use;
};

struct Pforge_FrameObject {
    PyFrameObject m_frame;

#if PYTHON_VERSION >= 0x3b0
    PyObject *m_generator;
    PyFrameState m_frame_state;

    // In Python 3.11, the frame object is no longer variable size, and as such
    // we inherit the wrong kind of header, not PyVarObject, leading to f_back
    // the PyFrameObject and and ob_size aliasing, which is not good, but we
    // want to expose the same binary interface, while still being variable size,
    // so what we do is to preserve the size in this field instead.
    Py_ssize_t m_ob_size;

#endif

    // Whether the locals storage is writable (the stack struct), or only a
    // snapshot copy made when an exception was raised. Kept before the
    // pointers, so that the pre-3.11 "m_locals_storage" below stays aligned.
    unsigned char m_locals_writable;

    // Our own extra stuff, attached variables.
    char const *m_type_description;
    // Pointer to the current locals storage: stack context struct while alive,
    // or the frame's heap buffer after copy-with-INCREF detach.
    void *m_locals_ptr;
    // Static cache slot owning this frame while claimed or idle, NULL when the
    // frame is uncached.
    struct Pforge_FrameCacheHandle *m_frame_cache;
#if PYTHON_VERSION >= 0x3b0
    _PyInterpreterFrame m_interpreter_frame;
#else
    // Storage for the locals struct; must be pointer aligned for the walkers.
    char m_locals_storage[1];
#endif
};

// TODO: We ought to start using the localsplus slots directly for storing Pforge's
// compiled local variables in the future, rather than having a separate shift-based
// storage offset, which will make them automatically visible to CPython stack-walking.
#if PYTHON_VERSION >= 0x3b0
#define PFORGE_FRAME_LOCALS_STORAGE(frame)                                                                             \
    ((char *)&(frame)->m_interpreter_frame.localsplus[Pforge_GetFrameCodeObject(frame)->co_nlocalsplus])
#else
#define PFORGE_FRAME_LOCALS_STORAGE(frame) ((frame)->m_locals_storage)
#endif

inline static void CHECK_CODE_OBJECT(PyCodeObject *code_object) { CHECK_OBJECT(code_object); }

PFORGE_MAY_BE_UNUSED static inline bool isFrameUnusable(struct Pforge_FrameObject *frame_object) {
    CHECK_OBJECT_X(frame_object);

    bool result =
        // Never used.
        frame_object == NULL ||
        // Still in use
        Py_REFCNT(frame_object) > 1 ||
#if PYTHON_VERSION < 0x300
        // Last used by another thread (TODO: Could just set it when reusing)
        frame_object->m_frame.f_tstate != PyThreadState_GET() ||
#endif
        // Not currently linked.
        frame_object->m_frame.f_back != NULL;

#if _DEBUG_REFRAME
    if (result && frame_object != NULL) {
        PRINT_COMPILED_FRAME("NOT REUSING FRAME:", frame_object);
    }
#endif

    return result;
}

#if _DEBUG_REFCOUNTS
extern int count_active_frame_cache_instances;
extern int count_allocated_frame_cache_instances;
extern int count_released_frame_cache_instances;
extern int count_hit_frame_cache_instances;
#endif

#if _DEBUG_FRAME
extern void dumpFrameStack(void);
#endif

#if PYTHON_VERSION >= 0x3b0
inline static PyCodeObject *Pforge_InterpreterFrame_GetCodeObject(_PyInterpreterFrame *frame) {
#if PYTHON_VERSION < 0x3d0
    PyCodeObject *result = frame->f_code;
#elif PYTHON_VERSION < 0x3e0
    PyCodeObject *result = (PyCodeObject *)frame->f_executable;
#else
    assert(!PyStackRef_IsNull(frame->f_executable));
    PyCodeObject *result = (PyCodeObject *)PyStackRef_AsPyObjectBorrow(frame->f_executable);
    CHECK_CODE_OBJECT(result);
#endif
    assert(result == (PyCodeObject *)Py_None || PyCode_Check(result));
    return result;
}
#endif

inline static PyCodeObject *Pforge_Frame_GetCodeObject(PyFrameObject *frame) {
#if PYTHON_VERSION >= 0x3b0
    assert(frame->f_frame);
    return Pforge_InterpreterFrame_GetCodeObject(frame->f_frame);
#else
    return frame->f_code;
#endif
}

inline static void assertPythonFrameObject(PyFrameObject *frame_object) {

    // TODO: Need to do this manually, as this is making frame caching code
    // vulnerable to mistakes, but so far the compiled frame type is private
    // assert(PyObject_IsInstance((PyObject *)frame_object, (PyObject *)&PyFrame_Type));
    CHECK_OBJECT(frame_object);

    CHECK_CODE_OBJECT(Pforge_Frame_GetCodeObject(frame_object));
}

inline static void assertFrameObject(struct Pforge_FrameObject *frame_object) {
    CHECK_OBJECT(frame_object);

    // TODO: Need to do this manually, as this is making frame caching code
    // vulnerable to mistakes, but so far the compiled frame type is private
    // assert(PyObject_IsInstance((PyObject *)frame_object, (PyObject *)&PyFrame_Type));

    assertPythonFrameObject(&frame_object->m_frame);
}

inline static void assertThreadFrameObject(Pforge_ThreadStateFrameType *frame) {
#if PYTHON_VERSION < 0x3b0
    assertPythonFrameObject(frame);
#else
    // For uncompiled frames of Python 3.11 these often do not exist. TODO: Figure
    // out what to check or how to know it's a compiled one.
    if (frame->frame_obj) {
        assertPythonFrameObject(frame->frame_obj);
    }
#endif
}

// Mark frame as currently executed. Starting with Python 3 that means it
// can or cannot be cleared, or should lead to a generator close. For Python2
// this is a no-op. Using a define to spare the compile from inlining an empty
// function.
#if PYTHON_VERSION >= 0x300

#if PYTHON_VERSION < 0x3b0

static inline void Pforge_PythonFrame_MarkAsExecuting(PyFrameObject *frame) {
#if PYTHON_VERSION >= 0x3a0
    frame->f_state = FRAME_EXECUTING;
#else
    frame->f_executing = 1;
#endif
}

#endif

static inline void Pforge_Frame_MarkAsExecuting(struct Pforge_FrameObject *frame) {
    CHECK_OBJECT(frame);
#if PYTHON_VERSION >= 0x3b0
    frame->m_frame_state = FRAME_EXECUTING;
#elif PYTHON_VERSION >= 0x3a0
    frame->m_frame.f_state = FRAME_EXECUTING;
#else
    frame->m_frame.f_executing = 1;
#endif
}
#else
#define Pforge_Frame_MarkAsExecuting(frame) ;
#endif

#if PYTHON_VERSION >= 0x300
static inline void Pforge_Frame_MarkAsNotExecuting(struct Pforge_FrameObject *frame) {
    CHECK_OBJECT(frame);
#if PYTHON_VERSION >= 0x3b0
    frame->m_frame_state = FRAME_SUSPENDED;
#elif PYTHON_VERSION >= 0x3a0
    frame->m_frame.f_state = FRAME_SUSPENDED;
#else
    frame->m_frame.f_executing = 0;
#endif
}
#else
#define Pforge_Frame_MarkAsNotExecuting(frame) ;
#define Pforge_PythonFrame_MarkAsExecuting(frame) ;
#endif

#if PYTHON_VERSION >= 0x300
static inline bool Pforge_Frame_IsExecuting(struct Pforge_FrameObject *frame) {
    CHECK_OBJECT(frame);
#if PYTHON_VERSION >= 0x3b0
    return frame->m_frame_state == FRAME_EXECUTING;
#elif PYTHON_VERSION >= 0x3a0
    return frame->m_frame.f_state == FRAME_EXECUTING;
#else
    return frame->m_frame.f_executing == 1;
#endif
}
#endif

#if PYTHON_VERSION >= 0x3d0
static inline bool Pforge_Frame_IsSuspended(struct Pforge_FrameObject *frame) {
    CHECK_OBJECT(frame);
    return frame->m_frame_state == FRAME_SUSPENDED;
}
#endif

#if PYTHON_VERSION >= 0x3b0

#if PYTHON_VERSION < 0x3d0
#define CURRENT_TSTATE_INTERPRETER_FRAME(tstate) tstate->cframe->current_frame
#else
#define CURRENT_TSTATE_INTERPRETER_FRAME(tstate) tstate->current_frame
#endif

PFORGE_MAY_BE_UNUSED inline static void pushFrameStackInterpreterFrame(PyThreadState *tstate,
                                                                       _PyInterpreterFrame *interpreter_frame) {
    _PyInterpreterFrame *old = CURRENT_TSTATE_INTERPRETER_FRAME(tstate);
    interpreter_frame->previous = old;
    CURRENT_TSTATE_INTERPRETER_FRAME(tstate) = interpreter_frame;

    if (old != NULL && !_PyFrame_IsIncomplete(old) && interpreter_frame->frame_obj) {
        interpreter_frame->frame_obj->f_back = old->frame_obj;
        CHECK_OBJECT_X(old->frame_obj);

        Py_XINCREF(old->frame_obj);
    }
}
#else
// Put frame at the top of the frame stack and mark as executing.
PFORGE_MAY_BE_UNUSED inline static void pushFrameStackPythonFrame(PyThreadState *tstate, PyFrameObject *frame_object) {
    PRINT_TOP_FRAME("Normal push entry top frame:");
    PRINT_COMPILED_FRAME("Pushing:", frame_object);

    // Make sure it's healthy.
    assertPythonFrameObject(frame_object);

    PyFrameObject *old = tstate->frame;
    CHECK_OBJECT_X(old);

    if (old) {
        assertPythonFrameObject(old);
        CHECK_CODE_OBJECT(Pforge_Frame_GetCodeObject(old));
    }

    // No recursion with identical frames allowed, assert against it.
    assert(old != frame_object);

    // Push the new frame as the currently active one.
    tstate->frame = frame_object;

    // Transfer ownership of old frame.
    if (old != NULL) {
        frame_object->f_back = old;
    }

    Pforge_PythonFrame_MarkAsExecuting(frame_object);
    Py_INCREF(frame_object);

    PRINT_TOP_FRAME("Normal push exit top frame:");
}
#endif

PFORGE_MAY_BE_UNUSED inline static void pushFrameStackCompiledFrame(PyThreadState *tstate,
                                                                    struct Pforge_FrameObject *frame_object) {
#if PYTHON_VERSION < 0x3b0
    pushFrameStackPythonFrame(tstate, &frame_object->m_frame);
#else
    pushFrameStackInterpreterFrame(tstate, &frame_object->m_interpreter_frame);

    Pforge_Frame_MarkAsExecuting(frame_object);
    Py_INCREF(frame_object);
#endif
}

PFORGE_MAY_BE_UNUSED inline static void popFrameStack(PyThreadState *tstate) {
#if _DEBUG_FRAME
    PRINT_TOP_FRAME("Normal pop entry top frame:");
#endif

#if PYTHON_VERSION < 0x3b0
    struct Pforge_FrameObject *frame_object = (struct Pforge_FrameObject *)(tstate->frame);
    CHECK_OBJECT(frame_object);

#if _DEBUG_FRAME
    printf("Taking off frame %s %s\n", Pforge_String_AsString(PyObject_Str((PyObject *)frame_object)),
           Pforge_String_AsString(PyObject_Repr((PyObject *)Pforge_Frame_GetCodeObject(&frame_object->m_frame))));
#endif

    // Put previous frame on top.
    tstate->frame = frame_object->m_frame.f_back;
    frame_object->m_frame.f_back = NULL;

    Pforge_Frame_MarkAsNotExecuting(frame_object);
    Py_DECREF(frame_object);
#else
    assert(CURRENT_TSTATE_INTERPRETER_FRAME(tstate));

    struct Pforge_FrameObject *frame_object =
        (struct Pforge_FrameObject *)CURRENT_TSTATE_INTERPRETER_FRAME(tstate)->frame_obj;
    CHECK_OBJECT(frame_object);

    CURRENT_TSTATE_INTERPRETER_FRAME(tstate) = CURRENT_TSTATE_INTERPRETER_FRAME(tstate)->previous;

    Pforge_Frame_MarkAsNotExecuting(frame_object);

    CHECK_OBJECT_X(frame_object->m_frame.f_back);
    Py_CLEAR(frame_object->m_frame.f_back);

    frame_object->m_interpreter_frame.previous = NULL;

    Py_DECREF(frame_object);
#endif

#if _DEBUG_FRAME
    PRINT_TOP_FRAME("Normal pop exit top frame:");
#endif
}

#if PYTHON_VERSION >= 0x300
PFORGE_MAY_BE_UNUSED static void Pforge_SetFrameGenerator(struct Pforge_FrameObject *pforge_frame,
                                                          PyObject *generator) {
#if PYTHON_VERSION < 0x3b0
    pforge_frame->m_frame.f_gen = generator;
#else
    pforge_frame->m_generator = generator;
#endif

    // Mark the frame as executing
    if (generator) {
        Pforge_Frame_MarkAsExecuting(pforge_frame);
    }
}
#endif

PFORGE_MAY_BE_UNUSED static inline void Pforge_Frame_EnsureTracked(struct Pforge_FrameObject *frame_object) {
    CHECK_OBJECT(frame_object);

    if (!_PyObject_GC_IS_TRACKED(frame_object)) {
        Pforge_GC_Track(frame_object);
    }
}

PFORGE_MAY_BE_UNUSED static inline void Pforge_Frame_ClearHiddenState(struct Pforge_FrameObject *frame_object) {
    assert(Pforge_Frame_CheckExact((PyObject *)frame_object));
    CHECK_OBJECT(frame_object);

#if PYTHON_VERSION < 0x3b0
    PyFrameObject *locals_owner = &frame_object->m_frame;
#else
    _PyInterpreterFrame *locals_owner = &frame_object->m_interpreter_frame;
#endif

    Py_CLEAR(locals_owner->f_locals);

#if PYTHON_VERSION >= 0x3d0
    Py_CLEAR(frame_object->m_frame.f_extra_locals);
    Py_CLEAR(frame_object->m_frame.f_locals_cache);
#endif
#if PYTHON_VERSION >= 0x3e0
    Py_CLEAR(frame_object->m_frame.f_overwritten_fast_locals);
#endif
#if PYTHON_VERSION < 0x370
    Py_CLEAR(frame_object->m_frame.f_exc_type);
    Py_CLEAR(frame_object->m_frame.f_exc_value);
    Py_CLEAR(frame_object->m_frame.f_exc_traceback);
#endif
}

PFORGE_MAY_BE_UNUSED static inline void Pforge_Frame_MarkAsIdle(struct Pforge_FrameObject *frame_object) {
    CHECK_OBJECT(frame_object);

    assert(frame_object->m_frame.f_back == NULL);
#if PYTHON_VERSION >= 0x3b0
    assert(frame_object->m_generator == NULL);
#elif PYTHON_VERSION >= 0x300
    assert(frame_object->m_frame.f_gen == NULL);
#endif
#if PYTHON_VERSION >= 0x300
    assert(!Pforge_Frame_IsExecuting(frame_object));
#endif

    Pforge_Frame_ClearHiddenState(frame_object);

    if (_PyObject_GC_IS_TRACKED(frame_object)) {
        Pforge_GC_UnTrack(frame_object);
    }
}

PFORGE_MAY_BE_UNUSED static inline bool Pforge_FrameCache_IsClaimed(struct Pforge_FrameCacheHandle *cache) {
    assert(cache != NULL);

#ifdef Py_GIL_DISABLED
    return _Py_atomic_load_uint8_relaxed(&cache->in_use) != 0;
#else
    return cache->in_use != 0;
#endif
}

PFORGE_MAY_BE_UNUSED static inline bool Pforge_FrameCache_Claim(struct Pforge_FrameCacheHandle *cache) {
    assert(cache != NULL);

#ifdef Py_GIL_DISABLED
    uint8_t expected = 0;
    return _Py_atomic_compare_exchange_uint8(&cache->in_use, &expected, 1);
#else
    if (cache->in_use != 0) {
        return false;
    }

    cache->in_use = 1;
    return true;
#endif
}

PFORGE_MAY_BE_UNUSED static inline void Pforge_FrameCache_Release(struct Pforge_FrameCacheHandle *cache) {
    assert(cache != NULL);
    assert(Pforge_FrameCache_IsClaimed(cache));

#ifdef Py_GIL_DISABLED
    _Py_atomic_store_uint8(&cache->in_use, 0);
#else
    cache->in_use = 0;
#endif
}

PFORGE_MAY_BE_UNUSED static inline struct Pforge_FrameCacheHandle *
Pforge_FrameCache_Take(struct Pforge_FrameCacheHandle *cache, struct Pforge_FrameObject **frame) {
    assert(cache != NULL);
    assert(frame != NULL);

    *frame = NULL;

    if (!Pforge_FrameCache_Claim(cache)) {
        return NULL;
    }

    struct Pforge_FrameObject *cached_frame = cache->frame;

    if (cached_frame == NULL) {
#if _DEBUG_REFCOUNTS
        count_active_frame_cache_instances += 1;
        count_allocated_frame_cache_instances += 1;
#endif

        return cache;
    }

    if (isFrameUnusable(cached_frame)) {
        cache->frame = NULL;
        cached_frame->m_frame_cache = NULL;

#if _DEBUG_REFCOUNTS
        count_released_frame_cache_instances += 1;
        count_allocated_frame_cache_instances += 1;
#endif

        Py_DECREF(cached_frame);

        return cache;
    }

    // A frame can be left in the cache while still tracked when it had
    // external references at return time. With a refcount of one, those are
    // gone now, so its hidden locals can be dropped before reuse.
    if (_PyObject_GC_IS_TRACKED(cached_frame)) {
        Pforge_Frame_ClearHiddenState(cached_frame);
    }

    Pforge_Frame_EnsureTracked(cached_frame);
    cached_frame->m_frame_cache = cache;

#if _DEBUG_REFCOUNTS
    count_hit_frame_cache_instances += 1;
#endif

    *frame = cached_frame;

    return cache;
}

PFORGE_MAY_BE_UNUSED static inline void Pforge_FrameCache_Attach(struct Pforge_FrameCacheHandle *cache,
                                                                 struct Pforge_FrameObject *frame) {
    assert(cache != NULL);
    assert(frame != NULL);
    assert(Pforge_FrameCache_IsClaimed(cache));
    assert(cache->frame == NULL);
    assert(frame->m_frame_cache == NULL);

    cache->frame = frame;
    frame->m_frame_cache = cache;
}

PFORGE_MAY_BE_UNUSED static inline void Pforge_FrameCache_Return(struct Pforge_FrameObject *frame) {
    CHECK_OBJECT(frame);

    struct Pforge_FrameCacheHandle *cache = frame->m_frame_cache;

    if (cache == NULL) {
        Py_DECREF(frame);

        return;
    }

    assert(cache->frame == frame);
    assert(Pforge_FrameCache_IsClaimed(cache));
    assert(Py_REFCNT(frame) >= 1);

    if (Py_REFCNT(frame) == 1) {
        Pforge_Frame_MarkAsIdle(frame);
    }

    Pforge_FrameCache_Release(cache);
}

PFORGE_MAY_BE_UNUSED static inline void Pforge_FrameCache_DropCall(struct Pforge_FrameObject *frame) {
    CHECK_OBJECT(frame);

    struct Pforge_FrameCacheHandle *cache = frame->m_frame_cache;

    if (cache == NULL) {
        Py_DECREF(frame);

        return;
    }

    assert(cache->frame == frame);
    assert(Pforge_FrameCache_IsClaimed(cache));

    cache->frame = NULL;
    frame->m_frame_cache = NULL;

#if _DEBUG_REFCOUNTS
    count_active_frame_cache_instances -= 1;
    count_released_frame_cache_instances += 1;
#endif

    Py_DECREF(frame);
    Pforge_FrameCache_Release(cache);
}

PFORGE_MAY_BE_UNUSED static inline void Pforge_FrameCache_DropCache(struct Pforge_FrameObject *frame) {
    CHECK_OBJECT(frame);

    struct Pforge_FrameCacheHandle *cache = frame->m_frame_cache;

    if (cache == NULL) {
        return;
    }

    assert(cache->frame == frame);
    assert(Pforge_FrameCache_IsClaimed(cache));

    cache->frame = NULL;
    frame->m_frame_cache = NULL;

#if _DEBUG_REFCOUNTS
    count_active_frame_cache_instances -= 1;
    count_released_frame_cache_instances += 1;
#endif

    Py_DECREF(frame);
    Pforge_FrameCache_Release(cache);
}

PFORGE_MAY_BE_UNUSED static inline void Pforge_FrameCache_ReleaseGeneratorFrame(struct Pforge_FrameObject *frame) {
    CHECK_OBJECT(frame);

    struct Pforge_FrameCacheHandle *cache = frame->m_frame_cache;

    if (cache == NULL) {
        Py_DECREF(frame);

        return;
    }

    assert(cache->frame == frame);
    assert(Pforge_FrameCache_IsClaimed(cache));
    assert(Py_REFCNT(frame) >= 2);

    // Release the generator's own reference, then let the cache decide whether
    // the frame becomes idle or stays tracked for external references.
    Py_DECREF(frame);
    Pforge_FrameCache_Return(frame);
}

PFORGE_MAY_BE_UNUSED static PyCodeObject *Pforge_GetFrameCodeObject(struct Pforge_FrameObject *pforge_frame) {
#if PYTHON_VERSION < 0x3b0
    return pforge_frame->m_frame.f_code;
#else
    return Pforge_InterpreterFrame_GetCodeObject(&pforge_frame->m_interpreter_frame);
#endif
}

PFORGE_MAY_BE_UNUSED static int Pforge_GetFrameLineNumber(struct Pforge_FrameObject *pforge_frame) {
    return pforge_frame->m_frame.f_lineno;
}

PFORGE_MAY_BE_UNUSED static PyObject **Pforge_GetCodeVarNames(PyCodeObject *code_object) {
#if PYTHON_VERSION < 0x3b0
    return &PyTuple_GET_ITEM(code_object->co_varnames, 0);
#else
    // TODO: Might get away with co_names which will be much faster, that functions
    // that build a new tuple, that we would have to keep around, but it might be
    // merged with closure variable names, etc. as as such might become wrong.
    return &PyTuple_GET_ITEM(code_object->co_localsplusnames, 0);
#endif
}

// Attach locals to a frame object. TODO: Upper case, this is for generated code only.
extern void Pforge_Frame_AttachLocals(struct Pforge_FrameObject *frame);

PFORGE_MAY_BE_UNUSED static Pforge_ThreadStateFrameType *_Pforge_GetThreadStateFrame(PyThreadState *tstate) {
#if PYTHON_VERSION < 0x3b0
    return tstate->frame;
#else
    return CURRENT_TSTATE_INTERPRETER_FRAME(tstate);
#endif
}

PFORGE_MAY_BE_UNUSED inline static void pushFrameStackGenerator(PyThreadState *tstate,
                                                                Pforge_ThreadStateFrameType *frame_object) {
#if PYTHON_VERSION < 0x3b0
    Pforge_ThreadStateFrameType *return_frame = _Pforge_GetThreadStateFrame(tstate);

    Py_XINCREF(return_frame);
    // Put the generator back on the frame stack.
    pushFrameStackPythonFrame(tstate, frame_object);
    Py_DECREF(frame_object);
#else
    pushFrameStackInterpreterFrame(tstate, frame_object);
#endif
}

PFORGE_MAY_BE_UNUSED inline static void pushFrameStackGeneratorCompiledFrame(PyThreadState *tstate,
                                                                             struct Pforge_FrameObject *frame_object) {
#if PYTHON_VERSION < 0x3b0
    pushFrameStackGenerator(tstate, &frame_object->m_frame);
#else
    pushFrameStackGenerator(tstate, &frame_object->m_interpreter_frame);
#endif
}

// The "PFORGE_TYPE_DESCRIPTION_*" indicator byte values alias the
// "PFORGE_FRAME_LOCALS_TYPE_*" macros of the "constants_blob_spec.h" file,
// so the actual values are only defined in one place.
#define PFORGE_TYPE_DESCRIPTION_NULL PFORGE_FRAME_LOCALS_TYPE_NULL
#define PFORGE_TYPE_DESCRIPTION_CELL PFORGE_FRAME_LOCALS_TYPE_CELL
#define PFORGE_TYPE_DESCRIPTION_OBJECT PFORGE_FRAME_LOCALS_TYPE_OBJECT
#define PFORGE_TYPE_DESCRIPTION_OBJECT_PTR PFORGE_FRAME_LOCALS_TYPE_OBJECT_PTR
#define PFORGE_TYPE_DESCRIPTION_BOOL PFORGE_FRAME_LOCALS_TYPE_BOOL
#define PFORGE_TYPE_DESCRIPTION_NILONG PFORGE_FRAME_LOCALS_TYPE_NILONG

// The type_description has one indicator byte per frame variable (in
// co_varnames order). In debug builds it gets a trailing NUL that the
// assertions rely on. The frame-locals walkers derive each member's size and
// alignment from these functions, so member offsets (including alignment
// padding) are computed at C compile time and stay correct independent of the
// target ABI.
PFORGE_MAY_BE_UNUSED static inline size_t Pforge_FrameLocals_Size(int type) {
    switch (type) {
    case PFORGE_TYPE_DESCRIPTION_OBJECT:
    case PFORGE_TYPE_DESCRIPTION_OBJECT_PTR:
    case PFORGE_TYPE_DESCRIPTION_CELL:
        return sizeof(PyObject *);
    case PFORGE_TYPE_DESCRIPTION_NILONG:
        return sizeof(pforge_ilong);
    case PFORGE_TYPE_DESCRIPTION_BOOL:
        return sizeof(pforge_bool);
    case PFORGE_TYPE_DESCRIPTION_NULL:
        return 0;
    default:
        PFORGE_CANNOT_GET_HERE("invalid frame locals type indicator");
        return 0;
    }
}

PFORGE_MAY_BE_UNUSED static inline size_t Pforge_FrameLocals_Align(int type) {
    switch (type) {
    case PFORGE_TYPE_DESCRIPTION_OBJECT:
    case PFORGE_TYPE_DESCRIPTION_OBJECT_PTR:
    case PFORGE_TYPE_DESCRIPTION_CELL:
    case PFORGE_TYPE_DESCRIPTION_NILONG:
        // All of these members contain (or are) a pointer, so their alignment
        // equals the pointer size on every supported ABI.
        return sizeof(PyObject *);
    case PFORGE_TYPE_DESCRIPTION_BOOL:
        return sizeof(pforge_bool);
    case PFORGE_TYPE_DESCRIPTION_NULL:
        return 1;
    default:
        PFORGE_CANNOT_GET_HERE("invalid frame locals type indicator");
        return 1;
    }
}

PFORGE_MAY_BE_UNUSED static inline size_t Pforge_FrameLocals_AlignUp(size_t value, size_t align) {
    return (value + align - 1) / align * align;
}

PFORGE_MAY_BE_UNUSED static inline bool Pforge_FrameLocals_NilongActive(pforge_ilong const *value) {
    return IS_NILONG_OBJECT_VALUE_VALID(value) || IS_NILONG_C_VALUE_VALID(value);
}

#if _DEBUG_REFCOUNTS
extern int count_active_Pforge_Frame_Type;
extern int count_allocated_Pforge_Frame_Type;
extern int count_released_Pforge_Frame_Type;
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
