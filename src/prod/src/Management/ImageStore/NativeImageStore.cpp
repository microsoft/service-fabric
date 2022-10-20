// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "NativeImageStoreProgressState.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Management;
using namespace ImageStore;
using namespace ImageModel;
using namespace ServiceModel;
using namespace FileStoreService;

#if defined(PLATFORM_UNIX)
inline static bool AreEqualPaths(std::wstring const & a, std::wstring const & b)
{
    wstring aa(a), bb(b);
    std::replace(aa.begin(), aa.end(), '\\', '/');
    std::replace(bb.begin(), bb.end(), '\\', '/');
    return _wcsicmp(aa.c_str(), bb.c_str()) == 0;
}
inline static bool StartsWithPaths(wstring const & bigStr, wstring const & smallStr)
{
    wstring bigStrToLower(bigStr);
    wstring smallStrToLower(smallStr);
    for(auto i=smallStrToLower.begin(); i!=smallStrToLower.end(); i++)
        *i = tolower(*i);
    for(auto j=bigStrToLower.begin(); j!=bigStrToLower.end(); j++)
        *j = tolower(*j);
    std::replace(bigStrToLower.begin(), bigStrToLower.end(), '\\', '/');
    std::replace(smallStrToLower.begin(), smallStrToLower.end(), '\\', '/');
    return (bigStrToLower.substr(0,smallStrToLower.size()).compare(smallStrToLower) == 0);
}
#endif

StringLiteral const TraceComponent("NativeImageStore");

GlobalWString NativeImageStore::DirectoryMarkerFileName = make_global<wstring>(L"_.dir");

class NativeImageStore::OperationBaseAsyncOperation 
    : public AsyncOperation
    , public TextTraceComponent<TraceTaskCodes::NativeImageStoreClient>
{
public:
    OperationBaseAsyncOperation(
        wstring const & operationName,
        shared_ptr<IFileStoreClient> const & client,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : AsyncOperation(callback, root)
        , error_()
        , operationName_(operationName)
        , client_(client)
        , timeoutHelper_(timeout)
        , retryTimer_()
        , timerLock_()
    {
    }

    OperationBaseAsyncOperation(
        ErrorCode const & error,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : AsyncOperation(callback, root)
        , error_(error)
        , operationName_()
        , client_()
        , timeoutHelper_(TimeSpan::MaxValue)
        , retryTimer_()
        , timerLock_()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<OperationBaseAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->TryComplete(thisSPtr, error_);
    }

    void SetFirstError(ErrorCode const & error)
    {
        AcquireWriteLock writeLock(errorLock_);
        error_ = ErrorCode::FirstError(error_, error);
    }

    ErrorCode const & GetFirstError()
    {
        AcquireReadLock readLock(errorLock_);
        return error_;
    }

    TimeSpan GetShortTimeoutInterval(uint retryCount)
    {
        auto shortTimeoutInterval = FileStoreServiceConfig::GetConfig().ShortRequestTimeout + Common::TimeSpan::FromMinutes(retryCount);
        return shortTimeoutInterval < this->GetRemainingTime() ? shortTimeoutInterval : this->GetRemainingTime();
    }

    typedef function<void(AsyncOperationSPtr const &)> RetryCallback;

    ErrorCode TryScheduleRetry(AsyncOperationSPtr const & thisSPtr, RetryCallback const & callback)
    {
        TimeSpan delay = FileStoreServiceConfig::GetConfig().ClientRetryInterval;
        if (delay > timeoutHelper_.GetRemainingTime())
        {
            // Not enough timeout left - just fail early
            return ErrorCodeValue::OperationCanceled;
        }

        {
            AcquireExclusiveLock lock(timerLock_);

            if (!this->InternalIsCompleted)
            {
                retryTimer_ = Timer::Create(
                    TimerTagDefault,
                    [this, thisSPtr, callback](TimerSPtr const & timer)
                    {
                        timer->Cancel();
                        callback(thisSPtr);
                    });
                retryTimer_->Change(delay);
            }
        }

        return ErrorCodeValue::Success;
    }

    void OnCompleted()
    {
        TimerSPtr snap;
        {
            AcquireExclusiveLock lock(timerLock_);
            snap = retryTimer_;
        }

        if (snap)
        {
            snap->Cancel();
        }
    }

    wstring GetOperationName() const { return operationName_; }
    shared_ptr<IFileStoreClient> const & GetClient() const { return client_; }
    TimeSpan GetRemainingTime() const { return timeoutHelper_.GetRemainingTime(); }

private:

    ErrorCode error_;
    wstring operationName_;
    shared_ptr<IFileStoreClient> client_;
    TimeoutHelper timeoutHelper_;
    TimerSPtr retryTimer_;
    ExclusiveLock timerLock_;
    RwLock errorLock_;
};


class NativeImageStore::ListFileAsyncOperation: public NativeImageStore::OperationBaseAsyncOperation
{
public:
    ListFileAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & target,
        wstring const & continuationToken,
        BOOLEAN const & isRecursive,
        bool isPaging,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(L"ListFile", client, timeout, callback, root)
        , target_(target)
        , continuationToken_(continuationToken)
        , isRecursive_(isRecursive)
        , isPaging_(isPaging)
        , storeFolders_()
        , storeFiles_()
        , shares_()
        , returnedContinuationToken_()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out Management::FileStoreService::StoreContentInfo & result)
    {
        auto casted = AsyncOperation::End<ListFileAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            result.StoreFiles = std::move(casted->storeFiles_);
            result.StoreFolders = std::move(casted->storeFolders_);
        }

        return casted->Error;
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out Management::FileStoreService::StorePagedContentInfo & result)
    {
        auto casted = AsyncOperation::End<ListFileAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            result.StoreFiles = std::move(casted->storeFiles_);
            result.StoreFolders = std::move(casted->storeFolders_);
            result.ContinuationToken = std::move(casted->returnedContinuationToken_);
        }

        return casted->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            "Begin(List): RemoteLocation={0}, ContinuationToken={1}",
            target_,
            continuationToken_);

        auto operation = this->GetClient()->BeginList(
            target_,
            continuationToken_,
            true,  //Set shouldIncludeDetails to true
            isRecursive_,
            isPaging_,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnListComplete(operation, false); },
            thisSPtr);

        this->OnListComplete(operation, true);
    }

private:

    void OnListComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        StoreFileInfoMap files;

        auto error = this->GetClient()->EndList(operation, files, storeFolders_, shares_, returnedContinuationToken_);
        WriteInfo(
            TraceComponent,
            "End(List): FileCount={0}, FolderCount={1}, ShareCount={2}, ContinuationToken={3}",
            files.size(),
            storeFolders_.size(),
            shares_.size(),
            returnedContinuationToken_);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "Lists failed: '{0}' error = {1}",
                target_,
                error);
        }
        else
        {
            for (auto it = files.begin(); it != files.end(); ++it)
            {
                storeFiles_.push_back(it->second);
            }
        }

        this->TryComplete(thisSPtr, move(error));
    }

    std::wstring target_;
    std::wstring continuationToken_;
    BOOLEAN isRecursive_;
    bool isPaging_;
    vector<StoreFolderInfo> storeFolders_;
    vector<StoreFileInfo> storeFiles_;
    vector<wstring> shares_;
    std::wstring returnedContinuationToken_;
};

class NativeImageStore::FileExistsAsyncOperation : public NativeImageStore::OperationBaseAsyncOperation
{
public:
    FileExistsAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & target,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(L"FileExists", client, timeout, callback, root)
        , target_(target)
        , exists_(false)
        , retryCount_(0)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out bool & exists)
    {
        auto casted = AsyncOperation::End<FileExistsAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            exists = casted->exists_;
        }

        return casted->Error;
    }

protected:

    virtual bool IsTargetPath(wstring const & listResultItem)
    {
#if !defined(PLATFORM_UNIX)
        return StringUtility::AreEqualCaseInsensitive(target_, listResultItem);
#else
        wstring listResult(listResultItem);
        std::replace(listResult.begin(), listResult.end(), '\\', '/');
        return AreEqualPaths(target_, listResult);
#endif
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "CheckExistence '{0}'",
            target_);
            
        auto operation = this->GetClient()->BeginCheckExistence(
            target_,
            GetShortTimeoutInterval(retryCount_),
            [this](AsyncOperationSPtr const & operation){ this->OnCheckExistenceComplete(operation, false); },
            thisSPtr);

        this->OnCheckExistenceComplete(operation, true);
    }

private:

    void StartList(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "Listing '{0}'",
            target_);

        auto operation = this->GetClient()->BeginList(
            target_,
            L"",
            false,
            true,
            false, //*isPaging*/
            GetShortTimeoutInterval(retryCount_),
            [this](AsyncOperationSPtr const & operation) { this->OnListComplete(operation, false); },
            thisSPtr);
        this->OnListComplete(operation, true);
    }

    void OnCheckExistenceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;
        auto error = this->GetClient()->EndCheckExistence(operation, exists_);

        if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::Timeout) && retryCount_ < FileStoreServiceConfig::GetConfig().MaxFileOperationFailureRetryCount)
            {
                ++retryCount_;
                WriteInfo(
                    TraceComponent,
                    "{0}: Exists failed: '{1}' error:{2} retryCount:{3}. Retrying the CheckExistence operation.",
                    GetOperationName(),
                    target_,
                    error,
                    retryCount_);

                OnStart(thisSPtr);
                return;
            }

            WriteInfo(
                TraceComponent,
                "Exists failed: '{0}' error = {1}",
                target_,
                error);

            if (error.IsError(ErrorCodeValue::InvalidOperation))
            {
                return this->StartList(thisSPtr);
            }
        }
        else
        {
            WriteNoise(
                TraceComponent,
                "Exists succeeded: '{0}' result = {1}",
                target_,
                exists_);
        }

        this->TryComplete(thisSPtr, move(error));
    }

    void OnListComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        StoreFileInfoMap files;
        vector<StoreFolderInfo> folders;
        vector<wstring> shares;
        wstring continuationToken;

        auto error = this->GetClient()->EndList(operation, files, folders, shares, continuationToken);
        if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::Timeout) && retryCount_ < FileStoreServiceConfig::GetConfig().MaxFileOperationFailureRetryCount)
            {
                ++retryCount_;
                WriteInfo(
                    TraceComponent,
                    "{0}: Exists failed: '{1}' error:{2} retryCount:{3}. Retrying the List operation.",
                    GetOperationName(),
                    target_,
                    error,
                    retryCount_);

                this->StartList(thisSPtr);
                return;
            }

            WriteInfo(
                TraceComponent,
                "Exists failed: '{0}' error = {1}",
                target_,
                error);
        }
        else
        {
            for (auto it = files.begin(); it != files.end(); ++it)
            {
                if (this->IsTargetPath(it->first))
                {
                    exists_ = true;
                    break;
                }
            }

            WriteNoise(
                TraceComponent,
                "Exists succeeded: '{0}' result = {1}",
                target_,
                exists_);
        }

        this->TryComplete(thisSPtr, move(error));
    }

    wstring target_;
    bool exists_;
    uint retryCount_;
};

class NativeImageStore::DirectoryExistsAsyncOperation : public NativeImageStore::FileExistsAsyncOperation
{
public:
    DirectoryExistsAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & target,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : FileExistsAsyncOperation(
            client, 
            NativeImageStore::GetDirectoryMarkerFileName(target),
            timeout, 
            callback, 
            root)
    {
    }
};

class NativeImageStore::ObjectExistsAsyncOperation : public NativeImageStore::FileExistsAsyncOperation
{

public:

    ObjectExistsAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & target,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : FileExistsAsyncOperation(
        client,
        target,
        timeout,
        callback,
        root)
        , targetDirectoryMarker_(NativeImageStore::GetDirectoryMarkerFileName(target))
    {
    }

protected:

    virtual bool IsTargetPath(wstring const & listResultItem)
    {
        return FileExistsAsyncOperation::IsTargetPath(listResultItem)
#if !defined(PLATFORM_UNIX)
            || StringUtility::AreEqualCaseInsensitive(targetDirectoryMarker_, listResultItem);
#else
            || AreEqualPaths(targetDirectoryMarker_, listResultItem);
#endif

    }

private:

    wstring targetDirectoryMarker_;
};

class NativeImageStore::DeleteAsyncOperation : public NativeImageStore::OperationBaseAsyncOperation
{
public:
    DeleteAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & target,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(L"Delete", client, timeout, callback, root)
        , target_(target)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<DeleteAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "Deleting '{0}'",
            target_);

        // Server deletes all prefix matches
        //
        auto operation = this->GetClient()->BeginDelete(
            target_,
            GetShortTimeoutInterval(retryCount_),
            [this](AsyncOperationSPtr const & operation){ this->OnDeleteComplete(operation, false); },
            thisSPtr);
        this->OnDeleteComplete(operation, true);
    }

