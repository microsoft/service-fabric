// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

StringLiteral const TraceComponent("BackupCopierProxy");

AzureStorageDownloadBackupAsyncOperation::AzureStorageDownloadBackupAsyncOperation(
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
    wstring const & targetFolderPath)
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
{
}

ErrorCode AzureStorageDownloadBackupAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    auto casted = AsyncOperation::End<AzureStorageDownloadBackupAsyncOperation>(operation);

    return casted->Error;
}

void AzureStorageDownloadBackupAsyncOperation::OnCompleted()
{
    __super::OnCompleted();
}

void AzureStorageDownloadBackupAsyncOperation::OnProcessJob(AsyncOperationSPtr const & thisSPtr)
{
    this->DoDownloadBackup(thisSPtr);
}


void AzureStorageDownloadBackupAsyncOperation::DoDownloadBackup(AsyncOperationSPtr const & thisSPtr)
{
    this->DoDataValidation(BackupCopierProxy::AzureBlobStoreDownload);

    this->Owner.WriteInfo(
        TraceComponent,
        "{0}+{1}: processing download backup.",
        BackupCopierAsyncOperationBase::TraceId,
        this->ActivityId);

    wstring cmdLineArgs, cmdLineArgsLogString;

    this->InitializeAndPopulateCommandLineArguments(cmdLineArgs, cmdLineArgsLogString);

    auto operation = this->Owner.BeginRunBackupCopierExe(
        this->ActivityId,
        BackupCopierProxy::AzureBlobStoreDownload,
        cmdLineArgs,
        cmdLineArgsLogString,
        this->Timeout,
        [this](AsyncOperationSPtr const & operation) { this->OnRunBackupCopierComplete(operation, false); },
        thisSPtr);

    this->OnRunBackupCopierComplete(operation, true);
}

void AzureStorageDownloadBackupAsyncOperation::OnRunBackupCopierComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->Owner.EndRunBackupCopierExe(operation);

    this->TryComplete(thisSPtr, move(error));
}
