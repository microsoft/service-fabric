// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

StringLiteral const TraceComponent("BackupCopierProxy");

FileShareDownloadBackupAsyncOperation::FileShareDownloadBackupAsyncOperation(
    __in BackupCopierProxy & owner,
    Common::ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    BackupStoreFileShareAccessType::Enum const & accessType,
    bool isPasswordEncrypted,
    wstring const & fileSharePath,
    wstring const & primaryUserName,
    wstring const & primaryPassword,
    wstring const & secondaryUserName,
    wstring const & secondaryPassword,
    wstring const & sourceFileOrFolderPath,
    wstring const & targetFolderPath)
    : FileShareBackupCopierAsyncOperationBase(
        owner,
        activityId,
        timeout,
        callback,
        parent,
        accessType,
        isPasswordEncrypted,
        fileSharePath,
        primaryUserName,
        primaryPassword,
        secondaryUserName,
        secondaryPassword,
        sourceFileOrFolderPath,
        targetFolderPath)
{
}

ErrorCode FileShareDownloadBackupAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    auto casted = AsyncOperation::End<FileShareDownloadBackupAsyncOperation>(operation);

    return casted->Error;
}

void FileShareDownloadBackupAsyncOperation::OnCompleted()
{
    __super::OnCompleted();
}

void FileShareDownloadBackupAsyncOperation::OnProcessJob(AsyncOperationSPtr const & thisSPtr)
{
    this->DoDownloadBackup(thisSPtr);
}

void FileShareDownloadBackupAsyncOperation::DoDownloadBackup(AsyncOperationSPtr const & thisSPtr)
{
    this->DoDataValidation(BackupCopierProxy::FileShareDownload);

    this->Owner.WriteInfo(
        TraceComponent,
        "{0}+{1}: processing download backup.",
        BackupCopierAsyncOperationBase::TraceId,
        this->ActivityId);

    wstring cmdLineArgs, cmdLineArgsLogString;

    this->InitializeAndPopulateCommandLineArguments(cmdLineArgs, cmdLineArgsLogString);

    auto operation = this->Owner.BeginRunBackupCopierExe(
        this->ActivityId,
        BackupCopierProxy::FileShareDownload,
        cmdLineArgs,
        cmdLineArgsLogString,
        this->Timeout,
        [this](AsyncOperationSPtr const & operation) { this->OnRunBackupCopierComplete(operation, false); },
        thisSPtr);

    this->OnRunBackupCopierComplete(operation, true);
}

void FileShareDownloadBackupAsyncOperation::OnRunBackupCopierComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->Owner.EndRunBackupCopierExe(operation);

    this->TryComplete(thisSPtr, move(error));
}
