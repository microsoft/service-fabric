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

StringLiteral const TraceComponent("DeleteSystemService");

EntreeService::DeleteSystemServiceAsyncOperation::DeleteSystemServiceAsyncOperation(
    __in GatewayProperties & properties,
    MessageUPtr && request,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : RequestAsyncOperationBase(properties, move(request), timeout, callback, parent)
    , svcName_()
{
}

void EntreeService::DeleteSystemServiceAsyncOperation::OnRetry(Common::AsyncOperationSPtr const & thisSPtr)
{
    this->DeleteSystemService(thisSPtr);
}

void EntreeService::DeleteSystemServiceAsyncOperation::OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr)
{
    auto error = this->InitializeServiceName();

    if (error.IsSuccess())
    {
        this->DeleteSystemService(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

ErrorCode EntreeService::DeleteSystemServiceAsyncOperation::InitializeServiceName()
{
    DeleteSystemServiceMessageBody body;
    if (this->ReceivedMessage->GetBody(body))
    {
        if (SystemServiceApplicationNameHelper::IsNamespaceManagerName(body.ServiceName))
        {
            svcName_ = SystemServiceApplicationNameHelper::GetInternalServiceName(body.ServiceName);

            return ErrorCodeValue::Success;
        }
        else if (SystemServiceApplicationNameHelper::IsPublicServiceName(body.ServiceName))
        {
            WriteWarning(
                TraceComponent, 
                "{0}: cannot delete system service: name={1}",
                this->TraceId,
                body.ServiceName);

            return ErrorCodeValue::AccessDenied;
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                "{0}: not a system service: name={1}",
                this->TraceId,
                body.ServiceName);

            return ErrorCodeValue::InvalidNameUri;
        }
    }
    else
    {
        return ErrorCode::FromNtStatus(this->ReceivedMessage->GetStatus());
    }
}

void EntreeService::DeleteSystemServiceAsyncOperation::DeleteSystemService(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = this->Properties.AdminClient.BeginDeleteService(
        svcName_,
        this->ActivityHeader,
        this->RemainingTime,
        [this](AsyncOperationSPtr const & operation) { this->OnDeleteServiceCompleted(operation, false); },
        thisSPtr);
    this->OnDeleteServiceCompleted(operation, true);
}

void EntreeService::DeleteSystemServiceAsyncOperation::OnDeleteServiceCompleted(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (expectedCompletedSynchronously != operation->CompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->Properties.AdminClient.EndDeleteService(operation);

    // FM will continue returning FMServiceDeleteInProgress until all replicas are dropped.
    // Block delete until FM finishes dropping all replicas since returning early just
    // causes confusion for users when the application/service is gone from CM/Naming,
    // but remains running in the system.
    //
    if (error.IsError(ErrorCodeValue::FMServiceDoesNotExist))
    {
        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess())
    {
        MessageUPtr message = NamingMessage::GetDeleteSystemServiceReply();

        this->SetReplyAndComplete(thisSPtr, move(message), error);
    }
    else if (NamingErrorCategories::IsRetryableAtGateway(error) || error.IsError(ErrorCodeValue::FMServiceDeleteInProgress))
    {
        WriteInfo(
            TraceComponent, 
            "{0}: retrying delete system service: name={1} error={2}",
            this->TraceId,
            svcName_,
            error);

        this->HandleRetryStart(thisSPtr);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void EntreeService::DeleteSystemServiceAsyncOperation::OnCompleted()
{
    // Delete the cached entry in all cases, even if the delete wasn't successful
    this->Properties.PsdCache.TryRemove(this->svcName_);
    RequestAsyncOperationBase::OnCompleted();
}
