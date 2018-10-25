// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Store;
using namespace Transport;
using namespace Federation;
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("ProcessRequestAsyncOperation");

ProcessRequestAsyncOperation::ProcessRequestAsyncOperation(
    RequestManager & requestManager,
    wstring const & storeRelativePath,
    OperationKind const operationKind,
    IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) 
    : AsyncOperation(callback, parent),
    ReplicaActivityTraceComponent(requestManager.ReplicaObj.PartitionedReplicaId, activityId),
    requestManager_(requestManager),
    storeRelativePath_(storeRelativePath),
    operationKind_(operationKind),
    receiverContext_(move(receiverContext)),
    timeoutHelper_(timeout),
    reply_(),
    metadataDeletionCompleted_(false)
{
}

ProcessRequestAsyncOperation::~ProcessRequestAsyncOperation()
{
}

ErrorCode ProcessRequestAsyncOperation::End(AsyncOperationSPtr const & asyncOperation,
                                            __out IpcReceiverContextUPtr & receiverContext,
                                            __out MessageUPtr & reply,
                                            __out Common::ActivityId & activityId)
{
    auto thisSPtr = AsyncOperation::End<ProcessRequestAsyncOperation>(asyncOperation);    

    receiverContext = move(thisSPtr->receiverContext_);
    reply = move(thisSPtr->reply_);
    activityId = thisSPtr->ActivityId;

    return thisSPtr->Error;
}

void ProcessRequestAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto error = this->ValidateRequest();
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "ValidateRequest failed with {0}",
            error);

        CompleteOperation(thisSPtr, error);
        return;
    }    

    this->NormalizeStoreRelativePath();
    
    bool isWriteOperation = IsWriteOperation(operationKind_);
    if(isWriteOperation)
    {        
        // TryAdd for folder is more expensive since we have to ensure there
        // are no write operations happening in a child folder
        //
        // It's not possible to determine if we are dealing with folders or
        // files by looking at storeRelativePath_
        //
        // Since we deal with folders only during delete, we only set isFolder to be
        // true for delete operation
        bool isFolder = operationKind_ == OperationKind::Delete;
        error = requestManager_.pendingWriteOperations_.TryAdd(storeRelativePath_, isFolder);
        if(!error.IsSuccess())
        {
            if(error.IsError(ErrorCodeValue::ObjectClosed))
            {
                error = ErrorCodeValue::NotPrimary;
            }

            if (error.IsError(ErrorCodeValue::FileUpdateInProgress) && 
                (operationKind_ == OperationKind::CommitUploadSession))
            {
                // No need to retry at the gateway
                error = ErrorCodeValue::OperationsPending;
            }

            WriteWarning(
                TraceComponent,
                TraceId,
                "Failed to add '{0}' to list of pending write operations. Error:{1}",
                storeRelativePath_,
                error);
            CompleteOperation(thisSPtr, error);
            return;
        }
    }

    auto operation = this->BeginOperation(
        [this](AsyncOperationSPtr const & operation) { this->OnOperationCompleted(operation, false); },
        thisSPtr);
    this->OnOperationCompleted(operation, true);
}

void ProcessRequestAsyncOperation::OnOperationCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = this->EndOperation(reply_, operation);

    bool isWriteOperation = IsWriteOperation(operationKind_);
    if(isWriteOperation)
    {
        requestManager_.pendingWriteOperations_.Remove(storeRelativePath_);
    }

    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "The request failed due to {0}. StoreRelativePath:{1}",
            error,
            storeRelativePath_);
    }

    CompleteOperation(operation->Parent, error);
} 

void ProcessRequestAsyncOperation::CompleteOperation(AsyncOperationSPtr const & thisSPtr, ErrorCode & errorCode)
{
    this->OnRequestCompleted(errorCode);

    TryComplete(thisSPtr, errorCode);
}

void ProcessRequestAsyncOperation::OnRequestCompleted(__inout ErrorCode & error)
{
    // 'NotPrimary' ErrorCode is converted to 'OperationCanceled' ErrorCode by Gateway and returned to the client 
    // since other system service cannot handle it. However, for FileStoreService, the operation can be retried on 
    // gateway. 
    //
    // Only exception is for UploadAction where the client would have to re-upload the file to the new primary. 
    // ProcessUploadRequestAsync overrides this method to handle this condition
    if(error.IsError(ErrorCodeValue::NotPrimary))
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Replacing original ErrorCode={0} with FileStoreServiceNotReady",            
            error);

        error.Overwrite(ErrorCodeValue::FileStoreServiceNotReady);
    }
    else 
    {
        ConvertObjectClosedErrorCode(error);
    }
}

