// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IN6ADDR_IS6TO4(a) \
    ((a)->sin6_addr.s6_addr16[0] == 0x2002)
#define IN6ADDR_ISISATAP(a) \
    (((a)->sin6_addr.s6_addr16[0] == 0xfe80) && \
    (((a)->sin6_addr.s6_addr16[1] == 0x0000) || \
    ((a)->sin6_addr.s6_addr16[1] == 0x0200)) && \
    ((a)->sin6_addr.s6_addr16[2] == 0x5efe))

#ifdef __cplusplus
}
#endif