private:

    void OnDeleteComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = this->GetClient()->EndDelete(operation);

        if (error.IsError(ErrorCodeValue::FileNotFound))
        {
            error = ErrorCodeValue::Success;
        }

        if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::Timeout) && retryCount_ < FileStoreServiceConfig::GetConfig().MaxFileOperationFailureRetryCount)
            {
                ++retryCount_;
                WriteInfo(
                    TraceComponent,
                    "{0}: Delete failed: '{1}' error:{2} retryCount:{3}. Retrying the Delete operation.",
                    GetOperationName(),
                    target_,
                    error,
                    retryCount_);

                OnStart(thisSPtr);
                return;
            }

            WriteInfo(
                TraceComponent,
                "Delete failed: '{0}' error = {1}",
                target_,
                error);
        }

        this->TryComplete(thisSPtr, move(error));
    }

    wstring target_;
    uint retryCount_;
};

class NativeImageStore::UploadFileAsyncOperation : public NativeImageStore::OperationBaseAsyncOperation
{
public:
    UploadFileAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & localFullPath,
        wstring const & remoteRelativePath,
        CopyFlag::Enum const flag,
        NativeImageStoreProgressStateSPtr const & progressState,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(L"UploadFile", client, timeout, callback, root)
        , localFullPath_(localFullPath)
        , remoteRelativePath_(remoteRelativePath)
        , flag_(flag)
        , progressState_(progressState)
        , uploaded_(false)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<UploadFileAsyncOperation>(operation)->Error;
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out bool & uploaded)
    {
        auto casted = AsyncOperation::End<UploadFileAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            uploaded = casted->uploaded_;
        }

        return casted->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (flag_ == CopyFlag::Overwrite)
        {
            this->StartUpload(thisSPtr);
        }
        else
        {
            auto operation = AsyncOperation::CreateAndStart<FileExistsAsyncOperation>(
                this->GetClient(),
                remoteRelativePath_,
                this->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnExistsComplete(operation, false); },
                thisSPtr);
            this->OnExistsComplete(operation, true);
        }
    }

private:

    void OnExistsComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        bool exists = false;

        auto error = FileExistsAsyncOperation::End(operation, exists);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "File exists failed: '{0}' error = {1}",
                remoteRelativePath_,
                error);

            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            if (exists) 
            { 
                uploaded_ = false;

                this->TryComplete(thisSPtr, ErrorCodeValue::Success);
            }
            else
            {
                this->StartUpload(thisSPtr);
            }
        }
    }

    void StartUpload(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "Uploading '{0}' -> '{1}'",
            localFullPath_,
            remoteRelativePath_);

        auto operation = this->GetClient()->BeginUpload(
            localFullPath_,
            remoteRelativePath_,
            (flag_ == CopyFlag::Overwrite),
            IFileStoreServiceClientProgressEventHandlerPtr(progressState_.get(), progressState_),
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnUploadComplete(operation, false); },
            thisSPtr);
        this->OnUploadComplete(operation, true);
    }

    void OnUploadComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = this->GetClient()->EndUpload(operation);

        if (!error.IsSuccess()) 
        { 
            WriteInfo(
                TraceComponent,
                "File upload failed: '{0}' error = {1}",
                remoteRelativePath_,
                error);
        }
        else
        {
            uploaded_ = true;
        }

        this->TryComplete(thisSPtr, move(error));
    }

    wstring localFullPath_;
    wstring remoteRelativePath_;
    CopyFlag::Enum flag_;
    NativeImageStoreProgressStateSPtr progressState_;
    bool uploaded_;
};

// Wraps parallel execution of multiple single file operations against server.
//
class NativeImageStore::ParallelOperationsBaseAsyncOperation : public NativeImageStore::OperationBaseAsyncOperation
{
public:
    ParallelOperationsBaseAsyncOperation(
        wstring const & operationName,
        shared_ptr<IFileStoreClient> const & client,
        vector<wstring> && sources,
        vector<wstring> && destinations,
        vector<CopyFlag::Enum> && flags,
        NativeImageStoreProgressStateSPtr const & progressState,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(operationName, client, timeout, callback, root)
        , sources_(move(sources))
        , destinations_(move(destinations))
        , flags_(move(flags))
        , progressState_(progressState)
        , pendingCount_(0)
        , errorCount_(0)
    {
    }

    ParallelOperationsBaseAsyncOperation(
        wstring const & operationName,
        shared_ptr<IFileStoreClient> const & client,
        vector<wstring> const& sources,
        vector<wstring> const& destinations,
        vector<CopyFlag::Enum> const& flags,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(operationName, client, timeout, callback, root)
        , sources_(sources)
        , destinations_(destinations)
        , flags_(flags)
        , progressState_()
        , pendingCount_(0)
        , errorCount_(0)
    {
    }

    ParallelOperationsBaseAsyncOperation(
        wstring const & operationName,
        shared_ptr<IFileStoreClient> const & client,
        vector<wstring> && destinations,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(operationName, client, timeout, callback, root)
        , sources_()
        , destinations_(move(destinations))
        , flags_()
        , progressState_()
        , pendingCount_(0)
        , errorCount_(0)
    {
    }

    ParallelOperationsBaseAsyncOperation(
        wstring const & operationName,
        shared_ptr<IFileStoreClient> const & client,
        vector<wstring> const& destinations,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(operationName, client, timeout, callback, root)
        , sources_()
        , destinations_(destinations)
        , flags_()
        , progressState_()
        , pendingCount_(0)
        , errorCount_(0)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<ParallelOperationsBaseAsyncOperation>(operation)->Error;
    }

protected:

    NativeImageStoreProgressStateSPtr GetProgressState() const { return progressState_; }

    virtual AsyncOperationSPtr OnBeginOperation(
        wstring const & src, 
        wstring const & dest, 
        CopyFlag::Enum flag,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root) = 0;

    virtual ErrorCode OnEndOperation(
        wstring const & src,
        wstring const & dest,
        AsyncOperationSPtr const & operation)
    {
        UNREFERENCED_PARAMETER(src);
        UNREFERENCED_PARAMETER(dest);

        return OperationBaseAsyncOperation::End(operation);
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!sources_.empty() && (sources_.size() != destinations_.size() || destinations_.size() != flags_.size()))
        {
            WriteWarning(
                TraceComponent,
                "{0} mismatched counts: src = {1} dest = {2} flags = {3}",
                this->GetOperationName(),
                sources_.size(),
                destinations_.size(),
                flags_.size());

            this->TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);

            return;
        }

        if (destinations_.empty())
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {
            pendingCount_.store(static_cast<LONG>(destinations_.size()));

            auto delay = TimeSpan::FromMilliseconds(ImageStoreServiceConfig::GetConfig().ClientOperationDelayMilliseconds);

            if (delay > TimeSpan::Zero)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} ImageStoreClient/ClientOperationDelay configuration: {1} ms",
                    this->GetOperationName(),
                    delay);
            }

            WriteInfo(
                TraceComponent,
                "{0} P-start: src = {1} dest = {2}",
                this->GetOperationName(),
                sources_.size(),
                destinations_.size());

            for (auto ix=0; ix<destinations_.size(); ++ix)
            {
                if (delay > TimeSpan::Zero)
                {
                    Sleep(static_cast<DWORD>(delay.TotalPositiveMilliseconds()));
                }
                
                auto src = sources_.empty() ? L"" : sources_[ix];
                auto dest = destinations_[ix];
                auto flag = flags_.empty() ? CopyFlag::Echo : flags_[ix];

                WriteNoise(
                    TraceComponent,
                    "{0} start: src = {1} dest = {2} flag = {3}, timeout = {4}", 
                    this->GetOperationName(), 
                    src, 
                    dest, 
                    flag, 
                    this->GetRemainingTime()
                );

                auto operation = this->OnBeginOperation(
                    src,
                    dest,
                    flag,
                    this->GetRemainingTime(),
                    [this, src, dest](AsyncOperationSPtr const & operation) { this->OnOperationComplete(src, dest, operation, false); },
                    thisSPtr);
                this->OnOperationComplete(src, dest, operation, true);
            }
        }
    }

private:

    void OnOperationComplete(
        wstring const & src,
        wstring const & dest,
        AsyncOperationSPtr const & operation, 
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = this->OnEndOperation(src, dest, operation);
        WriteNoise(
            TraceComponent,
            "{0} end: src = {1} dest = {2} P-remaining: {3}",
            this->GetOperationName(),
            src,
            dest,
            pendingCount_.load() - 1
        );

        if (!error.IsSuccess())
        {
            ++errorCount_;
            WriteWarning(
                TraceComponent,
                "{0} failed: src = '{1}' dest = '{2}' error = {3} P-remaining: {4}",
                this->GetOperationName(),
                src,
                dest,
                error,
                pendingCount_.load() - 1
            );

            this->SetFirstError(error);
        }

        auto pendingcount = --pendingCount_;
        if (pendingcount == 0)
        {
            this->TryComplete(operation->Parent, this->GetFirstError());
            WriteInfo(
                TraceComponent,
                "{0} P-complete: First Error: {1}, error count:{2}", this->GetOperationName(),
                this->GetFirstError(),
                errorCount_.load()
            );
            return;
        }

        if ((pendingcount % 100 == 0) || pendingcount < 100)
        {
            WriteInfo(
                TraceComponent,
                "{0} P-remaining: {1}", 
                this->GetOperationName(),
                pendingcount
            );
        }
    }

    vector<wstring> sources_;
    vector<wstring> destinations_;
    vector<CopyFlag::Enum> flags_;
    NativeImageStoreProgressStateSPtr progressState_;
    Common::atomic_long pendingCount_;
    Common::atomic_long errorCount_;
};

class NativeImageStore::ParallelUploadObjectsAsyncOperation : public NativeImageStore::ParallelOperationsBaseAsyncOperation
{
public:
    ParallelUploadObjectsAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & workingDirectory,
        vector<wstring> && sources,
        vector<wstring> && destinations,
        vector<CopyFlag::Enum> && flags,
        NativeImageStoreProgressStateSPtr const & progressState,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ParallelOperationsBaseAsyncOperation(
        L"P-UploadObjects",
        client,
        move(sources),
        move(destinations),
        move(flags),
        progressState,
        timeout,
        callback,
        root)
        , workingDirectory_(workingDirectory)
        , isRecursive_(true)
    {
    }

    ParallelUploadObjectsAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & workingDirectory,
        vector<wstring> const& sources,
        vector<wstring> const& destinations,
        vector<CopyFlag::Enum> const& flags,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ParallelOperationsBaseAsyncOperation(
        L"P-UploadObjects",
        client,
        sources,
        destinations,
        flags,
        timeout,
        callback,
        root)
        , workingDirectory_(workingDirectory)
        , isRecursive_(false)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<ParallelUploadObjectsAsyncOperation>(operation)->Error;
    }

protected:
    AsyncOperationSPtr OnBeginOperation(
        wstring const & src,
        wstring const & dest,
        CopyFlag::Enum flag,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
    {
        return AsyncOperation::CreateAndStart<UploadObjectAsyncOperation>(
            this->GetClient(),
            workingDirectory_,
            src,
            dest,
            flag,
            isRecursive_,
            this->GetProgressState(),
            timeout,
            callback,
            root);
    }

private:
    wstring workingDirectory_;
    bool isRecursive_;
};

class NativeImageStore::UploadDirectoryAsyncOperation : public NativeImageStore::OperationBaseAsyncOperation
{
public:

    UploadDirectoryAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & workingDirectory,
        wstring const & localDirectory,
        wstring const & remoteDirectory,
        CopyFlag::Enum const flag,
        NativeImageStoreProgressStateSPtr const & progressState,
        bool isRecursive,  
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(L"UploadDirectory", client, timeout, callback, root)
        , workingDirectory_(workingDirectory)
        , localDirectory_(localDirectory)
        , remoteDirectory_(remoteDirectory)
        , localDirectoryMarker_(Path::Combine(workingDirectory_, Guid::NewGuid().ToString()))
        , remoteDirectoryMarker_(NativeImageStore::GetDirectoryMarkerFileName(remoteDirectory))
        , flag_(flag)
        , progressState_(progressState)
        , uploaded_(false)
        , isRecursive_(isRecursive)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out bool & uploaded)
    {
        auto casted = AsyncOperation::End<UploadDirectoryAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            uploaded = casted->uploaded_;
        }

        return casted->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (isRecursive_)
        {
            if (this->TryCreateDirectoryMarkerFileOrFail(thisSPtr))
            {
                // Skip deleting sub-directories since the top-level
                // operation would have deleted the top-level directory
                // if needed.
                //
                this->StartUpload(thisSPtr);
            }
        }
        else if (flag_ == CopyFlag::Overwrite)
        {
            if (this->TryCreateDirectoryMarkerFileOrFail(thisSPtr))
            {
                this->StartDelete(thisSPtr);
            }
        }
        else
        {
            auto operation = AsyncOperation::CreateAndStart<DirectoryExistsAsyncOperation>(
                this->GetClient(),
                remoteDirectory_,
                this->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnExistsComplete(operation, false); },
                thisSPtr);
            this->OnExistsComplete(operation, true);
        }
    }

    void OnCompleted()
    {
        if(File::Exists(localDirectoryMarker_))
        {
            File::Delete2(localDirectoryMarker_);
        }

        AsyncOperation::OnCompleted();
    }

private:

    bool TryCreateDirectoryMarkerFileOrFail(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = File::Touch(localDirectoryMarker_);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "Failed to create directory marker '{0}': dir = '{1}' error = {2}",
                localDirectoryMarker_,
                Directory::GetCurrentDirectory(),
                error);

            this->TryComplete(thisSPtr, move(error));
        }

        return error.IsSuccess();
    }

    void OnExistsComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        bool exists = false;

        auto error = DirectoryExistsAsyncOperation::End(operation, exists);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "Directory exists failed: '{0}' error = {1}",
                remoteDirectoryMarker_,
                error);

            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            if (exists) 
            { 
                uploaded_ = false;

                this->TryComplete(thisSPtr, ErrorCodeValue::Success);
            }
            else
            {
                if (this->TryCreateDirectoryMarkerFileOrFail(thisSPtr))
                {
                    this->StartDelete(thisSPtr);
                }             
            }
        }
    }

    void StartDelete(AsyncOperationSPtr const & thisSPtr)
    {
        // Server-side must automatically delete all files under this top-level directory.
        // Otherwise, the client would have to first retrieve the list of N files and then
        // send N individual delete requests.
        //
        // For directories, the server marks all contained files deleting in a single transaction.
        //
        auto operation = AsyncOperation::CreateAndStart<DeleteAsyncOperation>(
            this->GetClient(),
            remoteDirectory_,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnParallelDeleteComplete(operation, false); },
            thisSPtr);
        this->OnParallelDeleteComplete(operation, true);
    }

    void OnParallelDeleteComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = DeleteAsyncOperation::End(operation);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            this->StartUpload(thisSPtr);
        }
    }

    void StartUpload(AsyncOperationSPtr const & thisSPtr)
    {
        // Server-side does not support uploading directories to allow interleaving
        // individual file copy operations from multiple folder copies when processing
        // replication operations on the secondaries. So we must upload each individual
        // file and directory (recursively) from the source folder.
        //
        vector<wstring> sources;
        vector<wstring> destinations;
        vector<CopyFlag::Enum> flags;
        
        vector<wstring> srcFiles = Directory::GetFiles(localDirectory_);
        for (auto it=srcFiles.begin(); it!=srcFiles.end(); ++it)
        {
            // Don't upload directory marker until all files at this level are uploaded
            //
            if (*it != *NativeImageStore::DirectoryMarkerFileName)
            {
                sources.push_back(Path::Combine(localDirectory_, *it));
                destinations.push_back(Path::Combine(remoteDirectory_, *it));
                flags.push_back(flag_);
            }
        }

        vector<wstring> srcDirectories = Directory::GetSubDirectories(localDirectory_);
        for (auto it=srcDirectories.begin(); it!=srcDirectories.end(); ++it)
        {
            sources.push_back(Path::Combine(localDirectory_, *it));
            destinations.push_back(Path::Combine(remoteDirectory_, *it));
            flags.push_back(flag_);
        }

        auto operation = AsyncOperation::CreateAndStart<NativeImageStore::ParallelUploadObjectsAsyncOperation>(
            this->GetClient(),
            workingDirectory_,
            move(sources),
            move(destinations),
            move(flags),
            progressState_,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnParallelUploadComplete(operation, false); },
            thisSPtr);
        this->OnParallelUploadComplete(operation, true);
    }

    void OnParallelUploadComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = ParallelUploadObjectsAsyncOperation::End(operation);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "Directory upload failed: '{0}' error = {1}",
                remoteDirectory_,
                error);

            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            // If this fails, then the directory upload has not completed, but files will still have been uploaded.
            // Leave them for some subsequent retry to overwrite.
            //
            auto uploadFileOperation = AsyncOperation::CreateAndStart<UploadFileAsyncOperation>(
                this->GetClient(),
                localDirectoryMarker_,
                remoteDirectoryMarker_,
                flag_,
                progressState_,
                this->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnUploadMarkerComplete(operation, false); },
                thisSPtr);
            this->OnUploadMarkerComplete(uploadFileOperation, true);
        }
    }

    void OnUploadMarkerComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = UploadFileAsyncOperation::End(operation);

        if (error.IsSuccess())
        {
            uploaded_ = true;
        }

        this->TryComplete(thisSPtr, move(error));
    }

    wstring workingDirectory_;
    wstring localDirectory_;
    wstring remoteDirectory_;
    wstring localDirectoryMarker_;
    wstring remoteDirectoryMarker_;
    CopyFlag::Enum flag_;
    NativeImageStoreProgressStateSPtr progressState_;
    bool uploaded_;
    bool isRecursive_;
};

// Uploads either a file or a directory (as multiple file uploads) to the server
//
class NativeImageStore::UploadObjectAsyncOperation : public NativeImageStore::OperationBaseAsyncOperation
{
public:
    UploadObjectAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & workingDirectory,
        wstring const & localFullPath,
        wstring const & remoteRelativePath,
        CopyFlag::Enum const flag,
        bool isRecursive,
        NativeImageStoreProgressStateSPtr const & progressState,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(L"UploadObject", client, timeout, callback, root)
        , workingDirectory_(workingDirectory)
        , localFullPath_(localFullPath)
        , remoteRelativePath_(remoteRelativePath)
        , flag_(flag)
        , progressState_(progressState)
        , uploaded_(false)
        , isFile_(false)
        , isRecursive_(isRecursive)
    {
    }

    UploadObjectAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & workingDirectory,
        wstring const & localFullPath,
        wstring const & remoteRelativePath,
        CopyFlag::Enum const flag,
        bool isRecursive,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(L"UploadObject", client, timeout, callback, root)
        , workingDirectory_(workingDirectory)
        , localFullPath_(localFullPath)
        , remoteRelativePath_(remoteRelativePath)
        , flag_(flag)
        , progressState_()
        , uploaded_(false)
        , isFile_(false)
        , isRecursive_(isRecursive)
    {
    }

    UploadObjectAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & workingDirectory,
        wstring const & localFullPath,
        wstring const & remoteRelativePath,
        CopyFlag::Enum const flag,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(L"UploadObject", client, timeout, callback, root)
        , workingDirectory_(workingDirectory)
        , localFullPath_(localFullPath)
        , remoteRelativePath_(remoteRelativePath)
        , flag_(flag)
        , progressState_()
        , uploaded_(false)
        , isFile_(false)
        , isRecursive_(false)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<UploadObjectAsyncOperation>(operation)->Error;
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out bool & uploaded)
    {
        auto casted = AsyncOperation::End<UploadObjectAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            uploaded = casted->uploaded_;
        }

        return casted->Error;
    }

    NativeImageStoreProgressStateSPtr GetProgressState() const { return progressState_; }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (File::Exists(localFullPath_))
        {
            isFile_ = true;

            auto operation = AsyncOperation::CreateAndStart<UploadFileAsyncOperation>(
                this->GetClient(),
                localFullPath_,
                remoteRelativePath_,
                flag_,
                progressState_,
                this->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation){ this->OnUploadObjectComplete(operation, false); },
                thisSPtr);
            this->OnUploadObjectComplete(operation, true);
        }
        else if (Directory::Exists(localFullPath_))
        {
            isFile_ = false;
                
            auto operation = AsyncOperation::CreateAndStart<UploadDirectoryAsyncOperation>(
                this->GetClient(),
                workingDirectory_,
                localFullPath_,
                remoteRelativePath_,
                flag_,
                progressState_,
                isRecursive_,
                this->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation){ this->OnUploadObjectComplete(operation, false); },
                thisSPtr);
            this->OnUploadObjectComplete(operation, true);
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "Source not found: '{0}'",
                localFullPath_);
            this->TryComplete(thisSPtr, ErrorCodeValue::FileNotFound);
        }
    }

private:

    void OnUploadObjectComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error;

        if (isFile_)
        {
            error = UploadFileAsyncOperation::End(operation, uploaded_);
        }
        else
        {
            error = UploadDirectoryAsyncOperation::End(operation, uploaded_);
        }

        this->TryComplete(thisSPtr, move(error));
    }

    wstring workingDirectory_;
    wstring localFullPath_;
    wstring remoteRelativePath_;
    CopyFlag::Enum flag_;
    NativeImageStoreProgressStateSPtr progressState_;
    bool uploaded_;
    bool isFile_;
    bool isRecursive_;
};

class NativeImageStore::DownloadFileAsyncOperation : public NativeImageStore::OperationBaseAsyncOperation
{
public:
    DownloadFileAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & remoteRelativePath,
        wstring const & localFullPath,
        StoreFileVersion const & version,
        CopyFlag::Enum const flag,
        vector<wstring> const & shares,
        NativeImageStoreProgressStateSPtr const & progressState,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(L"DownloadFile", client, timeout, callback, root)
        , remoteRelativePath_(remoteRelativePath)
        , localFullPath_(localFullPath)
        , version_(version)
        , flag_(flag)
        , shares_(shares)
        , progressState_(progressState)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<DownloadFileAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (flag_ != CopyFlag::Overwrite)
        {
            if (File::Exists(localFullPath_) || Directory::Exists(localFullPath_))
            {
                WriteNoise(
                    TraceComponent,
                    "Destination already exists: '{0}'",
                    localFullPath_);

                this->TryComplete(thisSPtr, ErrorCodeValue::Success);

                return;
            }
        }

        WriteNoise(
            TraceComponent,
            "Downloading '{0}' -> '{1}'",
            remoteRelativePath_,
            localFullPath_);

        auto operation = this->GetClient()->BeginDownload(
            remoteRelativePath_,
            localFullPath_,
            version_,
            shares_,
            IFileStoreServiceClientProgressEventHandlerPtr(progressState_.get(), progressState_),
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnDownloadObjectComplete(operation, false); },
            thisSPtr);
        this->OnDownloadObjectComplete(operation, true);
    }

private:

    void OnDownloadObjectComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = this->GetClient()->EndDownload(operation);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "Download failed: '{0}' to '{1}'",
                remoteRelativePath_,
                localFullPath_);
        }

        this->TryComplete(thisSPtr, move(error));
    }

    wstring remoteRelativePath_;
    wstring localFullPath_;
    StoreFileVersion version_;
    CopyFlag::Enum flag_;
    vector<wstring> shares_;
    NativeImageStoreProgressStateSPtr progressState_;
};

class NativeImageStore::ParallelDownloadFilesAsyncOperation : public NativeImageStore::ParallelOperationsBaseAsyncOperation
{
public:
    ParallelDownloadFilesAsyncOperation(
        wstring const & traceTag,
        shared_ptr<IFileStoreClient> const & client,
        vector<wstring> && sources,
        vector<wstring> && destinations,
        vector<CopyFlag::Enum> && flags,
        NativeImageStoreProgressStateSPtr && progressState,
        StoreFileInfoMap && versions,
        vector<wstring> const & shares,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ParallelOperationsBaseAsyncOperation(
            wformatString("P-DownloadFiles({0})", traceTag), 
            client,
            move(sources),
            move(destinations),
            move(flags),
            move(progressState),
            timeout,
            callback, 
            root)
        , versions_(move(versions))
        , shares_(shares)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<ParallelDownloadFilesAsyncOperation>(operation)->Error;
    }

protected:

    void OnCompleted()
    {
        this->GetProgressState()->StopDispatchingUpdates();

        ParallelOperationsBaseAsyncOperation::OnCompleted();
    }

