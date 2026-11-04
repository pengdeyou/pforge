//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

/** Compiled Coroutines.
 *
 * Unlike in CPython, we have one type for just coroutines, this doesn't do generators
 * nor asyncgen.
 *
 * It strives to be full replacement for normal coroutines.
 *
 */

// This file is included from another C file, help IDEs to still parse it on
// its own.
#ifdef __IDE_ONLY__
#include "pforge/prelude.h"

#include "pforge/compiled_types_common.h"
#include "pforge/freelists.h"
#include <structmember.h>
#endif

// For reporting about reference counts per type.
#if _DEBUG_REFCOUNTS
int count_active_Pforge_Coroutine_Type = 0;
int count_allocated_Pforge_Coroutine_Type = 0;
int count_released_Pforge_Coroutine_Type = 0;
int count_active_Pforge_CoroutineWrapper_Type = 0;
int count_allocated_Pforge_CoroutineWrapper_Type = 0;
int count_released_Pforge_CoroutineWrapper_Type = 0;
int count_active_Pforge_AIterWrapper_Type = 0;
int count_allocated_Pforge_AIterWrapper_Type = 0;
int count_released_Pforge_AIterWrapper_Type = 0;
#endif

static void Pforge_MarkCoroutineAsFinished(struct Pforge_CoroutineObject *coroutine) {
    coroutine->m_status = status_Finished;

#if PYTHON_VERSION >= 0x3b0
    if (coroutine->m_frame) {
        coroutine->m_frame->m_frame_state = FRAME_COMPLETED;
    }
#endif
}

static void Pforge_MarkCoroutineAsRunning(struct Pforge_CoroutineObject *coroutine) {
    coroutine->m_running = 1;

    if (coroutine->m_frame) {
        Pforge_Frame_MarkAsExecuting(coroutine->m_frame);
    }
}

static void Pforge_MarkCoroutineAsNotRunning(struct Pforge_CoroutineObject *coroutine) {
    coroutine->m_running = 0;

    if (coroutine->m_frame) {
        Pforge_Frame_MarkAsNotExecuting(coroutine->m_frame);
    }
}

static PyObject *_Pforge_Coroutine_send(PyThreadState *tstate, struct Pforge_CoroutineObject *coroutine,
                                        PyObject *value, bool closing,
                                        struct Pforge_ExceptionPreservationItem *exception_state);

static Py_hash_t Pforge_Coroutine_tp_hash(PyObject *self) {
    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;

    return coroutine->m_counter;
}

static PyObject *Pforge_Coroutine_get_name(PyObject *self, void *data) {
    CHECK_OBJECT(self);

    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;
    Py_INCREF(coroutine->m_name);
    return coroutine->m_name;
}

static int Pforge_Coroutine_set_name(PyObject *self, PyObject *value, void *data) {
    CHECK_OBJECT(self);
    CHECK_OBJECT_X(value);

    // Cannot be deleted, not be non-unicode value.
    if (unlikely((value == NULL) || !PyUnicode_Check(value))) {
        PyThreadState *tstate = PyThreadState_GET();

        SET_CURRENT_EXCEPTION_TYPE0_STR(tstate, PyExc_TypeError, "__name__ must be set to a string object");
        return -1;
    }

    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;
    PyObject *tmp = coroutine->m_name;
    Py_INCREF(value);
    coroutine->m_name = value;
    Py_DECREF(tmp);

    return 0;
}

static PyObject *Pforge_Coroutine_get_qualname(PyObject *self, void *data) {
    CHECK_OBJECT(self);

    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;
    Py_INCREF(coroutine->m_qualname);
    return coroutine->m_qualname;
}

static int Pforge_Coroutine_set_qualname(PyObject *self, PyObject *value, void *data) {
    CHECK_OBJECT(self);
    CHECK_OBJECT_X(value);

    // Cannot be deleted, not be non-unicode value.
    if (unlikely((value == NULL) || !PyUnicode_Check(value))) {
        PyThreadState *tstate = PyThreadState_GET();

        SET_CURRENT_EXCEPTION_TYPE0_STR(tstate, PyExc_TypeError, "__qualname__ must be set to a string object");
        return -1;
    }

    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;
    PyObject *tmp = coroutine->m_qualname;
    Py_INCREF(value);
    coroutine->m_qualname = value;
    Py_DECREF(tmp);

    return 0;
}

static PyObject *Pforge_Coroutine_get_cr_await(PyObject *self, void *data) {
    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;
    CHECK_OBJECT(coroutine);
    CHECK_OBJECT_X(coroutine->m_yield_from);

    if (coroutine->m_yield_from) {
        Py_INCREF(coroutine->m_yield_from);
        return coroutine->m_yield_from;
    } else {
        Py_INCREF_IMMORTAL(Py_None);
        return Py_None;
    }
}

static PyObject *Pforge_Coroutine_get_code(PyObject *self, void *data) {
    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;
    CHECK_OBJECT(coroutine);
    CHECK_OBJECT(coroutine->m_code_object);

    Py_INCREF(coroutine->m_code_object);
    return (PyObject *)coroutine->m_code_object;
}

static int Pforge_Coroutine_set_code(PyObject *self, PyObject *value, void *data) {
    CHECK_OBJECT(self);

    PyThreadState *tstate = PyThreadState_GET();

    SET_CURRENT_EXCEPTION_TYPE0_STR(tstate, PyExc_RuntimeError, "cr_code is not writable in Pforge");
    return -1;
}

static PyObject *Pforge_Coroutine_get_frame(PyObject *self, void *data) {
    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;
    CHECK_OBJECT(coroutine);
    CHECK_OBJECT_X(coroutine->m_frame);

    if (coroutine->m_frame) {
        Py_INCREF(coroutine->m_frame);
        return (PyObject *)coroutine->m_frame;
    } else {
        Py_INCREF_IMMORTAL(Py_None);
        return Py_None;
    }
}

static int Pforge_Coroutine_set_frame(PyObject *self, PyObject *value, void *data) {
    CHECK_OBJECT(self);
    CHECK_OBJECT_X(value);

    PyThreadState *tstate = PyThreadState_GET();

    SET_CURRENT_EXCEPTION_TYPE0_STR(tstate, PyExc_RuntimeError, "gi_frame is not writable in Pforge");
    return -1;
}

static void Pforge_Coroutine_release_closure(struct Pforge_CoroutineObject *coroutine) {
    for (Py_ssize_t i = 0; i < coroutine->m_closure_given; i++) {
        CHECK_OBJECT(coroutine->m_closure[i]);
        Py_DECREF(coroutine->m_closure[i]);
    }

    coroutine->m_closure_given = 0;
}

// Note: Shared with asyncgen.
static PyObject *_Pforge_YieldFromCore(PyThreadState *tstate, PyObject *yield_from, PyObject *send_value,
                                       PyObject **returned_value, bool mode) {
    // Send iteration value to the sub-generator, which may be a CPython
    // generator object, something with an iterator next, or a send method,
    // where the later is only required if values other than "None" need to
    // be passed in.
    CHECK_OBJECT(yield_from);
    CHECK_OBJECT_X(send_value);

    assert(send_value != NULL || HAS_ERROR_OCCURRED(tstate));

    PyObject *retval;

    struct Pforge_ExceptionPreservationItem exception_state;

    FETCH_ERROR_OCCURRED_STATE(tstate, &exception_state);

    if (HAS_EXCEPTION_STATE(&exception_state)) {
        // Exception, was thrown into us, need to send that to sub-generator.
        // We acquired ownership of the published exception and need to release it potentially.

        // Transfer exception owner this.
        retval = _Pforge_YieldFromPassExceptionTo(tstate, yield_from, &exception_state);

        // TODO: This wants to look at retval most definitely, send_value is going to be NULL.
        if (unlikely(send_value == NULL)) {
            PyObject *error = GET_ERROR_OCCURRED(tstate);

            if (error != NULL && EXCEPTION_MATCH_BOOL_SINGLE(tstate, error, PyExc_StopIteration)) {
                *returned_value = ERROR_GET_STOP_ITERATION_VALUE(tstate);
                assert(!HAS_ERROR_OCCURRED(tstate));

                return NULL;
            }
        }
    } else if (PyGen_CheckExact(yield_from) || PyCoro_CheckExact(yield_from)) {
        retval = Pforge_PyGen_Send(tstate, (PyGenObject *)yield_from, send_value);
    } else if (send_value == Py_None && Pforge_CoroutineWrapper_Check(yield_from)) {
        struct Pforge_CoroutineObject *yield_from_coroutine =
            ((struct Pforge_CoroutineWrapperObject *)yield_from)->m_coroutine;

        Py_INCREF_IMMORTAL(Py_None);

        struct Pforge_ExceptionPreservationItem no_exception_state;
        INIT_ERROR_OCCURRED_STATE(&no_exception_state);

        retval =
            _Pforge_Coroutine_send(tstate, yield_from_coroutine, Py_None, mode ? false : true, &no_exception_state);
    } else if (send_value == Py_None && Py_TYPE(yield_from)->tp_iternext != NULL) {
        retval = Py_TYPE(yield_from)->tp_iternext(yield_from);
    } else {
#if 0
        // TODO: Add slow mode traces.
        PRINT_ITEM(yield_from);
        PRINT_NEW_LINE();
#endif

        retval = PyObject_CallMethodObjArgs(yield_from, const_str_plain_send, send_value, NULL);
    }

    // Check the sub-generator result
    if (retval == NULL) {
        PyObject *error = GET_ERROR_OCCURRED(tstate);

        if (error == NULL) {
            Py_INCREF_IMMORTAL(Py_None);
            *returned_value = Py_None;
        } else if (likely(EXCEPTION_MATCH_BOOL_SINGLE(tstate, error, PyExc_StopIteration))) {
            // The sub-generator has given an exception. In case of
            // StopIteration, we need to check the value, as it is going to be
            // the expression value of this "yield from", and we are done. All
            // other errors, we need to raise.
            *returned_value = ERROR_GET_STOP_ITERATION_VALUE(tstate);
            assert(!HAS_ERROR_OCCURRED(tstate));

            assert(*returned_value != NULL);
        } else {
            *returned_value = NULL;
        }

        return NULL;
    } else {
        assert(!HAS_ERROR_OCCURRED(tstate));
        return retval;
    }
}

