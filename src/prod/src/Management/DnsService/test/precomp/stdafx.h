// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdio.h"
#include "Common/Common.h"
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "ktl.h"
#include "Runtime.h"

#include "FabricClient_.h"
#include "FabricCommon_.h"
#include "FabricRuntime_.h"

#include "ComUnknown.h"
#include "ComPointer.h"
#include "IDnsParser.h"
#include "INetIoManager.h"
#include "IFabricResolve.h"
#include "IDnsService.h"
#include "IDnsCache.h"

#if !defined(PLATFORM_UNIX)
#include "Iphlpapi.h"
#include "windns.h"
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#endif

#include "ComServiceManager.h"
#include "ComPropertyManager.h"
#include "ComFabricPropertyValueResult.h"
#include "ComFabricStatelessServicePartition2.h"
#include "NullTracer.h"
#include "FabricData.h"
#include "DnsServiceSynchronizer.h"
#include "DnsHelper.h"
#include "DnsValidateName.h"

using namespace DNS;
using namespace DNS::Test;

#if !defined(PLATFORM_UNIX)
static ULONG TAG = 'TEST';
#else
#define TAG 'TEST'
#endif