    AsyncOperationSPtr OnBeginOperation(
        wstring const & src,
        wstring const & dest,
        CopyFlag::Enum flag,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root) override
    {
        auto find_it = versions_.find(src);
        if (find_it == versions_.end())
        {
            WriteWarning(
                TraceComponent,
                "Skip object with missing version: '{0}'",
                src);

            Assert::TestAssert();

            return AsyncOperation::CreateAndStart<OperationBaseAsyncOperation>(
                ErrorCodeValue::Success,
                callback,
                root);
        }

        return AsyncOperation::CreateAndStart<DownloadFileAsyncOperation>(
            this->GetClient(),
            src,
            dest,
            find_it->second.Version,
            flag,
            shares_,
            this->GetProgressState(),
            timeout,
            callback,
            root);
    }

private:
    StoreFileInfoMap versions_;
    vector<wstring> shares_;
};

class NativeImageStore::DownloadObjectAsyncOperation : public NativeImageStore::OperationBaseAsyncOperation
{
public:
    DownloadObjectAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & remoteRelativePath,
        wstring const & localFullPath,
        CopyFlag::Enum flag,
        INativeImageStoreProgressEventHandlerPtr const & progressHandler,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(wformatString("DownloadObject({0})", remoteRelativePath), client, timeout, callback, root)
        , remoteRelativePath_(remoteRelativePath)
        , localFullPath_(localFullPath)
        , flag_(flag)
        , progressHandler_(progressHandler)
        , versions_()
        , shares_()
        , sources_()
        , destinations_()
        , flags_()
        , directoryMarkerStoreFileInfo_()
        , continuationToken_()
        , matchedDirectory_(false)
        , retryCount_(0)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<DownloadObjectAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if((File::Exists(localFullPath_) || Directory::Exists(localFullPath_)) && flag_ != CopyFlag::Enum::Overwrite)
        {
            WriteInfo(
                TraceComponent,
                "Destination '{0}' already exists. Skipping download since its Echo copy.",
                localFullPath_);
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        this->StartList(thisSPtr);
    }

private:
    void StartList(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "Listing '{0}' ContinuationToken: '{1}'",
            remoteRelativePath_,
            continuationToken_);

        auto operation = this->GetClient()->BeginList(
            remoteRelativePath_,
            continuationToken_,
            false, // shouldIncludeDetails
            true, // isRecursive
            true, // isPaging
            GetShortTimeoutInterval(retryCount_),
            [this](AsyncOperationSPtr const & operation){ this->OnListComplete(operation, false); },
            thisSPtr);
        this->OnListComplete(operation, true);
    }

    void OnListComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;
        vector<StoreFolderInfo> folders;
        StoreFileInfoMap currentVersion;

        auto error = this->GetClient()->EndList(operation, currentVersion, folders, shares_, continuationToken_);

        if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::Timeout) && retryCount_ < FileStoreServiceConfig::GetConfig().MaxFileOperationFailureRetryCount)
            {
                ++retryCount_;
                WriteInfo(
                    TraceComponent,
                    "{0}: Exists failed: '{1}' error:{2} retryCount:{3}. Retrying the List operation.",
                    GetOperationName(),
                    remoteRelativePath_,
                    error,
                    retryCount_);

                StartList(thisSPtr);
                return;
            }

            WriteWarning(
                TraceComponent,
                "List failed: '{0}' error = {1}",
                remoteRelativePath_,
                error);

            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            bool matchedFile = false;
           
            // Ensure backslash for filtering. Service should already be
            // doing directory-aware filtering, but don't depend on that.
            //
            auto remoteDirectoryPath = Path::Combine(remoteRelativePath_, L"");

            // Download during provision happens via ImageBuilder.exe, which
            // creates an internal client and copies using SMB so byte-level
            // progress is not available.
            //
            // If the download method changes in the future and we can get
            // byte-level progress, then the total download estimate can
            // be retrieved from FileStoreService by listing with 
            // shouldIncludeDetails=true.
            //
            // In contrast, upload (copy) of the application package that
            // happens in PowerShell goes through the FabricClient upload
            // API, which uses the FileTransferClient and provides byte-level
            // progress details.
            //
            auto progressState = make_shared<NativeImageStoreProgressState>(
                move(progressHandler_),
                wformatString(L"Download({0})", remoteRelativePath_),
                ProgressUnitType::Files);

            for (auto it = currentVersion.begin(); it != currentVersion.end(); ++it)
            {   
                bool addToDownload = false;

#if !defined(PLATFORM_UNIX)
                if (StringUtility::AreEqualCaseInsensitive(it->first, remoteRelativePath_))
#else
                if (AreEqualPaths(it->first, remoteRelativePath_))
#endif
                {
                    matchedFile = true;
                    addToDownload = true;
                    
                    // Download only the exact file if the src is a file.
                    //
                    sources_.clear();
                    destinations_.clear();
                    flags_.clear();
                    directoryMarkerStoreFileInfo_.reset();
                }
#if !defined(PLATFORM_UNIX)
                else if (StringUtility::AreEqualCaseInsensitive(Path::GetFileName(it->first), NativeImageStore::DirectoryMarkerFileName))
#else
                else if (AreEqualPaths(Path::GetFileName(it->first), NativeImageStore::DirectoryMarkerFileName))
#endif
                {
                    addToDownload = false;

#if !defined(PLATFORM_UNIX)
                    if (StringUtility::AreEqualCaseInsensitive(it->first, Path::Combine(remoteRelativePath_, NativeImageStore::DirectoryMarkerFileName)))
#else
                    if (AreEqualPaths(it->first, Path::Combine(remoteRelativePath_, NativeImageStore::DirectoryMarkerFileName)))
#endif
                    {
                        ASSERT_IF(directoryMarkerStoreFileInfo_, "directoryMarkerStoreFileInfo should be set only once");

                        // Download only directories that have completed upload.                    
                        matchedDirectory_ = true;
                        directoryMarkerStoreFileInfo_ = make_shared<StoreFileInfo>(it->second);
                    }
                }
#if !defined(PLATFORM_UNIX)
                else if (StringUtility::StartsWithCaseInsensitive(it->first, remoteDirectoryPath))
#else
                else if (StartsWithPaths(it->first, remoteDirectoryPath))
#endif
                {
                    // Download only contained files and sub-directories, not prefix-matched files.
                    //
                    addToDownload = true;
                }

                if (addToDownload)
                {
                    auto commonPath = it->first.substr(remoteRelativePath_.size());
                    this->versions_.insert(std::pair<std::wstring, StoreFileInfo>(it->first, it->second));

                    sources_.push_back(it->first);
                    destinations_.push_back(
                        wformatString(
                            "{0}{1}",
                            matchedFile ? localFullPath_ : GetTemporaryDirectoryPath(localFullPath_),
                            commonPath));
                    flags_.push_back(flag_);

                    if (matchedFile)
                    {
                        break;
                    }
                }
            } // foreach StoreFileInfo

            WriteInfo(
                TraceComponent,
                "SourceCount: {0}, DestinationCount:{1}, Version={2}",
                sources_.size(),
                destinations_.size(),
                versions_.size());

            if (matchedFile || (matchedDirectory_ && continuationToken_.empty()))
            {
                if (directoryMarkerStoreFileInfo_)
                {
                    ASSERT_IFNOT(matchedDirectory_, "matchedDirectory should be true if directoryMarkerStoreFileInfo is set");
                    Directory::Delete_WithRetry(GetTemporaryDirectoryPath(localFullPath_), true /*isRecursive*/, true /*deleteReadOnly*/);
                }

                progressState->IncrementTotalItems(sources_.size());

                auto parallelDownloadOperation = AsyncOperation::CreateAndStart<ParallelDownloadFilesAsyncOperation>(
                    remoteRelativePath_,
                    this->GetClient(),
                    move(sources_),
                    move(destinations_),
                    move(flags_),
                    move(progressState),
                    move(versions_),
                    shares_,
                    this->GetRemainingTime(),
                    [this](AsyncOperationSPtr const & operation) { this->OnParallelDownloadComplete(operation, false); },
                    thisSPtr);
                this->OnParallelDownloadComplete(parallelDownloadOperation, true);
            }
            else
            {
                if (continuationToken_.empty() && !matchedFile && !matchedDirectory_)
                {
                    WriteInfo(
                        TraceComponent,
                        "No file or completed directory to download for '{0}'",
                        remoteRelativePath_);

                    this->TryComplete(thisSPtr, ErrorCodeValue::FileNotFound);
                    return;
                }

                // Scheduled for retrieving more iamge store content
                error = TryScheduleRetry(
                    operation->Parent,
                    [this](AsyncOperationSPtr const & thisSPtr) { this->StartList(thisSPtr); });
                if (!error.IsSuccess())
                {
                    this->TryComplete(thisSPtr, move(error));
                }

                return;
            }
        }
    }

    void OnParallelDownloadComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }        

        auto error = ParallelDownloadFilesAsyncOperation::End(operation);

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        if(error.IsSuccess() && directoryMarkerStoreFileInfo_)
        {
            this->ValidateDirectoryMarkerFile(directoryMarkerStoreFileInfo_, thisSPtr);
            return;
        }

        this->TryComplete(thisSPtr, move(error));
    }

    void ValidateDirectoryMarkerFile(shared_ptr<StoreFileInfo> const & directoryMarkerStoreFileInfo, AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<ListFileAsyncOperation>(
            this->GetClient(),
            directoryMarkerStoreFileInfo->StoreRelativePath,
            L"",
            false /*isRecursive*/,
            false /*isPaging*/,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnListDirectoryMarkerFileComplete(operation, false); },
            thisSPtr);
        this->OnListDirectoryMarkerFileComplete(operation, true);
    }

    void OnListDirectoryMarkerFileComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if(operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        StoreContentInfo contentInfo;
        auto error = ListFileAsyncOperation::End(operation, contentInfo);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "List of directory marker file failed: '{0}' error = {1}",
                directoryMarkerStoreFileInfo_->StoreRelativePath,
                error);
            this->TryComplete(operation->Parent, error);
            return;
        }
        
        for (auto & storeFile : contentInfo.StoreFiles)
        {
            if(StringUtility::AreEqualCaseInsensitive(storeFile.StoreRelativePath, directoryMarkerStoreFileInfo_->StoreRelativePath) &&
                storeFile.Version == directoryMarkerStoreFileInfo_->Version)
            {
                error = Directory::Rename_WithRetry(
                    GetTemporaryDirectoryPath(localFullPath_),
                    localFullPath_,
                    true);
                
                WriteTrace(
                    error.ToLogLevel(),
                    TraceComponent,
                    "Renaming {0} to {1}. Error = {2}",
                    GetTemporaryDirectoryPath(localFullPath_),
                    localFullPath_,
                    error);

                this->TryComplete(operation->Parent, error);
                return;
            }
        }
        
        WriteWarning(
            TraceComponent,
            "{0} is no longer found. Failing download with FileNotFound.",
            *directoryMarkerStoreFileInfo_);

        this->TryComplete(operation->Parent, ErrorCodeValue::FileNotFound);
        return;
    }

    wstring GetTemporaryDirectoryPath(wstring const & localFullPath)
    {
        return wformatString("{0}.isstmp", localFullPath);
    }

    wstring remoteRelativePath_;
    wstring localFullPath_;
    CopyFlag::Enum flag_;
    INativeImageStoreProgressEventHandlerPtr progressHandler_;
    StoreFileInfoMap versions_;
    vector<wstring> shares_;
    uint retryCount_;
    vector<wstring> sources_;
    vector<wstring> destinations_;
    vector<CopyFlag::Enum> flags_;
    shared_ptr<StoreFileInfo> directoryMarkerStoreFileInfo_;
    wstring continuationToken_;
    bool matchedDirectory_;
};

class NativeImageStore::CopyFileAsyncOperation : public NativeImageStore::OperationBaseAsyncOperation
{
public:
    CopyFileAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & source,
        wstring const & destination,
        StoreFileVersion const & version,
        CopyFlag::Enum flag,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(L"CopyFile", client, timeout, callback, root)
        , source_(source)
        , destination_(destination)
        , version_(version)
        , flag_(flag)
        , copied_(false)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CopyFileAsyncOperation>(operation)->Error;
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out bool & copied)
    {
        auto casted = AsyncOperation::End<CopyFileAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            copied = casted->copied_;
        }

        return casted->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "Copying '{0}' -> '{1}'",
            source_,
            destination_);
        
        auto operation = this->GetClient()->BeginCopy(
            source_,
            version_,
            destination_,
            (flag_ == CopyFlag::Overwrite),
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnCopyComplete(operation, false); },
            thisSPtr);
        this->OnCopyComplete(operation, true);
    }

    void OnCopyComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = this->GetClient()->EndCopy(operation);
        if (error.IsSuccess())
        {
            copied_ = true;
        }
        else if (error.IsError(ErrorCodeValue::FileAlreadyExists))
        {
            error = ErrorCodeValue::Success;
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "File copy failed: '{0}' error = {1}",
                destination_,
                error);
        }

        this->TryComplete(operation->Parent, error);
    }

