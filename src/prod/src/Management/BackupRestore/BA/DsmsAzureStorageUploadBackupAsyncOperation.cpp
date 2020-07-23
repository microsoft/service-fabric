// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

StringLiteral const TraceComponent("BackupCopierProxy");

DsmsAzureStorageUploadBackupAsyncOperation::DsmsAzureStorageUploadBackupAsyncOperation(
    __in BackupCopierProxy & owner,
    Common::ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    wstring const & storageCredentialsSourceLocation,
    wstring const & containerName,
    wstring const & backupStoreBaseFolderPath,
    wstring const & sourceFileOrFolderPath,
    wstring const & targetFolderPath,
    wstring const & backupMetadataFileName)
    : DsmsAzureStorageBackupCopierAsyncOperationBase(
        owner,
        activityId,
        timeout,
        callback,
        parent,
        storageCredentialsSourceLocation,
        containerName,
        backupStoreBaseFolderPath,
        sourceFileOrFolderPath,
        targetFolderPath)
    , backupMetadataFileName_(backupMetadataFileName)
{
}

ErrorCode DsmsAzureStorageUploadBackupAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    auto casted = AsyncOperation::End<DsmsAzureStorageUploadBackupAsyncOperation>(operation);

    return casted->Error;
}

void DsmsAzureStorageUploadBackupAsyncOperation::OnCompleted()
{
    __super::OnCompleted();
}

void DsmsAzureStorageUploadBackupAsyncOperation::OnProcessJob(AsyncOperationSPtr const & thisSPtr)
{
    this->DoUploadBackup(thisSPtr);
}


void DsmsAzureStorageUploadBackupAsyncOperation::DoUploadBackup(AsyncOperationSPtr const & thisSPtr)
{
    this->DoDataValidation(BackupCopierProxy::DsmsAzureBlobStoreUpload);
    ASSERT_IF(backupMetadataFileName_.empty(), "{0}:{1} - BackupMetadataFileName should not be empty", this->ActivityId, BackupCopierProxy::AzureBlobStoreUpload);

    this->Owner.WriteInfo(
        TraceComponent,
        "{0}+{1}: processing upload backup.",
        BackupCopierAsyncOperationBase::TraceId,
        this->ActivityId);

    wstring cmdLineArgs, cmdLineArgsLogString;

    this->InitializeAndPopulateCommandLineArguments(cmdLineArgs, cmdLineArgsLogString);

    this->Owner.AddCommandLineArgument(cmdLineArgs, cmdLineArgsLogString, BackupCopierProxy::BackupMetadataFilePathKeyName, backupMetadataFileName_);

    auto operation = this->Owner.BeginRunBackupCopierExe(
        this->ActivityId,
        BackupCopierProxy::DsmsAzureBlobStoreUpload,
        cmdLineArgs,
        cmdLineArgsLogString,
        this->Timeout,
        [this](AsyncOperationSPtr const & operation) { this->OnRunBackupCopierComplete(operation, false); },
        thisSPtr);

    this->OnRunBackupCopierComplete(operation, true);
}

void DsmsAzureStorageUploadBackupAsyncOperation::OnRunBackupCopierComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->Owner.EndRunBackupCopierExe(operation);

    this->TryComplete(thisSPtr, move(error));
}
