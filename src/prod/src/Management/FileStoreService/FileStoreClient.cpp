// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace Api;
using namespace ServiceModel;
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("FileStoreClient");

class FileStoreClient::ClientBaseAsyncOperation : public AsyncOperation
{
public:
    ClientBaseAsyncOperation(
        FileStoreClient & owner,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , timeoutHelper_(timeout)
    {
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        bool shouldOpen = false;
        {
            AcquireReadLock lock(owner_.lock_);
            if(!owner_.opened_)
            {
                shouldOpen = true;
            }
        }

        if(shouldOpen)
        {
            AcquireWriteLock lock(owner_.lock_);
            if(!owner_.opened_)
            {
                auto error = owner_.clientFactory_->CreateFileStoreServiceClient(owner_.fileStoreServiceClient_);
                if(error.IsSuccess())
                {
                    owner_.opened_ = true;
                }
                else
                {
                    TryComplete(thisSPtr, error);
                    return;
                }
            }
        }

        this->OnStartOperation(thisSPtr);
    }

    virtual void OnStartOperation(AsyncOperationSPtr const & thisSPtr) = 0;    

protected:
    FileStoreClient & owner_;
    TimeoutHelper timeoutHelper_;
};

class FileStoreClient::UploadAsyncOperation : public ClientBaseAsyncOperation
{
public:
    UploadAsyncOperation(
        FileStoreClient & owner,
        wstring const & sourceFullPath,
        wstring const & storeRelativePath,
        bool shouldOverwrite,
        IFileStoreServiceClientProgressEventHandlerPtr && progressHandler,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)
        , sourceFullPath_(sourceFullPath)
        , storeRelativePath_(storeRelativePath)
        , shouldOverwrite_(shouldOverwrite)
        , progressHandler_(move(progressHandler))
    {
    }

    ~UploadAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisSPtr = AsyncOperation::End<UploadAsyncOperation>(operation);
        return thisSPtr->Error;
    }

protected:
    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        if(sourceFullPath_.empty())
        {
            TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
            return;
        }

        this->Upload(thisSPtr);
    }    

    void Upload(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            owner_.TraceId,
            "Begin(UploadFile): Source:{0}, Store:{1}, ShouldOverwrite:{2}",
            sourceFullPath_,
            storeRelativePath_,
            shouldOverwrite_);

        auto operation = owner_.fileStoreServiceClient_->BeginUploadFile(
            owner_.serviceName_,
            sourceFullPath_,
            storeRelativePath_,
            shouldOverwrite_,
            move(progressHandler_),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnUploadComplete(operation, false); },
            thisSPtr);
        this->OnUploadComplete(operation, true);
    }

    void OnUploadComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreServiceClient_->EndUploadFile(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(UploadFile): Store:{0}, Error:{1}",
            storeRelativePath_,
            error);
        
        TryComplete(operation->Parent, error);
    }
    
private:
    wstring const sourceFullPath_;
    wstring const storeRelativePath_;
    bool const shouldOverwrite_;
    IFileStoreServiceClientProgressEventHandlerPtr progressHandler_;
};

class FileStoreClient::DownloadAsyncOperation : public ClientBaseAsyncOperation
{
public:
    DownloadAsyncOperation(
        FileStoreClient & owner,
        wstring const & storeRelativePath,
        wstring const & destinationFullPath,
        StoreFileVersion const version,
        vector<wstring> const & availableShares,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)
        , storeRelativePath_(storeRelativePath)
        , destinationFullPath_(destinationFullPath)
        , version_(version)
        , availableShares_(availableShares)
    {
    }

    ~DownloadAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisSPtr = AsyncOperation::End<DownloadAsyncOperation>(operation);
        return thisSPtr->Error;
    }

protected:
    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {        
        this->Download(thisSPtr);
    }

    void Download(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            owner_.TraceId,
            "Begin(DownloadFile): Store:{0}, Version:{1}, Destination:{2}",
            storeRelativePath_,
            version_,
            destinationFullPath_);

        auto operation = owner_.fileStoreServiceClient_->BeginDownloadFile(
            owner_.serviceName_,
            storeRelativePath_,
            destinationFullPath_,
            version_,
            availableShares_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnDownloadComplete(operation, false); },
            thisSPtr);
        this->OnDownloadComplete(operation, true);
    }

    void OnDownloadComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreServiceClient_->EndDownloadFile(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(DownloadFile): Store:{0}, Error:{1}",
            storeRelativePath_,
            error);
        
        TryComplete(operation->Parent, error);
    }

