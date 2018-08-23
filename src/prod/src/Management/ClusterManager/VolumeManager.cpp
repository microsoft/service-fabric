//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ReplicaVolumeManager");

class VolumeManager::FinishVolumeRequestNoopAsyncOperation : public AsyncOperation
{
public:
    FinishVolumeRequestNoopAsyncOperation(
        ErrorCode &&,
        AsyncCallback const &,
        AsyncOperationSPtr const &);

    static ErrorCode End(AsyncOperationSPtr const &);

    void OnStart(AsyncOperationSPtr const &);

protected:
    void FinishVolumeRequest(AsyncOperationSPtr const &);

    ErrorCode error_;
};

class VolumeManager::FinishVolumeRequestAsyncOperation : public FinishVolumeRequestNoopAsyncOperation
{
public:
    FinishVolumeRequestAsyncOperation(
        StoreTransaction &&,
        ClientRequestSPtr &&,
        TimeSpan const,
        ErrorCode &&,
        AsyncCallback const &,
        AsyncOperationSPtr const &);

    void OnStart(AsyncOperationSPtr const &);

private:
    void OnCommitComplete(AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

    StoreTransaction storeTx_;
    ClientRequestSPtr clientRequest_;
    TimeoutHelper timeoutHelper_;
};

VolumeManager::VolumeManager(
    ClusterManagerReplica const & owner)
    : owner_(owner)
{
}

AsyncOperationSPtr VolumeManager::BeginCreateVolume(
    VolumeDescriptionSPtr const & volumeDescription,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (volumeDescription == nullptr)
    {
        WriteWarning(
            TraceComponent,
            "{0} Rejecting volume creation request because volume description was not provided.",
            clientRequest->ReplicaActivityId);
        ErrorCode errorNoVolDesc(ErrorCodeValue::InvalidArgument, GET_CM_RC(VolumeDescriptionMissing));
        return RejectRequest(move(clientRequest), move(errorNoVolDesc), callback, root);
    }
    ErrorCode error = volumeDescription->Validate();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} Validation failure for volume {1}. Error: {2} {3}",
            clientRequest->ReplicaActivityId,
            volumeDescription->VolumeName,
            error,
            error.Message);
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    auto storeTx = StoreTransaction::Create(this->owner_.ReplicatedStore, this->owner_.PartitionedReplicaId, clientRequest->ActivityId);

    StoreDataVolume volumeData(volumeDescription);
    bool inserted;
    error = storeTx.InsertIfNotFound(volumeData, inserted);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} Failed to insert data for volume {1} in the store. Error: {2} {3}",
            clientRequest->ReplicaActivityId,
            volumeDescription->VolumeName,
            error,
            error.Message);
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    if (!inserted)
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::VolumeAlreadyExists, callback, root);
    }

    WriteInfo(
        TraceComponent,
        "{0} volume {1} is being created",
        clientRequest->ReplicaActivityId,
        volumeDescription->VolumeName);

    return AsyncOperation::CreateAndStart<FinishVolumeRequestAsyncOperation>(
        move(storeTx),
        move(clientRequest),
        timeout,
        move(error),
        callback,
        root);
}

AsyncOperationSPtr VolumeManager::BeginDeleteVolume(
    wstring const & volumeName,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    auto storeTx = StoreTransaction::Create(this->owner_.ReplicatedStore, this->owner_.PartitionedReplicaId, clientRequest->ActivityId);

    StoreDataVolume volumeData(volumeName);
    ErrorCode error = storeTx.Exists(volumeData);
    if (!error.IsSuccess())
    {
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            error = ErrorCodeValue::VolumeNotFound;
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "{0} Failed to determine existence of data for volume {1} in the store. Error: {2} {3}",
                clientRequest->ReplicaActivityId,
                volumeName,
                error,
                error.Message);
        }
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    error = storeTx.Delete(volumeData);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} Failed to delete data for volume {1} in the store. Error: {2} {3}",
            clientRequest->ReplicaActivityId,
            volumeName,
            error,
            error.Message);
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    WriteInfo(
        TraceComponent,
        "{0} volume {1} is being deleted",
        clientRequest->ReplicaActivityId,
        volumeName);

    return AsyncOperation::CreateAndStart<FinishVolumeRequestAsyncOperation>(
        move(storeTx),
        move(clientRequest),
        timeout,
        move(error),
        callback,
        root);
}

ErrorCode VolumeManager::EndVolumeRequest(Common::AsyncOperationSPtr const & operation)
{
    return FinishVolumeRequestNoopAsyncOperation::End(operation);
}

AsyncOperationSPtr VolumeManager::RejectRequest(
    ClientRequestSPtr && clientRequest,
    ErrorCode && error,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (!error.IsSuccess())
    {
        CMEvents::Trace->RequestRejected(
            clientRequest->ReplicaActivityId,
            clientRequest->RequestMessageId,
            error);
    }
    return AsyncOperation::CreateAndStart<FinishVolumeRequestNoopAsyncOperation>(
        move(error),
        callback,
        root);
}

VolumeManager::FinishVolumeRequestNoopAsyncOperation::FinishVolumeRequestNoopAsyncOperation(
    ErrorCode && error,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : AsyncOperation(callback, root)
    , error_(move(error))
{
}

ErrorCode VolumeManager::FinishVolumeRequestNoopAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<FinishVolumeRequestNoopAsyncOperation>(operation)->Error;
}

void VolumeManager::FinishVolumeRequestNoopAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->FinishVolumeRequest(thisSPtr);
}

void VolumeManager::FinishVolumeRequestNoopAsyncOperation::FinishVolumeRequest(AsyncOperationSPtr const & thisSPtr)
{
    this->TryComplete(thisSPtr, error_);
}

VolumeManager::FinishVolumeRequestAsyncOperation::FinishVolumeRequestAsyncOperation(
    StoreTransaction && storeTx,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    ErrorCode && error,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : FinishVolumeRequestNoopAsyncOperation(move(error), callback, root)
    , storeTx_(move(storeTx))
    , clientRequest_(move(clientRequest))
    , timeoutHelper_(timeout)
{
}

void VolumeManager::FinishVolumeRequestAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = StoreTransaction::BeginCommit(
        move(storeTx_),
        timeoutHelper_.GetRemainingTime(),
        [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
        thisSPtr);
    this->OnCommitComplete(operation, true);
}

void VolumeManager::FinishVolumeRequestAsyncOperation::OnCommitComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode innerError = StoreTransaction::EndCommit(operation);

    if (!innerError.IsSuccess())
    {
        error_ = innerError;
    }
    else
    {
        clientRequest_->CompleteRequest(innerError);
    }

    this->FinishVolumeRequest(operation->Parent);
}