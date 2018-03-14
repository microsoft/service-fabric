// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "ktl.h"

#if !defined(PLATFORM_UNIX)
#include "Iphlpapi.h"
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <arpa/nameser.h>
#include <resolv.h>
#endif

#include "FabricClient_.h"
#include "FabricCommon_.h"
#include "FabricRuntime_.h"

#include "StateMachineBase.h"
#include "ComUnknown.h"
#include "ComPointer.h"
#include "IDnsParser.h"
#include "INetIoManager.h"
#include "IFabricResolve.h"
#include "IDnsService.h"
#include "INetworkParams.h"
#include "IDnsCache.h"
#include "DnsValidateName.h"

namespace DNS
{
#if !defined(PLATFORM_UNIX)
    const ULONG TAG = 'SSND';
    const WCHAR FabricSchemeName[] = L"fabric:";
#endif

    BOOLEAN CompareKString(
        __in const KString::SPtr& spKeyOne,
        __in const KString::SPtr& spKeyTwo
    );
    
#if defined(PLATFORM_UNIX)

#define TAG 'SSND'
#define FabricSchemeName L"fabric:"

int WSAAPI closesocket(
    __in SOCKET s
    );

#endif
}

using namespace DNS;
