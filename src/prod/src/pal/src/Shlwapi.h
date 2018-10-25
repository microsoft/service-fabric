// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LWSTDAPI_(type) type

LWSTDAPI_(struct IStream *) SHCreateMemStream(__in_bcount_opt(cbInit) const BYTE *pInit, UINT cbInit);

#ifdef __cplusplus
}
#endif
