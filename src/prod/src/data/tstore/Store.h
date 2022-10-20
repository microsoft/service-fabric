// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define STORE_TAG 'rtST'

namespace Data
{
    namespace TStore
    {

#define ApiEntry() \
        if (!this->TryAcquireServiceActivity()) \
        { \
        throw ktl:: Exception(K_STATUS_API_CLOSED); \
        } \
        KFinally([&] \
        { \
        this->ReleaseServiceActivity(); \
        })

        template <typename TKey, typename TValue>
        class Store
            : public KAsyncServiceBase
            , public KWeakRefType<Store<TKey, TValue>>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::RStore>
            , public TxnReplicator::IStateProvider2
            , public IConsolidationProvider<TKey, TValue>
            , public IStoreCopyProvider
            , public ISweepProvider
            , public IStore<TKey, TValue>
            , public IStateProviderInfo
        {
            K_FORCE_SHARED_WITH_INHERITANCE(Store)
                K_SHARED_INTERFACE_IMP(IStateProvider2)
                K_SHARED_INTERFACE_IMP(IConsolidationProvider)
                K_SHARED_INTERFACE_IMP(IStoreCopyProvider)
                K_SHARED_INTERFACE_IMP(ISweepProvider)
                K_SHARED_INTERFACE_IMP(IStore)
                K_SHARED_INTERFACE_IMP(IStateProviderInfo)

        public:
            typedef KDelegate<ULONG(const TKey & Key)> HashFunctionType;
            typedef KDelegate<bool(const TKey & Key)> FilterFunctionType;

            static NTSTATUS
                Create(
                    __in PartitionedReplicaId const & traceId,
                    __in IComparer<TKey>& keyComparer,
                    __in HashFunctionType func,
                    __in KAllocator& allocator,
                    __in KUriView & name,
                    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                    __in Data::StateManager::IStateSerializer<TKey>& keyStateSerializer,
                    __in Data::StateManager::IStateSerializer<TValue>& valueStateSerializer,
                    __out SPtr& result)
            {
                NTSTATUS status;

                SPtr output = _new(STORE_TAG, allocator) Store(traceId, keyComparer, func, name, stateProviderId, keyStateSerializer, valueStateSerializer);

                if (!output)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                status = output->Status();
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                result = Ktl::Move(output);
                return STATUS_SUCCESS;
            }

            __declspec(property(get = get_TraceComponent)) StoreTraceComponent::SPtr TraceComponent;
            StoreTraceComponent::SPtr get_TraceComponent() const
            {
                return traceComponent_;
            }

            __declspec(property(get = get_Name)) KUriView Name;
            KUriView get_Name() const
            {
                return static_cast<KUriView>(*name_);
            }

            __declspec(property(get = get_HasPersistedState)) bool HasPersistedState;
            bool get_HasPersistedState() const override
            {
                return hasPersistedState_;
            }

            __declspec(property(get = get_KeyConverter)) KSharedPtr<Data::StateManager::IStateSerializer<TKey>> KeyConverterSPtr;
            KSharedPtr<Data::StateManager::IStateSerializer<TKey>> get_KeyConverter() const override
            {
                return keyConverterSPtr_;
            }

            __declspec(property(get = get_ValueConverter)) KSharedPtr<Data::StateManager::IStateSerializer<TValue>> ValueConverterSPtr;
            KSharedPtr<Data::StateManager::IStateSerializer<TValue>> get_ValueConverter() const override
            {
                return valueConverterSPtr_;
            }

            __declspec(property(get = get_KeyComparer)) KSharedPtr<IComparer<TKey>> KeyComparerSPtr;
            KSharedPtr<IComparer<TKey>> get_KeyComparer() const override
            {
                return keyComparerSPtr_;
            }

            __declspec(property(get = get_TransactionalReplicator)) KSharedPtr<TxnReplicator::ITransactionalReplicator> TransactionalReplicatorSPtr;
            KSharedPtr<TxnReplicator::ITransactionalReplicator> get_TransactionalReplicator() const override
            {
                return GetReplicator();
            }

            __declspec(property(get = get_LockManager, put = set_LockManager)) LockManager::SPtr LockManagerSPtr;
            LockManager::SPtr get_LockManager() const override
            {
                return lockManager_;
            }

            // Exposed for testability
            void set_LockManager(LockManager::SPtr lockManager)
            {
                lockManager_ = lockManager;
            }

            __declspec(property(get = get_CurrentMetadataTable)) KSharedPtr<MetadataTable> CurrentMetadataTableSPtr;
            KSharedPtr<MetadataTable> get_CurrentMetadataTable() const override
            {
                return currentMetadataTableSPtr_.Get();
            }

            __declspec(property(get = get_MergeMetadataTable, put = set_MergeMetadataTable)) MetadataTable::SPtr MergeMetadataTableSPtr;
            MetadataTable::SPtr get_MergeMetadataTable() const override
            {
                return mergeMetadataTableSPtr_.Get();
            }

            void set_MergeMetadataTable(MetadataTable::SPtr tableSPtr) override
            {
                mergeMetadataTableSPtr_.Put(Ktl::Move(tableSPtr));
            }

            __declspec(property(get = get_NextMetadataTable)) MetadataTable::SPtr NextMetadataTableSPtr;
            MetadataTable::SPtr get_NextMetadataTable() const override
            {
                return nextMetadataTableSPtr_.Get();
            }

            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID get_StateProviderId() const override
            {
                return storeId_;
            }

            __declspec(property(get = get_SnapshotContainer)) KSharedPtr<SnapshotContainer<TKey, TValue>> SnapshotContainerSPtr;
            virtual KSharedPtr<SnapshotContainer<TKey, TValue>> get_SnapshotContainer() const override
            {
                return snapshotContainerSPtr_;
            }

            __declspec(property(get = get_IsValueAReferenceType)) bool IsValueAReferenceType;
            bool get_IsValueAReferenceType() const override
            {
                // TODO: Implement
                return false;
            }

            __declspec(property(get = get_WorkingDirectory)) KString::CSPtr WorkingDirectoryCSPtr;
            KString::CSPtr get_WorkingDirectory() const override
            {
                return workFolder_;
            }

            __declspec(property(get = get_Count)) LONG64 Count;
            LONG64 get_Count() const override
            {
                auto countSnap = count_;
                STORE_ASSERT(countSnap >= 0, "Count {1} cannot be negative", countSnap);
                return countSnap;
            }

            // Get the approximate store buffer size - data+metadata, does not account for static structures
            __declspec(property(get = get_Size)) LONG64 Size;
            LONG64 get_Size()
            {
                LONG64 size = GetMemorySize();

                // Disabling until #10584838 is resolved 
                //STORE_ASSERT(size >= 0, "Size {1} cannot be negative", size);
                return size;
            }

            __declspec(property(get = get_DictionaryChangeHandler, put = set_DictionaryChangeHandler)) KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> DictionaryChangeHandlerSPtr;
            KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> get_DictionaryChangeHandler() const override
            {
                return dictionaryChangeHandlerSPtr_.Get();
            }

            void set_DictionaryChangeHandler(KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> dictionaryChangeHandlerSPtr) override
            {
                dictionaryChangeHandlerSPtr_.Put(Ktl::Move(dictionaryChangeHandlerSPtr));
            }

            __declspec(property(get = get_DictionaryChangeHandlerMask, put = set_DictionaryChangeHandlerMask)) DictionaryChangeEventMask::Enum DictionaryChangeHandlerMask;
            virtual DictionaryChangeEventMask::Enum get_DictionaryChangeHandlerMask() const
            {
                return dictionaryChangeHandlerMask_;
            }

            virtual void set_DictionaryChangeHandlerMask(DictionaryChangeEventMask::Enum mask)
            {
                dictionaryChangeHandlerMask_ = mask;
            }

            __declspec(property(get = get_CacheThresholdInMB, put = set_CacheThresholdInMB)) LONG64 CacheThresholdInMB;
            LONG64 get_CacheThresholdInMB() const override
            {
                return sweepManagerSPtr_->MemoryBufferSize;
            }

            void set_CacheThresholdInMB(__in LONG64 sizeInMegabytes) override
            {
                sweepManagerSPtr_->MemoryBufferSize = sizeInMegabytes * 1024 * 1024;
            }

            KAllocator& get_Allocator() const override
            {
                return this->GetThisAllocator();
            }

            // This property is exposed for testability
            __declspec(property(get = get_DifferentialState)) KSharedPtr<DifferentialStoreComponent<TKey, TValue>> DifferentialState;
            virtual KSharedPtr<DifferentialStoreComponent<TKey, TValue>> get_DifferentialState() const
            {
                return differentialStoreComponentSPtr_.Get();
            }

            // This property is exposed for testability
            __declspec(property(get = get_TmpDiskMetadataFilePath)) KString::CSPtr TmpDiskMetadataFilePath;
            KString::CSPtr get_TmpDiskMetadataFilePath() const
            {
                return tmpMetadataFilePath_;
            }

            // This property is exposed for testability
            __declspec(property(get = get_CurrentDiskMetadataFilePath)) KString::CSPtr CurrentDiskMetadataFilePath;
            KString::CSPtr get_CurrentDiskMetadataFilePath() const
            {
                return currentMetadataFilePath_;
            }

            // This property is exposed for testability
            __declspec(property(get = get_ConsolidationManager)) KSharedPtr<ConsolidationManager<TKey, TValue>> ConsolidationManagerSPtr;
            KSharedPtr<ConsolidationManager<TKey, TValue>> get_ConsolidationManager() const
            {
                return consolidationManagerSPtr_;
            }

            // This property is exposed for testability
            __declspec(property(get = get_SweepManager)) KSharedPtr<SweepManager<TKey, TValue>> SweepManagerSPtr;
            KSharedPtr<SweepManager<TKey, TValue>> get_SweepManager() const
            {
                return sweepManagerSPtr_;
            }

            // This property is exposed for testability
            __declspec(property(get = get_ConsolidationTcs)) ktl::AwaitableCompletionSource<PostMergeMetadataTableInformation::SPtr>::SPtr ConsolidationTcs;
            ktl::AwaitableCompletionSource<PostMergeMetadataTableInformation::SPtr>::SPtr get_ConsolidationTcs()
            {
                return consolidationTcsSPtr_.Get();
            }

            // This property is exposed for testability
            __declspec(property(get = get_FilesToBeDeleted)) ConcurrentDictionary<FileMetadata::SPtr, bool>::SPtr FilesToBeDeletedSPtr;
            ConcurrentDictionary<FileMetadata::SPtr, bool>::SPtr get_FilesToBeDeleted() const
            {
                return filesToBeDeletedSPtr_;
            }

            __declspec(property(get = get_TestDelayOnConsolidation, put = set_TestDelayOnConsolidation)) ktl::AwaitableCompletionSource<bool>::SPtr TestDelayOnConsolidationSPtr;
            ktl::AwaitableCompletionSource<bool>::SPtr get_TestDelayOnConsolidation() const override
            {
                return testDelayOnConsolidationSPtr_;
            }

            void set_TestDelayOnConsolidation(ktl::AwaitableCompletionSource<bool>::SPtr completionSourceSPtr)
            {
                testDelayOnConsolidationSPtr_ = completionSourceSPtr;
            }

            __declspec(property(get = get_EnableBackgroundConsolidation, put = set_EnableBackgroundConsolidation)) bool EnableBackgroundConsolidation;
            bool get_EnableBackgroundConsolidation() const override
            {
                return enableBackgroundConsolidation_;
            }
            void set_EnableBackgroundConsolidation(__in bool enable)
            {
                enableBackgroundConsolidation_ = enable;
            }

            __declspec(property(get = get_EnableSweep, put = set_EnableSweep)) bool EnableSweep;
            bool get_EnableSweep() const override
            {
                return enableSweep_;
            }
            void set_EnableSweep(__in bool enable)
            {
                enableSweep_ = enable;
            }

            __declspec(property(get = get_SweepTask, put = set_SweepTask)) ktl::AwaitableCompletionSource<bool>::SPtr SweepTaskSourceSPtr;
            ktl::AwaitableCompletionSource<bool>::SPtr get_SweepTask()
            {
                return sweepTcsSPtr_.Get();
            }

            void set_SweepTask(ktl::AwaitableCompletionSource<bool>::SPtr sweepCompletionSource)
            {
                return sweepTcsSPtr_.Put(Ktl::Move(sweepCompletionSource));
            }

            __declspec(property(get = get_ShouldLoadValuesOnRecovery, put = set_ShouldLoadValuesOnRecovery)) bool ShouldLoadValuesOnRecovery;
            bool get_ShouldLoadValuesOnRecovery() const
            {
                return shouldLoadValuesInRecovery_;
            }
            void set_ShouldLoadValuesOnRecovery(__in bool shouldLoadValues)
            {
                shouldLoadValuesInRecovery_ = shouldLoadValues;
            }

            __declspec(property(get = get_MaxNumberOfInflightValueRecoveryTasks, put = set_MaxNumberOfInflightValueRecoveryTasks)) ULONG32 MaxNumberOfInflightValueRecoveryTasks;
            ULONG32 get_MaxNumberOfInflightValueRecoveryTasks() const
            {
                return numberOfInflightRecoveryTasks_;
            }
            void set_MaxNumberOfInflightValueRecoveryTasks(__in ULONG32 numRecoveryTasks)
            {
                numberOfInflightRecoveryTasks_ = numRecoveryTasks;
            }

            __declspec(property(get = get_MergeHelper)) MergeHelper::SPtr MergeHelperSPtr;
            MergeHelper::SPtr get_MergeHelper() const override
            {
                return mergeHelperSPtr_;
            }

            __declspec(property(get = get_EnableEnumerationWithRepeatableRead, put = set_EnableEnumerationWithRepeatableRead)) bool EnableEnumerationWithRepeatableRead;
            bool get_EnableEnumerationWithRepeatableRead() const
            {
                return enableEnumerationWithRepeatableRead_;
            }

            void set_EnableEnumerationWithRepeatableRead(bool enable)
            {
                enableEnumerationWithRepeatableRead_ = enable;
            }

            virtual NTSTATUS OnCreateStoreTransaction(
                __in LONG64 id,
                __in TxnReplicator::TransactionBase& transaction,
                __in LONG64 owner,
                __in ConcurrentDictionary2<LONG64, KSharedPtr<StoreTransaction<TKey, TValue>>>& container,
                __in IComparer<TKey> & keyComparer,
                __out typename StoreTransaction<TKey, TValue>::SPtr& result)
            {
                return StoreTransaction<TKey, TValue>::Create(id, transaction, owner, container, keyComparer, *traceComponent_, this->GetThisAllocator(), result);
            }

            KSharedPtr<StoreTransaction<TKey, TValue>> CreateStoreTransaction(__in TxnReplicator::TransactionBase& replicatorTransaction)
            {
                ApiEntry();

                try
                {
                    KSharedPtr<StoreTransaction<TKey, TValue>> storeTxnSPtr = nullptr;

                    NTSTATUS status = OnCreateStoreTransaction(replicatorTransaction.TransactionId, replicatorTransaction, storeId_, *inflightReadWriteStoreTransactionsSPtr_, *keyComparerSPtr_, storeTxnSPtr);
                    Diagnostics::Validate(status);

                    STORE_ASSERT(storeTxnSPtr != nullptr, "storeTxnSPtr != nullptr");
                    LONG64 transactionId = replicatorTransaction.TransactionId;
                    bool add = inflightReadWriteStoreTransactionsSPtr_->TryAdd(transactionId, storeTxnSPtr);
                    STORE_ASSERT(add == true, "add == true");

                    // Register for clear locks.
                    status = replicatorTransaction.AddLockContext(*storeTxnSPtr);
                    Diagnostics::Validate(status);

                    TxnReplicator::ITransactionalReplicator::SPtr replicator = GetReplicator();
                    auto role = replicator->Role;
                    if (role == FABRIC_REPLICA_ROLE_PRIMARY)
                    {
                        storeTxnSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;
                    }

                    return storeTxnSPtr;
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"CreateStoreTransaction", e, replicatorTransaction.TransactionId);
                    throw;
                }
            }

            //
            // Creates or retrieves a reader/writer transaction.
            //
            bool CreateOrFindTransaction(
                __in TxnReplicator::TransactionBase& replicatorTransaction,
                __out KSharedPtr<IStoreTransaction<TKey, TValue>>& result) override
            {
                ApiEntry();

                try
                {
                    KSharedPtr<StoreTransaction<TKey, TValue>> storeTransactionSPtr = nullptr;
                    bool success = inflightReadWriteStoreTransactionsSPtr_->TryGetValue(replicatorTransaction.TransactionId, storeTransactionSPtr);
                    if (!success)
                    {
                        storeTransactionSPtr = CreateStoreTransaction(replicatorTransaction);
                        result = static_cast<IStoreTransaction<TKey, TValue>*>(storeTransactionSPtr.RawPtr());
                        STORE_ASSERT(result != nullptr, "result != nullptr");
                        return false;
                    }

                    result = static_cast<IStoreTransaction<TKey, TValue>*>(storeTransactionSPtr.RawPtr());
                    STORE_ASSERT(result != nullptr, "result != nullptr");
                    return true;
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"CreateOrFindTransaction", e, replicatorTransaction.TransactionId);
                    throw;
                }
            }


            ktl::Awaitable<void> AddAsync(
                __in IStoreTransaction<TKey, TValue>& storeTransaction,
                __in TKey key,
                __in TValue value,
                __in Common::TimeSpan timeout,
                __in ktl::CancellationToken const & cancellationToken) override
            {
                ApiEntry();

                ULONG64 keyLockResourceNameHash = 0;
                KSharedPtr<StoreTransaction<TKey, TValue>> storeTransactionSPtr = static_cast<StoreTransaction<TKey, TValue>*>(&storeTransaction);

                try
                {
                    ThrowIfFaulted(*storeTransactionSPtr);
                    ThrowIfNotWritable(storeTransactionSPtr->Id);

                    // Extract key and value bytes.
                    auto keyBytes = GetKeyBytes(key);
                    auto valueBytes = GetValueBytes(value);

                    // Compute redo and undo information. This can be done outside the lock.
                    KSharedPtr<MetadataOperationDataKV<TKey, TValue> const> metadataCSPtr = nullptr;
                    NTSTATUS status = MetadataOperationDataKV<TKey, TValue>::Create(
                        key,
                        value,
                        Constants::SerializedVersion,
                        StoreModificationType::Enum::Add,
                        storeTransactionSPtr->Id,
                        keyBytes,
                        this->GetThisAllocator(),
                        metadataCSPtr);
                    Diagnostics::Validate(status);

                    RedoUndoOperationData::SPtr redoDataSPtr = nullptr;
                    status = RedoUndoOperationData::Create(this->GetThisAllocator(), valueBytes, nullptr, redoDataSPtr);
                    Diagnostics::Validate(status);

                    OperationData::SPtr redoSPtr = redoDataSPtr.DownCast<OperationData>();
                    OperationData::SPtr undoSPtr = nullptr;

                    keyLockResourceNameHash = GetHash(*keyBytes);

                    Common::TimeSpan primeLockTimeout = timeout;
                    if (primeLockTimeout < Common::TimeSpan::Zero)
                    {
                        primeLockTimeout = Common::TimeSpan::MaxValue;
                    }

                    co_await storeTransactionSPtr->AcquirePrimeLockAsync(
                        *lockManager_,
                        LockMode::Shared,
                        primeLockTimeout,
                        false);

                    // If the operation was cancelled during the lock wait, then terminate
                    cancellationToken.ThrowIfCancellationRequested();

                    ThrowIfFaulted(*storeTransactionSPtr);

                    co_await AcquireKeyModificationLockAsync(
                        *lockManager_,
                        StoreModificationType::Enum::Add,
                        keyLockResourceNameHash,
                        *storeTransactionSPtr,
                        timeout);

                    // If the operation was cancelled during the lock wait, then terminate
                    cancellationToken.ThrowIfCancellationRequested();

                    // CanKeyBeAdded check.
                    if (!CanKeyBeAdded(*storeTransactionSPtr, func_, key))
                    {
                        StoreEventSource::Events->StoreAddAsyncError(
                            traceComponent_->PartitionId, traceComponent_->TraceTag,
                            keyLockResourceNameHash,
                            storeTransactionSPtr->Id);

                        //key already exists.
                        throw ktl::Exception(SF_STATUS_WRITE_CONFLICT);
                    }

                    KSharedPtr<InsertedVersionedItem<TValue>> insertedVersion = nullptr;
                    status = InsertedVersionedItem<TValue>::Create(this->GetThisAllocator(), insertedVersion);
                    if (!NT_SUCCESS(status))
                    {
                        throw ktl::Exception(status);
                    }

                    // Replicate
                    co_await ReplicateOperationAsync(*storeTransactionSPtr, *metadataCSPtr, redoSPtr, undoSPtr, timeout, cancellationToken);

                    StoreEventSource::Events->StoreAddAsync(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        storeTransactionSPtr->Id,
                        keyLockResourceNameHash);

                    insertedVersion->Initialize(value);
                    insertedVersion->SetValueSize(GetValueSize(valueBytes));
                    auto component = storeTransactionSPtr->GetComponent(func_);
                    STORE_ASSERT(component != nullptr, "component != nullptr");
                    component->Add(false, key, *insertedVersion);
                }
                catch (ktl::Exception & e)
                {
                    TraceException(L"AddAsync", e, storeTransactionSPtr->Id, keyLockResourceNameHash);
                    throw;
                }

                co_return;
            }

