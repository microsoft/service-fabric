// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace Common;
using namespace Federation;
using namespace ServiceModel;
using namespace Transport;
using namespace Store;
using namespace Management::FileStoreService;

#define PATHBUFFER_SIZE 257
StringLiteral const TraceComponent("ProcessListRequestAsyncOperation");

ProcessListRequestAsyncOperation::ProcessListRequestAsyncOperation(
    __in RequestManager & requestManager,
    ListRequest && listRequest,
    IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : ProcessRequestAsyncOperation(requestManager, listRequest.StoreRelativePath, OperationKind::List, move(receiverContext), activityId, timeout, callback, parent)
    , continuationToken_(listRequest.ContinuationToken)
    , shouldIncludeDetails_(listRequest.ShouldIncludeDetails)
    , isRecursive_(listRequest.IsRecursive)
    , isPaging_(listRequest.IsPaging)
{
}

ProcessListRequestAsyncOperation::~ProcessListRequestAsyncOperation()
{
}

AsyncOperationSPtr ProcessListRequestAsyncOperation::BeginOperation(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    this->NormalizeContinuationToken();
    auto error = this->ListMetadata();

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
} 

ErrorCode ProcessListRequestAsyncOperation::EndOperation(
    __out Transport::MessageUPtr & reply,
    Common::AsyncOperationSPtr const & asyncOperation)
{
    auto errorCode = CompletedAsyncOperation::End(asyncOperation);
    if(errorCode.IsSuccess())
    {
        ListReply listReply(move(content_.StoreFiles), move(content_.StoreFolders), this->RequestManagerObj.StoreShareLocation, move(content_.ContinuationToken));
        reply = FileStoreServiceMessage::GetClientOperationSuccess(listReply, this->ActivityId);
    }
    return errorCode;
}


size_t ProcessListRequestAsyncOperation::GetContinuationTokenPosition(
    vector<FileMetadata> const & metadataList, 
    wstring const & continuationToken)
{
    if(continuationToken.empty() || metadataList[0].Key.compare(continuationToken) >= 0)
    {
        return 0;
    }

    if (metadataList.back().Key.compare(continuationToken) < 0)
    {
        return metadataList.size();
    }

    bool found = false;
    size_t low = 0;
    size_t high = metadataList.size() - 1;

    do
    {
        size_t middle = (high - low) / 2 + low;
        int result = metadataList[middle].Key.compare(continuationToken);
        if (result == 0)
        {
            return middle;
        }
        
        if (result > 0)
        {
            high = middle;
        }
        else
        {
            low = middle;
        }

        if (low == high - 1)
        {
            found = true;
        }
    } while (!found);

    return high;
}

ErrorCode ProcessListRequestAsyncOperation::ListMetadata()
{
    ErrorCode error(ErrorCodeValue::Success);
    vector<FileMetadata> metadataList;
   
    auto toStoreFileInfo = [](FileMetadata const & metadata, std::wstring const storeRoot, StoreFileInfo & fileInfo) -> ErrorCode
    {
        std::wstring filePath = Utility::GetVersionedFileFullPath(storeRoot, metadata.StoreRelativeLocation, metadata.CurrentVersion);
        if(File::Exists(filePath))
        {
            File file;
            ErrorCode error = ErrorCode::FromWin32Error(file.TryOpen(filePath, FileMode::Open, FileAccess::Read, FileShare::Read).ReadValue());
            if(!error.IsSuccess())
            {
                return error;
            }

            Common::DateTime lastWriteTime;
            error = File::GetLastWriteTime(filePath, lastWriteTime);
            if(!error.IsSuccess())
            {
                return error;
            }

            int64 size;
            error = file.GetSize(size);
            if(!error.IsSuccess())
            {
                return error;
            }

            error = file.Close2();
            if(!error.IsSuccess())
            {
                return error;
            }

            fileInfo = StoreFileInfo(metadata.StoreRelativeLocation, metadata.CurrentVersion, size, lastWriteTime);
            return ErrorCodeValue::Success;
        }

        return ErrorCodeValue::FileNotFound;
    };

    if(this->StoreRelativePath.empty())
    {
        error = this->ReplicatedStoreWrapperObj.ReadPrefix(this->ActivityId, this->NormalizedStoreRelativePath, metadataList);
    }
#if !defined(PLATFORM_UNIX)
    else if(Directory::Exists(Path::Combine(this->RequestManagerObj.ReplicaObj.StoreRoot, this->StoreRelativePath)))
    {
        error = this->ReplicatedStoreWrapperObj.ReadPrefix(this->ActivityId, this->NormalizedStoreRelativePath, metadataList);
    }
    else
    {
        // list the specific file
        FileMetadata metadata(this->StoreRelativePath);
        error = this->ReplicatedStoreWrapperObj.ReadExact(this->ActivityId, metadata);
        if(error.IsSuccess())
        {
            StoreFileInfo fileInfo;
            error = toStoreFileInfo(metadata, this->RequestManagerObj.ReplicaObj.StoreRoot, fileInfo);
            if(!error.IsSuccess())
            {
                return error;
            }

            return content_.TryAdd(move(fileInfo));
        }
        else if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteInfo(
                TraceComponent,
                TraceId,
                "FileNotFound={0}",
                this->StoreRelativePath);
        }
    }
#else
    else
    {
        bool isDirectory = false;
        // try to list the specific file
        FileMetadata metadata(this->StoreRelativePath);
        error = this->ReplicatedStoreWrapperObj.ReadExact(this->ActivityId, metadata);
        if (error.IsSuccess())
        {
            StoreFileInfo fileInfo;
            ErrorCode error = toStoreFileInfo(metadata, this->RequestManagerObj.ReplicaObj.StoreRoot, fileInfo);
            if (!error.IsSuccess())
            {
                return error;
            }

            return content_.TryAdd(move(fileInfo));
        }
        // if list the specific file failed, try folder
        else if (Directory::Exists(Path::Combine(this->RequestManagerObj.ReplicaObj.StoreRoot, this->StoreRelativePath)))
        {
            isDirectory = true;
            error = this->ReplicatedStoreWrapperObj.ReadPrefix(this->ActivityId, this->NormalizedStoreRelativePath, metadataList);
        }
        if (!isDirectory && error.IsError(ErrorCodeValue::NotFound))
        {
            WriteWarning(
                TraceComponent,
                TraceId,
                "FileNotFound={0}",
                this->StoreRelativePath);
        }
    }
#endif

    if(!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
    {
        return error;
    }

    size_t startIndex = this->GetContinuationTokenPosition(metadataList, continuationToken_);
    if (startIndex == metadataList.size())
    {
        return ErrorCodeValue::Success;
    }

    WriteInfo(
        TraceComponent,
        TraceId,
        "ContinuationToken={0}, MetadataListSize={1}, StartIndex={2}, StartRelativePath={3}",
        continuationToken_,
        metadataList.size(),
        startIndex,
        metadataList[startIndex].Key);

    if (this->isRecursive_)
    {
        for (std::vector<FileMetadata>::size_type ix = startIndex; ix != metadataList.size(); ++ix)
        {
            if(!FileState::IsStable(metadataList[ix].State))
            {
                continue;
            }

            StoreFileInfo fileInfo;
            if (this->shouldIncludeDetails_)
            {
                error = toStoreFileInfo(metadataList[ix], this->RequestManagerObj.ReplicaObj.StoreRoot, fileInfo);
                if (!error.IsSuccess())
                {
                    return error;
                }
            }

            error = content_.TryAdd(move(shouldIncludeDetails_ ? fileInfo : StoreFileInfo(metadataList[ix].StoreRelativeLocation, metadataList[ix].CurrentVersion, 0, Common::DateTime::Zero)));
            if (!error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent,
                    TraceId,
                    "Failed to add file {0} to the pager: error = {1}",
                    metadataList[ix].Key,
                    error);

                if (error.IsError(ErrorCodeValue::EntryTooLarge) && isPaging_)
                {
                    return ErrorCodeValue::Success;
                }
                else
                {
                    break;
                }
            }
        }
    }
    else
    {
        wstring folderPath;
        uint16 fileCounter = 0;

        for (std::vector<FileMetadata>::size_type ix = startIndex; ix != metadataList.size(); ++ix)
        {
            if(!FileState::IsStable(metadataList[ix].State))
            {
                continue;
            }

            std::wstring relativeLocation(metadataList[ix].get_RelativeLocation().begin(), metadataList[ix].get_RelativeLocation().end());
            std::wstring rest = this->StoreRelativePath.empty() || this->StoreRelativePath.compare(relativeLocation) == 0 ? relativeLocation : relativeLocation.substr(this->StoreRelativePath.length() + 1);
#if !defined(PLATFORM_UNIX)
            std::size_t index = rest.find_first_of('\\');
#else
            std::size_t index = rest.find_first_of('/');
#endif

            if (index == std::wstring::npos)
            {
                if (!folderPath.empty())
                {
                    error = content_.TryAdd(StoreFolderInfo(folderPath, fileCounter));
                    if (!error.IsSuccess())
                    {
                        WriteInfo(
                            TraceComponent,
                            TraceId,
                            "Failed to add folder {0} to the pager: error = {1}",
                            folderPath,
                            error);
                        break;
                    }
                    
                    folderPath = L"";
                }

                StoreFileInfo fileInfo;
                error = toStoreFileInfo(metadataList[ix], this->RequestManagerObj.ReplicaObj.StoreRoot, fileInfo);
                if (!error.IsSuccess())
                {
                    return error;
                }

                error = content_.TryAdd(move(fileInfo));
                if (!error.IsSuccess())
                {
                    WriteInfo(
                        TraceComponent,
                        TraceId,
                        "Failed to add file {0} to the pager: error = {1}",
                        metadataList[ix].StoreRelativeLocation,
                        error);

                    break;
                }
            }
            else
            {
                std::wstring currentFolderName = relativeLocation.substr(0, this->StoreRelativePath.empty() ? index : this->StoreRelativePath.length() + 1 + index);
                if (folderPath.empty())
                {
                    folderPath = currentFolderName;
                    fileCounter = 1;
                    continue;
                }

                if (folderPath.compare(currentFolderName) == 0)
                {
                    ++fileCounter;
                    continue;
                }

                error = content_.TryAdd(StoreFolderInfo(folderPath, fileCounter));
                if (!error.IsSuccess())
                {
                    WriteInfo(
                        TraceComponent,
                        TraceId,
                        "Failed to add folder {1} to the pager: error = {2}",
                        folderPath,
                        error);

                    break;
                }
                else
                {
                    folderPath = currentFolderName;
                    fileCounter = 1;
                }
            }
        }

        if (!folderPath.empty())
        {
            error = content_.TryAdd(StoreFolderInfo(folderPath, fileCounter));
            if (!error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent,
                    TraceId,
                    "Failed to add folder {1} to the pager: error = {2}",
                    folderPath,
                    error);
            }
        }
    }

    WriteInfo(
        TraceComponent,
        TraceId,
        "Error:{0}, ContinuationToken:{1}, Files:{2}, Folders:{3}",
        error,
        continuationToken_,
        content_.StoreFiles.size(),
        content_.StoreFolders.size());
    
    if (error.IsError(ErrorCodeValue::EntryTooLarge) && isPaging_)
    {
        return ErrorCodeValue::Success;
    }
    
    return error;
}

void ProcessListRequestAsyncOperation::NormalizeContinuationToken()
{
    if (continuationToken_.empty()) { return; }

#if !defined(PLATFORM_UNIX)
    StringUtility::Replace<wstring>(continuationToken_, L"/", L"\\");
    StringUtility::TrimSpaces(continuationToken_);
    StringUtility::TrimLeading<wstring>(continuationToken_, L"\\");
    StringUtility::TrimTrailing<wstring>(continuationToken_, L"\\");
    StringUtility::ToLower(continuationToken_);
#else
    StringUtility::Replace<wstring>(continuationToken_, L"\\", L"/");
    StringUtility::TrimSpaces(continuationToken_);
    StringUtility::TrimLeading<wstring>(continuationToken_, L"/");
    StringUtility::TrimTrailing<wstring>(continuationToken_, L"/");
#endif
}

