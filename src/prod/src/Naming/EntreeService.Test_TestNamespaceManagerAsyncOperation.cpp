// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

using namespace Api;
using namespace Client;
using namespace ClientServerTransport;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ServiceModel;
using namespace SystemServices;
using namespace Transport;

using namespace Naming;

StringLiteral const TraceComponent("Test_TestNamespaceManager");

EntreeService::Test_TestNamespaceManagerAsyncOperation::Test_TestNamespaceManagerAsyncOperation(
    __in GatewayProperties & properties,
    MessageUPtr && request,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : RequestAsyncOperationBase(properties, move(request), timeout, callback, parent)
{
}

bool EntreeService::Test_TestNamespaceManagerAsyncOperation::AccessCheck() 
{ 
    return this->ReceivedMessage->IsInRole(Transport::RoleMask::Admin);
}

void EntreeService::Test_TestNamespaceManagerAsyncOperation::OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr)
{
    ServiceTargetHeader serviceTargetHeader;
    if (this->ReceivedMessage->Headers.TryReadFirst(serviceTargetHeader))
    {
        auto targetServiceName = serviceTargetHeader.ServiceName.ToString();
        if (SystemServiceApplicationNameHelper::IsPublicServiceName(targetServiceName))
        {
            auto operation = this->Properties.SystemServiceClient->BeginResolve(
                targetServiceName,
                this->ActivityHeader.ActivityId,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnResolveCompleted(operation, false); },
                thisSPtr);
            this->OnResolveCompleted(operation, true);
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                "{0}: not a system service: name={1}",
                this->TraceId,
                targetServiceName);

            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidNameUri);
        }
    }
    else
    {
        auto error = ErrorCode::FromNtStatus(this->ReceivedMessage->GetStatus());

        WriteWarning(
            TraceComponent,
            "{0} Failed to read ServiceTargetHeader: {1}",
            this->TraceId,
            error);

        this->TryComplete(thisSPtr, error);
    }
}

void EntreeService::Test_TestNamespaceManagerAsyncOperation::OnResolveCompleted(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (expectedCompletedSynchronously != operation->CompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    SystemServiceLocation primaryLocation;
    ISendTarget::SPtr primaryTarget;
    auto error = this->Properties.SystemServiceClient->EndResolve(operation, primaryLocation, primaryTarget);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} Failed to resolve NamespaceManager: {1}",
            this->TraceId,
            error);

        this->TryComplete(thisSPtr, error);
    }
    else
    {
        this->SendRequest(thisSPtr, primaryLocation, primaryTarget);
    }
}

void EntreeService::Test_TestNamespaceManagerAsyncOperation::SendRequest(
    AsyncOperationSPtr const & thisSPtr, 
    SystemServiceLocation const & primaryLocation, 
    ISendTarget::SPtr const & primaryTarget)
{
    ForwardMessageHeader forwardHeader;
    if (this->ReceivedMessage->Headers.TryGetAndRemoveHeader(forwardHeader))
    {
        auto request = this->ReceivedMessage->Clone();

        request->Headers.Replace(ActorHeader(forwardHeader.Actor));

        ServiceDirectMessagingAgentMessage::WrapServiceRequest(*request, primaryLocation);

        auto operation = this->Properties.ReceivingChannel->Test_BeginRequestReply(
            move(request),
            primaryTarget,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnRequestCompleted(operation, false); },
            thisSPtr);
        this->OnRequestCompleted(operation, true);
    }
    else
    {
        WriteWarning(
            TraceComponent, 
            "{0} ForwardMessageHeader missing: {1}", 
            this->TraceId, 
            *this->ReceivedMessage);

        this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
    }
}

void EntreeService::Test_TestNamespaceManagerAsyncOperation::OnRequestCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    MessageUPtr reply;
    auto error = this->Properties.ReceivingChannel->Test_EndRequestReply(operation, reply);

    if (error.IsSuccess())
    {
        error = ServiceDirectMessagingAgentMessage::UnwrapServiceReply(*reply);

        if (error.IsSuccess())
        {
            this->SetReplyAndComplete(operation->Parent, move(reply), error);
        }
    }

    this->TryComplete(operation->Parent, error);
}
