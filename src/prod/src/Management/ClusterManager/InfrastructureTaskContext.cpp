// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("InfrastructureTaskContext");

RolloutContextType::Enum const InfrastructureTaskContextType(RolloutContextType::InfrastructureTask);

InfrastructureTaskContext::InfrastructureTaskContext()
    : RolloutContext(InfrastructureTaskContextType)
    , description_()
    , state_(InfrastructureTaskState::Invalid)
    , isInterrupted_(false)
    , healthCheckElapsedTime_(TimeSpan::Zero)
{
}

InfrastructureTaskContext::InfrastructureTaskContext(InfrastructureTaskContext && other)
    : RolloutContext(move(other))
    , description_(move(other.description_))
    , state_(move(other.state_))
    , isInterrupted_(false)
    , healthCheckElapsedTime_(move(other.healthCheckElapsedTime_))
{
}

InfrastructureTaskContext & InfrastructureTaskContext::operator =(InfrastructureTaskContext && other)
{
    if (this != &other)
    {
        __super::operator = (move(other));
        description_ = move(other.description_);
        state_ = move(other.state_);
        isInterrupted_ = false;
        healthCheckElapsedTime_ = move(other.healthCheckElapsedTime_);
    }

    return *this;
}

InfrastructureTaskContext::InfrastructureTaskContext(TaskInstance const & task)
    : RolloutContext(InfrastructureTaskContextType)
    , description_(InfrastructureTaskDescription(task.Id, task.Instance))
    , state_(InfrastructureTaskState::Invalid)
    , isInterrupted_(false)
    , healthCheckElapsedTime_(TimeSpan::Zero)
{
}

InfrastructureTaskContext::InfrastructureTaskContext(
    ComponentRoot const & replica,
    ClientRequestSPtr const & request,
    InfrastructureTaskDescription const & description)
    : RolloutContext(InfrastructureTaskContextType, replica, request)
    , description_(description)
    , state_(InfrastructureTaskState::PreProcessing)
    , isInterrupted_(false)
    , healthCheckElapsedTime_(TimeSpan::Zero)
{
    WriteNoise(
        TraceComponent, 
        "{0} ctor({1})",
        this->TraceId,
        description_);
}

InfrastructureTaskContext::~InfrastructureTaskContext()
{
    // Will be null for contexts instantiated for reading from disk.
    // Do not trace dtor for these contexts as it is noisy and not useful.
    //
    if (replicaSPtr_)
    {
        WriteNoise(
            TraceComponent,
            "{0} ~dtor",
            this->TraceId);
    }
}

ErrorCode InfrastructureTaskContext::TryAcceptStartTask(
    InfrastructureTaskDescription const & incomingDescription, 
    __out bool & taskComplete,
    __out bool & shouldCommit)
{
    taskComplete = false;

    if (this->IsFailed)
    {
        // Previous task failed due to an unexpected error. We will
        // only fail processing after successfully notifying IS of 
        // the failure, so allow accepting new tasks. Safety checks 
        // will still occur and block as appropriate during pre-processing.
        //
        shouldCommit = true;
        isInterrupted_ = false;
        this->UpdateDescription(incomingDescription);
        state_ = InfrastructureTaskState::PreProcessing;
        return ErrorCodeValue::Success;
    }

    if (incomingDescription.InstanceId < description_.InstanceId)
    {
        shouldCommit = false;
        return ErrorCodeValue::StaleInfrastructureTask;
    }
    else if (incomingDescription.InstanceId == description_.InstanceId)
    {
        if (!description_.AreNodeTasksEqual(incomingDescription))
        {
            shouldCommit = false;
            return ErrorCodeValue::StaleInfrastructureTask;
        }

        // Could be a local retry. Don't commit but make sure the context
        // is enqueued for processing.
        //
        shouldCommit = false;

        switch (state_)
        {
        case InfrastructureTaskState::PreProcessing:
            taskComplete = false;
            break;

        case InfrastructureTaskState::PreAcked:
            // Re-acknowledge a start request with the same instance ID
            description_.UpdateDoAsyncAck(incomingDescription.DoAsyncAck);
            state_ = InfrastructureTaskState::PreAckPending;
            shouldCommit = true;
            taskComplete = true;
            break;

        // The start task is complete if it has moved onto post processing
        //
        default:
            taskComplete = true;
            break;
        }

        return ErrorCodeValue::Success;
    }
    else
    {
        switch (state_)
        {
        case InfrastructureTaskState::PreProcessing:
        case InfrastructureTaskState::PreAckPending:
            if (description_.AreNodeTasksEqual(incomingDescription))
            {
                // Just update the instance that gets acked
                description_.UpdateTaskInstance(incomingDescription.InstanceId);
                description_.UpdateDoAsyncAck(incomingDescription.DoAsyncAck);
            }
            else
            {
                // Interrupt current task to reverse PreProcessing work
                isInterrupted_ = true;
                state_ = InfrastructureTaskState::PostProcessing;
            }

            break;

        case InfrastructureTaskState::PreAcked:
            if (description_.AreNodeTasksEqual(incomingDescription))
            {
                // Update instance and re-ack
                description_.UpdateTaskInstance(incomingDescription.InstanceId);
                description_.UpdateDoAsyncAck(incomingDescription.DoAsyncAck);
                state_ = InfrastructureTaskState::PreAckPending;
            }
            else
            {
                // Reverse PreProcessing work
                isInterrupted_ = true;
                state_ = InfrastructureTaskState::PostProcessing;
            }

            break;

        case InfrastructureTaskState::PostProcessing:
            // Need to let current task finish reversing PreProcessing work
            shouldCommit = false;
            return ErrorCodeValue::CMRequestAlreadyProcessing;

        case InfrastructureTaskState::PostAckPending:
        case InfrastructureTaskState::PostAcked:
        default:
            // Accept and start new task
            isInterrupted_ = false;
            this->UpdateDescription(incomingDescription);
            state_ = InfrastructureTaskState::PreProcessing;
            break;
        };

        shouldCommit = true;
        return ErrorCodeValue::Success;
    }
}

