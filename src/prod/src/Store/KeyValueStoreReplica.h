// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class KeyValueStoreTransaction;

    //
    // Two usage patterns are supported for this class:
    //
    //  - C++ classes (native system services) derive directly from KeyValueStoreReplica. 
    //  - COM wrappers (and FM) use containment to expose for managed KVS.
    //
    // This class is agnostic of the underlying IReplicatedStore implementation, which is one of:
    //
    //  - EseReplicatedStore (ReplicatedStore replication with ESE local store)
    //  - TSReplicatedStore (full V2 store stack for replication and local storage using TStore)
    //
    class KeyValueStoreReplica
        : public Api::IKeyValueStoreReplica
        , public Api::IStoreEventHandler
        , public Api::ISecondaryEventHandler
        , public Common::ComponentRoot
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY(KeyValueStoreReplica);

        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::TraceId;
        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::get_TraceId;

    public:

        // For sub-classing by native system services
        //
        KeyValueStoreReplica(
            Common::Guid const &, 
            FABRIC_REPLICA_ID);

        virtual ~KeyValueStoreReplica();

    public:

        // This is only used by FM for its unit tests.
        //
        static IReplicatedStoreUPtr CreateForUnitTests(
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            EseLocalStoreSettings const &,
            ComponentRoot const & root);

    public:

        // These are used by KeyValueStoreReplicaFactory for
        // the public KVS stack
        //
        static Common::ErrorCode CreateForPublicStack_V1(
            Common::Guid const &, 
            FABRIC_REPLICA_ID,
            Reliability::ReplicationComponent::ReplicatorSettingsUPtr &&,
            ReplicatedStoreSettings const &,
            EseLocalStoreSettings const &,
            Api::IStoreEventHandlerPtr const &,
            Api::ISecondaryEventHandlerPtr const &,
            Api::IClientFactoryPtr const &,
            std::wstring const & serviceName,
            __out Api::IKeyValueStoreReplicaPtr &);

        static Common::ErrorCode CreateForPublicStack_V2(
            Common::Guid const &, 
            FABRIC_REPLICA_ID,
            Reliability::ReplicationComponent::ReplicatorSettingsUPtr &&,
            TSReplicatedStoreSettingsUPtr &&,
            Api::IStoreEventHandlerPtr const &,
            Api::ISecondaryEventHandlerPtr const &,
            __out Api::IKeyValueStoreReplicaPtr &);

        // This is used by native system services and accepts both
        // V1 and V2 settings to minimize the amount of duplicated
        // logic in system service factory classes
        //
        Common::ErrorCode InitializeForSystemServices(
            bool enableTStore,
            std::wstring const & workingDirectory,
            std::wstring const & databaseDirectory,
            std::wstring const & sharedLogFileName,
            TxnReplicator::SLConfigBase const & sharedLogConfig,
            Common::Guid const & sharedLogGuid,
            Reliability::ReplicationComponent::ReplicatorSettingsUPtr && replicatorSettings,
            ReplicatedStoreSettings && replicatedStoreSettings_V1,
            EseLocalStoreSettings const & eseSettings,
            Api::IClientFactoryPtr const & clientFactory,
            Common::NamingUri const & serviceName,
            std::vector<byte> const & initData,
            bool allowRepairUpToQuorum = false);

        Common::ErrorCode InitializeForTestServices(
            bool enableTStore,
            std::wstring const & workingDirectory,
            std::wstring const & databaseDirectory,
            std::wstring const & sharedLogFileName,
            TxnReplicator::SLConfigBase const & sharedLogConfig,
            Common::Guid const & sharedLogGuid,
            Reliability::ReplicationComponent::ReplicatorSettingsUPtr && replicatorSettings,
            ReplicatedStoreSettings && replicatedStoreSettings_V1,
            EseLocalStoreSettings const & eseSettings,
            Api::IClientFactoryPtr const & clientFactory,
            Common::NamingUri const & serviceName,
            bool allowRepairUpToQuorum);

    public:

        __declspec(property(get=get_IsActivePrimary)) bool IsActivePrimary;
        __declspec(property(get=get_ReplicatedStore)) IReplicatedStore * ReplicatedStore;

        bool get_IsActivePrimary() const { return iReplicatedStore_->GetIsActivePrimary(); }
        IReplicatedStore * get_ReplicatedStore() const { return iReplicatedStore_.get(); }

    protected:

        // 
        // Used by subclass (if applicable) to process IStatefulServiceReplica events
        //
        virtual Common::ErrorCode OnOpen(Common::ComPointer<IFabricStatefulServicePartition> const &);
        virtual Common::ErrorCode OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out std::wstring & serviceLocation);
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    protected:
        
        //
        // IStoreEventHandler
        //

        virtual void OnDataLoss() override;

        virtual Common::AsyncOperationSPtr BeginOnDataLoss(
            __in Common::AsyncCallback const &,
            __in Common::AsyncOperationSPtr const &) override;

        virtual Common::ErrorCode EndOnDataLoss(
            __in Common::AsyncOperationSPtr const & asyncOperation,
            __out bool & isStateChanged) override;

    protected:

        //
        // ISecondaryEventHandler
        //
        virtual Common::ErrorCode OnCopyComplete(Api::IStoreEnumeratorPtr const &) override;
        virtual Common::ErrorCode OnReplicationOperation(Api::IStoreNotificationEnumeratorPtr const &) override;

    public:

        void TestAssertAndTransientFault(std::wstring const & reason) const;
        void TransientFault(std::wstring const & reason) const;

        Common::ErrorCode UpdateReplicatorSettings(
            Reliability::ReplicationComponent::ReplicatorSettingsUPtr const &);

    public:

        //
        // IStatefulServiceReplica
        //
        Common::AsyncOperationSPtr BeginOpen(
            ::FABRIC_REPLICA_OPEN_MODE openMode,
            Common::ComPointer<::IFabricStatefulServicePartition> const & partition,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndOpen(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out Common::ComPointer<::IFabricReplicator> & replicator);

        Common::AsyncOperationSPtr BeginChangeRole(
            ::FABRIC_REPLICA_ROLE newRole,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);    
        Common::ErrorCode EndChangeRole(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out std::wstring & serviceLocation);

        Common::AsyncOperationSPtr BeginClose(
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndClose(
            Common::AsyncOperationSPtr const & asyncOperation);

        void Abort();

        Common::ErrorCode UpdateInitializationData(std::vector<byte> &&) override;
        
    public:

        //
        // IKeyValueStoreReplica Methods
        //
        Common::ErrorCode GetCurrentEpoch(
            __out ::FABRIC_EPOCH & currentEpoch);

        Common::ErrorCode UpdateReplicatorSettings(
            ::FABRIC_REPLICATOR_SETTINGS const & replicatorSettings);

        Common::ErrorCode CreateTransaction(
            ::FABRIC_TRANSACTION_ISOLATION_LEVEL isolationLevel,
            __out Api::ITransactionPtr & transaction);

        Common::ErrorCode CreateTransaction(
            ::FABRIC_TRANSACTION_ISOLATION_LEVEL isolationLevel,
            ::FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS const & settings,
            __out Api::ITransactionPtr & transaction);

        Common::ErrorCode Add(
            Api::ITransactionBasePtr const & transaction,
            std::wstring const & key,
            LONG valueSizeInBytes,
            const BYTE * value);

        Common::ErrorCode Remove(
            Api::ITransactionBasePtr const & transaction,
            std::wstring const & key,
            ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

        Common::ErrorCode Update(
            Api::ITransactionBasePtr const & transaction,
            std::wstring const & key,
            LONG valueSizeInBytes,
            const BYTE * value,
            ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

        Common::ErrorCode Get(
            Api::ITransactionBasePtr const & transaction,
            std::wstring const & key,
            __out Api::IKeyValueStoreItemResultPtr & itemResult);

        Common::ErrorCode TryAdd(
            Api::ITransactionBasePtr const & transaction,
            std::wstring const & key,
            LONG valueSizeInBytes,
            const BYTE * value,
            __out bool & added);

        Common::ErrorCode TryRemove(
            Api::ITransactionBasePtr const & transaction,
            std::wstring const & key,
            ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
            __out bool & exists);

        Common::ErrorCode TryUpdate(
            Api::ITransactionBasePtr const & transaction,
            std::wstring const & key,
            LONG valueSizeInBytes,
            const BYTE * value,
            ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
            __out bool & exists);

        Common::ErrorCode TryGet(
            Api::ITransactionBasePtr const & transaction,
            std::wstring const & key,
            __out Api::IKeyValueStoreItemResultPtr & itemResult);

        Common::ErrorCode GetMetadata(
            Api::ITransactionBasePtr const & transaction,
            std::wstring const & key,
            __out Api::IKeyValueStoreItemMetadataResultPtr & itemMetadataResult);

        Common::ErrorCode TryGetMetadata(
            Api::ITransactionBasePtr const & transaction,
            std::wstring const & key,
            __out Api::IKeyValueStoreItemMetadataResultPtr & itemMetadataResult);

        Common::ErrorCode Contains(
            Api::ITransactionBasePtr const & transaction,
            std::wstring const & key,
            __out BOOLEAN & result);

        Common::ErrorCode Enumerate(
            Api::ITransactionBasePtr const & transaction,
            __out Api::IKeyValueStoreItemEnumeratorPtr & itemEnumerator);

        Common::ErrorCode EnumerateByKey(
            Api::ITransactionBasePtr const & transaction,
            std::wstring const & key,
            bool strictPrefix,
            __out Api::IKeyValueStoreItemEnumeratorPtr & itemEnumerator);

        Common::ErrorCode EnumerateMetadata(
            Api::ITransactionBasePtr const & transaction,
            __out Api::IKeyValueStoreItemMetadataEnumeratorPtr & itemMetadataEnumerator);

        Common::ErrorCode EnumerateMetadataByKey(
            Api::ITransactionBasePtr const & transaction,
            std::wstring const & key,
            bool strictPrefix,
            __out Api::IKeyValueStoreItemMetadataEnumeratorPtr & itemMetadataEnumerator);

        Common::ErrorCode Backup(
            std::wstring const & dir);

        virtual Common::AsyncOperationSPtr BeginBackup(
            std::wstring const & backupDir,
            FABRIC_STORE_BACKUP_OPTION backupOption,
            Api::IStorePostBackupHandlerPtr const & postBackupHandler,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndBackup(
            __in Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode Restore(
            std::wstring const & dir);

        virtual Common::AsyncOperationSPtr BeginRestore(
            std::wstring const & backupDir,
            RestoreSettings const &,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndRestore(
            Common::AsyncOperationSPtr const & operation);

        virtual Common::ErrorCode GetQueryStatus(
            __out Api::IStatefulServiceReplicaStatusResultPtr &);

    private:

        Common::ErrorCode InitializeReplicatedStore_V1(
            Reliability::ReplicationComponent::ReplicatorSettingsUPtr &&,
            ReplicatedStoreSettings const &,
            EseLocalStoreSettings const &,
            Api::IStoreEventHandlerPtr const &,
            Api::ISecondaryEventHandlerPtr const &,
            Api::IClientFactoryPtr const &,
            std::wstring const & serviceName,
            bool allowRepairUpToQuorum);

        Common::ErrorCode InitializeReplicatedStore_V2(
            Reliability::ReplicationComponent::ReplicatorSettingsUPtr &&,
            TSReplicatedStoreSettingsUPtr &&,
            Api::IStoreEventHandlerPtr const &,
            Api::ISecondaryEventHandlerPtr const &);

        void TryStartMigration();
        void TryStartMigration_Unlocked(KeyValueStoreMigratorSPtr const &);
        void TryCancelMigration();
        void OnMigrationComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
        std::unique_ptr<MigrationInitData> TryDeserializeMigrationInitData(std::vector<byte> const & initData);
        Common::ErrorCode TryInitializeMigrator(std::unique_ptr<MigrationInitData> && migrationData);
        Common::ErrorCode TryInitializeMigrator_Unlocked(std::unique_ptr<MigrationInitData> && migrationData);

    private:

        class OpenAsyncOperation;
        class ChangeRoleAsyncOperation;
        class CloseAsyncOperation;

        KeyValueStoreTransaction * GetCastedTransaction(Api::ITransactionBasePtr const &);
        void TrackLifeCycleAssertEvent(std::wstring const & eventName, Common::TimeSpan const timeout);
        void UntrackLifeCycleAssertEvent(std::wstring const & eventName);

        // Keep only implementation agnostic classes here as members.
        //
        IReplicatedStoreUPtr iReplicatedStore_;
        Common::ComPointer<IFabricStatefulServicePartition> partition_;
        DeadlockDetectorSPtr deadlockDetector_;

        // Migration support
        //
        std::unique_ptr<TSReplicatedStoreSettings> replicatedStoreSettings_V2_;
        KeyValueStoreMigratorSPtr migrator_;
        Common::ExclusiveLock migratorLock_;
    };
}