private:
    wstring const destinationFullPath_;
    wstring const storeRelativePath_;
    StoreFileVersion const version_;
    vector<wstring> const availableShares_;
};

FileStoreClient::FileStoreClient(    
    wstring const & serviceName,
    IClientFactoryPtr const & clientFactory) 
    : serviceName_(serviceName)
    , clientFactory_(clientFactory)
    , fileStoreServiceClient_() /* initialized during open */
    , lock_()
    , opened_(false)
{
    internalFileStoreClient_ = make_shared<InternalFileStoreClient>(
        serviceName,
        clientFactory,
        false /*shouldImpersonate*/);
}

FileStoreClient::~FileStoreClient()
{
}

ErrorCode FileStoreClient::SetSecurity(Transport::SecuritySettings && securitySettings)
{
    SecuritySettings securitySettingsForInternalStoreClient = securitySettings;
    auto error = internalFileStoreClient_->SetSecurity(move(securitySettingsForInternalStoreClient));
    if(!error.IsSuccess())
    {
        return error;
    }

    IClientSettingsPtr clientSettings;
    error = clientFactory_->CreateSettingsClient(clientSettings);

    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        TraceId,
        "SetSecurity: Error:{0}",
        error);

    if (error.IsSuccess())
    {
        clientSettings->SetSecurity(move(securitySettings));
    }    

    return error;
}

AsyncOperationSPtr FileStoreClient::BeginUpload(
    wstring const & sourceFullPath,
    wstring const & storeRelativePath,
    bool shouldOverwrite,
    IFileStoreServiceClientProgressEventHandlerPtr && progressHandler,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<UploadAsyncOperation>(
        *this,
        sourceFullPath,
        storeRelativePath,
        shouldOverwrite,
        move(progressHandler),
        timeout,
        callback,
        parent);
}

ErrorCode FileStoreClient::EndUpload(AsyncOperationSPtr const & operation)
{
    return UploadAsyncOperation::End(operation);
}

AsyncOperationSPtr FileStoreClient::BeginCopy(
    wstring const & sourceStoreRelativePath,
    StoreFileVersion const sourceVersion,
    wstring const & destinationStoreRelativePath,
    bool shouldOverwrite,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return internalFileStoreClient_->BeginCopy(
        sourceStoreRelativePath,
        sourceVersion,
        destinationStoreRelativePath,
        shouldOverwrite,
        timeout,
        callback,
        parent);
}

ErrorCode FileStoreClient::EndCopy(
    AsyncOperationSPtr const &operation)
{
    return internalFileStoreClient_->EndCopy(operation);
}

AsyncOperationSPtr FileStoreClient::BeginDownload(
    wstring const & storeRelativePath,
    wstring const & destinationFullPath,
    StoreFileVersion const version,
    vector<wstring> const & availableShares,
    IFileStoreServiceClientProgressEventHandlerPtr &&,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<DownloadAsyncOperation>(
        *this,
        storeRelativePath,
        destinationFullPath,
        version,
        availableShares,
        timeout,
        callback,
        parent);
}

ErrorCode FileStoreClient::EndDownload(AsyncOperationSPtr const &operation)
{
    return DownloadAsyncOperation::End(operation);
}

AsyncOperationSPtr FileStoreClient::BeginCheckExistence(
    std::wstring const & storeRelativePath,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const &callback,
    Common::AsyncOperationSPtr const &parent)
{
    return internalFileStoreClient_->BeginCheckExistence(
        storeRelativePath,
        timeout,
        callback,
        parent);
}

ErrorCode FileStoreClient::EndCheckExistence(
    Common::AsyncOperationSPtr const &operation,
    __out bool & result)
{
    return internalFileStoreClient_->EndCheckExistence(operation, result);
}

