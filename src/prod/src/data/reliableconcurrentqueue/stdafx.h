// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if !defined(PLATFORM_UNIX)
#include <sal.h>
#endif
#include "Common/Common.h"
#include <ktl.h>
#include <KTpl.h>
#include <kPriorityQueue.h>

// External dependencies
#include "../data.public.h"
#include "../utilities/Data.Utilities.Public.h"
#include "../txnreplicator/common/TransactionalReplicator.Common.Public.h"
#include "../txnreplicator/statemanager/StateManager.Public.h"
#include "../txnreplicator/TransactionalReplicator.Public.h"
#include "../txnreplicator/loggingreplicator/VersionManager.Public.h"
#include "../tstore/stdafx.h"
#include <SfStatus.h>

namespace Data
{
    namespace Collections
    {
        using ::_delete;
        using namespace Data::Utilities;
    }
}

#include "IReliableConcurrentQueue.h"
#include "ConcurrentQueue.h"
#include "ReliableConcurrentQueue.h"
#include "RCQStateProviderFactory.h"

namespace ReliableConcurrentQueueTests
{
    using ::_delete;
}

 #include "../testcommon/TestCommon.Public.h"

// The following are test headers
#include "ReliableConcurrentQueue.TestBase.h"

// The following are perf test headers
