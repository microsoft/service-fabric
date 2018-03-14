// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

StringLiteral const CloseInstanceTag("CloseInstance");
StringLiteral const CloseReplicaTag("CloseReplica");
StringLiteral const CloseReplicatorTag("CloseReplicator");

void FailoverUnitProxy::CloseAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    ProcessActions(thisSPtr, 0);
}

ErrorCode FailoverUnitProxy::CloseAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<FailoverUnitProxy::CloseAsyncOperation>(asyncOperation);
    return thisPtr->Error;
}

void FailoverUnitProxy::CloseAsyncOperation::ProcessActions(AsyncOperationSPtr const & thisSPtr, int nextStep)
{
    if (owner_.State == FailoverUnitProxyStates::Closed)
    {
        // Already closed, nothing to do
        bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
        ASSERT_IFNOT(didComplete, "Failed to complete CloseAsyncOperation");
        return;
    }

    // Note is required that no other actions are executing when close is called on the FUP
    // So, we do not need to honor the locks
    owner_.State = FailoverUnitProxyStates::Closing;
    owner_.UpdateReadWriteStatus();

    if (owner_.CurrentServiceRole != owner_.CurrentReplicatorRole)
    {
        RAPEventSource::Events->CloseServiceReplicatorRoleMismatch(owner_.FailoverUnitId.Guid, owner_.RAPId);

        owner_.Abort(false /*keepFUPOpen*/);

        ASSERT_IF(owner_.statefulService_ || owner_.statelessService_ || owner_.replicator_, 
                "FailoverUnitProxy::CloseAsyncOperation: Holding reference(s) while shutting down");

        bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
        ASSERT_IFNOT(didComplete, "Failed to complete CloseAsyncOperation");

        return;
    }

    for (;;)
    {
        bool completedSynchronously = true;
        bool shouldFinish = false;
        
        RAPEventSource::Events->CloseExecutingNextStep(owner_.FailoverUnitId.Guid, owner_.RAPId, nextStep);

        switch (nextStep)
        {
            case CloseInstance:
                nextStep = CloseReplicator;
                DoCloseInstance(thisSPtr, nextStep, completedSynchronously);
                break;
            case CloseReplicator:
                nextStep = CloseReplica;
                DoCloseReplicator(thisSPtr, nextStep, completedSynchronously);
                break;
            case CloseReplica: 
                nextStep = Done;
                DoCloseReplica(thisSPtr, nextStep, completedSynchronously);
                break;
            case Done:
                nextStep = -1;
                shouldFinish = true;
                break;
            default:
                Assert::CodingError("Unexpected step {0} when processing actions.", nextStep);
        }

        if (!completedSynchronously)
        {
            // Operation is still outstanding, it will resume action processing after it completes
            return;
        }

        // Completed all needed subactions successfully?
        if (shouldFinish)
        {
            RAPEventSource::Events->CloseMarkingFinished(owner_.FailoverUnitId.Guid, owner_.RAPId);

            ASSERT_IF(owner_.statefulService_ || owner_.statelessService_ || owner_.replicator_, 
                "FailoverUnitProxy::CloseAsyncOperation: Holding reference(s) while shutting down");

            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete CloseAsyncOperation");

            return;
        }
    }
}

void FailoverUnitProxy::CloseAsyncOperation::DoCloseInstance(AsyncOperationSPtr const & thisSPtr, int const nextStep, __out bool & isCompletedSynchronously)
{
    if(owner_.replicaState_ == FailoverUnitProxyStates::Opened && owner_.statelessService_)
    {
        AsyncOperationSPtr operation = AsyncOperation::CreateAndStart<CloseInstanceAsyncOperation>(
            owner_,
            [this, nextStep] (AsyncOperationSPtr const & operation) { this->CloseInstanceCompletedCallback(nextStep, operation); },
            thisSPtr);

        if(operation->CompletedSynchronously)
        {
            FinishCloseInstance(operation);
            isCompletedSynchronously = true;
        }
        else
        {
            isCompletedSynchronously = false;
        }
    }
    else
    {
        owner_.AbortInstance();
    
        // Nothing to do, so must be completing synchronously
        isCompletedSynchronously = true;
    }
}

void FailoverUnitProxy::CloseAsyncOperation::CloseInstanceCompletedCallback(int const nextStep, AsyncOperationSPtr const & closeInstanceAsyncOperation)
{
    if(!closeInstanceAsyncOperation->CompletedSynchronously)
    {
        AsyncOperationSPtr const & thisSPtr = closeInstanceAsyncOperation->Parent;
        
        FinishCloseInstance(closeInstanceAsyncOperation);

        ProcessActions(thisSPtr, nextStep);
    }
}

