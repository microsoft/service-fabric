// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef union _NET_LUID_LH
{
    ULONG64     Value;
    struct
    {
        ULONG64     Reserved:24;
        ULONG64     NetLuidIndex:24;
        ULONG64     IfType:16;                  // equal to IANA IF type
    }Info;
} NET_LUID_LH, *PNET_LUID_LH;

typedef NET_LUID_LH NET_LUID;
typedef NET_LUID* PNET_LUID;

#ifdef __cplusplus
}
#endif
