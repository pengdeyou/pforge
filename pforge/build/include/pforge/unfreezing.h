//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_UNFREEZING_H__
#define __PFORGE_UNFREEZING_H__

#ifdef __IDE_ONLY__
#include "pforge/prelude.h"
#endif

#ifndef PFORGE_LOADER_NAME_MAX_LEN
#define PFORGE_LOADER_NAME_MAX_LEN 2048
#endif

/* Modes for loading modules, can be compiled, external shared library, or
 * bytecode. */
#define PFORGE_COMPILED_MODULE 0
#define PFORGE_EXTENSION_MODULE_FLAG 1
#define PFORGE_PACKAGE_FLAG 2
#define PFORGE_BYTECODE_FLAG 4

#define PFORGE_ABORT_MODULE_FLAG 8

#define PFORGE_INTERNAL_MODULE_FLAG 16

#define PFORGE_PERFECT_SUPPORTED_FLAG 32

#define PFORGE_MAIN_MODULE_FLAG 128

#if _PFORGE_STANDALONE_MODE && !defined(_PFORGE_DEPLOYMENT_MODE) &&                                                    \
    !defined(_PFORGE_NO_DEPLOYMENT_EXCLUDED_MODULE_USAGE)
#define PFORGE_EXCLUDED_MODULE_FLAG 64
#endif

struct Pforge_MetaPathBasedLoaderEntry;

typedef PyObject *(*module_init_func)(PyThreadState *tstate, PyObject *module);

#if PYTHON_VERSION >= 0x370 && _PFORGE_EXE_MODE && !_PFORGE_STANDALONE_MODE &&                                         \
    defined(_PFORGE_FILE_REFERENCE_ORIGINAL_MODE)
#define _PFORGE_FREEZER_HAS_FILE_PATH
#endif

struct Pforge_MetaPathBasedLoaderEntry {
    // The module name data, optional to be overloaded by m_get_name to
    // resolve into runtime packages.
    char const *m_name;
    void (*m_get_name)(char *buffer, size_t buffer_size, char const *name);

    // Optional function to check if a given module name matches this entry.
    // When NULL, the name is compared against the runtime name.
    bool (*m_compare_name)(char const *name, char const *m_name);

    // Optional function returning a display name for this entry, used for
    // error messages and debug output. When NULL, the runtime name is used.
    char const *(*m_get_display_name)(void);

    // Optional entry of the "preLoad" code to be executed before this module,
    // or NULL. This avoids a name based connection to that trigger module.
    struct Pforge_MetaPathBasedLoaderEntry const *m_pre_load;

    // Optional entry of the "postLoad" code to be executed after this module,
    // or NULL. This avoids a name based connection to that trigger module.
    struct Pforge_MetaPathBasedLoaderEntry const *m_post_load;

    // Optional entry of the parent package, or NULL for top level modules.
    // This avoids a name based connection for "iter_modules".
    struct Pforge_MetaPathBasedLoaderEntry const *m_parent;

    // Entry function if compiled module, otherwise NULL.
    module_init_func python_init_func;

    // Function to import this module directly, or NULL. A NULL module means
    // the module is created and run, a provided module is run and tracked.
    // Returns a new reference, or NULL with an exception set.
    PyObject *(*m_import_module)(PyThreadState *tstate, struct Pforge_MetaPathBasedLoaderEntry const *entry,
                                 PyObject *module);

    // Optional load state slots of the entry importer, or NULL. The module
    // slot keeps a reference to the loaded module, so it cannot be unloaded,
    // the owner slot holds the thread state that currently loads the module.
    PyObject **m_module_state;
    PyThreadState **m_owner_state;

    // For bytecode modules, start and size inside the constants blob.
    int bytecode_index;
    int bytecode_size;

    // Flags: Indicators if this is compiled, bytecode or shared library.
    int flags;

    // For accelerated mode, we need to be able to tell where the module "__file__"
    // lives, so we can resolve resource reader paths, not relative to the binary
    // but to code location without loading it.
#if defined(_PFORGE_FREEZER_HAS_FILE_PATH)
#if defined _WIN32
    wchar_t const *file_path;
#else
    char const *file_path;
#endif
#endif
};

