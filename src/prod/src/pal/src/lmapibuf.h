// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"
#include "lmcons.h"

#ifdef __cplusplus
extern "C" {
#endif

NET_API_STATUS NET_API_FUNCTION
NetApiBufferFree (
    IN LPVOID Buffer
    );

#ifdef __cplusplus
}
#endif
