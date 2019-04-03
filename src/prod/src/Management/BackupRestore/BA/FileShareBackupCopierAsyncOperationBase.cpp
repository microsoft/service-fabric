// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Base64Encoder.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

FileShareBackupCopierAsyncOperationBase::FileShareBackupCopierAsyncOperationBase(
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
    : BackupCopierAsyncOperationBase(
        owner,
        activityId,
        timeout,
        callback,
        parent)
    , accessType_(accessType)
    , isPasswordEncrypted_(isPasswordEncrypted)
    , fileSharePath_(fileSharePath)
    , primaryUserName_(primaryUserName)
    , primaryPassword_(primaryPassword)
    , secondaryUserName_(secondaryUserName)
    , secondaryPassword_(secondaryPassword)
    , sourceFileOrFolderPath_(sourceFileOrFolderPath)
    , targetFolderPath_(targetFolderPath)
{
}

void FileShareBackupCopierAsyncOperationBase::DoDataValidation(wstring const & operationName)
{
    ASSERT_IF(fileSharePath_.empty(), "{0}:{1} - FileSharePath should not be empty", this->ActivityId, operationName);
    ASSERT_IF(sourceFileOrFolderPath_.empty(), "{0}:{1} - SourceFileOrFolderPath should not be empty", this->ActivityId, operationName);
}

void FileShareBackupCopierAsyncOperationBase::InitializeAndPopulateCommandLineArguments(wstring &cmdLineArgs, wstring &cmdLineArgsLogString)
{
    this->Owner.InitializeCommandLineArguments(cmdLineArgs, cmdLineArgsLogString);

    this->Owner.AddCommandLineArgument(
        cmdLineArgs,
        cmdLineArgsLogString,
        BackupCopierProxy::FileShareAccessTypeKeyName,
        (this->AccessType == BackupStoreFileShareAccessType::Enum::DomainUser) ? BackupCopierProxy::FileShareAccessTypeValue_DomainUser : BackupCopierProxy::FileShareAccessTypeValue_None);

    this->Owner.AddCommandLineArgument(
        cmdLineArgs,
        cmdLineArgsLogString,
        BackupCopierProxy::IsPasswordEncryptedKeyName,
        this->IsPasswordEncrypted ? BackupCopierProxy::BooleanStringValue_True : BackupCopierProxy::BooleanStringValue_False);

    this->Owner.AddCommandLineArgument(cmdLineArgs, cmdLineArgsLogString, BackupCopierProxy::FileSharePathKeyName, this->FileSharePath);
    
    
    if ((this->PrimaryUserName.length() > 0) && (this->PrimaryPassword.length() > 0))
    {
        this->Owner.AddCommandLineArgument(cmdLineArgs, cmdLineArgsLogString, BackupCopierProxy::PrimaryUserNameKeyName, this->PrimaryUserName);
        this->Owner.AddCommandLineArgument(
            cmdLineArgs,
            cmdLineArgsLogString,
            BackupCopierProxy::PrimaryPasswordKeyName,
            Base64Encoder::ConvertToUTF8AndEncodeW(this->PrimaryPassword));
    }

    if ((this->SecondaryUserName.length() > 0) && (this->SecondaryPassword.length() > 0))
    {
        this->Owner.AddCommandLineArgument(cmdLineArgs, cmdLineArgsLogString, BackupCopierProxy::SecondaryUserNameKeyName, this->SecondaryUserName);
        this->Owner.AddCommandLineArgument(
            cmdLineArgs,
            cmdLineArgsLogString,
            BackupCopierProxy::SecondaryPasswordKeyName,
            Base64Encoder::ConvertToUTF8AndEncodeW(this->SecondaryPassword));
    }

    this->Owner.AddCommandLineArgument(cmdLineArgs, cmdLineArgsLogString, BackupCopierProxy::SourceFileOrFolderPathKeyName, this->SourceFileOrFolderPath);
    this->Owner.AddCommandLineArgument(cmdLineArgs, cmdLineArgsLogString, BackupCopierProxy::TargetFolderPathKeyName, this->TargetFolderPath);
}
