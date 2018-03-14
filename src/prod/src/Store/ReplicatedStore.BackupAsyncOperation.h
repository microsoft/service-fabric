// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::BackupAsyncOperation
        : public Common::AsyncOperation
        , public Common::TextTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY(BackupAsyncOperation);
        
    public:
        BackupAsyncOperation(
            __in ReplicatedStore & owner,            
            __in Common::ErrorCode createError,
            __in std::wstring const & backupDir,
            __in StoreBackupOption::Enum backupOption,
            __in Api::IStorePostBackupHandlerPtr const & postBackupHandler,
            __in StoreBackupRequester::Enum backupRequester,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent)
            : Common::AsyncOperation(callback, parent, true)
            , owner_(owner)            
            , createError_(createError)
            , backupDir_(backupDir)
            , backupOption_(backupOption)
            , postBackupHandler_(postBackupHandler)     
            , backupRequester_(backupRequester)
            , activeBackupState_()
            , backupChainGuid_(Common::Guid::Empty())
            , backupIndex_(0)
            , backupInfo_()
            , backupInfoEx1_()
        {
        }

        virtual ~BackupAsyncOperation()
        {
        }

        static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

    protected:
        void OnStart(Common::AsyncOperationSPtr const & proxySPtr) override;
        void OnCompleted() override;

    private:        
        void InvokeUserPostBackupHandler(Common::AsyncOperationSPtr const & operation);

        void OnUserPostBackupCompleted(__in Common::AsyncOperationSPtr const & operation);
        
        void DoBackup(Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode DisableIncrementalBackup();
        Common::ErrorCode EnableIncrementalBackup();
        Common::ErrorCode PrepareForPostBackup(__out bool & invokeUserPostBackupHandler);

        ReplicatedStore & owner_;
        Common::ErrorCode createError_;
        std::wstring backupDir_;
        StoreBackupOption::Enum backupOption_;
        StoreBackupRequester::Enum backupRequester_;
        Api::IStorePostBackupHandlerPtr postBackupHandler_;        
        ScopedActiveBackupStateSPtr activeBackupState_;
        Common::Guid backupChainGuid_;
        ULONG backupIndex_;
        ::FABRIC_STORE_BACKUP_INFO backupInfo_;
        ::FABRIC_STORE_BACKUP_INFO_EX1 backupInfoEx1_;
    };
}
