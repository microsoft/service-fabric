// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FailoverManager;
    }
}

namespace ReliabilityUnitTest
{
    class FailoverManagerStoreTest;
}

namespace Store
{
    class ReplicatedStoreTest;
    class ComFabricStore_ReplicatedStoreRoot;
    class FabricTimeController;
    class FileStreamFullCopyManager;
    class ProgressVectorEntry;

    class ReplicatedStore
        : public IReplicatedStore
        , public Api::IStateProvider
        , public Common::RootedObject
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
    private:
        class NotificationManager;
        class OpenRepairAsyncOperation;

    protected:

        ReplicatedStore(
            Common::Guid const & partitionId,
            ::FABRIC_REPLICA_ID replicaId,
            Reliability::ReplicationComponent::ReplicatorSettingsUPtr &&,
            ReplicatedStoreSettings const & settings,
            Api::IStoreEventHandlerPtr const &,
            Api::ISecondaryEventHandlerPtr const &,
            Common::ComponentRoot const &);

        ReplicatedStore(
            Common::Guid const & partitionId,
            ::FABRIC_REPLICA_ID replicaId,
            Common::ComPointer<IFabricReplicator> &&,
            ReplicatedStoreSettings const & settings,
            Api::IStoreEventHandlerPtr const &,
            Api::ISecondaryEventHandlerPtr const &,
            Common::ComponentRoot const &);

    public:

        virtual ~ReplicatedStore();

        __declspec(property(get=get_ReplicatedStoreSettings)) Store::ReplicatedStoreSettings const & Settings;

        __declspec(property(get=get_LocalStoreInstanceName)) std::wstring const & LocalStoreInstanceName;
        __declspec(property(get=get_LocalStore)) ILocalStoreUPtr const & LocalStore;
        __declspec(property(get=get_DropLocalStoreLock)) Common::RwLock & DropLocalStoreLock;
        __declspec(property(get=get_IsLocalStoreClosed)) bool IsLocalStoreClosed;
        __declspec(property(get = get_IsLocalStoreLogTruncationRequired)) bool IsLocalStoreLogTruncationRequired;

        __declspec(property(get=get_InnerReplicator)) Common::ComPointer<IFabricStateReplicator> const & InnerReplicator;
        __declspec(property(get=get_InnerInternalReplicator)) Common::ComPointer<IFabricInternalStateReplicator> const & InnerInternalReplicator;
        __declspec(property(get=get_DataSerializer)) std::unique_ptr<Store::DataSerializer> const & DataSerializer;
        __declspec(property(get=get_NotificationManager)) std::unique_ptr<NotificationManager> const & Notifications;
        __declspec(property(get=get_FileStreamFullCopyManager)) std::shared_ptr<FileStreamFullCopyManager> const & FileStreamFullCopy;
        __declspec(property(get=get_EnableStreamFaultsAndEOSOperationAck)) bool EnableStreamFaultsAndEOSOperationAck;
        __declspec(property(get=get_PerfCounters)) ReplicatedStorePerformanceCounters const & PerfCounters;

        __declspec(property(get=get_IsActive)) bool IsActive;
        __declspec(property(get=get_IsActivePrimary)) bool IsActivePrimary;
        __declspec(property(get=get_ReplicaRole)) ::FABRIC_REPLICA_ROLE ReplicaRole;
        __declspec(property(get=get_TargetCopyOperationSize)) size_t TargetCopyOperationSize;

        __declspec(property(get=get_CommitBatchingPeriod)) int const CommitBatchingPeriod;
        __declspec(property(get=get_IsThrottleEnabled)) bool const IsThrottleEnabled;

        Store::ReplicatedStoreSettings const & get_ReplicatedStoreSettings() const { return settings_; }

        std::wstring const & get_LocalStoreInstanceName() const { return localStoreInstanceName_; }
        virtual ILocalStoreUPtr const & get_LocalStore() const = 0;
        Common::RwLock & get_DropLocalStoreLock() { return dropLocalStoreLock_; }
        bool get_IsLocalStoreClosed() const;
        bool get_IsLocalStoreLogTruncationRequired();

        Common::ComPointer<IFabricStateReplicator> const & get_InnerReplicator() const { return stateReplicatorCPtr_; }
        Common::ComPointer<IFabricInternalStateReplicator> const & get_InnerInternalReplicator() const { return internalStateReplicatorCPtr_; }
        std::unique_ptr<Store::DataSerializer> const & get_DataSerializer() const { return dataSerializerUPtr_; }
        std::unique_ptr<NotificationManager> const & get_NotificationManager() const { return notificationManagerUPtr_; }
        std::shared_ptr<FileStreamFullCopyManager> const & get_FileStreamFullCopyManager() const { return fileStreamFullCopyManagerSPtr_; }
        bool get_EnableStreamFaultsAndEOSOperationAck() const { return settings_.EnableStreamFaults; }
        ReplicatedStorePerformanceCounters const & get_PerfCounters() const { return *perfCounters_; }

