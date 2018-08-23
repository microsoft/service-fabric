// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "ws2tcpip.h"
#include "util/pal_string_util.h"
#include <string>
#include <unistd.h>

using namespace std;
using namespace Pal;

WINSOCK_API_LINKAGE
INT
WSAAPI
GetAddrInfoW(
    _In_opt_        PCWSTR              pNodeName,
    _In_opt_        PCWSTR              pServiceName,
    _In_opt_        const ADDRINFOW *   pHints,
    _Outptr_     PADDRINFOW *        ppResult
    )
{
    string nodeName;
    char const * nodeNamePtr = NULL;
    char hostname[NI_MAXHOST];
    if (pNodeName)
    {
        if(pNodeName[0] == 0)
        {
            gethostname(hostname, sizeof(hostname));
            nodeNamePtr = hostname;
        }
        else
        {
            nodeName = utf16to8(pNodeName);
            nodeNamePtr = nodeName.c_str();
        }
    }

    string serviceName;
    if (pServiceName)
    {
        serviceName = utf16to8(pServiceName);
    }

    return getaddrinfo(
        nodeNamePtr,
        serviceName.c_str(),
        pHints,
        ppResult);
}

WINSOCK_API_LINKAGE
VOID
WSAAPI
FreeAddrInfoW(
    _In_opt_        PADDRINFOW      pAddrInfo
    )
{
    freeaddrinfo(pAddrInfo);
}