private:
    wstring const source_;
    wstring const destination_;
    StoreFileVersion const version_;
    CopyFlag::Enum const flag_;

    bool copied_;
};

class NativeImageStore::ParallelCopyFilesAsyncOperation : public NativeImageStore::ParallelOperationsBaseAsyncOperation
{
public:
public:
    ParallelCopyFilesAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        vector<wstring> && sources,
        vector<wstring> && destinations,        
        StoreFileInfoMap && versions,
        vector<CopyFlag::Enum> && flags,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ParallelOperationsBaseAsyncOperation(
        L"P-CopyFiles",
        client,
        move(sources),
        move(destinations),
        move(flags),
        timeout,
        callback,
        root)
        , versions_(move(versions))
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<ParallelCopyFilesAsyncOperation>(operation)->Error;
    }

protected:

    AsyncOperationSPtr OnBeginOperation(
        wstring const & src,
        wstring const & dest,
        CopyFlag::Enum flag,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
    {
        auto find_it = versions_.find(src);
        if (find_it == versions_.end())
        {
            WriteWarning(
                TraceComponent,
                "Skip object with missing version: '{0}'",
                src);

            Assert::TestAssert();

            return AsyncOperation::CreateAndStart<OperationBaseAsyncOperation>(
                ErrorCodeValue::Success,
                callback,
                root);
        }

        return AsyncOperation::CreateAndStart<CopyFileAsyncOperation>(
            this->GetClient(),
            src,
            dest,
            find_it->second.Version,
            flag,
            timeout,
            callback,
            root);
    }

private:
    StoreFileInfoMap versions_;
};

class NativeImageStore::CopyDirectoryAsyncOperation : public NativeImageStore::OperationBaseAsyncOperation
{
public:
    CopyDirectoryAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        vector<wstring> && sources,
        vector<wstring> && destinations,
        StoreFileInfoMap && versions,
        CopyFlag::Enum flag,
        StoreFileInfo const & sourceDirectoryCompletionMarker,
        wstring const & destinationRoot,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(L"CopyDirectory", client, timeout, callback, root)
        , sources_(move(sources))
        , destinations_(move(destinations))
        , versions_(move(versions))
        , flag_(flag)
        , sourceDirectoryCompletionMarker_(sourceDirectoryCompletionMarker)
        , destinationRoot_(destinationRoot)
    {
    
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CopyDirectoryAsyncOperation>(operation)->Error;
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out bool & copied)
    {
        auto casted = AsyncOperation::End<CopyDirectoryAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            copied = casted->copied_;
        }

        return casted->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {      
        WriteInfo(
            TraceComponent,
            "SourceCount:{0}, DestinationCount:{1}, VersionsCount:{2}",
            sources_.size(),
            destinations_.size(),
            versions_.size());

        // All flag values will be the same. So pick the first one
        if (flag_ == CopyFlag::Overwrite)
        {
            this->StartDelete(thisSPtr);
        }
        else
        {
            auto operation = AsyncOperation::CreateAndStart<DirectoryExistsAsyncOperation>(
                this->GetClient(),
                destinationRoot_,
                this->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnExistsComplete(operation, false); },
                thisSPtr);
            this->OnExistsComplete(operation, true);
        }
    }   

private:    
    void OnExistsComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        bool exists = false;

        auto error = DirectoryExistsAsyncOperation::End(operation, exists);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "Directory exists failed: '{0}' error = {1}",
                destinationRoot_,
                error);

            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            if (exists)
            {
                copied_ = false;
                this->TryComplete(thisSPtr, ErrorCodeValue::Success);
            }
            else
            {
                this->StartDelete(thisSPtr);
            }
        }
    }

    void StartDelete(AsyncOperationSPtr const & thisSPtr)
    {
        // Server-side must automatically delete all files under this top-level directory.
        // Otherwise, the client would have to first retrieve the list of N files and then
        // send N individual delete requests.
        //
        // For directories, the server marks all contained files deleting in a single transaction.
        //
        auto operation = AsyncOperation::CreateAndStart<DeleteAsyncOperation>(
            this->GetClient(),
            destinationRoot_,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnParallelDeleteComplete(operation, false); },
            thisSPtr);
        this->OnParallelDeleteComplete(operation, true);
    }

    void OnParallelDeleteComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = DeleteAsyncOperation::End(operation);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "Directory delete failed: '{0}' error = {1}",
                destinationRoot_,
                error);

            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            this->StartCopy(thisSPtr);
        }
    }

    void StartCopy(AsyncOperationSPtr const & thisSPtr)
    {      
        // Copy flag will be same for all the items
        vector<CopyFlag::Enum> flags;
        for(int i = 0; i < destinations_.size(); ++i)
        {
            flags.push_back(flag_);
        }

        auto operation = AsyncOperation::CreateAndStart<ParallelCopyFilesAsyncOperation>(
            this->GetClient(),
            move(sources_),
            move(destinations_),
            move(versions_),
            move(flags),
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnParallelCopyComplete(operation, false); },
            thisSPtr);
        this->OnParallelCopyComplete(operation, true);
    }

    void OnParallelCopyComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = ParallelCopyFilesAsyncOperation::End(operation);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "Directory copy failed: '{0}' error = {1}",
                destinationRoot_,
                error);

            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            // If this fails, then the directory upload has not completed, but files will still have been copied.
            // Leave them for some subsequent retry to overwrite.            
            auto copyFileOperation = AsyncOperation::CreateAndStart<CopyFileAsyncOperation>(
                this->GetClient(),
                sourceDirectoryCompletionMarker_.StoreRelativePath,                
                Path::Combine(destinationRoot_, DirectoryMarkerFileName),
                sourceDirectoryCompletionMarker_.Version,
                CopyFlag::Overwrite,
                this->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnCopyMarkerComplete(operation, false); },
                thisSPtr);
            this->OnCopyMarkerComplete(copyFileOperation, true);
        }
    }

    void OnCopyMarkerComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = CopyFileAsyncOperation::End(operation);

        if (error.IsSuccess())
        {
            copied_ = true;
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "DirectoryMarker file copy failed: '{0}' error = {1}",
                destinationRoot_,
                error);
        }

        this->TryComplete(thisSPtr, move(error));
    }

private:
    StoreFileInfo const sourceDirectoryCompletionMarker_;
    wstring const destinationRoot_;
    vector<wstring> sources_;
    vector<wstring> destinations_;
    StoreFileInfoMap versions_;
    CopyFlag::Enum flag_;

    bool copied_;
};

class NativeImageStore::CopyObjectAsyncOperation : public NativeImageStore::OperationBaseAsyncOperation
{
public:
    CopyObjectAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & sourceStoreRelativePath,
        wstring const & destStoreRelativePath,
        shared_ptr<vector<wstring>> const & skipFiles,
        CopyFlag::Enum flag,
        BOOLEAN checkMarkFile,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : OperationBaseAsyncOperation(L"CopyObject", client, timeout, callback, root)
        , sourceStoreRelativePath_(sourceStoreRelativePath)
        , destStoreRelativePath_(destStoreRelativePath)
        , skipFiles_(skipFiles)
        , flag_(flag)
        , checkMakeFile_(checkMarkFile)
        , versions_()
        , copied_(false)
        , copyingSources_()
        , copyingDestinations_()
        , continuationToken_()
        , sourceDirectoryMarkerFile_()
        , matchedDirectory_(false)
        , retryCount_(0)
    {		
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CopyObjectAsyncOperation>(operation)->Error;
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out bool & copied)
    {
        auto casted = AsyncOperation::End<CopyObjectAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            copied = casted->copied_;
        }

        return casted->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->StartList(thisSPtr);
    }

private:
    void StartList(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "Listing '{0}'",
            sourceStoreRelativePath_);

        auto operation = this->GetClient()->BeginList(
            sourceStoreRelativePath_,
            continuationToken_,
            false,
            true,
            true,
            GetShortTimeoutInterval(retryCount_),
            [this](AsyncOperationSPtr const & operation){ this->OnListComplete(operation, false); },
            thisSPtr);
        this->OnListComplete(operation, true);
    }

    void OnListComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        vector<wstring> shares;
        vector<StoreFolderInfo> folders;
        AsyncOperationSPtr thisSPtr = operation->Parent;
        wstring continuationToken;
        StoreFileInfoMap currentVersion;

        auto error = this->GetClient()->EndList(operation, currentVersion, folders, shares, continuationToken_);

        if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::Timeout) && retryCount_ < FileStoreServiceConfig::GetConfig().MaxFileOperationFailureRetryCount)
            {
                ++retryCount_;
                WriteInfo(
                    TraceComponent,
                    "{0}: Exists failed: '{1}' error:{2} retryCount:{3}. Retrying the List operation.",
                    GetOperationName(),
                    sourceStoreRelativePath_,
                    error,
                    retryCount_);

                StartList(thisSPtr);
                return;
            }

            WriteInfo(
                TraceComponent,
                "List failed: '{0}' error = {1}",
                sourceStoreRelativePath_,
                error);

            this->TryComplete(thisSPtr, move(error));
            return;
        }

        bool matchedFile = false;
       
        // Ensure backslash for filtering. Service should already be
        // doing directory-aware filtering, but don't depend on that.
        //
        auto remoteSourceDirectoryPath = Path::Combine(sourceStoreRelativePath_, L"");
        
        for (auto it = currentVersion.begin(); it != currentVersion.end(); ++it)
        {
            bool addToCopy = false;

#if !defined(PLATFORM_UNIX)
            if (StringUtility::AreEqualCaseInsensitive(it->first, sourceStoreRelativePath_))
#else
            if (AreEqualPaths(it->first, sourceStoreRelativePath_))
#endif
            {
                matchedFile = true;
                addToCopy = true;

                // Copy only the exact file if the src is a file.
                //
                copyingSources_.clear();
                copyingDestinations_.clear();
            }
#if !defined(PLATFORM_UNIX)
            else if (StringUtility::AreEqualCaseInsensitive(it->first, Path::Combine(remoteSourceDirectoryPath, NativeImageStore::DirectoryMarkerFileName)))
#else
            else if (AreEqualPaths(it->first, Path::Combine(remoteSourceDirectoryPath, NativeImageStore::DirectoryMarkerFileName)))
#endif
            {
                // Copy only directories that have completed upload.
                matchedDirectory_ = true;
                sourceDirectoryMarkerFile_ = it->second;
            }
#if !defined(PLATFORM_UNIX)
            else if (StringUtility::StartsWithCaseInsensitive(it->first, remoteSourceDirectoryPath))
#else
            else if (StartsWithPaths(it->first, remoteSourceDirectoryPath))
#endif
            {
                // Download only contained files and sub-directories, not prefix-matched files.
                //
                addToCopy = true;
            }

            if (addToCopy)
            {
                wstring commonPath = it->first.substr(sourceStoreRelativePath_.size());

                bool shouldSkipFile = false;
                if (skipFiles_)
                {
                    for (auto const & skipFile : *skipFiles_)
                    {
#if !defined(PLATFORM_UNIX)
                        if (StringUtility::AreEqualCaseInsensitive(Path::GetFileName(commonPath), skipFile))
#else
                        if (AreEqualPaths(Path::GetFileName(commonPath), skipFile))
#endif
                        {
                            shouldSkipFile = true;
                            break;
                        }
                    }
                }

                if (!shouldSkipFile)
                {
                    this->versions_.insert(std::pair<std::wstring, StoreFileInfo>(it->first, it->second));

                    copyingSources_.push_back(it->first);
                    copyingDestinations_.push_back(wformatString("{0}{1}", destStoreRelativePath_, commonPath));
                }
                else
                {
                    Trace.WriteInfo(TraceComponent, "{0} is skipped", it->first);
                }

                if (matchedFile)
                {
                    break;
                }
            }
        }

        if (!matchedFile && !matchedDirectory_)
        {
            WriteInfo(
                TraceComponent,
                "No file or completed directory to copy for '{0}'",
                sourceStoreRelativePath_);

            this->TryComplete(thisSPtr, ErrorCodeValue::FileNotFound);
            return;
        }

        if (matchedFile)
        {
            TESTASSERT_IFNOT(copyingSources_.size() == 1, "sources has {0} items.", copyingSources_.size());
            TESTASSERT_IFNOT(copyingDestinations_.size() == 1, "destinations has {0} items.", copyingDestinations_.size());

            auto copyFileOperation = AsyncOperation::CreateAndStart<CopyFileAsyncOperation>(
                this->GetClient(),
                copyingSources_[0],
                copyingDestinations_[0],
                versions_.find(copyingSources_[0])->second.Version,
                flag_,
                this->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnCopyComplete(operation, false /*isDirectory*/, false); },
                thisSPtr);
            this->OnCopyComplete(copyFileOperation, false /*isDirectory*/, true);
        }
        else
        {
            if (continuationToken_.empty())
            {
                auto copyDirectoryOperation = AsyncOperation::CreateAndStart<CopyDirectoryAsyncOperation>(
                    this->GetClient(),
                    move(copyingSources_),
                    move(copyingDestinations_),
                    move(versions_),
                    flag_,
                    sourceDirectoryMarkerFile_,
                    destStoreRelativePath_,
                    this->GetRemainingTime(),
                    [this](AsyncOperationSPtr const & operation) { this->OnCopyComplete(operation, true /*isDirectory*/, false); },
                    thisSPtr);
                this->OnCopyComplete(copyDirectoryOperation, true /*isDirectory*/, true);
            }
            else
            {
                // Scheduled for retriving more image store content
                error = TryScheduleRetry(
                    operation->Parent,
                    [this](AsyncOperationSPtr const & thisSPtr) { this->StartList(thisSPtr); });
                if (!error.IsSuccess())
                {
                    this->TryComplete(thisSPtr, move(error));    
                }

                return;
            }
        }
    }

