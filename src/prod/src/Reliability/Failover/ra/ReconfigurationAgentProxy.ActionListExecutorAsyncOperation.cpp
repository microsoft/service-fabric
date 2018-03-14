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

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    ProcessNextAction(thisSPtr);
}

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CreateAndStart(
    ReconfigurationAgentProxy & owner,
    FailoverUnitProxyContext<ProxyRequestMessageBody> && context,
    ProxyActionsListTypes::Enum actionsList,
    std::function<void(Common::AsyncOperationSPtr const &)> completionCallback,
    bool continueOnFailure)
{
    auto operation = AsyncOperation::CreateAndStart<ReconfigurationAgentProxy::ActionListExecutorAsyncOperation>(
        owner,
        std::move(context),
        actionsList,
        [completionCallback](AsyncOperationSPtr const & inner) { if (!inner->CompletedSynchronously) completionCallback(inner); },
        owner.Root.CreateAsyncOperationRoot(),
        continueOnFailure);

    if (operation->CompletedSynchronously)
    {
        completionCallback(operation);
    }
}

ErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation, 
    __out FailoverUnitProxyContext<ProxyRequestMessageBody>* & context,
    __out ProxyActionsListTypes::Enum & actionsList,  
    __out ProxyReplyMessageBody & reply)
{
    auto thisPtr = AsyncOperation::End<ReconfigurationAgentProxy::ActionListExecutorAsyncOperation>(asyncOperation);

    context = &(thisPtr->context_);
    actionsList = thisPtr->actionListName_;

    ASSERT_IF(actionsList == ProxyActionsListTypes::ReplicatorGetQuery, "ReplicatorGetQuery action list must invoke the other overload of End");

    // Workaround for SQL Server 730204
    // Stamp service endpoint and replication endpoint on all messages to workaround race
    // TODO: Remove after race condition is addressed
    thisPtr->replyLocalReplica_.ServiceLocation = thisPtr->context_.FailoverUnitProxy->ReplicaDescription.ServiceLocation;
    thisPtr->replyLocalReplica_.ReplicationEndpoint = thisPtr->context_.FailoverUnitProxy->ReplicaDescription.ReplicationEndpoint;

    ProxyMessageFlags::Enum flags = context->Body.Flags;

    if(actionsList == ProxyActionsListTypes::ReplicatorUpdateReplicas)
    {
        // Even though the request contained flag for Catchup, it might have been converted to updatereplicas one
        // So we need to special case this
        flags = ProxyMessageFlags::Enum::None;
    }
    
    /*
        If the proxy error code was set then the async op must be completed with the same error 
        If the proxy error code has not been set then the async op could have completed with just an RAP error
        Do not transfer stack trace/message in that case
    */
    ASSERT_IF(!thisPtr->proxyErrorCode_.IsSuccess() && thisPtr->proxyErrorCode_.ErrorCode.ReadValue() != thisPtr->Error.ReadValue(), "Error code does not match");
    if (thisPtr->proxyErrorCode_.IsSuccess())
    {
        thisPtr->proxyErrorCode_ = ProxyErrorCode::CreateRAPError(thisPtr->Error.ReadValue());
    }

    /*
        At this time the proxy error code is consistent with the error code
    */
    if (actionsList == ProxyActionsListTypes::StatefulServiceClose || 
        actionsList == ProxyActionsListTypes::StatefulServiceDrop ||
        actionsList == ProxyActionsListTypes::StatelessServiceClose)
    {
        // If close fails we close the fup no need to send error in the message
        thisPtr->proxyErrorCode_.ReadValue();
        thisPtr->proxyErrorCode_ = ProxyErrorCode::Success();
    }
    
    reply = move(ProxyReplyMessageBody(thisPtr->replyFuDesc_, 
                                       thisPtr->replyLocalReplica_, 
                                       move(thisPtr->replyRemoteReplicas_), 
                                       std::move(thisPtr->proxyErrorCode_),
                                       flags));

    return thisPtr->Error;
}

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::ProcessNextAction(AsyncOperationSPtr const & thisSPtr)
{
    vector<ProxyActions::Enum> const & actions = actionsList_.Actions; 

    for(;;)
    {
        if (currentStep_ == 0)
        {
            RAPEventSource::Events->ActionListStart(
                context_.FailoverUnitProxy->FailoverUnitId.Guid,
                context_.FailoverUnitProxy->RAPId,
                actionListName_);
        }

        if (currentStep_ > (actions.size() - 1))
        {
            // We completed the last action (successfully, since this was called again)
            RAPEventSource::Events->ActionListEnd(
                context_.FailoverUnitProxy->FailoverUnitId.Guid,
                context_.FailoverUnitProxy->RAPId,
                actionListName_);

            // Report success
            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete ActionListExecutorAsyncOperation");

            // Finished processing
            break;
        }

        RAPEventSource::Events->Action(
            context_.FailoverUnitProxy->FailoverUnitId.Guid,
            context_.FailoverUnitProxy->RAPId,
            actions[currentStep_], 
            actionListName_);

        ProxyActions::Enum action = actions[currentStep_++];

        ProxyErrorCode errorCode;
        bool completedSynchronously;

        switch(action)
        {
            case ProxyActions::OpenPartition:
                errorCode = DoOpenPartition(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::ClosePartition:
                errorCode = DoClosePartition(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::OpenInstance:
                errorCode = DoOpenInstance(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::CloseInstance:
                errorCode = DoCloseInstance(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::AbortInstance:
                errorCode = DoAbortInstance(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::OpenReplica:
                errorCode = DoOpenReplica(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::ChangeReplicaRole:
                errorCode = DoChangeReplicaRole(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::CloseReplica:
                errorCode = DoCloseReplica(thisSPtr, completedSynchronously);
                break;        
            case ProxyActions::OpenReplicator:
                errorCode = DoOpenReplicator(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::ChangeReplicatorRole:
                errorCode = DoChangeReplicatorRole(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::CloseReplicator:
                errorCode = DoCloseReplicator(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::AbortReplicator:
                errorCode = DoAbortReplicator(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::AbortReplica:
                errorCode = DoAbortReplica(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::GetReplicatorStatus:
                errorCode = DoReplicatorGetStatus(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::GetReplicatorQuery:
                errorCode = DoReplicatorGetQuery(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::ReplicatorPreWriteStatusRevokeUpdateConfiguration:
                errorCode = DoReplicatorUpdateConfiguration(UpdateConfigurationReason::PreWriteStatusRevokeCatchup, thisSPtr, completedSynchronously);
                break;
            case ProxyActions::ReplicatorUpdateConfiguration:
                errorCode = DoReplicatorUpdateConfiguration(UpdateConfigurationReason::Default, thisSPtr, completedSynchronously);
                break;
            case ProxyActions::ReplicatorUpdateCatchUpConfiguration:
                errorCode = DoReplicatorUpdateConfiguration(UpdateConfigurationReason::Catchup, thisSPtr, completedSynchronously);
                break;
            case ProxyActions::ReplicatorUpdateCurrentConfiguration:
                errorCode = DoReplicatorUpdateConfiguration(UpdateConfigurationReason::Current, thisSPtr, completedSynchronously);
                break;
            case ProxyActions::CatchupReplicaSetQuorum:
                errorCode = DoReplicatorCatchupReplicas(CatchupType::CatchupQuorum, thisSPtr, completedSynchronously);
                break;
            case ProxyActions::PreWriteStatusRevokeCatchup:
                errorCode = DoReplicatorCatchupReplicas(CatchupType::PreWriteStatusRevokeCatchup, thisSPtr, completedSynchronously);
                break;
            case ProxyActions::CatchupReplicaSetAll:
                errorCode = DoReplicatorCatchupReplicas(CatchupType::CatchupDuringSwap, thisSPtr, completedSynchronously);
                break;
            case ProxyActions::CancelCatchupReplicaSet:
                errorCode = DoReplicatorCancelCatchupReplicas(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::BuildIdleReplica:
                errorCode = DoReplicatorBuildIdleReplica(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::RemoveIdleReplica:
                errorCode = DoReplicatorRemoveIdleReplica(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::ReportAnyDataLoss:
                errorCode = DoReplicatorReportAnyDataLoss(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::UpdateEpoch:
                errorCode = DoReplicatorUpdateEpoch(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::ReconfigurationStarting:
                errorCode = DoReconfigurationStarting(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::ReconfigurationEnding:
                errorCode = DoReconfigurationEnding(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::UpdateReadWriteStatus:
                errorCode = DoUpdateReadWriteStatus(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::FinalizeDemoteToSecondary:
                errorCode = DoFinalizeDemoteToSecondary(thisSPtr, completedSynchronously);
                break;
            case ProxyActions::UpdateServiceDescription:
                errorCode = DoUpdateServiceDescription(thisSPtr, completedSynchronously);
                break;
            default:
                Assert::CodingError("Unknown or unimplemented ProxyActions value {0}", action);
        }

        if (!errorCode.IsSuccess())
        {
            bool completed = ProcessActionFailure(thisSPtr, std::move(errorCode));
            if (completed)
            {
                return;
            }
        }

        if (!completedSynchronously)
        {
            // Operation is still outstanding, it will resume action processing after it completes
            break;
        }
    }
}

bool ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::ProcessActionFailure(Common::AsyncOperationSPtr const & thisSPtr, ProxyErrorCode && failingErrorCode)
{
    ASSERT_IF(failingErrorCode.IsSuccess(), "ProcessActionFailure() called for a success error code.");

    vector<ProxyActions::Enum> const & actions = actionsList_.Actions; 

    RAPEventSource::Events->ActionFailure(
        context_.FailoverUnitProxy->FailoverUnitId.Guid,
        context_.FailoverUnitProxy->RAPId,
        actions[currentStep_ - 1], 
        wformatString(failingErrorCode),
        actionListName_);

    if (!continueOnFailure_)
    {
        /*
            NOTE: Set the proxy error code before completing the async op.

            If not, then the stack trace etc is captured properly in case of sync completion because TryComplete
            will invoke the callback which will do nothing (due to sync completion) and control flow will return here
            allowing the proxy error code to be set. Subsequently when the stack unwindes, end will be called and 
            proxyErrorCode_ will be read and stack trace etc will be returned.

            In async completion, the callback will call End prior to control returning here and 
            the proxy error code is not set
        */
        proxyErrorCode_ = std::move(failingErrorCode);

        bool didComplete = this->TryComplete(thisSPtr, failingErrorCode.ErrorCode);
        ASSERT_IFNOT(didComplete, "Failed to complete ActionListExecutorAsyncOperation");

        return true;
    }

    return false;
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoOpenInstance(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    AsyncOperationSPtr operation = context_.FailoverUnitProxy->BeginOpenInstance(
        owner_.ApplicationHostObj,
        [this] (AsyncOperationSPtr const & operation) { this->OpenInstanceCompletedCallback(operation); },
        thisSPtr);

    if (operation->CompletedSynchronously)
    {
        isCompletedSynchronously = true;

        errorCode = FinishOpenInstance(operation);
    }
    else
    {
        isCompletedSynchronously = false;
    }

    return errorCode;
}

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::OpenInstanceCompletedCallback(AsyncOperationSPtr const & openInstanceAsyncOperation)
{
    if (!openInstanceAsyncOperation->CompletedSynchronously)
    {
        AsyncOperationSPtr const & thisSPtr = openInstanceAsyncOperation->Parent;

        ProxyErrorCode errorCode = FinishOpenInstance(openInstanceAsyncOperation);

        // If succeeded, resume action processing
        if (errorCode.IsSuccess())
        {
            ProcessNextAction(thisSPtr);
        }
        else
        {
            // Failure during the inner operation, fail the outer operation
            if (!ProcessActionFailure(thisSPtr, std::move(errorCode)))
            {
                ProcessNextAction(thisSPtr);
            }
        }
    }
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::FinishOpenInstance(AsyncOperationSPtr const & openInstanceAsyncOperation)
{
    ProxyErrorCode errorCode;

    std::wstring newServiceLocation;
    errorCode = context_.FailoverUnitProxy->EndOpenInstance(openInstanceAsyncOperation, newServiceLocation);

    if (errorCode.IsSuccess())
    {
        // Update the reply with the new service location
        replyLocalReplica_.ServiceLocation = newServiceLocation;
    }

    return errorCode;
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoCloseInstance(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    AsyncOperationSPtr operation = context_.FailoverUnitProxy->BeginCloseInstance(
        [this] (AsyncOperationSPtr const & operation) { this->CloseInstanceCompletedCallback(operation); },
        thisSPtr);

    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    if (operation->CompletedSynchronously)
    {
        errorCode = FinishCloseInstance(operation);
        isCompletedSynchronously = true;
    }
    else
    {
        isCompletedSynchronously = false;
    }

    return errorCode;
}

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CloseInstanceCompletedCallback(AsyncOperationSPtr const & closeInstanceAsyncOperation)
{
    if (!closeInstanceAsyncOperation->CompletedSynchronously)
    {       
        AsyncOperationSPtr const & thisSPtr = closeInstanceAsyncOperation->Parent;

        ProxyErrorCode errorCode = FinishCloseInstance(closeInstanceAsyncOperation);

        // If succeeded, resume action processing
        if (errorCode.IsSuccess())
        {
            ProcessNextAction(thisSPtr);
        }
        else
        {
            // Failure during the inner operation, fail the outer operation
            if (!ProcessActionFailure(thisSPtr, std::move(errorCode)))
            {
                ProcessNextAction(thisSPtr);
            }
        }
    }
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::FinishCloseInstance(AsyncOperationSPtr const & closeInstanceAsyncOperation)
{
    ProxyErrorCode errorCode;

    errorCode = context_.FailoverUnitProxy->EndCloseInstance(closeInstanceAsyncOperation);
	
    return errorCode;
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoAbortInstance(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    UNREFERENCED_PARAMETER(thisSPtr);

    // Synchronous operation, so always completed synchronously
    isCompletedSynchronously = true;
    
    context_.FailoverUnitProxy->AbortInstance();

    return ProxyErrorCode::Success();
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoOpenReplica(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    AsyncOperationSPtr operation = context_.FailoverUnitProxy->BeginOpenReplica(
        owner_.ApplicationHostObj,
        [this] (AsyncOperationSPtr const & operation) { this->OpenReplicaCompletedCallback(operation); },
        thisSPtr);

    if (operation->CompletedSynchronously)
    {
        isCompletedSynchronously = true;

        errorCode = FinishOpenReplica(operation);
    }
    else
    {
        isCompletedSynchronously = false;
    }

    return errorCode;
}

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::OpenReplicaCompletedCallback(AsyncOperationSPtr const & openReplicaAsyncOperation)
{
    if (!openReplicaAsyncOperation->CompletedSynchronously)
    {
        AsyncOperationSPtr const & thisSPtr = openReplicaAsyncOperation->Parent;

        ProxyErrorCode errorCode = FinishOpenReplica(openReplicaAsyncOperation);

        // If succeeded, resume action processing
        if (errorCode.IsSuccess())
        {
            ProcessNextAction(thisSPtr);
        }
        else
        {
            // Failure during the inner operation, fail the outer operation
            if (!ProcessActionFailure(thisSPtr, std::move(errorCode)))
            {
                ProcessNextAction(thisSPtr);
            }
        }
    }
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::FinishOpenReplica(AsyncOperationSPtr const & openReplicaAsyncOperation)
{
    return context_.FailoverUnitProxy->EndOpenReplica(openReplicaAsyncOperation);
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoChangeReplicaRole(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    AsyncOperationSPtr operation = context_.FailoverUnitProxy->BeginChangeReplicaRole(
        [this] (AsyncOperationSPtr const & operation) { this->ChangeReplicaRoleCompletedCallback(operation); },
        thisSPtr);

    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    if (operation->CompletedSynchronously)
    {
        errorCode = FinishChangeReplicaRole(operation);
        isCompletedSynchronously = true;
    }
    else
    {
        isCompletedSynchronously = false;
    }

    return errorCode;
}


void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::ChangeReplicaRoleCompletedCallback(AsyncOperationSPtr const & changeRoleAsyncOperation)
{
    if (!changeRoleAsyncOperation->CompletedSynchronously)
    {
        AsyncOperationSPtr const & thisSPtr = changeRoleAsyncOperation->Parent;

        ProxyErrorCode errorCode = FinishChangeReplicaRole(changeRoleAsyncOperation);

        // If succeeded, resume action processing
        if (errorCode.IsSuccess())
        {
            ProcessNextAction(thisSPtr);
        }
        else
        {
            // Failure during the inner operation, fail the outer operation
            if (!ProcessActionFailure(thisSPtr, std::move(errorCode)))
            {
                ProcessNextAction(thisSPtr);
            }
        }
    }
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::FinishChangeReplicaRole(AsyncOperationSPtr const & changeRoleAsyncOperation)
{
    ProxyErrorCode errorCode;

    std::wstring newServiceLocation;

    errorCode = context_.FailoverUnitProxy->EndChangeReplicaRole(changeRoleAsyncOperation, newServiceLocation);

    // Process next action if succeeded, else fail the set
    if (errorCode.IsSuccess())
    {
        replyLocalReplica_.ServiceLocation = newServiceLocation;
    }

    return errorCode;
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoCloseReplica(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    AsyncOperationSPtr operation = context_.FailoverUnitProxy->BeginCloseReplica(
        [this] (AsyncOperationSPtr const & operation) { this->CloseReplicaCompletedCallback(operation); },
        thisSPtr);

    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    if (operation->CompletedSynchronously)
    {
        errorCode = FinishCloseReplica(operation);
        isCompletedSynchronously = true;
    }
    else
    {
        isCompletedSynchronously = false;
    }

    return errorCode;
}

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CloseReplicaCompletedCallback(AsyncOperationSPtr const & closeReplicaAsyncOperation)
{
    if (!closeReplicaAsyncOperation->CompletedSynchronously)
    {       
        AsyncOperationSPtr const & thisSPtr = closeReplicaAsyncOperation->Parent;

        ProxyErrorCode errorCode = FinishCloseReplica(closeReplicaAsyncOperation);

        // If succeeded, resume action processing
        if (errorCode.IsSuccess())
        {
            ProcessNextAction(thisSPtr);
        }
        else
        {
            // Failure during the inner operation, fail the outer operation
            if (!ProcessActionFailure(thisSPtr, std::move(errorCode)))
            {
                ProcessNextAction(thisSPtr);
            }
        }
    }
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::FinishCloseReplica(AsyncOperationSPtr const & closeReplicaAsyncOperation)
{
    return context_.FailoverUnitProxy->EndCloseReplica(closeReplicaAsyncOperation);
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoAbortReplica(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    UNREFERENCED_PARAMETER(thisSPtr);

    // Synchronous operation, so always completed synchronously
    isCompletedSynchronously = true;
    
    context_.FailoverUnitProxy->AbortReplica();

    return ProxyErrorCode::Success();
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoOpenReplicator(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    AsyncOperationSPtr operation = context_.FailoverUnitProxy->BeginOpenReplicator(
        [this] (AsyncOperationSPtr const & operation) { this->OpenReplicatorCompletedCallback(operation); },
        thisSPtr);
 
    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    if (operation->CompletedSynchronously)
    {
        errorCode = FinishOpenReplicator(operation);

        isCompletedSynchronously = true;
    }
    else
    {
        isCompletedSynchronously = false;
    }

    return errorCode;
}

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::OpenReplicatorCompletedCallback(AsyncOperationSPtr const & openReplicatorAsyncOperation)
{
    if (!openReplicatorAsyncOperation->CompletedSynchronously)
    {
        AsyncOperationSPtr const & thisSPtr = openReplicatorAsyncOperation->Parent;
 
        ProxyErrorCode errorCode = FinishOpenReplicator(openReplicatorAsyncOperation);
        
        if (errorCode.IsSuccess())
        {
            ProcessNextAction(thisSPtr);
        }
        else
        {
            // Failure during the inner operation, fail the outer operation
            if (!ProcessActionFailure(thisSPtr, std::move(errorCode)))
            {
                ProcessNextAction(thisSPtr);
            }
        }
    }
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::FinishOpenReplicator(AsyncOperationSPtr const & openReplicatorAsyncOperation)
{
    ProxyErrorCode errorCode;

    std::wstring replicationEndpoint;
    errorCode = context_.FailoverUnitProxy->EndOpenReplicator(openReplicatorAsyncOperation, replicationEndpoint);

    if (errorCode.IsSuccess())
    {
        // Update the reply with the replication endpoint
        replyLocalReplica_.ReplicationEndpoint = replicationEndpoint;
    }

    return errorCode;
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoChangeReplicatorRole(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    AsyncOperationSPtr operation = context_.FailoverUnitProxy->BeginChangeReplicatorRole(
        [this] (AsyncOperationSPtr const & operation) { this->ChangeReplicatorRoleCompletedCallback(operation); },
        thisSPtr);

    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    if (operation->CompletedSynchronously)
    {
        errorCode = FinishChangeReplicatorRole(operation);
        isCompletedSynchronously = true;
    }
    else
    {
        isCompletedSynchronously = false;
    }

    return errorCode;
}

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::ChangeReplicatorRoleCompletedCallback(AsyncOperationSPtr const & changeRoleAsyncOperation)
{
    if (!changeRoleAsyncOperation->CompletedSynchronously)
    {
        AsyncOperationSPtr const & thisSPtr = changeRoleAsyncOperation->Parent;

        ProxyErrorCode errorCode = FinishChangeReplicatorRole(changeRoleAsyncOperation);

        // If succeeded, resume action processing
        if (errorCode.IsSuccess())
        {
            ProcessNextAction(thisSPtr);
        }
        else
        {
            // Failure during the inner operation, fail the outer operation
            if (!ProcessActionFailure(thisSPtr, std::move(errorCode)))
            {
                ProcessNextAction(thisSPtr);
            }
        }
    }
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::FinishChangeReplicatorRole(AsyncOperationSPtr const & changeRoleAsyncOperation)
{
    return context_.FailoverUnitProxy->EndChangeReplicatorRole(changeRoleAsyncOperation);
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoAbortReplicator(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    UNREFERENCED_PARAMETER(thisSPtr);

    // Synchronous operation, so always completed synchronously
    isCompletedSynchronously = true;
    
    context_.FailoverUnitProxy->AbortReplicator();

    return ProxyErrorCode::Success();
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoCloseReplicator(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    AsyncOperationSPtr operation = context_.FailoverUnitProxy->BeginCloseReplicator(
        [this] (AsyncOperationSPtr const & operation) { this->CloseReplicatorCompletedCallback(operation); },
        thisSPtr);

    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    if (operation->CompletedSynchronously)
    {
        errorCode = FinishCloseReplicator(operation);
        isCompletedSynchronously = true;
    }
    else
    {
        isCompletedSynchronously = false;
    }

    return errorCode;
}

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CloseReplicatorCompletedCallback(AsyncOperationSPtr const & closeReplicatorAsyncOperation)
{
    if (!closeReplicatorAsyncOperation->CompletedSynchronously)
    { 
        AsyncOperationSPtr const & thisSPtr = closeReplicatorAsyncOperation->Parent;
    
        ProxyErrorCode errorCode = FinishCloseReplicator(closeReplicatorAsyncOperation);

        // If succeeded, resume action processing
        if (errorCode.IsSuccess())
        {
            ProcessNextAction(thisSPtr);
        }
        else
        {
            // Failure during the inner operation, fail the outer operation
            if (!ProcessActionFailure(thisSPtr, std::move(errorCode)))
            {
                ProcessNextAction(thisSPtr);
            }
        }
    }
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::FinishCloseReplicator(AsyncOperationSPtr const & closeReplicatorAsyncOperation)
{
    return context_.FailoverUnitProxy->EndCloseReplicator(closeReplicatorAsyncOperation);
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoReplicatorGetStatus(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    UNREFERENCED_PARAMETER(thisSPtr);

    // Synchronous operation, so always completed synchronously
    isCompletedSynchronously = true;

    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    // Get the replicator status

    FABRIC_SEQUENCE_NUMBER firstLsn, lastLsn;

    errorCode = context_.FailoverUnitProxy->ReplicatorGetStatus(firstLsn, lastLsn);

    if (errorCode.IsSuccess())
    {
        replyLocalReplica_.FirstAcknowledgedLSN = firstLsn;
        replyLocalReplica_.LastAcknowledgedLSN = lastLsn;
    }

    return errorCode;
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoReplicatorGetStatusIfDataLoss(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    UNREFERENCED_PARAMETER(thisSPtr);

    // Synchronous operation, so always completed synchronously
    isCompletedSynchronously = true;

    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    if (context_.Body.LocalReplicaDescription.CurrentConfigurationRole == ReplicaRole::Primary &&
        context_.Body.FailoverUnitDescription.PreviousConfigurationEpoch != Epoch::InvalidEpoch() &&
        context_.Body.FailoverUnitDescription.IsDataLossBetweenPCAndCC)
    {
        return DoReplicatorGetStatus(thisSPtr, isCompletedSynchronously);
    }

    return ProxyErrorCode::Success();
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoReplicatorGetQuery(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    UNREFERENCED_PARAMETER(thisSPtr);
    UNREFERENCED_PARAMETER(isCompletedSynchronously);
    Assert::CodingError("DoReplicatorGetQuery should be handled in sub class");
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoReplicatorUpdateConfiguration(UpdateConfigurationReason::Enum reason, AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    UNREFERENCED_PARAMETER(thisSPtr);
    isCompletedSynchronously = true;
    return context_.FailoverUnitProxy->UpdateConfiguration(context_.Body, reason);
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoReplicatorCatchupReplicas(CatchupType::Enum catchupType, AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    AsyncOperationSPtr operation;
    
    operation = context_.FailoverUnitProxy->BeginReplicatorCatchupReplicaSet(
        catchupType,
        [this](AsyncOperationSPtr const & operation) { this->ReplicatorCatchupReplicasCompletedCallback(operation); },
        thisSPtr);

    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    if (operation->CompletedSynchronously)
    {
        errorCode = FinishReplicatorCatchupReplicas(operation);
        isCompletedSynchronously = true;
    }
    else
    {
        isCompletedSynchronously = false;
    }

    return errorCode;
}

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::ReplicatorCatchupReplicasCompletedCallback(AsyncOperationSPtr const & catchupReplicasAsyncOperation)
{
    if (!catchupReplicasAsyncOperation->CompletedSynchronously)
    {
        AsyncOperationSPtr const & thisSPtr = catchupReplicasAsyncOperation->Parent;

        ProxyErrorCode errorCode = FinishReplicatorCatchupReplicas(catchupReplicasAsyncOperation);

        // If succeeded, resume action processing
        if (errorCode.IsSuccess())
        {
            ProcessNextAction(thisSPtr);
        }
        else
        {
            // Failure during the inner operation, fail the outer operation
            if (!ProcessActionFailure(thisSPtr, std::move(errorCode)))
            {
                ProcessNextAction(thisSPtr);
            }
        }
    }
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::FinishReplicatorCatchupReplicas(AsyncOperationSPtr const & catchupReplicasAsyncOperation)
{
    return context_.FailoverUnitProxy->EndReplicatorCatchupReplicaSet(catchupReplicasAsyncOperation);
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoReplicatorCancelCatchupReplicas(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    UNREFERENCED_PARAMETER(thisSPtr);

    // Synchronous operation, so always completed synchronously
    isCompletedSynchronously = true;

    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    return context_.FailoverUnitProxy->CancelReplicatorCatchupReplicaSet();
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoReplicatorBuildIdleReplica(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    AsyncOperationSPtr operation = context_.FailoverUnitProxy->BeginReplicatorBuildIdleReplica(
        context_.Body.RemoteReplicaDescription,
        [this] (AsyncOperationSPtr const & operation) { this->ReplicatorBuildIdleReplicaCompletedCallback(operation); },
        thisSPtr);

    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    if (operation->CompletedSynchronously)
    {
        errorCode = FinishReplicatorBuildIdleReplica(operation);
        isCompletedSynchronously = true;
    }
    else
    {
        isCompletedSynchronously = false;
    }

    return errorCode;
}

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::ReplicatorBuildIdleReplicaCompletedCallback(AsyncOperationSPtr const & buildIdleReplicaAsyncOperation)
{
    if (!buildIdleReplicaAsyncOperation->CompletedSynchronously)
    {
        AsyncOperationSPtr const & thisSPtr = buildIdleReplicaAsyncOperation->Parent;

        ProxyErrorCode errorCode = FinishReplicatorBuildIdleReplica(buildIdleReplicaAsyncOperation);

        // If succeeded, resume action processing
        if (errorCode.IsSuccess())
        {
            ProcessNextAction(thisSPtr);
        }
        else
        {
            // Failure during the inner operation, fail the outer operation
            if (!ProcessActionFailure(thisSPtr, std::move(errorCode)))
            {
                ProcessNextAction(thisSPtr);
            }
        }
    }
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::FinishReplicatorBuildIdleReplica(AsyncOperationSPtr const & buildIdleReplicaAsyncOperation)
{
    return context_.FailoverUnitProxy->EndReplicatorBuildIdleReplica(buildIdleReplicaAsyncOperation);
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoReplicatorRemoveIdleReplica(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    UNREFERENCED_PARAMETER(thisSPtr);

    // Synchronous operation, so always completed synchronously
    isCompletedSynchronously = true;

    ProxyErrorCode errorCode = ProxyErrorCode::Success();
    
    errorCode = context_.FailoverUnitProxy->ReplicatorRemoveIdleReplica(context_.Body.RemoteReplicaDescription);

    return errorCode;
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoReplicatorReportAnyDataLoss(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    AsyncOperationSPtr operation = context_.FailoverUnitProxy->BeginReplicatorOnDataLoss(
        [this](AsyncOperationSPtr const & inner) { this->ReplicatorOnDataLossCompletedCallback(inner); },
        thisSPtr);

    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    if (operation->CompletedSynchronously)
    {
        errorCode = FinishReplicatorOnDataLoss(operation);
        isCompletedSynchronously = true;
    }
    else
    {
        isCompletedSynchronously = false;
    }

    return errorCode;
}

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::ReplicatorOnDataLossCompletedCallback(AsyncOperationSPtr const & onDataLossAsyncOperation)
{
    if (!onDataLossAsyncOperation->CompletedSynchronously)
    {
        AsyncOperationSPtr const & thisSPtr = onDataLossAsyncOperation->Parent;

        ProxyErrorCode errorCode = FinishReplicatorOnDataLoss(onDataLossAsyncOperation);

        // If succeeded, resume action processing
        if (errorCode.IsSuccess())
        {
            ProcessNextAction(thisSPtr);
        }
        else
        {
            // Failure during the inner operation, fail the outer operation
            if (!ProcessActionFailure(thisSPtr, std::move(errorCode)))
            {
                ProcessNextAction(thisSPtr);
            }
        }
    }
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::FinishReplicatorOnDataLoss(AsyncOperationSPtr const & onDataLossAsyncOperation)
{
    ProxyErrorCode errorCode;

    int64 lastLSN = 0;
    errorCode = context_.FailoverUnitProxy->EndReplicatorOnDataLoss(onDataLossAsyncOperation, lastLSN);

    if (errorCode.IsError(ErrorCodeValue::RAProxyStateChangedOnDataLoss))
    {
        ASSERT_IF(lastLSN == FABRIC_INVALID_SEQUENCE_NUMBER, "Sequence number must be set in on data loss {0}", context_.FailoverUnitProxy);
        replyLocalReplica_.LastAcknowledgedLSN = lastLSN;
    }

    return errorCode;
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoReplicatorUpdateEpoch(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{   
    ASSERT_IFNOT(context_.FailoverUnitProxy, "Failover unit is invalid");

    AsyncOperationSPtr operation = context_.FailoverUnitProxy->BeginReplicatorUpdateEpoch(
        [this] (AsyncOperationSPtr const & operation) { this->ReplicatorUpdateEpochCompletedCallback(operation); },
        thisSPtr);

    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    if (operation->CompletedSynchronously)
    {
        errorCode = FinishReplicatorUpdateEpoch(operation);
        isCompletedSynchronously = true;
    }
    else
    {
        isCompletedSynchronously = false;
    }

    return errorCode;
}

void ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::ReplicatorUpdateEpochCompletedCallback(AsyncOperationSPtr const & replicatorUpdateEpochAsyncOperation)
{
    if (!replicatorUpdateEpochAsyncOperation->CompletedSynchronously)
    {
        AsyncOperationSPtr const & thisSPtr = replicatorUpdateEpochAsyncOperation->Parent;

        ProxyErrorCode errorCode = FinishReplicatorUpdateEpoch(replicatorUpdateEpochAsyncOperation);

        // If succeeded, resume action processing
        if (errorCode.IsSuccess())
        {
            ProcessNextAction(thisSPtr);
        }
        else
        {
            // Failure during the inner operation, fail the outer operation
            if (!ProcessActionFailure(thisSPtr, std::move(errorCode)))
            {
                ProcessNextAction(thisSPtr);
            }
        }
    }
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::FinishReplicatorUpdateEpoch(AsyncOperationSPtr const & replicatorUpdateEpochAsyncOperation)
{
    return context_.FailoverUnitProxy->EndReplicatorUpdateEpoch(replicatorUpdateEpochAsyncOperation);
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoReconfigurationStarting(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    UNREFERENCED_PARAMETER(thisSPtr);

    // Synchronous operation, so always completed synchronously
    isCompletedSynchronously = true;

    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    context_.FailoverUnitProxy->OnReconfigurationStarting();

    return errorCode;
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoReconfigurationEnding(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    UNREFERENCED_PARAMETER(thisSPtr);

    // Synchronous operation, so always completed synchronously
    isCompletedSynchronously = true;

    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    ProxyErrorCode errorCode = ProxyErrorCode::Success();

    context_.FailoverUnitProxy->OnReconfigurationEnding();
    
    return errorCode;
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoUpdateReadWriteStatus(
    Common::AsyncOperationSPtr const & , 
    __out bool & isCompletedSynchronously)
{
    isCompletedSynchronously = true;

    context_.FailoverUnitProxy->UpdateReadWriteStatus();

    return ProxyErrorCode::Success();
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoFinalizeDemoteToSecondary(
    Common::AsyncOperationSPtr const &,
    __out bool & isCompletedSynchronously)
{
    isCompletedSynchronously = true;

    return context_.FailoverUnitProxy->FinalizeDemoteToSecondary();
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoUpdateServiceDescription(Common::AsyncOperationSPtr const &, __out bool & isCompletedSynchronously)
{
    isCompletedSynchronously = true;

    context_.FailoverUnitProxy->UpdateServiceDescription(context_.Body.ServiceDescription);

    return ProxyErrorCode::Success();
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoOpenPartition(Common::AsyncOperationSPtr const &, __out bool & isCompletedSynchronously)
{
    isCompletedSynchronously = true;

    context_.FailoverUnitProxy->OpenPartition();

    return ProxyErrorCode::Success();
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::DoClosePartition(Common::AsyncOperationSPtr const &, __out bool & isCompletedSynchronously)
{
    isCompletedSynchronously = true;

    // FOr the partition object both close and abort are the same - it simply releases the com reference
    context_.FailoverUnitProxy->AbortPartition();

    return ProxyErrorCode::Success();
}

