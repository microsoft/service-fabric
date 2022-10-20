// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Transport;
using namespace Management;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Naming;
using namespace BackupRestoreAgentComponent;

StringLiteral const TraceComponent("NodeToServiceAsyncOperation");

NodeToServiceAsyncOperation::NodeToServiceAsyncOperation(
    MessageUPtr && message,
    RequestReceiverContextUPtr && receiverContext,
    IpcServer & ipcServer,
    ServiceAdminClient & serviceAdminClient,
    IHostingSubsystemSPtr hosting,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : RequestResponseAsyncOperationBase(move(message), timeout, callback, parent)
    , ipcServer_(ipcServer)
    , serviceAdminClient_(serviceAdminClient)
    , receiverContext_(move(receiverContext))
    , hosting_(hosting)
{
}

NodeToServiceAsyncOperation::~NodeToServiceAsyncOperation()
{
}

void NodeToServiceAsyncOperation::OnStart(Common::AsyncOperationSPtr const & thisSPtr)
{
    RequestResponseAsyncOperationBase::OnStart(thisSPtr);
    wstring hostId;
    ErrorCode error;

    if (receivedMessage_->Headers.Action == BAMessage::ForwardToBRSAction)
    {
        error = this->GetHostId(
            ServiceTypeIdentifier::BackupRestoreServiceTypeId, 
            ServicePackageVersionInstance(), 
            SystemServiceApplicationNameHelper::SystemServiceApplicationName, 
            hostId);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        ForwardToService(hostId, thisSPtr);
    }
    else if (receivedMessage_->Headers.Action == BAMessage::ForwardToBAPAction)
    {
        // Get the ServiceTarget header
        ServiceTargetHeader serviceHeader;

        if (!receivedMessage_->Headers.TryReadFirst(serviceHeader))
        {
            // Header not found. Send error
            WriteWarning(TraceComponent, "{0} Service Header not found", this->ActivityId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
            return;
        }

        auto operation1 = serviceAdminClient_.BeginGetServiceDescription(
            serviceHeader.ServiceName.Name, 
            FabricActivityHeader(), 
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnGetServiceDescriptionCompleted(operation, false);
            },
            thisSPtr);

        this->OnGetServiceDescriptionCompleted(operation1, true);
    }
}

void NodeToServiceAsyncOperation::OnGetServiceDescriptionCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    ServiceDescriptionReplyMessageBody serviceDescriptionMsg; 
    auto error = serviceAdminClient_.EndGetServiceDescription(operation, serviceDescriptionMsg);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceComponent, "{0} EndGetServiceDescription failed with error {1]", this->ActivityId, error);
        this->TryComplete(thisSPtr, error);
        return;
    }

    // Get the PartitionTarget header
    PartitionTargetHeader partitionHeader;
    if (!receivedMessage_->Headers.TryReadFirst(partitionHeader))
    {
        // Header not found. Send error
        WriteWarning(TraceComponent, "{0} Partition Header not found", this->ActivityId);
        this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        return;
    }

    wstring hostId;

    error = this->GetHostId(
        serviceDescriptionMsg.ServiceDescription.Type, 
        serviceDescriptionMsg.ServiceDescription.PackageVersionInstance,
        serviceDescriptionMsg.ServiceDescription.ApplicationName,
        serviceDescriptionMsg.ServiceDescription.CreateActivationContext(partitionHeader.PartitionId),
        hostId);

    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
        return;
    }

    ForwardToService(hostId, thisSPtr);
}

void NodeToServiceAsyncOperation::ForwardToService(wstring & hostId, AsyncOperationSPtr const & thisSPtr)
{
    // Unwrap for BA here
    BAMessage::UnwrapForBA(*receivedMessage_);

    // Update the timeoutheader
    receivedMessage_->Headers.Replace(TimeoutHeader(this->RemainingTime));

    WriteInfo(TraceComponent, "{0} Forwarding message to service with action {1} and messageid {2}",
        this->ActivityId,
        receivedMessage_->Action,
        receivedMessage_->MessageId);

    auto operation = this->ipcServer_.BeginRequest(
        move(receivedMessage_),
        hostId,
        this->RemainingTime,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnForwardToServiceComplete(operation, false);
        },
        thisSPtr);

    this->OnForwardToServiceComplete(operation, true);
}

ErrorCode NodeToServiceAsyncOperation::GetHostId(
    ServiceTypeIdentifier const & serviceTypeId,
    ServicePackageVersionInstance const & packageVersionInstance,
    wstring const & applicationName,
    __out wstring& hostId)
{
    return this->GetHostId(
        serviceTypeId,
        packageVersionInstance,
        applicationName,
        ServicePackageActivationContext(),
        hostId);
}

ErrorCode NodeToServiceAsyncOperation::GetHostId(
    ServiceTypeIdentifier const & serviceTypeId,
    ServicePackageVersionInstance const & packageVersionInstance,
    wstring const & applicationName,
    ServicePackageActivationContext const& activationContext,
    __out wstring& hostId)
{
    auto error = hosting_->GetHostId(
        VersionedServiceTypeIdentifier(serviceTypeId, packageVersionInstance),
        applicationName,
        activationContext,
        hostId);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "Host Id for service type {0} not found {1}",
            serviceTypeId,
            error);
    }

    return error;
}

void NodeToServiceAsyncOperation::OnForwardToServiceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
    auto const & thisSPtr = operation->Parent;
    MessageUPtr reply;
    ErrorCode error = ipcServer_.EndRequest(operation, reply);

    if (error.IsSuccess())
    {
        error = BAMessage::ValidateIpcReply(*reply);
        if (error.IsSuccess())
        {
            this->SetReplyAndComplete(thisSPtr, move(reply), error);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void NodeToServiceAsyncOperation::OnCompleted()
{
    RequestResponseAsyncOperationBase::OnCompleted();
}

Common::ErrorCode NodeToServiceAsyncOperation::End(
    Common::AsyncOperationSPtr const & asyncOperation,
    __out Transport::MessageUPtr & reply,
    __out Federation::RequestReceiverContextUPtr & receiverContext)
{
    auto casted = AsyncOperation::End<NodeToServiceAsyncOperation>(asyncOperation);
    auto error = casted->Error;

    receiverContext = move(casted->ReceiverContext);
    if (error.IsSuccess())
    {
        reply = std::move(casted->reply_);
    }

    return error;
}
