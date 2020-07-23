// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace NightWatchTXRService
{
    class Service
        : public TXRStatefulServiceBase::StatefulServiceBase
    {
    public:
        Service(
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __in Common::ComponentRootSPtr const & root);

        virtual Common::ComPointer<IFabricStateProvider2Factory> GetStateProviderFactory() override;

        Common::ComPointer<IFabricDataLossHandler> GetDataLossHandler() override;

        Common::ErrorCode OnHttpPostRequest(
            __in Common::ByteBufferUPtr && testParametersJson,
            __in Common::ByteBufferUPtr & responseBody) override;

    protected:
        virtual ITestRunner::SPtr CreateTestRunner(
            __in Common::ByteBufferUPtr const & testParametersJson,
            __in TxnReplicator::ITransactionalReplicator & txnReplicator,
            __in KAllocator & allocator);

    private:
        ktl::Task StartTest(__in Common::ByteBufferUPtr const & testParametersJson);

        bool testInProgress_;
        PerfResult::SPtr testResultSptr_;
    };
}
