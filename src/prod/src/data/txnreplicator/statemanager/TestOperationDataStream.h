// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests
{
    /// <summary>
    /// Collection of named operation data
    /// </summary>
    class TestOperationDataStream final
        : public TxnReplicator::OperationDataStream
    {
        K_FORCE_SHARED(TestOperationDataStream)
        K_SHARED_INTERFACE_IMP(IDisposable)

    public:
        static SPtr Create(
            __in LONG64 prepareCheckpointLSN,
            __in KAllocator & allocator);

    public:

        ktl::Awaitable<NTSTATUS> GetNextAsync(
            __in ktl::CancellationToken const & cancellationToken,
            __out Data::Utilities::OperationData::CSPtr & result) noexcept override;

    public:
        void Dispose() override;

    private:
        TestOperationDataStream(__in LONG64 prepareCheckpointLSN);

    private:
        const LONG64 prepareCheckpointLSN_;

    private:
        /// <summary>
        /// Flag that is used to detect redundant calls to dispose.
        /// </summary>
        bool isDisposed_ = false;

        ULONG currentIndex_ = 0;
    };
}
