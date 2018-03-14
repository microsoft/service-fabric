// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace Store
{
    StringLiteral const TraceComponent("BackupAsyncOperation");

    ErrorCode ReplicatedStore::BackupAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<BackupAsyncOperation>(operation)->Error;
    }

    void ReplicatedStore::BackupAsyncOperation::OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        if (createError_.IsSuccess() && !owner_.TryAcquireActiveBackupState(backupRequester_, activeBackupState_))
        {
            auto errMsg = GET_STORE_RC(BackupInProgress_By_User);
            
            if (activeBackupState_->IsOwnedByLogTruncation)
            {
                errMsg = GET_STORE_RC(BackupInProgress_By_LogTruncation);
            }

            if (activeBackupState_->IsOwnedBySystem)
            {
                errMsg = GET_STORE_RC(BackupInProgress_By_System);
            }

            createError_ = ErrorCode(ErrorCodeValue::BackupInProgress, std::move(errMsg));
        }

        if (createError_.IsSuccess())
        {
            Threadpool::Post([this, proxySPtr]() { DoBackup(proxySPtr); });
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} ReplicatedStore::BackupAsyncOperation::OnStart() not proceeding with backup due to error: {1}",
                owner_.TraceId,
                createError_);

            this->TryComplete(proxySPtr, createError_);
        }
    }

    void ReplicatedStore::BackupAsyncOperation::OnCompleted()
    {
        owner_.ArmLogTruncationTimer();
        activeBackupState_.reset();        

        WriteInfo(
            TraceComponent,
            "{0} ReplicatedStore::BackupAsyncOperation::OnCompleted() completed. Invoking OnCompleted() in base class.",
            owner_.TraceId);

        __super::OnCompleted();
    }

    void ReplicatedStore::BackupAsyncOperation::DoBackup(AsyncOperationSPtr const & operation)
    {                
        bool invokeUserPostBackupHandler = false;

        auto error = owner_.BackupLocalImpl(backupDir_, backupOption_, backupChainGuid_, backupIndex_);

        if (error.IsSuccess())
        {
            error = PrepareForPostBackup(invokeUserPostBackupHandler);
        }

        if (error.IsSuccess() && invokeUserPostBackupHandler)
        {
            InvokeUserPostBackupHandler(operation);
        }
        else
        {
            TryComplete(operation, error);
        }

        WriteInfo(
            TraceComponent,
            "{0} ReplicatedStore::BackupAsyncOperation::DoBackup() completed: error = {1}",
            owner_.TraceId,
            error);
    }

    ErrorCode ReplicatedStore::BackupAsyncOperation::PrepareForPostBackup(__out bool & invokeUserPostBackupHandler)
    {
        ErrorCode error = ErrorCodeValue::Success;
        invokeUserPostBackupHandler = false;

        if (backupOption_ == StoreBackupOption::TruncateLogsOnly)
        {
            WriteInfo(
                TraceComponent,
                "{0} ReplicatedStore::BackupAsyncOperation::InvokePostBackupActions() post backup operation will not be invoked since only logs are truncated.",
                owner_.TraceId);

            error = this->DisableIncrementalBackup();
            return error;
        }

        if (!postBackupHandler_.get())
        {
            WriteInfo(
                TraceComponent,
                "{0} ReplicatedStore::BackupAsyncOperation::InvokePostBackupActions() post-backup handler not present, hence dis-allowing incremental backup next time",
                owner_.TraceId);

            error = this->DisableIncrementalBackup();
            return error;
        }
        
        backupInfoEx1_.BackupChainId = backupChainGuid_.AsGUID();
        backupInfoEx1_.BackupIndex = backupIndex_;

        backupInfo_.BackupFolder = backupDir_.c_str();
        error = StoreBackupOption::ToPublicApi(backupOption_, backupInfo_.BackupOption);

        if (!error.IsSuccess())
        {
            WriteError(
                TraceComponent,
                "{0} ReplicatedStore::BackupAsyncOperation::InvokePostBackupActions() failed: error = {1}",
                owner_.TraceId,
                error);

            error = this->DisableIncrementalBackup();
            return error;            
        }

        backupInfo_.Reserved = &backupInfoEx1_;

        invokeUserPostBackupHandler = true;
        return error;
    }

    void ReplicatedStore::BackupAsyncOperation::InvokeUserPostBackupHandler(__in AsyncOperationSPtr const & proxySPtr)
    {
        if (!postBackupHandler_.get()) { return; }

        postBackupHandler_->BeginPostBackup(
            this->backupInfo_,
            [this](AsyncOperationSPtr const & operation)
            {
                OnUserPostBackupCompleted(operation);
            },
            proxySPtr);
    }

    void ReplicatedStore::BackupAsyncOperation::OnUserPostBackupCompleted(AsyncOperationSPtr const & operation)
    {
        WriteInfo(
            TraceComponent,
            "{0} ReplicatedStore::BackupAsyncOperation::OnUserPostBackupCompleted() post-backup by user completed. Invoking EndPostBackup()",
            owner_.TraceId);

        bool status = false;
        ErrorCode error = postBackupHandler_->EndPostBackup(operation, status);

        if (!error.IsSuccess())
        {
            error = this->DisableIncrementalBackup();

            WriteWarning(
                TraceComponent,
                "{0} ReplicatedStore::BackupAsyncOperation::OnUserPostBackupCompleted() end post-backup operation failed: error = {1}",
                owner_.TraceId,
                error);
        }
        else
        {
            error = status ? this->EnableIncrementalBackup() : this->DisableIncrementalBackup();
        }

        TryComplete(operation->Parent, error);
    }

    ErrorCode ReplicatedStore::BackupAsyncOperation::DisableIncrementalBackup()
    {
        ErrorCode error = owner_.DisableIncrementalBackup();

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} ReplicatedStore::BackupAsyncOperation::AllowIncrementalBackup() failed to disallow incremental backup the next time",
                owner_.TraceId);
        }

        return error;
    }

    ErrorCode  ReplicatedStore::BackupAsyncOperation::EnableIncrementalBackup()
    {
        ErrorCode error = owner_.SetIncrementalBackupState(backupChainGuid_, backupIndex_);
        
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} ReplicatedStore::BackupAsyncOperation::AllowIncrementalBackup() failed to allow incremental backup the next time",
                owner_.TraceId);
        }

        return error;
    }
}
