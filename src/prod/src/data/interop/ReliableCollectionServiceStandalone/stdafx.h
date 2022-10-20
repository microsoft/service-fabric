// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// External dependencies
#include "../../txnreplicator/TransactionalReplicator.Public.h"
#include "../../tstore/Store.Public.h"
#include "../../logicallog/LogicalLog.Public.h"
#if defined(PLATFORM_UNIX)
#include "../../../resources/StringResourceRepo.h"
#endif
#ifndef WIN32
#define UNDER_PAL
#endif
#define _EXPORTING
#include "../ReliableCollectionRuntime/dll/ReliableCollectionRuntime.h"
namespace Data
{
    namespace Interop
    {
        using ::_delete;
    }
}
#include "../ReliableCollectionRuntimeImpl/StateProviderFactory.h"

// External test dependencies
#include "../../testcommon/TestCommon.Public.h"
#include "../../txnreplicator/testcommon/TransactionalReplicator.TestCommon.Public.h"

namespace Data
{
    namespace Integration
    {
        using ::_delete;
    }
}

#include "../../integration/Replica.h"

struct ReliableCollectionApis;
void GetReliableCollectionImplApiTable(ReliableCollectionApis* reliableCollectionApis);
void GetReliableCollectionMockApiTable(ReliableCollectionApis* reliableCollectionApis);
