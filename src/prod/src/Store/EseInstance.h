// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class EseSession;
    class EseInstance;
    class EseCommitAsyncOperation;

    typedef std::shared_ptr<EseInstance> EseInstanceSPtr;
    typedef std::shared_ptr<EseCommitAsyncOperation> EseCommitAsyncOperationSPtr;

    class EseInstance
        : public std::enable_shared_from_this<EseInstance>,
          public Common::FabricComponent,
          public Common::TextTraceComponent<Common::TraceTaskCodes::EseStore>
    {
        DENY_COPY(EseInstance);
    private:
        EseInstance(
            EseLocalStoreSettingsSPtr const &,
            EseStorePerformanceCountersSPtr const &,
            std::wstring const & instanceName,
            std::wstring const & eseFilename,
            std::wstring const & eseDirectory,
            bool isRestoreValidation = false);
		
    public:
        static std::shared_ptr<EseInstance> CreateSPtr(
            EseLocalStoreSettingsSPtr const &,
            EseStorePerformanceCountersSPtr const &,
            std::wstring const & instanceName,
            std::wstring const & eseFilename,
            std::wstring const & eseDirectory,
            bool isRestoreValidation = false);

        ~EseInstance();

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return instanceName_; }

        __declspec(property(get=get_EseLocalStoreSettings)) EseLocalStoreSettings const & Settings;
        EseLocalStoreSettings const & get_EseLocalStoreSettings() const { return *settings_; }

        static std::wstring GetRestoreDirectoryName(std::wstring const & eseDirectory);
        static std::wstring GetLocalBackupDirectoryName(std::wstring const & eseDirectory);		

        JET_ERR GetSystemParameter(ULONG parameterId, __out size_t &);
        Common::ErrorCode GetOpenFileSize(__out int64 &);

#if DBG
        void AssertSessionIsNotActive(EseSession const & session);
#endif // DBG
        
        JET_ERR Backup(std::wstring const & dir, LONG jetBackupOptions);

        JET_GRBIT GetOptions();

        Common::ErrorCode ForceMaxCacheSize();
        Common::ErrorCode ResetCacheSize();
        Common::ErrorCode JetToErrorCode(JET_ERR);
        bool ShouldAutoCompact(int64 fileSize);
        static bool ShouldAutoCompact(int64 fileSize, EseLocalStoreSettingsSPtr const &);

    protected:
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
        static Common::GlobalWString LocalBackupDirectorySuffix;
        static Common::GlobalWString RestoreDirectorySuffix;
        static Common::GlobalWString CompactionFileExtension;
		static Common::GlobalWString TempTruncateDirectoryPrefix;

        typedef std::vector<EseCommitAsyncOperationSPtr> EseCommitsVector;
        typedef std::map<__int64, EseCommitAsyncOperationSPtr> EseCommitsMap;

        friend class InstanceIdLock;
        friend class EseDatabase;
        friend class EseSession;
        friend class EseCommitAsyncOperation;

        class InstanceRef
        {
            DENY_COPY(InstanceRef);
        public:
            InstanceRef(EseInstance & instance);
            ~InstanceRef();

            __declspec (property(get=get_InstanceId)) JET_INSTANCE InstanceId;
            __declspec (property(get=get_IsValid)) bool IsValid;

            bool get_IsValid() const { return locked_; }

            JET_INSTANCE get_InstanceId() const { return instance_.instanceId_; }

        private:
            EseInstance & instance_;
            bool locked_;
        };

        JET_ERR AttachDatabase(
            EseSession const & session,
            std::wstring const & filename,
            bool callJetAttach,
            JET_GRBIT flags = 0);

        JET_ERR DetachDatabase(
            EseSession const & session,
            std::wstring const & filename);

        JET_ERR AddActiveSession(
            std::shared_ptr<EseSession> const & session);

        void RemoveActiveSession(
            EseSession const & session);

        void AbortActiveSessions();

        Common::ErrorCode TerminateWithRetries();

        JET_ERR SetSystemParameter(
            ULONG parameterId,
            JET_API_PTR lParam);

        JET_ERR SetSystemParameter(
            ULONG parameterId,
            std::wstring const & value);

        JET_API_PTR ConvertToDatabasePages(int cacheSizeInMB);

        bool TryAddPendingCommit(EseCommitAsyncOperationSPtr const &);
        bool CommitCompleted(__int64 commitId);

        void TryCompletePendingCommits(EseCommitsVector const & completedCommits, Common::ErrorCode const error);

        void OnCommitComplete(__int64 nextBatchStartCommitId, Common::ErrorCode const error);
        void OnFatalCommitError(Common::ErrorCode const error);

        Common::ErrorCode DoCompaction();
        Common::ErrorCode PrepareForRestore(const std::wstring & localBackupDirectory);
        Common::ErrorCode SetOpeningParameters();
        Common::ErrorCode DoRestore(const std::wstring & localBackupDirectory);
        Common::ErrorCode PrepareDatabaseForUse();

        Common::ErrorCode ComputeDatabasePageSize();
        Common::ErrorCode ComputeLogFileSize();

        static JET_ERR CommitCallback(
            JET_INSTANCE instance,
            JET_COMMIT_ID *pCommitIdSeen,
            JET_GRBIT grbit);

		static std::wstring GetTempTruncateDirectoryName(std::wstring const & eseDirectory);
		JET_ERR DeleteDirectoryForTruncation(std::wstring const & directory, std::wstring const & truncationStage);

#if DBG
        bool IsSessionActive(EseSession const & session);
#endif

        JET_INSTANCE instanceId_;
        RWLOCK(StoreEseInstance, thisLock_);

        // Avoid references - ESE commit completion posts to the threadpool
        //
        EseLocalStoreSettingsSPtr settings_;
        EseStorePerformanceCountersSPtr perfCounters_;
        std::wstring instanceName_;
        std::wstring eseFilename_;
        std::wstring eseDirectory_;
        std::wstring restoreDirectory_;
        bool isRestoreValidation_;

        int64 openFileSize_;
        std::map<std::wstring,int> attachedDatabases_;
        std::unordered_map<JET_SESID, std::shared_ptr<EseSession>> activeSessions_;
        LONG instanceIdRef_;
        volatile bool aborting_;
        volatile bool isHealthy_;
        Common::AutoResetEvent allEseCallsCompleted_;
        Common::AutoResetEvent allEseCallbacksCompleted_;

        EseCommitsMap pendingCommits_;
        __int64 nextBatchStartCommitId_;

        typedef std::unordered_map<JET_INSTANCE, EseInstanceSPtr> InstanceMap;

        static Common::Global<Common::RwLock> sLock_;
        static Common::Global<InstanceMap> eseInstances_;

        JET_API_PTR databasePageSize_;
        JET_API_PTR logFileSizeInKB_;
        JET_API_PTR minCacheSize_;
    };
}
