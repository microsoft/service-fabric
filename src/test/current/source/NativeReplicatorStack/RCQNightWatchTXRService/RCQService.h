//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace RCQNightWatchTXRService
{
    class RCQService : public NightWatchTXRService::Service
    {
    public:
        RCQService(
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __in Common::ComponentRootSPtr const & root);

        virtual Common::ComPointer<IFabricStateProvider2Factory> GetStateProviderFactory() override;
    
    protected:
        virtual NightWatchTXRService::ITestRunner::SPtr CreateTestRunner(
            __in Common::ByteBufferUPtr const & testParametersJson,
            __in TxnReplicator::ITransactionalReplicator & txnReplicator,
            __in KAllocator & allocator) override;
    };
}