// Direct entries for modules executed by the startup code. They are NULL if
// the module is not included, which standalone rejects, while accelerated mode
// falls back to a name based import.
extern struct Pforge_MetaPathBasedLoaderEntry const *const Pforge_MainModuleEntry;
extern struct Pforge_MetaPathBasedLoaderEntry const *const Pforge_ParentsMainModuleEntry;
extern struct Pforge_MetaPathBasedLoaderEntry const *const Pforge_ForkserverModuleEntry;
extern struct Pforge_MetaPathBasedLoaderEntry const *const Pforge_ResourceTrackerModuleEntry;
extern struct Pforge_MetaPathBasedLoaderEntry const *const Pforge_SemaphoreTrackerModuleEntry;
extern struct Pforge_MetaPathBasedLoaderEntry const *const Pforge_JoblibPopenLokyWin32ModuleEntry;
extern struct Pforge_MetaPathBasedLoaderEntry const *const Pforge_JoblibPopenLokyPosixModuleEntry;
extern struct Pforge_MetaPathBasedLoaderEntry const *const Pforge_JoblibResourceTrackerModuleEntry;
extern struct Pforge_MetaPathBasedLoaderEntry const *const Pforge_AnyioToProcessModuleEntry;

/* Import a module with explicit load state slots. A NULL module means the
 * module is created and run, a provided module is run and tracked. Returns a
 * new reference, or NULL with an exception set.
 */
extern PyObject *IMPORT_EMBEDDED_MODULE_STATE(PyThreadState *tstate,
                                              struct Pforge_MetaPathBasedLoaderEntry const *entry,
                                              PyObject **module_state, PyThreadState **owner_state, PyObject *module);

// Import a compiled module through its loader entry with explicit load state
// slots. A NULL module means the module is created and run, a provided module
// is run and tracked. Returns a new reference, or NULL with an exception set.
extern PyObject *IMPORT_COMPILED_MODULE(PyThreadState *tstate, struct Pforge_MetaPathBasedLoaderEntry const *entry,
                                        PyObject **module_state, PyThreadState **owner_state, PyObject *module);

/* Import a module through its loader entry, using the load state slots of the
 * entry. A NULL module means the module is created and run, a provided module
 * is run and tracked. Returns a new reference, or NULL with an exception set.
 */
extern PyObject *IMPORT_EMBEDDED_MODULE_ENTRY(PyThreadState *tstate,
                                              struct Pforge_MetaPathBasedLoaderEntry const *entry, PyObject *module);

/* Import a hard module through its loader entry, aborting on failure like the
 * "IMPORT_HARD_*" helpers do, for guaranteed imports that do not check.
 */
extern PyObject *Pforge_ImportHardModuleEntry(PyThreadState *tstate,
                                              struct Pforge_MetaPathBasedLoaderEntry const *entry);
/* Load the pre- or post-load trigger module of an entry, if any. */
extern void Pforge_LoadTriggeredModule(PyThreadState *tstate,
                                       struct Pforge_MetaPathBasedLoaderEntry const *trigger_entry);

/* For embedded modules, register the meta path based loader. Used by main
 * program/package only.
 */
extern void registerMetaPathBasedLoader(struct Pforge_MetaPathBasedLoaderEntry **loader_entries,
                                        unsigned char **bytecode_data, int entry_count);

/* Produce the runtime name of a loader entry into a buffer. When "m_get_name"
 * is non-NULL it is invoked with "m_name"; otherwise "m_name" is copied directly.
 */
extern void Pforge_LoaderEntryName(struct Pforge_MetaPathBasedLoaderEntry const *entry, char *buffer,
                                   size_t buffer_size);

/* Check if a module name matches a loader entry. When "m_compare_name" is
 * non-NULL it is invoked with "m_name"; otherwise the runtime name is produced
 * via "Pforge_LoaderEntryName" and compared with "strcmp".
 */
extern bool Pforge_LoaderEntryCompareName(struct Pforge_MetaPathBasedLoaderEntry const *entry, char const *name);

// For module mode, embedded modules may have to be shifted to below the
// namespace they are loaded into.
#if _PFORGE_MODULE_MODE
extern char const *getMetaPathBasedLoaderModuleRoot(void);
extern void updateMetaPathBasedLoaderModuleRoot(char const *module_root_name);
extern void getModuleNameWithPackageLoadedPrefix(char *buffer, size_t buffer_size, char const *name);
#endif

/* Create a loader object responsible for a package. */
extern PyObject *Pforge_Loader_New(struct Pforge_MetaPathBasedLoaderEntry const *entry);

// Create a distribution object from the given metadata.
extern PyObject *Pforge_Distribution_New(PyThreadState *tstate, PyObject *name);

// Check if we provide a distribution object ourselves.
extern bool Pforge_DistributionNext(Py_ssize_t *pos, PyObject **distribution_name_ptr);

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
