// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"
#include "winnt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ACCESS_MASK REGSAM;

typedef __success(return==ERROR_SUCCESS) LONG LSTATUS;

#ifdef __cplusplus
}
#endif
