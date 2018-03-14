// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        /// <summary>
        /// Collection of named operation data
        /// </summary>
        class TestOperationDataStream final
            : public OperationDataStream
            , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TestSession>
        {
            K_FORCE_SHARED(TestOperationDataStream)
            K_SHARED_INTERFACE_IMP(IDisposable)
 
        public:
            static SPtr Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in FABRIC_SEQUENCE_NUMBER checkpointLSN,      // prepare checkpoint LSN of the last completed checkpoint.
                __in VersionedState const & data,
                __in KAllocator & allocator) noexcept;

        public:
            ktl::Awaitable<NTSTATUS> GetNextAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out Data::Utilities::OperationData::CSPtr & result) noexcept override;

        public:
            void Dispose() override;

        private:
            TestOperationDataStream(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in FABRIC_SEQUENCE_NUMBER checkpointLSN,      // prepare checkpoint LSN of the last completed checkpoint.
                __in VersionedState const & data) noexcept;

        private:
            // For tracing only.
            FABRIC_STATE_PROVIDER_ID const stateProviderId_;

            // In-memory version of the checkpointed data.
            FABRIC_SEQUENCE_NUMBER const checkpointLSN_;
            VersionedState::CSPtr const data_;

        private:
            /// <summary>
            /// Flag that is used to detect redundant calls to dispose.
            /// </summary>
            bool isDisposed_ = false;

            // Indicates the stage of the copy.
            ULONG currentIndex_ = 0;
        };
    }
} 