        bool get_IsActive() const { return isActive_; }
        bool get_IsActivePrimary() const { return (isActive_ && replicaRole_ == ::FABRIC_REPLICA_ROLE_PRIMARY); }
        ::FABRIC_REPLICA_ROLE get_ReplicaRole() const { return replicaRole_; }
        size_t get_TargetCopyOperationSize() const { return targetCopyOperationSize_; }

        int const get_CommitBatchingPeriod() const { return settings_.CommitBatchingPeriod; }
        bool get_IsThrottleEnabled() const
        {
            return settings_.ThrottleReplicationQueueOperationCount > 0 || settings_.ThrottleReplicationQueueSizeBytes > 0;
        }

        __declspec(property(get=get_CopyStatistics)) Store::CopyStatistics & CopyStatistics;
        __declspec(property(get=get_FatalError)) Common::ErrorCode const & FatalError;

        Store::CopyStatistics & get_CopyStatistics() { return copyStatistics_; }
        Common::ErrorCode const & get_FatalError() { return fatalError_; }

        bool GetIsActivePrimary() const override { return this->IsActivePrimary; }

        void TryFaultStreamAndStopSecondaryPump(Common::ErrorCode const & fatalError, __in FABRIC_FAULT_TYPE faultType);
        void ReportTransientFault(Common::ErrorCode const & fatalError);
        void ReportPermanentFault(Common::ErrorCode const & fatalError);
        void ReportFault(Common::ErrorCode const & fatalError, FABRIC_FAULT_TYPE faultType);

        bool TryAcquireActiveBackupState(
            __in StoreBackupRequester::Enum backupRequester,
            __out ScopedActiveBackupStateSPtr & scopedBackupState);

        Common::ErrorCode BackupLocal(std::wstring const & dir);
        Common::ErrorCode RestoreLocal(std::wstring const & dir, RestoreSettings const & = RestoreSettings());

        /// <summary>
        /// Creates a full backup of the replicated store. Note that this method is not projected through the interface IReplicatedStore.
        /// This method is intended to be used only by the ReplicatedStore::BackupAsyncOperation class.
        /// Consumers of this class should instead use either
        ///   a. BackupLocal if they are creating only full backups
        ///   b. BeginBackupLocal/EndBackupLocal for full, incremental and log-truncation backups
        /// </summary>
        /// <param name="dir">The destination directory where the store should be backed up to.</param>
        /// <param name="backupOption">The backup option. The default option is StoreBackupOption::Full.</param>
        /// <returns>ErrorCode::Success() if the backup was successfully done. An appropriate ErrorCode otherwise.</returns>
        Common::ErrorCode BackupLocalImpl(
            std::wstring const & dir, 
            StoreBackupOption::Enum backupOption,
            __out Common::Guid & backupChainFullBackupGuid,
            __out ULONG & backupIndex);

        Common::AsyncOperationSPtr BeginBackupLocal(
            std::wstring const & backupDirectory,
            StoreBackupOption::Enum backupOption,
            Api::IStorePostBackupHandlerPtr const & postBackupHandler,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent,
            Common::ErrorCode createError = Common::ErrorCodeValue::Success) override;

        Common::AsyncOperationSPtr BeginBackupLocal(
            std::wstring const & backupDirectory,
            StoreBackupOption::Enum backupOption,
            Api::IStorePostBackupHandlerPtr const & postBackupHandler,
            StoreBackupRequester::Enum backupRequester,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent,
            Common::ErrorCode createError = Common::ErrorCodeValue::Success);

        Common::ErrorCode EndBackupLocal(__in Common::AsyncOperationSPtr const & operation) override;

