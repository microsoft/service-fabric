//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Store
{
    class KeyValueStoreMigrator 
        : public IReplicatedStoreTxEventHandler
        , public enable_shared_from_this<KeyValueStoreMigrator> // for setting IReplicatedStoreTxEventHandler on source
        , public RootedObject
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
    private:
        typedef IStoreBase::TransactionBase Transaction;
        typedef std::shared_ptr<Transaction> TransactionSPtr;

    public:

        static Common::ErrorCode CreateForTStoreMigration(
            Common::ComponentRoot const &,
            Store::PartitionedReplicaId const & trace,
            __in IReplicatedStore &, 
            TSReplicatedStoreSettingsUPtr &&, 
            std::unique_ptr<MigrationInitData> &&,
            __out std::shared_ptr<KeyValueStoreMigrator> &);

        bool TryUpdateInitializationData(std::unique_ptr<MigrationInitData> &&);

        Common::AsyncOperationSPtr BeginMigration(Common::AsyncCallback const &, Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndMigration(Common::AsyncOperationSPtr const &);

        MigrationState::Enum GetMigrationState() const;
        MigrationPhase::Enum GetMigrationPhase() const;

        void Cancel();

    public:

        //
        // IReplicatedStoreTxEventHandler
        //

        void OnCreateTransaction(Common::ActivityId const &, TransactionMapKey) override;
        void OnReleaseTransaction(Common::ActivityId const &, TransactionMapKey) override;

        Common::ErrorCode OnInsert(
            Common::ActivityId const &,
            TransactionMapKey,
            std::wstring const & type,
            std::wstring const & key,
            void const * value,
            size_t const valueSizeInBytes) override;

        Common::ErrorCode OnUpdate(
            Common::ActivityId const &,
            TransactionMapKey,
            std::wstring const & type,
            std::wstring const & key,
            void const * newValue,
            size_t const valueSizeInBytes) override;

        Common::ErrorCode OnDelete(
            Common::ActivityId const &,
            TransactionMapKey,
            std::wstring const & type,
            std::wstring const & key) override;

        Common::ErrorCode OnCommit(
            Common::ActivityId const &,
            TransactionMapKey) override;

    private:

        class TransactionContext;
        class MigrationAsyncOperation;

        typedef std::shared_ptr<TransactionContext> TransactionContextSPtr;

        KeyValueStoreMigrator(
            Common::ComponentRoot const &,
            Store::PartitionedReplicaId const &,
            __in IReplicatedStore &,
            TSReplicatedStoreSettingsUPtr &&,
            std::unique_ptr<MigrationInitData> &&);

        __declspec (property(get=get_Settings)) MigrationSettings const & Settings;
        MigrationSettings const & get_Settings() const;

        MigrationPhase::Enum GetNextMigrationPhase(MigrationPhase::Enum, MigrationState::Enum);
        bool IsMigrationComplete();

        Common::ErrorCode CreateCleanTStoreTarget();
        Common::ErrorCode CleanupTStoreTargetFiles();
        Common::ErrorCode AttachTStoreTarget();
        std::shared_ptr<ILocalStore> CreateTStoreTarget();

        Common::ErrorCode AttachExistingEseTarget();
        std::shared_ptr<ILocalStore> CreateEseTarget();

        Common::ErrorCode InitializeTarget(__in ILocalStore &);
        
        Common::ErrorCode ProcessMigrationBatch(
            std::wstring const & startType,
            std::wstring const & startKey,
            __out TransactionSPtr & destTx,
            __out bool & isComplete,
            __out std::wstring & currentType,
            __out std::wstring & currentKey);

        Common::ErrorCode FinishMigration();

        void OnMigrationSuccess();
        void OnMigrationCanceled();
        void OnMigrationFailure(ErrorCode const & error);

        Common::ErrorCode CloseTarget();
        Common::ErrorCode CleanupTarget();

        void FailMigration(std::wstring const & statusDetails);
        void SetMigrationState(MigrationState::Enum state);

        void OnTargetCommitComplete(
            Common::ActivityId const &,
            TransactionSPtr const & destTx,
            Common::AsyncOperationSPtr const &,
            bool expectedCompletedSynchronously);

        TransactionContextSPtr GetDestinationTxContext(Common::ActivityId const &, TransactionMapKey);

        void TrackDeletingKey(TransactionContextSPtr const &, std::wstring const & type, std::wstring const & key);
        void UntrackDeletingKey(TransactionContextSPtr const &, std::wstring const & type, std::wstring const & key);
        bool DeletingKeyExists(std::wstring const & type, std::wstring const & key);
        bool DeletedKeyExists(std::wstring const & type, std::wstring const & key);
        TransactionSPtr FinalizeCommit(Common::ActivityId const &, TransactionMapKey);
        void FinalizeRelease(Common::ActivityId const &, TransactionMapKey);

        static std::wstring GetFullKey(std::wstring const & type, std::wstring const & key);

        Common::ErrorCode BackupSourceStore();
        Common::ErrorCode BackupToFileShare(std::wstring const & connectionString);
        Common::ErrorCode BackupToFileShare(std::wstring const & connectionString, __out std::wstring & archiveDest);
        Common::ErrorCode BackupToAzureStorage(std::wstring const & connectionString);

        bool TryRestoreSourceStore();
        Common::ErrorCode RestoreSourceStore();
        Common::ErrorCode RestoreFromFileShare(std::wstring const & connectionString);
        Common::ErrorCode RestoreFromFileShare(std::wstring const & archiveSrc, std::wstring const & restoreSrc);
        Common::ErrorCode RestoreFromAzureStorage(std::wstring const & connectionString);

        Common::ErrorCode GetBackupConnectionString(__out Common::SecureString &);
        std::wstring ReadBackupConnectionString(
            Common::WStringLiteral const & targetSection, 
            Common::WStringLiteral const & targetKey, 
            __out bool & isEncrypted);

        Common::ErrorCode GetFileShareBackupPaths(
            std::wstring const & connectionString,
            __out std::wstring & backupDest,
            __out std::wstring & archiveDest);

        std::wstring GetBackupRootDirectory();
        Common::ErrorCode TraceAndGetError(std::wstring && msg, Common::ErrorCode const &);

        IReplicatedStore & sourceReplicatedStore_; // lifetime owned by KeyValueStoreReplica

        TSReplicatedStoreSettingsUPtr tstoreSettings_;
        ILocalStoreSPtr targetLocalStore_;

        std::unique_ptr<MigrationInitData> initData_;
        MigrationPhase::Enum phase_;
        MigrationSettings settings_;
        MigrationState::Enum state_;
        mutable Common::RwLock stateLock_;

        volatile bool isCanceled_;
        volatile bool isFinished_;

        size_t migrationCount_;

        // Tracks deleted keys from write stream to resolve migration
        // conflicts.
        //
        std::unordered_set<std::wstring> uncommittedDeletes_;
        std::unordered_set<std::wstring> deletedKeys_;
        Common::RwLock deletedKeysLock_;

        // Migration currently depends on the application handling
        // and retrying on write conflicts. We currently do not support 
        // a mode where migration and write stream tx are interleaved 
        // and serialized to avoid returning write conflicts to the 
        // application.
        //
        // This would have to be implemented with the caveat of
        // delaying both the write and migration streams if needed.
        // 
        std::unordered_map<TransactionMapKey, TransactionContextSPtr> txMap_;
        Common::RwLock txMapLock_;
        size_t txCount_;
        Common::ManualResetEvent txCountEvent_;
    };

    typedef std::shared_ptr<KeyValueStoreMigrator> KeyValueStoreMigratorSPtr;
}
