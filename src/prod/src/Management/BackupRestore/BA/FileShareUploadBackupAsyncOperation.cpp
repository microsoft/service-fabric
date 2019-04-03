// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

StringLiteral const TraceComponent("BackupCopierProxy");

FileShareUploadBackupAsyncOperation::FileShareUploadBackupAsyncOperation(
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
    wstring const & targetFolderPath,
    wstring const & backupMetadataFileName)
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
    , backupMetadataFileName_(backupMetadataFileName)
{
}

ErrorCode FileShareUploadBackupAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    auto casted = AsyncOperation::End<FileShareUploadBackupAsyncOperation>(operation);

    return casted->Error;
}

void FileShareUploadBackupAsyncOperation::OnCompleted()
{
    __super::OnCompleted();
}

void FileShareUploadBackupAsyncOperation::OnProcessJob(AsyncOperationSPtr const & thisSPtr)
{
    this->DoUploadBackup(thisSPtr);
}


void FileShareUploadBackupAsyncOperation::DoUploadBackup(AsyncOperationSPtr const & thisSPtr)
{
    this->DoDataValidation(BackupCopierProxy::FileShareUpload);
    ASSERT_IF(backupMetadataFileName_.empty(), "{0}:{1} - BackupMetadataFileName should not be empty", this->ActivityId, BackupCopierProxy::FileShareUpload);

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
        BackupCopierProxy::FileShareUpload,
        cmdLineArgs,
        cmdLineArgsLogString,
        this->Timeout,
        [this](AsyncOperationSPtr const & operation) { this->OnRunBackupCopierComplete(operation, false); },
        thisSPtr);

    this->OnRunBackupCopierComplete(operation, true);
}

void FileShareUploadBackupAsyncOperation::OnRunBackupCopierComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->Owner.EndRunBackupCopierExe(operation);

    this->TryComplete(thisSPtr, move(error));
}