static PyObject *_Pforge_YieldFromCoroutineCore(PyThreadState *tstate, struct Pforge_CoroutineObject *coroutine,
                                                PyObject *send_value, bool mode) {
    CHECK_OBJECT(coroutine);
    CHECK_OBJECT_X(send_value);

    PyObject *yield_from = coroutine->m_yield_from;
    CHECK_OBJECT(yield_from);

    // Need to make it unaccessible while using it.
    coroutine->m_yield_from = NULL;

    // Before yielding to an inner coroutine, swap the outer coroutine's
    // exception onto the thread state so that sys.exc_info() can see it
    SAVE_COROUTINE_EXCEPTION(tstate, coroutine);

    PyObject *returned_value;
    PyObject *yielded = _Pforge_YieldFromCore(tstate, yield_from, send_value, &returned_value, mode);

    RESTORE_COROUTINE_EXCEPTION(tstate, coroutine);

    if (yielded == NULL) {
        assert(coroutine->m_yield_from == NULL);
        Py_DECREF(yield_from);

        yielded = ((coroutine_code)coroutine->m_code)(tstate, coroutine, returned_value);
    } else {
        assert(coroutine->m_yield_from == NULL);
        coroutine->m_yield_from = yield_from;
    }

    return yielded;
}

#if _DEBUG_COROUTINE
PFORGE_MAY_BE_UNUSED static void _PRINT_COROUTINE_STATUS(char const *descriptor, char const *context,
                                                         struct Pforge_CoroutineObject *coroutine) {
    char const *status;

    switch (coroutine->m_status) {
    case status_Finished:
        status = "(finished)";
        break;
    case status_Running:
        status = "(running)";
        break;
    case status_Unused:
        status = "(unused)";
        break;
    default:
        status = "(ILLEGAL)";
        break;
    }

    PRINT_STRING(descriptor);
    PRINT_STRING(" : ");
    PRINT_STRING(context);
    PRINT_STRING(" ");
    PRINT_ITEM((PyObject *)coroutine);
    PRINT_STRING(" ");
    PRINT_REFCOUNT((PyObject *)coroutine);
    PRINT_STRING(status);
    PRINT_NEW_LINE();
}

#define PRINT_COROUTINE_STATUS(context, coroutine) _PRINT_COROUTINE_STATUS(__FUNCTION__, context, coroutine)

#endif

static PyObject *Pforge_YieldFromCoroutineNext(PyThreadState *tstate, struct Pforge_CoroutineObject *coroutine) {
    CHECK_OBJECT(coroutine);

#if _DEBUG_COROUTINE
    PRINT_COROUTINE_STATUS("Enter", coroutine);
    PRINT_NEW_LINE();
#endif
    PyObject *result = _Pforge_YieldFromCoroutineCore(tstate, coroutine, Py_None, true);
#if _DEBUG_COROUTINE
    PRINT_COROUTINE_STATUS("Leave", coroutine);
    PRINT_CURRENT_EXCEPTION();
    PRINT_NEW_LINE();
#endif
    return result;
}

static PyObject *Pforge_YieldFromCoroutineInitial(PyThreadState *tstate, struct Pforge_CoroutineObject *coroutine,
                                                  PyObject *send_value) {
    CHECK_OBJECT(coroutine);
    CHECK_OBJECT_X(send_value);

#if _DEBUG_COROUTINE
    PRINT_COROUTINE_STATUS("Enter", coroutine);
    PRINT_NEW_LINE();
#endif
    PyObject *result = _Pforge_YieldFromCoroutineCore(tstate, coroutine, send_value, false);
#if _DEBUG_COROUTINE
    PRINT_COROUTINE_STATUS("Leave", coroutine);
    PRINT_CURRENT_EXCEPTION();
    PRINT_NEW_LINE();
#endif
    return result;
}

// This function is called when sending a value or exception to be handled in the coroutine
// Note:
//   Exception arguments are passed for ownership and must be released before returning. The
//   value of exception_type may be NULL, and the actual exception will not necessarily
//   be normalized.

static PySendResult _Pforge_Coroutine_sendR(PyThreadState *tstate, struct Pforge_CoroutineObject *coroutine,
                                            PyObject *value, bool closing,
                                            struct Pforge_ExceptionPreservationItem *exception_state,
                                            PyObject **result) {
    CHECK_OBJECT(coroutine);
    assert(Pforge_Coroutine_Check((PyObject *)coroutine));
    CHECK_EXCEPTION_STATE_X(exception_state);
    CHECK_OBJECT_X(value);

#if _DEBUG_COROUTINE
    PRINT_COROUTINE_STATUS("Enter", coroutine);
    PRINT_COROUTINE_STRING("closing", closing ? "(closing) " : "(not closing) ");
    PRINT_COROUTINE_VALUE("value", value);
    PRINT_EXCEPTION_STATE(exception_state);
    PRINT_CURRENT_EXCEPTION();
    PRINT_NEW_LINE();
#endif

    // Not both a value and an exception please.
    if (value != NULL) {
        ASSERT_EMPTY_EXCEPTION_STATE(exception_state);
    }

    if (coroutine->m_status == status_Unused && value != NULL && value != Py_None) {
        // No exception if value is given.
        Py_XDECREF(value);

        SET_CURRENT_EXCEPTION_TYPE0_STR(tstate, PyExc_TypeError,
                                        "can't send non-None value to a just-started coroutine");
        return PYGEN_ERROR;
    }

    if (coroutine->m_status != status_Finished) {
        if (coroutine->m_running) {
            Py_XDECREF(value);

            SET_CURRENT_EXCEPTION_TYPE0_STR(tstate, PyExc_ValueError, "coroutine already executing");
            return PYGEN_ERROR;
        }

        // Put the coroutine back on the frame stack.
        Pforge_ThreadStateFrameType *return_frame = _Pforge_GetThreadStateFrame(tstate);

        // Consider it as running.
        if (coroutine->m_status == status_Unused) {
            coroutine->m_status = status_Running;
            assert(coroutine->m_resume_frame == NULL);

            // Value will not be used, can only be Py_None or NULL.
            Py_XDECREF(value);
            value = NULL;
        } else {
            assert(coroutine->m_resume_frame);
            pushFrameStackGenerator(tstate, coroutine->m_resume_frame);

            coroutine->m_resume_frame = NULL;
        }

        // Continue the yielder function while preventing recursion.
        Pforge_MarkCoroutineAsRunning(coroutine);

        // Check for thrown exception, publish it to the coroutine code.
        if (unlikely(HAS_EXCEPTION_STATE(exception_state))) {
            assert(value == NULL);

            // Transfer exception ownership to published.
            RESTORE_ERROR_OCCURRED_STATE(tstate, exception_state);
        }

#if _DEBUG_COROUTINE
        PRINT_COROUTINE_STATUS("Switching to coroutine", coroutine);
        PRINT_COROUTINE_VALUE("value", value);
        PRINT_CURRENT_EXCEPTION();
        PRINT_NEW_LINE();
        // dumpFrameStack();
#endif

        PyObject *yielded;

        if (coroutine->m_yield_from == NULL) {
            yielded = ((coroutine_code)coroutine->m_code)(tstate, coroutine, value);
        } else {
            // This does not release the value if any, so we need to do it afterwards.
            yielded = Pforge_YieldFromCoroutineInitial(tstate, coroutine, value);
            Py_XDECREF(value);
        }

        // If the coroutine returns with m_yield_from set, it wants us to yield
        // from that value from now on.
        while (yielded == NULL && coroutine->m_yield_from != NULL) {
            yielded = Pforge_YieldFromCoroutineNext(tstate, coroutine);
        }

        Pforge_MarkCoroutineAsNotRunning(coroutine);

        // Remove the back frame from coroutine if it's there.
        if (coroutine->m_frame) {
            // assert(tstate->frame == &coroutine->m_frame->m_frame);
            assertFrameObject(coroutine->m_frame);

            Py_CLEAR(coroutine->m_frame->m_frame.f_back);

            // Remember where to resume from.
            coroutine->m_resume_frame = _Pforge_GetThreadStateFrame(tstate);
        }

        // Return back to the frame that called us.
        _Pforge_GeneratorPopFrame(tstate, return_frame);

#if _DEBUG_COROUTINE
        PRINT_COROUTINE_STATUS("Returned from coroutine", coroutine);
        // dumpFrameStack();
#endif

#ifndef __PFORGE_NO_ASSERT__
        if (return_frame) {
            assertThreadFrameObject(return_frame);
        }
#endif

        if (yielded == NULL) {
#if _DEBUG_COROUTINE
            PRINT_COROUTINE_STATUS("finishing from yield", coroutine);
            PRINT_COROUTINE_STRING("closing", closing ? "(closing) " : "(not closing) ");
            PRINT_STRING("-> finishing coroutine sets status_Finished\n");
            PRINT_COROUTINE_VALUE("return_value", coroutine->m_returned);
            PRINT_CURRENT_EXCEPTION();
            PRINT_NEW_LINE();
#endif
            Pforge_MarkCoroutineAsFinished(coroutine);

            if (coroutine->m_frame != NULL) {
                struct Pforge_FrameObject *release_frame = coroutine->m_frame;
                coroutine->m_frame = NULL;

                Pforge_SetFrameGenerator(release_frame, NULL);
                Pforge_FrameCache_ReleaseGeneratorFrame(release_frame);
            }

            Pforge_Coroutine_release_closure(coroutine);

            // Create StopIteration if necessary, i.e. return value that is not "None" was
            // given. TODO: Push this further down the user line, we might be able to avoid
            // it for some uses, e.g. quick iteration entirely.
            if (coroutine->m_returned) {
                *result = coroutine->m_returned;

                coroutine->m_returned = NULL;

#if _DEBUG_COROUTINE
                PRINT_COROUTINE_STATUS("Return value to exception set", coroutine);
                PRINT_CURRENT_EXCEPTION();
                PRINT_NEW_LINE();
#endif
                return PYGEN_RETURN;
            } else {
                PyObject *error = GET_ERROR_OCCURRED(tstate);

                if (error == NULL) {
                    *result = NULL;

                    return PYGEN_RETURN;
                } else if (error == PyExc_StopIteration) {
                    RAISE_RUNTIME_ERROR_RAISED_STOP_ITERATION(tstate, "coroutine raised StopIteration");

#if _DEBUG_COROUTINE
                    PRINT_COROUTINE_STATUS("Leave with exception set", coroutine);
                    PRINT_CURRENT_EXCEPTION();
                    PRINT_NEW_LINE();
#endif
                }

                return PYGEN_ERROR;
            }
        } else {
            *result = yielded;
            return PYGEN_NEXT;
        }
    } else {
        Py_XDECREF(value);

        // Release exception if any, we are finished with it and will raise another.
        RELEASE_ERROR_OCCURRED_STATE_X(exception_state);

        /* This is for status_Finished */
        assert(coroutine->m_status == status_Finished);
        /* This check got added in Python 3.5.2 only. It's good to do it, but
         * not fully compatible, therefore guard it.
         */
#if PYTHON_VERSION >= 0x352 || !defined(_PFORGE_FULL_COMPAT)
        if (closing == false) {
#if _DEBUG_COROUTINE
            PRINT_COROUTINE_STATUS("Finished coroutine sent into -> RuntimeError", coroutine);
            PRINT_NEW_LINE();
#endif
            PyErr_Format(PyExc_RuntimeError,
#if !defined(_PFORGE_FULL_COMPAT)
                         "cannot reuse already awaited compiled_coroutine %S", coroutine->m_qualname
#else
                         "cannot reuse already awaited coroutine"
#endif
            );

            return PYGEN_ERROR;
        } else
#endif
        {
            *result = NULL;
            return PYGEN_RETURN;
        }
    }
}

