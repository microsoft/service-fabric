// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct in6_addr IN6_ADDR, *PIN6_ADDR, *LPIN6_ADDR;

#ifdef __cplusplus
}
#endif
