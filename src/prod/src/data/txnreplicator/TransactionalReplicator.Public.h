// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if !PLATFORM_UNIX
#include <sal.h>
#endif
#include <ktl.h>
#include <KTpl.h> 

namespace TxnReplicator
{
    using ::_delete;
}

// External dependencies.
#include "../../Reliability/Replication/Replication.Public.h"
#include "../../Reliability/Replication/IOperationDataFactory.h"

// Internal dependencies
#include "../utilities/Data.Utilities.Public.h"
#include "./common/TransactionalReplicator.Common.Public.h"
#include "../logicallog/LogicalLog.Public.h"
#include "./statemanager/StateManager.Public.h"

// Public headers.
#include "TransactionalReplicatorPointers.h"
#include "ITransactionalReplicatorFactory.h"
