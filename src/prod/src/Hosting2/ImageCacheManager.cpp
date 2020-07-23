//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ImageModel;
using namespace ImageStore;
using namespace ServiceModel;
using namespace Query;

StringLiteral const Trace_ImageCacheManager("ImageCacheManager");

class ImageCacheManager::DownloadLinkedAsyncOperation
    : public LinkableAsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(DownloadLinkedAsyncOperation)

public:
    DownloadLinkedAsyncOperation(
        __in ImageCacheManager & owner,
        wstring const & remoteSourcePath,
        wstring const & localDestinationPath,
        TimeSpan const timeout,
        wstring const & remoteChecksumObject,
        wstring const & expectedChecksumValue,
        bool refreshCache,
        CopyFlag::Enum copyFlag,
        bool copyToLocalStoreLayoutOnly,
        bool checkForArchive,
        Guid const & activityId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : LinkableAsyncOperation(callback, parent)
        , owner_(owner)
        , remoteSourcePath_(remoteSourcePath)
        , localDestinationPath_(localDestinationPath)
        , timeout_(timeout)
        , remoteChecksumObject_(remoteChecksumObject)
        , expectedChecksumValue_(expectedChecksumValue)
        , refreshCache_(refreshCache)
        , copyFlag_(copyFlag)
        , copyToLocalStoreLayoutOnly_(copyToLocalStoreLayoutOnly)
        , checkForArchive_(checkForArchive)
        , activityId_(activityId)
    {
    } 

    DownloadLinkedAsyncOperation(
        ImageCacheManager & owner,
        AsyncOperationSPtr const & primary,
        Guid const & activityId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : LinkableAsyncOperation(primary, callback, parent)
        , owner_(owner)
        , remoteSourcePath_()
        , localDestinationPath_()
        , timeout_()
        , remoteChecksumObject_()
        , expectedChecksumValue_()
        , refreshCache_()
        , copyFlag_()
        , copyToLocalStoreLayoutOnly_()
        , checkForArchive_()
        , activityId_(activityId)
    {
    }

    virtual ~DownloadLinkedAsyncOperation() {}

    __declspec(property(get = get_ActivityId)) Common::ActivityId const & ActivityId;
    inline Common::ActivityId const & get_ActivityId() const { return activityId_; }

    __declspec(property(get = get_LocalDestinationPath)) wstring const & LocalDestinationPath;
    inline wstring const & get_LocalDestinationPath() const { return localDestinationPath_; }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisSPtr = AsyncOperation::End<DownloadLinkedAsyncOperation>(operation);
        return thisSPtr->Error;
    }

    bool IsCurrentRemotePath(wstring const & remotePath)
    {
        return StringUtility::AreEqualCaseInsensitive(remotePath, this->remoteSourcePath_);
    }


protected:
    void OnResumeOutsideLockPrimary(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error = owner_.imageStore_->DownloadContent(
            remoteSourcePath_,
            localDestinationPath_,
            timeout_,
            remoteChecksumObject_,
            expectedChecksumValue_,
            refreshCache_,
            copyFlag_,
            copyToLocalStoreLayoutOnly_,
            checkForArchive_);

        TryComplete(thisSPtr, move(error));
    }

    void OnCompleted()
    {
        if (!this->IsSecondary)
        {
            WriteInfo(
                Trace_ImageCacheManager,
                "Primary completed for id:{0}, remoteSourcePath:{1}, localDestinationPath: {2}, remoteChecksumObject:{3}, expectedChecksumValue:{4}, refreshCache:{5}, copyFlag:{6}, copyToLocalStoreLayoutOnly:{7}, checkForArchive:{8}",
                activityId_,
                remoteSourcePath_,
                localDestinationPath_,
                remoteChecksumObject_,
                expectedChecksumValue_,
                refreshCache_,
                copyFlag_,
                copyToLocalStoreLayoutOnly_,
                checkForArchive_);
            owner_.RemoveActiveDownload(this->localDestinationPath_);
        }
        else
        {
            WriteInfo(
                Trace_ImageCacheManager,
                "Secondary completed for {0}", activityId_);
        }

        LinkableAsyncOperation::OnCompleted();
    }

private:
    ImageCacheManager & owner_;
    wstring const remoteSourcePath_;
    wstring const localDestinationPath_;
    TimeSpan const timeout_;
    wstring const remoteChecksumObject_;
    wstring const expectedChecksumValue_;
    bool refreshCache_;
    CopyFlag::Enum copyFlag_;
    bool copyToLocalStoreLayoutOnly_;
    bool checkForArchive_;
    Common::ActivityId const activityId_;
};