private:

    void OnCopyComplete(AsyncOperationSPtr const & operation, bool isDirectory, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error;
        if (isDirectory)
        {
            error = CopyDirectoryAsyncOperation::End(operation, copied_);
        }
        else
        {
            error = CopyFileAsyncOperation::End(operation, copied_);
        }

        this->TryComplete(thisSPtr, move(error));
    }

    wstring const sourceStoreRelativePath_;
    wstring const destStoreRelativePath_;
    shared_ptr<vector<wstring>> skipFiles_;
    CopyFlag::Enum const flag_;
    BOOLEAN const checkMakeFile_;

    StoreFileInfoMap versions_;
    bool copied_;
    vector<wstring> copyingSources_;
    vector<wstring> copyingDestinations_;
    std::wstring continuationToken_;
    StoreFileInfo sourceDirectoryMarkerFile_;
    bool matchedDirectory_;
    uint retryCount_;
};

class NativeImageStore::ParallelDownloadObjectsAsyncOperation : public NativeImageStore::ParallelOperationsBaseAsyncOperation
{
public:
    ParallelDownloadObjectsAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        vector<wstring> && sources,
        vector<wstring> && destinations,
        vector<CopyFlag::Enum> && flags,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ParallelOperationsBaseAsyncOperation(
            L"P-DownloadObjects", 
            client,
            move(sources),
            move(destinations),
            move(flags),
            timeout,
            callback, 
            root)
    {
    }

    ParallelDownloadObjectsAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        vector<wstring> const& sources,
        vector<wstring> const& destinations,
        vector<CopyFlag::Enum> const& flags,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ParallelOperationsBaseAsyncOperation(
            L"P-DownloadObjects", 
            client,
            sources,
            destinations,
            flags,
            timeout,
            callback, 
            root)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<ParallelDownloadObjectsAsyncOperation>(operation)->Error;
    }

protected:

    AsyncOperationSPtr OnBeginOperation(
        wstring const & src,
        wstring const & dest,
        CopyFlag::Enum flag,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
    {
        return AsyncOperation::CreateAndStart<DownloadObjectAsyncOperation>(
            this->GetClient(),
            src,
            dest,
            flag,
            INativeImageStoreProgressEventHandlerPtr(),
            timeout,
            callback,
            root);
    }
};

class NativeImageStore::ParallelExistsAsyncOperation : public NativeImageStore::ParallelOperationsBaseAsyncOperation
{
public:
    ParallelExistsAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        vector<wstring> && targets,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ParallelOperationsBaseAsyncOperation(
            L"P-Exists", 
            client,
            vector<wstring>(),
            move(targets),
            vector<CopyFlag::Enum>(),
            timeout,
            callback, 
            root)
        , exists_()
    {
    }

    ParallelExistsAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        vector<wstring> const& targets,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ParallelOperationsBaseAsyncOperation(
            L"P-Exists", 
            client,
            vector<wstring>(),
            targets,
            vector<CopyFlag::Enum>(),
            timeout,
            callback, 
            root)
        , exists_()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out map<wstring, bool> & exists)
    {
        auto casted = AsyncOperation::End<ParallelExistsAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            exists = move(casted->exists_);
        }

        return casted->Error;
    }

protected:

    AsyncOperationSPtr OnBeginOperation(
        wstring const & src,
        wstring const & dest,
        CopyFlag::Enum flag,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
    {
        UNREFERENCED_PARAMETER(src);
        UNREFERENCED_PARAMETER(flag);
        
        return AsyncOperation::CreateAndStart<ObjectExistsAsyncOperation>(
            this->GetClient(),
            dest,
            timeout,
            callback,
            root);
    }

    ErrorCode OnEndOperation(wstring const & src, wstring const & dest, AsyncOperationSPtr const & operation)
    {
        UNREFERENCED_PARAMETER(src);

        bool exists = false;
        auto error = ObjectExistsAsyncOperation::End(operation, exists);

        if (error.IsSuccess())
        {
            exists_[dest] = exists;
        }

        return error;
    }

private:
    map<wstring, bool> exists_;
};

class NativeImageStore::ParallelDeleteAsyncOperation : public NativeImageStore::ParallelOperationsBaseAsyncOperation
{
public:
    ParallelDeleteAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        vector<wstring> && targets,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ParallelOperationsBaseAsyncOperation(
            L"P-Delete", 
            client,
            vector<wstring>(),
            move(targets),
            vector<CopyFlag::Enum>(),
            timeout,
            callback, 
            root)
    {
    }

    ParallelDeleteAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        vector<wstring> const& targets,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ParallelOperationsBaseAsyncOperation(
            L"P-Delete", 
            client,
            vector<wstring>(),
            targets,
            vector<CopyFlag::Enum>(),
            timeout,
            callback, 
            root)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<ParallelDeleteAsyncOperation>(operation)->Error;
    }

protected:

    AsyncOperationSPtr OnBeginOperation(
        wstring const & src,
        wstring const & dest,
        CopyFlag::Enum flag,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
    {
        UNREFERENCED_PARAMETER(src);
        UNREFERENCED_PARAMETER(flag);

        return AsyncOperation::CreateAndStart<DeleteAsyncOperation>(
            this->GetClient(),
            dest,
            timeout,
            callback,
            root);
    }
};

class NativeImageStore::ListUploadSessionAsyncOperation 
    : public NativeImageStore::OperationBaseAsyncOperation
{
public: 

    ListUploadSessionAsyncOperation(
        shared_ptr<IFileStoreClient> const& client,
        wstring const & remoteLocation,
        Common::Guid const & sessionId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const parent)
        : OperationBaseAsyncOperation(L"ListUploadSession", client, timeout, callback, parent)
        , remoteLocation_(remoteLocation)
        , sessionId_(sessionId)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out UploadSession & result)
    {
        auto casted = AsyncOperation::End<ListUploadSessionAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            result = casted->result_;
        }

        return casted->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "Listing upload session '{0}'",
            this->sessionId_);

        auto listUploadSessionOperation = this->GetClient()->BeginListUploadSession(
            this->remoteLocation_,
            this->sessionId_,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnListUploadSessionComplete(operation, false); },
            thisSPtr);
            
        this->OnListUploadSessionComplete(listUploadSessionOperation, true);
    }

private:

    void OnListUploadSessionComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;
        auto error = this->GetClient()->EndListUploadSession(operation, this->result_);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "Checking upload session failed: '{0}' error = {1}",
                this->sessionId_,
                error);
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "Checking upload session succeeded: '{0}' result = {1}",
                this->sessionId_,
                this->result_.UploadSessions.size());
        }

        this->TryComplete(thisSPtr, move(error));
    }

    wstring remoteLocation_;
    Common::Guid sessionId_;
    UploadSession result_;
};

class NativeImageStore::UploadChunkAsyncOperation 
    : public NativeImageStore::OperationBaseAsyncOperation
{
public:

    UploadChunkAsyncOperation(
        shared_ptr<IFileStoreClient> const & client,
        wstring const & remoteDestination,
        wstring const & localSource,
        Common::Guid const & sessionId,
        uint64 startPosition,
        uint64 endPosition,
        uint64 fileSize,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const parent)
        : OperationBaseAsyncOperation(L"UploadChunk", client, timeout, callback, parent)
        , remoteDestination_(remoteDestination)
        , localSource_(localSource)
        , sessionId_(sessionId)
        , startPosition_(startPosition)
        , endPosition_(endPosition)
        , fileSize_(fileSize)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<UploadChunkAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const& thisSPtr)
    {
        if (this->fileSize_ < this->endPosition_ || this->startPosition_ > this->endPosition_)
        {
            WriteWarning(
                TraceComponent,
                "Invalid range:'{0}'~'{1}','{2}'",
                this->startPosition_,
                this->endPosition_,
                this->fileSize_);

            this->TryComplete(thisSPtr, ErrorCodeValue::UploadSessionRangeNotSatisfiable);
            return;
        }

        auto listUploadSessionOperation = this->GetClient()->BeginListUploadSession(
            L"",
            this->sessionId_,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnListUploadSessionComplete(operation, false); },
            thisSPtr);

        this->OnListUploadSessionComplete(listUploadSessionOperation, true);
    }

private:

    void OnListUploadSessionComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) 
        { 
            return; 
        }
        
        AsyncOperationSPtr const & thisSPtr = operation->Parent;
        UploadSession result;
        auto error = this->GetClient()->EndListUploadSession(operation, result);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "ListUploadSession failed:'{0}',Error:{1}",
                sessionId_,
                error);

            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            if (result.UploadSessions.size() == 0)
            {
                auto createUploadSessionOperation = this->GetClient()->BeginCreateUploadSession(
                    this->remoteDestination_,
                    this->sessionId_,
                    this->fileSize_,
                    this->GetRemainingTime(),
                    [this](AsyncOperationSPtr const & operation) { this->OnCreateUploadSessionComplete(operation, false); },
                    thisSPtr);

                this->OnCreateUploadSessionComplete(createUploadSessionOperation, true);
            }
            else
            {
                if (StringUtility::Compare(result.UploadSessions.front().StoreRelativeLocation, this->remoteDestination_) != 0)
                {
                    WriteWarning(
                        TraceComponent,
                        "SessionId conflict:{0}, '{1}'-'{2}'",
                        this->sessionId_,
                        result.UploadSessions.front().StoreRelativeLocation,
                        this->remoteDestination_);

                    this->TryComplete(thisSPtr, ErrorCodeValue::UploadSessionIdConflict);
                    return;
                }

                if (result.UploadSessions.front().FileSize != this->fileSize_)
                {
                    WriteWarning(
                        TraceComponent,
                        "Session file size mismatch:{0}",
                        this->sessionId_);

                    this->TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
                    return;
                }

                bool isValid = false;
                for (std::vector<UploadSessionInfo::ChunkRangePair>::const_iterator it = result.UploadSessions.front().ExpectedRanges.begin(); it != result.UploadSessions.front().ExpectedRanges.end(); ++it)
                {
                    if (it->StartPosition <= this->startPosition_ && it->EndPosition >= this->endPosition_)
                    {
                        isValid = true;
                        break;
                    }
                }

                if (!isValid)
                {
                    WriteWarning(
                        TraceComponent,
                        "Range conflict:'{0}'~'{1}'",
                        this->startPosition_,
                        this->endPosition_);

                    this->TryComplete(thisSPtr, ErrorCodeValue::UploadSessionRangeNotSatisfiable);
                    return;
                }

                this->StartUploadChunk(thisSPtr);
            }
        }
    }

    void OnCreateUploadSessionComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = this->GetClient()->EndCreateUploadSession(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            "CreateUploadSession:{0},Error:{1}",
            this->sessionId_,
            error);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        this->StartUploadChunk(thisSPtr);
    }

    void StartUploadChunk(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "UploadChunk:'{0}'->'{1}'~'{2}'",
            this->sessionId_,
            this->startPosition_,
            this->endPosition_);
           
        auto uploadSessionOperation = this->GetClient()->BeginUploadChunk(
            this->localSource_,
            this->sessionId_,
            this->startPosition_,
            this->endPosition_,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnUploadChunkComplete(operation, false); },
            thisSPtr);
            
        this->OnUploadChunkComplete(uploadSessionOperation, true);
    }

    void OnUploadChunkComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = this->GetClient()->EndUploadChunk(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            "UploadChunk:'{0}' -> '{1}'~'{2}',Error:{3}",
            this->sessionId_,
            this->startPosition_,
            this->endPosition_,
            error);
          
        this->TryComplete(thisSPtr, move(error));
    }
    
    wstring remoteDestination_;
    wstring localSource_;
    Common::Guid sessionId_;
    uint64 startPosition_;
    uint64 endPosition_;
    uint64 fileSize_;
    bool uploaded_;
};

