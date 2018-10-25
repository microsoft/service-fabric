// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MICROSOFT_KERBEROS_NAME_A
#define MICROSOFT_KERBEROS_NAME_A   "Kerberos"
#define MICROSOFT_KERBEROS_NAME_W   L"Kerberos"
#ifdef WIN32_CHICAGO
#define MICROSOFT_KERBEROS_NAME MICROSOFT_KERBEROS_NAME_A
#else
#define MICROSOFT_KERBEROS_NAME MICROSOFT_KERBEROS_NAME_W
#endif
#endif  // MICROSOFT_KERBEROS_NAME_A

#ifndef NEGOSSP_NAME
#define NEGOSSP_NAME_W  L"Negotiate"
#define NEGOSSP_NAME_A  "Negotiate"

#ifdef UNICODE
#define NEGOSSP_NAME    NEGOSSP_NAME_W
#else
#define NEGOSSP_NAME    NEGOSSP_NAME_A
#endif
#endif // NEGOSSP_NAME

#ifdef __cplusplus
}
#endif