            ktl::Awaitable<bool> ConditionalUpdateAsync(
                __in IStoreTransaction<TKey, TValue>& storeTransaction,
                __in TKey key,
                __in TValue value,
                __in Common::TimeSpan timeout,
                __in ktl::CancellationToken const & cancellationToken,
                __in_opt LONG64 conditionalVersion = -1) override
            {
                ApiEntry();

                KSharedPtr<StoreTransaction<TKey, TValue>> storeTransactionSPtr = static_cast<StoreTransaction<TKey, TValue>*>(&storeTransaction);
                ULONG64 keyLockResourceNameHash = 0;

                try
                {
                    ThrowIfFaulted(*storeTransactionSPtr);
                    ThrowIfNotWritable(storeTransactionSPtr->Id);

                    // Extract key and value bytes.
                    auto keyBytes = GetKeyBytes(key);
                    auto valueBytes = GetValueBytes(value);

                    // Compute redo and undo information. This can be done outside the lock.
                    KSharedPtr<MetadataOperationDataKV<TKey, TValue> const> metadataCSPtr = nullptr;
                    NTSTATUS status = MetadataOperationDataKV<TKey, TValue>::Create(
                        key,
                        value,
                        Constants::SerializedVersion,
                        StoreModificationType::Enum::Update,
                        storeTransactionSPtr->Id,
                        keyBytes,
                        this->GetThisAllocator(),
                        metadataCSPtr);
                    Diagnostics::Validate(status);

                    RedoUndoOperationData::SPtr redoDataSPtr = nullptr;
                    status = RedoUndoOperationData::Create(this->GetThisAllocator(), valueBytes, nullptr, redoDataSPtr);
                    Diagnostics::Validate(status);

                    OperationData::SPtr redoSPtr = redoDataSPtr.DownCast<OperationData>();
                    OperationData::SPtr undoSPtr = nullptr;

                    keyLockResourceNameHash = GetHash(*keyBytes);

                    Common::TimeSpan primeLockTimeout = timeout;
                    if (primeLockTimeout < Common::TimeSpan::Zero)
                    {
                        primeLockTimeout = Common::TimeSpan::MaxValue;
                    }

                    co_await storeTransactionSPtr->AcquirePrimeLockAsync(
                        *lockManager_,
                        LockMode::Shared,
                        primeLockTimeout,
                        false);

                    // If the operation was cancelled during the lock wait, then terminate
                    cancellationToken.ThrowIfCancellationRequested();

                    ThrowIfFaulted(*storeTransactionSPtr);

                    co_await AcquireKeyModificationLockAsync(
                        *lockManager_,
                        StoreModificationType::Enum::Update,
                        keyLockResourceNameHash,
                        *storeTransactionSPtr,
                        timeout);

                    // If the operation was cancelled during the lock wait, then terminate
                    cancellationToken.ThrowIfCancellationRequested();

                    LONG64 currentVersion = Constants::InvalidLsn;
                    if (!CanKeyBeUpdatedOrDeleted(*storeTransactionSPtr, conditionalVersion, key, currentVersion))
                    {
                        bool isVersionMismatch = conditionalVersion > -1 && currentVersion != conditionalVersion;
                        if (isVersionMismatch)
                        {
                            StoreEventSource::Events->StoreConditionalUpdateAsyncVersionMismatch(
                                traceComponent_->PartitionId, traceComponent_->TraceTag,
                                keyLockResourceNameHash,
                                storeTransactionSPtr->Id,
                                currentVersion,
                                conditionalVersion);
                        }
                        else
                        {
                            StoreEventSource::Events->StoreConditionalUpdateAsyncKeyDoesNotExist(
                                traceComponent_->PartitionId, traceComponent_->TraceTag,
                                keyLockResourceNameHash,
                                storeTransactionSPtr->Id);
                        }

                        co_return false;
                    }

                    KSharedPtr<UpdatedVersionedItem<TValue>> updatedVersion = nullptr;
                    status = UpdatedVersionedItem<TValue>::Create(this->GetThisAllocator(), updatedVersion);
                    Diagnostics::Validate(status);

                    // Replicate
                    co_await ReplicateOperationAsync(*storeTransactionSPtr, *metadataCSPtr, redoSPtr, undoSPtr, timeout, cancellationToken);

                    StoreEventSource::Events->StoreConditionalUpdateAsync(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        -1,
                        storeTransactionSPtr->Id,
                        keyLockResourceNameHash);

                    updatedVersion->Initialize(value);
                    updatedVersion->SetValueSize(GetValueSize(valueBytes));
                    auto component = storeTransactionSPtr->GetComponent(func_);
                    STORE_ASSERT(component != nullptr, "component != nullptr");
                    component->Add(false, key, *updatedVersion);
                    co_return true;
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"ConditionalUpdateAsync", e, storeTransactionSPtr->Id, keyLockResourceNameHash);
                    throw;
                }
            }

            ktl::Awaitable<bool> ConditionalRemoveAsync(
                __in IStoreTransaction<TKey, TValue>& storeTransaction,
                __in TKey key,
                __in Common::TimeSpan timeout,
                __in ktl::CancellationToken const & cancellationToken,
                __in_opt LONG64 conditionalVersion = -1) override
            {
                ApiEntry();

                KSharedPtr<StoreTransaction<TKey, TValue>> storeTransactionSPtr = static_cast<StoreTransaction<TKey, TValue>*>(&storeTransaction);
                ULONG64 keyLockResourceNameHash = 0;

                try
                {
                    ThrowIfFaulted(*storeTransactionSPtr);
                    ThrowIfNotWritable(storeTransactionSPtr->Id);

                    // Extract key and value bytes.
                    auto keyBytes = GetKeyBytes(key);

                    // Compute redo and undo information. This can be done outside the lock.
                    KSharedPtr<MetadataOperationDataK<TKey> const> metadataCSPtr = nullptr;
                    NTSTATUS status = MetadataOperationDataK<TKey>::Create(
                        key,
                        Constants::SerializedVersion,
                        StoreModificationType::Enum::Remove,
                        storeTransactionSPtr->Id,
                        keyBytes,
                        this->GetThisAllocator(),
                        metadataCSPtr);
                    Diagnostics::Validate(status);

                    RedoUndoOperationData::SPtr redoDataSPtr = nullptr;
                    status = RedoUndoOperationData::Create(this->GetThisAllocator(), nullptr, nullptr, redoDataSPtr);
                    Diagnostics::Validate(status);

                    OperationData::SPtr redoSPtr = static_cast<OperationData *>(redoDataSPtr.RawPtr());
                    OperationData::SPtr undoSPtr = nullptr;

                    keyLockResourceNameHash = GetHash(*keyBytes);

                    Common::TimeSpan primeLockTimeout = timeout;
                    if (primeLockTimeout < Common::TimeSpan::Zero)
                    {
                        primeLockTimeout = Common::TimeSpan::MaxValue;
                    }

                    co_await storeTransactionSPtr->AcquirePrimeLockAsync(
                        *lockManager_,
                        LockMode::Shared,
                        primeLockTimeout,
                        false);

                    // If the operation was cancelled during the lock wait, then terminate
                    cancellationToken.ThrowIfCancellationRequested();

                    ThrowIfFaulted(*storeTransactionSPtr);

                    co_await AcquireKeyModificationLockAsync(
                        *lockManager_,
                        StoreModificationType::Enum::Remove,
                        keyLockResourceNameHash,
                        *storeTransactionSPtr,
                        timeout);

                    // If the operation was cancelled during the lock wait, then terminate
                    cancellationToken.ThrowIfCancellationRequested();

                    LONG64 currentVersion = Constants::InvalidLsn;
                    if (!CanKeyBeUpdatedOrDeleted(*storeTransactionSPtr, conditionalVersion, key, currentVersion))
                    {
                        bool isVersionMismatch = conditionalVersion > -1 && conditionalVersion != currentVersion;
                        if (isVersionMismatch)
                        {
                            StoreEventSource::Events->StoreConditionalRemoveAsyncVersionMismatch(
                                traceComponent_->PartitionId, traceComponent_->TraceTag,
                                keyLockResourceNameHash,
                                storeTransactionSPtr->Id,
                                currentVersion,
                                conditionalVersion);
                        }
                        else
                        {
                            StoreEventSource::Events->StoreConditionalRemoveAsyncKeyDoesNotExist(
                                traceComponent_->PartitionId, traceComponent_->TraceTag,
                                keyLockResourceNameHash,
                                storeTransactionSPtr->Id);
                        }

                        co_return false;
                    }

                    KSharedPtr<DeletedVersionedItem<TValue>> deletedVersion = nullptr;
                    status = DeletedVersionedItem<TValue>::Create(this->GetThisAllocator(), deletedVersion);
                    Diagnostics::Validate(status);

                    co_await ReplicateOperationAsync(*storeTransactionSPtr, *metadataCSPtr, redoSPtr, undoSPtr, timeout, cancellationToken);

                    StoreEventSource::Events->StoreConditonalRemoveAsync(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        -1,
                        storeTransactionSPtr->Id,
                        keyLockResourceNameHash);

                    deletedVersion->Initialize();
                    auto component = storeTransactionSPtr->GetComponent(func_);
                    STORE_ASSERT(component != nullptr, "component != nullptr");
                    component->Add(false, key, *deletedVersion);
                    co_return true;
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"ConditionalRemoveAsync", e, storeTransactionSPtr->Id, keyLockResourceNameHash);
                    throw;
                }
            }

            ktl::Awaitable<bool> ConditionalGetAsync(
                __in IStoreTransaction<TKey, TValue>& storeTransaction,
                __in TKey key,
                __in Common::TimeSpan timeout,
                __out KeyValuePair<LONG64, TValue>& value,
                __in ktl::CancellationToken const & cancellationToken) override
            {
                ApiEntry();
                bool exists = co_await TryGetValueAsync(storeTransaction, key, timeout, value, ReadMode::CacheResult, cancellationToken);
                co_return exists;
            }

            ktl::Awaitable<bool> ContainsKeyAsync(
                __in IStoreTransaction<TKey, TValue>& storeTransaction,
                __in TKey key,
                __in Common::TimeSpan timeout,
                __in ktl::CancellationToken const & cancellationToken) noexcept override
            {
                ApiEntry();
                KeyValuePair<LONG64, TValue> dummyValue;
                bool exists = co_await TryGetValueAsync(storeTransaction, key, timeout, dummyValue, ReadMode::Off, cancellationToken);
                co_return exists;
            }

            ktl::Awaitable<bool> TryGetValueAsync(
                __in IStoreTransaction<TKey, TValue>& storeTransaction,
                __in TKey key,
                __in Common::TimeSpan timeout,
                __out KeyValuePair<LONG64, TValue>& value,
                __in ReadMode readMode,
                __in ktl::CancellationToken const & cancellationToken)
            {
                KSharedPtr<StoreTransaction<TKey, TValue>> storeTransactionSPtr = static_cast<StoreTransaction<TKey, TValue>*>(&storeTransaction);
                ULONG64 keyLockResourceNameHash = 0;

                try
                {
                    KSharedPtr<DifferentialStoreComponent<TKey, TValue>> componentSPtr = differentialStoreComponentSPtr_.Get();

                    ThrowIfFaulted(*storeTransactionSPtr);
                    ThrowIfNotReadable(*storeTransactionSPtr);

                    Common::TimeSpan primeLockTimeout = timeout;
                    if (primeLockTimeout < Common::TimeSpan::Zero)
                    {
                        primeLockTimeout = Common::TimeSpan::MaxValue;
                    }
                    co_await storeTransactionSPtr->AcquirePrimeLockAsync(*lockManager_, LockMode::Shared, primeLockTimeout, false);

                    // If the operation was cancelled during the lock wait, then terminate
                    cancellationToken.ThrowIfCancellationRequested();

                    ThrowIfFaulted(*storeTransactionSPtr);

                    KSharedPtr<VersionedItem<TValue>> versionedItemSPtr = nullptr;
                    KSharedPtr<StoreComponentReadResult<TValue>> readResultSPtr = nullptr;

                    auto keyBytes = GetKeyBytes(key);
                    keyLockResourceNameHash = GetHash(*keyBytes);

                    if (!storeTransactionSPtr->IsWriteSetEmpty)
                    {
                        auto writeset = storeTransactionSPtr->GetComponent(func_);
                        STORE_ASSERT(writeset != nullptr, "writeset != nullptr");
                        versionedItemSPtr = writeset->Read(key);

                        if (versionedItemSPtr != nullptr)
                        {
                            if (versionedItemSPtr->GetRecordKind() == RecordKind::DeletedVersion)
                            {
                                StoreEventSource::Events->StoreTryGetValueAsyncNotFound(
                                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                                    storeTransactionSPtr->Id,
                                    keyLockResourceNameHash,
                                    static_cast<ULONG32>(storeTransactionSPtr->ReadIsolationLevel),
                                    -1,
                                    L"keyfounddeleted");

                                value.Key = versionedItemSPtr->GetVersionSequenceNumber();
                                value.Value = TValue();
                                co_return false;
                            }
                            else
                            {
                                // Key exists
                                // Safe to get the value from versioned item since it is in the write set
                                value.Key = versionedItemSPtr->GetVersionSequenceNumber();
                                value.Value = versionedItemSPtr->GetValue();
                                co_return true;
                            }
                        }
                    }

                    if (versionedItemSPtr == nullptr)
                    {
                        if (storeTransactionSPtr->ReadIsolationLevel == StoreTransactionReadIsolationLevel::Enum::Snapshot)
                        {
                            TxnReplicator::Transaction::SPtr transaction = static_cast<TxnReplicator::Transaction *>(storeTransactionSPtr->ReplicatorTransaction.RawPtr());
                            LONG64 visibilitySequenceNumber;
                            Diagnostics::Validate(co_await transaction->GetVisibilitySequenceNumberAsync(visibilitySequenceNumber));

                            readResultSPtr = co_await TryGetValueForReadOnlyTransactionsAsync(key, visibilitySequenceNumber, true, readMode, cancellationToken);
                            versionedItemSPtr = readResultSPtr->VersionedItem;

                            // Check if transaction has been disposed since replicator can dispose  a long running tx or 
                            // on change role and it can race with a read, 
                            // so checking here coz read from the snapshot container has already completed.
                            ThrowIfFaulted(*storeTransactionSPtr);
                        }
                        else
                        {
                            STORE_ASSERT(storeTransactionSPtr->ReadIsolationLevel == StoreTransactionReadIsolationLevel::Enum::ReadRepeatable,
                                //|| storeTransactionSPtr->ReadIsolationLevel == StoreTransactionReadIsolationLevel::Enum::ReadCommitted,
                                "store transaction should be read committed or repeatable read");
                            co_await AcquireKeyReadLockAsync(*lockManager_, keyLockResourceNameHash, *storeTransactionSPtr, timeout);

                            // If the operation was cancelled during the lock wait, then terminate
                            cancellationToken.ThrowIfCancellationRequested();

                            // todo read past lock
                            readResultSPtr = co_await TryGetValueForReadOnlyTransactionsAsync(key, -1, false, readMode, cancellationToken);
                            versionedItemSPtr = readResultSPtr->VersionedItem;

                            //if (storeTransactionSPtr->ReadIsolationLevel == StoreTransactionReadIsolationLevel::Enum::ReadCommitted)
                            //{
                            //   lockManager_->ReleaseLock(readLock);
                            //}
                        }
                    }

                    // Make sure a read does not start in primary role and completes in secondary role.
                    ThrowIfNotReadable(*storeTransactionSPtr);


                    if (versionedItemSPtr != nullptr)
                    {
                        if (versionedItemSPtr->GetRecordKind() == RecordKind::DeletedVersion)
                        {
                            StoreEventSource::Events->StoreTryGetValueAsyncNotFound(
                                traceComponent_->PartitionId, traceComponent_->TraceTag,
                                storeTransactionSPtr->Id,
                                keyLockResourceNameHash,
                                static_cast<ULONG32>(storeTransactionSPtr->ReadIsolationLevel),
                                -1,
                                L"keyfounddeleted");
                            co_return false;
                        }
                        else
                        {
                            STORE_ASSERT(readResultSPtr != nullptr, "Read result should not be null");
                            STORE_ASSERT(readResultSPtr->HasValue(), "Read result should have a value");
                            value.Key = versionedItemSPtr->GetVersionSequenceNumber();
                            value.Value = readResultSPtr->Value;
                            co_return true;
                        }
                    }

                    StoreEventSource::Events->StoreTryGetValueAsyncNotFound(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        storeTransactionSPtr->Id,
                        keyLockResourceNameHash,
                        static_cast<ULONG32>(storeTransactionSPtr->ReadIsolationLevel),
                        -1,
                        L"keydoesnotexist");
                    co_return false;
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"TryGetValueAsync", e, storeTransactionSPtr->Id, keyLockResourceNameHash);
                    throw;
                }
            }

            ktl::Awaitable<void> BackupCheckpointAsync(
                __in KString const & backupDirectory,
                __in ktl::CancellationToken const & cancellationToken) override
            {
                ApiEntry();

                KString & backupDirectoryCast = const_cast<KString&>(backupDirectory);
                KString::SPtr backupDirectorySPtr = &backupDirectoryCast;
                SharedException::CSPtr exceptionSPtr = nullptr;

                StoreEventSource::Events->StoreBackupCheckpointAsyncApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    Data::Utilities::ToStringLiteral(*backupDirectorySPtr),
                    L"starting");

                // Argument checks.
                ULONG64 numberOfItems = Common::Directory::GetSubDirectories(backupDirectorySPtr->operator LPCWSTR()).size() + Common::Directory::GetFiles(backupDirectorySPtr->operator LPCWSTR(), L"*", false, true).size();
                if (numberOfItems != 0)
                {
                    throw ktl::Exception(STATUS_DIRECTORY_NOT_EMPTY);
                }

                // Consistency checks.
                STORE_ASSERT(Common::File::Exists(this->CurrentDiskMetadataFilePath->operator LPCWSTR()), "no metadata table exists on disk");

                // Read the current state on-disk state into memory.
                // Complete checkpoint never runs where there is a back-up call, so it is safe to do this here.
                MetadataTable::SPtr snapshotMetadataTableSPtr = nullptr;
                NTSTATUS status = MetadataTable::Create(this->GetThisAllocator(), snapshotMetadataTableSPtr);
                Diagnostics::Validate(status);
                try
                {
                    co_await MetadataManager::OpenAsync(*snapshotMetadataTableSPtr, *this->CurrentDiskMetadataFilePath, this->GetThisAllocator(), *traceComponent_);

                    // Backup the current metadata table.
                    auto backupMetadataFileName = Common::Path::Combine(backupDirectorySPtr->operator LPCWSTR(), CurrentDiskMetadataFileName.operator LPCWSTR());
                    auto fileCopy = Common::File::Copy(this->CurrentDiskMetadataFilePath->operator LPCWSTR(), backupMetadataFileName, false);
                    if (fileCopy.IsSuccess() == false)
                    {
                        throw ktl::Exception(fileCopy.ToHResult());
                    }

                    // Backup all currently referenced tables.
                    auto enumerator = snapshotMetadataTableSPtr->Table->GetEnumerator();
                    while (enumerator->MoveNext())
                    {
                        cancellationToken.ThrowIfCancellationRequested();

                        auto fileMetadataSPtr = enumerator->Current().Value;

                        // Copy key file.
                        KString::SPtr keyFileNameSPtr = nullptr;
                        status = KString::Create(keyFileNameSPtr, this->GetThisAllocator(), *fileMetadataSPtr->FileName);
                        Diagnostics::Validate(status);
                        status = keyFileNameSPtr->Concat(KeyCheckpointFile::GetFileExtension());
                        Diagnostics::Validate(status);

                        auto fullKeyCheckpointFileName = Common::Path::Combine(workFolder_->operator LPCWSTR(), keyFileNameSPtr->operator LPCWSTR());
                        STORE_ASSERT(Common::File::Exists(fullKeyCheckpointFileName), "Unexpected missing key file: {1}", fullKeyCheckpointFileName);
                        auto backupKeyFileName = Common::Path::Combine(backupDirectorySPtr->operator LPCWSTR(), keyFileNameSPtr->operator LPCWSTR());

                        StoreEventSource::Events->StoreBackupCheckpointAsyncFile(
                            traceComponent_->PartitionId, traceComponent_->TraceTag,
                            Data::Utilities::ToStringLiteral(*backupDirectorySPtr),
                            Data::Utilities::ToStringLiteral(KStringView(fullKeyCheckpointFileName.c_str())),
                            Data::Utilities::ToStringLiteral(KStringView(backupKeyFileName.c_str())));

                        fileCopy = Common::File::Copy(fullKeyCheckpointFileName, backupKeyFileName, false);
                        if (fileCopy.IsSuccess() == false)
                        {
                            throw ktl::Exception(fileCopy.ToHResult());
                        }

                        // Copy value file.
                        KString::SPtr valueFileNameSPtr = nullptr;
                        status = KString::Create(valueFileNameSPtr, this->GetThisAllocator(), *fileMetadataSPtr->FileName);
                        Diagnostics::Validate(status);
                        status = valueFileNameSPtr->Concat(ValueCheckpointFile::GetFileExtension());
                        Diagnostics::Validate(status);

                        auto fullValueCheckpointFileName = Common::Path::Combine(workFolder_->operator LPCWSTR(), valueFileNameSPtr->operator LPCWSTR());
                        STORE_ASSERT(Common::File::Exists(fullValueCheckpointFileName), "Unexpected missing value file: {1}", fullValueCheckpointFileName);
                        auto backupValueFileName = Common::Path::Combine(backupDirectorySPtr->operator LPCWSTR(), valueFileNameSPtr->operator LPCWSTR());

                        StoreEventSource::Events->StoreBackupCheckpointAsyncFile(
                            traceComponent_->PartitionId, traceComponent_->TraceTag,
                            Data::Utilities::ToStringLiteral(*backupDirectorySPtr),
                            Data::Utilities::ToStringLiteral(KStringView(fullValueCheckpointFileName.c_str())),
                            Data::Utilities::ToStringLiteral(KStringView(backupValueFileName.c_str())));


                        fileCopy = Common::File::Copy(fullValueCheckpointFileName, backupValueFileName, false);
                        if (fileCopy.IsSuccess() == false)
                        {
                            throw ktl::Exception(fileCopy.ToHResult());
                        }
                    }
                }
                catch (ktl::Exception const & e)
                {
                    exceptionSPtr = SharedException::Create(e, GetThisAllocator());
                }

                co_await snapshotMetadataTableSPtr->CloseAsync();

                if (exceptionSPtr != nullptr)
                {
                    StoreEventSource::Events->StoreBackupCheckpointAsyncError(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        exceptionSPtr->Info.GetStatus());
                    //clang compiler error, needs to assign before throw.
                    auto ex = exceptionSPtr->Info;
                    throw ex;
                }

                StoreEventSource::Events->StoreBackupCheckpointAsyncApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    Data::Utilities::ToStringLiteral(*backupDirectorySPtr),
                    L"completed");
            }

            ktl::Awaitable<void> RestoreCheckpointAsync(
                __in KString const & backupDirectory,
                __in ktl::CancellationToken const & cancellationToken) override
            {
                ApiEntry();
                KString & backupDirectoryCast = const_cast<KString&>(backupDirectory);
                KString::SPtr backupDirectorySPtr = &backupDirectoryCast;
                SharedException::CSPtr exceptionSPtr = nullptr;

                StoreEventSource::Events->StoreRestoreCheckpointAsyncApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    Data::Utilities::ToStringLiteral(*backupDirectorySPtr),
                    L"started");

                KString::SPtr backupMetadataTableFilePathSPtr = nullptr;
                NTSTATUS status = KString::Create(backupMetadataTableFilePathSPtr, this->GetThisAllocator(), *backupDirectorySPtr);
                Diagnostics::Validate(status);

                auto concatSuccess = backupMetadataTableFilePathSPtr->Concat(Common::Path::GetPathSeparatorWstr().c_str());
                STORE_ASSERT(concatSuccess == TRUE, "Should have concatenated");
                concatSuccess = backupMetadataTableFilePathSPtr->Concat(CurrentDiskMetadataFileName);
                STORE_ASSERT(concatSuccess == TRUE, "Should have concatenated");

                MetadataTable::SPtr metadataTableSPtr = nullptr;
                status = MetadataTable::Create(this->GetThisAllocator(), metadataTableSPtr);
                Diagnostics::Validate(status);

                try
                {
                    // Validate the backup.
                    if (!Common::File::Exists(backupMetadataTableFilePathSPtr->operator LPCWSTR()))
                    {
                        throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
                    }

                    // Validate the backup contains all tables referenced in the metadata table.
                    co_await MetadataManager::OpenAsync(*metadataTableSPtr, *backupMetadataTableFilePathSPtr, this->GetThisAllocator(), *traceComponent_);
                    auto enumerator = metadataTableSPtr->Table->GetEnumerator();
                    while (enumerator->MoveNext())
                    {
                        cancellationToken.ThrowIfCancellationRequested();

                        auto fileMetadataSPtr = enumerator->Current().Value;

                        KString::SPtr keyFileNameSPtr = nullptr;
                        status = KString::Create(keyFileNameSPtr, this->GetThisAllocator(), *fileMetadataSPtr->FileName);
                        Diagnostics::Validate(status);
                        status = keyFileNameSPtr->Concat(KeyCheckpointFile::GetFileExtension());
                        Diagnostics::Validate(status);

                        KString::SPtr valueFileNameSPtr = nullptr;
                        status = KString::Create(valueFileNameSPtr, this->GetThisAllocator(), *fileMetadataSPtr->FileName);
                        Diagnostics::Validate(status);
                        status = valueFileNameSPtr->Concat(ValueCheckpointFile::GetFileExtension());
                        Diagnostics::Validate(status);

                        auto keyFile = Common::Path::Combine(backupDirectorySPtr->operator LPCWSTR(), keyFileNameSPtr->operator LPCWSTR());
                        auto valueFile = Common::Path::Combine(backupDirectorySPtr->operator LPCWSTR(), valueFileNameSPtr->operator LPCWSTR());

                        if (!Common::File::Exists(keyFile))
                        {
                            throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
                        }

                        if (!Common::File::Exists(valueFile))
                        {
                            throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
                        }
                    }

                    // Consistency checks.  preethas: Asserts temporarily disabled until disk leak is figured out.
                    STORE_ASSERT(!Common::Directory::Exists(this->CurrentDiskMetadataFilePath->operator LPCWSTR()), "state manager should already have cleared all state from all state providers via ChangeRole(None)");
                    STORE_ASSERT(!Common::Directory::Exists(this->TmpDiskMetadataFilePath->operator LPCWSTR()), "state manager should already have cleared all state from all state providers via ChangeRole(None)");

                    // Restore the current master table from the backup, and all referenced tables.
                    StoreEventSource::Events->StoreRestoreCheckpointAsyncFile(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        Data::Utilities::ToStringLiteral(*backupDirectorySPtr),
                        Data::Utilities::ToStringLiteral(*backupMetadataTableFilePathSPtr),
                        Data::Utilities::ToStringLiteral(*this->CurrentDiskMetadataFilePath));
                    auto fileCopy = Common::File::Copy(backupMetadataTableFilePathSPtr->operator LPCWSTR(), this->CurrentDiskMetadataFilePath->operator LPCWSTR(), true);
                    if (fileCopy.IsSuccess() == false)
                    {
                        throw ktl::Exception(fileCopy.ToHResult());
                    }

                    // Restore all references tables.
                    auto enumerator2 = metadataTableSPtr->Table->GetEnumerator();
                    while (enumerator2->MoveNext())
                    {
                        cancellationToken.ThrowIfCancellationRequested();

                        auto fileMetadataSPtr = enumerator2->Current().Value;

                        KString::SPtr keyFileNameSPtr = nullptr;
                        status = KString::Create(keyFileNameSPtr, this->GetThisAllocator(), *fileMetadataSPtr->FileName);
                        Diagnostics::Validate(status);
                        status = keyFileNameSPtr->Concat(KeyCheckpointFile::GetFileExtension());
                        Diagnostics::Validate(status);

                        KString::SPtr valueFileNameSPtr = nullptr;
                        status = KString::Create(valueFileNameSPtr, this->GetThisAllocator(), *fileMetadataSPtr->FileName);
                        Diagnostics::Validate(status);
                        status = valueFileNameSPtr->Concat(ValueCheckpointFile::GetFileExtension());
                        Diagnostics::Validate(status);

                        auto keyFileName = Common::Path::Combine(workFolder_->operator LPCWSTR(), keyFileNameSPtr->operator LPCWSTR());
                        auto valueFileName = Common::Path::Combine(workFolder_->operator LPCWSTR(), valueFileNameSPtr->operator LPCWSTR());

                        auto backupKeyFileName = Common::Path::Combine(backupDirectorySPtr->operator LPCWSTR(), keyFileNameSPtr->operator LPCWSTR());
                        auto backupValueFileName = Common::Path::Combine(backupDirectorySPtr->operator LPCWSTR(), valueFileNameSPtr->operator LPCWSTR());

                        StoreEventSource::Events->StoreRestoreCheckpointAsyncFile(
                            traceComponent_->PartitionId, traceComponent_->TraceTag,
                            Data::Utilities::ToStringLiteral(*backupDirectorySPtr),
                            Data::Utilities::ToStringLiteral(KStringView(backupKeyFileName.c_str())),
                            Data::Utilities::ToStringLiteral(KStringView(keyFileName.c_str())));

                        fileCopy = Common::File::Copy(backupKeyFileName, keyFileName, true);
                        if (fileCopy.IsSuccess() == false)
                        {
                            throw ktl::Exception(fileCopy.ToHResult());
                        }

                        StoreEventSource::Events->StoreRestoreCheckpointAsyncFile(
                            traceComponent_->PartitionId, traceComponent_->TraceTag,
                            Data::Utilities::ToStringLiteral(*backupDirectorySPtr),
                            Data::Utilities::ToStringLiteral(KStringView(backupValueFileName.c_str())),
                            Data::Utilities::ToStringLiteral(KStringView(valueFileName.c_str())));

                        fileCopy = Common::File::Copy(backupValueFileName, valueFileName, true);
                        if (fileCopy.IsSuccess() == false)
                        {
                            throw ktl::Exception(fileCopy.ToHResult());
                        }
                    }
                }
                catch (ktl::Exception const & e)
                {
                    exceptionSPtr = SharedException::Create(e, GetThisAllocator());
                }

                co_await metadataTableSPtr->CloseAsync();

                if (exceptionSPtr != nullptr)
                {
                    StoreEventSource::Events->StoreRestoreCheckpointAsyncError(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        exceptionSPtr->Info.GetStatus());
                    //clang compiler error, needs to assign before throw.
                    auto ex = exceptionSPtr->Info;
                    throw ex;
                }

                StoreEventSource::Events->StoreRestoreCheckpointAsyncApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    Data::Utilities::ToStringLiteral(*backupDirectorySPtr),
                    L"completed");
            }

            ktl::Awaitable<KSharedPtr<IEnumerator<TKey>>> CreateKeyEnumeratorAsync(__in IStoreTransaction<TKey, TValue> & storeTransaction) override
            {
                ApiEntry();

                if (storeTransaction.ReadIsolationLevel != StoreTransactionReadIsolationLevel::Snapshot)
                {
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                TKey defaultKey;
                return CreateKeyEnumeratorAsync(storeTransaction, defaultKey, false, defaultKey, false);
            }

            ktl::Awaitable<KSharedPtr<IEnumerator<TKey>>> CreateKeyEnumeratorAsync(__in IStoreTransaction<TKey, TValue> & storeTransaction, TKey firstKey) override
            {
                ApiEntry();

                if (storeTransaction.ReadIsolationLevel != StoreTransactionReadIsolationLevel::Snapshot)
                {
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                TKey defaultKey;
                return CreateKeyEnumeratorAsync(storeTransaction, firstKey, true, defaultKey, false);
            }

            ktl::Awaitable<KSharedPtr<IEnumerator<TKey>>> CreateKeyEnumeratorAsync(__in IStoreTransaction<TKey, TValue> & storeTransaction, TKey firstKey, TKey lastKey) override
            {
                ApiEntry();

                if (storeTransaction.ReadIsolationLevel != StoreTransactionReadIsolationLevel::Snapshot)
                {
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                if (keyComparerSPtr_->Compare(firstKey, lastKey) > 0)
                {
                    throw ktl::Exception(STATUS_INVALID_PARAMETER_3);
                }

                try
                {
                    return CreateKeyEnumeratorAsync(storeTransaction, firstKey, true, lastKey, true);
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"CreateKeyEnumeratorAsync", e);
                    throw;
                }
            }

            ktl::Awaitable<KSharedPtr<IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>>> CreateEnumeratorAsync(
                __in IStoreTransaction<TKey, TValue> & storeTransaction,
                __in ReadMode readMode=ReadMode::ReadValue) override
            {
                ApiEntry();

                TKey defaultValue = TKey();
                return CreateEnumeratorAsync(storeTransaction, defaultValue, false, defaultValue, false, readMode);
            }

            ktl::Awaitable<KSharedPtr<IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>>> CreateEnumeratorAsync(
                __in IStoreTransaction<TKey, TValue> & storeTransaction,
                __in TKey firstKey,
                __in TKey lastKey,
                __in ReadMode readMode=ReadMode::ReadValue)
            {
                ApiEntry();

                return CreateEnumeratorAsync(storeTransaction, firstKey, true, lastKey, true, readMode);
            }

            ktl::Awaitable<KSharedPtr<IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>>> CreateEnumeratorAsync(
                __in IStoreTransaction<TKey, TValue> & storeTransaction,
                __in TKey firstKey,
                __in bool useFirstKey,
                __in TKey lastKey,
                __in bool useLastKey,
                __in ReadMode readMode=ReadMode::ReadValue)
            {
                ApiEntry();

                if (!EnableEnumerationWithRepeatableRead && storeTransaction.ReadIsolationLevel != StoreTransactionReadIsolationLevel::Snapshot)
                {
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                if (useFirstKey && useLastKey)
                {
                    if (keyComparerSPtr_->Compare(firstKey, lastKey) > 0)
                    {
                        throw ktl::Exception(STATUS_INVALID_PARAMETER_4);
                    }
                }

                try
                {
                    return CreateKeyValueEnumeratorAsync(storeTransaction, firstKey, useFirstKey, lastKey, useLastKey, readMode);
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"CreateEnumeratorAsync", e);
                    throw;
                }
            }


#pragma region ISweepProvider

            void SetKeySize(__in LONG64 keySize)
            {
                keySize_ = keySize;
            }

            LONG64 GetEstimatedKeySize()
            {
                if (keySize_ > 0)
                {
                    return keySize_;
                }

                auto cachedEstimator = keySizeEstimatorSPtr_.Get();
                CODING_ERROR_ASSERT(cachedEstimator != nullptr);
                return cachedEstimator->GetEstimatedKeySize();
            }

            LONG64 GetMemorySize() override
            {
                LONG64 differentialSize = 0;
                LONG64 consolidatedSize = 0;
                LONG64 snapshotSize = 0;

                LONG64 estimatedKeySize = GetEstimatedKeySize();

                auto cachedDifferentialStoreComponentSPtr = differentialStoreComponentSPtr_.Get();
                STORE_ASSERT(cachedDifferentialStoreComponentSPtr != nullptr, "differential store component cannot be null");

                // We are conservatively estimating that each key in the differential component has both previous and current version set, so 2 VersionedItems for each key

                differentialSize = cachedDifferentialStoreComponentSPtr->Size + cachedDifferentialStoreComponentSPtr->Count() * (estimatedKeySize + 2 * Constants::ValueMetadataSizeBytes);
                STORE_ASSERT(differentialSize >= 0, "Differential size {1} should not be negative", differentialSize);

                consolidatedSize = consolidationManagerSPtr_->GetMemorySize(estimatedKeySize);
                STORE_ASSERT(consolidatedSize >= 0, "Consolidated size {1} should not be negative", consolidatedSize);

                snapshotSize = snapshotContainerSPtr_->GetMemorySize(estimatedKeySize);
                STORE_ASSERT(snapshotSize >= 0, "Snapshot size {1} should not be negative", snapshotSize);

                return differentialSize + consolidatedSize + snapshotSize;
            }

#pragma endregion

            void TrimFiles()
            {
                StoreEventSource::Events->StoreMetadataTableTrimFilesStart(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    Data::Utilities::ToStringLiteral(*workFolder_));

                KHashSet<KString::SPtr>::SPtr referencedFiles = GetCurrentMetadataFilesSet();
                KArray<KString::SPtr> filesToBeDeleted(this->GetThisAllocator());

                KSharedArray<KString::SPtr>::SPtr checkpointFilesSPtr = GetCheckpointFiles();

                for (ULONG32 i = 0; i < checkpointFilesSPtr->Count(); i++)
                {
                    KString::SPtr filenameSPtr = (*checkpointFilesSPtr)[i];
                    if (!referencedFiles->TryRemove(filenameSPtr))
                    {
                        // The file does not exist in the set, so it must not be referenced - so delete it
                        StoreEventSource::Events->StoreMetadataTableTrimFilesToDelete(
                            traceComponent_->PartitionId, traceComponent_->TraceTag,
                            Data::Utilities::ToStringLiteral(*filenameSPtr));

                        filesToBeDeleted.Append(filenameSPtr);
                    }
                }

                // Safety verification - if we didn't find all the currently referenced files, something went wrong
                // and we should not deleted any files (case mismatch? drive\network path mismatch?)
                STORE_ASSERT(referencedFiles->Count() == 0, "failed to find all referenced files when trimming unreferenced files. {1} remaining. NOT deleting any files", referencedFiles->Count());

                // We can now assume it's safe to delete the unreferenced files
                for (ULONG32 i = 0; i < filesToBeDeleted.Count(); i++)
                {
                    KString::SPtr tableFileSPtr = filesToBeDeleted[i];

                    StoreEventSource::Events->StoreMetadataTableTrimFilesDeleting(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        Data::Utilities::ToStringLiteral(*tableFileSPtr));

                    try
                    {
                        Common::File::Delete(tableFileSPtr->operator LPCWSTR(), true);
                    }
                    catch (ktl::Exception const & e)
                    {
                        TraceException(L"TrimFiles", e);
                        throw;
                    }
                }

                StoreEventSource::Events->StoreMetadataTableTrimFilesComplete(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    Data::Utilities::ToStringLiteral(*workFolder_));
            }

            KSharedArray<KString::SPtr>::SPtr GetCheckpointFiles()
            {
                KString::SPtr keyCheckpointFilePatternSPtr = nullptr;
                NTSTATUS status = KString::Create(keyCheckpointFilePatternSPtr, this->GetThisAllocator(), L"*");
                Diagnostics::Validate(status);
                BOOLEAN result = keyCheckpointFilePatternSPtr->Concat(KeyCheckpointFile::GetFileExtension());
                STORE_ASSERT(result, "Unable to build key checkpoint file pattern");

                KString::SPtr valueCheckpointFilePatternSPtr = nullptr;
                status = KString::Create(valueCheckpointFilePatternSPtr, this->GetThisAllocator(), L"*");
                Diagnostics::Validate(status);
                result = valueCheckpointFilePatternSPtr->Concat(ValueCheckpointFile::GetFileExtension());
                STORE_ASSERT(result, "Unable to build key checkpoint file pattern");

                KSharedArray<KString::SPtr>::SPtr checkpointFiles = _new(this->GetThisAllocationTag(), this->GetThisAllocator()) KSharedArray<KString::SPtr>();

                vector<wstring> keyFiles = Common::Directory::GetFiles(workFolder_->operator LPCWSTR(), keyCheckpointFilePatternSPtr->operator LPCWSTR(), true, true);
                vector<wstring> valueFiles = Common::Directory::GetFiles(workFolder_->operator LPCWSTR(), valueCheckpointFilePatternSPtr->operator LPCWSTR(), true, true);

                for (wstring keyFile : keyFiles)
                {
                    KString::SPtr keyFileSPtr = nullptr;
                    status = KString::Create(keyFileSPtr, this->GetThisAllocator(), KStringView(keyFile.c_str()));
                    status = checkpointFiles->Append(keyFileSPtr);
                    STORE_ASSERT(NT_SUCCESS(status), "Unable to add key checkpoint file to checkpoint files");
                }

                for (wstring valueFile : valueFiles)
                {
                    KString::SPtr valueFileSPtr = nullptr;
                    status = KString::Create(valueFileSPtr, this->GetThisAllocator(), KStringView(valueFile.c_str()));
                    status = checkpointFiles->Append(valueFileSPtr);
                    STORE_ASSERT(NT_SUCCESS(status), "Unable to add value checkpoint file to checkpoint files");
                }

                return checkpointFiles;
            }

            KHashSet<KString::SPtr>::SPtr GetCurrentMetadataFilesSet()
            {
                // Construct a hash set from the currently referenced files.
                KHashSet<KString::SPtr>::SPtr referencedFilesSPtr = nullptr;
                KHashSet<KString::SPtr>::Create(32, K_DefaultHashFunction, StringEquals, this->GetThisAllocator(), referencedFilesSPtr);

                auto cachedCurrentMetadataTableSPtr = currentMetadataTableSPtr_.Get();
                if (cachedCurrentMetadataTableSPtr != nullptr)
                {
                    auto tableEnumeratorSPtr = cachedCurrentMetadataTableSPtr->Table->GetEnumerator();
                    STORE_ASSERT(tableEnumeratorSPtr != nullptr, "enumerator should not be null");
                    while (tableEnumeratorSPtr->MoveNext())
                    {
                        auto item = tableEnumeratorSPtr->Current();
                        auto fileMetadataSPtr = item.Value;
                        auto keyCheckpointFileName = fileMetadataSPtr->CheckpointFileSPtr->KeyCheckpointFileNameSPtr;
                        auto valueCheckpointFileName = fileMetadataSPtr->CheckpointFileSPtr->ValueCheckpointFileNameSPtr;

                        bool added = referencedFilesSPtr->TryAdd(keyCheckpointFileName);
                        STORE_ASSERT(added, "key checkpoint file should have been added to hash set");
                        added = referencedFilesSPtr->TryAdd(valueCheckpointFileName);
                        STORE_ASSERT(added, "value checkpoint file should have been added to hash set");
                    }
                }

                return referencedFilesSPtr;
            }

            void StartBackgroundSweep()
            {
                STORE_ASSERT(sweepManagerSPtr_ != nullptr, "sweep manager has not been initialized");
                if (hasPersistedState_ && enableSweep_)
                {
                    ktl::Awaitable<void> sweepAwaitable = sweepManagerSPtr_->SweepAsync();
                    ktl::Task sweepTask = ToTask(sweepAwaitable);
                    STORE_ASSERT(sweepTask.IsTaskStarted(), "Expected sweep task to start");
                }
            }

#pragma region IStateProvider2 implementation

            KUriView const GetName() const override
            {
                return Name;
            }

            KArray<TxnReplicator::StateProviderInfo> GetChildren(KUriView const & rootName) override
            {
                UNREFERENCED_PARAMETER(rootName);
                return KArray<TxnReplicator::StateProviderInfo>(this->GetThisAllocator(), 0);
            }

            VOID Initialize(
                __in KWeakRef<TxnReplicator::ITransactionalReplicator> & transactionalReplicatorWRef,
                __in KStringView const & workFolder,
                __in_opt KSharedArray<TxnReplicator::IStateProvider2::SPtr> const * const children) override
            {
                transactionalReplicatorSPtr_ = &transactionalReplicatorWRef;
                STORE_ASSERT(transactionalReplicatorSPtr_->IsAlive(), "transactional replicator expired");

                hasPersistedState_ = GetReplicator()->HasPeristedState;

                STORE_ASSERT(children == nullptr, "children == nullptr");

                KString::SPtr tmp = nullptr;
                NTSTATUS status = KString::Create(tmp, this->GetThisAllocator(), workFolder);
                if (NT_SUCCESS(status) == false)
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                workFolder_ = KString::CSPtr(tmp.RawPtr());
                tmpMetadataFilePath_ = GetCheckpointFilePath(TempDiskMetadataFileName);
                currentMetadataFilePath_ = GetCheckpointFilePath(CurrentDiskMetadataFileName);
                bkpMetadataFilePath_ = GetCheckpointFilePath(BkpDiskMetadataFileName);

                status = LockManager::Create(this->GetThisAllocator(), lockManager_);
                if (!NT_SUCCESS(status))
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                status = SnapshotContainer<TKey, TValue>::Create(*valueConverterSPtr_, *traceComponent_, this->GetThisAllocator(), snapshotContainerSPtr_);
                if (!NT_SUCCESS(status))
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                KSharedPtr<DifferentialStoreComponent<TKey, TValue>> differentialStoreComponentSPtr = nullptr;

                status = DifferentialStoreComponent<TKey, TValue>::Create(
                    func_,
                    *GetReplicator(),
                    *snapshotContainerSPtr_,
                    storeId_,
                    *keyComparerSPtr_,
                    *traceComponent_,
                    this->GetThisAllocator(),
                    differentialStoreComponentSPtr);

                differentialStoreComponentSPtr_.Put(Ktl::Move(differentialStoreComponentSPtr));
                if (!NT_SUCCESS(status))
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                status = ConsolidationManager<TKey, TValue>::Create(
                    *this,
                    *traceComponent_,
                    this->GetThisAllocator(),
                    consolidationManagerSPtr_);
                if (!NT_SUCCESS(status))
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                status = SweepManager<TKey, TValue>::Create(*this, *consolidationManagerSPtr_, *traceComponent_, this->GetThisAllocator(), sweepManagerSPtr_);
                if (!NT_SUCCESS(status))
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                KeySizeEstimator::SPtr estimatorSPtr = nullptr;
                status = KeySizeEstimator::Create(this->GetThisAllocator(), estimatorSPtr);
                keySizeEstimatorSPtr_.Put(Ktl::Move(estimatorSPtr));
                if (!NT_SUCCESS(status))
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                mergeHelperSPtr_ = nullptr;
                status = MergeHelper::Create(this->GetThisAllocator(), mergeHelperSPtr_);
                if (!NT_SUCCESS(status))
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                LongComparer::SPtr longComparerSPtr = nullptr;
                LongComparer::Create(this->GetThisAllocator(), longComparerSPtr);
                IComparer<LONG64>::SPtr txnComparer = static_cast<IComparer<LONG64> *>(longComparerSPtr.RawPtr());

                status = ConcurrentDictionary2<LONG64, KSharedPtr<StoreTransaction<TKey, TValue>>>::Create(
                    100,
                    K_DefaultHashFunction,
                    *txnComparer,
                    this->GetThisAllocator(),
                    inflightReadWriteStoreTransactionsSPtr_);
                if (!NT_SUCCESS(status))
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                KSharedPtr<MetadataTable> currentMetadataTableSPtr = nullptr;
                status = MetadataTable::Create(this->GetThisAllocator(), currentMetadataTableSPtr);
                if (!NT_SUCCESS(status))
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                currentMetadataTableSPtr_.Put(Ktl::Move(currentMetadataTableSPtr));

                KSharedPtr<FileMetaDataComparer> fileMetaDataComparer = nullptr;
                status = FileMetaDataComparer::Create(this->GetThisAllocator(), fileMetaDataComparer);
                if (!NT_SUCCESS(status))
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                KSharedPtr<IComparer<FileMetadata::SPtr>> comparer = &(*fileMetaDataComparer);
                status = ConcurrentDictionary<FileMetadata::SPtr, bool>::Create(
                    ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::DefaultConcurrencyLevel,
                    ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::DefaultInitialCapacity,
                    FileMetadata::HashFunction,
                    comparer,
                    this->GetThisAllocator(),
                    filesToBeDeletedSPtr_);

                if (!NT_SUCCESS(status))
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                status = ReaderWriterAsyncLock::Create(this->GetThisAllocator(), this->GetThisAllocationTag(), metadataTableLockSPtr_);
                if (!NT_SUCCESS(status))
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                consolidationTcsSPtr_.Put(nullptr);
                status = ktl::CancellationTokenSource::Create(this->GetThisAllocator(), this->GetThisAllocationTag(), consolidationTaskCancellationSourceSPtr_);
                if (!NT_SUCCESS(status))
                {
                    this->SetConstructorStatus(status);
                    return;
                }

                this->StartBackgroundSweep();
            }

            ktl::Awaitable<void> OpenAsync(
                __in ktl::CancellationToken const & cancellationToken) override
            {
                StoreEventSource::Events->StoreOpenAsyncApi(traceComponent_->PartitionId, traceComponent_->TraceTag, L"starting");

                ktl::kservice::OpenAwaiter::SPtr openAwaiter;
                NTSTATUS status = ktl::kservice::OpenAwaiter::Create(GetThisAllocator(), GetThisAllocationTag(), *this, openAwaiter);
                Diagnostics::Validate(status);
                status = co_await *openAwaiter;
                STORE_ASSERT(NT_SUCCESS(status), "Unsuccessful open. Status: {1}", status);

                StoreEventSource::Events->StoreOpenAsyncApi(traceComponent_->PartitionId, traceComponent_->TraceTag, L"completed");
            }

            ktl::Awaitable<void> ChangeRoleAsync(
                __in FABRIC_REPLICA_ROLE newRole,
                __in ktl::CancellationToken const &token
            ) override
            {
                ApiEntry();
                StoreEventSource::Events->StoreChangeRoleAsyncApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    static_cast<int>(newRole));
                co_return;
            }

            ktl::Awaitable<void> CloseAsync(__in ktl::CancellationToken const & token) override
            {
                StoreEventSource::Events->StoreCloseAsyncApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    L"starting");

                isClosing_ = true;

                KSharedPtr<StoreTransaction<TKey, TValue>> storeTransactionSPtr = nullptr;
                NTSTATUS status = StoreTransaction<TKey, TValue>::Create(
                    CreateTransactionId(),
                    storeId_,
                    *keyComparerSPtr_,
                    *traceComponent_,
                    this->GetThisAllocator(),
                    storeTransactionSPtr);
                Diagnostics::Validate(status);
                try
                {
                    // Drain all existing inflight transactions. Since isClosing_ = true, all new transactions will see SF_STATUS_OBJECT_CLOSED
                    co_await storeTransactionSPtr->AcquirePrimeLockAsync(*lockManager_, LockMode::Exclusive, Common::TimeSpan::FromMilliseconds(10000), false);
                }
                catch (ktl::Exception const & e)
                {
                    // If we fail to acquire the prime lock due to timeout, we still want to proceed with Close
                    if (e.GetStatus() != SF_STATUS_TIMEOUT)
                    {
                        throw;
                    }
                    else
                    {
                        StoreEventSource::Events->StoreOnCleanupAsyncApiPrimeLockNotAcquired(traceComponent_->PartitionId, traceComponent_->TraceTag);
                    }
                }

                // Unlock immediately since lock needs to be released before closing lock manager
                storeTransactionSPtr->Unlock();

                // OnServiceClose will not be called until all pending activities (i.e. operations) have finished
                // If there's an operation waiting infinitely for a lock, it will prevent OnServiceClose from proceeding
                // We'll close lock manager now so that all waiters are expired
                co_await lockManager_->CloseAsync();

                // Now let ServiceClose be scheduled
                ktl::kservice::CloseAwaiter::SPtr closeAwaiter;
                status = ktl::kservice::CloseAwaiter::Create(GetThisAllocator(), GetThisAllocationTag(), *this, closeAwaiter);
                Diagnostics::Validate(status);
                status = co_await *closeAwaiter;
                STORE_ASSERT(NT_SUCCESS(status), "Unsuccessful close. Status: {1}", status);

                StoreEventSource::Events->StoreCloseAsyncApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    L"completed");
            }

            void Abort() noexcept override
            {
                StoreEventSource::Events->StoreAbortAsyncApi(traceComponent_->PartitionId, traceComponent_->TraceTag, L"starting");

                ktl::Awaitable<void> closeAwaitable = CloseAsync(ktl::CancellationToken::None);
                ktl::Task task = ToTask(closeAwaitable);
                STORE_ASSERT(task.IsTaskStarted(), "Expected close task to start");
            }

            void PrepareCheckpoint(
                __in LONG64 checkpointLSN) override
            {
                ApiEntry();

                // checkpointLSN=0 is possible on the first checkpoint of a replica after being copied
                STORE_ASSERT(checkpointLSN >= 0, "checkpointLSN: {1} < 0", checkpointLSN);

                StoreEventSource::Events->StorePrepareCheckpointStartApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    checkpointLSN);

                // We cannot assert that lastPrepareCheckpointLSN is InvalidLSN because of double prepare case. 
                lastPrepareCheckpointLSN_ = checkpointLSN;

                auto cachedDeltaDifferentialComponentSPtr = deltaDifferentialStoreComponentSPtr_.Get();
                auto cachedDifferentialComponentSPtr = differentialStoreComponentSPtr_.Get();
                STORE_ASSERT(cachedDifferentialComponentSPtr != nullptr, "cachedDifferentialComponentSPtr != nullptr");

                try
                {
                    // Take a snap of the differential state and then create a new empty differential state.
                    // No lock is needed here because applies cannot happen in parallel when checkpoint is in progress.
                    // Read will not have any correctness issues without taking locks. 
                    // Consolidated state should be set before reinitializing differential state

                    if (cachedDeltaDifferentialComponentSPtr != nullptr)
                    {
                        AppendDeltaDifferentialState();
                    }
                    else
                    {
                        // Since this assignment is not thread safe, protecting it.
                        deltaDifferentialStoreComponentSPtr_.Put(Ktl::Move(cachedDifferentialComponentSPtr));

                        cachedDeltaDifferentialComponentSPtr = deltaDifferentialStoreComponentSPtr_.Get();
                        STORE_ASSERT(cachedDeltaDifferentialComponentSPtr != nullptr, "cachedDeltaDifferentialComponentSPtr != nullptr");

                        consolidationManagerSPtr_->AppendDeltaDifferentialState(*cachedDeltaDifferentialComponentSPtr);
                    }

                    // Differential state should be re-initialized only after consolidated state is made to point to consolidating state.
                    auto cachedEstimator = keySizeEstimatorSPtr_.Get();
                    auto lastEstimate = cachedEstimator->GetEstimatedKeySize();

                    KSharedPtr<DifferentialStoreComponent<TKey, TValue>> differentialStoreComponentSPtr = nullptr;
                    NTSTATUS status = DifferentialStoreComponent<TKey, TValue>::Create(
                        func_,
                        *GetReplicator(),
                        *snapshotContainerSPtr_,
                        storeId_,
                        *keyComparerSPtr_,
                        *traceComponent_,
                        this->GetThisAllocator(),
                        differentialStoreComponentSPtr);
                    Diagnostics::Validate(status);

                    STORE_ASSERT(differentialStoreComponentSPtr != nullptr, "differentialStoreComponentSPtr != nullptr");
                    differentialStoreComponentSPtr_.Put(Ktl::Move(differentialStoreComponentSPtr));

                    KeySizeEstimator::SPtr estimatorSPtr = nullptr;
                    KeySizeEstimator::Create(this->GetThisAllocator(), estimatorSPtr);
                    estimatorSPtr->SetInitialEstimate(lastEstimate);
                    keySizeEstimatorSPtr_.Put(Ktl::Move(estimatorSPtr));
                }
                catch (const ktl::Exception & e)
                {
                    TraceException(L"PrepareCheckpoint", e);
                    throw;
                }

                StoreEventSource::Events->StorePrepareCheckpointCompletedApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    Count);
            }

            ktl::Awaitable<void> PerformCheckpointAsync(
                __in ktl::CancellationToken const & cancellationToken) override
            {
                ApiEntry();

                Common::Stopwatch stopwatch;
                stopwatch.Start();

                StoreEventSource::Events->StorePerformCheckpointAsyncStartApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    lastPrepareCheckpointLSN_,
                    GetCheckpointLSN());

                co_await CheckpointAsync(cancellationToken);
                lastPerformCheckpointLSN_ = lastPrepareCheckpointLSN_;
                lastPrepareCheckpointLSN_ = -1;

                ULONG64 diskSize = co_await GetDiskSizeAsync(nextMetadataTableSPtr_);
                LONG64 memorySize = this->Size;

                perfCounters_->ItemCount.Value = count_;
                perfCounters_->DiskSize.Value = diskSize;
                perfCounters_->MemorySize.Value = memorySize;

                StoreEventSource::Events->StorePerformCheckpointAsyncCompletedApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    fileId_,
                    stopwatch.ElapsedMilliseconds,
                    count_,
                    diskSize,
                    memorySize);
            }

            ktl::Awaitable<void> CompleteCheckpointAsync(
                __in ktl::CancellationToken const & cancellationToken) override
            {
                ApiEntry();

                try
                {
                    Common::Stopwatch stopwatch;
                    stopwatch.Start();

                    StoreEventSource::Events->StoreCompleteCheckpointAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"starting");

                    auto cachedNextMetadataTableSPtr = nextMetadataTableSPtr_.Get();
                    if (Common::File::Exists(static_cast<LPCWSTR>(*tmpMetadataFilePath_)))
                    {
                        co_await MetadataManager::SafeFileReplaceAsync(
                            *currentMetadataFilePath_,
                            *tmpMetadataFilePath_,
                            *bkpMetadataFilePath_,
                            true,
                            *traceComponent_,
                            this->GetThisAllocator());
                    }

                    auto snapTime = stopwatch.ElapsedMilliseconds;
                    auto timeToFileReplace = snapTime;

                    if (cachedNextMetadataTableSPtr != nullptr)
                    {
                        {
                            co_await metadataTableLockSPtr_->AcquireWriteLockAsync(Constants::MetadataTableLockTimeoutMilliseconds);
                            KFinally([&] { metadataTableLockSPtr_->ReleaseWriteLock(); });

                            auto cachedCurrentMetadataTableSPtr = currentMetadataTableSPtr_.Get();
                            STORE_ASSERT(cachedCurrentMetadataTableSPtr != nullptr, "cachedCurrentMetadataTableSPtr != nullptr");

                            MetadataTable::SPtr tempMetadataTable = cachedCurrentMetadataTableSPtr;
                            currentMetadataTableSPtr_.Put(Ktl::Move(cachedNextMetadataTableSPtr));

                            nextMetadataTableSPtr_.Put(nullptr);
                            co_await tempMetadataTable->ReleaseReferenceAsync();
                        }
                    }

                    auto timeToSwap = stopwatch.ElapsedMilliseconds - snapTime;
                    snapTime = stopwatch.ElapsedMilliseconds;

                    StoreEventSource::Events->StoreCompleteCheckpointAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"next metadata table has been reset to the current metadatat table");

                    // Release files that need to be deleted
                    KArray<FileMetadata::SPtr> filesToDelete(this->GetThisAllocator(), filesToBeDeletedSPtr_->Count);

                    auto enumerator = filesToBeDeletedSPtr_->GetEnumerator();
                    while (enumerator->MoveNext())
                    {
                        FileMetadata::SPtr fileMetadata = enumerator->Current().Key;
                        STORE_ASSERT(fileMetadata->ReferenceCount > 0, "Invalid reference count {1} for file id {2}", fileMetadata->ReferenceCount, fileMetadata->FileId);

                        bool immediate = enumerator->Current().Value;
                        if (immediate)
                        {
                            filesToDelete.Append(fileMetadata);
                        }
                        else
                        {
                            STORE_ASSERT(filesToBeDeletedSPtr_->TryUpdate(fileMetadata, true, immediate), "Unable to update list of files to be deleted for file {1}", fileMetadata->FileId);
                        }
                    }

                    auto timeToComputeToBeDeleted = stopwatch.ElapsedMilliseconds - snapTime;
                    snapTime = stopwatch.ElapsedMilliseconds;

                    // Delete files in filesToDelete
                    for (ULONG i = 0; i < filesToDelete.Count(); i++)
                    {
                        auto fileMetadata = filesToDelete[i];
                        co_await fileMetadata->ReleaseReferenceAsync();
                        STORE_ASSERT(filesToBeDeletedSPtr_->Remove(fileMetadata), "Failed to remove file id {1} from filesToBeDeleted", fileMetadata->FileId);
                    }

                    stopwatch.Stop();
                    auto timeToDelete = stopwatch.ElapsedMilliseconds - snapTime;
                    snapTime = stopwatch.ElapsedMilliseconds;

                    StoreEventSource::Events->StoreCompleteCheckpointAsyncCompletedApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        snapTime,
                        timeToFileReplace,
                        timeToSwap,
                        timeToComputeToBeDeleted,
                        timeToDelete,
                        Size);

                    co_return;
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"CompleteCheckpointAsync", e);
                    throw;
                }
            }

            ktl::Awaitable<void> RecoverCheckpointAsync(
                __in ktl::CancellationToken const & cancellationToken) override
            {
                ApiEntry();

                StoreEventSource::Events->StoreRecoverCheckpointAsyncApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    L"starting",
                    -1,
                    -1,
                    -1);

                lastPrepareCheckpointLSN_ = -1;
                lastPerformCheckpointLSN_ = -1;

                KSharedPtr<MetadataTable> cachedCurrentMetadataTableSPtr = currentMetadataTableSPtr_.Get();
                STORE_ASSERT(cachedCurrentMetadataTableSPtr != nullptr, "cachedCurrentMetadataTableSPtr != nullptr");

                if (hasPersistedState_ == false)
                {
                    // TODO: Trace that volatile store has recovered
                    cachedCurrentMetadataTableSPtr->CheckpointLSN = Constants::ZeroLsn;
                }
                else if (!Common::File::Exists(currentMetadataFilePath_->operator LPCWSTR()))
                {
                    StoreEventSource::Events->StoreRecoverCheckpointAsyncFirstCheckpoint(traceComponent_->PartitionId, traceComponent_->TraceTag);

                    cachedCurrentMetadataTableSPtr->CheckpointLSN = Constants::ZeroLsn;

                    // Checkpoint metadata table
                    co_await MetadataManager::WriteAsync(*cachedCurrentMetadataTableSPtr, *tmpMetadataFilePath_, this->GetThisAllocator());

                    // Move the first checkpoint.
                    Common::ErrorCode errorCode = Common::File::Move(tmpMetadataFilePath_->operator LPCWSTR(), currentMetadataFilePath_->operator LPCWSTR(), false);
                    if (errorCode.IsSuccess() == false)
                    {
                        throw ktl::Exception(errorCode.ToHResult());
                    }
                }
                else
                {
                    co_await MetadataManager::OpenAsync(*cachedCurrentMetadataTableSPtr, *currentMetadataFilePath_, this->GetThisAllocator(), *traceComponent_);

                    try
                    {
                        co_await RecoverConsolidatedStateAsync(cancellationToken);
                    }
                    catch (ktl::Exception & e)
                    {
                        TraceException(L"TStore.RecoverCheckpointAsync", e);
                        throw;
                    }
                }

                TrimFiles();

                co_await FireRebuildNotificationCallerHoldsLockAsync();

                StoreEventSource::Events->StoreRecoverCheckpointAsyncApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    L"completed",
                    Count,
                    Size,
                    cachedCurrentMetadataTableSPtr->CheckpointLSN);
            }

            ktl::Awaitable<void> PrepareForRemoveAsync(
                __in TxnReplicator::TransactionBase const & replicatorTransaction,
                __in ktl::CancellationToken const & cancellationToken
            ) override
            {
                ApiEntry();

                KSharedPtr<IStoreTransaction<TKey, TValue>> istoreTransactionSPtr = nullptr;
                TxnReplicator::TransactionBase & replicatorTransactionCast = const_cast<TxnReplicator::TransactionBase&>(replicatorTransaction);

                CreateOrFindTransaction(replicatorTransactionCast, istoreTransactionSPtr);
                KSharedPtr<StoreTransaction<TKey, TValue>> storeTransactionSPtr = static_cast<StoreTransaction<TKey, TValue> *>(istoreTransactionSPtr.RawPtr());
                ThrowIfFaulted(*storeTransactionSPtr);

                // It should be called on the primary only.
                ThrowIfNotWritable(storeTransactionSPtr->Id);

                try
                {
                    // Acquire exclusive lock on store component.
                    ULONG64 remainingTime = co_await storeTransactionSPtr->AcquirePrimeLockAsync(*lockManager_, LockMode::Exclusive, Common::TimeSpan::FromMilliseconds(1000), true);

                    ThrowIfFaulted(*storeTransactionSPtr);

                    // If the operation was cancelled during the lock wait, then terminate.
                    cancellationToken.ThrowIfCancellationRequested();

                    if (remainingTime == 0)
                    {
                        throw ktl::Exception(SF_STATUS_TIMEOUT);
                    }
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"TStore.PrepareForRemoveAsync", e);
                    throw;
                }

                co_return;
            }

            ktl::Awaitable<void> RemoveStateAsync(
                __in ktl::CancellationToken const & cancellationToken
            ) override
            {
                ApiEntry();

                try
                {
                    StoreEventSource::Events->StoreRemoveStateAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"starting");

                    co_await CancelBackgroundConsolidationTaskAsync();
                    co_await CancelSweepTaskAsync();

                    isClosing_ = true;

                    if (Common::Directory::Exists(workFolder_->operator LPCWSTR()))
                    {
                        auto enumerator = filesToBeDeletedSPtr_->GetEnumerator();
                        while (enumerator->MoveNext())
                        {
                            FileMetadata::SPtr fileMetadata = enumerator->Current().Key;
                            // Instead of releasing ref, just dispose.
                            co_await fileMetadata->CloseAsync();
                        }

                        filesToBeDeletedSPtr_->Clear();

                        IDictionary<ULONG32, FileMetadata::SPtr>::SPtr table;
                        KSharedPtr<IEnumerator<KeyValuePair<ULONG32, FileMetadata::SPtr>>> tableEnumerator;

                        auto cachedCurrentMetadataTableSPtr = currentMetadataTableSPtr_.Get();
                        if (cachedCurrentMetadataTableSPtr != nullptr)
                        {
                            table = cachedCurrentMetadataTableSPtr->Table;
                            tableEnumerator = table->GetEnumerator();
                            while (tableEnumerator->MoveNext())
                            {
                                KeyValuePair<ULONG32, FileMetadata::SPtr> entry = tableEnumerator->Current();
                                FileMetadata::SPtr fileMetadata = entry.Value;
                                // Let the directory delete, remove these files.
                                fileMetadata->CanBeDeleted = false;
                            }

                            co_await cachedCurrentMetadataTableSPtr->CloseAsync();
                        }

                        auto cachedNextMetadataTableSPtr = nextMetadataTableSPtr_.Get();
                        if (cachedNextMetadataTableSPtr != nullptr)
                        {
                            table = cachedNextMetadataTableSPtr->Table;
                            tableEnumerator = table->GetEnumerator();
                            while (tableEnumerator->MoveNext())
                            {
                                KeyValuePair<ULONG32, FileMetadata::SPtr> entry = tableEnumerator->Current();
                                FileMetadata::SPtr fileMetadata = entry.Value;
                                fileMetadata->CanBeDeleted = false;
                            }

                            co_await cachedNextMetadataTableSPtr->CloseAsync();
                        }

                        auto cachedMergeMetadataTableSPtr = mergeMetadataTableSPtr_.Get();
                        if (cachedMergeMetadataTableSPtr != nullptr)
                        {
                            table = cachedMergeMetadataTableSPtr->Table;
                            tableEnumerator = table->GetEnumerator();
                            while (tableEnumerator->MoveNext())
                            {
                                KeyValuePair<ULONG32, FileMetadata::SPtr> entry = tableEnumerator->Current();
                                FileMetadata::SPtr fileMetadata = entry.Value;
                                fileMetadata->CanBeDeleted = false;
                            }

                            co_await cachedMergeMetadataTableSPtr->CloseAsync();
                        }

                        if (snapshotContainerSPtr_ != nullptr)
                        {
                            co_await snapshotContainerSPtr_->CloseAsync();
                        }

                        // Delete metadata files explicitely
                        Common::ErrorCode errorCode;
                        if (Common::File::Exists(currentMetadataFilePath_->operator LPCWSTR()))
                        {
                            errorCode = Common::File::Delete2(currentMetadataFilePath_->operator LPCWSTR());
                            if (errorCode.IsSuccess() == false)
                            {
                                throw ktl::Exception(errorCode.ToHResult());
                            }
                        }

                        if (Common::File::Exists(tmpMetadataFilePath_->operator LPCWSTR()))
                        {
                            errorCode = Common::File::Delete2(tmpMetadataFilePath_->operator LPCWSTR());
                            if (errorCode.IsSuccess() == false)
                            {
                                throw ktl::Exception(errorCode.ToHResult());
                            }
                        }

                        errorCode = Common::Directory::Delete(workFolder_->operator LPCWSTR(), true);
                        if (!errorCode.IsSuccess())
                        {
                            throw ktl::Exception(errorCode.ToHResult());
                        }

                        currentMetadataTableSPtr_.Put(nullptr);
                        nextMetadataTableSPtr_.Put(nullptr);
                        mergeMetadataTableSPtr_.Put(nullptr);
                    }

                    perfCounters_->DiskSize.Value = 0;
                    perfCounters_->ItemCount.Value = 0;

                    StoreEventSource::Events->StoreRemoveStateAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"completed");

                    co_return;
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"RemoveStateAsync", e);
                    throw;
                }
            }

            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyAsync(
                __in LONG64 logicalSequenceNumber,
                __in TxnReplicator::TransactionBase const & replicatorTransaction,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in_opt Utilities::OperationData const * const metadataPtr,
                __in_opt Utilities::OperationData const * const dataPtr) override
            {
                ApiEntry();

                try
                {
                    STORE_ASSERT(wasCopyAborted_ == false, "Copy was aborted. Cannot Apply");

                    // Undo can be null.
                    STORE_ASSERT(metadataPtr != nullptr, "metadataPtr != nullptr");
                    STORE_ASSERT(metadataPtr->BufferCount > 0, "metadataPtr->BufferCount {1} > 0", metadataPtr->BufferCount);

                    TxnReplicator::OperationContext::CSPtr operationContextCSPtr = nullptr;
                    TxnReplicator::ApplyContext::Enum roleType = static_cast<TxnReplicator::ApplyContext::Enum>(applyContext & TxnReplicator::ApplyContext::ROLE_MASK);
                    TxnReplicator::ApplyContext::Enum operationType =
                        static_cast<TxnReplicator::ApplyContext::Enum>(applyContext & TxnReplicator::ApplyContext::OPERATION_MASK);

                    LONG64 commitSequenceNumber = replicatorTransaction.CommitSequenceNumber;

                    TxnReplicator::TransactionBase::CSPtr replicatorTransactionCSPtr = &replicatorTransaction;
                    STORE_ASSERT(replicatorTransactionCSPtr != nullptr, "replicator transaction should not be null");
                    STORE_ASSERT(replicatorTransactionCSPtr.RawPtr() != nullptr, "replicator transaction should not be null");

                    // Undo should be null
                    STORE_ASSERT(operationType == TxnReplicator::ApplyContext::REDO || dataPtr == nullptr,
                        "operationType == ApplyContext::REDO || dataPtr == nullptr");

                    // Idempotency check
                    auto cachedMetadataTable = currentMetadataTableSPtr_.Get();
                    if (cachedMetadataTable != nullptr && IsDuplicateApply(commitSequenceNumber, *cachedMetadataTable, roleType, operationType))
                    {
                        co_return nullptr;
                    }

                    MetadataOperationData::CSPtr metadataOperationDataCSPtr = nullptr;
                    if (roleType == TxnReplicator::ApplyContext::PRIMARY)
                    {
                        MetadataOperationData const * const metadataOperationDataPtr = static_cast<MetadataOperationData const * const>(metadataPtr);
                        metadataOperationDataCSPtr = metadataOperationDataPtr;
                        STORE_ASSERT(metadataOperationDataCSPtr != nullptr, "metadataOperationDataCSPtr != nullptr");

                    }
                    else
                    {
                        OperationData::CSPtr dataSPtr = metadataPtr;
                        metadataOperationDataCSPtr = MetadataOperationData::Deserialize(Constants::SerializedVersion, this->GetThisAllocator(), dataSPtr);
                    }

                    if (roleType == TxnReplicator::ApplyContext::PRIMARY)
                    {
                        STORE_ASSERT(operationType == TxnReplicator::ApplyContext::REDO, "operationType == TxnReplicator::ApplyContext::REDO");

                        STORE_ASSERT(logicalSequenceNumber > 0, "invalid logicalsequence number {1}", logicalSequenceNumber);
                        STORE_ASSERT(commitSequenceNumber > 0, "invalid logicalsequence number {1}", commitSequenceNumber);

                        auto storeTransactionSPtr = co_await ApplyOnPrimaryAsync(commitSequenceNumber, *replicatorTransactionCSPtr, *metadataOperationDataCSPtr);
                        operationContextCSPtr = storeTransactionSPtr.RawPtr();
                    }
                    else if (roleType == TxnReplicator::ApplyContext::SECONDARY)
                    {
                        // Reload the cached table since it may have been reset during copy
                        cachedMetadataTable = currentMetadataTableSPtr_.Get();

                        // Idempotency check. Now the current metadata table cannot be null.
                        if (IsDuplicateApply(commitSequenceNumber, *cachedMetadataTable, roleType, operationType))
                        {
                            co_return nullptr;
                        }

                        // Replicator currently only applys after commit. Hence UNDO is not expected
                        STORE_ASSERT(operationType == TxnReplicator::ApplyContext::REDO || operationType == TxnReplicator::ApplyContext::FALSE_PROGRESS, "unexpected operation type {1}", static_cast<ULONG32>(operationType));

                        // Last performed checkpoint lsn is guaranteed to be stable
                        STORE_ASSERT(lastPerformCheckpointLSN_ < commitSequenceNumber, "Last performed checkpoint LSN must be stable: lastPerformCheckpointLSN {1} < sequenceNumber {2}", lastPerformCheckpointLSN_, commitSequenceNumber);
                        if (operationType == TxnReplicator::ApplyContext::REDO)
                        {
                            // isIdempotent = could it be a duplicate
                            // If replica is not readable (all operations since the copy have not been replayed without checkpoint LSN is recovered, then it could be duplicate)
                            KSharedPtr<TxnReplicator::ITransactionalReplicator> replicatorSPtr = GetReplicator();
                            bool isIdempotent = !replicatorSPtr->IsReadable || cachedMetadataTable->CheckpointLSN == -1;
                            auto operationRedoUndo = RedoUndoOperationData::Deserialize(*dataPtr, this->GetThisAllocator());
                            auto storeTransactionSPtr = co_await ApplyOnSecondaryAsync(commitSequenceNumber, *replicatorTransactionCSPtr, *metadataOperationDataCSPtr, *operationRedoUndo, isIdempotent);
                            operationContextCSPtr = storeTransactionSPtr.RawPtr();
                        }
                        else
                        {
                            auto storeTransactionSPtr = co_await UndoFalseProgressOnSecondaryAsync(commitSequenceNumber, *replicatorTransactionCSPtr, *metadataOperationDataCSPtr, dataPtr);
                            operationContextCSPtr = storeTransactionSPtr.RawPtr();
                        }
                    }
                    else if (roleType == TxnReplicator::ApplyContext::RECOVERY)
                    {
                        // Checks.
                        STORE_ASSERT(operationType == TxnReplicator::ApplyContext::REDO ||
                            operationType == TxnReplicator::ApplyContext::UNDO,
                            "operationType == TxnReplicator::ApplyContext::REDO || operationType == TxnReplicator::ApplyContext::UNDO");

                        STORE_ASSERT(logicalSequenceNumber > 0, "unexpected sequence number");

                        // Apply operation as part of recovery.
                        STORE_ASSERT(metadataOperationDataCSPtr != nullptr, "metadataOperationDataCSPtr != nullptr");

                        OperationData::CSPtr operationDataCSPtr = dataPtr;
                        auto operationContextSPtr = co_await ApplyOnRecoveryAsync(commitSequenceNumber, *replicatorTransactionCSPtr, *metadataOperationDataCSPtr, *operationDataCSPtr);
                        operationContextCSPtr = operationContextSPtr.RawPtr();
                    }

                    co_return operationContextCSPtr;
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"ApplyAsync", e, replicatorTransaction.TransactionId);
                    throw;
                }
            }

            void Unlock(__in TxnReplicator::OperationContext const & operationContext) override
            {
                ApiEntry();

                const StoreTransaction<TKey, TValue>& storeTransaction = static_cast<StoreTransaction<TKey, TValue> const &>(operationContext);

                try
                {
                    if (storeTransaction.IsCompleted)
                    {
                        return;
                    }

                    // Release all transaction locks.
                    (const_cast<StoreTransaction<TKey, TValue>&>(storeTransaction)).Unlock();
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"Unlock", e, storeTransaction.Id);
                    throw;
                }
            }

            TxnReplicator::OperationDataStream::SPtr GetCurrentState() override
            {
                ApiEntry();
                try
                {
                    TxnReplicator::OperationDataStream::SPtr resultSPtr = nullptr;

                    if (hasPersistedState_ == false)
                    {
                        KSharedPtr<VolatileStoreCopyStream<TKey, TValue>> volatileCopyStream = nullptr;
                        NTSTATUS status = VolatileStoreCopyStream<TKey, TValue>::Create(
                            *consolidationManagerSPtr_,
                            *keyComparerSPtr_,
                            *keyConverterSPtr_,
                            *valueConverterSPtr_,
                            *traceComponent_,
                            this->GetThisAllocator(),
                            volatileCopyStream);
                        Diagnostics::Validate(status);
                        resultSPtr = static_cast<TxnReplicator::OperationDataStream *>(volatileCopyStream.RawPtr());
                    }
                    else
                    {
                        StoreCopyStream::SPtr copyStreamSPtr = nullptr;
                        NTSTATUS status = StoreCopyStream::Create(*this, *traceComponent_, this->GetThisAllocator(), perfCounters_, copyStreamSPtr);
                        Diagnostics::Validate(status);
                        resultSPtr = static_cast<TxnReplicator::OperationDataStream *>(copyStreamSPtr.RawPtr());
                    }

                    StoreEventSource::Events->StoreGetCurrentStateApi(traceComponent_->PartitionId, traceComponent_->TraceTag);

                    return resultSPtr;
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"GetCurrentState", e);
                    throw;
                }
            }

            ktl::Awaitable<void> BeginSettingCurrentStateAsync() override
            {
                ApiEntry();

                StoreEventSource::Events->StoreBeginSettingCurrentStateApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    L"starting");

                try
                {
                    co_await CancelSweepTaskAsync();

                    // Drop all old in-memory state and re-create it.
                    // Differential state.
                    KSharedPtr<DifferentialStoreComponent<TKey, TValue>> cachedDifferentialStoreComponentSPtr;
                    NTSTATUS status = DifferentialStoreComponent<TKey, TValue>::Create(
                        func_,
                        *GetReplicator(),
                        *snapshotContainerSPtr_,
                        storeId_,
                        *keyComparerSPtr_,
                        *traceComponent_,
                        this->GetThisAllocator(),
                        cachedDifferentialStoreComponentSPtr);
                    Diagnostics::Validate(status);
                    differentialStoreComponentSPtr_.Put(Ktl::Move(cachedDifferentialStoreComponentSPtr));

                    // Consolidation Manager.
                    status = ConsolidationManager<TKey, TValue>::Create(
                        *this,
                        *traceComponent_,
                        this->GetThisAllocator(),
                        consolidationManagerSPtr_);
                    Diagnostics::Validate(status);

                    // Snapshot Container.
                    co_await snapshotContainerSPtr_->CloseAsync();

                    status = SnapshotContainer<TKey, TValue>::Create(*valueConverterSPtr_, *traceComponent_, this->GetThisAllocator(), snapshotContainerSPtr_);
                    Diagnostics::Validate(status);

                    // Inflight transactions container.
                    LongComparer::SPtr longComparerSPtr = nullptr;
                    LongComparer::Create(this->GetThisAllocator(), longComparerSPtr);
                    IComparer<LONG64>::SPtr txnComparer = static_cast<IComparer<LONG64> *>(longComparerSPtr.RawPtr());

                    status = ConcurrentDictionary2<LONG64, KSharedPtr<StoreTransaction<TKey, TValue>>>::Create(
                        100,
                        K_DefaultHashFunction,
                        *txnComparer,
                        this->GetThisAllocator(),
                        inflightReadWriteStoreTransactionsSPtr_);

                    Diagnostics::Validate(status);

                    // Lock Manager.
                    co_await CloseAndReopenLockManagerAsync();

                    // Prepare the new persisted state management.
                    // Do not have to lock as there can be no readers yet and no checkpoint yet.
                    // Should file metadata be derefrenced?
                    MetadataTable::SPtr cachedCurrentMetadataTableSPtr = currentMetadataTableSPtr_.Get();
                    if (cachedCurrentMetadataTableSPtr != nullptr)
                    {
                        IDictionary<ULONG32, FileMetadata::SPtr>::SPtr table = cachedCurrentMetadataTableSPtr->Table;
                        IEnumerator<KeyValuePair<ULONG32, FileMetadata::SPtr>>::SPtr tableEnumerator = table->GetEnumerator();

                        while (tableEnumerator->MoveNext())
                        {
                            FileMetadata::SPtr fileMetadataSPtr = tableEnumerator->Current().Value;

                            // Set for delete in the next complete cycle.
                            fileMetadataSPtr->CanBeDeleted = true;
                            fileMetadataSPtr->AddReference();

                            STORE_ASSERT(
                                fileMetadataSPtr->ReferenceCount == 2,
                                "Unexpected filemetadata count {1} for file {2}",
                                fileMetadataSPtr->ReferenceCount,
                                fileMetadataSPtr->FileId);

                            bool added = filesToBeDeletedSPtr_->TryAdd(fileMetadataSPtr, true);
                            STORE_ASSERT(added, "Failed to add file id {1}", fileMetadataSPtr->FileId);
                        }

                        co_await cachedCurrentMetadataTableSPtr->CloseAsync();
                        this->StartBackgroundSweep();
                    }

                    // Reset current metadata table.
                    currentMetadataTableSPtr_.Put(nullptr);

                    // Prepare for set current state.
                    if (hasPersistedState_ == false)
                    {
                        KSharedPtr<VolatileCopyManager<TKey, TValue>> volatileCopyManager = nullptr;
                        status = VolatileCopyManager<TKey, TValue>::Create(
                            *consolidationManagerSPtr_,
                            *keyConverterSPtr_,
                            *valueConverterSPtr_,
                            *traceComponent_,
                            this->GetThisAllocator(),
                            volatileCopyManager);
                        Diagnostics::Validate(status);

                        copyManagerSPtr_ = static_cast<ICopyManager *>(volatileCopyManager.RawPtr());
                    }
                    else
                    {
                        CopyManager::SPtr persistentCopyManager = nullptr;
                        status = CopyManager::Create(*workFolder_, *traceComponent_, this->GetThisAllocator(), perfCounters_, persistentCopyManager);
                        Diagnostics::Validate(status);

                        copyManagerSPtr_ = static_cast<ICopyManager *>(persistentCopyManager.RawPtr());
                    }
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"BeginSettingCurrentState", e);
                    throw;
                }

                StoreEventSource::Events->StoreBeginSettingCurrentStateApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    L"completed");

                co_return;
            }

            ktl::Awaitable<void> SetCurrentStateAsync(
                __in LONG64 stateRecordNumber,
                __in OperationData const & operationData,
                __in ktl::CancellationToken const & cancellationToken
            )  override
            {
                ApiEntry();
                try
                {
                    STORE_ASSERT(copyManagerSPtr_ != nullptr, "unexpectedly no longer building a new master table");
                    OperationData::CSPtr operationDataCSPtr = &operationData;
                    co_await copyManagerSPtr_->AddCopyDataAsync(*operationDataCSPtr);
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"SetCurrentStateAsync", e);
                    throw;
                }

                co_return;
            }

            ktl::Awaitable<void> EndSettingCurrentStateAsync(
                __in ktl::CancellationToken const & cancellationToken
            ) override
            {
                ApiEntry();
                try
                {
                    StoreEventSource::Events->StoreEndSettingCurrentStateAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"starting");

                    wasCopyAborted_ = !copyManagerSPtr_->IsCopyCompleted;

                    if (wasCopyAborted_)
                    {
                        StoreEventSource::Events->StoreEndSettingCurrentStateAsyncAbortedApi(
                            traceComponent_->PartitionId,
                            traceComponent_->TraceTag);
                        co_await copyManagerSPtr_->CloseAsync();
                    }
                    else if (hasPersistedState_)
                    {
                        co_await RecoverCopyStateAsync(cancellationToken);
                    }
                    else
                    {
                        // Create an empty metadata table for the volatile store
                        KSharedPtr<MetadataTable> currentMetadataTableSPtr = nullptr;
                        NTSTATUS status = MetadataTable::Create(this->GetThisAllocator(), currentMetadataTableSPtr);
                        Diagnostics::Validate(status);

                        currentMetadataTableSPtr_.Put(Ktl::Move(currentMetadataTableSPtr));

                        // Update count
                        KSharedPtr<VolatileCopyManager<TKey, TValue>> volatileCopyManager = dynamic_cast<VolatileCopyManager<TKey, TValue> *>(copyManagerSPtr_.RawPtr());
                        STORE_ASSERT(volatileCopyManager != nullptr, "Volatile copy manager is null");
                        count_ = volatileCopyManager->Count;
                    }

                    StoreEventSource::Events->StoreEndSettingCurrentStateAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"completed");
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"EndSettingCurrentStateAsync", e);
                    throw;
                }

                co_return;
            }

