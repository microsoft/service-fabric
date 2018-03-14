// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::FaultAnalysisService;
using namespace Query;
using namespace ServiceModel;
using namespace ClientServerTransport;
using namespace Transport;

StringLiteral const TraceComponent("ProcessQueryAsyncOperation");

void ProcessQueryAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    wstring actionStateFilterString;
    wstring actionTypeFilterString;
    DWORD actionStateFilter = 0;
    DWORD actionTypeFilter = 0;

    AsyncOperationSPtr inner = nullptr;

    Trace.WriteNoise(TraceComponent, "FaultAnalysisServiceAgent::ProcessQueryAsyncOperation::OnStart - queryName='{0}'", queryName_);
    if (queryName_ == Query::QueryNames::GetTestCommandList)
    {
        if (queryArgs_.TryGetValue(QueryResourceProperties::Action::StateFilter, actionStateFilterString))
        {
            if (!StringUtility::TryFromWString<DWORD>(actionStateFilterString, actionStateFilter))
            {
                TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
            }
        }

        if (queryArgs_.TryGetValue(QueryResourceProperties::Action::TypeFilter, actionTypeFilterString))
        {
            if (!StringUtility::TryFromWString<DWORD>(actionTypeFilterString, actionTypeFilter))
            {
                TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);

            }
        }

        inner = owner_.BeginSendQueryToService(
            (FABRIC_TEST_COMMAND_STATE_FILTER)actionStateFilter,
            (FABRIC_TEST_COMMAND_TYPE_FILTER)actionTypeFilter,
            activityId_,
            this->RemainingTime,
            [this](AsyncOperationSPtr const& asyncOperation)
        {
            OnRequestCompleted(asyncOperation, true, false);
        },
            thisSPtr);
        OnRequestCompleted(inner, true, true);
    }
    else if (queryName_ == Query::QueryNames::GetNodeList)
    {
        inner = owner_.BeginGetStoppedNodeList(
            activityId_,
            this->RemainingTime,
            [this](AsyncOperationSPtr const& asyncOperation)
        {
            OnRequestCompleted(asyncOperation, false, false);
        },
            thisSPtr);
        OnRequestCompleted(inner, false, true);
    }
    else
    {
        Trace.WriteWarning(TraceComponent, "FaultAnalysisServiceAgent::ProcessQueryAsyncOperation::OnStart - unexpected queryName='{0}'", queryName_);
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
        error = owner_.EndSendQueryToService(asyncOperation, reply);
    }
    else
    { 
        error = owner_.EndGetStoppedNodeList(asyncOperation, reply);    
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

bool ProcessQueryAsyncOperation::IsRetryable(Common::ErrorCode const & error)
{
    switch (error.ReadValue())
    {
    case ErrorCodeValue::NoWriteQuorum:
    case ErrorCodeValue::ReconfigurationPending:
    case ErrorCodeValue::CMRequestAlreadyProcessing:

        //case ErrorCodeValue::StoreRecordAlreadyExists: // same as StoreWriteConflict
    case ErrorCodeValue::StoreOperationCanceled:
    case ErrorCodeValue::StoreWriteConflict:
    case ErrorCodeValue::StoreSequenceNumberCheckFailed:

    case ErrorCodeValue::CMImageBuilderRetryableError:
        return true;

    default:
        return false;
    }
}
