// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

StringLiteral const TraceLifeCycle("LifeCycle");

void ReconfigurationAgentProxy::CloseAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    RAPEventSource::Events->LifeCycleClosing(owner_.id_);

    owner_.ipcClient_.UnregisterMessageHandler(RAMessage::Actor);

    if (owner_.messageQueue_)
    {
        owner_.messageQueue_->Close();
    }

    owner_.LocalLoadReportingComponentObj.Close();
    owner_.LocalMessageSenderComponentObj.Close();
    owner_.LocalHealthReportingComponentObj.Close();
    
    map<FailoverUnitId, FailoverUnitProxySPtr> lfupmContents(owner_.LocalFailoverUnitProxyMapObj.PrivatizeLFUPM());

    for(auto iter = lfupmContents.begin(); iter != lfupmContents.end(); ++iter)
    {
        FailoverUnitProxySPtr fup = iter->second;
        outstandingOperations_++;

        auto operation = fup->BeginMarkForCloseAndDrainOperations(
            false,
            [this, fup] (AsyncOperationSPtr const & operation) { this->DrainCompletedCallback(fup, operation); },
            thisSPtr);

        if (operation->CompletedSynchronously)
        {
            FinishDrain(fup, operation);
        }
    }

    if ((--outstandingOperations_) == 0)
    {
        thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
}

ErrorCode ReconfigurationAgentProxy::CloseAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<ReconfigurationAgentProxy::CloseAsyncOperation>(asyncOperation);
    return thisPtr->Error;
}

void ReconfigurationAgentProxy::CloseAsyncOperation::DrainCompletedCallback(FailoverUnitProxySPtr const & fup, AsyncOperationSPtr const & drainAsyncOperation)
{
    if (!drainAsyncOperation->CompletedSynchronously)
    {
        FinishDrain(fup, drainAsyncOperation);
    }
}

void ReconfigurationAgentProxy::CloseAsyncOperation::FinishDrain(FailoverUnitProxySPtr const & fup, AsyncOperationSPtr const & drainAsyncOperation)
{    
    ErrorCode errorCode = fup->EndMarkForCloseAndDrainOperations(drainAsyncOperation);
        
    if (errorCode.IsSuccess())
    {
        auto operation = fup->BeginClose(
            [this, fup] (AsyncOperationSPtr const & operation) { this->CloseCompletedCallback(fup, operation); },
            drainAsyncOperation->Parent);

        if (operation->CompletedSynchronously)
        {
            FinishClose(fup, operation);
        }
    }
    else
    {
        // It is possible that RAP Close is canceled
        AsyncOperationSPtr const & thisSPtr = drainAsyncOperation->Parent;
        thisSPtr->TryComplete(thisSPtr, errorCode);
    }
}

void ReconfigurationAgentProxy::CloseAsyncOperation::CloseCompletedCallback(FailoverUnitProxySPtr const & fup, AsyncOperationSPtr const & closeAsyncOperation)
{
    if (!closeAsyncOperation->CompletedSynchronously)
    {
        FinishClose(fup, closeAsyncOperation);
    }
}

void ReconfigurationAgentProxy::CloseAsyncOperation::FinishClose(FailoverUnitProxySPtr const & fup, AsyncOperationSPtr const & closeAsyncOperation)
{    
    ErrorCode errorCode = fup->EndClose(closeAsyncOperation);
        
    AsyncOperationSPtr const & thisSPtr = closeAsyncOperation->Parent;

    if ((--outstandingOperations_) == 0)
    {
        thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
}