static PyObject *_Pforge_Coroutine_send(PyThreadState *tstate, struct Pforge_CoroutineObject *coroutine,
                                        PyObject *value, bool closing,
                                        struct Pforge_ExceptionPreservationItem *exception_state) {

    PyObject *result;
    PySendResult res = _Pforge_Coroutine_sendR(tstate, coroutine, value, closing, exception_state, &result);

    switch (res) {
    case PYGEN_RETURN:
        if (result != NULL) {
            if (result != Py_None) {
                Pforge_SetStopIterationValue(tstate, result);
            }

            Py_DECREF(result);
        }

        return NULL;
    case PYGEN_NEXT:
        return result;
    case PYGEN_ERROR:
        return NULL;
    default:
        PFORGE_CANNOT_GET_HERE("invalid _Pforge_Coroutine_sendR result");
    }
}

static PyObject *_Pforge_Coroutine_send_api(struct Pforge_CoroutineObject *coroutine, PyObject *value) {
    CHECK_OBJECT(coroutine);
    CHECK_OBJECT(value);

    // We need to transfer ownership of the sent value.
    Py_INCREF(value);

    PyThreadState *tstate = PyThreadState_GET();

    struct Pforge_ExceptionPreservationItem exception_state;
    INIT_ERROR_OCCURRED_STATE(&exception_state);

    PyObject *result = _Pforge_Coroutine_send(tstate, coroutine, value, false, &exception_state);

    if (result == NULL) {
        if (HAS_ERROR_OCCURRED(tstate) == false) {
            SET_CURRENT_EXCEPTION_STOP_ITERATION_EMPTY(tstate);
        }
    }

    return result;
}

static PyObject *Pforge_Coroutine_send(PyObject *coroutine_obj, PyObject *value) {
    return _Pforge_Coroutine_send_api((struct Pforge_CoroutineObject *)coroutine_obj, value);
}

// Note: Used by compiled frames.
static bool _Pforge_Coroutine_close(PyThreadState *tstate, struct Pforge_CoroutineObject *coroutine) {
#if _DEBUG_COROUTINE
    PRINT_COROUTINE_STATUS("Enter", coroutine);
#endif
    CHECK_OBJECT(coroutine);

    if (coroutine->m_status == status_Unused) {
#if PYTHON_VERSION == 0x3c0
        coroutine->m_closed_before_start = true;
#endif
        Pforge_MarkCoroutineAsFinished(coroutine);

        return true;
    }

    if (coroutine->m_status == status_Running) {
        struct Pforge_ExceptionPreservationItem exception_state;
        SET_EXCEPTION_PRESERVATION_STATE_FROM_ARGS(tstate, &exception_state, PyExc_GeneratorExit, NULL, NULL);

        PyObject *result = _Pforge_Coroutine_send(tstate, coroutine, NULL, true, &exception_state);

        if (unlikely(result)) {
            Py_DECREF(result);

            SET_CURRENT_EXCEPTION_TYPE0_STR(tstate, PyExc_RuntimeError, "coroutine ignored GeneratorExit");
            return false;
        } else {
            return DROP_ERROR_OCCURRED_GENERATOR_EXIT_OR_STOP_ITERATION(tstate);
        }
    }

    return true;
}

static PyObject *_Pforge_Coroutine_close_api(struct Pforge_CoroutineObject *coroutine, PyObject *args) {
    PyThreadState *tstate = PyThreadState_GET();

    bool r = _Pforge_Coroutine_close(tstate, coroutine);

    if (unlikely(r == false)) {
        return NULL;
    } else {
        Py_INCREF_IMMORTAL(Py_None);
        return Py_None;
    }
}

static PyObject *Pforge_Coroutine_close(PyObject *coroutine_obj, PyObject *args) {
    return _Pforge_Coroutine_close_api((struct Pforge_CoroutineObject *)coroutine_obj, args);
}

static bool _Pforge_Generator_check_throw(PyThreadState *tstate,
                                          struct Pforge_ExceptionPreservationItem *exception_state);

