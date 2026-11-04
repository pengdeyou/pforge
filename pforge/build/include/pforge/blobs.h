//     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file

#pragma once
#ifndef __PFORGE_BLOBS_H__
#define __PFORGE_BLOBS_H__

/** Declaration of binary blobs.
 *
 * There are multiple ways, the blobs are accessed, and their
 * definition depends on how that is done.
 *
 * It could be a Windows resource, then it must be a pointer. If it's defined
 * externally in a C file, or at link time with "ld", it must be an array. This
 * hides these facts.
 *
 */

#include "pforge/defines.h"

#if defined(_PFORGE_CONSTANTS_FROM_INCBIN) || defined(_PFORGE_CONSTANTS_FROM_C23_EMBED)

#ifdef __cplusplus
#define PFORGE_CONSTANTS_EXTERN_C_START extern "C" {
#define PFORGE_CONSTANTS_EXTERN_C_END }
#else
#define PFORGE_CONSTANTS_EXTERN_C_START
#define PFORGE_CONSTANTS_EXTERN_C_END
#endif

#define PFORGE_DECLARE_CONSTANT_BLOB(blob_name, blob_symbol_name, modifier)                                            \
    PFORGE_CONSTANTS_EXTERN_C_START                                                                                    \
    extern unsigned modifier char *get##blob_symbol_name##Data(void);                                                  \
    PFORGE_CONSTANTS_EXTERN_C_END

#elif defined(_PFORGE_CONSTANTS_FROM_MACOS_SECTION)

#include <assert.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>
#include <string.h>

#ifdef __LP64__
#define mach_header_arch mach_header_64
#else
#define mach_header_arch mach_header
#endif

#ifdef __cplusplus
#define PFORGE_CONSTANTS_EXTERN_C_START extern "C" {
#define PFORGE_CONSTANTS_EXTERN_C_END }
#else
#define PFORGE_CONSTANTS_EXTERN_C_START
#define PFORGE_CONSTANTS_EXTERN_C_END
#endif

// Helper to find the mach header of the current module or executable
PFORGE_MAY_BE_UNUSED static inline const struct mach_header_arch *_getPforgeMachHeader(void) {
#if _PFORGE_EXE_MODE
    return &_mh_execute_header;
#else
    Dl_info where;
    int res = dladdr((void *)_getPforgeMachHeader, &where);
    assert(res != 0);

    char const *dll_filename = where.dli_fname;
    unsigned long image_count = _dyld_image_count();

    for (unsigned long i = 0; i < image_count; i++) {
        struct mach_header const *header = _dyld_get_image_header(i);
        if (header == NULL) {
            continue;
        }
        if (strcmp(dll_filename, _dyld_get_image_name(i)) == 0) {
            return (const struct mach_header_arch *)header;
        }
    }
    return NULL;
#endif
}

#define PFORGE_DECLARE_CONSTANT_BLOB(blob_name, blob_symbol_name, modifier)                                            \
    PFORGE_CONSTANTS_EXTERN_C_START                                                                                    \
    PFORGE_MAY_BE_UNUSED static inline unsigned modifier char *get##blob_symbol_name##Data(void) {                     \
        const struct mach_header_arch *header = _getPforgeMachHeader();                                                \
        unsigned long size;                                                                                            \
        return (unsigned modifier char *)getsectiondata(header, #blob_name, #blob_name, &size);                        \
    }                                                                                                                  \
    PFORGE_CONSTANTS_EXTERN_C_END

#else

#ifdef __cplusplus
#define PFORGE_CONSTANTS_EXTERN_C_START extern "C" {
#define PFORGE_CONSTANTS_EXTERN_C_END }
#else
#define PFORGE_CONSTANTS_EXTERN_C_START
#define PFORGE_CONSTANTS_EXTERN_C_END
#endif

#define PFORGE_DECLARE_CONSTANT_BLOB(blob_name, blob_symbol_name, modifier)                                            \
    PFORGE_CONSTANTS_EXTERN_C_START                                                                                    \
    extern modifier unsigned char blob_name##_data[];                                                                  \
    PFORGE_CONSTANTS_EXTERN_C_END                                                                                      \
    PFORGE_MAY_BE_UNUSED static inline unsigned modifier char *get##blob_symbol_name##Data(void) {                     \
        return (unsigned modifier char *)(blob_name##_data);                                                           \
    }

#endif // INCBIN

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
