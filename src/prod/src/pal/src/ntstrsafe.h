// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef _Null_terminated_ char* NTSTRSAFE_PSTR;
typedef _Null_terminated_ const char* NTSTRSAFE_PCSTR;
typedef _Null_terminated_ wchar_t* NTSTRSAFE_PWSTR;
typedef _Null_terminated_ const wchar_t* NTSTRSAFE_PCWSTR;
typedef _Null_terminated_ const wchar_t UNALIGNED* NTSTRSAFE_PCUWSTR;

#ifdef __cplusplus
}
#endif