// This function is called when yielding to a coroutine through "_Pforge_YieldFromPassExceptionTo"
// and potentially wrapper objects used by generators, or by the throw method itself.
// Note:
//   Exception arguments are passed for ownership and must be released before returning. The
//   value of exception_type will not be NULL, but the actual exception will not necessarily
//   be normalized.
static PyObject *_Pforge_Coroutine_throw2(PyThreadState *tstate, struct Pforge_CoroutineObject *coroutine, bool closing,
                                          struct Pforge_ExceptionPreservationItem *exception_state) {
    CHECK_OBJECT(coroutine);
    assert(Pforge_Coroutine_Check((PyObject *)coroutine));
    CHECK_EXCEPTION_STATE(exception_state);

#if _DEBUG_COROUTINE
    PRINT_COROUTINE_STATUS("Enter", coroutine);
    PRINT_COROUTINE_STRING("closing", closing ? "(closing) " : "(not closing) ");
    PRINT_COROUTINE_VALUE("yield_from", coroutine->m_yield_from);
    PRINT_EXCEPTION_STATE(exception_state);
    PRINT_NEW_LINE();
#endif

    if (coroutine->m_yield_from != NULL) {
        if (EXCEPTION_STATE_MATCH_BOOL_SINGLE(tstate, exception_state, PyExc_GeneratorExit)) {
            // Coroutines need to close the yield_from.
            Pforge_MarkCoroutineAsRunning(coroutine);
            bool res = Pforge_gen_close_iter(tstate, coroutine->m_yield_from);
            Pforge_MarkCoroutineAsNotRunning(coroutine);

            if (res == false) {
                // Release exception, we are done with it now and pick up the new one.
                RELEASE_ERROR_OCCURRED_STATE(exception_state);

                FETCH_ERROR_OCCURRED_STATE(tstate, exception_state);
            }

            // Transferred exception ownership to "_Pforge_Coroutine_send".
            return _Pforge_Coroutine_send(tstate, coroutine, NULL, false, exception_state);
        }

        PyObject *ret;

#if _DEBUG_COROUTINE
        PRINT_COROUTINE_STATUS("Passing to yielded from", coroutine);
        PRINT_COROUTINE_VALUE("m_yield_from", coroutine->m_yield_from);
        PRINT_NEW_LINE();
#endif

        if (Pforge_Generator_Check(coroutine->m_yield_from)) {
            struct Pforge_GeneratorObject *gen = ((struct Pforge_GeneratorObject *)coroutine->m_yield_from);
            // Transferred exception ownership to "_Pforge_Generator_throw2".
            Pforge_MarkCoroutineAsRunning(coroutine);
            ret = _Pforge_Generator_throw2(tstate, gen, exception_state);
            Pforge_MarkCoroutineAsNotRunning(coroutine);
        } else if (Pforge_Coroutine_Check(coroutine->m_yield_from)) {
            struct Pforge_CoroutineObject *coro = ((struct Pforge_CoroutineObject *)coroutine->m_yield_from);
            // Transferred exception ownership to "_Pforge_Coroutine_throw2".
            Pforge_MarkCoroutineAsRunning(coroutine);
            ret = _Pforge_Coroutine_throw2(tstate, coro, true, exception_state);
            Pforge_MarkCoroutineAsNotRunning(coroutine);
#if PFORGE_UNCOMPILED_THROW_INTEGRATION
        } else if (PyGen_CheckExact(coroutine->m_yield_from) || PyCoro_CheckExact(coroutine->m_yield_from)) {
            PyGenObject *gen = (PyGenObject *)coroutine->m_yield_from;

            // Transferred exception ownership to "Pforge_UncompiledGenerator_throw".
            Pforge_MarkCoroutineAsRunning(coroutine);
            ret = Pforge_UncompiledGenerator_throw(tstate, gen, 1, exception_state);
            Pforge_MarkCoroutineAsNotRunning(coroutine);
#endif
        } else if (Pforge_CoroutineWrapper_Check(coroutine->m_yield_from)) {
            struct Pforge_CoroutineObject *coro =
                ((struct Pforge_CoroutineWrapperObject *)coroutine->m_yield_from)->m_coroutine;

            // Transferred exception ownership to "_Pforge_Coroutine_throw2".
            Pforge_MarkCoroutineAsRunning(coroutine);
            ret = _Pforge_Coroutine_throw2(tstate, coro, true, exception_state);
            Pforge_MarkCoroutineAsNotRunning(coroutine);
#if PYTHON_VERSION >= 0x360
        } else if (Pforge_AsyncgenAsend_Check(coroutine->m_yield_from)) {
            struct Pforge_AsyncgenAsendObject *asyncgen_asend =
                ((struct Pforge_AsyncgenAsendObject *)coroutine->m_yield_from);

            // Transferred exception ownership to "_Pforge_AsyncgenAsend_throw2".
            Pforge_MarkCoroutineAsRunning(coroutine);
            ret = _Pforge_AsyncgenAsend_throw2(tstate, asyncgen_asend, exception_state);
            Pforge_MarkCoroutineAsNotRunning(coroutine);
#endif
        } else {
            PyObject *meth = LOOKUP_ATTRIBUTE(tstate, coroutine->m_yield_from, const_str_plain_throw);
            if (unlikely(meth == NULL)) {
                if (!CHECK_AND_CLEAR_ATTRIBUTE_ERROR_OCCURRED(tstate)) {
                    // Release exception, we are done with it now.
                    RELEASE_ERROR_OCCURRED_STATE(exception_state);

                    return NULL;
                }

                // Passing exception ownership to that code.
                goto throw_here;
            }

            CHECK_EXCEPTION_STATE(exception_state);

#if 0
            // TODO: Add slow mode traces.
            PRINT_ITEM(coroutine->m_yield_from);
            PRINT_NEW_LINE();
#endif
            Pforge_MarkCoroutineAsRunning(coroutine);
            ret = Pforge_CallGeneratorThrowMethod(meth, exception_state);
            Pforge_MarkCoroutineAsNotRunning(coroutine);

            Py_DECREF(meth);

            // Release exception, we are done with it now.
            RELEASE_ERROR_OCCURRED_STATE(exception_state);
        }

        if (unlikely(ret == NULL)) {
            // Return value or exception, not to continue with yielding from.
            if (coroutine->m_yield_from != NULL) {
                CHECK_OBJECT(coroutine->m_yield_from);
#if _DEBUG_COROUTINE
                PRINT_COROUTINE_STATUS("Null return, yield from removal:", coroutine);
                PRINT_COROUTINE_VALUE("yield_from", coroutine->m_yield_from);
#endif
                Py_DECREF(coroutine->m_yield_from);
                coroutine->m_yield_from = NULL;
            }

            PyObject *val;
            if (Pforge_PyGen_FetchStopIterationValue(tstate, &val)) {
                CHECK_OBJECT(val);

#if _DEBUG_COROUTINE
                PRINT_COROUTINE_STATUS("Sending return value into ourselves", coroutine);
                PRINT_COROUTINE_VALUE("value", val);
                PRINT_NEW_LINE();
#endif

                struct Pforge_ExceptionPreservationItem no_exception_state;
                INIT_ERROR_OCCURRED_STATE(&no_exception_state);

                // The ownership of val is transferred.
                ret = _Pforge_Coroutine_send(tstate, coroutine, val, false, &no_exception_state);
            } else {
#if _DEBUG_COROUTINE
                PRINT_COROUTINE_STATUS("Sending exception value into ourselves", coroutine);
                PRINT_CURRENT_EXCEPTION();
                PRINT_NEW_LINE();
#endif

                struct Pforge_ExceptionPreservationItem no_exception_state;
                INIT_ERROR_OCCURRED_STATE(&no_exception_state);

                ret = _Pforge_Coroutine_send(tstate, coroutine, NULL, false, &no_exception_state);
            }

#if _DEBUG_COROUTINE
            PRINT_COROUTINE_STATUS("Leave with value/exception from sending into ourselves:", coroutine);
            PRINT_COROUTINE_STRING("closing", closing ? "(closing) " : "(not closing) ");
            PRINT_COROUTINE_VALUE("return_value", ret);
            PRINT_CURRENT_EXCEPTION();
            PRINT_NEW_LINE();
#endif
        } else {
#if _DEBUG_COROUTINE
            PRINT_COROUTINE_STATUS("Leave with return value:", coroutine);
            PRINT_COROUTINE_STRING("closing", closing ? "(closing) " : "(not closing) ");
            PRINT_COROUTINE_VALUE("return_value", ret);
            PRINT_CURRENT_EXCEPTION();
            PRINT_NEW_LINE();
#endif
        }

        return ret;
    }

throw_here:
    // We continue to have exception ownership here.

    if (unlikely(_Pforge_Generator_check_throw(tstate, exception_state) == false)) {
        // Exception was released by _Pforge_Generator_check_throw already.
        return NULL;
    }

    if (coroutine->m_status == status_Running) {
        // Transferred exception ownership to "_Pforge_Coroutine_send".
        PyObject *result = _Pforge_Coroutine_send(tstate, coroutine, NULL, false, exception_state);
        return result;
    } else if (coroutine->m_status == status_Finished) {

        /* This check got added in Python 3.5.2 only. It's good to do it, but
         * not fully compatible, therefore guard it.
         */
#if PYTHON_VERSION >= 0x352 || !defined(_PFORGE_FULL_COMPAT)
        if (closing == false) {
#if _DEBUG_COROUTINE
            PRINT_STRING("Finished coroutine thrown into -> RuntimeError\n");
            PRINT_ITEM(coroutine->m_qualname);
            PRINT_NEW_LINE();
#endif
            PyErr_Format(PyExc_RuntimeError,
#if !defined(_PFORGE_FULL_COMPAT)
                         "cannot reuse already awaited compiled_coroutine %S", coroutine->m_qualname
#else
                         "cannot reuse already awaited coroutine"
#endif
            );

            RELEASE_ERROR_OCCURRED_STATE(exception_state);

            return NULL;
        }
#endif
        // Passing exception to publication.
        RESTORE_ERROR_OCCURRED_STATE(tstate, exception_state);

        return NULL;
    } else {
        _Pforge_Generator_add_throw_traceback_frame(tstate, exception_state, coroutine->m_code_object,
                                                    coroutine->m_module);

        // Passing exception to publication.
        RESTORE_ERROR_OCCURRED_STATE(tstate, exception_state);

#if _DEBUG_COROUTINE
        PRINT_COROUTINE_STATUS("Finishing from exception", coroutine);
        PRINT_NEW_LINE();
#endif

        Pforge_MarkCoroutineAsFinished(coroutine);

        return NULL;
    }
}

static PyObject *_Pforge_Coroutine_throw_api(struct Pforge_CoroutineObject *coroutine, PyObject *args) {
    CHECK_OBJECT(coroutine);
    CHECK_OBJECT_DEEP(args);

    PyObject *exception_type;
    PyObject *exception_value = NULL;
    PyTracebackObject *exception_tb = NULL;

    // This takes no references, that is for us to do.
    int res = PyArg_UnpackTuple(args, "throw", 1, 3, &exception_type, &exception_value, &exception_tb);

    if (unlikely(res == 0)) {
        return NULL;
    }

#if _DEBUG_COROUTINE
    PRINT_COROUTINE_STATUS("Enter", coroutine);
    PRINT_EXCEPTION(exception_type, exception_value, exception_tb);
    PRINT_NEW_LINE();
#endif

    PyThreadState *tstate = PyThreadState_GET();

    // Handing ownership of exception over, we need not release it ourselves
    struct Pforge_ExceptionPreservationItem exception_state;
    if (_Pforge_Generator_make_throw_exception_state(tstate, &exception_state, exception_type, exception_value,
                                                     exception_tb) == false) {
        return NULL;
    }

    PyObject *result = _Pforge_Coroutine_throw2(tstate, coroutine, false, &exception_state);

    if (result == NULL) {
        if (HAS_ERROR_OCCURRED(tstate) == false) {
            SET_CURRENT_EXCEPTION_STOP_ITERATION_EMPTY(tstate);
        }
    }

#if _DEBUG_COROUTINE
    PRINT_COROUTINE_STATUS("Leave", coroutine);
    PRINT_EXCEPTION(exception_type, exception_value, exception_tb);
    PRINT_COROUTINE_VALUE("return value", result);
    PRINT_CURRENT_EXCEPTION();
#endif

    return result;
}

static PyObject *Pforge_Coroutine_throw(PyObject *coroutine_obj, PyObject *args) {
    return _Pforge_Coroutine_throw_api((struct Pforge_CoroutineObject *)coroutine_obj, args);
}

static PyObject *Pforge_Coroutine_tp_repr(PyObject *self) {
    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;

    CHECK_OBJECT(self);
    CHECK_OBJECT(coroutine->m_qualname);

    return PyUnicode_FromFormat("<compiled_coroutine object %s at %p>", Pforge_String_AsString(coroutine->m_qualname),
                                coroutine);
}

static int Pforge_Coroutine_tp_traverse(PyObject *self, visitproc visit, void *arg) {
    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;

    CHECK_OBJECT(self);

    // TODO: Identify the impact of not visiting owned objects like module
    Py_VISIT(coroutine->m_yield_from);
    Py_VISIT(coroutine->m_frame);

    for (Py_ssize_t i = 0; i < coroutine->m_closure_given; i++) {
        Py_VISIT(coroutine->m_closure[i]);
    }

#if PYTHON_VERSION >= 0x370
    Py_VISIT(coroutine->m_exc_state.exception_value);
#endif
    Py_VISIT(coroutine->m_returned);

    return 0;
}

static struct Pforge_CoroutineWrapperObject *free_list_coro_wrappers = NULL;
static int free_list_coro_wrappers_count = 0;
PFORGE_FREELIST_DECLARE_LOCK(free_list_coro_wrappers)

static PyObject *Pforge_Coroutine_await(PyObject *self) {
    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;

    CHECK_OBJECT(self);

#if _DEBUG_COROUTINE
    PRINT_COROUTINE_STATUS("Enter", coroutine);
    PRINT_NEW_LINE();
#endif

#if _DEBUG_REFCOUNTS
    count_active_Pforge_CoroutineWrapper_Type += 1;
    count_allocated_Pforge_CoroutineWrapper_Type += 1;
#endif

    struct Pforge_CoroutineWrapperObject *result;

    allocateFromFreeListFixed(free_list_coro_wrappers, struct Pforge_CoroutineWrapperObject,
                              Pforge_CoroutineWrapper_Type);

    if (unlikely(result == NULL)) {
        return NULL;
    }

    result->m_coroutine = coroutine;
    Py_INCREF(result->m_coroutine);

    Pforge_GC_Track(result);

    return (PyObject *)result;
}

