// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace Store;
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("ProcessInternalListRequestAsyncOperation");

ProcessInternalListRequestAsyncOperation::ProcessInternalListRequestAsyncOperation(
    __in RequestManager & requestManager,
    ListRequest && listRequest,
    IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) 
    : ProcessRequestAsyncOperation(requestManager, listRequest.StoreRelativePath, OperationKind::InternalList, move(receiverContext), activityId, timeout, callback, parent)
    , isPresent_(false)
    , state_()
    , currentVersion_()
{
}

ProcessInternalListRequestAsyncOperation::~ProcessInternalListRequestAsyncOperation()
{
}

ErrorCode ProcessInternalListRequestAsyncOperation::ValidateRequest()
{
    if(this->StoreRelativePath.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ProcessInternalListRequestAsyncOperation::BeginOperation(                                
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{       
    auto error = this->ListMetadata();
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
}

ErrorCode ProcessInternalListRequestAsyncOperation::EndOperation(
    __out Transport::MessageUPtr & reply,
    Common::AsyncOperationSPtr const & asyncOperation)
{
    auto errorCode = CompletedAsyncOperation::End(asyncOperation);
    if(errorCode.IsSuccess())
    {
        InternalListReply listReply(isPresent_, state_, currentVersion_, this->RequestManagerObj.StoreShareLocation);
        reply = FileStoreServiceMessage::GetClientOperationSuccess(listReply, this->ActivityId);
    }

    return errorCode;
}

ErrorCode ProcessInternalListRequestAsyncOperation::ListMetadata()
{       
    auto localFullpath = Path::Combine(this->RequestManagerObj.ReplicaObj.StoreRoot, this->StoreRelativePath);
    // list the specific file
    FileMetadata metadata(this->StoreRelativePath);
    auto error = this->ReplicatedStoreWrapperObj.ReadExact(this->ActivityId, metadata);

    if (error.IsError(ErrorCodeValue::Success) && 
        this->RequestManagerObj.IsDataLossExpected() &&
        FileState::IsStable(metadata.State))
    {        
        if (!File::Exists(localFullpath))
        {
            // Dataloss only happens if machines are destructively re-imaged (losing data) and
            // FSS full builds are not given time to complete before more machines in the replica set continue getting re-imaged.
            // This happens on-premise if the cluster admin does not wait for node deactivation to fully complete at each re-image step or
            // in Azure if the time to fully rebuild a FSS replica exceeds the timeout allowed in the given durability tier.

            // Above dataloss sceanrio can lead to inconsistent metadata and files on disk. Usually orphaned files are not a major concern.
            // But there could be orphaned metadata entries not backed up by files in the disk of any of the replicas.
            // This would cause secondaries to stuck in IB state, since they would be trying to copy missing files.
            // This would also cause primary to stuck in recovery since it will be unable to delete orphaned metadata entries (no write quorum).
            // Note: Once dataloss handler is returned, FSS has read access to the partition to read metadata but no write access.
            // The fix here is to identify the scenario and send a specific error code so that secondary can progress.

            WriteInfo(
                TraceComponent,
                this->RequestManagerObj.TraceId,
                "Dataloss detected where metadata is present but the file {0} is not present on disk. Returning FSSPrimaryInDatalossRecovery error for secondary to progress.",
                localFullpath);

            return ErrorCodeValue::FSSPrimaryInDatalossRecovery;
        }
    }

    if(!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
    {
        return error;
    }

    if(error.IsError(ErrorCodeValue::NotFound))
    {
        isPresent_ = false;
        return ErrorCodeValue::Success;
    }

    isPresent_ = true;
    state_ = metadata.State;
    currentVersion_ = metadata.CurrentVersion;

    if (!File::Exists(localFullpath))
    {
        WriteWarning(
            TraceComponent,
            this->RequestManagerObj.TraceId,
            "File doesn't exist locally: {0}.",
            localFullpath);
    }

    return ErrorCodeValue::Success;
}