#pragma endregion

#pragma region IStateProviderInfo implementation
            KString* GetLangTypeInfo(
                __in KStringView const & lang) const  override
            {
                if (lang_ != nullptr && lang.CompareNoCase(*lang_) == 0)
                {
                    return langTypeInfo_.RawPtr();
                }
                else
                {
                    return nullptr;
                }
            }

            NTSTATUS SetLangTypeInfo(
                __in KStringView const & lang,
                __in KStringView const & typeInfo)  override
            {
                NTSTATUS status = KString::Create(lang_, GetThisAllocator(), lang);
                if (!NT_SUCCESS(status))
                {
                    return status;
                }
                return KString::Create(langTypeInfo_, GetThisAllocator(), typeInfo);
            }

            StateProviderKind GetKind() const override
            {
                return StateProviderKind::Store;
            }
#pragma endregion

            ktl::Awaitable<KSharedPtr<MetadataTable>> GetMetadataTableAsync() override
            {
                KSharedPtr<MetadataTable> snapshotMetadataTable = nullptr;

                {
                    co_await metadataTableLockSPtr_->AcquireWriteLockAsync(Constants::MetadataTableLockTimeoutMilliseconds);
                    KFinally([&] { metadataTableLockSPtr_->ReleaseWriteLock(); });

                    snapshotMetadataTable = currentMetadataTableSPtr_.Get();
                    snapshotMetadataTable->AddReference();
                }

                co_return snapshotMetadataTable;
            }

            OperationData::SPtr GetKeyBytes(__in TKey& key)
            {
                Utilities::BinaryWriter binaryWriter(this->GetThisAllocator());
                keyConverterSPtr_->Write(key, binaryWriter);

                OperationData::SPtr operationDataSPtr = OperationData::Create(this->GetThisAllocator());
                operationDataSPtr->Append(*binaryWriter.GetBuffer(0));

                return operationDataSPtr;
            }

            OperationData::SPtr GetValueBytes(__in TValue& value)
            {
                Utilities::BinaryWriter binaryWriter(this->GetThisAllocator());
                valueConverterSPtr_->Write(value, binaryWriter);

                OperationData::SPtr operationDataSPtr = OperationData::Create(this->GetThisAllocator());
                operationDataSPtr->Append(*binaryWriter.GetBuffer(0));

                return operationDataSPtr;
            }

        protected:
            void OnServiceOpen() override
            {
                OnServiceOpenAsync();
            }

            void OnServiceClose() override
            {
                OnServiceCloseAsync();
            }

        private:
            ktl::Task OnServiceOpenAsync() noexcept
            {
                SetDeferredCloseBehavior();

                StoreEventSource::Events->StoreOnServiceOpenAsync(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    L"starting");

                co_await lockManager_->OpenAsync();

                if (hasPersistedState_)
                {
                    STORE_ASSERT(Common::Directory::Exists(WorkingDirectoryCSPtr->operator LPCWSTR()), "working directory should exist");
                }

                perfCounters_ = StorePerformanceCounters::CreateInstance(
                    traceComponent_->PartitionId,
                    ReplicaId,
                    storeId_);

                CompleteOpen(STATUS_SUCCESS);
                StoreEventSource::Events->StoreOnServiceOpenAsync(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    L"completed");
                co_return;
            }

            ktl::Task OnServiceCloseAsync() noexcept
            {
                StoreEventSource::Events->StoreOnServiceCloseAsync(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    L"starting");

                try
                {
                    co_await this->CleanupAsync();
                }
                catch (ktl::Exception& e)
                {
                    TraceException(L"TStore.OnServiceCloseAsync", e);
                    CompleteClose(e.GetStatus());
                }

                CompleteClose(STATUS_SUCCESS);

                StoreEventSource::Events->StoreOnServiceCloseAsync(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    L"completed");
                co_return;
            }

            ktl::Awaitable<KSharedPtr<StoreComponentReadResult<TValue>>> TryGetValueForReadOnlyTransactionsAsync(
                __in TKey& key,
                __in LONG64 visibilitySequenceNumber,
                __in bool includeSnapshotContainer,
                __in ReadMode readMode,
                __in ktl::CancellationToken const & cancellationToken)
            {
                bool itemPresentInSnapshotComponent = false;
                KSharedPtr<StoreComponentReadResult<TValue>> readResultSPtr = nullptr;
                TValue defaultValue = TValue();
                StoreComponentReadResult<TValue>::Create(nullptr, defaultValue, this->GetThisAllocator(), readResultSPtr);

                KSharedPtr<VersionedItem<TValue>> versionedItem = nullptr;
                auto cachedDifferentialStoreComponentSPtr_ = differentialStoreComponentSPtr_.Get();
                STORE_ASSERT(cachedDifferentialStoreComponentSPtr_ != nullptr, "cachedDifferentialStoreComponentSPtr_ != nullptr");

                // Read from differential state.
                if (visibilitySequenceNumber == Constants::InvalidLsn)
                {
                    versionedItem = cachedDifferentialStoreComponentSPtr_->Read(key);
                }
                else
                {
                    versionedItem = cachedDifferentialStoreComponentSPtr_->Read(key, visibilitySequenceNumber);
                }

                if (versionedItem != nullptr)
                {
                    // Found in differential state, build return value
                    TValue userValue = TValue();
                    if (versionedItem->GetRecordKind() != RecordKind::DeletedVersion)
                    {
                        userValue = versionedItem->GetValue();
                    }

                    StoreComponentReadResult<TValue>::Create(versionedItem, userValue, this->GetThisAllocator(), readResultSPtr);
                }
                else
                {
                    if (includeSnapshotContainer)
                    {
                        STORE_ASSERT(visibilitySequenceNumber != Constants::InvalidLsn, "visibilitySequenceNumber != Constants::InvalidLsn");

                        // Check the snapshot container if there is a match to the visibility lsn.
                        // This check needs to be done even if the entry is present in consolidated state because the snapshot container could be ahead or
                        // behind the consolidated state.
                        auto snapshotComponent = snapshotContainerSPtr_->Read(visibilitySequenceNumber);

                        if (snapshotComponent != nullptr)
                        {
                            auto readResultFromSnapshotComponent = co_await snapshotComponent->ReadAsync(key, visibilitySequenceNumber, readMode);
                            if (readResultFromSnapshotComponent->HasValue())
                            {
                                KSharedPtr<VersionedItem<TValue>> versionedItemFromSnapshotComponent = readResultFromSnapshotComponent->VersionedItem;
                                STORE_ASSERT(versionedItemFromSnapshotComponent != nullptr, "versionedItemFromSnapshotComponent != nullptr");

                                readResultSPtr = readResultFromSnapshotComponent;
                                itemPresentInSnapshotComponent = true;
                            }
                        }
                    }

                    if (!itemPresentInSnapshotComponent)
                    {
                        readResultSPtr = co_await ReadFromConsolidatedStateAsync(key, visibilitySequenceNumber, readMode, cancellationToken);
                    }
                }

                co_return readResultSPtr;
            }

            ktl::Awaitable<KSharedPtr<StoreComponentReadResult<TValue>>> ReadFromConsolidatedStateAsync(
                __in TKey & key,
                __in LONG64 visibilitySequenceNumber,
                __in ReadMode readMode,
                __in ktl::CancellationToken const & cancellationToken)
            {
                KSharedPtr<VersionedItem<TValue>> versionedItem = nullptr;
                TValue value = TValue();

                // Note: Retry needs to be done at this layer since every time a load fails due to AddRef failure, the versioned item needs to be re-read
                while (true)
                {
                    if (visibilitySequenceNumber == Constants::InvalidLsn)
                    {
                        versionedItem = consolidationManagerSPtr_->Read(key);
                    }
                    else
                    {
                        versionedItem = consolidationManagerSPtr_->Read(key, visibilitySequenceNumber);
                    }

                    if (versionedItem == nullptr || readMode == ReadMode::Off || versionedItem->GetRecordKind() == RecordKind::DeletedVersion)
                    {
                        break;
                    }

                    // If we find a versioned item, get its value
                    bool shouldValueBeLoaded = false;

                    {
                        versionedItem->AcquireLock();
                        KFinally([&] { versionedItem->ReleaseLock(*traceComponent_); });
                        if (versionedItem->IsInMemory())
                        {
                            versionedItem->SetInUse(true);
                            value = versionedItem->GetValue();
                            break;
                        }
                        else
                        {
                            shouldValueBeLoaded = true;
                        }
                    }

                    if (shouldValueBeLoaded)
                    {
                        bool hasValue = co_await TryLoadValueAsync(*versionedItem, readMode, value);
                        if (hasValue)
                        {
                            if (readMode == ReadMode::CacheResult)
                            {
                                // If there are multiple loads in progress there could be some overcounting here - not worth locking for it.
                                consolidationManagerSPtr_->AddToMemorySize(versionedItem->GetValueSize());
                            }

                            break;
                        }
                    }
                }

                KSharedPtr<StoreComponentReadResult<TValue>> resultSPtr = nullptr;
                StoreComponentReadResult<TValue>::Create(versionedItem, value, this->GetThisAllocator(), resultSPtr);
                co_return resultSPtr;
            }

            ktl::Awaitable<bool> TryLoadValueAsync(__in VersionedItem<TValue> & versionedItem, __in ReadMode readMode, __out TValue & value)
            {
                SharedException::CSPtr exceptionCSPtr = nullptr;
                bool currentAddRefSucceeded = false;
                bool nextAddRefSucceeded = false;
                bool mergeAddRefSucceeded = false;
                value = TValue();

                bool useMergeTable = false;
                bool useNextTable = false;
                bool useCurrentTable = false;

                bool successful = true;

                // Snap tables upfront so that the current does not become nextby the time this function completes
                MetadataTable::SPtr cachedCurrentMetadataTableSPtr = currentMetadataTableSPtr_.Get();
                STORE_ASSERT(cachedCurrentMetadataTableSPtr != nullptr, "current metadata table cannot be null");

                MetadataTable::SPtr cachedNextMetadataTableSPtr = nextMetadataTableSPtr_.Get();
                MetadataTable::SPtr cachedMergeMetadataTableSPtr = mergeMetadataTableSPtr_.Get();

                try
                {
                    STORE_ASSERT(versionedItem.GetRecordKind() != RecordKind::DeletedVersion, "Versioned item should not be a deleted kind");
                    KSharedPtr<VersionedItem<TValue>> versionedItemSPtr = &versionedItem;
                    // Snap the current and next table upfront in the order of current first and then next.
                    // Snap current and the next upfront so that current does not become next by the time this function completes
                    cachedCurrentMetadataTableSPtr = currentMetadataTableSPtr_.Get();

                    currentAddRefSucceeded = cachedCurrentMetadataTableSPtr->TryAddReference();
                    if (!currentAddRefSucceeded)
                    {
                        successful = false;
                    }
                    else
                    {
                        useCurrentTable = true;

                        if (cachedNextMetadataTableSPtr != nullptr)
                        {
                            nextAddRefSucceeded = cachedNextMetadataTableSPtr->TryAddReference();
                            if (!nextAddRefSucceeded)
                            {
                                successful = false;
                            }
                        }

                        if (successful)
                        {
                            useNextTable = nextAddRefSucceeded;

                            if (cachedMergeMetadataTableSPtr != nullptr)
                            {
                                mergeAddRefSucceeded = cachedMergeMetadataTableSPtr->TryAddReference();
                                if (!mergeAddRefSucceeded)
                                {
                                    successful = false;
                                }
                                else
                                {
                                    useMergeTable = true;
                                }
                            }
                        }
                    }

                    if (successful)
                    {
                        // Order is important. Check the merge table first and then next
                        if (useMergeTable && cachedMergeMetadataTableSPtr != nullptr && cachedMergeMetadataTableSPtr->Table->ContainsKey(versionedItemSPtr->GetFileId()))
                        {
                            value = co_await versionedItemSPtr->GetValueAsync(*cachedMergeMetadataTableSPtr, *valueConverterSPtr_, readMode, *traceComponent_, ktl::CancellationToken::None);
                        }
                        else
                        {
                            // Load from next metadata table
                            if (useNextTable && cachedNextMetadataTableSPtr != nullptr && cachedNextMetadataTableSPtr->Table->ContainsKey(versionedItemSPtr->GetFileId()))
                            {
                                value = co_await versionedItemSPtr->GetValueAsync(*cachedNextMetadataTableSPtr, *valueConverterSPtr_, readMode, *traceComponent_, ktl::CancellationToken::None);
                            }
                            else if (useCurrentTable)
                            {
                                // Load using current metadata table
                                STORE_ASSERT(cachedCurrentMetadataTableSPtr->Table->ContainsKey(versionedItemSPtr->GetFileId()), "Current metadata table must contain the file id");
                                value = co_await versionedItemSPtr->GetValueAsync(*cachedCurrentMetadataTableSPtr, *valueConverterSPtr_, readMode, *traceComponent_, ktl::CancellationToken::None);
                            }
                        }
                    }
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"TryLoadValueAsync", e);
                    exceptionCSPtr = SharedException::Create(e, this->GetThisAllocator());
                    successful = false;
                }

                if (currentAddRefSucceeded)
                {
                    co_await cachedCurrentMetadataTableSPtr->ReleaseReferenceAsync();
                }

                if (nextAddRefSucceeded)
                {
                    co_await cachedNextMetadataTableSPtr->ReleaseReferenceAsync();
                }

                if (mergeAddRefSucceeded)
                {
                    co_await cachedMergeMetadataTableSPtr->ReleaseReferenceAsync();
                }

                if (exceptionCSPtr != nullptr)
                {
                    auto exec = exceptionCSPtr->Info;
                    throw exec;
                }

                co_return successful;
            }

            ktl::Awaitable<void> CheckpointAsync(__in ktl::CancellationToken const & cancellationToken)
            {
                // Acquire prime lock for checkpointing.
                typename StoreTransaction<TKey, TValue>::SPtr storeTransactionSPtr = nullptr;

                try
                {
                    NTSTATUS status = StoreTransaction<TKey, TValue>::Create(
                        CreateTransactionId(),
                        storeId_,
                        *keyComparerSPtr_,
                        *traceComponent_,
                        this->GetThisAllocator(),
                        storeTransactionSPtr);
                    Diagnostics::Validate(status);

                    co_await storeTransactionSPtr->AcquirePrimeLockAsync(*lockManager_, LockMode::Shared, Common::TimeSpan::FromMilliseconds(1000), false);

                    // If the operation was cancelled during lock await, then terminate.
                    cancellationToken.ThrowIfCancellationRequested();
                }
                catch (ktl::Exception & e)
                {
                    TraceException(L"TStore.CheckpointAsync.AcquireLock", e);
                    throw;
                }

                KFinally([&]
                {
                    storeTransactionSPtr->Close();
                });

                try
                {
                    StoreEventSource::Events->StoreCheckpointAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"dehydrating");

                    KSharedPtr<DifferentialStoreComponent<TKey, TValue>> deltaStateSPtr = deltaDifferentialStoreComponentSPtr_.Get();

                    // Sort
                    STORE_ASSERT(deltaStateSPtr != nullptr, "Delta state cannot be null");
                    deltaStateSPtr->Sort();

                    if (hasPersistedState_ == false)
                    {
                        // Once the delta state has been sorted, skip writing to disk

                        // Reset delta state.
                        deltaDifferentialStoreComponentSPtr_.Put(nullptr);

                        MetadataTable::SPtr tempMetadataTableSPtr = nullptr;
                        NTSTATUS status = MetadataTable::Create(this->GetThisAllocator(), tempMetadataTableSPtr, GetCheckpointLSN());
                        Diagnostics::Validate(status);

                        co_await ConsolidateAndSetNextMetadataTableAsync(*tempMetadataTableSPtr, true);

                        co_return;
                    }

                    auto diffItemsEnumeratorSPtr = deltaStateSPtr->GetKeyAndValues();
                    KSharedPtr<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> itemsEnumeratorSPtr =
                        static_cast<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>> *>(diffItemsEnumeratorSPtr.RawPtr());

                    STORE_ASSERT(itemsEnumeratorSPtr != nullptr, "itemsEnumeratorSPtr != nullptr");

                    auto cachedCurrentMetadataTableSPtr = currentMetadataTableSPtr_.Get();
                    STORE_ASSERT(cachedCurrentMetadataTableSPtr != nullptr, "cachedCurrentMetadataTableSPtr != nullptr");

                    // When there are items to be checkpointed.
                    if (deltaStateSPtr->isReadOnlyListNonEmpty())
                    {
                        KGuid fileGuid;
                        fileGuid.CreateNew();

                        KString::SPtr name = nullptr;
                        NTSTATUS status = KString::Create(name, this->GetThisAllocator(), KStringView::MaxGuidString);

                        Diagnostics::Validate(status);

                        BOOLEAN result = name->FromGUID(fileGuid);
                        STORE_ASSERT(result == TRUE, "Unable to get string from GUID");

                        KStringView fileName(*name);

                        auto fileId = IncrementFileId();
                        auto fileStamp = IncrementFileStamp();
                        auto path = GetCheckpointFilePath(fileName);

                        // todo: fix checkpoint to take in const path.
                        KString& filePath = const_cast<KString&>(*path);

                        CheckpointFile::SPtr checkpointFileSPtr = co_await CheckpointFile::CreateAsync<TKey, TValue>(
                            fileId,
                            filePath,
                            *itemsEnumeratorSPtr,
                            *keyConverterSPtr_,
                            *valueConverterSPtr_,
                            fileStamp,
                            this->GetThisAllocator(),
                            *traceComponent_,
                            perfCounters_,
                            true);

                        ASSERT_IF(checkpointFileSPtr == nullptr, "Checkpoint file cannot be null");

                        KSharedPtr<KString> keyFileNameSPtr = nullptr;
                        status = KString::Create(keyFileNameSPtr, this->GetThisAllocator(), fileName);
                        Diagnostics::Validate(status);

                        // Update  metadata to include the new id and checkpoint it.
                        FileMetadata::SPtr fileMetadataSPtr = nullptr;

                        status = FileMetadata::Create(
                            fileId,
                            *keyFileNameSPtr,
                            checkpointFileSPtr->KeyCount,
                            checkpointFileSPtr->KeyCount,
                            fileStamp,
                            checkpointFileSPtr->DeletedKeyCount,
                            false,
                            this->GetThisAllocator(),
                            *traceComponent_,
                            fileMetadataSPtr);
                        Diagnostics::Validate(status);

                        fileMetadataSPtr->CheckpointFileSPtr = *checkpointFileSPtr;

                        // Populate next metadata table
                        MetadataTable::SPtr tempMetadataTableSPtr = nullptr;
                        status = MetadataTable::Create(this->GetThisAllocator(), tempMetadataTableSPtr, GetCheckpointLSN());
                        Diagnostics::Validate(status);

                        CopyMetadataTable(*cachedCurrentMetadataTableSPtr, *tempMetadataTableSPtr);
                        MetadataManager::AddFile(*(tempMetadataTableSPtr->Table), fileId, *fileMetadataSPtr);

                        co_await ConsolidateAndSetNextMetadataTableAsync(*tempMetadataTableSPtr, true);

                        auto cachedNextMetadataTableSPtr = nextMetadataTableSPtr_.Get();
                        STORE_ASSERT(cachedNextMetadataTableSPtr != nullptr, "cachedNextMetadataTableSPtr != nullptr");

                        // Checkpoint metadata table
                        co_await MetadataManager::WriteAsync(*cachedNextMetadataTableSPtr, *tmpMetadataFilePath_, this->GetThisAllocator());
                    }
                    else
                    {
                        // Following copy, updated checkpoint file needs to be written even there is no delta and no clear.

                        // Populate next metadata table so that current reference is not changed.
                        MetadataTable::SPtr tempMetadataTableSPtr = nullptr;
                        NTSTATUS status = MetadataTable::Create(this->GetThisAllocator(), tempMetadataTableSPtr, GetCheckpointLSN());
                        Diagnostics::Validate(status);

                        CopyMetadataTable(*cachedCurrentMetadataTableSPtr, *tempMetadataTableSPtr);

                        co_await ConsolidateAndSetNextMetadataTableAsync(*tempMetadataTableSPtr, true);

                        auto cachedNextMetadataTableSPtr = nextMetadataTableSPtr_.Get();
                        STORE_ASSERT(cachedNextMetadataTableSPtr != nullptr, "cachedNextMetadataTableSPtr != nullptr");

                        // Checkpoint metadata table
                        co_await MetadataManager::WriteAsync(*cachedNextMetadataTableSPtr, *tmpMetadataFilePath_, this->GetThisAllocator());
                    }

                    // Reset delta state.
                    deltaDifferentialStoreComponentSPtr_.Put(nullptr);
                }
                catch (ktl::Exception& e)
                {
                    TraceException(L"TStore.CheckpointAsync", e);
                    STORE_ASSERT(
                        e.GetStatus() == STATUS_INVALID_PARAMETER_3 ||
                        e.GetStatus() == STATUS_INVALID_PARAMETER_4 ||
                        e.GetStatus() == SF_STATUS_OBJECT_CLOSED ||
                        e.GetStatus() == SF_STATUS_TIMEOUT,
                        "Unexpected exception in CheckpointAsync");
                    storeTransactionSPtr->Close();

                    throw;
                }

                co_return;
            }

            ktl::Awaitable<void> ConsolidateAndSetNextMetadataTableAsync(
                __in MetadataTable& tempMetadataTable,
                __in bool consolidate = true)
            {
                MetadataTable::SPtr tempMetadataTableSPtr = &tempMetadataTable;

                if (EnableBackgroundConsolidation)
                {
                    ProcessConsolidationTask(*tempMetadataTableSPtr);

                  // Update the consolidated state only after assigning the next metadata table.
                  // Set next metadatatable only after adding and merging of items.
                  // Merge metadata table gets updated as part of processing consolidation task.

                  nextMetadataTableSPtr_.Put(Ktl::Move(tempMetadataTableSPtr));

                  if (consolidate)
                  {
                     auto cachedConsolidationTcsSPtr = consolidationTcsSPtr_.Get();

                        // Do not start consolidation task, if it is already in progress
                        if (cachedConsolidationTcsSPtr == nullptr)
                        {
                            mergeMetadataTableSPtr_.Put(Ktl::Move(nullptr));

                            ktl::AwaitableCompletionSource<PostMergeMetadataTableInformation::SPtr>::SPtr newConsolidationTcsSPtr = nullptr;

                            // Consolidate after writing file so that the correct offset can be updated
                            NTSTATUS status = ktl::AwaitableCompletionSource<PostMergeMetadataTableInformation::SPtr>::Create(this->GetThisAllocator(), STORE_TAG, newConsolidationTcsSPtr);
                            Diagnostics::Validate(status);

                            auto cachedTempMetadataTableSPtr = nextMetadataTableSPtr_.Get();
                            auto cancellationToken = consolidationTaskCancellationSourceSPtr_->Token;
                            consolidationTcsSPtr_.Put(Ktl::Move(newConsolidationTcsSPtr));

                            cachedConsolidationTcsSPtr = consolidationTcsSPtr_.Get();
                            ktl::Task task = consolidationManagerSPtr_->ConsolidateAsync(*cachedTempMetadataTableSPtr, *cachedConsolidationTcsSPtr, perfCounters_, cancellationToken);
                            STORE_ASSERT(task.IsTaskStarted(), "Expected consolidation task to start");
                        }
                    }
                }
                else
                {
                    auto cachedConsolidationTaskSPtr = consolidationTcsSPtr_.Get();

                    STORE_ASSERT(cachedConsolidationTaskSPtr == nullptr, "ConsolidationTask should be null");
                    mergeMetadataTableSPtr_.Put(Ktl::Move(nullptr));

                    if (consolidate)
                    {
                        ktl::AwaitableCompletionSource<PostMergeMetadataTableInformation::SPtr>::SPtr newConsolidationTcsSPtr = nullptr;

                        // Consolidate after writing file so that the correct offset can be updated
                        NTSTATUS status = ktl::AwaitableCompletionSource<PostMergeMetadataTableInformation::SPtr>::Create(this->GetThisAllocator(), STORE_TAG, newConsolidationTcsSPtr);
                        Diagnostics::Validate(status);
                        auto token = consolidationTaskCancellationSourceSPtr_->Token;
                        ktl::Task task = consolidationManagerSPtr_->ConsolidateAsync(*tempMetadataTableSPtr, *newConsolidationTcsSPtr, perfCounters_, token);
                        STORE_ASSERT(task.IsTaskStarted(), "Expected consolidation task to start");
                        co_await newConsolidationTcsSPtr->GetAwaitable();
                        consolidationTcsSPtr_.Put(Ktl::Move(newConsolidationTcsSPtr));
                        ProcessConsolidationTask(*tempMetadataTableSPtr);
                        mergeMetadataTableSPtr_.Put(Ktl::Move(nullptr));
                    }

                  // Update the consolidated state only after assigning the next metadata table.
                  // Set next metadata table only after adding and merging of items.
                  // Merge metadata table gets updated as a part of processing consolidation task
                  nextMetadataTableSPtr_.Put(Ktl::Move(tempMetadataTableSPtr));

                  if (consolidate)
                  {
                     consolidationManagerSPtr_->ResetToNewAggregatedState();
                  }

                    cachedConsolidationTaskSPtr = consolidationTcsSPtr_.Get();
                    STORE_ASSERT(cachedConsolidationTaskSPtr == nullptr, "ConsolidationTask should be null");
                }

                co_return;
            }

            void ProcessConsolidationTask(__in MetadataTable & tmpMetadataTable)
            {
                PostMergeMetadataTableInformation::SPtr mergeMetadataTableInformationSPtr = nullptr;
                auto cachedConsolidationTaskSPtr = consolidationTcsSPtr_.Get();

                // Check if previous consolidation task has completed
                if (cachedConsolidationTaskSPtr != nullptr && cachedConsolidationTaskSPtr->IsCompleted())
                {
                    if (cachedConsolidationTaskSPtr->IsExceptionSet())
                    {
                        ktl::Exception exception = ktl::Exception::ToException(cachedConsolidationTaskSPtr->GetAwaitable().GetExceptionHandle());
                        consolidationTcsSPtr_.Put(nullptr);
                        throw exception;
                    }
                    else
                    {
                        mergeMetadataTableInformationSPtr = cachedConsolidationTaskSPtr->GetResult();
                        consolidationTcsSPtr_.Put(nullptr);
                    }
                }

                if (mergeMetadataTableInformationSPtr != nullptr)
                {
                    if (mergeMetadataTableInformationSPtr->NewMergedFileSPtr != nullptr)
                    {
                        auto fileMetadataSPtr = mergeMetadataTableInformationSPtr->NewMergedFileSPtr;
                        MetadataManager::AddFile(*tmpMetadataTable.Table, fileMetadataSPtr->FileId, *fileMetadataSPtr);
                    }

                    auto deletedFileIdsSPtr = mergeMetadataTableInformationSPtr->DeletedFileIdsSPtr;

                    for (ULONG32 i = 0; i < deletedFileIdsSPtr->Count(); i++)
                    {
                        auto fileId = (*deletedFileIdsSPtr)[i];

                        // Mark the file as about to be deleted, and remove it from the metadata table.
                        FileMetadata::SPtr fileMetadataSPtr = nullptr;
                        STORE_ASSERT(tmpMetadataTable.Table->TryGetValue(fileId, fileMetadataSPtr), "Expected to find {1} in table", fileId);
                        fileMetadataSPtr->CanBeDeleted = true;

                        // Add ref for files to be deleted. These are commneted out because the AddRef and ReleaseRef cancel eaco other out.
                        //fileMetadataSPtr->AddReference();
                        bool add = filesToBeDeletedSPtr_->TryAdd(fileMetadataSPtr, /*immediate:*/ true);
                        STORE_ASSERT(add, "Failed to add file id {1} to files to be deleted", fileMetadataSPtr->FileId);

                        // Release ref for file metadata, but it will be deleted on complete checkpoint along wtih current metadatatale being replaced
                        // co_await fileMetadataSPtr->ReleaseReferenceAsync();
                        MetadataManager::RemoveFile(*tmpMetadataTable.Table, fileId);
                    }

                    auto cachedConsolidationTcsSPtr = consolidationTcsSPtr_.Get();
                    STORE_ASSERT(cachedConsolidationTcsSPtr == nullptr, "ConsolidationTask should be null");
                }
            }

            ktl::Awaitable<void> CancelBackgroundConsolidationTaskAsync()
            {
                // If consolidation task is in progress, then cancel it
                consolidationTaskCancellationSourceSPtr_->Cancel();
                KSharedPtr<ktl::AwaitableCompletionSource<PostMergeMetadataTableInformation::SPtr>> cachedConsolidationTcsSPtr = consolidationTcsSPtr_.Get();

                try
                {
                    if (cachedConsolidationTcsSPtr != nullptr)
                    {
                        co_await cachedConsolidationTcsSPtr->GetAwaitable();
                    }
                }
                catch (ktl::Exception const & e)
                {
                    // Trace and swallow exception
                    TraceException(L"TStore.Cleanup - cancelling consolidation task", e);
                }
            }

            ktl::Awaitable<void> CancelSweepTaskAsync()
            {
                // If sweep task is in progress, then cancel it
                co_await sweepManagerSPtr_->CancelSweepTaskAsync();
            }

            TxnReplicator::ITransactionalReplicator::SPtr GetReplicator() const
            {
                STORE_ASSERT(transactionalReplicatorSPtr_ != nullptr, "transactionalReplicatorSPtr_ != nullptr")

                    TxnReplicator::ITransactionalReplicator::SPtr transactionalReplicatorSPtr = transactionalReplicatorSPtr_->TryGetTarget();
                STORE_ASSERT(transactionalReplicatorSPtr != nullptr, "transactionalReplicatorSPtr != nullptr");

                return transactionalReplicatorSPtr;
            }

            ULONG64 IncrementFileStamp() override
            {
                ULONG64 logicalTimeStamp = 0;
                K_LOCK_BLOCK(timeStampLock_)
                {
                    logicalTimeStamp = ++logicalTimeStamp_;
                }

                return logicalTimeStamp;
            }

            ULONG32 IncrementFileId() override
            {
                ULONG32 fileId = 0;
                K_LOCK_BLOCK(fileIdLock_)
                {
                    fileId = ++fileId_;
                }

                return fileId;
            }

            LONG64 CreateTransactionId() override
            {
                TxnReplicator::Transaction::SPtr replicatorTransaction = TxnReplicator::Transaction::CreateTransaction(*GetReplicator(), this->GetThisAllocator());
                LONG64 transactionId = replicatorTransaction->TransactionId;
                replicatorTransaction->Dispose();
                return transactionId;
            }

            void CopyMetadataTable(__in MetadataTable& src, __in MetadataTable& dest)
            {
                IEnumerator<KeyValuePair<ULONG32, FileMetadata::SPtr>>::SPtr enumerator = src.Table->GetEnumerator();

                while (enumerator->MoveNext())
                {
                    KeyValuePair<ULONG32, FileMetadata::SPtr> entry = enumerator->Current();
                    auto fileId = entry.Key;
                    auto fileMetadataSPtr = entry.Value;
                    fileMetadataSPtr->AddReference();
                    dest.Table->Add(fileId, fileMetadataSPtr);
                }
            }

            ktl::Awaitable<KSharedPtr<StoreTransaction<TKey, TValue>>> ApplyOnPrimaryAsync(
                __in LONG64 sequenceNumber,
                __in TxnReplicator::TransactionBase const & replicatorTransaction,
                __in const MetadataOperationData& metadataOperationData)
            {
                KSharedPtr<DifferentialStoreComponent<TKey, TValue>> componentSPtr = differentialStoreComponentSPtr_.Get();
                STORE_ASSERT(componentSPtr != nullptr, "componentSPtr != nullptr");

                KSharedPtr<StoreTransaction<TKey, TValue>> storeTransactionSPtr = nullptr;
                inflightReadWriteStoreTransactionsSPtr_->TryGetValue(replicatorTransaction.TransactionId, storeTransactionSPtr);
                STORE_ASSERT(storeTransactionSPtr != nullptr, "storeTransactionSPtr != nullptr");

                StoreEventSource::Events->StoreApplyOnPrimaryAsync(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    sequenceNumber,
                    storeTransactionSPtr->Id);

                MetadataOperationData::CSPtr metadataOperationDataCSPtr(&metadataOperationData);
                STORE_ASSERT(metadataOperationDataCSPtr != nullptr, "metadataOperationDataCSPtr != nullptr");

                const MetadataOperationDataK<TKey>* metadataOperationDataK = static_cast<const MetadataOperationDataK<TKey> *>(&metadataOperationData);

                KSharedPtr<MetadataOperationDataK<TKey> const> metadataOperationDataKCSPtr = metadataOperationDataK;
                STORE_ASSERT(metadataOperationDataKCSPtr != nullptr, "metadataOperationDataKCSPtr != nullptr");

                TKey key = metadataOperationDataKCSPtr->Key;

                const StoreTransaction<TKey, TValue>& storeTransaction = *storeTransactionSPtr;

                KSharedPtr<WriteSetStoreComponent<TKey, TValue>> component = (const_cast<StoreTransaction<TKey, TValue>&>(storeTransaction)).GetComponent(func_);
                STORE_ASSERT(component != nullptr, "component != nullptr");
                auto versionedItem = component->Read(key);
                STORE_ASSERT(versionedItem != nullptr, "versionedItem != nullptr");

                // Update commit sequence number
                versionedItem->SetVersionSequenceNumber(sequenceNumber);

                componentSPtr->Add(key, *versionedItem, *consolidationManagerSPtr_);

                if (metadataOperationDataKCSPtr->ModificationType == StoreModificationType::Add)
                {
                    IncrementCount(replicatorTransaction.TransactionId, sequenceNumber);

                    auto keySize = (*metadataOperationDataKCSPtr->KeyBytes)[0]->QuerySize();
                    auto cachedEstimator = keySizeEstimatorSPtr_.Get();
                    cachedEstimator->AddKeySize(keySize);
                }
                else if (metadataOperationDataKCSPtr->ModificationType == StoreModificationType::Remove)
                {
                    DecrementCount(replicatorTransaction.TransactionId, sequenceNumber);
                }

                co_await FireSingleItemNotificationOnPrimaryAsync(replicatorTransaction, key, *metadataOperationDataCSPtr, sequenceNumber);

                co_return storeTransactionSPtr;
            }

            ktl::Awaitable<KSharedPtr<StoreTransaction<TKey, TValue>>> ApplyOnRecoveryAsync(
                __in LONG64 sequenceNumber,
                __in TxnReplicator::TransactionBase const & replicatorTransaction,
                __in MetadataOperationData const & metadataOperationData,
                __in OperationData const & data)
            {
                // Retrieve redo/undo operation.
                auto operationRedoUndo = RedoUndoOperationData::Deserialize(data, this->GetThisAllocator());
                STORE_ASSERT(metadataOperationData.ModificationType == StoreModificationType::Enum::Add ||
                    metadataOperationData.ModificationType == StoreModificationType::Enum::Update ||
                    metadataOperationData.ModificationType == StoreModificationType::Enum::Remove,
                    "Modification type should be add, remove or update");

                STORE_ASSERT(metadataOperationData.KeyBytes != nullptr, "metadataOperationData.KeyBytes != nullptr");

                // Find or create the store transaction correspondent to this replicator transaction.
                bool firstCreated = false;
                KSharedPtr<StoreTransaction<TKey, TValue>> storeTransactionSPtr = nullptr;
                TxnReplicator::TransactionBase& transaction = const_cast<TxnReplicator::TransactionBase&>(replicatorTransaction);

                KSharedPtr<IStoreTransaction<TKey, TValue>> istoreTransactionSPtr = nullptr;
                firstCreated = !CreateOrFindTransaction(transaction, istoreTransactionSPtr);
                storeTransactionSPtr = static_cast<StoreTransaction<TKey, TValue>*>(istoreTransactionSPtr.RawPtr());

                STORE_ASSERT(storeTransactionSPtr != nullptr, "storeTransactionSPtr != nullptr");

                auto keyLockResourceNameHash = GetHash(*metadataOperationData.KeyBytes);

                if (firstCreated)
                {
                    StoreEventSource::Events->StoreApplyOnRecoveryAsync(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"starting",
                        sequenceNumber,
                        storeTransactionSPtr->Id,
                        keyLockResourceNameHash,
                        static_cast<ULONG32>(metadataOperationData.ModificationType));
                }
                else
                {
                    StoreEventSource::Events->StoreApplyOnRecoveryAsync(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"completed",
                        sequenceNumber,
                        storeTransactionSPtr->Id,
                        keyLockResourceNameHash,
                        static_cast<ULONG32>(metadataOperationData.ModificationType));
                }

                MetadataOperationData::CSPtr metadataOperationDataCSPtr = &metadataOperationData;

                // Apply Operation
                switch (metadataOperationData.ModificationType)
                {
                case StoreModificationType::Enum::Add:
                    co_await OnApplyAddAsync(sequenceNumber, *storeTransactionSPtr, *metadataOperationDataCSPtr, *operationRedoUndo, true);
                    break;
                case StoreModificationType::Enum::Update:
                    co_await OnApplyUpdateAsync(sequenceNumber, *storeTransactionSPtr, *metadataOperationDataCSPtr, *operationRedoUndo, true);
                    break;
                case StoreModificationType::Enum::Remove:
                    co_await OnApplyRemoveAsync(sequenceNumber, *storeTransactionSPtr, *metadataOperationDataCSPtr, *operationRedoUndo, true);
                    break;
                }

                co_return firstCreated ? storeTransactionSPtr : nullptr;
            }

            ktl::Awaitable<KSharedPtr<StoreTransaction<TKey, TValue>>> ApplyOnSecondaryAsync(
                __in LONG64 sequenceNumber,
                __in TxnReplicator::TransactionBase const & replicatorTransaction,
                __in MetadataOperationData const & metadataOperationData,
                __in RedoUndoOperationData const & operationRedoUndo,
                __in bool isIdempotent)
            {
                // Retrieve redo/undo operation
                auto modificationType = metadataOperationData.ModificationType;
                STORE_ASSERT(
                    modificationType == StoreModificationType::Enum::Add ||
                    modificationType == StoreModificationType::Enum::Remove ||
                    modificationType == StoreModificationType::Enum::Update,
                    "unexpected store operation type {1}", static_cast<ULONG32>(modificationType));

                STORE_ASSERT(metadataOperationData.KeyBytes != nullptr, "unexpected key bytes");

                // Find or create the store transaction correspondent to this replicator transaction.
                bool firstCreated = false;
                KSharedPtr<StoreTransaction<TKey, TValue>> storeTransactionSPtr = nullptr;
                TxnReplicator::TransactionBase& transaction = const_cast<TxnReplicator::TransactionBase&>(replicatorTransaction);

                KSharedPtr<IStoreTransaction<TKey, TValue>> istoreTransactionSPtr = nullptr;
                firstCreated = !CreateOrFindTransaction(transaction, istoreTransactionSPtr);
                storeTransactionSPtr = static_cast<StoreTransaction<TKey, TValue>*>(istoreTransactionSPtr.RawPtr());

                STORE_ASSERT(storeTransactionSPtr != nullptr, "storeTransactionSPtr != nullptr");

                auto keyLockResourceNameHash = GetHash(*metadataOperationData.KeyBytes);

                if (firstCreated)
                {
                    StoreEventSource::Events->StoreApplyOnSecondaryAsync(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"starting",
                        sequenceNumber,
                        storeTransactionSPtr->Id,
                        keyLockResourceNameHash,
                        static_cast<ULONG32>(modificationType));
                }
                else
                {
                    StoreEventSource::Events->StoreApplyOnSecondaryAsync(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"completed",
                        sequenceNumber,
                        storeTransactionSPtr->Id,
                        keyLockResourceNameHash,
                        static_cast<ULONG32>(modificationType));
                }

                MetadataOperationData::CSPtr metadataOperationDataCSPtr = &metadataOperationData;
                RedoUndoOperationData::CSPtr redoUndoDataCSPtr = &operationRedoUndo;

                // Apply operation
                switch (metadataOperationData.ModificationType)
                {
                case StoreModificationType::Enum::Add:
                    co_await OnApplyAddAsync(sequenceNumber, *storeTransactionSPtr, *metadataOperationDataCSPtr, *redoUndoDataCSPtr, isIdempotent);
                    break;
                case StoreModificationType::Enum::Update:
                    co_await OnApplyUpdateAsync(sequenceNumber, *storeTransactionSPtr, *metadataOperationDataCSPtr, *redoUndoDataCSPtr, isIdempotent);
                    break;
                case StoreModificationType::Enum::Remove:
                    co_await OnApplyRemoveAsync(sequenceNumber, *storeTransactionSPtr, *metadataOperationDataCSPtr, *redoUndoDataCSPtr, isIdempotent);
                    break;
                }

                co_return firstCreated ? storeTransactionSPtr : nullptr;
            }

            ktl::Awaitable<KSharedPtr<StoreTransaction<TKey, TValue>>> UndoFalseProgressOnSecondaryAsync(
                __in LONG64 sequenceNumber,
                __in TxnReplicator::TransactionBase const & replicatorTransaction,
                __in OperationData const & metadataOperationData,
                __in OperationData const * const operationData)
            {
                STORE_ASSERT(operationData == nullptr, "operation data must be null");

                MetadataOperationData::CSPtr operationMetadataCSPtr = MetadataOperationData::Deserialize(1, this->GetThisAllocator(), &metadataOperationData);

                auto modificationType = operationMetadataCSPtr->ModificationType;
                STORE_ASSERT(
                    modificationType == StoreModificationType::Enum::Add ||
                    modificationType == StoreModificationType::Enum::Remove ||
                    modificationType == StoreModificationType::Enum::Update,
                    "unexpected store operation type {1}", static_cast<ULONG32>(modificationType));
                STORE_ASSERT(operationMetadataCSPtr->KeyBytes != nullptr, "unexpected key bytes");

                // Find or create the store transaction correspondent to this replicator transaction
                bool firstCreated = false;
                KSharedPtr<StoreTransaction<TKey, TValue>> storeTransactionSPtr = nullptr;
                TxnReplicator::TransactionBase& transaction = const_cast<TxnReplicator::TransactionBase&>(replicatorTransaction);

                KSharedPtr<IStoreTransaction<TKey, TValue>> istoreTransactionSPtr = nullptr;
                firstCreated = !CreateOrFindTransaction(transaction, istoreTransactionSPtr);
                storeTransactionSPtr = static_cast<StoreTransaction<TKey, TValue>*>(istoreTransactionSPtr.RawPtr());

                STORE_ASSERT(storeTransactionSPtr != nullptr, "storeTransactionSPtr != nullptr");

                if (firstCreated)
                {
                    StoreEventSource::Events->StoreUndoFalseProgressOnSecondaryAsync(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"starting",
                        sequenceNumber,
                        storeTransactionSPtr->Id);
                }
                else
                {
                    StoreEventSource::Events->StoreUndoFalseProgressOnSecondaryAsync(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"completed",
                        sequenceNumber,
                        storeTransactionSPtr->Id);
                }

                // Undo false progress
                co_await UndoFalseProgressAsync(sequenceNumber, *storeTransactionSPtr, *operationMetadataCSPtr, operationMetadataCSPtr->ModificationType);

                co_return firstCreated ? storeTransactionSPtr : nullptr;
            }

            ktl::Awaitable<void> UndoFalseProgressAsync(
                __in LONG64 sequenceNumber,
                __in StoreTransaction<TKey, TValue> & rwtx,
                __in MetadataOperationData const & metadataOperationData,
                __in StoreModificationType::Enum storeModificationType)
            {
                // Retrieve key and value
                TKey key;
                try
                {
                    key = GetKeyFromBytes(*metadataOperationData.KeyBytes);
                }
                catch (const ktl::Exception &)
                {
                    StoreEventSource::Events->StoreUndoFalseProgressOnSecondaryAsyncError(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        rwtx.Id,
                        L"deserialization");

                    // Rethrow exception
                    throw;
                }

                try
                {
                    KSharedPtr<DifferentialStoreComponent<TKey, TValue>> cachedDifferentialState = differentialStoreComponentSPtr_.Get();
                    STORE_ASSERT(cachedDifferentialState != nullptr, "cachedDifferentialState should not be null");

                    // Modify differential state. This is done before any apply comes in the replication stream
                    bool dupeKeyInTxn = cachedDifferentialState->UndoFalseProgress(key, sequenceNumber, storeModificationType);
                    if (dupeKeyInTxn)
                    {
                        co_return;
                    }

                    KSharedPtr<VersionedItem<TValue>> differentialVersionedItemSPtr = cachedDifferentialState->Read(key);
                    KSharedPtr<StoreComponentReadResult<TValue>> readResultSPtr;

                    if (differentialVersionedItemSPtr != nullptr)
                    {
                        auto value = differentialVersionedItemSPtr->GetRecordKind() != RecordKind::DeletedVersion
                            ? differentialVersionedItemSPtr->GetValue()
                            : TValue();
                        auto status = StoreComponentReadResult<TValue>::Create(
                            differentialVersionedItemSPtr,
                            value,
                            this->GetThisAllocator(),
                            readResultSPtr);

                        if (!NT_SUCCESS(status))
                        {
                            throw ktl::Exception(status);
                        }
                    }
                    else
                    {
                        readResultSPtr = co_await ReadFromConsolidatedStateAsync(
                            key,
                            Constants::InvalidLsn,
                            ReadMode::CacheResult,
                            ktl::CancellationToken::None);
                    }

                    KSharedPtr<StoreTransaction<TKey, TValue>> rwtxSPtr = &rwtx;
                    MetadataOperationData::CSPtr metadataOperationDataCSPtr = &metadataOperationData;

                    co_await FireUndoNotificationsAsync(*rwtx.ReplicatorTransaction, key, *metadataOperationDataCSPtr, readResultSPtr, sequenceNumber);
                    UpdateCountForUndoOperation(rwtxSPtr->Id, sequenceNumber, *metadataOperationDataCSPtr, readResultSPtr->VersionedItem);
                }
                catch (const ktl::Exception & e)
                {
                    // Check inner exception
                    TraceException(L"OnUndoFalseProgress", e, rwtx.Id);
                    STORE_ASSERT(e.GetStatus() == SF_STATUS_OBJECT_CLOSED || e.GetStatus() == SF_STATUS_TIMEOUT, "Unexpected exception");

                    // Rethrow inner exception
                    throw;
                }
            }

            void UpdateCountForUndoOperation(__in LONG64 txnId, __in LONG64 commitSequenceNumber, __in const MetadataOperationData & metadataOperationData, __in KSharedPtr<VersionedItem<TValue>> previousVersionedItem)
            {
                // Last operation on the key in txn is Add or Update
                if (metadataOperationData.ModificationType == StoreModificationType::Add || metadataOperationData.ModificationType == StoreModificationType::Update)
                {
                    if (previousVersionedItem == nullptr || previousVersionedItem->GetRecordKind() == RecordKind::DeletedVersion)
                    {
                        DecrementCount(txnId, commitSequenceNumber);
                        return;
                    }

                    return;
                }

                // Last operation on the key in txn is Remove
                if (metadataOperationData.ModificationType == StoreModificationType::Remove)
                {
                    if (previousVersionedItem == nullptr || previousVersionedItem->GetRecordKind() == RecordKind::DeletedVersion)
                    {
                        // If the previous versioned item does not exist, it implies this must be nooped
                        return;
                    }

                    IncrementCount(txnId, commitSequenceNumber);
                    return;
                }
            }

            ktl::Awaitable<void> OnApplyAddAsync(
                __in LONG64 sequenceNumber,
                __in StoreTransaction<TKey, TValue> const & storeTransaction,
                __in MetadataOperationData const & metadataOperationData,
                __in RedoUndoOperationData const & data,
                __in bool isIdempotent)
            {
                TKey key;
                TValue value;
                try
                {
                    key = GetKeyFromBytes(*metadataOperationData.KeyBytes);
                    value = GetValueFromBytes(*data.ValueOperationData);
                }
                catch (ktl::Exception& e)
                {
                    StoreEventSource::Events->StoreOnApplyAddError(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        storeTransaction.Id,
                        L"deserialization");

                    TraceException(L"OnApplyAdd", e, storeTransaction.Id);
                    throw;
                }

                auto keyLockResourceNameHash = GetHash(*metadataOperationData.KeyBytes);

                try
                {
                    auto cachedDifferentialStoreComponentSPtr = differentialStoreComponentSPtr_.Get();
                    STORE_ASSERT(cachedDifferentialStoreComponentSPtr != nullptr, "cachedDifferentialStoreComponentSPtr != nullptr");

                    if (!isIdempotent)
                    {
                        KSharedPtr<VersionedItem<TValue>> currentVersionedItemSPtr = cachedDifferentialStoreComponentSPtr->Read(key);
                        if (currentVersionedItemSPtr == nullptr)
                        {
                            currentVersionedItemSPtr = consolidationManagerSPtr_->Read(key);
                        }

                        STORE_ASSERT(currentVersionedItemSPtr == nullptr || currentVersionedItemSPtr->GetRecordKind() == RecordKind::DeletedVersion,
                            "Cannot add an item that already exists. lsn={1} txn={2} key={3}", sequenceNumber, storeTransaction.Id, keyLockResourceNameHash);
                    }

                    if (!isIdempotent || ShouldValueBeAddedToDifferentialState(key, sequenceNumber))
                    {
                        // Add the change to the store transaction write-set.
                        KSharedPtr<InsertedVersionedItem<TValue>> insertedVersionedItemSPtr = nullptr;
                        NTSTATUS status = InsertedVersionedItem<TValue>::Create(this->GetThisAllocator(), insertedVersionedItemSPtr);
                        Diagnostics::Validate(status);

                        insertedVersionedItemSPtr->InitializeOnApply(sequenceNumber, value);
                        insertedVersionedItemSPtr->SetValueSize(GetValueSize(data.ValueOperationData));
                        cachedDifferentialStoreComponentSPtr->Add(key, *insertedVersionedItemSPtr, *consolidationManagerSPtr_);

                        // Update count, notifications and trace
                        LONG64 newCount = IncrementCount(storeTransaction.ReplicatorTransaction->TransactionId, storeTransaction.ReplicatorTransaction->CommitSequenceNumber);
                        UNREFERENCED_PARAMETER(newCount);

                        auto keySize = (*metadataOperationData.KeyBytes)[0]->QuerySize();
                        auto cachedEstimator = keySizeEstimatorSPtr_.Get();
                        cachedEstimator->AddKeySize(keySize);

                        co_await FireItemAddedNotificationOnSecondaryAsync(*storeTransaction.ReplicatorTransaction, key, value, sequenceNumber);

                        //StoreEventSource::Events->StoreOnApplyAdd(
                        //    traceComponent_->PartitionId, traceComponent_->TraceTag,
                        //    sequenceNumber,
                        //    storeTransaction.Id,
                        //    keyLockResourceNameHash,
                        //    newCount);
                    }
                }
                catch (ktl::Exception& e)
                {
                    TraceException(L"OnApplyAdd", e, storeTransaction.Id, keyLockResourceNameHash);
                    STORE_ASSERT(
                        e.GetStatus() == SF_STATUS_OBJECT_CLOSED ||
                        e.GetStatus() == SF_STATUS_TIMEOUT,
                        "Unexpected exception in AcquireKeyReadLockAsync");

                    throw;
                }
            }

            ktl::Awaitable<void> OnApplyUpdateAsync(
                __in LONG64 sequenceNumber,
                __in StoreTransaction<TKey, TValue> const & storeTransaction,
                __in MetadataOperationData const & metadataOperationData,
                __in RedoUndoOperationData const & data,
                __in bool isIdempotent)
            {
                TKey key;
                TValue value;
                try
                {
                    key = GetKeyFromBytes(*metadataOperationData.KeyBytes);
                    value = GetValueFromBytes(*data.ValueOperationData);
                }
                catch (ktl::Exception& e)
                {
                    StoreEventSource::Events->StoreOnApplyUpdateError(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        storeTransaction.Id,
                        L"deserialization");

                    TraceException(L"OnApplyUpdate", e, storeTransaction.Id);
                    throw;
                }

                ULONG64 keyLockResourceNameHash = GetHash(*metadataOperationData.KeyBytes);

                try
                {
                    auto cachedDifferentialStoreComponentSPtr = differentialStoreComponentSPtr_.Get();
                    STORE_ASSERT(cachedDifferentialStoreComponentSPtr != nullptr, "cachedDifferentialStoreComponentSPtr != nullptr");

                    if (!isIdempotent)
                    {
                        KSharedPtr<VersionedItem<TValue>> currentVersionedItemSPtr = cachedDifferentialStoreComponentSPtr->Read(key);
                        if (currentVersionedItemSPtr == nullptr)
                        {
                            currentVersionedItemSPtr = consolidationManagerSPtr_->Read(key);
                        }

                        STORE_ASSERT(
                            currentVersionedItemSPtr->GetRecordKind() != RecordKind::DeletedVersion,
                            "Cannot update an item that does not exist (deleteVersion). lsn={1} txn={2} key={3}",
                            sequenceNumber, storeTransaction.Id, keyLockResourceNameHash);
                    }

                    if (!isIdempotent || (isIdempotent && ShouldValueBeAddedToDifferentialState(key, sequenceNumber)))
                    {
                        // Add the change to the store transaction write-set.
                        KSharedPtr<UpdatedVersionedItem<TValue>> updatedVersionedItemSPtr = nullptr;
                        NTSTATUS status = UpdatedVersionedItem<TValue>::Create(this->GetThisAllocator(), updatedVersionedItemSPtr);
                        Diagnostics::Validate(status);

                        updatedVersionedItemSPtr->InitializeOnApply(sequenceNumber, value);
                        updatedVersionedItemSPtr->SetValueSize(GetValueSize(data.ValueOperationData));
                        cachedDifferentialStoreComponentSPtr->Add(key, *updatedVersionedItemSPtr, *consolidationManagerSPtr_);

                        // Update count, notifications and trace

                        KSharedPtr< const StoreTransaction<TKey, TValue>> storeTransactionSPtr = &storeTransaction;
                        co_await FireItemUpdatedNotificationOnSecondaryAsync(*storeTransaction.ReplicatorTransaction, key, value, sequenceNumber);

                        //StoreEventSource::Events->StoreOnApplyUpdate(
                        //    traceComponent_->PartitionId, traceComponent_->TraceTag,
                        //    sequenceNumber,
                        //    storeTransactionSPtr->Id,
                        //    keyLockResourceNameHash);
                    }
                }
                catch (ktl::Exception& e)
                {
                    TraceException(L"OnApplyUpdate", e, storeTransaction.Id, keyLockResourceNameHash);
                    STORE_ASSERT(
                        e.GetStatus() == SF_STATUS_OBJECT_CLOSED ||
                        e.GetStatus() == SF_STATUS_TIMEOUT,
                        "Unexpected exception in OnApplyUpdate");

                    throw;
                }
            }

            ktl::Awaitable<void> OnApplyRemoveAsync(
                __in LONG64 sequenceNumber,
                __in StoreTransaction<TKey, TValue> const & storeTransaction,
                __in MetadataOperationData const & metadataOperationData,
                __in RedoUndoOperationData const & data,
                __in bool isIdempotent)
            {
                UNREFERENCED_PARAMETER(data);

                TKey key;
                try
                {
                    key = GetKeyFromBytes(*metadataOperationData.KeyBytes);
                }
                catch (ktl::Exception& e)
                {
                    StoreEventSource::Events->StoreOnApplyRemoveError(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        storeTransaction.Id,
                        L"deserialization");
                    TraceException(L"OnApplyRemove", e, storeTransaction.Id);
                    throw;
                }

                ULONG64 keyLockResourceNameHash = GetHash(*metadataOperationData.KeyBytes);

                try
                {
                    auto cachedDifferentialStoreComponentSPtr = differentialStoreComponentSPtr_.Get();
                    STORE_ASSERT(cachedDifferentialStoreComponentSPtr != nullptr, "cachedDifferentialStoreComponentSPtr != nullptr");

                    if (!isIdempotent)
                    {
                        KSharedPtr<VersionedItem<TValue>> currentVersionedItemSPtr = cachedDifferentialStoreComponentSPtr->Read(key);
                        if (currentVersionedItemSPtr == nullptr)
                        {
                            currentVersionedItemSPtr = consolidationManagerSPtr_->Read(key);
                        }

                        STORE_ASSERT(currentVersionedItemSPtr != nullptr, "Cannot remove an item that does not exist (null). lsn={1} txn={2} key={3}", sequenceNumber, storeTransaction.Id, keyLockResourceNameHash);
                        STORE_ASSERT(currentVersionedItemSPtr->GetRecordKind() != RecordKind::DeletedVersion,
                            "Cannot remove an item that already exist (deleted version). lsn={1} txn={2} key={3}",
                            sequenceNumber, storeTransaction.Id, keyLockResourceNameHash);
                    }

                    if (!isIdempotent || ShouldValueBeAddedToDifferentialState(key, sequenceNumber))
                    {
                        // Add the change to the store transaction write-set.
                        KSharedPtr<DeletedVersionedItem<TValue>> deletedVersionedItemSPtr = nullptr;
                        NTSTATUS status = DeletedVersionedItem<TValue>::Create(this->GetThisAllocator(), deletedVersionedItemSPtr);
                        Diagnostics::Validate(status);

                        deletedVersionedItemSPtr->InitializeOnApply(sequenceNumber);
                        cachedDifferentialStoreComponentSPtr->Add(key, *deletedVersionedItemSPtr, *consolidationManagerSPtr_);

                        // Update count, notifications and trace
                        auto newCount = DecrementCount(storeTransaction.Id, sequenceNumber);
                        UNREFERENCED_PARAMETER(newCount);

                        co_await FireItemRemovedNotificationOnSecondaryAsync(*storeTransaction.ReplicatorTransaction, key, sequenceNumber);

                        //StoreEventSource::Events->StoreOnApplyRemove(
                        //    traceComponent_->PartitionId, traceComponent_->TraceTag,
                        //    sequenceNumber,
                        //    storeTransaction.Id,
                        //    keyLockResourceNameHash,
                        //    newCount);
                    }
                }
                catch (ktl::Exception& e)
                {
                    TraceException(L"OnApplyRemove", e, storeTransaction.Id, keyLockResourceNameHash);
                    STORE_ASSERT(
                        e.GetStatus() == SF_STATUS_OBJECT_CLOSED ||
                        e.GetStatus() == SF_STATUS_TIMEOUT,
                        "Unexpected exception in OnApplyRemove");

                    throw;
                }
            }

            bool IsDuplicateApply(
                __in LONG64 commitSequenceNumber,
                __in MetadataTable& currentMetadataTable,
                __in TxnReplicator::ApplyContext::Enum roleType,
                __in TxnReplicator::ApplyContext::Enum operationType)
            {
                STORE_ASSERT(commitSequenceNumber != currentMetadataTable.CheckpointLSN, "commitSequenceNumber != currentMetadataTable.CheckpointLSN");

                if (commitSequenceNumber < currentMetadataTable.CheckpointLSN)
                {
                    STORE_ASSERT(roleType != TxnReplicator::ApplyContext::PRIMARY, "roleContext != ApplyContext::PRIMARY");
                    STORE_ASSERT(operationType == TxnReplicator::ApplyContext::REDO, "roleContext == ApplyContext::REDO");

                    return true;
                }

                return false;
            }

            bool CanKeyBeAdded(
                __in StoreTransaction<TKey, TValue>& storeTransaction,
                __in HashFunctionType hashFunc_,
                __in TKey& key) const
            {
                // Check to see if this key was already added as part of this store transaction.
                auto component = storeTransaction.GetComponent(hashFunc_);
                STORE_ASSERT(component != nullptr, "component != nullptr");
                auto versionedItem = component->Read(key);

                KSharedPtr<DifferentialStoreComponent<TKey, TValue>> componentSPtr = differentialStoreComponentSPtr_.Get();
                STORE_ASSERT(componentSPtr != nullptr, "componentSPtr != nullptr");

                if (versionedItem == nullptr)
                {
                    versionedItem = componentSPtr->Read(key);
                }

                if (versionedItem == nullptr)
                {
                    versionedItem = consolidationManagerSPtr_->Read(key);
                }

                if (versionedItem == nullptr || versionedItem->GetRecordKind() == RecordKind::DeletedVersion)
                {
                    return true;
                }

                return false;
            }

            bool CanKeyBeUpdatedOrDeleted(
                __in StoreTransaction<TKey, TValue>& storeTransaction,
                __in LONG64 conditionalVersion,
                __in TKey& key,
                __out LONG64 & currentVersion) const // currentVersion is only set if conditionalVersion > -1
            {
                KSharedPtr<DifferentialStoreComponent<TKey, TValue>> componentSPtr = differentialStoreComponentSPtr_.Get();

                // Check to see if this key was already added as part of this store transaction.
                auto component = storeTransaction.GetComponent(func_);
                STORE_ASSERT(component != nullptr, "component != nullptr");
                auto versionedItem = component->Read(key);

                if (versionedItem == nullptr)
                {
                    // Capture the differential state. Use it in read-only mode
                    versionedItem = componentSPtr->Read(key);
                }

                if (versionedItem == nullptr)
                {
                    versionedItem = consolidationManagerSPtr_->Read(key);
                }

                if (versionedItem != nullptr && versionedItem->GetRecordKind() != RecordKind::DeletedVersion)
                {
                    if (conditionalVersion > -1)
                    {
                        LONG64 versionSequenceNumber = versionedItem->GetVersionSequenceNumber();
                        currentVersion = versionSequenceNumber;
                        if (versionSequenceNumber == conditionalVersion)
                        {
                            return true;
                        }
                    }
                    else
                    {
                        return true;
                    }
                }

                return false;
            }

            ktl::Awaitable<void> AcquireKeyModificationLockAsync(
                __in LockManager& lockManager,
                __in StoreModificationType::Enum storeModificationType,
                __in ULONG64 keyLockResourceNameHash,
                __in StoreTransaction<TKey, TValue>& storeTransaction,
                __in Common::TimeSpan timeout)
            {
                KSharedPtr<StoreTransaction<TKey, TValue>> storeTransactionSPtr(&storeTransaction);
                LockManager::SPtr lockManagerSPtr(&lockManager);

                if (timeout < Common::TimeSpan::Zero)
                {
                    timeout = Common::TimeSpan::MaxValue;
                }

                STORE_ASSERT(
                    storeModificationType == StoreModificationType::Enum::Add ||
                    storeModificationType == StoreModificationType::Enum::Update ||
                    storeModificationType == StoreModificationType::Enum::Remove,
                    "Modification type should be add, update, or remove. Got: {1}", static_cast<int>(storeModificationType));

                // todo: If the caller assumes locking responsability, then return immediately.

                try
                {
                    co_await storeTransactionSPtr->AcquireKeyLockAsync(*lockManagerSPtr, keyLockResourceNameHash, LockMode::Enum::Exclusive, timeout);
                }
                catch (ktl::Exception& e)
                {
                    TraceException(L"AcquireKeyModificationLockAsync", e, storeTransaction.Id, keyLockResourceNameHash);
                    throw;
                }
            }

            ktl::Awaitable<void> AcquireKeyReadLockAsync(
                __in LockManager& lockManager,
                __in ULONG64 keyLockResourceNameHash,
                __in StoreTransaction<TKey, TValue>& storeTransaction,
                __in Common::TimeSpan timeout)
            {
                // tood: this implementation is incomplete.
                KSharedPtr<StoreTransaction<TKey, TValue>> storeTransactionSPtr(&storeTransaction);
                LockManager::SPtr lockManagerSPtr(&lockManager);

                LockMode::Enum lockMode = LockMode::Enum::Shared;
                if ((storeTransactionSPtr->LockingHints & LockingHints::Enum::UpdateLock) != 0)
                {
                    lockMode = LockMode::Enum::Update;
                }

                if (timeout < Common::TimeSpan::Zero)
                {
                    timeout = Common::TimeSpan::MaxValue;
                }

                // todo: If the caller assumes locking responsability, then return immediately.
                try
                {
                    co_await storeTransactionSPtr->AcquireKeyLockAsync(*lockManagerSPtr, keyLockResourceNameHash, lockMode, timeout);
                }
                catch (ktl::Exception& e)
                {
                    TraceException(L"AcquireKeyReadLockAsync", e, storeTransaction.Id, keyLockResourceNameHash);
                    STORE_ASSERT(
                        e.GetStatus() == STATUS_INVALID_PARAMETER_3 ||
                        e.GetStatus() == STATUS_INVALID_PARAMETER_4 ||
                        e.GetStatus() == SF_STATUS_OBJECT_CLOSED ||
                        e.GetStatus() == SF_STATUS_TIMEOUT,
                        "Unexpected exception in AcquireKeyReadLockAsync");

                    throw;
                }
            }

            ktl::Awaitable<void> CloseAndReopenLockManagerAsync() noexcept
            {
                co_await lockManager_->CloseAsync();

                NTSTATUS status = LockManager::Create(this->GetThisAllocator(), lockManager_);
                Diagnostics::Validate(status);

                co_await lockManager_->OpenAsync();
            }

            TKey GetKeyFromBytes(OperationData& data)
            {
                STORE_ASSERT(data.BufferCount > 0, "data.Count > 0");
                Utilities::BinaryReader reader(*data[0], this->GetThisAllocator());
                return keyConverterSPtr_->Read(reader);
            }

            TValue GetValueFromBytes(OperationData& data)
            {
                STORE_ASSERT(data.BufferCount > 0, "data.Count > 0");
                Utilities::BinaryReader reader(*data[0], this->GetThisAllocator());
                return valueConverterSPtr_->Read(reader);
            }

            ULONG32 GetValueSize(__in OperationData::SPtr valueBytes)
            {
                if (valueBytes == nullptr)
                {
                    return 0;
                }

                STORE_ASSERT(valueBytes->BufferCount > 0, "valueBytes->BufferCount {1} > 0", valueBytes->BufferCount);
                return (*valueBytes)[0]->QuerySize();
            }

            ULONG64 GetResourceNameHash(__in TKey& key)
            {
                BinaryWriter writer(this->GetThisAllocator());
                keyConverterSPtr_->Write(key, writer);
                auto resourceNameHash = CRC64::ToCRC64(*(writer.GetBuffer(0)), 0, writer.Position);
                return resourceNameHash;
            }

            ULONG64 GetHash(__in OperationData& bytes)
            {
                auto hash = CRC64::ToCRC64(*bytes[0], 0, bytes[0]->QuerySize());
                return hash;
            }

            LONG64 IncrementCount(__in LONG64 txnId, __in LONG64 commitLSN)
            {
                LONG64 newCount = InterlockedIncrement64(&count_);
                STORE_ASSERT(newCount >= 0, "New count {1} cannot be negative. TxnID: {2}, CommitLSN: {3}", newCount, txnId, commitLSN);
                return newCount;
            }

            LONG64 DecrementCount(__in LONG64 txnId, __in LONG64 commitLSN)
            {
                LONG64 newCount = InterlockedDecrement64(&count_);
                STORE_ASSERT(newCount >= 0, "New count {1} cannot be negative. TxnID: {2}, CommitLSN: {3}", newCount, txnId, commitLSN);
                return newCount;
            }

            //
            // Appends to diferential state.
            //
            void AppendDeltaDifferentialState()
            {
                KSharedPtr<DifferentialStoreComponent<TKey, TValue>> cachedComponentSPtr = differentialStoreComponentSPtr_.Get();
                KSharedPtr<DifferentialStoreComponent<TKey, TValue>> cachedDeltaComponentSPtr = deltaDifferentialStoreComponentSPtr_.Get();

                STORE_ASSERT(cachedComponentSPtr != nullptr, "cachedComponentSPtr != nullptr");
                STORE_ASSERT(cachedDeltaComponentSPtr != nullptr, "cachedDeltaComponentSPtr != nullptr");

                KSharedPtr<DifferentialKeysEnumerator<TKey, TValue>> keysEnumerator = cachedComponentSPtr->GetKeys();
                while (keysEnumerator->MoveNext())
                {
                    TKey key = keysEnumerator->Current();
                    auto versions = cachedComponentSPtr->ReadVersions(key);
                    if (versions->PreviousVersionSPtr)
                    {
                        cachedDeltaComponentSPtr->Add(key, *(versions->PreviousVersionSPtr), *consolidationManagerSPtr_);
                    }

                    STORE_ASSERT(versions->CurrentVersionSPtr, "versions->CurrentVersionSPtr");
                    cachedDeltaComponentSPtr->Add(key, *(versions->CurrentVersionSPtr), *consolidationManagerSPtr_);
                }
            }

            bool ShouldValueBeAddedToDifferentialState(__in TKey key, __in LONG64 versionSequenceNumber)
            {
                // Check if a higher version sequence number is present in the consolidated state.
                auto versionedItemSPtr = consolidationManagerSPtr_->Read(key);
                if (versionedItemSPtr != nullptr)
                {
                    if (versionedItemSPtr->GetVersionSequenceNumber() >= versionSequenceNumber)
                    {
                        return false;
                    }
                }

                return true;
            }

            ktl::Awaitable<void> CleanupAsync()
            {
                StoreEventSource::Events->StoreOnCleanupAsyncApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    L"starting");

                try
                {
                    co_await CancelBackgroundConsolidationTaskAsync();
                    // TODO: Remove this trace once #11183365 is resolved
                    StoreEventSource::Events->StoreOnCleanupAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"Finished trying to cancel background consolidation task");

                    co_await CancelSweepTaskAsync();
                    // TODO: Remove this trace once #11183365 is resolved
                    StoreEventSource::Events->StoreOnCleanupAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"Finished trying to cancel sweep task");

                    perfCounters_ = nullptr;

                    // TODO: Remove this trace once #11183365 is resolved
                    StoreEventSource::Events->StoreOnCleanupAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"Acquired prime lock");

                    if (copyManagerSPtr_ != nullptr)
                    {
                        co_await copyManagerSPtr_->CloseAsync();
                    }

                    // TODO: Remove this trace once #11183365 is resolved
                    StoreEventSource::Events->StoreOnCleanupAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"Closed copy manager");

                    if (snapshotContainerSPtr_ != nullptr)
                    {
                        co_await snapshotContainerSPtr_->CloseAsync();
                    }

                    // TODO: Remove this trace once #11183365 is resolved
                    StoreEventSource::Events->StoreOnCleanupAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"Closed snapshot container");

                    auto enumerator = filesToBeDeletedSPtr_->GetEnumerator();
                    while (enumerator->MoveNext())
                    {
                        FileMetadata::SPtr fileMetadata = enumerator->Current().Key;
                        fileMetadata->CanBeDeleted = false;
                        co_await fileMetadata->ReleaseReferenceAsync();
                    }

                    // TODO: Remove this trace once #11183365 is resolved
                    StoreEventSource::Events->StoreOnCleanupAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"Closed files");

                    if (metadataTableLockSPtr_ != nullptr && !metadataTableLockSPtr_->IsClosed)
                    {
                        metadataTableLockSPtr_->Close();
                    }

                    auto cachedCurrentMetadataTableSPtr = currentMetadataTableSPtr_.Get();
                    if (cachedCurrentMetadataTableSPtr != nullptr)
                    {
                        co_await cachedCurrentMetadataTableSPtr->CloseAsync();
                    }

                    auto cachedNextMetadataTableSPtr = nextMetadataTableSPtr_.Get();
                    if (cachedNextMetadataTableSPtr != nullptr)
                    {
                        StoreEventSource::Events->StoreOnCleanupAsyncApi(
                            traceComponent_->PartitionId, traceComponent_->TraceTag,
                            L"closing next in-progress master table");

                        co_await cachedNextMetadataTableSPtr->CloseAsync();
                    }

                    auto cachedMergeMetadataTableSPtr = mergeMetadataTableSPtr_.Get();
                    if (cachedMergeMetadataTableSPtr != nullptr)
                    {
                        co_await cachedMergeMetadataTableSPtr->CloseAsync();
                    }

                    // TODO: Remove this trace once #11183365 is resolved
                    StoreEventSource::Events->StoreOnCleanupAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"Closed metadata tables");

                    // Release files that need to be deleted.
                    currentMetadataTableSPtr_.Put(nullptr);
                    nextMetadataTableSPtr_.Put(nullptr);
                    mergeMetadataTableSPtr_.Put(nullptr);

                    consolidationManagerSPtr_ = nullptr;
                    sweepManagerSPtr_ = nullptr;

                    auto cachedDifferentialStoreComponentSPtr = differentialStoreComponentSPtr_.Get();
                    STORE_ASSERT(cachedDifferentialStoreComponentSPtr != nullptr, "cachedDifferentialStoreComponentSPtr != nullptr");
                    cachedDifferentialStoreComponentSPtr->Close();

                    auto cachedDeltaDifferentialStoreComponentSPtr = deltaDifferentialStoreComponentSPtr_.Get();
                    if (cachedDeltaDifferentialStoreComponentSPtr != nullptr)
                    {
                        cachedDeltaDifferentialStoreComponentSPtr->Close();
                    }

                    transactionalReplicatorSPtr_ = nullptr;

                    StoreEventSource::Events->StoreOnCleanupAsyncApi(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        L"completed");
                    co_return;
                }
                catch (ktl::Exception const & e)
                {
                    TraceException(L"CleanupAsync", e);
                    AssertException(L"CleanupAsync", e);
                }
            }

            ktl::Awaitable<void> ReplicateOperationAsync(
                __in StoreTransaction<TKey, TValue>& storeTransaction,
                __in MetadataOperationData const & metadata,
                __in_opt OperationData::SPtr redoData,
                __in_opt OperationData::SPtr undoData,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken)
            {
                TxnReplicator::TransactionBase::SPtr transactionBaseSPtr = storeTransaction.ReplicatorTransaction;
                TxnReplicator::Transaction::SPtr transactionSPtr(static_cast<TxnReplicator::Transaction*>(transactionBaseSPtr.RawPtr()));
                OperationData::CSPtr metadataCSPtr(&metadata);

                Common::TimeSpan replicationTimeout = timeout;

                if (timeout < Common::TimeSpan::Zero)
                {
                    replicationTimeout = Common::TimeSpan::MaxValue;
                }

                ULONG32 exponentialBackoff = Constants::StartingBackOffInMs;
                ULONG32 retryCount = 0;

                Common::Stopwatch stopwatch;
                stopwatch.Start();

                while (true)
                {
                    ThrowIfNotWritable(storeTransaction.Id);

                    try
                    {
                        NTSTATUS status = transactionSPtr->AddOperation(metadataCSPtr.RawPtr(), undoData.RawPtr(), redoData.RawPtr(), storeId_, nullptr);
                        Diagnostics::Validate(status);
                        co_return;
                    }
                    catch (ktl::Exception const & exception)
                    {
                        if (TxnReplicator::TransactionBase::IsRetriableException(exception))
                        {
                            if (exponentialBackoff * 2 < Constants::MaxBackOffInMs)
                            {
                                exponentialBackoff *= 2;
                            }
                            else
                            {
                                exponentialBackoff = Constants::MaxBackOffInMs;
                            }
                        }
                        else
                        {
                            TraceException(L"ReplicateOperationAsync", exception, storeTransaction.Id);
                            throw;
                        }
                    }

                    if (stopwatch.Elapsed > replicationTimeout)
                    {
                        throw ktl::Exception(SF_STATUS_TIMEOUT);
                    }

                    retryCount++;
                    if (retryCount == Constants::MaxRetryCount)
                    {
                        // TODO: Reconsider infinite retry.
                        //WriteWarning(TraceComponent, "Retried AddOperation {1} times. TransactionId: {2} Backoff: {3}", TraceId, MaxRetryCount, transaction.TransactionId, exponentialBackoff);
                        retryCount = 0;
                    }

                    NTSTATUS status = co_await KTimer::StartTimerAsync(
                        this->GetThisAllocator(),
                        this->GetThisAllocationTag(),
                        exponentialBackoff,
                        nullptr);
                    STORE_ASSERT(NT_SUCCESS(status), "Unsuccessfully started timer: Status {1}", status);

                    cancellationToken.ThrowIfCancellationRequested();
                }

                co_return;
            }

            ktl::Awaitable<void> RecoverConsolidatedStateAsync(__in ktl::CancellationToken const & cancellationToken)
            {
                auto cachedCurrentMetadataTableSPtr = currentMetadataTableSPtr_.Get();
                STORE_ASSERT(cachedCurrentMetadataTableSPtr != nullptr, "cachedCurrentMetadataTableSPtr != nullptr");
                STORE_ASSERT(cachedCurrentMetadataTableSPtr->Table, "cachedCurrentMetadataTableSPtr->Table != nullptr");

                KSharedPtr<RecoveryStoreComponent<TKey, TValue>>  recoveryComponentSPtr = nullptr;
                NTSTATUS status = RecoveryStoreComponent<TKey, TValue>::Create(
                    *cachedCurrentMetadataTableSPtr,
                    *workFolder_,
                    *traceComponent_,
                    *keyComparerSPtr_,
                    *keyConverterSPtr_,
                    FALSE,
                    this->GetThisAllocator(),
                    recoveryComponentSPtr);
                Diagnostics::Validate(NT_SUCCESS(status));

                STORE_ASSERT(isClosing_ == false, "Store should not be closing during recovery");

                co_await recoveryComponentSPtr->RecoverAsync(cancellationToken);
                auto cachedEstimator = keySizeEstimatorSPtr_.Get();
                auto averageKeySize = recoveryComponentSPtr->TotalKeyCount > 0 ? recoveryComponentSPtr->TotalKeySize / recoveryComponentSPtr->TotalKeyCount : 0;
                cachedEstimator->SetInitialEstimate(averageKeySize);

                fileId_ = recoveryComponentSPtr->FileId;

                logicalTimeStamp_ = recoveryComponentSPtr->LogicalCheckpointFileTimeStamp;

                STORE_ASSERT(consolidationManagerSPtr_ != nullptr, "consolidationManagerSPtr_ != nullptr");

                LONG64 oldCount = InterlockedExchange64(&count_, 0);
                STORE_ASSERT(oldCount >= 0, "Old count {1} must not be negative", oldCount);

                bool snapEnableSweep = enableSweep_;
                KSharedPtr<KSharedArray<ktl::Awaitable<TValue>>> loadTasks = _new(STORE_TAG, this->GetThisAllocator()) KSharedArray<ktl::Awaitable<TValue>>();
                Common::Stopwatch stopwatch;

                if (shouldLoadValuesInRecovery_)
                {
                    StoreEventSource::Events->StorePreloadValues(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        numberOfInflightRecoveryTasks_,
                        true,
                        0);
                    enableSweep_ = false;
                    stopwatch.Start();
                }

                KSharedPtr<RecoveryStoreEnumerator<TKey, TValue>> enumerator = recoveryComponentSPtr->GetEnumerable();
                while (enumerator->MoveNext())
                {
                    KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> row = enumerator->Current();
                    if (row.Value->GetRecordKind() == RecordKind::DeletedVersion)
                    {
                        continue;
                    }

                    OnRecoverKeyCallback(row.Key, *row.Value);

                    consolidationManagerSPtr_->Add(row.Key, *row.Value);

                    if (shouldLoadValuesInRecovery_)
                    {
                        ktl::Awaitable<TValue> task = row.Value->GetValueAsync(
                            *cachedCurrentMetadataTableSPtr,
                            *valueConverterSPtr_,
                            ReadMode::CacheResult,
                            *traceComponent_,
                            cancellationToken);
                        loadTasks->Append(Ktl::Move(task));
                        STORE_ASSERT(loadTasks->Count() <= numberOfInflightRecoveryTasks_, "Number of inflight value load tasks is limited to {1}. It cannot be {2}", numberOfInflightRecoveryTasks_, loadTasks->Count());

                        // Check if the number of inflight value load tasks is at the max limit
                        if (loadTasks->Count() == numberOfInflightRecoveryTasks_)
                        {
                            // Wait for at least one of them to complete
                            co_await TaskUtilities<TValue>::WhenAny(*loadTasks, this->GetThisAllocator());

                            // Remove all completed tasks from the list
                            for (ULONG32 i = 0; i < loadTasks->Count(); i++)
                            {
                                ktl::Awaitable<TValue>& loadTask = (*loadTasks)[i];
                                if (loadTask.IsComplete())
                                {
                                    if (loadTask.IsExceptionCompletion())
                                    {
                                        auto exceptionHandle = loadTask.GetExceptionHandle();
                                        ktl::Exception exception = ktl::Exception::ToException(exceptionHandle);


                                        throw exception;
                                    }

                                    loadTasks->Remove(i);
                                    i--;
                                }
                            }
                        }
                    }

                    // Using invalid LSN in Recovery since we do not have the LSNs
                    IncrementCount(Constants::InvalidLsn, Constants::InvalidLsn);
                }

                if (shouldLoadValuesInRecovery_)
                {
                    // TODO: Replace with WhenAll
                    for (ULONG32 i = 0; i < loadTasks->Count(); i++)
                    {
                        co_await (*loadTasks)[i];
                    }

                    stopwatch.Stop();

                    // Introducing a memory barrier since order of the instructions is significant.
                    // We do not want to enable sweep before all load operations have completed.
                    MemoryBarrier();
                    enableSweep_ = snapEnableSweep;
                    StoreEventSource::Events->StorePreloadValues(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        numberOfInflightRecoveryTasks_,
                        false,
                        stopwatch.ElapsedMilliseconds);
                }

                auto consolidatedCount = consolidationManagerSPtr_->Count();
                auto currentCount = Count;
                STORE_ASSERT(consolidatedCount == currentCount, "consolidatedComponent.Count {1} == this.Count {2}", consolidatedCount, currentCount);

                ULONG64 diskSize = co_await GetDiskSizeAsync(currentMetadataTableSPtr_);
                LONG64 memorySize = this->Size;

                perfCounters_->ItemCount.Value = currentCount;
                perfCounters_->DiskSize.Value = diskSize;
                perfCounters_->MemorySize.Value = memorySize;

                StoreEventSource::Events->StoreSize(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    currentCount,
                    diskSize,
                    memorySize);

                co_return;
            }

            ktl::Awaitable<void> RecoverCopyStateAsync(__in ktl::CancellationToken const & cancellationToken)
            {
                ApiEntry();

                STORE_ASSERT(hasPersistedState_, "RecoverCopyState can only be called for persistent stores");

                CopyManager::SPtr persistentCopyManager = dynamic_cast<CopyManager *>(copyManagerSPtr_.RawPtr());
                STORE_ASSERT(persistentCopyManager != nullptr, "Unable to cast copy manager to persistent copy manager");

                StoreEventSource::Events->StoreRecoverCopyStateAsyncApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    L"starting",
                    -1);

                MetadataTable::SPtr currentMetadataTable = currentMetadataTableSPtr_.Get();
                STORE_ASSERT(currentMetadataTable == nullptr, "Current metadata table should be null.");

                // Copy Manager's metadat table can be null when set current state is not called. 
                // To avoid dropping and creating the store, SM calls begin and end setting current state.
                if (persistentCopyManager->MetadataTableSPtr == nullptr)
                {
                    MetadataTable::SPtr metadataTable = nullptr;
                    NTSTATUS status = MetadataTable::Create(GetThisAllocator(), metadataTable);
                    Diagnostics::Validate(status);

                    currentMetadataTableSPtr_.Put(Ktl::Move(metadataTable));
                }
                else
                {
                    auto copiedMetadataTableSPtr = persistentCopyManager->MetadataTableSPtr;
                    currentMetadataTableSPtr_.Put(Ktl::Move(copiedMetadataTableSPtr));
                }

                try
                {
                    co_await RecoverConsolidatedStateAsync(cancellationToken);
                }
                catch (ktl::Exception & e)
                {
                    TraceException(L"RecoverCopyStateAsync", e);
                    throw;
                }

                co_await FireRebuildNotificationCallerHoldsLockAsync();

                co_await copyManagerSPtr_->CloseAsync();
                copyManagerSPtr_ = nullptr;

                StoreEventSource::Events->StoreRecoverCopyStateAsyncApi(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    L"completed",
                    Count);
            }

            ktl::Awaitable<KSharedPtr<IEnumerator<TKey>>> CreateKeyEnumeratorAsync(
                __in IStoreTransaction<TKey, TValue>& storeTransaction,
                __in TKey & firstKey,
                __in bool useFirstKey,
                __in TKey & lastKey,
                __in bool useLastKey)
            {
                // Key Enumerables
                KSharedPtr<IFilterableEnumerator<TKey>> differentialStateFilterableEnumeratorSPtr = nullptr;
                KSharedPtr<IEnumerator<TKey>> consolidatedStateEnumeratorSPtr = nullptr;
                KSharedPtr<IFilterableEnumerator<TKey>> snapshotStateFilterableEnumeratorSPtr = nullptr;
                KSharedPtr<IEnumerator<TKey>> rwtxStateEnumeratorSPtr = nullptr;

                KSharedPtr<IStoreTransaction<TKey, TValue>> storeTransactionSPtr = &storeTransaction;

                // Include the transaction write set if necessary
                KSharedPtr<StoreTransaction<TKey, TValue>> rwtxSPtr = static_cast<StoreTransaction<TKey, TValue> *>(storeTransactionSPtr.RawPtr());
                ThrowIfFaulted(*rwtxSPtr);
                STORE_ASSERT(rwtxSPtr != nullptr, "transaction cannot be null");

                auto snapshotFirstKey = firstKey;
                auto snapshotLastKey = lastKey;

                // Capture the snapshot state. Use it in read-only mode
                KSharedPtr<SnapshotComponent<TKey, TValue>> snapshotStateSPtr = nullptr;

                TxnReplicator::Transaction::SPtr transaction = static_cast<TxnReplicator::Transaction *>(rwtxSPtr->ReplicatorTransaction.RawPtr());
                LONG64 visibilitySequenceNumber;
                Diagnostics::Validate(co_await transaction->GetVisibilitySequenceNumberAsync(visibilitySequenceNumber));

                // If we try to read the visibility lsn before contents have been moved to snapshot, then this can be empty.
                // We could miss a key if a key got deleted, since we do not move deleted keys into consolidated state
                if (rwtxSPtr->ReadIsolationLevel == StoreTransactionReadIsolationLevel::Enum::Snapshot) // TODO: Also check for ReadCommittedSnapshot
                {
                    snapshotStateSPtr = snapshotContainerSPtr_->Read(visibilitySequenceNumber);
                }

                auto cachedDifferentialStoreComponentSPtr = differentialStoreComponentSPtr_.Get();
                STORE_ASSERT(cachedDifferentialStoreComponentSPtr != nullptr, "differential store component cannot be null");

                // Retreive enumerables.
                differentialStateFilterableEnumeratorSPtr = cachedDifferentialStoreComponentSPtr->GetEnumerableNewKeys();

                if (useFirstKey)
                {
                    differentialStateFilterableEnumeratorSPtr->MoveTo(snapshotFirstKey);
                }

                consolidatedStateEnumeratorSPtr = consolidationManagerSPtr_->GetSortedKeyEnumerable(useFirstKey, snapshotFirstKey, useLastKey, snapshotLastKey/*, keyFilter*/);

                if (snapshotStateSPtr != nullptr)
                {
                    snapshotStateFilterableEnumeratorSPtr = snapshotStateSPtr->GetEnumerable();

                    if (useFirstKey)
                    {
                        snapshotStateFilterableEnumeratorSPtr->MoveTo(snapshotFirstKey);
                    }
                }

                // Add the store transaction enumerable
                if (!rwtxSPtr->IsWriteSetEmpty)
                {
                    rwtxStateEnumeratorSPtr = rwtxSPtr->GetComponent(func_)->GetSortedKeyEnumerable(useFirstKey, snapshotFirstKey, useLastKey, snapshotLastKey);
                }

                StoreEventSource::Events->StoreCreateKeyEnumeratorAsync(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    rwtxSPtr->Id,
                    visibilitySequenceNumber);

                // Build enumerable.
                KSharedPtr<KSharedArray<KSharedPtr<IEnumerator<TKey>>>> enumerablesSPtr = nullptr;
                enumerablesSPtr = _new(STORE_TAG, this->GetThisAllocator()) KSharedArray<KSharedPtr<IEnumerator<TKey>>>();
                STORE_ASSERT(enumerablesSPtr != nullptr, "enumerablesSPtr != nullptr");

                KSharedPtr<IEnumerator<TKey>> differentialStateEnumeratorSPtr = static_cast<IEnumerator<TKey> *>(differentialStateFilterableEnumeratorSPtr.RawPtr());
                enumerablesSPtr->Append(differentialStateEnumeratorSPtr);

                enumerablesSPtr->Append(consolidatedStateEnumeratorSPtr);

                if (rwtxStateEnumeratorSPtr != nullptr)
                {
                    enumerablesSPtr->Append(rwtxStateEnumeratorSPtr);
                }

                if (snapshotStateFilterableEnumeratorSPtr != nullptr)
                {
                    KSharedPtr<IEnumerator<TKey>> snapshotStateEnumeratorSPtr = static_cast<IEnumerator<TKey> *>(snapshotStateFilterableEnumeratorSPtr.RawPtr());
                    enumerablesSPtr->Append(snapshotStateEnumeratorSPtr);
                }

                // Merge the key sources, in order, while enumerating
                KSharedPtr<IEnumerator<TKey>> orderedKeyEnumerableSPtr = nullptr;
                NTSTATUS status = SortedSequenceMergeEnumerator<TKey>::Create(
                    *enumerablesSPtr,
                    *keyComparerSPtr_,
                    useFirstKey,
                    snapshotFirstKey,
                    useLastKey,
                    snapshotLastKey,
                    this->GetThisAllocator(),
                    orderedKeyEnumerableSPtr);
                Diagnostics::Validate(status);

                // De-dupe and skip deleted keys while enumerating
                KSharedPtr<IEnumerator<TKey>> keyEnumeratorSPtr = nullptr;
                status = StoreKeysEnumerator<TKey, TValue>::Create(
                    *keyComparerSPtr_,
                    *rwtxSPtr,
                    func_,
                    *cachedDifferentialStoreComponentSPtr,
                    *consolidationManagerSPtr_,
                    visibilitySequenceNumber,
                    snapshotStateSPtr,
                    *orderedKeyEnumerableSPtr,
                    this->GetThisAllocator(),
                    keyEnumeratorSPtr);
                Diagnostics::Validate(status);

                co_return keyEnumeratorSPtr;
            }

            ktl::Awaitable<KSharedPtr<IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>>> CreateKeyValueEnumeratorAsync(
                __in IStoreTransaction<TKey, TValue>& storeTransaction,
                __in TKey & firstKey,
                __in bool useFirstKey,
                __in TKey & lastKey,
                __in bool useLastKey,
                __in ReadMode readMode)
            {
                KSharedPtr<IStoreTransaction<TKey, TValue>> storeTransactionSPtr = &storeTransaction;
                TKey snapFirstKey = firstKey;
                TKey snapLastKey = lastKey;

                KSharedPtr<IEnumerator<TKey>> keyEnumerator = co_await CreateKeyEnumeratorAsync(*storeTransactionSPtr, snapFirstKey, useFirstKey, snapLastKey, useLastKey);

                // Get values for each key asynchronously, while enumerating
                KSharedPtr<IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>> enumeratorSPtr = nullptr;
                NTSTATUS status = StoreKeyValueEnumerator<TKey, TValue>::Create(
                    *this,
                    *keyComparerSPtr_,
                    *keyEnumerator,
                    *storeTransactionSPtr,
                    readMode,
                    this->GetThisAllocator(),
                    enumeratorSPtr);
                Diagnostics::Validate(status);
                co_return enumeratorSPtr;
            }

            ktl::Awaitable<ULONG64> GetDiskSizeAsync(__in ThreadSafeSPtrCache<MetadataTable> const& metadataTableSPtr) const
            {
                ULONG64 result = 0;
                auto cachedMetadataTableSPtr = metadataTableSPtr.Get();

                if (cachedMetadataTableSPtr != nullptr)
                {
                    result += cachedMetadataTableSPtr->MetadataFileSize;
                    auto tableEnumeratorSPtr = cachedMetadataTableSPtr->Table->GetEnumerator();
                    while (tableEnumeratorSPtr->MoveNext())
                    {
                        auto fileMetadataSPtr = tableEnumeratorSPtr->Current().Value;
                        result += co_await fileMetadataSPtr->GetFileSizeAsync();
                    }
                }

                co_return result;
            }

