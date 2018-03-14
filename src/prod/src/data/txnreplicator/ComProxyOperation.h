// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // The Com Proxy that represents a V1 replicator replication/copy operation
    //
    class ComProxyOperation final
        : public KObject<ComProxyOperation>
        , public KShared<ComProxyOperation>
        , public Data::LoggingReplicator::IOperation
    {
        K_FORCE_SHARED(ComProxyOperation)
        K_SHARED_INTERFACE_IMP(IOperation)

    public:

        static NTSTATUS Create(
            __in Common::ComPointer<IFabricOperation> comOperation,
            __in KAllocator & allocator,
            __out ComProxyOperation::SPtr & comProxyOperation);

        void Acknowledge() override;

        FABRIC_OPERATION_TYPE get_OperationType() const override;

        LONG64 get_SequenceNumber() const override;

        LONG64 get_AtomicGroupId() const override;

        Data::Utilities::OperationData::CSPtr get_Data() const override;

    private:

        ComProxyOperation(__in Common::ComPointer<IFabricOperation> comOperation);

        Common::ComPointer<IFabricOperation> comOperation_;
    };
}
