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

void FailoverUnitProxy::ChangeReplicaRoleAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    FABRIC_REPLICA_ROLE newRole;

    {
        AcquireExclusiveLock grab(owner_.lock_);
        
        TraceBeforeStart(grab);

        if (owner_.replicaState_ == FailoverUnitProxyStates::Closed)
        {
            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete ChangeReplicaRoleAsyncOperation");

            return;
        }

        ASSERT_IFNOT(owner_.IsStatefulServiceFailoverUnitProxy(), "Attempt to change role on an FUP that is not hosting a stateful service");
        
        // Already in the role?
        if(owner_.currentServiceRole_ == owner_.replicaDescription_.CurrentConfigurationRole)
        {
            // Already in the role, so just report success

            // Service location is unchanged
            serviceLocation_ = owner_.replicaDescription_.ServiceLocation;

            bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            ASSERT_IFNOT(didComplete, "Failed to complete ChangeReplicaRoleAsyncOperation");

            return;
        }

        ASSERT_IFNOT(owner_.replicaState_ == FailoverUnitProxyStates::Opened, "Attempt to change role on closed service.");

        ASSERT_IF(owner_.serviceOperationManager_ == nullptr, "Service operation manager expected but not available.");
        
        if (!TryStartUserApi(grab, wformatString(owner_.replicaDescription_.CurrentConfigurationRole), thisSPtr))
        {
            return;
        }

        previousRole_ = owner_.currentServiceRole_;
        newRole = ReplicaRole::ConvertToPublicReplicaRole(owner_.replicaDescription_.CurrentConfigurationRole);
    }

    // Start changing the service role
    AsyncOperationSPtr operation = owner_.statefulService_->BeginChangeRole(
        newRole,
        [this] (AsyncOperationSPtr const & operation) { this->ChangeRoleCompletedCallback(operation); },
        thisSPtr);

    TryContinueUserApi(operation);

    if (operation->CompletedSynchronously)
    {
        FinishChangeRole(operation);
    }
}

ProxyErrorCode FailoverUnitProxy::ChangeReplicaRoleAsyncOperation::End(AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation)
{
    auto thisPtr = AsyncOperation::End<FailoverUnitProxy::ChangeReplicaRoleAsyncOperation>(asyncOperation);
    serviceLocation = thisPtr->serviceLocation_;
    return thisPtr->EndHelper();
}

void FailoverUnitProxy::ChangeReplicaRoleAsyncOperation::ChangeRoleCompletedCallback(AsyncOperationSPtr const & changeRoleAsyncOperation)
{
    if (!changeRoleAsyncOperation->CompletedSynchronously)
    {
        FinishChangeRole(changeRoleAsyncOperation);
    }
}

void FailoverUnitProxy::ChangeReplicaRoleAsyncOperation::FinishChangeRole(AsyncOperationSPtr const & changeRoleAsyncOperation)
{   
    ErrorCode errorCode = owner_.statefulService_->EndChangeRole(changeRoleAsyncOperation, serviceLocation_);

    Transport::MessageUPtr outMsg = nullptr;
    
    {        
        AcquireExclusiveLock grab(owner_.lock_);

        owner_.reportedLoadStore_.OnFTChangeRole();

        if(errorCode.IsSuccess())
        {
            owner_.replicaDescription_.ServiceLocation = serviceLocation_;
            owner_.currentServiceRole_ = owner_.replicaDescription_.CurrentConfigurationRole;

            if (owner_.currentServiceRole_ == ReplicaRole::Primary)
            {
                /*
                    Keep the primary IB right now
                    After OnDataLoss completes it will transition to RD
                    This is so that Build waits until OnDataLoss completes
                */
                owner_.currentReplicaState_ = ReplicaStates::InBuild;
            }

            RAPEventSource::Events->FailoverUnitProxyEndpointUpdate(
                owner_.FailoverUnitId.Guid,
                owner_.RAPId,
                interface_,
                serviceLocation_);

            // If FUP is opening then it is the initial change role from SF Open 
            // This does not require an additional replica endpoint updated message
            if (owner_.currentServiceRole_ == ReplicaRole::Primary && owner_.State != FailoverUnitProxyStates::Opening)
            {
                // Note that the message stage needs to be set under the lock                
                owner_.messageStage_ = ProxyMessageStage::RAProxyEndpointUpdated;
                
                // Create the endpoint message to send to RA                
                ReplicaMessageBody msgBody(owner_.failoverUnitDescription_, owner_.replicaDescription_, owner_.serviceDescription_);

                outMsg = RAMessage::GetReplicaEndpointUpdated().CreateMessage<ReplicaMessageBody>(msgBody, msgBody.FailoverUnitDescription.FailoverUnitId.Guid);                
            }
        }
        else
        {
            owner_.currentServiceRole_ = previousRole_;
        }

        FinishUserApi(grab, errorCode);
    }

    if (outMsg)
    {
        owner_.PublishEndpoint(move(outMsg));
    }

    AsyncOperationSPtr const & thisSPtr = changeRoleAsyncOperation->Parent;
    bool didComplete = thisSPtr->TryComplete(thisSPtr, errorCode);
    ASSERT_IFNOT(didComplete, "Failed to complete ChangeReplicaRoleAsyncOperation");
}
