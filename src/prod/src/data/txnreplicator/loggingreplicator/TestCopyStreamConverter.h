// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{

    class TestCopyStreamConverter final
        : public KObject<TestCopyStreamConverter>
        , public KShared<TestCopyStreamConverter>
        , public Data::LoggingReplicator::IOperationStream
    {
        K_FORCE_SHARED(TestCopyStreamConverter)
        K_SHARED_INTERFACE_IMP(IOperationStream)

    public:
        static TestCopyStreamConverter::SPtr Create(
            __in Data::LoggingReplicator::CopyStream & copyStream,
            __in ULONG32 maximumsuccessfulOperationCount_,
            __in KAllocator & allocator);

        ktl::Awaitable<NTSTATUS> GetOperationAsync(__out Data::LoggingReplicator::IOperation::SPtr & result) override;

    private:
        TestCopyStreamConverter(
            __in Data::LoggingReplicator::CopyStream & copyStream,
            __in ULONG32 maximumsuccessfulOperationCount_);

        Data::LoggingReplicator::CopyStream::SPtr copyStream_;
        ULONG32 maximumsuccessfulOperationCount_;
        ULONG32 currentOperationCount_;
    };
}
