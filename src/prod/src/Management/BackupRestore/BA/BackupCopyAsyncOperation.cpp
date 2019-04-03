// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

using namespace Common;
using namespace Transport;
using namespace Management;
using namespace BackupRestoreAgentComponent;

StringLiteral const TraceComponent("BackupCopyAsyncOperation");

BackupCopyAsyncOperation::BackupCopyAsyncOperation(
    MessageUPtr && message,
    IpcReceiverContextUPtr && receiverContext,
    BackupCopierProxySPtr backupCopierProxy,
    bool isUpload,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : RequestResponseAsyncOperationBase(move(message), timeout, callback, parent)
    , receiverContext_(move(receiverContext))
    , backupCopierProxy_(backupCopierProxy)
    , isUpload_(isUpload)
    , backupFileOrFolderCount_(1)
    , completedBackupFileOrFolderCount_(0)
    , lastSeenError_(Common::ErrorCodeValue::Success)
    , completedBackupCounterLock_()
{
}

BackupCopyAsyncOperation::~BackupCopyAsyncOperation()
{
}

void BackupCopyAsyncOperation::OnStart(Common::AsyncOperationSPtr const & thisSPtr)
{
    RequestResponseAsyncOperationBase::OnStart(thisSPtr);

    if (isUpload_)
    {
        WriteInfo(TraceComponent, "{0}: Starting backup upload operation", this->ActivityId);

        // Get the message body.
        UploadBackupMessageBodySPtr uploadBackupMessageSPtr = std::make_shared<UploadBackupMessageBody>();
        receivedMessage_->GetBody<UploadBackupMessageBody>(*uploadBackupMessageSPtr);

        switch (uploadBackupMessageSPtr->StoreInfo.StoreType)
        {
        case BackupStoreType::AzureStore:
            {
                WriteInfo(TraceComponent, "{0}: Starting Azure storage upload", this->ActivityId);

                auto uploadOperation = backupCopierProxy_->BeginUploadBackupToAzureStorage(
                    this->ActivityId,
                    this->RemainingTime,
                    [this](AsyncOperationSPtr const & operation)
                    {
                        this->OnUploadBackupCompleted(operation, BackupStoreType::AzureStore, false);
                    },
                    thisSPtr,
                    uploadBackupMessageSPtr->StoreInfo.AzureStorageInfo.ConnectionString,
                    uploadBackupMessageSPtr->StoreInfo.AzureStorageInfo.IsConnectionStringEncrypted,
                    uploadBackupMessageSPtr->StoreInfo.AzureStorageInfo.ContainerName,
                    uploadBackupMessageSPtr->StoreInfo.AzureStorageInfo.FolderPath,
                    uploadBackupMessageSPtr->LocalBackupPath,
                    uploadBackupMessageSPtr->DestinationFolderName,
                    uploadBackupMessageSPtr->BackupMetadataFilePath
                    );

                this->OnUploadBackupCompleted(uploadOperation, BackupStoreType::AzureStore, true);
            }
            break;

        case BackupStoreType::FileShare:
            {
                WriteInfo(TraceComponent, "{0}: Starting File share upload", this->ActivityId);

                auto uploadOperation = backupCopierProxy_->BeginUploadBackupToFileShare(
                    this->ActivityId,
                    this->RemainingTime,
                    [this](AsyncOperationSPtr const & operation)
                    {
                        this->OnUploadBackupCompleted(operation, BackupStoreType::FileShare, false);
                    },
                    thisSPtr,
                    uploadBackupMessageSPtr->StoreInfo.FileShareInfo.AccessType,
                    uploadBackupMessageSPtr->StoreInfo.FileShareInfo.IsPasswordEncrypted,
                    uploadBackupMessageSPtr->StoreInfo.FileShareInfo.FileSharePath,
                    uploadBackupMessageSPtr->StoreInfo.FileShareInfo.PrimaryUserName,
                    uploadBackupMessageSPtr->StoreInfo.FileShareInfo.PrimaryPassword,
                    uploadBackupMessageSPtr->StoreInfo.FileShareInfo.SecondaryUserName,
                    uploadBackupMessageSPtr->StoreInfo.FileShareInfo.SecondaryPassword,
                    uploadBackupMessageSPtr->LocalBackupPath,
                    uploadBackupMessageSPtr->DestinationFolderName,
                    uploadBackupMessageSPtr->BackupMetadataFilePath
                    );

                this->OnUploadBackupCompleted(uploadOperation, BackupStoreType::FileShare, true);
            }
            break;

        case BackupStoreType::DsmsAzureStore:
            {
                WriteInfo(TraceComponent, "{0}: Starting Dsms Azure storage upload", this->ActivityId);

                auto uploadOperation = backupCopierProxy_->BeginUploadBackupToDsmsAzureStorage(
                    this->ActivityId,
                    this->RemainingTime,
                    [this](AsyncOperationSPtr const & operation)
                {
                    this->OnUploadBackupCompleted(operation, BackupStoreType::DsmsAzureStore, false);
                },
                    thisSPtr,
                    uploadBackupMessageSPtr->StoreInfo.AzureStorageInfo.ConnectionString,
                    uploadBackupMessageSPtr->StoreInfo.AzureStorageInfo.ContainerName,
                    uploadBackupMessageSPtr->StoreInfo.AzureStorageInfo.FolderPath,
                    uploadBackupMessageSPtr->LocalBackupPath,
                    uploadBackupMessageSPtr->DestinationFolderName,
                    uploadBackupMessageSPtr->BackupMetadataFilePath
                    );

                this->OnUploadBackupCompleted(uploadOperation, BackupStoreType::DsmsAzureStore, true);
            }
            break;
        }
    }
    else
    {
        WriteInfo(TraceComponent, "{0}: Starting backup download operation", this->ActivityId);

        // Get the message body.
        DownloadBackupMessageBodySPtr downloadBackupMessageSPtr = std::make_shared<DownloadBackupMessageBody>();
        receivedMessage_->GetBody<DownloadBackupMessageBody>(*downloadBackupMessageSPtr);

        switch(downloadBackupMessageSPtr->StoreInfo.StoreType)
        {
        case BackupStoreType::AzureStore:
            {
                backupFileOrFolderCount_ = (int) downloadBackupMessageSPtr->BackupLocationList.size();

                WriteInfo(
                    TraceComponent,
                    "{0}: Starting download from Azure storage. BackupLocationCount: {1}",
                    this->ActivityId,
                    backupFileOrFolderCount_);

                for (int backupLocationIndex = 0; backupLocationIndex < backupFileOrFolderCount_; backupLocationIndex++)
                {
                    auto downloadOperation = backupCopierProxy_->BeginDownloadBackupFromAzureStorage(
                        this->ActivityId,
                        this->RemainingTime,
                        [this, backupLocationIndex](AsyncOperationSPtr const & operation)
                        {
                            this->OnDownloadBackupCompleted(operation, BackupStoreType::AzureStore, backupLocationIndex, backupFileOrFolderCount_, false);
                        },
                        thisSPtr,
                        downloadBackupMessageSPtr->StoreInfo.AzureStorageInfo.ConnectionString,
                        downloadBackupMessageSPtr->StoreInfo.AzureStorageInfo.IsConnectionStringEncrypted,
                        downloadBackupMessageSPtr->StoreInfo.AzureStorageInfo.ContainerName,
                        downloadBackupMessageSPtr->StoreInfo.AzureStorageInfo.FolderPath,
                        downloadBackupMessageSPtr->BackupLocationList[backupLocationIndex],
                        downloadBackupMessageSPtr->DestinationRootPath
                        );

                    this->OnDownloadBackupCompleted(downloadOperation, BackupStoreType::AzureStore, backupLocationIndex, backupFileOrFolderCount_, true);

                    if (downloadOperation->CompletedSynchronously &&
                        !this->lastSeenError_.IsSuccess() &&
                        (backupFileOrFolderCount_ != completedBackupFileOrFolderCount_))
                    {
                        // Terminate this loop 
                        WriteInfo(
                            TraceComponent,
                            "{0}: Terminating backup downloads as one of the download failed. BackupLocationIndex: {1}, BackupLocationCount: {2}",
                            this->ActivityId,
                            backupLocationIndex,
                            backupFileOrFolderCount_);

                        break;
                    }
                }
            }
            break;
        case BackupStoreType::FileShare:
            {
                backupFileOrFolderCount_ = (int) downloadBackupMessageSPtr->BackupLocationList.size();

                WriteInfo(
                    TraceComponent, 
                    "{0}: Starting download from File share. BackupLocationCount: {1}", 
                    this->ActivityId, 
                    backupFileOrFolderCount_);

                for (int backupLocationIndex = 0; backupLocationIndex < backupFileOrFolderCount_; backupLocationIndex++)
                {
                    auto downloadOperation = backupCopierProxy_->BeginDownloadBackupFromFileShare(
                        this->ActivityId,
                        this->RemainingTime,
                        [this, backupLocationIndex](AsyncOperationSPtr const & operation)
                        {
                            this->OnDownloadBackupCompleted(operation, BackupStoreType::FileShare, backupLocationIndex, backupFileOrFolderCount_, false);
                        },
                        thisSPtr,
                        downloadBackupMessageSPtr->StoreInfo.FileShareInfo.AccessType,
                        downloadBackupMessageSPtr->StoreInfo.FileShareInfo.IsPasswordEncrypted,
                        downloadBackupMessageSPtr->StoreInfo.FileShareInfo.FileSharePath,
                        downloadBackupMessageSPtr->StoreInfo.FileShareInfo.PrimaryUserName,
                        downloadBackupMessageSPtr->StoreInfo.FileShareInfo.PrimaryPassword,
                        downloadBackupMessageSPtr->StoreInfo.FileShareInfo.SecondaryUserName,
                        downloadBackupMessageSPtr->StoreInfo.FileShareInfo.SecondaryPassword,
                        downloadBackupMessageSPtr->BackupLocationList[backupLocationIndex],
                        downloadBackupMessageSPtr->DestinationRootPath
                        );

                    this->OnDownloadBackupCompleted(downloadOperation, BackupStoreType::FileShare, backupLocationIndex, backupFileOrFolderCount_, true);

                    if (downloadOperation->CompletedSynchronously && 
                        !this->lastSeenError_.IsSuccess() && 
                        (backupFileOrFolderCount_ != completedBackupFileOrFolderCount_))
                    {
                        // Terminate this loop 
                        WriteInfo(
                            TraceComponent,
                            "{0}: Terminating backup downloads as one of the download failed. BackupLocationIndex: {1}, BackupLocationCount: {2}",
                            this->ActivityId,
                            backupLocationIndex,
                            backupFileOrFolderCount_);

                        break;
                    }
                }
            }
            break;
        case BackupStoreType::DsmsAzureStore:
            {
                backupFileOrFolderCount_ = (int) downloadBackupMessageSPtr->BackupLocationList.size();

                WriteInfo(
                    TraceComponent,
                    "{0}: Starting download from Dsms Azure storage. BackupLocationCount: {1}",
                    this->ActivityId,
                    backupFileOrFolderCount_);

                for (int backupLocationIndex = 0; backupLocationIndex < backupFileOrFolderCount_; backupLocationIndex++)
                {
                    auto downloadOperation = backupCopierProxy_->BeginDownloadBackupFromDsmsAzureStorage(
                        this->ActivityId,
                        this->RemainingTime,
                        [this, backupLocationIndex](AsyncOperationSPtr const & operation)
                    {
                        this->OnDownloadBackupCompleted(operation, BackupStoreType::DsmsAzureStore, backupLocationIndex, backupFileOrFolderCount_, false);
                    },
                        thisSPtr,
                        downloadBackupMessageSPtr->StoreInfo.AzureStorageInfo.ConnectionString,
                        downloadBackupMessageSPtr->StoreInfo.AzureStorageInfo.ContainerName,
                        downloadBackupMessageSPtr->StoreInfo.AzureStorageInfo.FolderPath,
                        downloadBackupMessageSPtr->BackupLocationList[backupLocationIndex],
                        downloadBackupMessageSPtr->DestinationRootPath
                        );

                    this->OnDownloadBackupCompleted(downloadOperation, BackupStoreType::DsmsAzureStore, backupLocationIndex, backupFileOrFolderCount_, true);

                    if (downloadOperation->CompletedSynchronously &&
                        !this->lastSeenError_.IsSuccess() &&
                        (backupFileOrFolderCount_ != completedBackupFileOrFolderCount_))
                    {
                        // Terminate this loop 
                        WriteInfo(
                            TraceComponent,
                            "{0}: Terminating backup downloads as one of the download failed. BackupLocationIndex: {1}, BackupLocationCount: {2}",
                            this->ActivityId,
                            backupLocationIndex,
                            backupFileOrFolderCount_);

                        break;
                    }
                }
            }
            break;
        }
    }
}

void BackupCopyAsyncOperation::OnUploadBackupCompleted(
    AsyncOperationSPtr const & uploadBackupOperation,
    BackupStoreType::Enum storeType,
    bool expectedCompletedSynchronously)
{
    if (uploadBackupOperation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
    WriteInfo(TraceComponent, "{0}: Completing backup upload", this->ActivityId);

    Common::ErrorCode uploadError;

    switch (storeType)
    {
    case BackupStoreType::Enum::AzureStore:
        uploadError = backupCopierProxy_->EndUploadBackupToAzureStorage(uploadBackupOperation);
        break;
    case BackupStoreType::Enum::FileShare:
        uploadError = backupCopierProxy_->EndUploadBackupToFileShare(uploadBackupOperation);
        break;
    case BackupStoreType::Enum::DsmsAzureStore:
        uploadError = backupCopierProxy_->EndUploadBackupToDsmsAzureStorage(uploadBackupOperation);
        break;
    default:
        CODING_ASSERT("Unsupported StoreType value: {0}", storeType);
        break;
    }

    if (uploadError.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0}: Backup upload completed with success", this->ActivityId);
        reply_ = BAMessage::CreateSuccessReply(this->ActivityId);
    }
    else
    {
        WriteInfo(TraceComponent, "{0}: Backup upload failed with error: {1}", this->ActivityId, uploadError);
        reply_ = BAMessage::CreateIpcFailureMessage(SystemServices::IpcFailureBody(uploadError.ReadValue()), this->ActivityId);
    }

    this->TryComplete(uploadBackupOperation->Parent, uploadError.ReadValue());
}

void BackupCopyAsyncOperation::OnDownloadBackupCompleted(
    AsyncOperationSPtr const & downloadBackupOperation,
    BackupStoreType::Enum storeType,
    int backupLocationIndex,
    size_t backupLocationCount,
    bool expectedCompletedSynchronously)
{
    if (downloadBackupOperation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    WriteInfo(
        TraceComponent,
        "{0}: Completing backup download. BackupLocationIndex: {1}, BackupLocationCount: {2}",
        this->ActivityId,
        backupLocationIndex,
        backupLocationCount);
 
    Common::ErrorCode downloadError;

    switch (storeType)
    {
    case BackupStoreType::Enum::AzureStore:
        downloadError = backupCopierProxy_->EndDownloadBackupFromAzureStorage(downloadBackupOperation);
        break;
    case BackupStoreType::Enum::DsmsAzureStore:
        downloadError = backupCopierProxy_->EndDownloadBackupFromDsmsAzureStorage(downloadBackupOperation);
        break;
    case BackupStoreType::Enum::FileShare:
        downloadError = backupCopierProxy_->EndDownloadBackupFromFileShare(downloadBackupOperation);
        break;
    default:
        CODING_ASSERT("Unsupported StoreType value: {0}", storeType);
        break;
    }

    {
        AcquireExclusiveLock lock(completedBackupCounterLock_);

        // Increment completed backup download count.
        completedBackupFileOrFolderCount_++;

        if (downloadError.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: Backup download completed with success. BackupLocationIndex: {1}, BackupLocationCount: {2}",
                this->ActivityId,
                backupLocationIndex,
                backupLocationCount);
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: Backup download failed with error: {1}. BackupLocationIndex: {2}, BackupLocationCount: {3}",
                this->ActivityId,
                downloadError,
                backupLocationIndex,
                backupLocationCount);

            lastSeenError_.Overwrite(downloadError);
        }

        if (backupFileOrFolderCount_ == completedBackupFileOrFolderCount_)
        {
            if (lastSeenError_.IsSuccess())
            {
                WriteInfo(TraceComponent, "{0}: All Backup downloads completed with success", this->ActivityId);
                reply_ = BAMessage::CreateSuccessReply(this->ActivityId);
            }
            else
            {
                WriteInfo(TraceComponent, "{0}: At least one of the Backup download failed. Last seen error: {1}", this->ActivityId, lastSeenError_);
                reply_ = BAMessage::CreateIpcFailureMessage(SystemServices::IpcFailureBody(lastSeenError_.ReadValue()), this->ActivityId);
            }

            this->TryComplete(downloadBackupOperation->Parent, lastSeenError_.ReadValue());
        }
    }
}

void BackupCopyAsyncOperation::OnCompleted()
{
    RequestResponseAsyncOperationBase::OnCompleted();

    if (isUpload_)
    {
        WriteInfo(TraceComponent, "{0}: Finished upload operation", this->ActivityId);
    }
    else
    {
        WriteInfo(TraceComponent, "{0}: Finished download operation", this->ActivityId);
    }
}

Common::ErrorCode BackupCopyAsyncOperation::End(
    Common::AsyncOperationSPtr const & asyncOperation,
    __out Transport::MessageUPtr & reply,
    __out Transport::IpcReceiverContextUPtr & receiverContext)
{
    auto casted = AsyncOperation::End<BackupCopyAsyncOperation>(asyncOperation);
    auto error = casted->Error;
    receiverContext = move(casted->ReceiverContext);

    if (error.IsSuccess())
    {
        reply = std::move(casted->reply_);
    }

    return error;
}