#if PYTHON_VERSION >= 0x3a0
static PySendResult _Pforge_Coroutine_am_send(PyObject *self, PyObject *arg, PyObject **result) {
    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;

#if _DEBUG_COROUTINE
    PRINT_COROUTINE_STATUS("Enter", coroutine);
#endif
    *result = NULL;
    PyThreadState *tstate = PyThreadState_GET();

    // We need to transfer ownership of the sent value.
    Py_INCREF(arg);

    struct Pforge_ExceptionPreservationItem exception_state;
    INIT_ERROR_OCCURRED_STATE(&exception_state);

    PySendResult res = _Pforge_Coroutine_sendR(tstate, coroutine, arg, false, &exception_state, result);

#if _DEBUG_COROUTINE
    PRINT_COROUTINE_STATUS("Leave", coroutine);
    PRINT_COROUTINE_VALUE("result", *result);
    PRINT_NEW_LINE();
#endif
    return res;
}
#endif

static void Pforge_Coroutine_tp_finalize(PyObject *self) {
    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;

    if (coroutine->m_status != status_Running) {
        return;
    }

    PyThreadState *tstate = PyThreadState_GET();

    struct Pforge_ExceptionPreservationItem saved_exception_state;
    FETCH_ERROR_OCCURRED_STATE(tstate, &saved_exception_state);

    bool close_result = _Pforge_Coroutine_close(tstate, coroutine);

    if (unlikely(close_result == false)) {
        PyErr_WriteUnraisable((PyObject *)coroutine);
    }

    /* Restore the saved exception if any. */
    RESTORE_ERROR_OCCURRED_STATE(tstate, &saved_exception_state);
}

// Freelist setup
#define MAX_COROUTINE_FREE_LIST_COUNT 100
static struct Pforge_CoroutineObject *free_list_coroutines = NULL;
static int free_list_coroutines_count = 0;
PFORGE_FREELIST_DECLARE_LOCK(free_list_coroutines)

static void Pforge_Coroutine_tp_dealloc(PyObject *self) {
    struct Pforge_CoroutineObject *coroutine = (struct Pforge_CoroutineObject *)self;

#if _DEBUG_REFCOUNTS
    count_active_Pforge_Coroutine_Type -= 1;
    count_released_Pforge_Coroutine_Type += 1;
#endif
    if (coroutine->m_weakrefs != NULL) {
        Pforge_GC_UnTrack(coroutine);

        // TODO: Avoid API call and make this our own function to reuse the
        // pattern of temporarily untracking the value or maybe even to avoid
        // the need to do it. It may also be a lot of work to do that though
        // and maybe having weakrefs is uncommon.
        PyObject_ClearWeakRefs((PyObject *)coroutine);

        Pforge_GC_Track(coroutine);
    }

    if (Pforge_CallFinalizerFromDealloc((PyObject *)coroutine) == false) {
        return;
    }

    // Now it is safe to release references and memory for it.
    Pforge_GC_UnTrack(coroutine);

#if _DEBUG_COROUTINE
    PRINT_COROUTINE_STATUS("Enter", coroutine);
    PRINT_NEW_LINE();
#endif

    Pforge_Coroutine_release_closure(coroutine);

#if PYTHON_VERSION >= 0x370
    Py_XDECREF(coroutine->m_origin);
    coroutine->m_origin = NULL;
#endif

    if (coroutine->m_frame) {
        struct Pforge_FrameObject *release_frame = coroutine->m_frame;
        coroutine->m_frame = NULL;

        Pforge_SetFrameGenerator(release_frame, NULL);
        Pforge_FrameCache_ReleaseGeneratorFrame(release_frame);
    }

    Py_DECREF(coroutine->m_name);
    Py_DECREF(coroutine->m_qualname);

    // TODO: Maybe push this into the freelist code and do
    // it on allocation.
    _PyGC_CLEAR_FINALIZED((PyObject *)coroutine);

    /* Put the object into free list or release to GC */
    releaseToFreeList(free_list_coroutines, coroutine, MAX_COROUTINE_FREE_LIST_COUNT);
}

// TODO: Set "__doc__" automatically for method clones of compiled types from
// the documentation of built-in original type.
static PyMethodDef Pforge_Coroutine_methods[] = {{"send", (PyCFunction)Pforge_Coroutine_send, METH_O, NULL},
                                                 {"throw", (PyCFunction)Pforge_Coroutine_throw, METH_VARARGS, NULL},
                                                 {"close", (PyCFunction)Pforge_Coroutine_close, METH_NOARGS, NULL},
                                                 {NULL}};

// TODO: Set "__doc__" automatically for method clones of compiled types from
// the documentation of built-in original type.
static PyGetSetDef Pforge_Coroutine_tp_getset[] = {
    {(char *)"__name__", Pforge_Coroutine_get_name, Pforge_Coroutine_set_name, NULL},
    {(char *)"__qualname__", Pforge_Coroutine_get_qualname, Pforge_Coroutine_set_qualname, NULL},
    {(char *)"cr_await", Pforge_Coroutine_get_cr_await, NULL, NULL},
    {(char *)"cr_code", Pforge_Coroutine_get_code, Pforge_Coroutine_set_code, NULL},
    {(char *)"cr_frame", Pforge_Coroutine_get_frame, Pforge_Coroutine_set_frame, NULL},

    {NULL}};

static PyMemberDef Pforge_Coroutine_members[] = {
    {(char *)"cr_running", T_BOOL, offsetof(struct Pforge_CoroutineObject, m_running), READONLY},
#if PYTHON_VERSION >= 0x370
    {(char *)"cr_origin", T_OBJECT, offsetof(struct Pforge_CoroutineObject, m_origin), READONLY},

#endif
    {NULL}};

static PyAsyncMethods Pforge_Coroutine_as_async = {
    (unaryfunc)Pforge_Coroutine_await, // am_await
    0,                                 // am_aiter
    0                                  // am_anext
#if PYTHON_VERSION >= 0x3a0
    ,
    (sendfunc)_Pforge_Coroutine_am_send // am_send
#endif

};

PyTypeObject Pforge_Coroutine_Type = {
    PyVarObject_HEAD_INIT(NULL, 0) "compiled_coroutine",                // tp_name
    sizeof(struct Pforge_CoroutineObject),                              // tp_basicsize
    sizeof(struct Pforge_CellObject *),                                 // tp_itemsize
    (destructor)Pforge_Coroutine_tp_dealloc,                            // tp_dealloc
    0,                                                                  // tp_print
    0,                                                                  // tp_getattr
    0,                                                                  // tp_setattr
    &Pforge_Coroutine_as_async,                                         // tp_as_async
    (reprfunc)Pforge_Coroutine_tp_repr,                                 // tp_repr
    0,                                                                  // tp_as_number
    0,                                                                  // tp_as_sequence
    0,                                                                  // tp_as_mapping
    (hashfunc)Pforge_Coroutine_tp_hash,                                 // tp_hash
    0,                                                                  // tp_call
    0,                                                                  // tp_str
    0,                                                                  // tp_getattro (PyObject_GenericGetAttr)
    0,                                                                  // tp_setattro
    0,                                                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_HAVE_FINALIZE, // tp_flags
    0,                                                                  // tp_doc
    (traverseproc)Pforge_Coroutine_tp_traverse,                         // tp_traverse
    0,                                                                  // tp_clear
    0,                                                                  // tp_richcompare
    offsetof(struct Pforge_CoroutineObject, m_weakrefs),                // tp_weaklistoffset
    0,                                                                  // tp_iter
    0,                                                                  // tp_iternext
    Pforge_Coroutine_methods,                                           // tp_methods
    Pforge_Coroutine_members,                                           // tp_members
    Pforge_Coroutine_tp_getset,                                         // tp_getset
    0,                                                                  // tp_base
    0,                                                                  // tp_dict
    0,                                                                  // tp_descr_get
    0,                                                                  // tp_descr_set
    0,                                                                  // tp_dictoffset
    0,                                                                  // tp_init
    0,                                                                  // tp_alloc
    0,                                                                  // tp_new
    0,                                                                  // tp_free
    0,                                                                  // tp_is_gc
    0,                                                                  // tp_bases
    0,                                                                  // tp_mro
    0,                                                                  // tp_cache
    0,                                                                  // tp_subclasses
    0,                                                                  // tp_weaklist
    0,                                                                  // tp_del
    0,                                                                  // tp_version_tag
    (destructor)Pforge_Coroutine_tp_finalize,                           // tp_finalize
};

static void Pforge_CoroutineWrapper_tp_dealloc(PyObject *self) {
    struct Pforge_CoroutineWrapperObject *cw = (struct Pforge_CoroutineWrapperObject *)self;

    Pforge_GC_UnTrack(self);

    assert(Py_REFCNT(cw) == 0);
    Py_SET_REFCNT(cw, 1);

#if _DEBUG_REFCOUNTS
    count_active_Pforge_CoroutineWrapper_Type -= 1;
    count_released_Pforge_CoroutineWrapper_Type += 1;
#endif
    CHECK_OBJECT(cw->m_coroutine);

    Py_DECREF(cw->m_coroutine);
    cw->m_coroutine = NULL;

    assert(Py_REFCNT(cw) == 1);
    Py_SET_REFCNT(cw, 0);

    releaseToFreeList(free_list_coro_wrappers, cw, MAX_COROUTINE_FREE_LIST_COUNT);
}

static PyObject *Pforge_CoroutineWrapper_tp_iternext(PyObject *self) {
    struct Pforge_CoroutineWrapperObject *cw = (struct Pforge_CoroutineWrapperObject *)self;

    CHECK_OBJECT(self);

    return _Pforge_Coroutine_send_api(cw->m_coroutine, Py_None);
}

