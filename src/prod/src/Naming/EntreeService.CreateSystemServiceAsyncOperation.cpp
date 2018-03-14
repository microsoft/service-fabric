// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;
using namespace SystemServices;
using namespace Transport;

using namespace Naming;

StringLiteral const TraceComponent("CreateSystemService");

EntreeService::CreateSystemServiceAsyncOperation::CreateSystemServiceAsyncOperation(
    __in GatewayProperties & properties,
    MessageUPtr && request,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : RequestAsyncOperationBase(properties, move(request), timeout, callback, parent)
    , svcDescription_()
    , cuid_(Guid::NewGuid())
{
}

void EntreeService::CreateSystemServiceAsyncOperation::OnRetry(Common::AsyncOperationSPtr const & thisSPtr)
{
    this->CreateSystemService(thisSPtr);
}

void EntreeService::CreateSystemServiceAsyncOperation::OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr)
{
    auto error = this->InitializeServiceDescription();

    if (error.IsSuccess())
    {
        this->CreateSystemService(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

ErrorCode EntreeService::CreateSystemServiceAsyncOperation::InitializeServiceDescription()
{
    CreateSystemServiceMessageBody body;
    if (this->ReceivedMessage->GetBody(body))
    {
        if (SystemServiceApplicationNameHelper::IsPublicServiceName(body.ServiceDescription.Name))
        {
            WriteWarning(
                TraceComponent, 
                "{0}: system service name must be unqualified: name={1}",
                this->TraceId,
                body.ServiceDescription.Name);

            return ErrorCodeValue::AccessDenied;
        }
        else
        {
            WriteInfo(
                TraceComponent, 
                "{0}: creating system service: {1}",
                this->TraceId,
                body.ServiceDescription);

            return NamingService::CreateSystemServiceDescriptionFromTemplate(
                this->ActivityHeader.ActivityId,
                body.ServiceDescription, 
                svcDescription_);
        }
    }
    else
    {
        return ErrorCode::FromNtStatus(this->ReceivedMessage->GetStatus());
    }
}

void EntreeService::CreateSystemServiceAsyncOperation::CreateSystemService(AsyncOperationSPtr const & thisSPtr)
{
    vector<ConsistencyUnitDescription> cuids;
    cuids.push_back(cuid_);

    auto operation = this->Properties.AdminClient.BeginCreateService(
        svcDescription_,
        cuids,
        this->ActivityHeader,
        this->RemainingTime,
        [this](AsyncOperationSPtr const & operation) { this->OnCreateServiceCompleted(operation, false); },
        thisSPtr);
    this->OnCreateServiceCompleted(operation, true);
}

void EntreeService::CreateSystemServiceAsyncOperation::OnCreateServiceCompleted(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (expectedCompletedSynchronously != operation->CompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    CreateServiceReplyMessageBody replyBody;
    auto error = this->Properties.AdminClient.EndCreateService(operation, replyBody);

    if (error.IsSuccess())
    {
        error = replyBody.ErrorCodeValue;
    }

    if (error.IsSuccess())
    {
        this->SetReplyAndComplete(thisSPtr, NamingMessage::GetServiceOperationReply(), error);
    }
    else if (NamingErrorCategories::IsRetryableAtGateway(error))
    {
        WriteInfo(
            TraceComponent, 
            "{0}: retrying create system service: name={1} cuid={2} error={3}",
            this->TraceId,
            svcDescription_.Name,
            cuid_,
            error);

        this->HandleRetryStart(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}