AsyncOperationSPtr FileStoreClient::BeginList(
    wstring const & storeRelativePath,
    wstring const & continuationToken,
    BOOLEAN const & shouldIncludeDetails,
    BOOLEAN const & isRecursive,
    bool isPaging,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return internalFileStoreClient_->BeginList(
        storeRelativePath,
        continuationToken,
        shouldIncludeDetails,
        isRecursive,
        isPaging,
        timeout,
        callback,
        parent);
}

ErrorCode FileStoreClient::EndList(
    AsyncOperationSPtr const &operation,
    __out StoreFileInfoMap & availableFiles,
    __out vector<StoreFolderInfo> & availableFolders,
    __out vector<wstring> & availableShares,
    __out wstring & continuationToken)
{
    return internalFileStoreClient_->EndList(operation, availableFiles, availableFolders, availableShares, continuationToken);
}

AsyncOperationSPtr FileStoreClient::BeginDelete(
    wstring const & storeRelativePath,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return internalFileStoreClient_->BeginDelete(
        storeRelativePath,
        timeout,
        callback,
        parent);
}

ErrorCode FileStoreClient::EndDelete(AsyncOperationSPtr const &operation)
{
    return internalFileStoreClient_->EndDelete(operation);
}

AsyncOperationSPtr FileStoreClient::BeginListUploadSession(
    std::wstring const & storeRelativePath,
    Common::Guid const & sessionId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return internalFileStoreClient_->BeginListUploadSession(
        storeRelativePath,
        sessionId,
        timeout,
        callback,
        parent);
}

ErrorCode FileStoreClient::EndListUploadSession(
    Common::AsyncOperationSPtr const & operation,
    __out UploadSession & uploadSession)
{
    return internalFileStoreClient_->EndListUploadSession(operation, uploadSession);
}

AsyncOperationSPtr FileStoreClient::BeginCreateUploadSession(
    std::wstring const & storeRelativePath,
    Common::Guid const & sessionId,
    uint64 fileSize,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return internalFileStoreClient_->BeginCreateUploadSession(
        storeRelativePath,
        sessionId,
        fileSize,
        timeout,
        callback,
        parent);
}

ErrorCode FileStoreClient::EndCreateUploadSession(
    Common::AsyncOperationSPtr const & operation)
{
    return internalFileStoreClient_->EndCreateUploadSession(operation);
}

AsyncOperationSPtr FileStoreClient::BeginUploadChunk(
    std::wstring const & localSource,
    Common::Guid const & sessionId,
    uint64 startPosition,
    uint64 endPosition,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return internalFileStoreClient_->BeginUploadChunk(
        localSource,
        sessionId,
        startPosition,
        endPosition,
        timeout,
        callback,
        parent);
}

ErrorCode FileStoreClient::EndUploadChunk(Common::AsyncOperationSPtr const & operation)
{
    return internalFileStoreClient_->EndUploadChunk(operation);
}

AsyncOperationSPtr FileStoreClient::BeginUploadChunkContent(
    Transport::MessageUPtr &chunkContentMessage,
    Management::FileStoreService::UploadChunkContentDescription & uploadChunkContentDescription,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return internalFileStoreClient_->BeginUploadChunkContent(
        chunkContentMessage,
        uploadChunkContentDescription,
        timeout,
        callback,
        parent);
}

Common::ErrorCode FileStoreClient::EndUploadChunkContent(
    Common::AsyncOperationSPtr const & operation)
{
    return internalFileStoreClient_->EndUploadChunkContent(operation);
}

AsyncOperationSPtr FileStoreClient::BeginDeleteUploadSession(
    Common::Guid const & sessionId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return internalFileStoreClient_->BeginDeleteUploadSession(
        sessionId,
        timeout,
        callback,
        parent);
}

ErrorCode FileStoreClient::EndDeleteUploadSession(Common::AsyncOperationSPtr const & operation)
{
    return internalFileStoreClient_->EndDeleteUploadSession(operation);
}

AsyncOperationSPtr FileStoreClient::BeginCommitUploadSession(
    std::wstring const & storeRelativePath,
    Common::Guid const & sessionId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return internalFileStoreClient_->BeginCommitUploadSession(
        storeRelativePath,
        sessionId,
        timeout,
        callback,
        parent);
}

ErrorCode FileStoreClient::EndCommitUploadSession(Common::AsyncOperationSPtr const & operation)
{
    return internalFileStoreClient_->EndCommitUploadSession(operation);
}