void ProcessRequestAsyncOperation::ConvertObjectClosedErrorCode(__inout ErrorCode & error)
{
    //The "ReconfigurationPending" will be converted to "OperationCanceled" at the naming gateway before returning to the client.
    //The most async operations at the image store client take the "OperationCanceled" as retryable. 
    if (error.IsError(ErrorCodeValue::ObjectClosed))
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Replacing original ErrorCode={0} with ReconfigurationPending",
            error);

        error.Overwrite(ErrorCodeValue::ReconfigurationPending);
    }
}

void ProcessRequestAsyncOperation::NormalizeStoreRelativePath()
{
    if(storeRelativePath_.empty()) { return; }

#if !defined(PLATFORM_UNIX)
    StringUtility::Replace<wstring>(storeRelativePath_, L"/", L"\\");
    StringUtility::TrimSpaces(storeRelativePath_);    
    StringUtility::TrimLeading<wstring>(storeRelativePath_, L"\\");
    StringUtility::TrimTrailing<wstring>(storeRelativePath_, L"\\");
#else
    StringUtility::Replace<wstring>(storeRelativePath_, L"\\", L"/");
    StringUtility::TrimSpaces(storeRelativePath_);
    StringUtility::TrimLeading<wstring>(storeRelativePath_, L"/");
    StringUtility::TrimTrailing<wstring>(storeRelativePath_, L"/");
#endif
}

bool ProcessRequestAsyncOperation::IsWriteOperation(OperationKind operationKind)
{
    return (operationKind == OperationKind::Upload || 
        operationKind == OperationKind::Delete || 
        operationKind == OperationKind::Copy ||
        operationKind == OperationKind::CommitUploadSession);
}

ErrorCode ProcessRequestAsyncOperation::DeleteMetadata(Store::StoreTransaction const & storeTx, FileMetadata const & metadata)
{
    auto error = storeTx.Delete(metadata);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "Unable to delete metadata. metadata:{0}, Error:{1}",
            metadata,
            error);
        return error;
    }

    return ErrorCodeValue::Success;
}

bool ProcessRequestAsyncOperation::DeleteIfMetadataNotInStableState(Common::AsyncOperationSPtr const & thisSPtr)
{
    bool toBeDeleted = false;
    FileMetadata metadata(this->StoreRelativePath);
    auto error = this->ReplicatedStoreWrapperObj.ReadExact(this->ActivityId, metadata);
    if (error.IsSuccess())
    {
        if (!FileState::IsStable(metadata.State))
        {
            auto operation = AsyncOperation::CreateAndStart<StoreTransactionAsyncOperation>(
                RequestManagerObj,
                false, // useSimpleTx
                [this, &metadata](Store::StoreTransaction const & storeTx) { return DeleteMetadata(storeTx, metadata); },
                ActivityId,
                TimeSpan::MaxValue,
                [this, &metadata](AsyncOperationSPtr const & operation) { this->OnDeleteMetadataCompleted(operation, false, metadata); },
                thisSPtr);
            toBeDeleted = true;
            this->OnDeleteMetadataCompleted(operation, true, metadata);
        }
    }

    if (toBeDeleted)
    {
        // In case if you remove the wait in the future, make sure to not pass metadata by ref.
        metadataDeletionCompleted_.Wait();
    }

    return toBeDeleted;
}

void ProcessRequestAsyncOperation::OnDeleteMetadataCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously, FileMetadata const & metadata)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = StoreTransactionAsyncOperation::End(operation);
    AsyncOperationSPtr thisSPtr = operation->Parent;
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "Unable to delete metadata which is left in intermediate state. Id:{0} storePath:{1}: error:{2} Metadata:{3}.",
            metadata.UploadRequestId,
            this->StoreRelativePath,
            error,
            metadata);
    }

    metadataDeletionCompleted_.Set();
    return;
}

