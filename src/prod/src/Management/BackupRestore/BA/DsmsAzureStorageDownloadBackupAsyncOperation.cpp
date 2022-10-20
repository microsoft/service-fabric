// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

StringLiteral const TraceComponent("BackupCopierProxy");

DsmsAzureStorageDownloadBackupAsyncOperation::DsmsAzureStorageDownloadBackupAsyncOperation(
    __in BackupCopierProxy & owner,
    Common::ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    wstring const & storageCredentialsSourceLocation,
    wstring const & containerName,
    wstring const & backupStoreBaseFolderPath,
    wstring const & sourceFileOrFolderPath,
    wstring const & targetFolderPath)
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
{
}

ErrorCode DsmsAzureStorageDownloadBackupAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    auto casted = AsyncOperation::End<DsmsAzureStorageDownloadBackupAsyncOperation>(operation);

    return casted->Error;
}

void DsmsAzureStorageDownloadBackupAsyncOperation::OnCompleted()
{
    __super::OnCompleted();
}

void DsmsAzureStorageDownloadBackupAsyncOperation::OnProcessJob(AsyncOperationSPtr const & thisSPtr)
{
    this->DoDownloadBackup(thisSPtr);
}


void DsmsAzureStorageDownloadBackupAsyncOperation::DoDownloadBackup(AsyncOperationSPtr const & thisSPtr)
{
    this->DoDataValidation(BackupCopierProxy::DsmsAzureBlobStoreDownload);

    this->Owner.WriteInfo(
        TraceComponent,
        "{0}+{1}: processing download backup.",
        BackupCopierAsyncOperationBase::TraceId,
        this->ActivityId);

    wstring cmdLineArgs, cmdLineArgsLogString;

    this->InitializeAndPopulateCommandLineArguments(cmdLineArgs, cmdLineArgsLogString);

    auto operation = this->Owner.BeginRunBackupCopierExe(
        this->ActivityId,
        BackupCopierProxy::DsmsAzureBlobStoreDownload,
        cmdLineArgs,
        cmdLineArgsLogString,
        this->Timeout,
        [this](AsyncOperationSPtr const & operation) { this->OnRunBackupCopierComplete(operation, false); },
        thisSPtr);

    this->OnRunBackupCopierComplete(operation, true);
}

void DsmsAzureStorageDownloadBackupAsyncOperation::OnRunBackupCopierComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->Owner.EndRunBackupCopierExe(operation);

    this->TryComplete(thisSPtr, move(error));
}
