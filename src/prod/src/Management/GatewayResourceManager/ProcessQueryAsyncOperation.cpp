// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::GatewayResourceManager;
using namespace Query;
using namespace ServiceModel;
using namespace ClientServerTransport;
using namespace Transport;
using namespace ServiceModel::ModelV2;

StringLiteral const TraceComponent("ProcessQueryAsyncOperation");

void ProcessQueryAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    wstring gatewayNameFilterString;

    AsyncOperationSPtr inner = nullptr;

    Trace.WriteNoise(TraceComponent, "GatewayResourceManagerAgent::ProcessQueryAsyncOperation::OnStart - queryName='{0}'", queryName_);
    if (queryName_ == Query::QueryNames::GetGatewayResourceList)
    {
        GatewayResourceQueryDescription description;
        description.GetDescriptionFromQueryArgumentMap(queryArgs_);

        inner = owner_.BeginGetGatewayResourceList(
            description,
            this->RemainingTime,
            [this](AsyncOperationSPtr const& asyncOperation)
        {
            OnRequestCompleted(asyncOperation, true, false);
        },
            thisSPtr);
        OnRequestCompleted(inner, true, true);
    }
    else
    {
        Trace.WriteWarning(TraceComponent, "GatewayResourceManagerAgent::ProcessQueryAsyncOperation::OnStart - unexpected queryName='{0}'", queryName_);
        TryComplete(thisSPtr, ErrorCodeValue::NotImplemented);
    }
}

void ProcessQueryAsyncOperation::OnRequestCompleted(
     AsyncOperationSPtr const & asyncOperation,
     bool isCommandStatusQuery, 
     bool expectedCompletedSynchronously)
{
    if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    MessageUPtr reply;
    ErrorCode error = ErrorCodeValue::NotImplemented;

    if (isCommandStatusQuery)
    {
        error = owner_.EndGetGatewayResourceList(asyncOperation, reply);
    }

    if (error.IsSuccess())
    {
        reply_ = move(reply);
    }
    else
    {
        Trace.WriteWarning(TraceComponent, "ProcessQueryAsyncOperation::OnRequestCompleted returned error='{0}'", error);
    }
    
    TryComplete(asyncOperation->Parent, error);  
}

ErrorCode ProcessQueryAsyncOperation::End(Common::AsyncOperationSPtr const & asyncOperation, __out Transport::MessageUPtr &reply)
{
    auto casted = AsyncOperation::End<ProcessQueryAsyncOperation>(asyncOperation);
    if (casted->Error.IsSuccess())
    {
        if (casted->reply_)
        {
            reply = move(casted->reply_);
        }
    }

    return casted->Error;
}


void ProcessQueryAsyncOperation::OnTimeout(AsyncOperationSPtr const & thisSPtr)
{
    UNREFERENCED_PARAMETER(thisSPtr);

    this->CancelRetryTimer();
}

void ProcessQueryAsyncOperation::OnCancel()
{
    this->CancelRetryTimer();
}

void ProcessQueryAsyncOperation::OnCompleted()
{
    TimedAsyncOperation::OnCompleted();

    this->CancelRetryTimer();
}

void ProcessQueryAsyncOperation::CancelRetryTimer()
{
    TimerSPtr snap;
    {
        AcquireExclusiveLock lock(timerLock_);
        snap = retryTimer_;
    }

    if (snap)
    {
        snap->Cancel();
    }
}