class ImageCacheManager::GetChecksumLinkedAsyncOperation
    : public LinkableAsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(GetChecksumLinkedAsyncOperation)

public:
    GetChecksumLinkedAsyncOperation(
        __in ImageCacheManager & owner,
        wstring const & storeManifestChechsumFilePath,
        Guid const & activityId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : LinkableAsyncOperation(callback, parent)
        , owner_(owner)
        , storeManifestChechsumFilePath_(storeManifestChechsumFilePath)
        , expectedChecksum_()
        , activityId_(activityId)
    {
    }

    GetChecksumLinkedAsyncOperation(
        ImageCacheManager & owner,
        AsyncOperationSPtr const & primary,
        Guid const & activityId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : LinkableAsyncOperation(primary, callback, parent)
        , owner_(owner)
        , storeManifestChechsumFilePath_()
        , expectedChecksum_()
        , activityId_(activityId)
    {
    }

    virtual ~GetChecksumLinkedAsyncOperation() {}

    __declspec(property(get = get_ActivityId)) Common::Guid const & ActivityId;
    inline Common::Guid const & get_ActivityId() const { return activityId_; }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out wstring & expectedChecksum)
    {
        auto thisSPtr = AsyncOperation::End<GetChecksumLinkedAsyncOperation>(operation);
        if (thisSPtr->Error.IsSuccess())
        {
            expectedChecksum = thisSPtr->expectedChecksum_;
        }

        return thisSPtr->Error;
    }

protected:
    void OnResumeOutsideLockPrimary(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error = owner_.imageStore_->GetStoreChecksumValue(
            this->storeManifestChechsumFilePath_,
            this->expectedChecksum_);

        TryComplete(thisSPtr, move(error));
    }

    void OnCompleted()
    {
        if (!this->IsSecondary)
        {
            WriteInfo(
                Trace_ImageCacheManager,
                "Primary completed for {0} store checksum {1}",
                activityId_,
                storeManifestChechsumFilePath_);
            owner_.RemoveActiveGetCheckSum(this->storeManifestChechsumFilePath_);
        }
        else
        {
            WriteInfo(
                Trace_ImageCacheManager,
                "Secondary completed for {0} store checksum path {1}",
                activityId_,
                storeManifestChechsumFilePath_);
        }

        LinkableAsyncOperation::OnCompleted();
    }

private:
    ImageCacheManager & owner_;
    wstring const storeManifestChechsumFilePath_;
    wstring expectedChecksum_;
    Guid const activityId_;
};

ImageCacheManager::ImageCacheManager(
    ComponentRoot const & root,
    __in ImageStoreUPtr & imageStore)
    : RootedObject(root),
    imageStore_(imageStore)
    , downloadLock_()
    , checksumLock_()
{
}

