// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "inaddr.h"
#include "in6addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef sockaddr_in6 SOCKADDR_IN6, *PSOCKADDR_IN6, FAR *LPSOCKADDR_IN6;

#define IN6ADDR_6TO4PREFIX_LENGTH 16

#define IN6ADDR_LINKLOCALPREFIX_LENGTH 64

extern CONST IN_ADDR in4addr_any;

extern CONST IN6_ADDR in6addr_linklocalprefix;

#ifdef __cplusplus
}
#endif
