// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class CopyContext final
            : public TxnReplicator::OperationDataStream 
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(CopyContext)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:
            static CopyContext::SPtr Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in LONG64 replicaId,
                __in ProgressVector & progressVector,
                __in IndexingLogRecord const & logHeadRecord,
                __in LONG64 logTailLsn,
                __in LONG64 latestRecoveredAtomicRedoOperationLsn,
                __in KAllocator & allocator);

            ktl::Awaitable<NTSTATUS> GetNextAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out Utilities::OperationData::CSPtr & result) noexcept override;

            void Dispose() override;

        private:
            CopyContext(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in LONG64 replicaId,
                __in ProgressVector & progressVector,
                __in IndexingLogRecord const & logHeadRecord,
                __in LONG64 logTailLsn,
                __in LONG64 LatestRecoveredAtomicRedoOperationLsn,
                __in KAllocator & allocator);

            Utilities::OperationData::CSPtr copyData_;
            bool isDone_;
        };
    }
}
