//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

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
#include "../tstore/Store.Public.h"
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
#include "RCQStateProviderFactory.h"