        Common::AsyncOperationSPtr BeginRestoreLocal(
            std::wstring const & backupDirectory,
            RestoreSettings const &,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndRestoreLocal(__in Common::AsyncOperationSPtr const & operation) override;

        Common::ErrorCode DisableIncrementalBackup();
        Common::ErrorCode SetIncrementalBackupState(Common::Guid const & backupChainfullBackupGuid, ULONG const & prevBackupIndex);

        // *********
        // TestHooks
        // *********

        void Test_SetTargetCopyOperationSize(size_t);
        Common::ErrorCode Test_ValidateAndMergeBackupChain(std::wstring const & restoreDir, __out std::wstring & mergedRestoreDir);
        void Test_SetDatalossCallbackActive(bool value) { this->isDatalossCallbackActive_.store(value); }
        void Test_SetLogTruncationTestMode(bool value) { test_IsLogTruncationTestMode_ = value; }
        void Test_IncrementLogTruncationTimerFireCount();
        bool Test_IsLogTruncationTimerSet() { return logTruncationTimer_ != nullptr; }
        long Test_GetLogTruncationTimerFireCount() { return test_LogTruncationTimerFireCount_; }
        void Test_SetSecondaryPumpEnabled(bool value) { test_SecondaryPumpEnabled_ = value; };
        void Test_TryFaultStreamAndStopSecondaryPump(__in FABRIC_FAULT_TYPE faultType);
        void Test_SetStartFabricTimerWithCancelWait(bool value) { test_StartFabricTimerWithCancelWait_ = value; };
        bool Test_GetCopyStreamComplete();
        void Test_SetCopyStreamComplete(bool);
        Common::TimeSpan const & Test_GetLocalApplyDelay();
        void Test_SetLocalApplyDelay(Common::TimeSpan const &);
        bool Test_GetSlowCommitTestEnabled() const { return test_SlowCommitEnabled_; }
        void Test_SetSecondaryApplyFaultInjectionEnabled(bool value) { test_SecondaryApplyFaultInjectionEnabled_ = value; }
        void Test_SetTombstoneCleanupEnabled(bool value) { test_TombstoneCleanupEnabled_ = value; }
        void Test_SetEnableTombstoneCleanup2(bool value) { settings_.EnableTombstoneCleanup2 = value; }
#if defined(PLATFORM_UNIX)
        bool Test_IsSecondaryPumpClosed();
#endif
        void Test_SetTestHookContext(TestHooks::TestHookContext const &) override;

        //
        // IStateProvider
        //
        Common::ErrorCode Test_GetBeginUpdateEpochInjectedFault() override;

    private:

        void Test_WaitForSignalIfSet(std::wstring const & signalName);
        void Test_BlockUpdateEpochIfNeeded();
        void Test_BlockSecondaryPumpIfNeeded();

        Common::ErrorCode Test_GetInjectedFault(std::wstring const & tag);

    public:

		//
		// Implemented by the derived class to create the specific local store object, e.g. esestore
        //
        virtual std::wstring GetLocalStoreRootDirectory() = 0;
        virtual std::wstring GetFileStreamFullCopyRootDirectory() = 0;

        virtual void CreateLocalStore() = 0;
        virtual void ReleaseLocalStore() = 0;

        virtual ILocalStoreUPtr CreateTemporaryLocalStore() = 0;
        virtual ILocalStoreUPtr CreateFullCopyLocalStore() = 0;
        virtual ILocalStoreUPtr CreatePartialCopyLocalStore() = 0;
#ifndef PLATFORM_UNIX
        virtual ILocalStoreUPtr CreateRebuildLocalStore() = 0;
#endif

        virtual Common::ErrorCode CreateLocalStoreFromFullCopyData() = 0;
        virtual Common::ErrorCode MarkPartialCopyLocalStoreComplete() = 0;
        virtual Common::ErrorCode TryRecoverPartialCopyLocalStore(__out ILocalStoreUPtr &) = 0;

        virtual Common::ErrorCode DeleteFullCopyDatabases() = 0;
        virtual Common::ErrorCode DeletePartialCopyDatabases() = 0;
        virtual Common::ErrorCode BackupAndDeleteDatabase() = 0;
        virtual void BestEffortDeleteAllCopyDatabases() = 0;

        virtual Common::ErrorCode PreCopyNotification(std::vector<std::wstring> const &) = 0;
        virtual Common::ErrorCode PostCopyNotification() = 0;

        virtual bool ShouldRestartOnWriteError(Common::ErrorCode const &) = 0;

        virtual Common::ErrorCode CheckPreOpenLocalStoreHealth(__out bool & reportedHealth) 
        { 
            reportedHealth = false; 
            return Common::ErrorCodeValue::Success; 
        };
        virtual Common::ErrorCode CheckPostOpenLocalStoreHealth(bool) { return Common::ErrorCodeValue::Success; };

        // *******************
        // IStoreBase methods
        // *******************
        virtual ::FABRIC_TRANSACTION_ISOLATION_LEVEL GetDefaultIsolationLevel();

        virtual Common::ErrorCode SetDefaultIsolationLevel(::FABRIC_TRANSACTION_ISOLATION_LEVEL isolationLevel);

        // Overload specific to ReplicatedStore.
        // Supports plumbing of Id from application
        // across replica boundaries
        //
        virtual Common::ErrorCode CreateTransaction(
            __out TransactionSPtr & transactionSPtr);

        // Overload specific to ReplicatedStore.
        // Supports plumbing of Id from application
        // across replica boundaries
        //
        virtual Common::ErrorCode CreateTransaction(
            Common::ActivityId const & activityId,
            __out TransactionSPtr & transactionSPtr);

        virtual Common::ErrorCode CreateTransaction(
            Common::ActivityId const & activityId,
            TransactionSettings const &,
            __out TransactionSPtr & transactionSPtr);

        // ************************
        // IReplicatedStore methods
        // ************************

        virtual Common::ErrorCode CreateSimpleTransaction(
            __out TransactionSPtr & transactionSPtr);

        virtual Common::ErrorCode CreateSimpleTransaction(
            Common::ActivityId const & activityId,
            __out TransactionSPtr & transactionSPtr);

        virtual Common::ErrorCode Insert(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            __in void const * value,
            size_t valueSizeInBytes);

        //Internal method, not from public Interface
        virtual Common::ErrorCode InternalInsert(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            __in void const * value,
            size_t valueSizeInBytes);

        virtual Common::ErrorCode Update(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            ::FABRIC_SEQUENCE_NUMBER checkOperationNumber,
            std::wstring const & newKey,
            __in_opt void const * newValue,
            size_t valueSizeInBytes);

        //Internal method, not from public Interface
        virtual Common::ErrorCode InternalUpdate(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            ::FABRIC_SEQUENCE_NUMBER checkOperationNumber,
            std::wstring const & newKey,
            __in_opt void const * newValue,
            size_t valueSizeInBytes);

        virtual Common::ErrorCode Delete(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            ::FABRIC_SEQUENCE_NUMBER checkOperationNumber = ILocalStore::SequenceNumberIgnore);

        //Internal method, not from public Interface
        virtual Common::ErrorCode InternalDelete(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            ::FABRIC_SEQUENCE_NUMBER checkOperationNumber = ILocalStore::SequenceNumberIgnore);

        Common::ErrorCode InitializeLocalStoreForUnittests(bool databaseShouldExist) override;
        Common::ErrorCode InitializeLocalStoreForJobQueue(bool shouldExist);

        virtual FILETIME GetStoreUtcFILETIME();

        virtual Common::ErrorCode CreateEnumerationByTypeAndKey(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & keyStart,
            __out EnumerationSPtr & enumerationSPtr);

        virtual Common::ErrorCode InternalCreateEnumerationByTypeAndKey(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & keyStart,
            __out EnumerationSPtr & enumerationSPtr);

        virtual Common::ErrorCode InternalReadExact(
            __in TransactionSPtr const & tx,
            __in std::wstring const & type,
            __in std::wstring const & key,
            __out std::vector<byte> & value,
            __out __int64 & operationLsn);

        virtual Common::ErrorCode GetCurrentEpoch(__out FABRIC_EPOCH & epoch) const override;

        FABRIC_EPOCH GetCachedCurrentEpoch() const;
        void UpdateCachedCurrentEpoch(FABRIC_EPOCH const & epoch);

        //
        // Services using the replicated store can use this method to update the replicator settings
        //
        virtual Common::ErrorCode UpdateReplicatorSettings(FABRIC_REPLICATOR_SETTINGS const &) override;
        virtual Common::ErrorCode UpdateReplicatorSettings(Reliability::ReplicationComponent::ReplicatorSettingsUPtr const &) override;

        virtual bool IsThrottleNeeded() const;
        virtual Common::ErrorCode SetThrottleCallback(ThrottleCallback const & throttleCallback);

        bool IsMigrationSourceSupported() override { return true; }
        bool ShouldMigrateKey(std::wstring const & type, std::wstring const & key) override;

        //
        // IStatefulServiceReplica
        //

        Common::AsyncOperationSPtr BeginOpen(
            __in ::FABRIC_REPLICA_OPEN_MODE openMode,
            __in Common::ComPointer<IFabricStatefulServicePartition> const & partition,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndOpen(
            __in Common::AsyncOperationSPtr const & operation,
            __out Common::ComPointer<::IFabricReplicator> & replicator) override;

        Common::AsyncOperationSPtr BeginChangeRole(
            __in ::FABRIC_REPLICA_ROLE newRole,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndChangeRole(
            __in Common::AsyncOperationSPtr const & operation,
            __out std::wstring & serviceLocation) override;

        Common::AsyncOperationSPtr BeginClose(
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndClose(
            __in Common::AsyncOperationSPtr const & operation) override;

        void Abort() override;

        Common::ErrorCode SynchronousClose();

        //
        // FabricComponent
        //
        Common::ErrorCode OnOpen() override;
        Common::ErrorCode OnClose() override;
        void OnAbort() override;

        //
        // IStateProvider
        //

        Common::AsyncOperationSPtr BeginUpdateEpoch(
            FABRIC_EPOCH const & epoch,
            FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & state) override;
        Common::ErrorCode EndUpdateEpoch(
            Common::AsyncOperationSPtr const & asyncOperation) override;

        Common::ErrorCode GetLastCommittedSequenceNumber(
            __out FABRIC_SEQUENCE_NUMBER & sequenceNumber) override;

        Common::AsyncOperationSPtr BeginOnDataLoss(
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & state) override;
        Common::ErrorCode EndOnDataLoss(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out bool & isStateChanged) override;

        Common::ErrorCode GetCopyContext(
            __out Common::ComPointer<::IFabricOperationDataStream> & copyContextStream) override;

        Common::ErrorCode GetCopyState(
            ::FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            Common::ComPointer<::IFabricOperationDataStream> const & copyContextStream,
            Common::ComPointer<::IFabricOperationDataStream> & copyStateStream) override;

        // ********************
        // Logical Timer support
        // ********************

        virtual Common::ErrorCode ReplicatedStore::GetCurrentStoreTime(__out int64 &);

        // *********************
        // Repair Policy support
        // *********************

        Common::ErrorCode InitializeRepairPolicy(
            Api::IClientFactoryPtr const &,
            std::wstring const & serviceName,
            bool allowRepairUpToQuorum = false);

        //
        // Query
        //

        Common::ErrorCode GetQueryStatus(__out Api::IStatefulServiceReplicaStatusResultPtr &) override;
        void SetQueryStatusDetails(std::wstring const &) override;
        void SetMigrationQueryResult(std::unique_ptr<MigrationQueryResult> &&) override;

        void AppendQueryStatusDetails(std::wstring const &);
        void ClearQueryStatusDetails();

        //
        // Database migration
        //

        void SetTxEventHandler(IReplicatedStoreTxEventHandlerWPtr const &) override;
        void AbortOutstandingTransactions() override;
        void ClearTxEventHandlerAndBlockWrites() override;

        //
        // Health Reporting
        //
        
        void OnSlowCommit();

    public:

        // ****************
        // Helper functions
        // ****************

        void IncrementReaderCounterLocked();
        void DecrementReaderCounterLocked();

        static bool IsStoreMetadataType(std::wstring const & type);
        static std::wstring CreateTombstoneKey1(std::wstring const & type, std::wstring const & key);

        bool TryParseTombstoneKey1(std::wstring const & key, __out std::wstring & dataType, __out std::wstring & dataKey);
        void UpdatePendingTransactions(int pendingTransactionCount) { pendingTransactions_ = pendingTransactionCount; }

        Common::ErrorCode CreateLocalStoreTransaction(__out TransactionSPtr &);
        Common::ErrorCode CreateLocalStoreTransaction(ILocalStoreUPtr const &, __out TransactionSPtr &);

        Common::ErrorCode GetTombstoneLowWatermark(__out TombstoneLowWatermarkData &);
        Common::ErrorCode GetTombstoneLowWatermark(TransactionSPtr const &, __out TombstoneLowWatermarkData &);

        Common::ErrorCode GetProgressVector(
            ILocalStoreUPtr const & localStoreOverride,
            DataSerializerUPtr const & dataSerializerOverride,
            ::FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            __out std::vector<ProgressVectorEntry> &);

        class Enumeration;
        class Transaction;
        class TransactionBase;
        class HealthTracker;

    protected:

        void ReportHealth(Common::SystemHealthReportCode::Enum, std::wstring const & description, Common::TimeSpan const ttl);

    private:
        class BackupAsyncOperation;
        class ChangeRoleAsyncOperation;
        class CloseAsyncOperation;
        class OnDataLossAsyncOperation;
        class OpenAsyncOperation;
        class RestoreAsyncOperation;
        class StateChangeAsyncOperation;
        class UpdateEpochAsyncOperation;

        class SecondaryPump;
        class SimpleTransaction;
        class SimpleTransactionGroup;
        class StateMachine;
        class TransactionReplicator;
        class TransactionTracker;

        friend class Reliability::FailoverManagerComponent::FailoverManager;
        friend class ReliabilityUnitTest::FailoverManagerStoreTest;

        typedef std::shared_ptr<ReplicatedStore::SimpleTransactionGroup> SimpleTransactionGroupSPtr;

    private:

        void InitializeCtor(
            ReplicatedStoreSettings const &,
            Api::IStoreEventHandlerPtr const &,
            Api::ISecondaryEventHandlerPtr const &);

        // **************
        // Helper methods
        // ************** 

        std::shared_ptr<TransactionReplicator> TryGetTxReplicator() const;

        Common::ErrorCode CheckAccess(::FABRIC_SERVICE_PARTITION_ACCESS_STATUS);
        Common::ErrorCode CheckWriteAccess(TransactionSPtr const &, std::wstring const & type);
        Common::ErrorCode CheckReadAccess(TransactionSPtr const &, std::wstring const & type);
        Common::ErrorCode CheckTypeAccess(std::wstring const &);
        Common::ErrorCode DataExists(TransactionSPtr const &, std::wstring const & type, std::wstring const & key, __out ::FABRIC_SEQUENCE_NUMBER & operationLSN);

        Common::ErrorCode InnerGetCurrentProgress(TransactionSPtr const &, __out ::FABRIC_SEQUENCE_NUMBER & operationLSN);
        Common::ErrorCode InnerGetCurrentEpoch(
            TransactionSPtr const &,
            std::unique_ptr<Store::DataSerializer> const &,
            __out FABRIC_EPOCH & epoch) const;
        Common::ErrorCode InnerLoadCachedCurrentEpoch();
        Common::ErrorCode InnerLoadCachedCurrentEpoch(TransactionSPtr &&);
        Common::ErrorCode LoadCachedCurrentEpoch();
        Common::ErrorCode RecoverEpochHistory();

        void ScheduleTombstoneCleanupIfNeeded(size_t updatedTombstonesCount);
        void CleanupTombstones(size_t updatedTombstonesCount);
        Common::ErrorCode CleanupTombstones1(
            TransactionSPtr const & txSPtr, 
            EnumerationSPtr const & enumSPtr,
            __out size_t & removedCount,
            __out FABRIC_SEQUENCE_NUMBER & lastRemovedLsn);
        Common::ErrorCode CleanupTombstones2(
            TransactionSPtr const & txSPtr, 
            EnumerationSPtr const & enumSPtr,
            __out size_t & removedCount,
            __out FABRIC_SEQUENCE_NUMBER & lastRemovedLsn);
        Common::ErrorCode UpdateTombstone1(
            TransactionSPtr const &, 
            std::wstring const & type, 
            std::wstring const & key, 
            ::FABRIC_SEQUENCE_NUMBER operationLSN, 
            __out bool & countTombstone);
        Common::ErrorCode UpdateTombstone1(
            TransactionSPtr const &, 
            std::wstring const & type, 
            std::wstring const & key, 
            ::FABRIC_SEQUENCE_NUMBER operationLSN, 
            __out bool & countTombstone,
            __out std::wstring & tombstoneKey);
        Common::ErrorCode UpdateTombstone2(
            TransactionSPtr const &, 
            std::wstring const & type, 
            std::wstring const & key, 
            ::FABRIC_SEQUENCE_NUMBER operationLSN, 
            size_t index);
        Common::ErrorCode UpdateTombstone2(
            TransactionSPtr const &, 
            std::wstring const & type, 
            std::wstring const & key, 
            ::FABRIC_SEQUENCE_NUMBER operationLSN, 
            size_t index,
            __out std::wstring & tombstoneKey);
        Common::ErrorCode FinalizeTombstone(
            TransactionSPtr const & txSPtr,
            std::wstring const & type,
            std::wstring const & key,
            ::FABRIC_SEQUENCE_NUMBER operationLSN,
            size_t tombstoneIndex,
            __out bool & shouldCountTombstone);

        EnumerationSPtr CreateEnumerationByKey(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & keyStart);

        // ********************
        // Logical Timer support
        // ********************

        bool TryStartOrCancelFabricTimer();
        bool TryStartFabricTimer();
        bool TryCancelFabricTimer();

        // ********************
        // StateMachine support
        // ********************

        void FinishSecondaryPump();
        void FinishTransaction();
        void UntrackAndFinishTransaction(TransactionBase const &);

        // The following state machine callback functions occur under the state machine lock
        void SecondaryPumpClosedEventCallback(Common::ErrorCode const &, ReplicatedStoreState::Enum);
        void FinishTransactionEventCallback(Common::ErrorCode const &, ReplicatedStoreState::Enum);
        void OpenEventCallback(Common::AsyncOperationSPtr const & outerOperation, Common::ErrorCode const &, ReplicatedStoreState::Enum, bool);

        void OnOpenLocalStoreAsyncComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
        void OnOpenLocalStoreSyncComplete(Common::AsyncOperationSPtr const & outerOperation, Common::ErrorCode const &);
        Common::ErrorCode PreRepairOpen();
        void OnOpenRepairCompleted(Common::AsyncOperationSPtr const & outerOperation, bool expectedCompletedSynchronously);
        void PostRepairOpen(Common::AsyncOperationSPtr const & outerOperation, Common::ErrorCode const &);

        void ChangeRoleEventCallback(
            Common::AsyncOperationSPtr const & outerOperation, 
            Common::ErrorCode const &, 
            ReplicatedStoreState::Enum,
            ::FABRIC_REPLICA_ROLE newRole);

        void CloseEventCallback(Common::AsyncOperationSPtr const & outerOperation, Common::ErrorCode const &, ReplicatedStoreState::Enum);

        // Helper functions called by the state machine callback functions (still under state machine lock)
        bool TryCompletePendingChangeRoleOperation(Common::ErrorCode const &);
        bool TryCompletePendingCloseOperation(Common::ErrorCode const &);
        bool TryStartSecondaryPump();
        bool TryCancelSecondaryPump();
        Common::ErrorCode InitializePrimaryProcessing();
        void TryUninitializePrimaryProcessing();
        void StartPrimaryPendingTimer();
        void CancelPrimaryPendingTimer();
        void Cleanup();
        void PostCompletion(std::wstring const & tag, Common::AsyncOperationSPtr const &, Common::ErrorCode const &);

        Common::ErrorCode AttachOrCreateSimpleTransactionGroup(
            Common::ActivityId const & activityId, 
            __out std::shared_ptr<ReplicatedStore::TransactionBase> &);

        void SimpleTransactionGroupTimerCallback();
        void OnRollbackSimpleTransactionGroup(SimpleTransactionGroup * group);
        void CloseCurrentSimpleTransactionGroup();
        void InnerCloseCurrentSimpleTransactionGroup();

        Common::ErrorCode InitializeLocalStore(bool shouldExist);
        Common::ErrorCode InnerInitializeLocalStoreAndSerializer();

        std::wstring GetCopyInstanceName(std::wstring const & copyTag) const;
        Common::ErrorCode LoadCopyOperationSizeFromReplicator();

    public:
        Common::ErrorCode RecoverTombstones();
        size_t Test_GetTombstoneCount() const;
        Common::ErrorCode Test_TerminateLocalStore();

    private:
        void SetTombstoneCount(size_t count);
        void IncrementTombstoneCount(size_t count);
        void DecrementTombstoneCount(size_t count);
        void UpdateTombstonePerfCounter();

        Common::ErrorCode TryLoadFullCopyLocalStore();
        Common::ErrorCode GetLocalStoreForFullCopy(__inout ILocalStoreUPtr &);
        Common::ErrorCode GetLocalStoreForPartialCopy(__inout ILocalStoreUPtr &);
        Common::ErrorCode GetLocalStoreForCopy(
            __inout ILocalStoreUPtr &,
            std::wstring const & copyTag,
            std::function<ILocalStoreUPtr(void)> const &);

        Common::ErrorCode FinishFileStreamFullCopy(CopyType::Enum, std::wstring const & srcRestoreDirectory);
        Common::ErrorCode RestoreFileStreamToMainLocalStore(std::wstring const & srcRestoreDirectory);
        Common::ErrorCode RebuildFileStreamToNewLocalStore(std::wstring const & srcRestoreDirectory);
        Common::ErrorCode FinishFullCopy(ILocalStoreUPtr &&);
        Common::ErrorCode FinishFullCopy_CallerHoldsLock(ILocalStoreUPtr &&);
        Common::ErrorCode FinishPartialCopy(__in SecondaryPump &, ILocalStoreUPtr &&);
        Common::ErrorCode CleanupPartialCopy(ILocalStoreUPtr &&);
        Common::ErrorCode RecoverPartialCopy();

        Common::ErrorCode MarkPartialCopyReplayInProgress(FABRIC_SEQUENCE_NUMBER lsn);
        Common::ErrorCode MarkPartialCopyReplayCompleted();
        Common::ErrorCode CheckPartialCopyReplayCompleted();

        Common::ErrorCode CleanupLocalStore();
        Common::ErrorCode TerminateLocalStore();

        Common::ErrorCode ValidateRestoreServiceData(std::wstring const & dir, bool performLsnCheck);
        Common::ErrorCode ValidateRestoreLSN(ILocalStoreUPtr const & restoreLocalStore, bool performLsnCheck);
        Common::ErrorCode DeleteRestoreProgressVectorData(ILocalStoreUPtr const & restoreLocalStore);
        Common::ErrorCode WriteRestoreMetadata(
            std::wstring const & dir, 
            StoreBackupType::Enum const & backupType,
            Common::Guid const & backupChainGuid,
            ULONG & backupIndex);

		void GetBackupFiles(__in std::wstring const & restoreDir, __out std::unordered_set<std::wstring> & backupFiles);

        Common::ErrorCode ValidateBackupChain(
            __in std::wstring const & restoreDir, 
            __out std::map<ULONG, std::wstring> & backupChainOrderedDirList);

		Common::ErrorCode ValidateBackupInfo(__in std::wstring const & dataFileName, __in BackupRestoreData const & restoreData);
		Common::ErrorCode ValidateBackupFiles(__in std::wstring const & restoreDir, __in BackupRestoreData const & restoreData);

        Common::ErrorCode ValidateAndMergeBackupChain(std::wstring const & restoreDir, __out std::wstring & mergedRestoreDir);
        Common::ErrorCode ReadAndValidateRestoreData(std::wstring const & dir, bool validateBackupInfo, __out BackupRestoreData & restoreData);
        Common::ErrorCode GetCurrentPartitionInfo(__out ServiceModel::ServicePartitionInformation &);

        Common::ErrorCode GetIncrementalBackupState(
            __in TransactionSPtr & txSPtr, 
            __out bool & allowed,
            __out Common::Guid & backupChainfullBackupGuid,
            __out ULONG & prevBackupIndex);

        Common::ErrorCode SetIncrementalBackupState(bool allow, Common::Guid const & backupChainfullBackupGuid, ULONG const & prevBackupIndex);
        Common::ErrorCode InlineReopen();

		Common::ErrorCode DisableIncrementalBackup(__in TransactionSPtr & txSPtr);
		Common::ErrorCode SetIncrementalBackupState(
			__in TransactionSPtr & txSPtr,
			bool allow, 
			Common::Guid const & backupChainfullBackupGuid, 
			ULONG const & prevBackupIndex);

        void LogTruncationTimerCallback();
        void LogTruncationCallback(Common::AsyncOperationSPtr const & asyncOp);
        void ArmLogTruncationTimer();
        void TryStartLogTruncationTimer(::FABRIC_REPLICA_ROLE newRole);
        void TryStopLogTruncationTimer();

        void OnWriteError(__in ReplicatedStore::TransactionBase &, Common::ErrorCode const &);

    private:

        Reliability::ReplicationComponent::ReplicatorSettingsUPtr replicatorSettings_;
        Store::ReplicatedStoreSettings settings_;
        Api::IStoreEventHandlerPtr storeEventHandler_;
        Api::ISecondaryEventHandlerPtr secondaryEventHandler_;

        std::unique_ptr<StateMachine> stateMachineUPtr_;
        Common::AsyncOperationSPtr pendingChangeRoleOperation_;
        Common::AsyncOperationSPtr pendingCloseOperation_;

        std::wstring localStoreInstanceName_;

        Common::ComPointer<IFabricStatefulServicePartition> partitionCPtr_;
        Common::ComPointer<IFabricReplicator> replicatorCPtr_;
        Common::ComPointer<IFabricStateReplicator> stateReplicatorCPtr_;
        Common::ComPointer<IFabricInternalStateReplicator> internalStateReplicatorCPtr_;
        std::shared_ptr<TransactionReplicator> txReplicatorSPtr_;
        std::weak_ptr<TransactionReplicator> txReplicatorWPtr_;
        std::unique_ptr<TransactionTracker> txTrackerUPtr_;
        Common::TimeSpan test_localApplyDelay_;

        RWLOCK(DropLocalStoreRwLock, dropLocalStoreLock_);
        Common::atomic_bool isLocalStoreTerminated_;
        std::unique_ptr<Store::DataSerializer> dataSerializerUPtr_;
        std::unique_ptr<NotificationManager> notificationManagerUPtr_;
        std::shared_ptr<SecondaryPump> secondaryPumpSPtr_;
        ::FABRIC_REPLICA_ROLE replicaRole_;
		Common::atomic_bool allowIncrementalBackup_;

        Common::atomic_uint64 tombstoneCount_;
        Common::atomic_bool tombstoneCleanupActive_;
        RWLOCK(StoreLowWatermark, lowWatermarkLock_);
        int lowWatermarkReaderCount_;

        MUTABLE_RWLOCK(StoreEpoch, cachedCurrentEpochLock_);
        ::FABRIC_EPOCH cachedCurrentEpoch_;

        size_t targetCopyOperationSize_;

        // Statistics for debugging
        //
        Store::CopyStatistics copyStatistics_;

        bool isRoleSet_;
        bool isActive_;

        bool expectingPermanentFault_;

        // ReportFault when fatal error happened
        Common::ErrorCode fatalError_;
        RWLOCK(StoreFatalError, fatalErrorLock_);

        int pendingTransactions_;
        
        // SimpleTransactions
        Common::DateTime lastSimpleTransactionTimestamp_;
        Common::TimerSPtr simpleTransactionGroupTimer_;
        std::shared_ptr<SimpleTransactionGroup> simpleTransactionGroupSPtr_;
        RWLOCK(StoreTranscationGroup, transactionGroupLock_);

        // Used for testing error paths
        bool test_SecondaryPumpEnabled_;
        bool test_SlowCommitEnabled_;
        bool test_SecondaryApplyFaultInjectionEnabled_;
        bool test_TombstoneCleanupEnabled_;
        bool test_IsLogTruncationTestMode_;
        long test_LogTruncationTimerFireCount_;

        // Used for fault injection from FabricTest
        //
        TestHooks::TestHookContext testHookContext_;

        bool enableFabricTimer_;
        FabricTimeControllerSPtr fabricTimeControllerSPtr_;
        bool test_StartFabricTimerWithCancelWait_;

        StoreBackupState storeBackupState_;
        Common::atomic_bool isDatalossCallbackActive_;
        Common::TimerSPtr logTruncationTimer_;
        Common::RwLock logTruncationTimerLock_;

        IRepairPolicySPtr repairPolicySPtr_;
        std::shared_ptr<HealthTracker> healthTrackerSPtr_;
        std::shared_ptr<FileStreamFullCopyManager> fileStreamFullCopyManagerSPtr_;

        Common::RwLock queryStatusDetailsLock_;
        std::wstring queryStatusDetails_;
        MigrationQueryResultSPtr migrationDetails_;

        ReplicatedStorePerformanceCountersSPtr perfCounters_;
        IReplicatedStoreTxEventHandlerWPtr txEventHandler_;
        volatile bool isReadOnlyForMigration_;
    };
}
