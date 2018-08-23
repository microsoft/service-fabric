// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NET_API_STATUS          DWORD
#define API_RET_TYPE            NET_API_STATUS      // Old value: do not use
#if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#define NET_API_FUNCTION    __stdcall
#else
#define NET_API_FUNCTION
#endif

#define MAX_PREFERRED_LENGTH    ((DWORD) -1)

#ifdef __cplusplus
}
#endif
