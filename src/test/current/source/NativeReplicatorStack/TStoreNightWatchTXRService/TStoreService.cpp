// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ktl;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace TStoreNightWatchTXRService;
using namespace TxnReplicator;

StringLiteral const TraceComponent("TStoreNightWatchTXRService");

TStoreService::TStoreService(
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in Common::ComponentRootSPtr const & root)
    : NightWatchTXRService::Service(partitionId, replicaId, root)
{
}

ComPointer<IFabricStateProvider2Factory> TStoreService::GetStateProviderFactory()
{
    KAllocator & allocator = this->KtlSystemValue->PagedAllocator();

    return TestComStateProvider2Factory::Create(allocator);
}

NightWatchTXRService::ITestRunner::SPtr TStoreService::CreateTestRunner(
    __in Common::ByteBufferUPtr const & testParametersJson,
    __in TxnReplicator::ITransactionalReplicator & txnReplicator,
    __in KAllocator & allocator)
{
    TStoreTestRunner::SPtr runner = TStoreTestRunner::Create(testParametersJson, txnReplicator, allocator);
    return runner.RawPtr();
}
