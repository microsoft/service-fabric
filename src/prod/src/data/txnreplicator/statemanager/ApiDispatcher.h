// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        //
        // Dispatches APIs to the State Provider 2.
        //
        class ApiDispatcher final :
            public KObject<ApiDispatcher>,
            public KShared<ApiDispatcher>,
            public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::SM>
        {
            K_FORCE_SHARED(ApiDispatcher);

        public:
            enum FailureAction
            {
                Throw = 0,                  // Just throw
                AbortStateProvider = 1,     // Abort the state provider
            };

        public:
            static SPtr Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in IStateProvider2Factory & stateProviderFactory,
                __in KAllocator & allocator);

        public: // Factory APIs
             NTSTATUS CreateStateProvider(
                __in KUri const & name,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in KString const & typeString,
                __in_opt Utilities::OperationData const * const initializationParameters,
                __out TxnReplicator::IStateProvider2::SPtr & outStateProvider) noexcept;

        public: // Single: Life cycle APIs 
            NTSTATUS Initialize(
                __in Metadata const & metadata,
                __in KWeakRef<TxnReplicator::ITransactionalReplicator> & transactionalReplicatorWRef,
                __in KStringView const & workFolder,
                __in_opt KSharedArray<TxnReplicator::IStateProvider2::SPtr> const * const children) noexcept;

            ktl::Awaitable<NTSTATUS> OpenAsync(
                __in Metadata const & metadata,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> ChangeRoleAsync(
                __in Metadata const & metadata,
                __in FABRIC_REPLICA_ROLE role,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> CloseAsync(
                __in Metadata const & metadata,
                __in FailureAction failureAction,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            void Abort(
                __in Metadata const & metadata) noexcept;

        public: // Checkpoint API
            ktl::Awaitable<NTSTATUS> RecoverCheckpointAsync(
                __in Metadata const & metadata,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

        public: // Copy operations
            ktl::Awaitable<NTSTATUS> BeginSettingCurrentStateAsync(
                __in Metadata const & metadata) noexcept;

            ktl::Awaitable<NTSTATUS> SetCurrentStateAsync(
                __in Metadata const & metadata,
                __in LONG64 stateRecordNumber,
                __in Data::Utilities::OperationData const & data,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> EndSettingCurrentStateAsync(
                __in Metadata const & metadata,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

        public: // Multiple: Life cycle operations
            ktl::Awaitable<NTSTATUS> OpenAsync(
                __in KArray<Metadata::CSPtr> const & metadataArray,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> ChangeRoleAsync(
                __in KArray<Metadata::CSPtr> const & metadataArray,
                __in FABRIC_REPLICA_ROLE role,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> CloseAsync(
                __in KArray<Metadata::CSPtr> const & metadataArray,
                __in FailureAction failureAction,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            void Abort(
                __in KArray<Metadata::CSPtr> const & metadataArray,
                __in ULONG startingIndex,
                __in ULONG count) noexcept;

            ktl::Awaitable<NTSTATUS> EndSettingCurrentStateAsync(
                __in KArray<Metadata::CSPtr> const & metadataArray,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

        public: // Multiple: Checkpoint operations
            NTSTATUS PrepareCheckpoint(
                __in KArray<Metadata::CSPtr> const & metadataArray,
                __in FABRIC_SEQUENCE_NUMBER checkpointLSN) noexcept;

            ktl::Awaitable<NTSTATUS> PerformCheckpointAsync(
                __in KArray<Metadata::CSPtr> const & metadataArray,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> CompleteCheckpointAsync(
                __in KArray<Metadata::CSPtr> const & metadataArray,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> RecoverCheckpointAsync(
                __in KArray<Metadata::CSPtr> const & metadataArray,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> RestoreCheckpointAsync(
                __in KArray<Metadata::CSPtr> const & metadataArray,
                __in KArray<KString::CSPtr> const & backupDirectoryArray,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> RemoveStateAsync(
                __in KArray<Metadata::CSPtr> const & metadataArray,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

        public: // Remove State Provider operations.
            ktl::Awaitable<NTSTATUS> PrepareForRemoveAsync(
                __in Metadata const & metadata,
                __in TxnReplicator::Transaction const & transaction,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> RemoveStateAsync(
                __in Metadata const & metadata,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

        private: // Checkpoint operations
            NTSTATUS PrepareCheckpoint(
                __in Metadata const & metadata,
                __in FABRIC_SEQUENCE_NUMBER checkpointLSN) noexcept;

            ktl::Awaitable<NTSTATUS> PerformCheckpointAsync(
                __in Metadata const & metadata,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> CompleteCheckpointAsync(
                __in Metadata const & metadata,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> RestoreCheckpointAsync(
                __in Metadata const & metadata,
                __in KString const & backupDirectory,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

        private:
            NOFAIL ApiDispatcher(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in IStateProvider2Factory & stateProviderFactory) noexcept;

        private:
            static Common::WStringLiteral const OpenAsync_FunctionName;
            static Common::WStringLiteral const CloseAsync_FunctionName;
            static Common::WStringLiteral const RecoverCheckpointAsync_FunctionName;
            static Common::WStringLiteral const PerformCheckpointAsync_FunctionName;
            static Common::WStringLiteral const CompleteCheckpointAsync_FunctionName;
            static Common::WStringLiteral const RestoreCheckpointAsync_FunctionName;
            static Common::WStringLiteral const BeginSettingCurrentState_FunctionName;
            static Common::WStringLiteral const EndSettingCurrentStateAsync_FunctionName;
            static Common::WStringLiteral const PrepareForRemoveAsync_FunctionName;
            static Common::WStringLiteral const RemoveStateAsync_FunctionName;

        private:
            IStateProvider2Factory::SPtr const stateProviderFactorySPtr_;
        };
    }
}