#pragma region Notifications
            ktl::Awaitable<void> FireSingleItemNotificationOnPrimaryAsync(
                __in TxnReplicator::TransactionBase const& replicatorTransaction,
                __in TKey key,
                __in const MetadataOperationData & metadataOperationData,
                __in LONG64 sequenceNumber)
            {
                KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> cachedEventHandler = dictionaryChangeHandlerSPtr_.Get();
                if (cachedEventHandler == nullptr)
                {
                    co_return;
                }

                try
                {
                    if (metadataOperationData.ModificationType == StoreModificationType::Remove)
                    {
                        if (dictionaryChangeHandlerMask_ & DictionaryChangeEventMask::Remove)
                        {
                            co_await cachedEventHandler->OnRemovedAsync(replicatorTransaction, key, sequenceNumber, /* isPrimary = */ true);
                        }

                        co_return;
                    }

                    KSharedPtr<MetadataOperationData> metadataOperationDataSPtr = const_cast<MetadataOperationData *>(&metadataOperationData);
                    KSharedPtr<MetadataOperationDataKV<TKey, TValue>> cachedKeyValueMetadataOperationDataSPtr =
                        static_cast<MetadataOperationDataKV<TKey, TValue> *>(metadataOperationDataSPtr.RawPtr());
                    STORE_ASSERT(cachedKeyValueMetadataOperationDataSPtr != nullptr, "metadataOperationData should be MetadataOperationDataKV. lsn={1}", sequenceNumber);

                    // Retrieve value
                    if (metadataOperationDataSPtr->ModificationType == StoreModificationType::Add)
                    {
                        if (dictionaryChangeHandlerMask_ & DictionaryChangeEventMask::Add)
                        {
                            co_await cachedEventHandler->OnAddedAsync(replicatorTransaction, key, cachedKeyValueMetadataOperationDataSPtr->Value, sequenceNumber, /* isPrimary = */ true);
                        }
                    }
                    else if (metadataOperationDataSPtr->ModificationType == StoreModificationType::Update)
                    {
                        if (dictionaryChangeHandlerMask_ & DictionaryChangeEventMask::Update)
                        {
                            co_await cachedEventHandler->OnUpdatedAsync(replicatorTransaction, key, cachedKeyValueMetadataOperationDataSPtr->Value, sequenceNumber, /* isPrimary = */ true);
                        }
                    }
                }
                catch (const ktl::Exception & ex)
                {
                    TraceException(L"FireSingleItemNotificationOnPrimary", ex, replicatorTransaction.TransactionId);
                    throw;
                }
            }

            ktl::Awaitable<void> FireItemAddedNotificationOnSecondaryAsync(
                __in TxnReplicator::TransactionBase& replicatorTransaction,
                __in TKey key,
                __in TValue value,
                __in LONG64 sequenceNumber)
            {
                KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> cachedEventHandler = dictionaryChangeHandlerSPtr_.Get();
                if (cachedEventHandler == nullptr || !(dictionaryChangeHandlerMask_ & DictionaryChangeEventMask::Add))
                {
                    co_return;
                }

                try
                {
                    co_await cachedEventHandler->OnAddedAsync(replicatorTransaction, key, value, sequenceNumber, /* isPrimary = */ false);
                }
                catch (const ktl::Exception & ex)
                {
                    TraceException(L"FireItemAddedNotificationOnSecondary", ex, replicatorTransaction.TransactionId);
                    throw;
                }
            }

            ktl::Awaitable<void> FireItemUpdatedNotificationOnSecondaryAsync(
                __in TxnReplicator::TransactionBase& replicatorTransaction,
                __in TKey key,
                __in TValue value,
                __in LONG64 sequenceNumber)
            {
                KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> cachedEventHandler = dictionaryChangeHandlerSPtr_.Get();
                if (cachedEventHandler == nullptr || !(dictionaryChangeHandlerMask_ & DictionaryChangeEventMask::Update))
                {
                    co_return;
                }

                try
                {
                    co_await cachedEventHandler->OnUpdatedAsync(replicatorTransaction, key, value, sequenceNumber, /* isPrimary = */ false);
                }
                catch (const ktl::Exception & ex)
                {
                    TraceException(L"FireUpdatedNotificationOnSecondary", ex, replicatorTransaction.TransactionId);
                    throw;
                }
            }

            ktl::Awaitable<void> FireItemRemovedNotificationOnSecondaryAsync(
                __in TxnReplicator::TransactionBase& replicatorTransaction,
                __in TKey key,
                __in LONG64 sequenceNumber)
            {
                KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> cachedEventHandler = dictionaryChangeHandlerSPtr_.Get();
                if (cachedEventHandler == nullptr || !(dictionaryChangeHandlerMask_ & DictionaryChangeEventMask::Remove))
                {
                    co_return;
                }

                try
                {
                    co_await cachedEventHandler->OnRemovedAsync(replicatorTransaction, key, sequenceNumber, /* isPrimary = */ false);
                }
                catch (const ktl::Exception & ex)
                {
                    TraceException(L"FireRemovedNotificationOnSecondary", ex, replicatorTransaction.TransactionId);
                    throw;
                }
            }

            ktl::Awaitable<void> FireRebuildNotificationCallerHoldsLockAsync()
            {
                KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> cachedEventHandler = dictionaryChangeHandlerSPtr_.Get();
                if (cachedEventHandler == nullptr || !(dictionaryChangeHandlerMask_ & DictionaryChangeEventMask::Rebuild))
                {
                    co_return;
                }

                Common::Stopwatch stopwatch;
                stopwatch.Start();
                StoreEventSource::Events->StoreRebuildNotificationStarting(traceComponent_->PartitionId, traceComponent_->TraceTag);

                try
                {
                    auto stateSPtr = CreateRebuiltStateEnumerator();
                    KSharedPtr<IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>> enumeratorSPtr =
                        static_cast<IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>> *>(stateSPtr.RawPtr());
                    STORE_ASSERT(enumeratorSPtr != nullptr, "rebuilt state enumerator should not be null");
                    co_await cachedEventHandler->OnRebuiltAsync(*enumeratorSPtr);

                    // Ensure that the enumerable cannot be used anymore
                    stateSPtr->InvalidateEnumerator();
                }
                catch (const ktl::Exception & ex)
                {
                    TraceException(L"FireRebuildNotificationCallerHoldsLockAsync", ex);
                    throw;
                }

                StoreEventSource::Events->StoreRebuildNotificationCompleted(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    stopwatch.ElapsedMilliseconds);

                co_return;
            }

            KSharedPtr<RebuiltStateAsyncEnumerator<TKey, TValue>> CreateRebuiltStateEnumerator()
            {
                TKey defaultKey;
                auto consolidatedStateKeys = consolidationManagerSPtr_->GetSortedKeyEnumerable(false, defaultKey, false, defaultKey);
                STORE_ASSERT(consolidatedStateKeys != nullptr, "consolidated state keys enumerator should not be null");
                auto cachedDiffState = differentialStoreComponentSPtr_.Get();
                STORE_ASSERT(cachedDiffState != nullptr, "differential state should not be null");
                auto cachedCurrentMetadataTable = currentMetadataTableSPtr_.Get();
                STORE_ASSERT(cachedCurrentMetadataTable != nullptr, "current metadata table should not be null");

                KSharedPtr<RebuiltStateAsyncEnumerator<TKey, TValue>> enumeratorSPtr = nullptr;
                auto status = RebuiltStateAsyncEnumerator<TKey, TValue>::Create(
                    false,
                    *consolidatedStateKeys,
                    *cachedDiffState,
                    *consolidationManagerSPtr_,
                    *cachedCurrentMetadataTable,
                    *valueConverterSPtr_,
                    *traceComponent_,
                    GetThisAllocator(),
                    enumeratorSPtr);
                Diagnostics::Validate(status);

                return enumeratorSPtr;
            }

            // Invariant: For a given key at max one committed transaction can be undone. 
            // Previous state is last committed transaction on the key before the one being undone.
            // Previous State can be from differential or consolidated.
            // 
            // Previous State  |   This Txn Last Operation |   Expected Result
            // NA|Remove           Add                         Remove
            // NA|Remove           Update                      Remove
            // NA|Remove           Remove                      Noop
            // 
            // A|U                 Add                         Update
            // A|U                 Update                      Update
            // A|U                 Remove                      Add
            ktl::Awaitable<void> FireUndoNotificationsAsync(
                __in TxnReplicator::TransactionBase& replicatorTransaction,
                __in TKey key,
                __in const MetadataOperationData & metadataOperationData,
                __in KSharedPtr<StoreComponentReadResult<TValue>> & readResult,
                __in LONG64 sequenceNumber)
            {

                KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> cachedEventHandler = dictionaryChangeHandlerSPtr_.Get();
                if (cachedEventHandler == nullptr)
                {
                    co_return;
                }

                auto versionedItem = readResult->VersionedItem;

                try
                {
                    // Last operation on the key in txn is Add or Update
                    if (metadataOperationData.ModificationType == StoreModificationType::Add || metadataOperationData.ModificationType == StoreModificationType::Update)
                    {
                        if (versionedItem == nullptr || versionedItem->GetRecordKind() == RecordKind::DeletedVersion)
                        {
                            if (dictionaryChangeHandlerMask_ & DictionaryChangeEventMask::Remove)
                            {
                                co_await cachedEventHandler->OnRemovedAsync(replicatorTransaction, key, sequenceNumber, /* isPrimary = */ false);
                            }

                            co_return;
                        }

                        if (dictionaryChangeHandlerMask_ & DictionaryChangeEventMask::Update)
                        {
                            co_await cachedEventHandler->OnUpdatedAsync(
                                replicatorTransaction,
                                key,
                                readResult->Value,
                                sequenceNumber,
                                /* isPrimary = */ false);
                        }

                        co_return;
                    }

                    // Last operation on the key in txn is Remove
                    if (metadataOperationData.ModificationType == StoreModificationType::Remove)
                    {
                        if (versionedItem == nullptr || versionedItem->GetRecordKind() == RecordKind::DeletedVersion)
                        {
                            // If item previously did not exist, since the last notification for the given key is remove, this must be nooped
                            co_return;
                        }

                        if (dictionaryChangeHandlerMask_ & DictionaryChangeEventMask::Add)
                        {
                            co_await cachedEventHandler->OnAddedAsync(
                                replicatorTransaction,
                                key,
                                readResult->Value,
                                sequenceNumber,
                                /* isPrimary = */ false);
                        }

                        co_return;
                    }

                }
                catch (const ktl::Exception & ex)
                {
                    TraceException(L"FireUndoNotification", ex, replicatorTransaction.TransactionId);
                    throw;
                }
            }