AsyncOperationSPtr ImageCacheManager::BeginDownload(
    wstring const & remoteSourcePath,
    wstring const & localDestinationPath,
    wstring const & remoteChecksumObject,
    wstring const & expectedChecksumValue,
    bool refreshCache,
    CopyFlag::Enum copyFlag,
    bool copyToLocalStoreLayoutOnly,
    bool checkForArchive,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    AsyncOperationSPtr linkedDownloadOperation;
    {

        AsyncOperationSPtr operation;
        Guid activityId = Guid::NewGuid();

        AcquireWriteLock lock(downloadLock_);
        auto it = activeDownloads_.find(localDestinationPath);
        if (it == activeDownloads_.end())
        {
            WriteInfo(
                Trace_ImageCacheManager,
                "Creating primary operation:{0} for remoteSourcePath: {1}, localDestinationPath: {2}",
                activityId,
                remoteSourcePath,
                localDestinationPath);

            linkedDownloadOperation = AsyncOperation::CreateAndStart<DownloadLinkedAsyncOperation>(
                *this,
                remoteSourcePath,
                localDestinationPath,
                ServiceModelConfig::GetConfig().MaxOperationTimeout,
                remoteChecksumObject,
                expectedChecksumValue,
                refreshCache,
                copyFlag,
                copyToLocalStoreLayoutOnly,
                checkForArchive,
                activityId,
                callback,
                parent);

            activeDownloads_.insert(make_pair(
                localDestinationPath,
                linkedDownloadOperation));
        }
        else
        {
            operation = it->second;
        }

        if (linkedDownloadOperation == nullptr)
        {
            auto downloadPrimaryOperation = AsyncOperation::Get<DownloadLinkedAsyncOperation>(operation);
            if (downloadPrimaryOperation->IsCurrentRemotePath(remoteSourcePath))
            {
                //Add the secondary async operation
                linkedDownloadOperation = AsyncOperation::CreateAndStart<DownloadLinkedAsyncOperation>(
                    *this,
                    operation,
                    activityId,
                    callback,
                    parent);

                WriteInfo(
                    Trace_ImageCacheManager,
                    "{0}: linked to primary:{1} for remoteSourcePath: {2}, localDestinationPath: {3}",
                    activityId,
                    downloadPrimaryOperation->ActivityId,
                    remoteSourcePath,
                    localDestinationPath);
            }
            else
            {
                WriteInfo(
                    Trace_ImageCacheManager,
                    "Different RemoteSourcePath {0} -> {1}",
                    remoteSourcePath,
                    localDestinationPath);
                Assert::CodingError("Downloading to same destination: {0} from different source location {1}, expected {2}",
                    localDestinationPath,
                    remoteSourcePath,
                    downloadPrimaryOperation->LocalDestinationPath);
            }
        }
    }
    auto linkableOp = AsyncOperation::Get<LinkableAsyncOperation>(linkedDownloadOperation);
    linkableOp->ResumeOutsideLock(linkedDownloadOperation);
    return linkedDownloadOperation;
}

ErrorCode ImageCacheManager::EndDownload(AsyncOperationSPtr const & operation)
{
    return DownloadLinkedAsyncOperation::End(operation);
}

AsyncOperationSPtr ImageCacheManager::BeginGetChecksum(
    wstring const & storeManifestChechsumFilePath,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    AsyncOperationSPtr linkedGetChecksumOperation;
    {
        AcquireWriteLock lock(checksumLock_);
        Guid activityId = Guid::NewGuid();
        auto it = activeGetChecksum_.find(storeManifestChechsumFilePath);
        if (it == activeGetChecksum_.end())
        {
            linkedGetChecksumOperation = AsyncOperation::CreateAndStart<GetChecksumLinkedAsyncOperation>(
                *this,
                storeManifestChechsumFilePath,
                activityId,
                callback,
                parent);

            activeGetChecksum_.insert(make_pair(storeManifestChechsumFilePath, linkedGetChecksumOperation));
        }
        else
        {
            linkedGetChecksumOperation = AsyncOperation::CreateAndStart<GetChecksumLinkedAsyncOperation>(
                *this,
                it->second,
                activityId,
                callback,
                parent);
        }
    }

    auto casted = AsyncOperation::Get<GetChecksumLinkedAsyncOperation>(linkedGetChecksumOperation);
    casted->ResumeOutsideLock(linkedGetChecksumOperation);
    return linkedGetChecksumOperation;
}

ErrorCode ImageCacheManager::EndGetChecksum(AsyncOperationSPtr const & operation, __out wstring & checksum)
{
    return GetChecksumLinkedAsyncOperation::End(operation, checksum);
}

void ImageCacheManager::RemoveActiveDownload(wstring const & localDestinationPath)
{
    AcquireWriteLock lock(downloadLock_);
    auto it = activeDownloads_.find(localDestinationPath);
    if (it != activeDownloads_.end())
    {
        activeDownloads_.erase(it);
    }
    WriteNoise(
        Trace_ImageCacheManager,
        "Remove LocalDestinationPath {0} from ActiveDownload",
        localDestinationPath);
}

void ImageCacheManager::RemoveActiveGetCheckSum(wstring const & storeManifestChechsumFilePath)
{
    AcquireWriteLock lock(checksumLock_);
    auto it = activeGetChecksum_.find(storeManifestChechsumFilePath);
    if (it != activeGetChecksum_.end())
    {
        activeGetChecksum_.erase(it);
    }
    WriteNoise(
        Trace_ImageCacheManager,
        "Remove ChecksumFilePath {0} from ActiveGetChecksum_",
        storeManifestChechsumFilePath);
}


ErrorCode ImageCacheManager::GetStoreChecksumValue(
    wstring const & remoteObject,
    _Inout_ wstring & checksumValue) const
{
    return imageStore_->GetStoreChecksumValue(remoteObject, checksumValue);
}