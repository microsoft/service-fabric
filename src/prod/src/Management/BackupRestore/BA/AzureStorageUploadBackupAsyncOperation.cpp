// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

StringLiteral const TraceComponent("BackupCopierProxy");

AzureStorageUploadBackupAsyncOperation::AzureStorageUploadBackupAsyncOperation(
    __in BackupCopierProxy & owner,
    Common::ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    wstring const & connectionString,
    bool isConnectionStringEncrypted,
    wstring const & containerName,
    wstring const & backupStoreBaseFolderPath,
    wstring const & sourceFileOrFolderPath,
    wstring const & targetFolderPath,
    wstring const & backupMetadataFileName)
    : AzureStorageBackupCopierAsyncOperationBase(
        owner,
        activityId,
        timeout,
        callback,
        parent,
        connectionString,
        isConnectionStringEncrypted,
        containerName,
        backupStoreBaseFolderPath,
        sourceFileOrFolderPath,
        targetFolderPath)
    , backupMetadataFileName_(backupMetadataFileName)
{
}

ErrorCode AzureStorageUploadBackupAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    auto casted = AsyncOperation::End<AzureStorageUploadBackupAsyncOperation>(operation);

    return casted->Error;
}

void AzureStorageUploadBackupAsyncOperation::OnCompleted()
{
    __super::OnCompleted();
}

void AzureStorageUploadBackupAsyncOperation::OnProcessJob(AsyncOperationSPtr const & thisSPtr)
{
    this->DoUploadBackup(thisSPtr);
}


void AzureStorageUploadBackupAsyncOperation::DoUploadBackup(AsyncOperationSPtr const & thisSPtr)
{
    this->DoDataValidation(BackupCopierProxy::AzureBlobStoreUpload);
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
        BackupCopierProxy::AzureBlobStoreUpload,
        cmdLineArgs,
        cmdLineArgsLogString,
        this->Timeout,
        [this](AsyncOperationSPtr const & operation) { this->OnRunBackupCopierComplete(operation, false); },
        thisSPtr);

    this->OnRunBackupCopierComplete(operation, true);
}

void AzureStorageUploadBackupAsyncOperation::OnRunBackupCopierComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->Owner.EndRunBackupCopierExe(operation);

    this->TryComplete(thisSPtr, move(error));
}
