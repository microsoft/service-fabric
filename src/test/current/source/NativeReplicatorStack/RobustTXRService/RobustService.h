// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace RobustTXRService
{
    class RobustService
        : public NightWatchTXRService::Service
    {
    public:
        RobustService(
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __in Common::ComponentRootSPtr const & root);

    private:
        virtual NightWatchTXRService::ITestRunner::SPtr CreateTestRunner(
            __in Common::ByteBufferUPtr const & testParametersJson,
            __in TxnReplicator::ITransactionalReplicator & txnReplicator,
            __in KAllocator & allocator) override;
    };
}
