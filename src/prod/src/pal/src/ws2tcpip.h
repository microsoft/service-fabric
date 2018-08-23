// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#include "winsock2.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef addrinfo ADDRINFOW;
typedef ADDRINFOW * PADDRINFOW;

WINSOCK_API_LINKAGE
INT
WSAAPI
GetAddrInfoW(
    _In_opt_        PCWSTR              pNodeName,
    _In_opt_        PCWSTR              pServiceName,
    _In_opt_        const ADDRINFOW *   pHints,
    _Outptr_     PADDRINFOW *        ppResult
    );

WINSOCK_API_LINKAGE
VOID
WSAAPI
FreeAddrInfoW(
    _In_opt_        PADDRINFOW      pAddrInfo
    );

#ifdef __cplusplus
}
#endif