static int Pforge_CoroutineWrapper_tp_traverse(PyObject *self, visitproc visit, void *arg) {
    struct Pforge_CoroutineWrapperObject *cw = (struct Pforge_CoroutineWrapperObject *)self;

    CHECK_OBJECT(self);

    Py_VISIT((PyObject *)cw->m_coroutine);
    return 0;
}

static PyObject *Pforge_CoroutineWrapper_send(PyObject *cw_obj, PyObject *arg) {
    struct Pforge_CoroutineWrapperObject *cw = (struct Pforge_CoroutineWrapperObject *)cw_obj;
    CHECK_OBJECT(cw);
    CHECK_OBJECT(arg);

    return _Pforge_Coroutine_send_api(cw->m_coroutine, arg);
}

static PyObject *Pforge_CoroutineWrapper_throw(PyObject *cw_obj, PyObject *args) {
    struct Pforge_CoroutineWrapperObject *cw = (struct Pforge_CoroutineWrapperObject *)cw_obj;
    CHECK_OBJECT(cw);
    CHECK_OBJECT_DEEP(args);

    return _Pforge_Coroutine_throw_api(cw->m_coroutine, args);
}

static PyObject *Pforge_CoroutineWrapper_close(PyObject *cw_obj, PyObject *args) {
    struct Pforge_CoroutineWrapperObject *cw = (struct Pforge_CoroutineWrapperObject *)cw_obj;
    CHECK_OBJECT(cw);

    return _Pforge_Coroutine_close_api(cw->m_coroutine, args);
}

static PyObject *Pforge_CoroutineWrapper_tp_repr(PyObject *self) {
    struct Pforge_CoroutineWrapperObject *cw = (struct Pforge_CoroutineWrapperObject *)self;

    CHECK_OBJECT(self);
    CHECK_OBJECT(cw->m_coroutine);
    CHECK_OBJECT(cw->m_coroutine->m_qualname);

    return PyUnicode_FromFormat("<compiled_coroutine_wrapper object %s at %p>",
                                Pforge_String_AsString(cw->m_coroutine->m_qualname), cw);
}

static PyMethodDef Pforge_CoroutineWrapper_methods[] = {
    {"send", (PyCFunction)Pforge_CoroutineWrapper_send, METH_O, NULL},
    {"throw", (PyCFunction)Pforge_CoroutineWrapper_throw, METH_VARARGS, NULL},
    {"close", (PyCFunction)Pforge_CoroutineWrapper_close, METH_NOARGS, NULL},
    {NULL}};

PyTypeObject Pforge_CoroutineWrapper_Type = {
    PyVarObject_HEAD_INIT(NULL, 0) "compiled_coroutine_wrapper",
    sizeof(struct Pforge_CoroutineWrapperObject),      // tp_basicsize
    0,                                                 // tp_itemsize
    (destructor)Pforge_CoroutineWrapper_tp_dealloc,    // tp_dealloc
    0,                                                 // tp_print
    0,                                                 // tp_getattr
    0,                                                 // tp_setattr
    0,                                                 // tp_as_async
    (reprfunc)Pforge_CoroutineWrapper_tp_repr,         // tp_repr
    0,                                                 // tp_as_number
    0,                                                 // tp_as_sequence
    0,                                                 // tp_as_mapping
    0,                                                 // tp_hash
    0,                                                 // tp_call
    0,                                                 // tp_str
    0,                                                 // tp_getattro (PyObject_GenericGetAttr)
    0,                                                 // tp_setattro
    0,                                                 // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,           // tp_flags
    0,                                                 // tp_doc
    (traverseproc)Pforge_CoroutineWrapper_tp_traverse, // tp_traverse
    0,                                                 // tp_clear
    0,                                                 // tp_richcompare
    0,                                                 // tp_weaklistoffset
    0,                                                 // tp_iter (PyObject_SelfIter)
    (iternextfunc)Pforge_CoroutineWrapper_tp_iternext, // tp_iternext
    Pforge_CoroutineWrapper_methods,                   // tp_methods
    0,                                                 // tp_members
    0,                                                 // tp_getset
    0,                                                 // tp_base
    0,                                                 // tp_dict
    0,                                                 // tp_descr_get
    0,                                                 // tp_descr_set
    0,                                                 // tp_dictoffset
    0,                                                 // tp_init
    0,                                                 // tp_alloc
    0,                                                 // tp_new
    0,                                                 // tp_free
};

#if PYTHON_VERSION >= 0x3b0

// Not exported by the core library.
static int Pforge_PyInterpreterFrame_GetLine(_PyInterpreterFrame *frame) {
    // TODO: For Pforge frames there is a better way actually, since
    // we have the line number stored.

    int addr = _PyInterpreterFrame_LASTI(frame) * sizeof(_Py_CODEUNIT);
    return PyCode_Addr2Line(Pforge_InterpreterFrame_GetCodeObject(frame), addr);
}

static PyObject *computeCoroutineOrigin(PyThreadState *tstate, int origin_depth) {
    _PyInterpreterFrame *current_frame = CURRENT_TSTATE_INTERPRETER_FRAME(tstate);

    // Create result tuple with correct size.
    int frame_count = 0;
    _PyInterpreterFrame *frame = current_frame;
    while (frame != NULL && frame_count < origin_depth) {
        frame = frame->previous;
        frame_count += 1;
    }
    PyObject *cr_origin = MAKE_TUPLE_EMPTY_VAR(tstate, frame_count);

    frame = current_frame;
    for (int i = 0; i < frame_count; i++) {
        PyCodeObject *code_object = Pforge_InterpreterFrame_GetCodeObject(frame);

        // TODO: Clarify why Pforge_InterpreterFrame_GetCodeObject does return
        // "None" sometimes.
        if (code_object == (PyCodeObject *)Py_None) {
            continue;
        }

        int line = Pforge_PyInterpreterFrame_GetLine(frame);

        assert(code_object->co_filename != NULL);
        assert(code_object->co_name != NULL);
        PyObject *frame_info = Py_BuildValue("OiO", code_object->co_filename, line, code_object->co_name);
        assert(frame_info);

        PyTuple_SET_ITEM(cr_origin, i, frame_info);
        frame = frame->previous;
    }

    return cr_origin;
}

#elif PYTHON_VERSION >= 0x370
static PyObject *computeCoroutineOrigin(PyThreadState *tstate, int origin_depth) {
    PyFrameObject *frame = PyEval_GetFrame();

    int frame_count = 0;

    while (frame != NULL && frame_count < origin_depth) {
        frame = frame->f_back;
        frame_count += 1;
    }

    PyObject *cr_origin = MAKE_TUPLE_EMPTY_VAR(tstate, frame_count);

    frame = PyEval_GetFrame();

    for (int i = 0; i < frame_count; i++) {
        PyCodeObject *code_object = Pforge_Frame_GetCodeObject(frame);

        // TODO: See 3.8+ code above, this ought not to be done.
        if (code_object == (PyCodeObject *)Py_None) {
            continue;
        }

        PyObject *filename = code_object->co_filename;
        CHECK_OBJECT(filename);
        PyObject *co_name = frame->f_code->co_name;
        CHECK_OBJECT(co_name);

        PyObject *frame_info = Py_BuildValue("OiO", filename, PyFrame_GetLineNumber(frame), co_name);

        assert(frame_info);

        PyTuple_SET_ITEM(cr_origin, i, frame_info);

        frame = frame->f_back;
    }

    return cr_origin;
}
#endif

PyObject *Pforge_Coroutine_New(PyThreadState *tstate, coroutine_code code, PyObject *module, PyObject *name,
                               PyObject *qualname, PyCodeObject *code_object, struct Pforge_CellObject **closure,
                               Py_ssize_t closure_given, Py_ssize_t heap_storage_size) {
#if _DEBUG_REFCOUNTS
    count_active_Pforge_Coroutine_Type += 1;
    count_allocated_Pforge_Coroutine_Type += 1;
#endif

    struct Pforge_CoroutineObject *result;

    // TODO: Change the var part of the type to 1 maybe
    Py_ssize_t full_size = closure_given + (heap_storage_size + sizeof(void *) - 1) / sizeof(void *);

    // Macro to assign result memory from GC or free list.
    allocateFromFreeList(free_list_coroutines, struct Pforge_CoroutineObject, Pforge_Coroutine_Type, full_size);

    // For quicker access of generator heap.
    result->m_heap_storage = &result->m_closure[closure_given];

    result->m_code = (void *)code;

    CHECK_OBJECT(module);
    result->m_module = module;

    CHECK_OBJECT(name);
    result->m_name = name;
    Py_INCREF(name);

    // The "qualname" defaults to NULL for most compact C code.
    if (qualname == NULL) {
        qualname = name;
    }
    CHECK_OBJECT(qualname);

    result->m_qualname = qualname;
    Py_INCREF(qualname);

    result->m_yield_from = NULL;

    memcpy(&result->m_closure[0], closure, closure_given * sizeof(struct Pforge_CellObject *));
    result->m_closure_given = closure_given;

    result->m_weakrefs = NULL;

    result->m_status = status_Unused;
    result->m_running = 0;
    result->m_awaiting = false;

#if PYTHON_VERSION == 0x3c0
    result->m_closed_before_start = false;
#endif

    result->m_yield_return_index = 0;

    result->m_returned = NULL;

    result->m_frame = NULL;
    result->m_code_object = code_object;

    result->m_resume_frame = NULL;

#if PYTHON_VERSION >= 0x370
    int origin_depth = tstate->coroutine_origin_tracking_depth;

    if (origin_depth == 0) {
        result->m_origin = NULL;
    } else {
        result->m_origin = computeCoroutineOrigin(tstate, origin_depth);
    }
#endif

#if PYTHON_VERSION >= 0x370
    result->m_exc_state = Pforge_ExceptionStackItem_Empty;
#endif

    static long Pforge_Coroutine_counter = 0;
    result->m_counter = Pforge_Coroutine_counter++;

    Pforge_GC_Track(result);
    return (PyObject *)result;
}

