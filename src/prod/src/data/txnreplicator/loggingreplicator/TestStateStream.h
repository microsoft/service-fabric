// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{

    class TestStateStream final
        : public TxnReplicator::OperationDataStream
    {
        K_FORCE_SHARED(TestStateStream)

    public:
        static TestStateStream::SPtr Create(__in KAllocator & allocator, __in ULONG32 successfulOperationCount);

        ktl::Awaitable<NTSTATUS> GetNextAsync(
            __in ktl::CancellationToken const & cancellationToken,
            __out Data::Utilities::OperationData::CSPtr & result) noexcept override;

        void Dispose() override;

    private:

        TestStateStream(__in ULONG32 successfulOperationCount);

        ULONG32 successfulOperationCount_;
        ULONG32 currentOperationCount_;
    };
}
