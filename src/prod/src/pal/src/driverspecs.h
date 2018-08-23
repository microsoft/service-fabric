// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// 333
#define __drv_functionClass _Function_class_

// 484
#define _IRQL_requires_max_(irql)  /* see kernelspecs.h */
#define __drv_maxIRQL _IRQL_requires_max_ /* legacy */

// 536
#define _IRQL_requires_same_  /* see kernelspecs.h */
#define __drv_sameIRQL _IRQL_requires_same_ /* legacy */

// 639
__ANNOTATION(SAL_IsAliased(void);)
#define __drv_aliasesMem _Post_ _SA_annotes0(SAL_IsAliased)

//
// Allocate/release memory-like objects.
// Kind is unused, but should be "mem" for malloc/free
// and "object" for new/delete.
__ANNOTATION(SAL_NeedsRelease(enum __SAL_YesNo);)
#define __drv_allocatesMem(kind) _Post_ _SA_annotes1(SAL_NeedsRelease,__yes)

#define __drv_freesMem(kind) _Post_ _SA_annotes1(SAL_NeedsRelease,__no)

#ifdef __cplusplus
}
#endif