#pragma endregion

            KString::CSPtr GetCheckpointFilePath(__in KStringView const & fileName)
            {
                BOOLEAN concatenatedPath;

                KString::SPtr filePath = nullptr;
                NTSTATUS status = KString::Create(filePath, this->GetThisAllocator(), *workFolder_);
                Diagnostics::Validate(status);

                concatenatedPath = filePath->Concat(Common::Path::GetPathSeparatorWstr().c_str());
                STORE_ASSERT(concatenatedPath == TRUE, "concatentaing file seperators failed");

                concatenatedPath = filePath->Concat(fileName);
                STORE_ASSERT(concatenatedPath == TRUE, "concatentaing file path and name failed");

                return filePath.RawPtr();
            }

            void TraceException(
                __in KStringView const & methodName,
                __in ktl::Exception const & exception,
                __in LONG64 txnId=Constants::InvalidTxnId,
                __in ULONG64 keyHash=Constants::InvalidKeyHash)
            {
                KDynStringA stackString(this->GetThisAllocator());
                Diagnostics::GetExceptionStackTrace(exception, stackString);
                StoreEventSource::Events->StoreException(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    Data::Utilities::ToStringLiteral(methodName),
                    txnId,
                    keyHash,
                    exception.GetStatus(),
                    Data::Utilities::ToStringLiteral(stackString));
            }

            void AssertException(__in KStringView const & methodName, __in ktl::Exception const & exception)
            {
                KDynStringA stackString(this->GetThisAllocator());
                Diagnostics::GetExceptionStackTrace(exception, stackString);
                STORE_ASSERT(
                    false,
                    "UnexpectedException: Message: {2} Code:{4}\nStack: {3}",
                    ToStringLiteral(methodName),
                    ToStringLiteral(stackString),
                    exception.GetStatus());
            }

            LONG64 GetCheckpointLSN()
            {
                STORE_ASSERT(lastPrepareCheckpointLSN_ > -1, "PrepareCheckpoint must have been called");
                MetadataTable::SPtr currentTable = currentMetadataTableSPtr_.Get();
                STORE_ASSERT(currentTable != nullptr, "CurrentMetadataTable cannot be null");

                if (currentTable->CheckpointLSN > lastPrepareCheckpointLSN_)
                {
                    return currentTable->CheckpointLSN;
                }

                return lastPrepareCheckpointLSN_;
            }

            void ThrowIfFaulted(__in StoreTransaction<TKey, TValue> const & rwtx)
            {
                UNREFERENCED_PARAMETER(rwtx);
                ThrowIfFaulted();
                // TODO: rwtx->ReplicatorTransaction->ThrowIfTransactionManagerIsNull();
            }

            void ThrowIfFaulted()
            {
                if (isClosing_)
                {
                    throw ktl::Exception(SF_STATUS_OBJECT_CLOSED);
                }
            }

            void ThrowIfNotWritable(LONG64 transactionId)
            {
                STORE_ASSERT(transactionalReplicatorSPtr_->IsAlive(), "transactional replicator has expired");
                KSharedPtr<TxnReplicator::ITransactionalReplicator> replicatorSPtr = transactionalReplicatorSPtr_->TryGetTarget();
                Data::Utilities::IStatefulPartition::SPtr partition = replicatorSPtr->StatefulPartition;

                FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus;
                auto status = partition->GetWriteStatus(&writeStatus);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                STORE_ASSERT(writeStatus != FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID, "invalid partition status");

                if (writeStatus == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY)
                {
                    StoreEventSource::Events->StoreThrowIfNotWritable(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        transactionId,
                        static_cast<ULONG32>(writeStatus),
                        static_cast<ULONG32>(replicatorSPtr->Role));
                    throw ktl::Exception(SF_STATUS_NOT_PRIMARY);
                }
            }

            void ThrowIfNotReadable(__in StoreTransaction<TKey, TValue> const & rtx)
            {
                STORE_ASSERT(transactionalReplicatorSPtr_->IsAlive(), "transactional replicator has expired");
                KSharedPtr<TxnReplicator::ITransactionalReplicator> replicatorSPtr = transactionalReplicatorSPtr_->TryGetTarget();
                Data::Utilities::IStatefulPartition::SPtr partition = replicatorSPtr->StatefulPartition;

                FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus;
                auto status = partition->GetReadStatus(&readStatus);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                STORE_ASSERT(readStatus != FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID, "invalid partition status");

                auto role = replicatorSPtr->Role;
                bool grantReadStatus = readStatus == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED;

                // If the replica is readable, no need to throw
                // Note that this takes dependency on RA not making a replica readable
                // during Unknown and Idle.
                if (grantReadStatus)
                {
                    return;
                }

                // If Primary, ReadStatus must be adhered to
                // Not doing so will cause the replica to be readable during dataloss
                if (role == FABRIC_REPLICA_ROLE_PRIMARY)
                {
                    StoreEventSource::Events->StoreThrowIfNotReadable(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        rtx.Id,
                        static_cast<ULONG32>(readStatus),
                        static_cast<ULONG32>(role));
                    throw ktl::Exception(SF_STATUS_NOT_READABLE);
                }

                // If role is ActiveSecondary, we need to ignore ReadStatus since it is never granted on Active Secondary.
                // Instead we will use IsReadable which indicates that logging replicator has ensured the state is locally consistent
                bool isConsistent = replicatorSPtr->IsReadable;
                if (role == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
                {
                    if (!isAlwaysReadable_)
                    {
                        grantReadStatus = false;
                    }
                    else
                    {
                        grantReadStatus = isConsistent;
                    }

                    if (grantReadStatus)
                    {
                        // TODO: Check for ReadCommittedSnapshot level and no locking hints
                        grantReadStatus = (rtx.ReadIsolationLevel == StoreTransactionReadIsolationLevel::Enum::Snapshot);
                    }
                }

                if (!grantReadStatus)
                {
                    StoreEventSource::Events->StoreThrowIfNotReadable(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        rtx.Id,
                        static_cast<ULONG32>(readStatus),
                        static_cast<ULONG32>(role));
                    throw ktl::Exception(SF_STATUS_NOT_READABLE);
                }
            }

            static BOOLEAN StringEquals(const KString::SPtr & keyOne, const KString::SPtr & keyTwo)
            {
                return *keyOne == *keyTwo ? TRUE : FALSE;
            }

        protected:
            FAILABLE Store(
                __in PartitionedReplicaId const & traceId,
                __in IComparer<TKey>& keyComparer,
                __in HashFunctionType func,
                __in KUriView& name,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in Data::StateManager::IStateSerializer<TKey>& keyStateSerializer,
                __in Data::StateManager::IStateSerializer<TValue>& valueStateSerializer);

            HashFunctionType GetFunc()
            {
                return func_;
            }

            virtual void OnRecoverKeyCallback(__in TKey key, __in VersionedItem<TValue> &value)
            {
                // Used by ReliableConcurrentQueue (derived class of Store)
                UNREFERENCED_PARAMETER(key);
                UNREFERENCED_PARAMETER(value);
            }

        private:
            // Constants
            KStringView const CurrentDiskMetadataFileName = L"current_metadata.sfm";
            KStringView const TempDiskMetadataFileName = L"temp_metadata.sfm";
            KStringView const BkpDiskMetadataFileName = L"backup_metadata.sfm";

            StoreTraceComponent::SPtr traceComponent_;
            StorePerformanceCountersSPtr perfCounters_;

            bool hasPersistedState_;

            FABRIC_STATE_PROVIDER_ID storeId_;
            LONG64 count_;
            KSharedPtr<IComparer<TKey>> keyComparerSPtr_ = nullptr;
            HashFunctionType func_;
            LockManager::SPtr lockManager_ = nullptr;
            ThreadSafeSPtrCache<DifferentialStoreComponent<TKey, TValue>> differentialStoreComponentSPtr_;
            ThreadSafeSPtrCache<DifferentialStoreComponent<TKey, TValue>> deltaDifferentialStoreComponentSPtr_;
            MergeHelper::SPtr mergeHelperSPtr_ = nullptr;
            KSharedPtr<ConsolidationManager<TKey, TValue>> consolidationManagerSPtr_ = nullptr;
            KSharedPtr<SweepManager<TKey, TValue>> sweepManagerSPtr_ = nullptr;
            ThreadSafeSPtrCache<KeySizeEstimator> keySizeEstimatorSPtr_ = { nullptr };
            ThreadSafeSPtrCache<ktl::AwaitableCompletionSource<PostMergeMetadataTableInformation::SPtr>> consolidationTcsSPtr_ = { nullptr };
            ktl::CancellationTokenSource::SPtr consolidationTaskCancellationSourceSPtr_ = nullptr;
            bool enableBackgroundConsolidation_;
            ktl::AwaitableCompletionSource<bool>::SPtr testDelayOnConsolidationSPtr_ = nullptr;
            KSharedPtr<SnapshotContainer<TKey, TValue>> snapshotContainerSPtr_ = nullptr;
            ICopyManager::SPtr copyManagerSPtr_;
            KUri::SPtr name_;
            KSharedPtr<Data::StateManager::IStateSerializer<TKey>> keyConverterSPtr_ = nullptr;
            KSharedPtr<Data::StateManager::IStateSerializer<TValue>> valueConverterSPtr_ = nullptr;
            KSharedPtr<ConcurrentDictionary2<LONG64, KSharedPtr<StoreTransaction<TKey, TValue>>>> inflightReadWriteStoreTransactionsSPtr_ = nullptr;
            bool isClosing_;
            LONG64 lastPrepareCheckpointLSN_;
            LONG64 lastPerformCheckpointLSN_;
            ThreadSafeSPtrCache<MetadataTable> currentMetadataTableSPtr_ = { nullptr };
            ReaderWriterAsyncLock::SPtr metadataTableLockSPtr_ = nullptr;
            ThreadSafeSPtrCache<MetadataTable> nextMetadataTableSPtr_ = { nullptr };
            ThreadSafeSPtrCache<MetadataTable> mergeMetadataTableSPtr_ = { nullptr };
            ConcurrentDictionary<FileMetadata::SPtr, bool>::SPtr filesToBeDeletedSPtr_ = nullptr;
            KSharedPtr<KWeakRef<TxnReplicator::ITransactionalReplicator>> transactionalReplicatorSPtr_ = nullptr;
            ThreadSafeSPtrCache<IDictionaryChangeHandler<TKey, TValue>> dictionaryChangeHandlerSPtr_ = { nullptr };
            DictionaryChangeEventMask::Enum dictionaryChangeHandlerMask_;
            ULONG32 fileId_;
            KSpinLock fileIdLock_;
            ULONG64 logicalTimeStamp_;
            KSpinLock timeStampLock_;
            KString::CSPtr workFolder_;
            KString::CSPtr tmpMetadataFilePath_;
            KString::CSPtr currentMetadataFilePath_;
            KString::CSPtr bkpMetadataFilePath_;
            bool isAlwaysReadable_;
            bool enableSweep_;
            ThreadSafeSPtrCache<ktl::AwaitableCompletionSource<bool>> sweepTcsSPtr_ = {nullptr};
            ktl::CancellationTokenSource::SPtr sweepTaskCancellationSourceSPtr_ = nullptr;
            LONG64 sweepInProgress_;
            bool enableEnumerationWithRepeatableRead_;
            bool shouldLoadValuesInRecovery_;
            ULONG32 numberOfInflightRecoveryTasks_;
            bool wasCopyAborted_;
            KString::SPtr langTypeInfo_;
            KString::SPtr lang_;
            LONG64 keySize_ = -1;
        };

        template <typename TKey, typename TValue>
        Store<TKey, TValue>::Store(
            __in PartitionedReplicaId const & traceId,
            __in IComparer<TKey>& keyComparer,
            __in HashFunctionType func,
            __in KUriView& name,
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in Data::StateManager::IStateSerializer<TKey>& keyStateSerializer,
            __in Data::StateManager::IStateSerializer<TValue>& valueStateSerializer)
            :keyComparerSPtr_(&keyComparer),
            func_(func),
            name_(),
            storeId_(stateProviderId),
            keyConverterSPtr_(&keyStateSerializer),
            valueConverterSPtr_(&valueStateSerializer),
            PartitionedReplicaTraceComponent(traceId),
            deltaDifferentialStoreComponentSPtr_(nullptr),
            differentialStoreComponentSPtr_(nullptr),
            transactionalReplicatorSPtr_(nullptr),
            perfCounters_(nullptr),
            lastPerformCheckpointLSN_(0),
            isClosing_(false),
            count_(0),
            fileId_(0),
            logicalTimeStamp_(0),
            enableBackgroundConsolidation_(true),
            isAlwaysReadable_(true), // TODO: should be configured on creation
            enableSweep_(false), // Factory will enable sweep
            sweepInProgress_(0),
            enableEnumerationWithRepeatableRead_(false),
            shouldLoadValuesInRecovery_(false),
            numberOfInflightRecoveryTasks_(1),
            wasCopyAborted_(false),
            dictionaryChangeHandlerMask_(DictionaryChangeEventMask::Enum::All),
            hasPersistedState_(true)
        {
            NTSTATUS status = StoreTraceComponent::Create(PartitionId, ReplicaId, storeId_, this->GetThisAllocator(), traceComponent_);
            Diagnostics::Validate(status);

            StoreEventSource::Events->StoreConstructor(
                traceComponent_->PartitionId,
                traceComponent_->TraceTag,
                ToStringLiteral(name.Get(KUriView::eRaw)));

            status = KUri::Create(name, GetThisAllocator(), name_);
            Diagnostics::Validate(status);
        }

        template <typename TKey, typename TValue>
        Store<TKey, TValue>::~Store()
        {
            StoreEventSource::Events->StoreDestructor(traceComponent_->PartitionId, traceComponent_->TraceTag);
        }
    }
}