static inline PyCodeObject *_Pforge_PyGen_GetCode(PyGenObject *gen) {
#if PYTHON_VERSION < 0x3c0
    return (PyCodeObject *)gen->gi_code;
#elif PYTHON_VERSION < 0x3d0
    _PyInterpreterFrame *frame = (_PyInterpreterFrame *)(gen->gi_iframe);
    return frame->f_code;
#elif PYTHON_VERSION < 0x3e0
    _PyInterpreterFrame *frame = (_PyInterpreterFrame *)(gen->gi_iframe);
    return (PyCodeObject *)frame->f_executable;
#else
    return Pforge_InterpreterFrame_GetCodeObject(&gen->gi_iframe);
#endif
}

static int gen_is_coroutine(PyObject *obj) {
    if (PyGen_CheckExact(obj)) {
        PyCodeObject *code = _Pforge_PyGen_GetCode((PyGenObject *)obj);

        if (code->co_flags & CO_ITERABLE_COROUTINE) {
            return 1;
        }
    }

    return 0;
}

static PyObject *Pforge_GetAwaitableIter(PyThreadState *tstate, PyObject *value) {
    CHECK_OBJECT(value);

#if _DEBUG_COROUTINE
    PRINT_STRING("Pforge_GetAwaitableIter: Enter ");
    PRINT_ITEM(value);
    PRINT_NEW_LINE();
#endif

    unaryfunc getter_func = NULL;

    if (PyCoro_CheckExact(value) || gen_is_coroutine(value)) {
        Py_INCREF(value);
        return value;
    }

    if (Py_TYPE(value)->tp_as_async != NULL) {
        getter_func = Py_TYPE(value)->tp_as_async->am_await;
    }

    if (getter_func != NULL) {
        PyObject *result = (*getter_func)(value);

        if (result != NULL) {
            if (unlikely(PyCoro_CheckExact(result) || gen_is_coroutine(result) || Pforge_Coroutine_Check(result))) {
#if PYTHON_VERSION >= 0x3f0
                SET_CURRENT_EXCEPTION_TYPE_COMPLAINT_QUALIFIED("%s.__await__() must return an iterator, not coroutine",
                                                               value);
#else
                SET_CURRENT_EXCEPTION_TYPE0_STR(tstate, PyExc_TypeError, "__await__() returned a coroutine");
#endif

                Py_DECREF(result);

                return NULL;
            }

            if (unlikely(!HAS_ITERNEXT(result))) {
#if PYTHON_VERSION >= 0x3f0
                SET_CURRENT_EXCEPTION_TYPE_COMPLAINT_QUALIFIED2("%s.__await__() must return an iterator, not %s", value,
                                                                result);
#else
                SET_CURRENT_EXCEPTION_TYPE_COMPLAINT("__await__() returned non-iterator of type '%s'", result);
#endif

                Py_DECREF(result);

                return NULL;
            }
        }

        return result;
    }

#if PYTHON_VERSION >= 0x3f0 || (PYTHON_VERSION >= 0x3e0 && defined(_PFORGE_FULL_COMPAT))
    SET_CURRENT_EXCEPTION_TYPE0_FORMAT1(PyExc_TypeError, "'%.100s' object can't be awaited", Py_TYPE(value)->tp_name);
#else
    SET_CURRENT_EXCEPTION_TYPE_COMPLAINT("object %s can't be used in 'await' expression", value);
#endif

    return NULL;
}

#if PYTHON_VERSION >= 0x366
static void FORMAT_AWAIT_ERROR(PyThreadState *tstate, PyObject *value, int await_kind) {
    CHECK_OBJECT(value);

    if (await_kind == await_enter) {
        PyErr_Format(PyExc_TypeError,
                     "'async with' received an object from __aenter__ that does not implement __await__: %s",
                     Py_TYPE(value)->tp_name);
    } else if (await_kind == await_exit) {
        PyErr_Format(PyExc_TypeError,
                     "'async with' received an object from __aexit__ that does not implement __await__: %s",
                     Py_TYPE(value)->tp_name);
    }

    assert(HAS_ERROR_OCCURRED(tstate));
}
#endif

PyObject *ASYNC_AWAIT(PyThreadState *tstate, PyObject *awaitable, int await_kind) {
    CHECK_OBJECT(awaitable);

#if _DEBUG_COROUTINE
    PRINT_STRING("ASYNC_AWAIT: Enter for awaitable ");
    PRINT_STRING(await_kind == await_enter ? "enter" : "exit");
    PRINT_STRING(" ");
    PRINT_ITEM(awaitable);
    PRINT_NEW_LINE();
#endif

    PyObject *awaitable_iter = Pforge_GetAwaitableIter(tstate, awaitable);

    if (unlikely(awaitable_iter == NULL)) {
#if PYTHON_VERSION >= 0x366
        FORMAT_AWAIT_ERROR(tstate, awaitable, await_kind);
#endif
        return NULL;
    }

#if PYTHON_VERSION >= 0x352 || !defined(_PFORGE_FULL_COMPAT)
    /* This check got added in Python 3.5.2 only. It's good to do it, but
     * not fully compatible, therefore guard it.
     */

    if (Pforge_Coroutine_Check(awaitable)) {
        struct Pforge_CoroutineObject *awaited_coroutine = (struct Pforge_CoroutineObject *)awaitable;

        if (awaited_coroutine->m_awaiting) {
            Py_DECREF(awaitable_iter);

            SET_CURRENT_EXCEPTION_TYPE0_STR(tstate, PyExc_RuntimeError, "coroutine is being awaited already");

            return NULL;
        }
    }
#endif

#if _DEBUG_COROUTINE
    PRINT_STRING("ASYNC_AWAIT: Result ");
    PRINT_ITEM(awaitable);
    PRINT_NEW_LINE();
#endif

    return awaitable_iter;
}

#if PYTHON_VERSION >= 0x352

/* Our "aiter" wrapper clone */
struct Pforge_AIterWrapper {
    /* Python object folklore: */
    PyObject_HEAD

        PyObject *aw_aiter;
};

static PyObject *Pforge_AIterWrapper_tp_repr(PyObject *self) {
    struct Pforge_AIterWrapper *aw = (struct Pforge_AIterWrapper *)self;

    return PyUnicode_FromFormat("<compiled_aiter_wrapper object of %R at %p>", aw->aw_aiter, aw);
}

static PyObject *Pforge_AIterWrapper_iternext(PyObject *self) {
    struct Pforge_AIterWrapper *aw = (struct Pforge_AIterWrapper *)self;

    CHECK_OBJECT(aw);

    PyThreadState *tstate = PyThreadState_GET();

#if PYTHON_VERSION < 0x360
    SET_CURRENT_EXCEPTION_TYPE0_VALUE0(tstate, PyExc_StopIteration, aw->aw_aiter);
#elif PYTHON_VERSION < 0x3c0
    if (!PyTuple_Check(aw->aw_aiter) && !PyExceptionInstance_Check(aw->aw_aiter)) {
        SET_CURRENT_EXCEPTION_TYPE0_VALUE0(tstate, PyExc_StopIteration, aw->aw_aiter);
    } else {
        PyObject *result = CALL_FUNCTION_WITH_SINGLE_ARG(tstate, PyExc_StopIteration, aw->aw_aiter);

        if (unlikely(result == NULL)) {
            return NULL;
        }

        struct Pforge_ExceptionPreservationItem exception_state = {_Py_NewRef(PyExc_StopIteration), result, NULL};

        RESTORE_ERROR_OCCURRED_STATE(tstate, &exception_state);
    }
#else
    struct Pforge_ExceptionPreservationItem exception_state = {Pforge_CreateStopIteration(tstate, aw->aw_aiter)};

    RESTORE_ERROR_OCCURRED_STATE(tstate, &exception_state);
#endif

    return NULL;
}

static int Pforge_AIterWrapper_traverse(PyObject *self, visitproc visit, void *arg) {
    struct Pforge_AIterWrapper *aw = (struct Pforge_AIterWrapper *)self;

    CHECK_OBJECT(aw);

    Py_VISIT((PyObject *)aw->aw_aiter);
    return 0;
}

static struct Pforge_AIterWrapper *free_list_coroutine_aiter_wrappers = NULL;
static int free_list_coroutine_aiter_wrappers_count = 0;
PFORGE_FREELIST_DECLARE_LOCK(free_list_coroutine_aiter_wrappers)

static void Pforge_AIterWrapper_dealloc(PyObject *self) {
    struct Pforge_AIterWrapper *aw = (struct Pforge_AIterWrapper *)self;

#if _DEBUG_REFCOUNTS
    count_active_Pforge_AIterWrapper_Type -= 1;
    count_released_Pforge_AIterWrapper_Type += 1;
#endif

    Pforge_GC_UnTrack((PyObject *)aw);

    CHECK_OBJECT(aw->aw_aiter);
    Py_DECREF(aw->aw_aiter);

    /* Put the object into free list or release to GC */
    releaseToFreeList(free_list_coroutine_aiter_wrappers, aw, MAX_COROUTINE_FREE_LIST_COUNT);
}

static PyAsyncMethods Pforge_AIterWrapper_as_async = {
    0, // am_await (PyObject_SelfIter)
    0, // am_aiter
    0  // am_anext
};

PyTypeObject Pforge_AIterWrapper_Type = {
    PyVarObject_HEAD_INIT(NULL, 0) "compiled_aiter_wrapper",
    sizeof(struct Pforge_AIterWrapper),                          // tp_basicsize
    0,                                                           // tp_itemsize
    (destructor)Pforge_AIterWrapper_dealloc,                     // tp_dealloc
    0,                                                           // tp_print
    0,                                                           // tp_getattr
    0,                                                           // tp_setattr
    &Pforge_AIterWrapper_as_async,                               // tp_as_async
    (reprfunc)Pforge_AIterWrapper_tp_repr,                       // tp_repr
    0,                                                           // tp_as_number
    0,                                                           // tp_as_sequence
    0,                                                           // tp_as_mapping
    0,                                                           // tp_hash
    0,                                                           // tp_call
    0,                                                           // tp_str
    0,                                                           // tp_getattro (PyObject_GenericGetAttr)
    0,                                                           // tp_setattro
    0,                                                           // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                     // tp_flags
    "A wrapper object for '__aiter__' backwards compatibility.", // tp_doc
    (traverseproc)Pforge_AIterWrapper_traverse,                  // tp_traverse
    0,                                                           // tp_clear
    0,                                                           // tp_richcompare
    0,                                                           // tp_weaklistoffset
    0,                                                           // tp_iter (PyObject_SelfIter)
    (iternextfunc)Pforge_AIterWrapper_iternext,                  // tp_iternext
    0,                                                           // tp_methods
    0,                                                           // tp_members
    0,                                                           // tp_getset
    0,                                                           // tp_base
    0,                                                           // tp_dict
    0,                                                           // tp_descr_get
    0,                                                           // tp_descr_set
    0,                                                           // tp_dictoffset
    0,                                                           // tp_init
    0,                                                           // tp_alloc
    0,                                                           // tp_new
    0,                                                           // tp_free
};

