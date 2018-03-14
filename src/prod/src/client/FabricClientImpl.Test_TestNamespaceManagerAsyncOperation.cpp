// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "Communication/NamespaceManager/NamespaceManagerMessage.h"

using namespace std;

using namespace Api;
using namespace ClientServerTransport;
using namespace Common;
using namespace Communication::NamespaceManager;
using namespace Reliability;
using namespace ServiceModel;
using namespace SystemServices;
using namespace Transport;
using namespace Naming;

using namespace Client;

FabricClientImpl::Test_TestNamespaceManagerAsyncOperation::Test_TestNamespaceManagerAsyncOperation(
    __in FabricClientImpl & owner,
    wstring const & svcName,
    size_t byteCount,
    ISendTarget::SPtr const & directTarget,
    SystemServiceLocation const & primaryLocation,
    bool gatewayProxy,
    TimeSpan const timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent)
    , owner_(owner)
    , svcName_(svcName)
    , byteCount_(byteCount)
    , directTarget_(directTarget)
    , primaryLocation_(primaryLocation)
    , gatewayProxy_(gatewayProxy)
    , timeoutHelper_(timeout)
{
}

ErrorCode FabricClientImpl::Test_TestNamespaceManagerAsyncOperation::End(AsyncOperationSPtr const & operation, __out vector<byte> & result)
{
    auto casted = AsyncOperation::End<Test_TestNamespaceManagerAsyncOperation>(operation);

    if (casted->Error.IsSuccess())
    {
        result.swap(casted->result_);
    }

    return casted->Error;
}

void FabricClientImpl::Test_TestNamespaceManagerAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (!svcName_.empty())
    {
        auto targetServiceName = SystemServiceApplicationNameHelper::IsPublicServiceName(svcName_)
            ? svcName_
            : SystemServiceApplicationNameHelper::CreatePublicNamespaceManagerServiceName(svcName_);

        if (!NamingUri::TryParse(targetServiceName, svcUri_))
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidNameUri);
            return;
        }
    }

    auto message = NamespaceManagerMessage::CreateTestRequest(ActivityId(), byteCount_);

    if (directTarget_)
    {
        ServiceDirectMessagingAgentMessage::WrapServiceRequest(*message, primaryLocation_);

        auto operation = owner_.clientConnectionManager_->Test_BeginSend(
            move(message),
            directTarget_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnDirectSendCompleted(operation, false); },
            thisSPtr);
        this->OnDirectSendCompleted(operation, true);
    }
    else if (gatewayProxy_)
    {
        AddServiceTargetHeader(*message);
        message->Headers.Replace(ForwardMessageHeader(message->Actor, message->Action));

        // Replaces only the actor for the gateway to proxy the direct client request
        // (i.e. the Action is not "Forward")
        //
        message->Headers.Replace(ActorHeader(Actor::NamingGateway));

        auto operation = AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            owner_,
            NamingUri(),
            move(message),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnRequestCompleted(operation, false); },
            thisSPtr,
            ErrorCodeValue::Success);
        this->OnRequestCompleted(operation, true);
    }
    else
    {
        AddServiceTargetHeader(*message);
        ServiceRoutingAgentMessage::WrapForRoutingAgent(
            *message,
            ServiceTypeIdentifier::NamespaceManagerServiceTypeId);

        auto operation = AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            owner_,
            move(message),
            NamingUri(),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnForwardCompleted(operation, false); },
            thisSPtr);
        this->OnForwardCompleted(operation, true);
    }
}

void FabricClientImpl::Test_TestNamespaceManagerAsyncOperation::OnDirectSendCompleted(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ClientServerReplyMessageUPtr reply;
    auto error = owner_.clientConnectionManager_->Test_EndSend(operation, reply);

    if (error.IsSuccess())
    {
        error = ServiceDirectMessagingAgentMessage::UnwrapServiceReply(*reply);
    }

    this->ProcessReply(operation->Parent, reply, error);
}

void FabricClientImpl::Test_TestNamespaceManagerAsyncOperation::OnRequestCompleted(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ClientServerReplyMessageUPtr reply;
    auto error = RequestReplyAsyncOperation::End(operation, reply);

    this->ProcessReply(operation->Parent, reply, error);
}

void FabricClientImpl::Test_TestNamespaceManagerAsyncOperation::OnForwardCompleted(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ClientServerReplyMessageUPtr reply;
    auto error = ForwardToServiceAsyncOperation::End(operation, reply);

    this->ProcessReply(operation->Parent, reply, error);
}

void FabricClientImpl::Test_TestNamespaceManagerAsyncOperation::ProcessReply(
    AsyncOperationSPtr const & thisSPtr, 
    __in ClientServerReplyMessageUPtr & reply, 
    __in ErrorCode & error)
{
    if (error.IsSuccess())
    {
        Communication::NamespaceManager::TestReplyBody body;
        if (reply->GetBody(body))
        {
            error = body.GetErrorDetails().TakeError();
            
            if (error.IsSuccess())
            {
                result_.swap(*body.GetData());
            }
        }
        else
        {
            error = ErrorCode::FromNtStatus(reply->GetStatus());
        }
    }

    this->TryComplete(thisSPtr, error);
}

void FabricClientImpl::Test_TestNamespaceManagerAsyncOperation::AddServiceTargetHeader(__in ClientServerRequestMessage & message)
{
    message.Headers.Add(ServiceTargetHeader(svcUri_));
}
