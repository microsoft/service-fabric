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
        // Manages State Providers.
        //
        class StateManager final : 
            public KAsyncServiceBase, 
            public KWeakRefType<StateManager>,
            public IStateManager,
            public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::SM>
        {
            friend class StateManagerTests::RecoveryTest;

            K_FORCE_SHARED(StateManager)

            K_SHARED_INTERFACE_IMP(IStateManager)
            K_SHARED_INTERFACE_IMP(IStateProviderMap)
            K_SHARED_INTERFACE_IMP(ITransactionStateManager)
            K_SHARED_INTERFACE_IMP(IStateProviderManager)

            K_WEAKREF_INTERFACE_IMP(IStateManager, StateManager)
            K_WEAKREF_INTERFACE_IMP(ITransactionStateManager, StateManager)
            K_WEAKREF_INTERFACE_IMP(IStateProviderManager, StateManager)

        public: // Static consts.
            static const FABRIC_STATE_PROVIDER_ID StateManagerId;
            static const KUriView StateManagerName;

        public: // Life cycle
            static NTSTATUS Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in TxnReplicator::IRuntimeFolders & runtimeFolders,
                __in Data::Utilities::IStatefulPartition & partition,
                __in IStateProvider2Factory & stateProviderFactory,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in bool hasPersistedState,
                __in KAllocator& allocator,
                __out SPtr & result) noexcept;

            void Initialize(
                __in TxnReplicator::ITransactionalReplicator & transactionalReplicator,
                __in TxnReplicator::ILoggingReplicator & loggingReplicator) override;

            ktl::Awaitable<NTSTATUS> OpenAsync(
                __in bool completeCheckpoint,
                __in bool cleanupRestore,
                __in ktl::CancellationToken & cancellationToken) noexcept override;

            ktl::Awaitable<void> ChangeRoleAsync(
                __in FABRIC_REPLICA_ROLE newRole,
                __in ktl::CancellationToken & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> CloseAsync(
                __in ktl::CancellationToken & cancellationToken) noexcept override;

        public: // Notification APIs
            NTSTATUS RegisterStateManagerChangeHandler(
                __in TxnReplicator::IStateManagerChangeHandler & stateManagerChangeHandler) noexcept override;

            NTSTATUS UnRegisterStateManagerChangeHandler() noexcept override;

        public: // IStateProviderMap APIs
            /// <summary>
            /// Gets the given state provider registered with the state manager.
            /// </summary>
            // Error Code Info:
            //  Success:
            //      SF_STATUS_NAME_DOES_NOT_EXIST: Requested ISP2 does not exist, return nullptr
            //      STATUS_SUCCESS: Indicates function successfully return a valid SPtr to the requested ISP2
            //  User Should Handle:
            //      Retry-able:
            //          SF_STATUS_NOT_READABLE: Partition is not readable
            //          SF_STATUS_OBJECT_CLOSED: StateManager is closed
            //      Failed:
            //
            //  User should not handle:
            //      SF_STATUS_INVALID_NAME_URI: Input invalid Name
            //      STATUS_INSUFFICIENT_RESOURCES: Out of memory to allocate
            NTSTATUS Get(
                __in KUriView const & stateProviderName,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept override;

            /// <summary>
            /// Gets state providers registered with the state manager.
            /// </summary>
            // Error Code Info:
            //  Success: 
            //      STATUS_SUCCESS: Indicates function successfully return a valid enumerator sptr
            //  User Should Handle:
            //      Retry-able:
            //          SF_STATUS_OBJECT_CLOSED: StateManager is closed
            //      Failed:
            //
            //  User should not handle:
            //      STATUS_INSUFFICIENT_RESOURCES: Out of memory to allocate
            NTSTATUS CreateEnumerator(
                __in bool parentsOnly,
                __out Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, TxnReplicator::IStateProvider2::SPtr>>::SPtr & outEnumerator) noexcept override;

            /// <summary>
            /// Adds a state provider to the state manager.
            /// </summary>
            // Error Code Info:
            //  Success: 
            //      STATUS_SUCCESS: Indicates function successfully add a state provider to state manager
            //  User Should Handle:
            //      Retry-able:
            //          SF_STATUS_NOT_PRIMARY: Partition access status is not primary
            //          SF_STATUS_OBJECT_CLOSED: StateManager is closed
            //          SF_STATUS_TIMEOUT: Function call timed out 
            //          STATUS_CANCELLED: Function canceled
            //      Failed:
            //          SF_STATUS_NAME_ALREADY_EXISTS: The state provider with specific name is already exist
            //  User should not handle:
            //      SF_STATUS_INVALID_NAME_URI: Input invalid Name
            //      STATUS_INSUFFICIENT_RESOURCES: Out of memory to allocate
            //      STATUS_INVALID_PARAMETER: Invalid initialization parameters
            ktl::Awaitable<NTSTATUS> AddAsync(
                __in TxnReplicator::Transaction & transaction,
                __in KUriView const & stateProviderName,
                __in KStringView const & stateProviderTypeName,
                __in_opt Data::Utilities::OperationData const * const initializationParameters,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken) noexcept override;

            /// <summary>
            // Removes a state provider from the state manager.
            /// </summary>
            // Error Code Info:
            //  Success: 
            //      STATUS_SUCCESS: Indicates function successfully add a state provider to state manager
            //  User Should Handle:
            //      Retry-able:
            //          SF_STATUS_NOT_PRIMARY: Partition access status is not primary
            //          SF_STATUS_OBJECT_CLOSED: StateManager is closed
            //          SF_STATUS_TIMEOUT: Function call timed out 
            //          STATUS_CANCELLED: Function canceled
            //      Failed:
            //          SF_STATUS_NAME_DOES_NOT_EXIST: The state provider with specific name does not exist
            //  User should not handle:
            //      SF_STATUS_INVALID_NAME_URI: Input invalid Name
            //      STATUS_INSUFFICIENT_RESOURCES: Out of memory to allocate
            ktl::Awaitable<NTSTATUS> RemoveAsync(
                __in TxnReplicator::Transaction & transaction,
                __in KUriView const & stateProviderName,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken) noexcept override;

            /// <summary>
            // Gets or Adds state provider.
            /// </summary>
            // Error Code Info:
            //  Success: 
            //      STATUS_SUCCESS: Indicates function successfully add or get a state provider to/from state manager
            //  User Should Handle:
            //      Retry-able:
            //          SF_STATUS_NOT_PRIMARY: Partition access status is not primary
            //          SF_STATUS_NOT_READABLE: Partition is not readable
            //          SF_STATUS_OBJECT_CLOSED: StateManager is closed
            //          SF_STATUS_TIMEOUT: Function call timed out 
            //          STATUS_CANCELLED: Function canceled
            //      Failed:
            //
            //  User should not handle:
            //      SF_STATUS_INVALID_NAME_URI: Input invalid Name
            //      STATUS_INSUFFICIENT_RESOURCES: Out of memory to allocate
            //      STATUS_INVALID_PARAMETER: Invalid initialization parameters
            ktl::Awaitable<NTSTATUS> GetOrAddAsync(
                __in TxnReplicator::Transaction & transaction,
                __in KUriView const & stateProviderName,
                __in KStringView const & stateProviderTypeName,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider,
                __out bool & alreadyExist,
                __in_opt Utilities::OperationData const * const initializationParameters,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken) noexcept override;

        public: // ITransactionStateManager APIs (Interface between Transaction and StateManager)
            NTSTATUS SingleOperationTransactionAbortUnlock(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in TxnReplicator::OperationContext const & operationContext) noexcept override;

            NTSTATUS ErrorIfStateProviderIsNotRegistered(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in LONG64 transactionId) noexcept override;

            ktl::Awaitable<NTSTATUS> BeginTransactionAsync(
                __in TxnReplicator::Transaction & transaction,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const undo,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __out FABRIC_SEQUENCE_NUMBER & commitLsn) noexcept override;

            NTSTATUS BeginTransaction(
                __in TxnReplicator::Transaction & transaction,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const undo,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext) noexcept override;

            NTSTATUS AddOperation(
                __in TxnReplicator::Transaction & transaction,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const undo,
                __in_opt Utilities::OperationData const * const redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext) noexcept override;

            ktl::Awaitable<NTSTATUS> AddOperationAsync(
                __in TxnReplicator::AtomicOperation & atomicOperation,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const  undo,
                __in_opt Utilities::OperationData const * const  redo,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __out FABRIC_SEQUENCE_NUMBER & commitLsn) noexcept override;

            ktl::Awaitable<NTSTATUS> AddOperationAsync(
                __in TxnReplicator::AtomicRedoOperation & atomicRedoOperation,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in_opt Utilities::OperationData const * const metaData,
                __in_opt Utilities::OperationData const * const  redo,
                __out_opt TxnReplicator::OperationContext const * const operationContext,
                __out FABRIC_SEQUENCE_NUMBER & commitLsn) noexcept override;
        
        public: // TxnReplicator::IStateProviderManager (Interface between LoggingReplicator and StateManager)
            ktl::Awaitable<NTSTATUS> ApplyAsync(
                __in LONG64 logicalSequenceNumber,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in_opt Data::Utilities::OperationData const * const metadataPtr,
                __in_opt Data::Utilities::OperationData const * const dataPtr,
                __out TxnReplicator::OperationContext::CSPtr & result) noexcept override;

            NTSTATUS Unlock(__in TxnReplicator::OperationContext const & operationContext) noexcept override;

            NTSTATUS PrepareCheckpoint(__in LONG64 checkpointLSN) noexcept override;

            ktl::Awaitable<NTSTATUS> PerformCheckpointAsync(
                __in ktl::CancellationToken const & cancellationToken) noexcept override;

            ktl::Awaitable<NTSTATUS> CompleteCheckpointAsync(
                __in ktl::CancellationToken const & cancellationToken) noexcept override;

            ktl::Awaitable<NTSTATUS> BackupCheckpointAsync(
                __in KString const & backupDirectory,
                __in ktl::CancellationToken const & cancellationToken) noexcept override;

            ktl::Awaitable<NTSTATUS> RestoreCheckpointAsync(
                __in KString const & backupDirectory,
                __in ktl::CancellationToken const & cancellationToken) noexcept override;

            NTSTATUS GetCurrentState(
                __in FABRIC_REPLICA_ID targetReplicaId, 
                __out TxnReplicator::OperationDataStream::SPtr & result) noexcept override;

            ktl::Awaitable<NTSTATUS> BeginSettingCurrentStateAsync() noexcept override;
            
            ktl::Awaitable<NTSTATUS> EndSettingCurrentState() noexcept override;

            ktl::Awaitable<NTSTATUS> SetCurrentStateAsync(
                __in LONG64 stateRecordNumber,
                __in Data::Utilities::OperationData const & data) noexcept override;

        protected:
            VOID OnServiceOpen() override;
            VOID OnServiceClose() override;

        private:
            static KString::CSPtr CreateReplicaFolderPath(
                __in LPCWSTR workFolder, 
                __in KGuid partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in KAllocator & allocator);

        private: // Constructor.
            FAILABLE StateManager(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in TxnReplicator::IRuntimeFolders & runtimeFolders,
                __in Data::Utilities::IStatefulPartition & partition,
                __in IStateProvider2Factory & stateProviderFactoryFunction,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in bool hasPersistedState) noexcept;

        private: // Methods
            ktl::Task OnServiceOpenAsync() noexcept;
            ktl::Task OnServiceCloseAsync() noexcept;

            NTSTATUS CreateInternalEnumerator(
                __in bool parentsOnly,
                Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, TxnReplicator::IStateProvider2::SPtr>>::SPtr & outEnumerator) noexcept;

            // Combined operations
            ktl::Awaitable<void> CleanUpIncompleteRestoreAsync(__in ktl::CancellationToken const & cancellationToken);

            __checkReturn ktl::Awaitable<NTSTATUS> AddSingleAsync(
                __in TxnReplicator::Transaction & transaction,
                __in KUri const & stateProviderName,
                __in KString const & stateProviderTypeName,
                __in_opt Data::Utilities::OperationData const * const initializationParameters,
                __in TxnReplicator::IStateProvider2 & stateProvider,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in bool shouldAcquireLock,
                __in FABRIC_STATE_PROVIDER_ID parentId,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            __checkReturn ktl::Awaitable<NTSTATUS> RemoveSingleAsync(
                __in TxnReplicator::Transaction & transaction,
                __in KUri const & stateProviderName,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> ReplicateAsync(
                __in TxnReplicator::Transaction & transaction,
                __in Utilities::OperationData const & metadataOperation,
                __in Utilities::OperationData const * const redoOperation,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            // Calls on a State Provider
            // Port Note: Not static so that we do not need to pass the trace type.
            NTSTATUS GetChildren(
                __in TxnReplicator::IStateProvider2 & root,
                __inout KHashTable<KUri::CSPtr, StateProviderInfoInternal> & children) noexcept;
            
            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyAsync(
                __in FABRIC_SEQUENCE_NUMBER sequenceNumber,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in_opt Utilities::OperationData const * const metadataPtr,
                __in_opt Utilities::OperationData const * const dataPtr);

            void AddChildrenToList(
                __in Metadata & metadata, 
                __inout KSharedArray<Metadata::SPtr>::SPtr & children);
        
        private: // Helper functions
            NTSTATUS Copy(
                __in KUriView const & uriView,
                __out KUri::CSPtr & copyStateProviderName) const noexcept;

            NTSTATUS Copy(
                __in KStringView const & stringView,
                __out KString::CSPtr & copyString) const noexcept;

            ktl::Awaitable<NTSTATUS> AcquireChangeRoleLockAsync() noexcept;
            void ReleaseChangeRoleLock() noexcept;

            inline TxnReplicator::ILoggingReplicator::SPtr GetReplicator() const;

            void TraceException(
                __in wstring const & message, 
                __in ktl::Exception const & exception) const noexcept;

            void TraceException(
                __in FABRIC_STATE_PROVIDER_ID stateProviderID,
                __in Common::WStringLiteral const & message,
                __in ktl::Exception const & exception) const noexcept;

            void TraceError(
                __in wstring const & message,
                __in NTSTATUS status) const noexcept;

            __checkReturn NTSTATUS IsRegistered(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in bool checkForDeleting,
                __in LONG64 transactionId) noexcept;

            __checkReturn NTSTATUS CheckIfNameIsValid(
                __in KUriView const & stateProviderName,
                __in bool stateManagerNameAllowed) const noexcept;

            __checkReturn NTSTATUS CheckIfReadable(__in KUriView const & stateProviderName) const noexcept;

            __checkReturn NTSTATUS CheckIfWritable(
                __in Common::StringLiteral const & apiName,
                __in TxnReplicator::Transaction const & transaction,
                __in KUriView const & stateProviderName) const noexcept;

            void ReleaseLockAndThrowIfOnFailure(
                __in LONG64 transactionId,
                __in StateManagerLockContext & stateManagerLockContext,
                __in NTSTATUS status) const;

            // Port Note: Removed service name since it is not useful. 
            // This would be breaking change if we want to keep backwards compat.
            // Port Note: Renamed to *FileName since it does not actually get the file path but just the name.
            KString::CSPtr CreateStateProviderWorkDirectory(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId);

            KString::CSPtr GetStateProviderWorkDirectory(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId) const;

            bool TryDeleteStateProviderWorkDirectory(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId);

            // This function converts Version 0 state provider folder structures to Version 1.
            // Version 0: ~work/<pid>_<rid>_<spid>
            // Version 1: ~work/<pid>_<rid>/<spid>
            NTSTATUS UpgradeStateProviderFolderFrom0to1IfNecessary(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in KString const & destination,
                __out bool & isMoved) noexcept;

            NTSTATUS GetVersion0StateProviderFolderPath(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __out wstring & path) noexcept;

            Utilities::KHashSet<KString::CSPtr>::SPtr GetCurrentStateProviderDirectories();
            KArray<KString::CSPtr> GetDirectoriesInWorkDirectory();

            FABRIC_STATE_PROVIDER_ID CreateStateProviderId() noexcept;

            void UpdateLastStateProviderId(__in FABRIC_STATE_PROVIDER_ID id) noexcept;

            ktl::Awaitable<void> CleanUpMetadataAsync(
                __in ktl::CancellationToken const & cancellationToken);

            void RemoveUnreferencedStateProviders();

            bool Contains(
                __in KArray<Metadata::CSPtr> const & metadataArray,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId);

            ktl::Awaitable<void> CloseOpenedStateProviderOnFailureAsync(
                __in Metadata & metadata) noexcept;

        private:
            ktl::Awaitable<void> InitializeStateProvidersAsync(
                __in Metadata & metadata,
                __in bool shouldMetadataBeAdded);

        private: // Single State Provider Dispatcher.
            KSharedArray<Metadata::SPtr>::SPtr InitializeStateProvidersInOrder(__in Metadata & metadata);

        private: // Multiple State Provider Dispatcher
            // Calls on the state Providers.
            // 11908658: Should be in StateProviderManager/Dispatcher.
            ktl::Awaitable<NTSTATUS> OpenStateProvidersAsync(
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> ChangeRoleOnStateProvidersAsync(
                __in FABRIC_REPLICA_ROLE newRole,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> ChangeRoleToNoneOnDeletedStateProvidersAsync(
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> CloseStateProvidersAsync(
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            void AbortStateProviders() noexcept;

        private: // Multiple State Provider Checkpoint Operation Dispatcher
            ktl::Awaitable<NTSTATUS> RecoverStateProvidersAsync(
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> RestoreStateProvidersAsync(
                __in KString const & backupDirectory,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            NTSTATUS PrepareCheckpointOnStateProviders(
                __in FABRIC_SEQUENCE_NUMBER checkpointLSN) noexcept;

            ktl::Awaitable<NTSTATUS> PerformCheckpointOnStateProvidersAsync(
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> CompleteCheckpointOnStateProvidersAsync(
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> RemoveStateOnStateProvidersAsync(
                __in ktl::CancellationToken const & cancellationToken) noexcept;

        private: // Local State Manager Operations
            ktl::Awaitable<void> RecoverStateManagerCheckpointAsync(__in ktl::CancellationToken const & cancellationToken);

            void BeginSetCurrentLocalState();

            ktl::Awaitable<void> SetCurrentLocalStateAsync(
                __in LONG64 stateRecordNumber,
                __in Utilities::OperationData const & data,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<void> CopyToLocalStateAsync(
                __in SerializableMetadata const & serializableMetadata, 
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<NTSTATUS> EndSettingCurrentLocalStateAsync(
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyOnLocalStateAsync(
                __in FABRIC_SEQUENCE_NUMBER applyLSN,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in_opt Utilities::OperationData const * const metadataPtr,
                __in_opt Utilities::OperationData const * const dataPtr);

            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyOnPrimaryAsync(
                __in FABRIC_SEQUENCE_NUMBER applyLSN,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in MetadataOperationData const & metdataOperationData,
                __in Utilities::OperationData const * const dataPtr);

            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyOnPrimaryInsertAsync(
                __in FABRIC_SEQUENCE_NUMBER applyLSN,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in MetadataOperationData const & metdataOperationData,
                __in RedoOperationData const & redoOperationData,
                __in Metadata & metadata);

            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyOnPrimaryDeleteAsync(
                __in FABRIC_SEQUENCE_NUMBER applyLSN,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in MetadataOperationData const & metdataOperationData,
                __in Metadata & metadata);

            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyOnFalseProgressAsync(
                __in FABRIC_SEQUENCE_NUMBER applyLSN,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in MetadataOperationData const & metdataOperationData);

            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyOnSecondaryAsync(
                __in FABRIC_SEQUENCE_NUMBER applyLSN,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in MetadataOperationData const & metdataOperationData,
                __in Utilities::OperationData const * const dataPtr);

            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyOnSecondaryInsertAsync(
                __in FABRIC_SEQUENCE_NUMBER applyLSN,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in MetadataOperationData const & metdataOperationData,
                __in RedoOperationData const & redoOperationData);

            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyOnSecondaryDeleteAsync(
                __in FABRIC_SEQUENCE_NUMBER applyLSN,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in MetadataOperationData const & metdataOperationData);

            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyOnRecoveryAsync(
                __in FABRIC_SEQUENCE_NUMBER applyLSN,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in MetadataOperationData const & metdataOperationData,
                __in Utilities::OperationData const * const dataPtr);

            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyOnRecoveryInsertAsync(
                __in FABRIC_SEQUENCE_NUMBER applyLSN,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in MetadataOperationData const & metdataOperationData,
                __in RedoOperationData const & redoOperationData);

            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyOnRecoveryRemoveAsync(
                __in FABRIC_SEQUENCE_NUMBER applyLSN,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in MetadataOperationData const & metdataOperationData);

            NTSTATUS UnlockOnLocalState(
                __in NamedOperationContext const & namedOPerationContext) noexcept;

        private:
            FABRIC_REPLICA_ROLE GetCurrentRole() const noexcept;

        private: // Notifications
            // Rebuild notification.
            NTSTATUS NotifyStateManagerStateChanged(
                __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, TxnReplicator::IStateProvider2::SPtr>> & stateProviders) noexcept;

            NTSTATUS NotifyStateManagerStateChanged(
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in KUri const & name,
                __in TxnReplicator::IStateProvider2 & stateProvider,
                __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
                __in NotifyStateManagerChangedAction::Enum action) noexcept;

        private: // Private static const member
            static KStringView const InsertOperation;
            static KStringView const DeleteOperation;

        private: // Const member variables
            FABRIC_STATE_PROVIDER_ID const EmptyStateProviderId = 0;

            // Version of the code.
            LONG32 const version_ = Constants::CurrentVersion;

            // Note: Today this is trace rate instead of max.
            ULONG32 const MaxRetryCount = 16;

            ULONG32 const StartingBackOffInMs = 8;

            // Max back off in milliseconds.
            ULONG32 const MaxBackOffInMs = 4 * 1024;

        private: // Initializer list initialized.
            TxnReplicator::IRuntimeFolders::SPtr const runtimeFolders_;
            Data::Utilities::IStatefulPartition::SPtr const partitionSPtr_;
            KString::CSPtr const workDirectoryPath_;
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;

            ApiDispatcher::SPtr const apiDispatcher_;
            SerializationMode::Enum const serializationMode_;

            // Note: There is a bug in the managed code that it uses IStateProvider2 
            // which is not equatable and may not implement hash function.
            // So instead, we use state provider id here.
            KArray<Metadata::CSPtr> copyProgressArray_;

            Utilities::ThreadSafeSPtrCache<TxnReplicator::IStateManagerChangeHandler> changeHandlerCache_;

            bool hasPersistedState_ = true;

        private: // In-class member initialized
            OpenParameters::CSPtr openParametersSPtr_ = nullptr;

        private: // StateProviderId Lock and variables protected by it.
            KSpinLock stateProviderIdLock_;
            FABRIC_STATE_PROVIDER_ID lastStateProviderId_ = -1;

        private: // Role Lock and variables protected by it.
            mutable KReaderWriterSpinLock roleLock_;
            bool changeRoleCompleted_ = true;
            FABRIC_REPLICA_ROLE role_ = FABRIC_REPLICA_ROLE_UNKNOWN;

        private: // Constructor initialized.
            KSharedPtr<MetadataManager> metadataManagerSPtr_;
            KSharedPtr<CheckpointManager> checkpointManagerSPtr_;

            // 11908685: Use KAsyncLock instead since it is write only and timeout is not required.
            Utilities::ReaderWriterAsyncLock::SPtr changeRoleLockSPtr_;

        private: // Initialized at initialize
            KWeakRef<TxnReplicator::ITransactionalReplicator>::SPtr transactionalReplicatorWRef_;
            KWeakRef<TxnReplicator::ILoggingReplicator>::SPtr loggingReplicatorWRef_;
        };
    }
}

#define ApiEntry() \
    if (!this->TryAcquireServiceActivity()) \
    { \
        throw Exception(SF_STATUS_OBJECT_CLOSED);\
    } \
    KFinally([&] \
    { \
        this->ReleaseServiceActivity(); \
    }) 

#define ApiEntryAsync() \
    if (!this->TryAcquireServiceActivity()) \
    { \
        co_return SF_STATUS_OBJECT_CLOSED;\
    } \
    KFinally([&] \
    { \
        this->ReleaseServiceActivity(); \
    }) 

#define ApiEntryReturn() \
    if (!this->TryAcquireServiceActivity()) \
    { \
        return SF_STATUS_OBJECT_CLOSED; \
    } \
    KFinally([&] \
    { \
        this->ReleaseServiceActivity(); \
    })
