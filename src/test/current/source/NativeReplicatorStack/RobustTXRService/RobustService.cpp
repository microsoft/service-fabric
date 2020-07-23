// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <SfStatus.h>

using namespace ktl;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace NightWatchTXRService;
using namespace RobustTXRService;
using namespace TxnReplicator;
using namespace TxnReplicator::TestCommon;

StringLiteral const TraceComponent("RobustTXRService");

RobustService::RobustService(
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in Common::ComponentRootSPtr const & root)
    : Service(partitionId, replicaId, root)
{
}

ITestRunner::SPtr RobustService::CreateTestRunner(
    __in Common::ByteBufferUPtr const & testParametersJson,
    __in TxnReplicator::ITransactionalReplicator & txnReplicator,
    __in KAllocator & allocator)
{
    return RobustTestRunner::Create(testParametersJson, txnReplicator, allocator).RawPtr();
}