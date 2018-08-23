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

typedef sa_family_t ADDRESS_FAMILY;

typedef struct sockaddr SOCKADDR, *PSOCKADDR, FAR *LPSOCKADDR;

typedef struct _SOCKET_ADDRESS {
    __field_bcount(iSockaddrLength) LPSOCKADDR lpSockaddr;
    INT iSockaddrLength;
} SOCKET_ADDRESS, *PSOCKET_ADDRESS, *LPSOCKET_ADDRESS;

typedef sockaddr_storage SOCKADDR_STORAGE;
typedef SOCKADDR_STORAGE *PSOCKADDR_STORAGE, FAR *LPSOCKADDR_STORAGE;

typedef enum {
    ScopeLevelInterface    = 1,
    ScopeLevelLink         = 2,
    ScopeLevelSubnet       = 3,
    ScopeLevelAdmin        = 4,
    ScopeLevelSite         = 5,
    ScopeLevelOrganization = 8,
    ScopeLevelGlobal       = 14,
    ScopeLevelCount        = 16
} SCOPE_LEVEL;

typedef struct _WSABUF {
    ULONG len;     /* the length of the buffer */
    __field_bcount(len) CHAR FAR *buf; /* the pointer to the buffer */
} WSABUF, FAR * LPWSABUF;

#ifdef __cplusplus
}
#endif
