//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ktl;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace RCQNightWatchTXRService;
using namespace TxnReplicator;

StringLiteral const TraceComponent("RCQNightWatchTXRService");

RCQService::RCQService(
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in Common::ComponentRootSPtr const & root)
    : NightWatchTXRService::Service(partitionId, replicaId, root)
{
}

ComPointer<IFabricStateProvider2Factory> RCQService::GetStateProviderFactory()
{
    KAllocator & allocator = this->KtlSystemValue->PagedAllocator();

    return TestComStateProvider2Factory::Create(allocator);
}

NightWatchTXRService::ITestRunner::SPtr RCQService::CreateTestRunner(
    __in Common::ByteBufferUPtr const & testParametersJson,
    __in TxnReplicator::ITransactionalReplicator & txnReplicator,
    __in KAllocator & allocator)
{
    RCQTestRunner::SPtr runner = RCQTestRunner::Create(testParametersJson, txnReplicator, allocator);
    return runner.RawPtr();
}