void FailoverUnitProxy::CloseAsyncOperation::FinishCloseInstance(AsyncOperationSPtr const & closeInstanceAsyncOperation)
{
    auto errorCode = CloseInstanceAsyncOperation::End(closeInstanceAsyncOperation);

    if (!errorCode.IsSuccess())
    {
        RAPEventSource::Events->CloseOperationFailed(
            owner_.FailoverUnitId.Guid,
            owner_.RAPId,
            CloseInstanceTag,
            errorCode.ReadValue());

        owner_.AbortInstance();
    }
}

void FailoverUnitProxy::CloseAsyncOperation::DoCloseReplica(AsyncOperationSPtr const & thisSPtr, int const nextStep, __out bool & isCompletedSynchronously)
{
    if(owner_.replicaState_ == FailoverUnitProxyStates::Opened && owner_.statefulService_)
    {
        AsyncOperationSPtr operation = AsyncOperation::CreateAndStart<CloseReplicaAsyncOperation>(
            owner_,
            [this, nextStep] (AsyncOperationSPtr const & operation) { this->CloseReplicaCompletedCallback(nextStep, operation); },
            thisSPtr);

        if(operation->CompletedSynchronously)
        {
            FinishCloseReplica(operation);
            isCompletedSynchronously = true;
        }
        else
        {
            isCompletedSynchronously = false;
        }
    }
    else
    {
        owner_.AbortReplica();

        // Nothing to do, so must be completing synchronously
        isCompletedSynchronously = true;
    }
}

void FailoverUnitProxy::CloseAsyncOperation::CloseReplicaCompletedCallback(int const nextStep, AsyncOperationSPtr const & closeReplicaAsyncOperation)
{
    if(!closeReplicaAsyncOperation->CompletedSynchronously)
    {
        AsyncOperationSPtr const & thisSPtr = closeReplicaAsyncOperation->Parent;
        
        FinishCloseReplica(closeReplicaAsyncOperation);

        ProcessActions(thisSPtr, nextStep);
    }
}

void FailoverUnitProxy::CloseAsyncOperation::FinishCloseReplica(AsyncOperationSPtr const & closeReplicaAsyncOperation)
{
    auto errorCode = CloseReplicaAsyncOperation::End(closeReplicaAsyncOperation);

    if (!errorCode.IsSuccess())
    {
        RAPEventSource::Events->CloseOperationFailed(
            owner_.FailoverUnitId.Guid,
            owner_.RAPId,
            CloseReplicaTag,
            errorCode.ReadValue());

        owner_.AbortReplica();
    }
}

void FailoverUnitProxy::CloseAsyncOperation::DoCloseReplicator(AsyncOperationSPtr const & thisSPtr, int const nextStep, __out bool & isCompletedSynchronously)
{
    if(owner_.replicatorState_ == ReplicatorStates::Opened && owner_.replicator_)
    {
        AsyncOperationSPtr operation = AsyncOperation::CreateAndStart<CloseReplicatorAsyncOperation>(
            owner_,
            [this, nextStep] (AsyncOperationSPtr const & operation) { this->CloseReplicatorCompletedCallback(nextStep, operation); },
            thisSPtr);

        if(operation->CompletedSynchronously)
        {
            FinishCloseReplicator(operation);
            isCompletedSynchronously = true;
        }
        else
        {
            isCompletedSynchronously = false;
        }
    }
    else
    {
        owner_.AbortReplicator();

        // Nothing to do, so must be completing synchronously
        isCompletedSynchronously = true;
    }
}

void FailoverUnitProxy::CloseAsyncOperation::CloseReplicatorCompletedCallback(int const nextStep, AsyncOperationSPtr const & closeReplicatorAsyncOperation)
{
    if(!closeReplicatorAsyncOperation->CompletedSynchronously)
    {
        AsyncOperationSPtr const & thisSPtr = closeReplicatorAsyncOperation->Parent;
        
        FinishCloseReplicator(closeReplicatorAsyncOperation);

        ProcessActions(thisSPtr, nextStep);
    }
}

void FailoverUnitProxy::CloseAsyncOperation::FinishCloseReplicator(AsyncOperationSPtr const & closeReplicatorAsyncOperation)
{
    auto errorCode = CloseReplicatorAsyncOperation::End(closeReplicatorAsyncOperation);

    if (!errorCode.IsSuccess())
    {
        RAPEventSource::Events->CloseOperationFailed(
            owner_.FailoverUnitId.Guid,
            owner_.RAPId,
            CloseReplicatorTag,
            errorCode.ReadValue());

        owner_.AbortReplicator();
    }
}
