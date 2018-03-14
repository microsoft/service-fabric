// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::Assert;
using Common::AsyncCallback;
using Common::AsyncOperation;
using Common::AsyncOperationSPtr;
using Common::ComPointer;
using Common::ComponentRoot;
using Common::ErrorCode;

OperationStream::OperationStream(
    bool isReplicationStream,
    SecondaryReplicatorSPtr const & secondary,
    DispatchQueueSPtr const & dispatchQueue,
    Common::Guid const & partitionId,
    ReplicationEndpointId const & endpointUniqueId,
    bool supportsFaults)
    :   Common::ComponentRoot(),
        secondary_(secondary),
        dispatchQueue_(dispatchQueue),
        partitionId_(partitionId),
        purpose_(isReplicationStream ? Constants::ReplOperationTrace : Constants::CopyOperationTrace),
        checkUpdateEpochOperations_(isReplicationStream),
        endpointUniqueId_(endpointUniqueId),
        supportsFaults_(supportsFaults)
{
    ReplicatorEventSource::Events->OperationStreamCtor(
        partitionId_, 
        endpointUniqueId_,
        purpose_);
}

OperationStream::~OperationStream()
{
    ReplicatorEventSource::Events->OperationStreamDtor(
        partitionId_, 
        endpointUniqueId_,
        purpose_);
}

// ******* GetOperation *******
AsyncOperationSPtr OperationStream::BeginGetOperation(
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
{
    AsyncOperationSPtr getOperation = AsyncOperation::CreateAndStart<GetOperationAsyncOperation>(*this, callback, state);
    AsyncOperation::Get<GetOperationAsyncOperation>(getOperation)->ResumeOutsideLock(getOperation);
    return getOperation;
}

ErrorCode OperationStream::EndGetOperation(
    AsyncOperationSPtr const & asyncOperation, 
    __out IFabricOperation * & operation)
{
    return GetOperationAsyncOperation::End(asyncOperation, operation);
}

ErrorCode OperationStream::ReportFault(
    __in FABRIC_FAULT_TYPE faultType)
{
    if (!supportsFaults_)
    {
        // If the config is not enabled, do nothing
        return Common::ErrorCodeValue::InvalidOperation;
    }

    return ReportFaultInternal(
        faultType, 
        L"State provider faulted stream");
}

ErrorCode OperationStream::ReportFaultInternal(
    __in FABRIC_FAULT_TYPE faultType,
    __in std::wstring const & errorMessage)
{
    secondary_->OnStreamFault(faultType, errorMessage);
    return ErrorCode();
}

} // end namespace ReplicationComponent
} // end namespace Reliability
