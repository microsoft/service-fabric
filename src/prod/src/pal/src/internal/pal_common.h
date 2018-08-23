// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define PLATFORM_UNIX 1
#define PAL_STDCPP_COMPAT 1
#define UNICODE 1
#define BIT64 1
#define _WIN64 1

/* Avoiding redefining 'new' here since KTL in kallocator.h defines the same and
 * there is a race condition to get to it. This is yanked from pal.h. Maybe need
 * to be moved closer to top */
#ifdef __cplusplus
#if !defined(__PLACEMENT_NEW_INLINE)
// This is already defined in llvm libc++ /usr/includ/c++/v1/new no need to
// redefine again. But have to guard redefinition in KTL and elsewhere with the
// below define
#define __PLACEMENT_NEW_INLINE
#endif // __PLACEMENT_NEW_INLINE
#endif

#include "rt/sal.h"
#ifdef __cplusplus
#include "pal_mstypes.h"
#endif
#include "palrt.h"

