// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ERROR_CURRENT_DIRECTORY          16L
#define ERROR_SEEK                       25L
#define ERROR_NETNAME_DELETED            64L
#define ERROR_ALREADY_ASSIGNED           85L
#define ERROR_MR_MID_NOT_FOUND           317L
#define ERROR_NET_OPEN_FAILED            570L
#define ERROR_CONTROL_C_EXIT             572L    // winnt
#define ERROR_UNHANDLED_EXCEPTION        574L
#define ERROR_ASSERTION_FAILURE          668L
#define ERROR_ABANDONED_WAIT_0           735L
#define ERROR_RETRY                      1237L
#define ERROR_REVISION_MISMATCH          1306L
#define ERROR_NO_SUCH_ALIAS              1376L
#define ERROR_MEMBER_IN_ALIAS            1378L
#define ERROR_ALIAS_EXISTS               1379L
#define ERROR_CLUSTER_INCOMPATIBLE_VERSIONS 5075L
#define WSAEINTR                         10004L
#define WSAEFAULT                        10014L
#define WSAEINVAL                        10022L
#define WSAEAFNOSUPPORT                  10047L

#ifdef RC_INVOKED
#define _HRESULT_TYPEDEF_(_sc) _sc
#else // RC_INVOKED
#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)
#endif // RC_INVOKED

#define E_BOUNDS                         _HRESULT_TYPEDEF_(0x8000000BL)

#ifdef __cplusplus
}
#endif