static PyObject *Pforge_AIterWrapper_New(PyObject *aiter) {
    CHECK_OBJECT(aiter);

#if _DEBUG_REFCOUNTS
    count_active_Pforge_AIterWrapper_Type += 1;
    count_allocated_Pforge_AIterWrapper_Type += 1;
#endif
    struct Pforge_AIterWrapper *result;

    allocateFromFreeListFixed(free_list_coroutine_aiter_wrappers, struct Pforge_AIterWrapper, Pforge_AIterWrapper_Type);

    CHECK_OBJECT(aiter);

    Py_INCREF(aiter);
    result->aw_aiter = aiter;

    Pforge_GC_Track(result);
    return (PyObject *)result;
}

#endif

PyObject *ASYNC_MAKE_ITERATOR(PyThreadState *tstate, PyObject *value) {
    CHECK_OBJECT(value);

#if _DEBUG_COROUTINE
    PRINT_STRING("AITER entry:");
    PRINT_ITEM(value);

    PRINT_NEW_LINE();
#endif

    unaryfunc getter_func = NULL;

    if (Py_TYPE(value)->tp_as_async) {
        getter_func = Py_TYPE(value)->tp_as_async->am_aiter;
    }

    if (unlikely(getter_func == NULL)) {
        PyErr_Format(PyExc_TypeError, "'async for' requires an object with __aiter__ method, got %s",
                     Py_TYPE(value)->tp_name);

        return NULL;
    }

    PyObject *iter = (*getter_func)(value);

    if (unlikely(iter == NULL)) {
        return NULL;
    }

#if PYTHON_VERSION >= 0x370
    if (unlikely(Py_TYPE(iter)->tp_as_async == NULL || Py_TYPE(iter)->tp_as_async->am_anext == NULL)) {
        PyErr_Format(PyExc_TypeError,
                     "'async for' received an object from __aiter__ that does not implement __anext__: %s",
                     Py_TYPE(iter)->tp_name);

        Py_DECREF(iter);
        return NULL;
    }
#endif

#if PYTHON_VERSION >= 0x352
    /* Starting with Python 3.5.2 it is acceptable to return an async iterator
     * directly, instead of an awaitable.
     */
    if (Py_TYPE(iter)->tp_as_async != NULL && Py_TYPE(iter)->tp_as_async->am_anext != NULL) {
        PyObject *wrapper = Pforge_AIterWrapper_New(iter);
        Py_DECREF(iter);

        iter = wrapper;
    }
#endif

    PyObject *awaitable_iter = Pforge_GetAwaitableIter(tstate, iter);

    if (unlikely(awaitable_iter == NULL)) {
#if PYTHON_VERSION >= 0x360
        _PyErr_FormatFromCause(
#else
        PyErr_Format(
#endif
            PyExc_TypeError, "'async for' received an invalid object from __aiter__: %s", Py_TYPE(iter)->tp_name);

        Py_DECREF(iter);

        return NULL;
    }

    Py_DECREF(iter);

    return awaitable_iter;
}

PyObject *ASYNC_ITERATOR_NEXT(PyThreadState *tstate, PyObject *value) {
    CHECK_OBJECT(value);

#if _DEBUG_COROUTINE
    PRINT_STRING("ANEXT entry:");
    PRINT_ITEM(value);

    PRINT_NEW_LINE();
#endif

    unaryfunc getter_func = NULL;

    if (Py_TYPE(value)->tp_as_async) {
        getter_func = Py_TYPE(value)->tp_as_async->am_anext;
    }

    if (unlikely(getter_func == NULL)) {
        SET_CURRENT_EXCEPTION_TYPE_COMPLAINT("'async for' requires an iterator with __anext__ method, got %s", value);

        return NULL;
    }

    PyObject *next_value = (*getter_func)(value);

    if (unlikely(next_value == NULL)) {
        return NULL;
    }

    PyObject *awaitable_iter = Pforge_GetAwaitableIter(tstate, next_value);

    if (unlikely(awaitable_iter == NULL)) {
#if PYTHON_VERSION >= 0x360
        _PyErr_FormatFromCause(
#else
        PyErr_Format(
#endif
            PyExc_TypeError, "'async for' received an invalid object from __anext__: %s", Py_TYPE(next_value)->tp_name);

        Py_DECREF(next_value);

        return NULL;
    }

    Py_DECREF(next_value);

    return awaitable_iter;
}

static void _initCompiledCoroutineTypes(void) {
    Pforge_PyType_Ready(&Pforge_Coroutine_Type, // type
                        &PyCoro_Type,           // base
                        true,                   // generic_get_attr
                        false,                  // generic_set_attr
                        false,                  // self_iter
                        false,                  // await_self_iter
                        false                   // await_self_aiter
    );

    // Be a paranoid subtype of uncompiled function, we want nothing shared.
    assert(Pforge_Coroutine_Type.tp_doc != PyCoro_Type.tp_doc || PyCoro_Type.tp_doc == NULL);
    assert(Pforge_Coroutine_Type.tp_traverse != PyCoro_Type.tp_traverse);
    assert(Pforge_Coroutine_Type.tp_clear != PyCoro_Type.tp_clear || PyCoro_Type.tp_clear == NULL);
    assert(Pforge_Coroutine_Type.tp_richcompare != PyCoro_Type.tp_richcompare || PyCoro_Type.tp_richcompare == NULL);
    assert(Pforge_Coroutine_Type.tp_weaklistoffset != PyCoro_Type.tp_weaklistoffset);
    assert(Pforge_Coroutine_Type.tp_iter != PyCoro_Type.tp_iter || PyCoro_Type.tp_iter == NULL);
    assert(Pforge_Coroutine_Type.tp_iternext != PyCoro_Type.tp_iternext || PyCoro_Type.tp_iternext == NULL);
    assert(Pforge_Coroutine_Type.tp_as_async != PyCoro_Type.tp_as_async || PyCoro_Type.tp_as_async == NULL);
    assert(Pforge_Coroutine_Type.tp_methods != PyCoro_Type.tp_methods);
    assert(Pforge_Coroutine_Type.tp_members != PyCoro_Type.tp_members);
    assert(Pforge_Coroutine_Type.tp_getset != PyCoro_Type.tp_getset);
    assert(Pforge_Coroutine_Type.tp_dict != PyCoro_Type.tp_dict);
    assert(Pforge_Coroutine_Type.tp_descr_get != PyCoro_Type.tp_descr_get || PyCoro_Type.tp_descr_get == NULL);

    assert(Pforge_Coroutine_Type.tp_descr_set != PyCoro_Type.tp_descr_set || PyCoro_Type.tp_descr_set == NULL);
    assert(Pforge_Coroutine_Type.tp_dictoffset != PyCoro_Type.tp_dictoffset || PyCoro_Type.tp_dictoffset == 0);
    // TODO: These get changed and into the same thing, not sure what to compare against, project something
    // assert(Pforge_Coroutine_Type.tp_init != PyCoro_Type.tp_init || PyCoro_Type.tp_init == NULL);
    // assert(Pforge_Coroutine_Type.tp_alloc != PyCoro_Type.tp_alloc || PyCoro_Type.tp_alloc == NULL);
    // assert(Pforge_Coroutine_Type.tp_new != PyCoro_Type.tp_new || PyCoro_Type.tp_new == NULL);
    // assert(Pforge_Coroutine_Type.tp_free != PyCoro_Type.tp_free || PyCoro_Type.tp_free == NULL);
    assert(Pforge_Coroutine_Type.tp_bases != PyCoro_Type.tp_bases);
    assert(Pforge_Coroutine_Type.tp_mro != PyCoro_Type.tp_mro);
    assert(Pforge_Coroutine_Type.tp_cache != PyCoro_Type.tp_cache || PyCoro_Type.tp_cache == NULL);
    assert(Pforge_Coroutine_Type.tp_subclasses != PyCoro_Type.tp_subclasses || PyCoro_Type.tp_cache == NULL);
    assert(Pforge_Coroutine_Type.tp_weaklist != PyCoro_Type.tp_weaklist);
    assert(Pforge_Coroutine_Type.tp_del != PyCoro_Type.tp_del || PyCoro_Type.tp_del == NULL);
    assert(Pforge_Coroutine_Type.tp_finalize != PyCoro_Type.tp_finalize || PyCoro_Type.tp_finalize == NULL);

    Pforge_PyType_Ready(&Pforge_CoroutineWrapper_Type, // type
                        NULL,                          // base
                        true,                          // generic_get_attr
                        false,                         // generic_set_attr
                        true,                          // self_iter
                        false,                         // await_self_iter
                        false                          // await_self_aiter
    );

#if PYTHON_VERSION >= 0x352
    Pforge_PyType_Ready(&Pforge_AIterWrapper_Type, // type
                        NULL,                      // base
                        true,                      // generic_get_attr
                        false,                     // generic_set_attr
                        true,                      // self_iter
                        true,                      // await_self_iter
                        false                      // await_self_aiter
    );
#endif
}

// Chain asyncgen code to coroutine and generator code, as it uses same functions,
// and then we can have some things static if both are in the same compilation unit.

#if PYTHON_VERSION >= 0x360
#include "CompiledAsyncgenType.c"
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