class NativeImageStore::DeleteUploadSessionAsyncOperation
    : public NativeImageStore::OperationBaseAsyncOperation
{
public:

    DeleteUploadSessionAsyncOperation(
        shared_ptr<IFileStoreClient> const& client,
        Common::Guid const & sessionId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : OperationBaseAsyncOperation(L"DeleteUploadSession", client, timeout, callback, parent)
        , sessionId_(sessionId)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<DeleteUploadSessionAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "DeleteUploadSession:'{0}'",
            this->sessionId_);

        auto deleteUploadSessionOperation = this->GetClient()->BeginDeleteUploadSession(
            this->sessionId_,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnDeleteUploadSessionComplete(operation, false); },
            thisSPtr);

        this->OnDeleteUploadSessionComplete(deleteUploadSessionOperation, true);
    }

private:

    void OnDeleteUploadSessionComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;
        auto error = this->GetClient()->EndDeleteUploadSession(operation);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "DeleteUuploadSession failed:'{0}',Error = {1}",
                this->sessionId_,
                error);
        }
       
        this->TryComplete(thisSPtr, move(error));
    }

    Common::Guid const & sessionId_;
};

class NativeImageStore::CommitUploadSessionAsyncOperation
    : public NativeImageStore::OperationBaseAsyncOperation
{
public:
    CommitUploadSessionAsyncOperation(
        shared_ptr<IFileStoreClient> const& client,
        Common::Guid const & sessionId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : OperationBaseAsyncOperation(L"CommitUploadSession", client, timeout, callback, parent)
        , sessionId_(sessionId)
    {
    
    }
    
    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CommitUploadSessionAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "DeleteUploadSession:'{0}'",
            this->sessionId_);

        auto listUploadSessionOperation = this->GetClient()->BeginListUploadSession(
            L"",
            this->sessionId_,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnListUploadSessionComplete(operation, false); },
            thisSPtr);

        this->OnListUploadSessionComplete(listUploadSessionOperation, true);
    }

private:

    void OnListUploadSessionComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        UploadSession result;
        auto error = this->GetClient()->EndListUploadSession(operation, result);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "ListUploadSession failed:'{0}',Error:{1}",
                sessionId_,
                error);

            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            if (result.UploadSessions.empty())
            {
                WriteWarning(
                    TraceComponent,
                    "Invalid SessionId:'{0}'",
                    this->sessionId_);

                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
                return;
            }

            if (!result.UploadSessions.begin()->ExpectedRanges.empty())
            {
                WriteWarning(
                    TraceComponent,
                    "Incompleted session:'{0}'",
                    this->sessionId_);

                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
                return;
            }

            std::wstring storeRelativePath = result.UploadSessions.begin()->StoreRelativeLocation;
            auto commitUploadSessionOperation = this->GetClient()->BeginCommitUploadSession(
                storeRelativePath,
                this->sessionId_,
                this->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnCommitUploadSessionComplete(operation, false); },
                thisSPtr);

            this->OnCommitUploadSessionComplete(commitUploadSessionOperation, true);
        }
    }

    void OnCommitUploadSessionComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = this->GetClient()->EndCommitUploadSession(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            "CommitUploadSession:{0},Error:{1}",
            this->sessionId_,
            error);

        this->TryComplete(thisSPtr, move(error));
    }

    Common::Guid sessionId_;
};

//
// class NativeImageStore
//
NativeImageStore::NativeImageStore(
    wstring const & rootUri,
    wstring const & imageCacheDirectory,
    wstring const & workingDirectory, 
    std::shared_ptr<FileStoreService::IFileStoreClient> const & client)
    : IImageStore(L"", imageCacheDirectory)
    , workingDirectory_(workingDirectory)
    , client_(client)
{
    WriteInfo(
        TraceComponent,
        "ctor: root = {0} working = {1}",
        rootUri,
        workingDirectory_);

    markFile_ = std::wstring(L"_.dir");
}

ErrorCode NativeImageStore::CreateNativeImageStoreClient(
    bool const isInternal,
    wstring const & imageCacheDirectory,
    wstring const & workingDirectory,    
    Api::IClientFactoryPtr const & clientFactory,
    __out ImageStoreUPtr & imageStoreClientUPtr)
{
    shared_ptr<FileStoreService::IFileStoreClient> fileStoreClient;
    auto error = GetFileStoreClient(isInternal, clientFactory, fileStoreClient);
    if(!error.IsSuccess())
    {
        return error;
    }

    imageStoreClientUPtr = make_unique<NativeImageStore>(
        *Management::ImageStore::Constants::NativeImageStoreSchemaTag,
        imageCacheDirectory,
        workingDirectory, 
        fileStoreClient);

    return ErrorCodeValue::Success;
}

ErrorCode NativeImageStore::CreateNativeImageStoreClient(
    bool const isInternal,
    wstring const & workingDirectory,
    Api::IClientFactoryPtr const & clientFactory,
    __out Api::INativeImageStoreClientPtr & imageStoreClient)
{
    shared_ptr<FileStoreService::IFileStoreClient> fileStoreClient;
    auto error = GetFileStoreClient(isInternal, clientFactory, fileStoreClient);
    if(!error.IsSuccess())
    {
        return error;
    }

    auto nativeImageStorePtr = make_shared<NativeImageStore>(
        *Management::ImageStore::Constants::NativeImageStoreSchemaTag,
        L"" /*imageCacheDirectory*/,
        workingDirectory,
        fileStoreClient);

    imageStoreClient = RootedObjectPointer<Api::INativeImageStoreClient>(
        nativeImageStorePtr.get(), 
        nativeImageStorePtr->CreateComponentRoot());

    return ErrorCodeValue::Success;
}

ErrorCode NativeImageStore::GetFileStoreClient(
    bool const isInternal, 
    IClientFactoryPtr const & clientFactory,    
    __out shared_ptr<FileStoreService::IFileStoreClient> & fileStoreClient)
{        
    if(isInternal)
    {
        fileStoreClient = make_shared<InternalFileStoreClient>(
            *SystemServiceApplicationNameHelper::PublicImageStoreServiceName,
            clientFactory);        
    }
    else
    {        
        fileStoreClient = make_shared<FileStoreClient>(
            *SystemServiceApplicationNameHelper::PublicImageStoreServiceName,
            clientFactory);
    }

    return ErrorCodeValue::Success;
}

wstring NativeImageStore::RemoveSchemaTag(wstring const & uri)
{
    return uri.substr(Management::ImageStore::Constants::NativeImageStoreSchemaTag->size());
}

ErrorCode NativeImageStore::SetSecurity(Transport::SecuritySettings && securitySettings)
{
    WriteInfo(
        TraceComponent,
        "Applying security settings: {0}",
        securitySettings);

    return client_->SetSecurity(move(securitySettings));
}

ErrorCode NativeImageStore::UploadContent(
    std::wstring const & remoteDestination,
    std::wstring const & localSource,
    Common::TimeSpan const timeout,
    CopyFlag::Enum const & copyFlag)
{
    WriteInfo(TraceComponent, "Upload {0} to {1}", localSource, remoteDestination);

    // src, dest
    bool result = false;
    return this->SynchronousUpload(
        localSource,
        remoteDestination,
        timeout,
        copyFlag,
        result);
}


Common::AsyncOperationSPtr NativeImageStore::BeginUploadContent(
    wstring const & remoteDestination,
    wstring const & localSource,
    Common::TimeSpan const timeout,
    CopyFlag::Enum copyFlag,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<UploadObjectAsyncOperation>(
        client_,
        workingDirectory_,
        localSource,
        remoteDestination,
        copyFlag,
        timeout,
        callback,
        parent);

    return operation;
}

Common::ErrorCode NativeImageStore::EndUploadContent(
    Common::AsyncOperationSPtr const & operation)
{
    Common::ErrorCode error = UploadObjectAsyncOperation::End(operation);
    return error;
}

ErrorCode NativeImageStore::UploadContent(
    std::wstring const & remoteDestination,
    std::wstring const & localSource,
    Api::INativeImageStoreProgressEventHandlerPtr const & progressHandler,
    Common::TimeSpan const timeout,
    CopyFlag::Enum const & copyFlag)
{
    WriteInfo(TraceComponent, "Upload {0} to {1}", localSource, remoteDestination);
   
    bool result = false;
    return this->SynchronousUpload(
        localSource,
        remoteDestination,
        progressHandler,
        timeout,
        copyFlag,
        result);
}

Common::AsyncOperationSPtr NativeImageStore::BeginCopyContent(
    std::wstring const & remoteSource,
    std::wstring const & remoteDestination,
    std::shared_ptr<std::vector<std::wstring>> const & skipFiles,
    CopyFlag::Enum copyFlag,
    BOOLEAN checkMarkFile,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<CopyObjectAsyncOperation>(
        client_,
        remoteSource,
        remoteDestination,
        skipFiles,
        copyFlag,
        checkMarkFile,
        timeout,
        callback,
        parent);

    return operation;
}

Common::ErrorCode NativeImageStore::EndCopyContent(
    Common::AsyncOperationSPtr const & operation)
{
    Common::ErrorCode error = CopyObjectAsyncOperation::End(operation);
    return error;
}

ErrorCode NativeImageStore::CopyContent(
    std::wstring const & remoteSource,
    std::wstring const & remoteDestination,
    Common::TimeSpan const timeout,
    std::shared_ptr<std::vector<std::wstring>> const & skipFiles,
    CopyFlag::Enum const & copyFlag,
    BOOLEAN const & checkMarkFile)
{
    // src, dest
    bool result = false;
    return this->SynchronousCopy(
        remoteSource, 
        remoteDestination, 
        skipFiles, 
        copyFlag, 
        checkMarkFile,
        timeout,
        result);
}

ErrorCode NativeImageStore::DownloadContent(
    std::wstring const & remoteSource,
    std::wstring const & localDestination,
    Common::TimeSpan const timeout,
    CopyFlag::Enum const & copyFlag)
{
    return this->DownloadContent(
        remoteSource,
        localDestination,
        INativeImageStoreProgressEventHandlerPtr(),
        timeout,
        copyFlag);
}

ErrorCode NativeImageStore::DownloadContent(
    std::wstring const & remoteSource,
    std::wstring const & localDestination,
    INativeImageStoreProgressEventHandlerPtr const & progressHandler,
    Common::TimeSpan const timeout,
    CopyFlag::Enum const & copyFlag)
{
    bool result = false;
    return this->SynchronousDownload(
        remoteSource, 
        localDestination, 
        copyFlag, 
        progressHandler,
        timeout,
        result);
}

ErrorCode NativeImageStore::ListContent(
    std::wstring const & remoteLocation,
    BOOLEAN const & isRecursive,
    Common::TimeSpan const timeout,
    __out Management::FileStoreService::StoreContentInfo & result)
{
    return this->SynchronousList(remoteLocation, isRecursive, timeout, result);
}

ErrorCode NativeImageStore::ListPagedContent(
    Management::ImageStore::ImageStoreListDescription const & listDescription,
    Common::TimeSpan const timeout,
    __out Management::FileStoreService::StorePagedContentInfo & result)
{
    return this->SynchronousList(listDescription, timeout, result);
}

