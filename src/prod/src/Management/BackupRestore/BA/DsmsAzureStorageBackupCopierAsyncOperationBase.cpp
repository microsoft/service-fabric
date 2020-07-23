// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;


DsmsAzureStorageBackupCopierAsyncOperationBase::DsmsAzureStorageBackupCopierAsyncOperationBase(
    __in BackupCopierProxy & owner,
    Common::ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    wstring const & storageCredentialsSourceLocation,
    wstring const & containerName,
    wstring const & targetBaseFolderPath,
    wstring const & sourceFileOrFolderPath,
    wstring const & targetFolderPath)
    : BackupCopierAsyncOperationBase(
        owner,
        activityId,
        timeout,
        callback,
        parent)
    , storageCredentialsSourceLocation_(storageCredentialsSourceLocation)
    , containerName_(containerName)
    , backupStoreBaseFolderPath_(targetBaseFolderPath)
    , sourceFileOrFolderPath_(sourceFileOrFolderPath)
    , targetFolderPath_(targetFolderPath)
{
}

void DsmsAzureStorageBackupCopierAsyncOperationBase::DoDataValidation(wstring const & operationName)
{
    ASSERT_IF(storageCredentialsSourceLocation_.empty(), "{0}:{1} - StorageCredentialsSourceLocation should not be empty", this->ActivityId, operationName);
    ASSERT_IF(containerName_.empty(), "{0}:{1} - ContainerName should not be empty", this->ActivityId, operationName);
    ASSERT_IF(sourceFileOrFolderPath_.empty(), "{0}:{1} - SourceFileOrFolderPath should not be empty", this->ActivityId, operationName);
}

void DsmsAzureStorageBackupCopierAsyncOperationBase::InitializeAndPopulateCommandLineArguments(wstring &cmdLineArgs, wstring &cmdLineArgsLogString)
{
    this->Owner.InitializeCommandLineArguments(cmdLineArgs, cmdLineArgsLogString);

    this->Owner.AddCommandLineArgument(cmdLineArgs, cmdLineArgsLogString, BackupCopierProxy::StorageCredentialsSourceLocationKeyName, this->StorageCredentialsSourceLocation);

    this->Owner.AddCommandLineArgument(cmdLineArgs, cmdLineArgsLogString, BackupCopierProxy::ContainerNameKeyName, this->ContainerName);
    this->Owner.AddCommandLineArgument(cmdLineArgs, cmdLineArgsLogString, BackupCopierProxy::BackupStoreBaseFolderPathKeyName, this->BackupStoreBaseFolderPath);

    this->Owner.AddCommandLineArgument(cmdLineArgs, cmdLineArgsLogString, BackupCopierProxy::SourceFileOrFolderPathKeyName, this->SourceFileOrFolderPath);
    this->Owner.AddCommandLineArgument(cmdLineArgs, cmdLineArgsLogString, BackupCopierProxy::TargetFolderPathKeyName, this->TargetFolderPath);
}
