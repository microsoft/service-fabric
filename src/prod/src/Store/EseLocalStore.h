// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class Common::ExclusiveFile;

    typedef std::wstring StoreInstanceId;
    // These objects are short-lived, so wraparound is less of a concern
    typedef int TransactionInstanceId;
    typedef int EnumerationInstanceId;

    class EseLocalStore 
        : public ILocalStore
        , public Common::TextTraceComponent<Common::TraceTaskCodes::EseStore>
    {
    protected:
        static Common::GlobalWString sequenceNumberColumnName;
        static Common::GlobalWString lastModifiedColumnName;
        static Common::GlobalWString lastModifiedOnPrimaryColumnName;
        static Common::GlobalWString typeColumnName;
        static Common::GlobalWString keyColumnName;
        static Common::GlobalWString valueColumnName;
        static Common::GlobalWString operationLSNColumnName;
    
        static Common::GlobalWString primaryIndexName;
        static wchar_t const primaryIndexKeysData[];
        static Common::GlobalWString primaryIndexKeys;

        static wchar_t const operationLSNIndexKeysData[];
        static Common::GlobalWString operationLSNIndexKeys;
        static Common::GlobalWString operationLSNIndexName;
    
        static const size_t maxTypeLengthCharacters;
        static const size_t maxKeyLengthCharacters;
        static const size_t maxValueLengthBytes;
        static const _int64 maxOperationLSN_;
        static const _int64 lastModifiedOnPrimaryDefVal;

        static Common::GlobalWString ValidationDirectorySuffix;
        static Common::GlobalWString FullBackupSubDirName;

    public:
        static const LONG LOCAL_STORE_FLAG_NONE                     = 0x00;
        static const LONG LOCAL_STORE_FLAG_USE_EXCLUSIVE_FILE_LOCK  = 0x01;
        static const LONG LOCAL_STORE_FLAG_USE_LSN_COLUMN           = 0x02;
        static const LONG LOCAL_STORE_FLAG_USE_LAST_MODIFIED_ON_PRIMARY_COLUMN = 0x04;

        EseLocalStore(
            std::wstring const & directory,
            std::wstring const & fileName,
            LONG localStoreFlags);

        EseLocalStore(
            std::wstring const & directory,
            EseLocalStoreSettings const & settings,
            LONG localStoreFlags);

        virtual ~EseLocalStore();

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        __declspec(property(get=get_Directory)) std::wstring const & Directory;
        __declspec(property(get=get_FileName)) std::wstring const & FileName;
        __declspec(property(get=get_PerfCounters)) EseStorePerformanceCounters const & PerfCounters;
        __declspec(property(get=get_Settings)) EseLocalStoreSettings const & Settings;

        std::wstring const & get_TraceId() const { return traceId_; }
        std::wstring const & get_Directory() const { return directory_; }
        std::wstring const & get_FileName() const { return fileName_; }
        EseStorePerformanceCounters const & get_PerfCounters() const { return *perfCounters_; }
        EseLocalStoreSettings const & get_Settings() const { return *settings_; }

        virtual bool StoreExists();

        // Note, the lifecycle methods are not thread safe
        virtual Common::ErrorCode Initialize(std::wstring const & instanceName) override;
        virtual Common::ErrorCode Initialize(std::wstring const & instanceName, Federation::NodeId const &) override;
        virtual Common::ErrorCode Terminate() override;
        virtual void Drain() override;
        virtual Common::ErrorCode Cleanup() override;

        virtual ::FABRIC_TRANSACTION_ISOLATION_LEVEL GetDefaultIsolationLevel();

        virtual Common::ErrorCode CreateTransaction(
            __out TransactionSPtr & transactionSPtr);

        virtual Common::ErrorCode Insert(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            __in void const * value,
            size_t valueSizeInBytes,
            _int64 sequenceNumber = ILocalStore::SequenceNumberIgnore,
            FILETIME const * lastModifiedOnPrimaryUtc = nullptr);

        virtual Common::ErrorCode Update(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            _int64 checkSequenceNumber,
            std::wstring const & newKey,
            __in_opt void const * newValue,
            size_t valueSizeInBytes,
            _int64 sequenceNumber = ILocalStore::SequenceNumberIgnore,
            FILETIME const * lastModifiedOnPrimaryUtc = nullptr);

        virtual Common::ErrorCode Delete(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            _int64 checkSequenceNumber = ILocalStore::SequenceNumberIgnore);

        virtual Common::ErrorCode GetOperationLSN(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            __out ::FABRIC_SEQUENCE_NUMBER & operationLSN) override;

        virtual Common::ErrorCode UpdateOperationLSN(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            __in ::FABRIC_SEQUENCE_NUMBER operationLSN);

        virtual FILETIME GetStoreUtcFILETIME();
        _int64 GetStoreUtcInt64();
        _int64 FILETIMEToInt64(FILETIME const * filetime);
        FILETIME Int64ToFILETIME(_int64 const & time);

        Common::ErrorCode CreateEnumerationByPrimaryIndex(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & typeStart,
            __in std::wstring const & keyStart,
            __out EnumerationSPtr & enumerationSPtr);

        virtual Common::ErrorCode CreateEnumerationByTypeAndKey(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & keyStart,
            __out EnumerationSPtr & enumerationSPtr);

        virtual Common::ErrorCode CreateEnumerationByOperationLSN(
            __in TransactionSPtr const & transaction,
            __in _int64 fromOperationNumber,
            __out EnumerationSPtr & enumerationSPtr);

        virtual Common::ErrorCode GetLastChangeOperationLSN(
            __in TransactionSPtr const & transaction,
            __out ::FABRIC_SEQUENCE_NUMBER & operationLSN);

        Common::ErrorCode EstimateRowCount(__out size_t &);
        Common::ErrorCode EstimateDbSizeBytes(__out size_t &);
        Common::ErrorCode GetOpenFileSize(__out int64 &);
        
        Common::ErrorCode Backup(
            std::wstring const & backupDir, 
            StoreBackupOption::Enum backupOption = StoreBackupOption::Full);

        Common::ErrorCode PrepareRestoreForValidation(
            std::wstring const & backupDir,
            std::wstring const & instanceName);

        Common::ErrorCode MergeBackupChain(
            std::map<ULONG, std::wstring> const & backupChainOrderedDirList,
            __out std::wstring & mergedBackupDir) override;

        bool IsLogTruncationRequired() override;
		bool IsIncrementalBackupEnabled() override;

        Common::ErrorCode PrepareRestoreFromValidation();

        Common::ErrorCode PrepareRestore(std::wstring const & src);

        Common::ErrorCode StartDefragmentation();
        Common::ErrorCode StopDefragmentation();

        Common::ErrorCode ForceMaxCacheSize();
        Common::ErrorCode PrefetchDatabaseTypes(std::vector<std::wstring> const &);
        Common::ErrorCode ResetOptimizeTableScans();

        Common::ErrorCode JetToErrorCode(JET_ERR);
        bool ShouldAutoCompact(int64 fileSize);

    private:
        __declspec(property(get=get_IsExclusiveFileLockEnabled)) bool IsExclusiveFileLockEnabled;
        __declspec(property(get=get_IsOperationLSNColumnEnabled)) bool IsOperationLSNColumnEnabled;
        __declspec(property(get = get_IsLastModifiedOnPrimaryColumnEnabled)) bool IsLastModifiedOnPrimaryColumnEnabled;

        bool get_IsExclusiveFileLockEnabled() const { return (localStoreFlags_ & LOCAL_STORE_FLAG_USE_EXCLUSIVE_FILE_LOCK) != 0; }
        bool get_IsOperationLSNColumnEnabled() const { return (localStoreFlags_ & LOCAL_STORE_FLAG_USE_LSN_COLUMN) != 0; }
        bool get_IsLastModifiedOnPrimaryColumnEnabled() const 
        { 
            return (localStoreFlags_ & LOCAL_STORE_FLAG_USE_LAST_MODIFIED_ON_PRIMARY_COLUMN) != 0; 
        }

        JET_ERR GetV1CompatibleSequenceNumber(__in EseCursor &, __out _int64 &);

        Common::ErrorCode CreateDbDirectory();
        Common::ErrorCode DeleteDbDirectories();
        Common::ErrorCode DeleteDirectoryWithRetries(std::wstring const & dbDirectory);

        void ConstructorInitialize();
        Common::ErrorCode InnerInitialize(
            std::wstring const & instanceName, 
            bool isRestoreValidation);
        JET_ERR CreateInitialTablesAndColumns();
    
		Common::ErrorCode GetJetBackupOptions(__in StoreBackupOption::Enum backupOption, __out LONG & jetBackupOptions);
		
        static std::wstring GetValidationDirectoryName(std::wstring const & eseDirectory);		

        void OnConstructTransaction();
        void OnDestructTransaction();

        Common::ErrorCode CreateEnumerationByTypeAndKey(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & keyStart,
            bool strictType,
            __out EnumerationSPtr & enumerationSPtr);

        class EsePoolItem
        {
            DENY_COPY(EsePoolItem);
        private:
            EsePoolItem(EseLocalStore &store);

        public:
            static std::shared_ptr<EsePoolItem> CreateSPtr(EseLocalStore &);
            ~EsePoolItem();

            __declspec(property(get=get_Session)) EseSessionSPtr const & Session;
            __declspec(property(get=get_Database)) EseDatabaseSPtr const & Database;
            __declspec(property(get=get_UpdateCursor)) EseCursorSPtr const & UpdateCursor;
            __declspec(property(get=get_TraceId)) TransactionInstanceId TraceId;

            EseLocalStore & get_LocalStore() { return owner_; }
            EseSessionSPtr const & get_Session() { return session_; }
            EseDatabaseSPtr const & get_Database() { return database_; }
            EseCursorSPtr const & get_UpdateCursor() { return cursor_; }
            
            // The following is for the use of EsePool class
            JET_ERR InitializeForUse();

            std::shared_ptr<EseCursor> CreateCursor() const;
            
        private:
            EseLocalStore & owner_;
            EseSessionSPtr session_;
            EseDatabaseSPtr database_;
            EseCursorSPtr cursor_;
        };

        typedef std::shared_ptr<EsePoolItem> EsePoolItemSPtr;
        
        //
        // EsePool creates a pool of EsePoolItem objects that get reused.
        // The pool is created with three parameters:
        //      - min-pool-size
        //      - adjustment-size
        //      - eviction-period
        //
        // Pool size adjustment percentage is used as follows:
        //      - When pool is created it is initialized by allocating
        //        min-pool-size items
        //      - When a pool item is requested and all are in use, we grow
        //        the pool by the adjustment-size
        //      - When the pool has items that are not used in evictionPeriod
        //        we shrink the pool. The pool is shrunk to the count
        //        that has been used within eviction period.
        //      - When a pooled item is returned after use, a recycle
        //        (i.e. recreation of all Ese objects) can be requested.
        //        This is done to recreate objects after ESE error.
        //
        // For example consider:
        //      min pool size - N
        //      adjustment size - M
        //      adjustment frequency - T minutes
        //
        //      We allocate N items on the creation of the pool.
        //      If a pool item is requested and all N are in use, we create another M.
        //      When all N + M are in use we create another M, etc.
        //      When the pool has N + M items, and only N + O items have been used within the
        //      eviction period pool is shrunk to N+ O
        //      The shrinkage only happens when an item is returned to pool
        //      (i.e. there is no timer)
        //
        class EsePool : public std::enable_shared_from_this<EsePool>
        {
            DENY_COPY(EsePool);
        public:
            EsePool(EseLocalStore &store, uint minSize, uint adjustmentSize, Common::TimeSpan evictionPeriod);
            ~EsePool();

            Common::ErrorCode Initialize();
            void Abort();
            Common::ErrorCode GetItem(EsePoolItemSPtr & esePoolItem);
            void ReturnToPool(EsePoolItemSPtr const & esePoolItem, bool deletePoolItem);

        private:
            void Grow(uint adjustmentSize);
            void Shrink(uint shrinkCount);

            // Min size of the pool
            uint minSize_;
            // Size used for pool size adjustments
            uint adjustmentSize_;
            // Eviction period
            int64 evictionPeriodInTicks_;
            // Last time use was above different thresholds
            std::map<uint, int64> lastPeakUseTimeTickcount_;
            // Indicates if a pool growth work item has been queue up in the thread pool
            bool poolGrowthPending_;
            bool aborting_;

            EseLocalStore &store_;
            std::unordered_map<JET_SESID, EsePoolItemSPtr> inUsePoolItems_;
            std::unordered_map<JET_SESID, EsePoolItemSPtr> notInUsePoolItems_;
            RWLOCK(StoreEsePool, lock_);
        };

        typedef std::shared_ptr<EsePool> EsePoolSPtr;
        EsePoolSPtr esePool_;

        class Transaction : public TransactionBase
        {
            DENY_COPY(Transaction);
        public:
            Transaction(EseLocalStore &store, EsePoolSPtr const esePool);
            Transaction(EseLocalStore &store, Common::ErrorCodeValue::Enum const);
            virtual ~Transaction();

            __declspec(property(get=get_LocalStore)) EseLocalStore & LocalStore;
            __declspec(property(get=get_ErrorCode)) Common::ErrorCode const & ErrorCode;
            __declspec(property(get=get_TraceId)) TransactionInstanceId TraceId;
            __declspec(property(get=get_IsActive)) bool IsActive;
            __declspec(property(get=get_Rolledback)) bool Rolledback;

            EseLocalStore & get_LocalStore() const { return owner_; }
            Common::ErrorCode const & get_ErrorCode() const { return error_; }
            TransactionInstanceId get_TraceId() const { return traceId_; }
            bool get_IsActive() const { return active_.load(); }
            bool get_Rolledback() const { return rolledback_; }

            Common::ErrorCode TryGetSession(__out EseSessionSPtr &);
            Common::ErrorCode TryGetDatabase(__out EseDatabaseSPtr &);
            Common::ErrorCode TryGetUpdateCursor(__out EseCursorSPtr &);
            Common::ErrorCode TryCreateCursor(__out EseCursorSPtr &);

            virtual Common::AsyncOperationSPtr BeginCommit(
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & state);

            virtual Common::ErrorCode EndCommit(
                __in Common::AsyncOperationSPtr const & operation,
                __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber);

            virtual Common::ErrorCode Commit(
                ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber,
                Common::TimeSpan const timeout = Common::TimeSpan::MaxValue);

            virtual void Rollback();
            Common::ErrorCode SetEseError(JET_ERR error);
            Common::ErrorCode SetError(Common::ErrorCode errorCode) { error_ = errorCode; return error_; }
            void OnConstructEnumeration() const { ++enumerationCount_; }
            void OnDestructEnumeration() const { --enumerationCount_; }

        private:
            Common::ErrorCode TryGetPoolItem(__out EsePoolItemSPtr &);
            EsePoolItemSPtr TryTakePoolItem();

            Common::AsyncOperationSPtr BeginCommit(
                bool lazyCommit,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & state);

            EseLocalStore & owner_;
            EsePoolSPtr const esePool_;
            EsePoolItemSPtr esePoolItem_;
            Common::RwLock esePoolItemLock_;

            Common::ErrorCode error_;
            Common::atomic_bool active_;
            bool rolledback_;
            mutable int enumerationCount_;
            TransactionInstanceId traceId_;
            Common::Stopwatch lifetimeStopwatch_;
        };

        typedef std::shared_ptr<EseLocalStore::Transaction> EseStoreTransactionSPtr;
        typedef std::weak_ptr<EseLocalStore::Transaction> EseStoreTransactionWPtr;

        class Enumeration : public EnumerationBase
        {
            DENY_COPY(Enumeration);
        public:
            virtual ~Enumeration();

            __declspec(property(get=get_LocalStore)) EseLocalStore & LocalStore;
            EseLocalStore & get_LocalStore() const { return owner_; }

            virtual Common::ErrorCode MoveNext() override;
            virtual Common::ErrorCode CurrentOperationLSN(/*out*/_int64 & sequenceNumber) override;
            virtual Common::ErrorCode CurrentLastModifiedFILETIME(/*out*/FILETIME & fileTime) override;
            virtual Common::ErrorCode CurrentLastModifiedOnPrimaryFILETIME(/*out*/FILETIME & fileTime) override;
            virtual Common::ErrorCode CurrentType(/*out*/std::wstring & buffer) override;
            virtual Common::ErrorCode CurrentKey(/*out*/std::wstring & buffer) override;
            virtual Common::ErrorCode CurrentValue(/*out*/std::vector<unsigned _int8> & buffer) override;
            virtual Common::ErrorCode CurrentValueSize(/*out*/size_t & size) override;

            Common::ErrorCode SetError(Common::ErrorCode errorCode) { error_ = errorCode; return error_; }
            Common::ErrorCode SetEseError(JET_ERR error) { return SetError(owner_.JetToErrorCode(error)); }

        protected:
            Enumeration(
                    EseLocalStore &, 
                    EseStoreTransactionSPtr const &);

            Enumeration(
                EseLocalStore &,
                EseStoreTransactionSPtr const &, 
                Common::ErrorCodeValue::Enum const);

            __declspec(property(get=get_Cursor)) EseCursor & Cursor;
            __declspec(property(get=get_TraceId)) EnumerationInstanceId TraceId;
            __declspec(property(get=get_TransactionTraceId)) TransactionInstanceId const & TransactionTraceId;

            EseCursor & get_Cursor() { return *cursor_; }
            EnumerationInstanceId get_TraceId() const { return traceId_; }
            TransactionInstanceId const & get_TransactionTraceId() const { return txTraceId_; }

            virtual JET_ERR InitializeCursorAndSeekFirstItem() = 0;

            Common::ErrorCode MoveNext(__out EseStoreTransactionSPtr &);

        private:

            Common::ErrorCode TryGetTxSPtr(__out EseStoreTransactionSPtr &);
            Common::ErrorCode CheckState(__out EseStoreTransactionSPtr &);

            EseLocalStore & owner_;
            EseStoreTransactionWPtr txWPtr_;
            std::shared_ptr<EseCursor> cursor_;
            Common::ErrorCode error_;
            int offset_;
            EnumerationInstanceId traceId_;
            TransactionInstanceId txTraceId_;
        };

        class KeyEnumeration : public Enumeration
        {
        public:
            KeyEnumeration(
                EseLocalStore &,
                EseStoreTransactionSPtr const &,
                std::wstring const & type,
                std::wstring const & keyStart,
                bool strictType);

            virtual Common::ErrorCode MoveNext() override;

        protected:
            virtual JET_ERR InitializeCursorAndSeekFirstItem();

        private:
            std::wstring type_;
            std::wstring keyStart_;
            std::wstring buffer_;
            bool strictType_;
        };

        class ChangeEnumeration : public Enumeration
        {
        public:
            ChangeEnumeration(
                EseLocalStore &,
                EseStoreTransactionSPtr const &,
                ::FABRIC_SEQUENCE_NUMBER fromOperationLSN);

        protected:
            virtual JET_ERR InitializeCursorAndSeekFirstItem();

        private:
            ::FABRIC_SEQUENCE_NUMBER fromOperationLSN_;
        };

    protected:
        friend class EsePool;

        std::unique_ptr<Common::ExclusiveFile> exclusiveFileLock_;
        EseInstanceSPtr instance_;
        EseSessionSPtr session_;
        EseDatabaseSPtr database_;
        EseLocalStoreSettingsSPtr settings_;
        std::wstring directory_;
        std::wstring const fileName_;
        std::wstring filePath_;

        JET_COLUMNID sequenceNumberColumnId_;
        JET_COLUMNID lastModifiedColumnId_;
        JET_COLUMNID lastModifiedOnPrimaryColumnId_;
        JET_COLUMNID typeColumnId_;
        JET_COLUMNID keyColumnId_;
        JET_COLUMNID valueColumnId_;
        JET_COLUMNID operationLSNColumnId_;
        RWLOCK(StoreEseLocalStore, lock_);
        mutable Common::atomic_long transactionCount_;
        volatile bool aborting_;
        volatile bool drained_;
        // Transactions have a reference on the EseLocalStore object w/o controlling lifetime.
        // Destruction of the store would wait for the transaction objects to be destructed.
        std::shared_ptr<Common::AutoResetEvent> allStoreObjectsDestructedSPtr_;
        mutable std::wstring traceId_;
        LONG localStoreFlags_;

    private:
        friend class EseDefragmenter;

        EseStorePerformanceCountersSPtr perfCounters_;
        EseDefragmenterSPtr defragmenter_;
    };
}