Common::AsyncOperationSPtr NativeImageStore::BeginListContent(
    std::wstring const & remoteLocation,
    BOOLEAN const & isRecursive,
    Common::TimeSpan const timeout,
    AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<ListFileAsyncOperation>(
        client_,
        remoteLocation,
        L"",
        isRecursive,
        false,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode NativeImageStore::EndListContent(
    Common::AsyncOperationSPtr const & operation,
    Management::FileStoreService::StoreContentInfo & result)
{
    return  ListFileAsyncOperation::End(operation, result);
}

Common::AsyncOperationSPtr NativeImageStore::BeginListPagedContent(
    Management::ImageStore::ImageStoreListDescription const & listDescription,
    Common::TimeSpan const timeout,
    AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<ListFileAsyncOperation>(
        client_,
        listDescription.RemoteLocation,
        listDescription.ContinuationToken,
        listDescription.IsRecursive,
        true,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode NativeImageStore::EndListPagedContent(
    Common::AsyncOperationSPtr const & operation,
    Management::FileStoreService::StorePagedContentInfo & result)
{
    return  ListFileAsyncOperation::End(operation, result);
}

ErrorCode NativeImageStore::DoesContentExist(
    std::wstring const & remoteLocation,
    Common::TimeSpan const timeout,
    __out bool & result)
{
    return this->SynchronousExists(remoteLocation, timeout, result);
}

ErrorCode NativeImageStore::DeleteContent(
    std::wstring const & remoteLocation,
    Common::TimeSpan const timeout)
{
    bool result = false;
    return this->SynchronousDelete(remoteLocation, timeout, result);
}

Common::AsyncOperationSPtr NativeImageStore::BeginDeleteContent(
    std::wstring const & remoteLocation,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<DeleteAsyncOperation>(
        client_,
        remoteLocation,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode NativeImageStore::EndDeleteContent(
    Common::AsyncOperationSPtr const & operation)
{
    Common::ErrorCode error = DeleteAsyncOperation::End(operation);
    return error;
}

Common::AsyncOperationSPtr NativeImageStore::BeginUploadChunk(
    std::wstring const & remoteDestination,
    std::wstring const & localSource,
    Common::Guid const & sessionId,
    uint64 startPosition,
    uint64 endPosition,
    uint64 fileLength,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<UploadChunkAsyncOperation>(
        client_,
        remoteDestination,
        localSource,
        sessionId,
        startPosition,
        endPosition,
        fileLength,
        timeout,
        callback,
        parent);

    return operation;
}

Common::ErrorCode NativeImageStore::EndUploadChunk(
    Common::AsyncOperationSPtr const & operation)
{
    return UploadChunkAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr NativeImageStore::BeginDeleteUploadSession(
    Common::Guid const & sessionId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<DeleteUploadSessionAsyncOperation>(
        client_,
        sessionId,
        timeout,
        callback,
        parent);

    return operation;
}

Common::ErrorCode NativeImageStore::EndDeleteUploadSession(
    Common::AsyncOperationSPtr const & operation)
{
    return DeleteUploadSessionAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr NativeImageStore::BeginListUploadSession(
    std::wstring const & remoteLocation,
    Common::Guid const & sessionId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<ListUploadSessionAsyncOperation>(
        client_,
        remoteLocation,
        sessionId,
        timeout,
        callback,
        parent);

    return operation;
}

Common::ErrorCode NativeImageStore::EndListUploadSession(
    Common::AsyncOperationSPtr const & operation,
    __out Management::FileStoreService::UploadSession & result)
{
    ErrorCode error = ListUploadSessionAsyncOperation::End(operation, result);
    return error;
}

Common::AsyncOperationSPtr NativeImageStore::BeginCommitUploadSession(
    Common::Guid const & sessionId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<CommitUploadSessionAsyncOperation>(
        client_,
        sessionId,
        timeout,
        callback,
        parent);

    return operation;
}

Common::ErrorCode NativeImageStore::EndCommitUploadSession(
    Common::AsyncOperationSPtr const & operation)
{
    return CommitUploadSessionAsyncOperation::End(operation);
}

ErrorCode NativeImageStore::OnUploadToStore(
    std::vector<std::wstring> const & remoteObjects,
    std::vector<std::wstring> const & localObjects,
    std::vector<CopyFlag::Enum> const & flags,
    Common::TimeSpan const timeout)
{
    return this->SynchronousUpload(localObjects, remoteObjects, flags, timeout);
}

ErrorCode NativeImageStore::OnDownloadFromStore(
    std::vector<std::wstring> const & remoteObjects,
    std::vector<std::wstring> const & localObjects,
    std::vector<CopyFlag::Enum> const & flags,
    Common::TimeSpan const timeout)
{
    vector<wstring> localFileNames;    
    for(size_t i = 0; i < localObjects.size(); i++)
    {           
        localFileNames.push_back(GetLocalFileNameForDownload(localObjects[i]));        
    }

    return this->SynchronousDownload(remoteObjects, localFileNames, flags, timeout);
}

ErrorCode NativeImageStore::OnDoesContentExistInStore(
    std::vector<std::wstring> const & remoteObjects,
    Common::TimeSpan const timeout,
    __out std::map<std::wstring, bool> & objectExistsMap)
{
    return this->SynchronousExists(remoteObjects, timeout, objectExistsMap);
}

ErrorCode NativeImageStore::OnRemoveObjectFromStore(
    std::vector<std::wstring> const & remoteObjects,
    Common::TimeSpan const timeout)
{
    return this->SynchronousDelete(remoteObjects, timeout);
}

ErrorCode NativeImageStore::SynchronousUpload(
    wstring const & src,
    wstring const & dest,
    Common::TimeSpan timeout,
    CopyFlag::Enum const flag,
    __out bool & uploaded)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);

    auto operation = AsyncOperation::CreateAndStart<UploadObjectAsyncOperation>(
        client_,
        workingDirectory_,
        src,
        dest,
        flag,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();

    return UploadObjectAsyncOperation::End(operation, uploaded);
}

ErrorCode NativeImageStore::SynchronousUpload(
    wstring const & src,
    wstring const & dest,
    INativeImageStoreProgressEventHandlerPtr const & progressHandler,
    Common::TimeSpan timeout,
    CopyFlag::Enum const flag,
    __out bool & uploaded)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);
    auto progressState = make_shared<NativeImageStoreProgressState>(progressHandler, wformatString("Upload({0})", src), ProgressUnitType::Bytes);

    auto operation = AsyncOperation::CreateAndStart<UploadObjectAsyncOperation>(
        client_,
        workingDirectory_,
        src,
        dest,
        flag,
        false, // isRecursive
        progressState,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();

    progressState->StopDispatchingUpdates();

    return UploadObjectAsyncOperation::End(operation, uploaded);
}

ErrorCode NativeImageStore::SynchronousCopy(
    wstring const & src,
    wstring const & dest,
    shared_ptr<vector<wstring>> const & skipFiles,
    CopyFlag::Enum const flag,
    BOOLEAN const & checkMarkFile,
    Common::TimeSpan const timeout,
    __out bool & copied)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);

    auto operation = AsyncOperation::CreateAndStart<CopyObjectAsyncOperation>(
        client_,
        src,
        dest,
        skipFiles,
        flag,
        checkMarkFile,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();

    return CopyObjectAsyncOperation::End(operation, copied);
}

ErrorCode NativeImageStore::SynchronousUpload(
    vector<wstring> const & sources,
    vector<wstring> const & destinations,
    vector<CopyFlag::Enum> const & flags,
    Common::TimeSpan const timeout)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);

    auto operation = AsyncOperation::CreateAndStart<ParallelUploadObjectsAsyncOperation>(
        client_,
        workingDirectory_,
        sources,
        destinations,
        flags,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();

    return ParallelUploadObjectsAsyncOperation::End(operation);
}

ErrorCode NativeImageStore::SynchronousDownload(
    wstring const & src,
    wstring const & dest,
    CopyFlag::Enum const flag,
    INativeImageStoreProgressEventHandlerPtr const & progressHandler,
    Common::TimeSpan timeout,
    __out bool & result)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);

    auto operation = AsyncOperation::CreateAndStart<DownloadObjectAsyncOperation>(
        client_,
        src,
        dest,
        flag,
        progressHandler,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();

    auto error = DownloadObjectAsyncOperation::End(operation);

    if (error.IsSuccess())
    {
        result = true;
    }

    return error;
}

ErrorCode NativeImageStore::SynchronousDownload(
    vector<wstring> const & sources,
    vector<wstring> const & destinations,
    vector<CopyFlag::Enum> const & flags,
    Common::TimeSpan timeout)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);

    auto operation = AsyncOperation::CreateAndStart<ParallelDownloadObjectsAsyncOperation>(
        client_,
        sources,
        destinations,
        flags,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();

    return ParallelDownloadObjectsAsyncOperation::End(operation);
}

ErrorCode NativeImageStore::SynchronousExists(
    wstring const & target,
    Common::TimeSpan const timeout,
    __out bool & exists)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);

    auto operation = AsyncOperation::CreateAndStart<ObjectExistsAsyncOperation>(
        client_,
        target,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();

    return ObjectExistsAsyncOperation::End(operation, exists);
}

ErrorCode NativeImageStore::SynchronousList(
    std::wstring const & target,
    BOOLEAN const & isRecursive,
    Common::TimeSpan const timeout,
    __out Management::FileStoreService::StoreContentInfo & result)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);

    auto operation = AsyncOperation::CreateAndStart<ListFileAsyncOperation>(
        client_,
        target,
        L"",
        isRecursive,
        false,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();
    return ListFileAsyncOperation::End(operation, result);
}

ErrorCode NativeImageStore::SynchronousList(
    Management::ImageStore::ImageStoreListDescription const & listDescription,
    Common::TimeSpan const timeout,
    __out Management::FileStoreService::StorePagedContentInfo & result)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);

    auto operation = AsyncOperation::CreateAndStart<ListFileAsyncOperation>(
        client_,
        listDescription.RemoteLocation,
        listDescription.ContinuationToken,
        listDescription.IsRecursive,
        true,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();
    return ListFileAsyncOperation::End(operation, result);
}

ErrorCode NativeImageStore::SynchronousExists(
    vector<wstring> const & targets,
    Common::TimeSpan const timeout,
    __out map<wstring, bool> & exists)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);

    auto operation = AsyncOperation::CreateAndStart<ParallelExistsAsyncOperation>(
        client_,
        targets,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();

    return ParallelExistsAsyncOperation::End(operation, exists);
}

ErrorCode NativeImageStore::SynchronousDelete(
    wstring const & target,
    Common::TimeSpan const timeout,
    __out bool & result)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);

    auto operation = AsyncOperation::CreateAndStart<DeleteAsyncOperation>(
        client_,
        target,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();

    auto error = DeleteAsyncOperation::End(operation);

    if (error.IsSuccess())
    {
        result = true;
    }

    return error;
}

ErrorCode NativeImageStore::SynchronousDelete(
    vector<wstring> const & target, 
    Common::TimeSpan const timeout)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);

    auto operation = AsyncOperation::CreateAndStart<ParallelDeleteAsyncOperation>(
        client_,
        target,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();

    return ParallelDeleteAsyncOperation::End(operation);
}

ErrorCode NativeImageStore::SynchronousUploadChunk(
    std::wstring const & remoteDestination,
    std::wstring const & localSource,
    Common::Guid const & sessionId,
    uint64 startPosition,
    uint64 endPosition,
    uint64 fileLength,
    Common::TimeSpan const timeout)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);
    auto operation = AsyncOperation::CreateAndStart<UploadChunkAsyncOperation>(
        client_,
        remoteDestination,
        localSource,
        sessionId,
        startPosition,
        endPosition,
        fileLength,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();
    return UploadChunkAsyncOperation::End(operation);
}

ErrorCode NativeImageStore::SynchronousListUploadSession(
    std::wstring const & remoteLocation,
    Common::Guid const & sessionId,
    Common::TimeSpan const timeout,
    __out Management::FileStoreService::UploadSession & result)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);
    auto operation = AsyncOperation::CreateAndStart<ListUploadSessionAsyncOperation>(
        client_,
        remoteLocation,
        sessionId,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();
    return ListUploadSessionAsyncOperation::End(operation, result);
}

ErrorCode NativeImageStore::SynchronousDeleteUploadSession(
    Common::Guid const & sessionId,
    Common::TimeSpan const timeout)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);
    auto operation = AsyncOperation::CreateAndStart<DeleteUploadSessionAsyncOperation>(
        client_,
        sessionId,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();
    return DeleteUploadSessionAsyncOperation::End(operation);
}

ErrorCode NativeImageStore::SynchronousCommitUploadSession(
    Common::Guid const & sessionId,
    Common::TimeSpan const timeout)
{
    if (timeout <= Common::TimeSpan::Zero)
    {
        return ErrorCodeValue::Timeout;
    }

    ManualResetEvent completionEvent(false);
    auto operation = AsyncOperation::CreateAndStart<CommitUploadSessionAsyncOperation>(
        client_,
        sessionId,
        timeout,
        [&completionEvent](AsyncOperationSPtr const &) { completionEvent.Set(); },
        AsyncOperationSPtr());

    completionEvent.WaitOne();
    return CommitUploadSessionAsyncOperation::End(operation);
}

wstring NativeImageStore::GetDirectoryMarkerFileName(wstring const & directory)
{
    return Path::Combine(directory, *NativeImageStore::DirectoryMarkerFileName); 
}

