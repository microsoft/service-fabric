// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        /// <summary>
        /// Collection of named operation data
        /// </summary>
        class CopyOperationDataStream final
            : public TxnReplicator::OperationDataStream
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::SM>
        {
            K_FORCE_SHARED(CopyOperationDataStream)

            K_SHARED_INTERFACE_IMP(IDisposable)

        public:
            static NTSTATUS Create(
                __in Utilities::PartitionedReplicaId const & traceType,
                __in KArray<SerializableMetadata::CSPtr> const & state,
                __in FABRIC_REPLICA_ID targetReplicaId,
                __in SerializationMode::Enum serailizationMode,
                __in KAllocator & allocator,
                __out SPtr & result);

        public:
            ktl::Awaitable<NTSTATUS> GetNextAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out Utilities::OperationData::CSPtr & result) noexcept override;

        public:
            void Dispose() override;

        private:
            CopyOperationDataStream(
                __in Utilities::PartitionedReplicaId const & traceType,
                __in KArray<SerializableMetadata::CSPtr> const & state,
                __in FABRIC_REPLICA_ID targetReplicaId,
                __in SerializationMode::Enum serailizationMode);

        private:
            FABRIC_REPLICA_ID const targetReplicaId_;

            // Operation data enumerator.
            OperationDataEnumerator::SPtr const checkpointOperationDataEnumerator_;

        private:
            /// <summary>
            /// Flag that is used to detect redundant calls to dispose.
            /// </summary>
            bool isDisposed_ = false;

            /// <summary>
            /// Current wire format version.
            /// </summary>
            int version_ = 1;

            ULONG currentIndex_ = 0;
        };
    }
}
