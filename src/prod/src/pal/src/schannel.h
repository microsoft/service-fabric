// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UNISP_NAME_A    "Microsoft Unified Security Protocol Provider"
#define UNISP_NAME_W    L"Microsoft Unified Security Protocol Provider"

#ifdef UNICODE

#define UNISP_NAME  UNISP_NAME_W

#else

#define UNISP_NAME  UNISP_NAME_A

#endif

#ifdef __cplusplus
}
#endif
