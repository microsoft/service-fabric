// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestOperation final
        : public KObject<TestOperation>
        , public KShared<TestOperation>
        , public Data::LoggingReplicator::IOperation
    {
        K_FORCE_SHARED(TestOperation)
        K_SHARED_INTERFACE_IMP(IOperation)

    public:
        static TestOperation::SPtr Create(
            __in Data::Utilities::OperationData const & operationData,
            __in KAllocator & allocator);

        void Acknowledge() override;

        FABRIC_OPERATION_TYPE get_OperationType() const override;

        LONG64 get_SequenceNumber() const override;

        LONG64 get_AtomicGroupId() const override;

        Data::Utilities::OperationData::CSPtr get_Data() const override;

    private:
        TestOperation(__in Data::Utilities::OperationData const & operationData);

        Data::Utilities::OperationData::CSPtr operationData_;

    };
}