ErrorCode InfrastructureTaskContext::TryAcceptFinishTask(
    TaskInstance const & taskInstance, 
    __out bool & taskComplete,
    __out bool & shouldCommit)
{
    taskComplete = false;

    if (this->IsFailed)
    {
        shouldCommit = false;
        return ErrorCodeValue::OperationFailed;
    }

    if (taskInstance.Instance < description_.InstanceId)
    {
        return ErrorCodeValue::StaleInfrastructureTask;
    }
    else
    {
        switch (state_)
        {
        case InfrastructureTaskState::PreProcessing:
        case InfrastructureTaskState::PreAckPending:
            //
            // Intentional fall-through:
            // Infrastructure went ahead without waiting.
            // Update instance and start PostProcessing work.
            //
        case InfrastructureTaskState::PreAcked:
            // Accept finish task request
            shouldCommit = true;
            description_.UpdateTaskInstance(taskInstance.Instance);
            state_ = InfrastructureTaskState::PostProcessing;
            return ErrorCodeValue::Success;

        case InfrastructureTaskState::PostProcessing:
            shouldCommit = false;

            if (taskInstance.Instance == description_.InstanceId)
            {
                return ErrorCodeValue::Success;
            }
            else
            {
                // Need to let current task finish reversing PreProcessing work
                return ErrorCodeValue::CMRequestAlreadyProcessing;
            }

        case InfrastructureTaskState::PostAckPending:
        case InfrastructureTaskState::PostAcked:
        default:
            // Greater instance also considered complete since we
            // never did any pre processing work
            taskComplete = true;

            // Update instance and re-ack
            shouldCommit = true;
            description_.UpdateTaskInstance(taskInstance.Instance);
            state_ = InfrastructureTaskState::PostAckPending;
            return ErrorCodeValue::Success;
        };
    }
}

void InfrastructureTaskContext::UpdateState(InfrastructureTaskState::Enum state)
{
    state_ = state;
}

void InfrastructureTaskContext::UpdateHealthCheckElapsedTime(Common::TimeSpan const elapsed)
{
    healthCheckElapsedTime_ = healthCheckElapsedTime_.AddWithMaxAndMinValueCheck(elapsed);
}

void InfrastructureTaskContext::SetHealthCheckElapsedTime(TimeSpan const value)
{
    healthCheckElapsedTime_ = value;
}

void InfrastructureTaskContext::SetLastHealthCheckResult(bool isHealthy)
{
    lastHealthCheckResult_ = isHealthy;
}

void InfrastructureTaskContext::UpdateDescription(InfrastructureTaskDescription const & incomingDescription)
{
    description_ = incomingDescription;
}

std::wstring const & InfrastructureTaskContext::get_Type() const
{
    return Constants::StoreType_InfrastructureTaskContext;
}

std::wstring InfrastructureTaskContext::ConstructKey() const
{
    return description_.TaskId;
}

void InfrastructureTaskContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("InfrastructureTaskContext({0})[{1}, {2}]", this->Status, state_, description_);
}
