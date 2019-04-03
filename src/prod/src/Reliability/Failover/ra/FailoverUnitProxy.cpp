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
using namespace ServiceModel;
using namespace Common::ApiMonitoring;

StringLiteral const RejectDueToAbortTag("Abort");
StringLiteral const RejectDueToIncompatibleTag("Incompatible");

StringLiteral const ReplicaTag("Replica");
StringLiteral const InstanceTag("Instance");
StringLiteral const ReplicatorTag("Replicator");

Global<FailoverUnitProxy::ReadWriteStatusCalculator> const FailoverUnitProxy::ReadWriteStatusCalculatorObj = make_global<FailoverUnitProxy::ReadWriteStatusCalculator>();

class FailoverUnitProxy::WaitForDrainAsyncOperation : public AsyncOperation
{
public:
    WaitForDrainAsyncOperation(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & state) 
        :   AsyncOperation(callback, state)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operationSPtr)
    {
        return AsyncOperation::End<WaitForDrainAsyncOperation>(operationSPtr)->Error;
    }
protected:
    virtual void OnStart(AsyncOperationSPtr const &)
    {
    }
};

void ActionListInfo::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w << Name;
}

FailoverUnitProxy::~FailoverUnitProxy()
{
    RAPEventSource::Events->FailoverUnitProxyDestructor(
        FailoverUnitId.Guid,
        RAPId,
        replicaDescription_.ReplicaId,
        replicaDescription_.InstanceId);
}

void FailoverUnitProxy::AcquireLock()
{
    lock_.AcquireExclusive();
}

void FailoverUnitProxy::ReleaseLock()
{
    lock_.ReleaseExclusive();
}

void FailoverUnitProxy::Cleanup()
{
    ASSERT_IFNOT(state_ == FailoverUnitProxyStates::Closed && replicaState_ == FailoverUnitProxyStates::Closed && replicatorState_ == ReplicatorStates::Closed,
        "FailoverUnitProxy::Cleanup invalid fup state");

    ASSERT_IF(statelessService_ || statefulService_ || statefulServicePartition_ || statelessServicePartition_ || replicator_, 
        "FailoverUnitProxy::Cleanup, still holding references"); 

    remoteReplicas_.clear();
    configurationReplicas_.clear();
}

void FailoverUnitProxy::Reuse(
    Reliability::FailoverUnitDescription const & failoverUnitDescription,
    std::wstring const & runtimeId)
{
    ASSERT_IFNOT(currentlyExecutingActionsLists_.size() == 0, "FailoverUnitProxy::Reuse There are still actions");

    failoverUnitDescription_ = failoverUnitDescription;
    runtimeId_ = runtimeId;

    serviceOperationManager_ = nullptr;
    replicatorOperationManager_ = nullptr;
    isOpenForBusiness_ = true;
    isMarkedForClose_ = false;
    isMarkedForAbort_ = false;
    messageStage_ = ProxyMessageStage::None;
    replicatorState_  = ReplicatorStates::Closed;
    replicaState_ = FailoverUnitProxyStates::Closed;
    currentServiceRole_ = ReplicaRole::Unknown;
    currentReplicatorRole_ = ReplicaRole::Unknown;
    currentReplicaState_ = ReplicaStates::InCreate;
    lastUpdateEpochPrimaryEpochValue_ = Epoch::InvalidEpoch();
    replicaOpenMode_ = ReplicaOpenMode::Invalid;
    configurationStage_ = ProxyConfigurationStage::Current;
    catchupResult_ = CatchupResult::NotStarted;
    isCatchupCancel_ = false;
    isServiceAvailabilityImpacted_ = false;
    readWriteStatusState_ = ReadWriteStatusState();
}

void FailoverUnitProxy::AddActionList(ProxyActionsListTypes::Enum const & plannedActions, bool impactsServiceAvailability)
{
    ActionListInfo info;
    info.Name = plannedActions;
    info.ImpactsServiceAvailability = impactsServiceAvailability;

    currentlyExecutingActionsLists_.push_back(info);

    if (impactsServiceAvailability)
    {
        isServiceAvailabilityImpacted_ = true;
    }
}

bool FailoverUnitProxy::TryAddToCurrentlyExecutingActionsLists(
    ProxyActionsListTypes::Enum const & plannedActions, 
    bool impactsServiceAvailability,
    FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext)
{
    bool cancelNeeded;    
    return TryAddToCurrentlyExecutingActionsLists(plannedActions, impactsServiceAvailability, msgContext, cancelNeeded);
}

bool FailoverUnitProxy::TryAddToCurrentlyExecutingActionsLists(
    ProxyActionsListTypes::Enum const & plannedActions, 
    bool impactsServiceAvailability,
    FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext,
    __out bool & cancelNeeded)
{
    cancelNeeded = false;

    {
        // First check if the context is still valid
        if (!msgContext.Validate())
        {
            // The trace will happen in the context if it is not valid
            return false;
        }

        if (isMarkedForAbort_)
        {
            return false;
        }

        if(ProxyActionsList::AreAcceptableActionsListsForParallelExecution(currentlyExecutingActionsLists_, plannedActions))
        {
            if (!isMarkedForClose_ && !ProxyActionsList::AreStandaloneActionsLists(plannedActions))
            {
                if (isOpenForBusiness_)
                {
                    // Record that we are accepting these actions
                    AddActionList(plannedActions, impactsServiceAvailability);

                    return true;
                }
            }
            else
            {
                if (!isMarkedForClose_ && currentlyExecutingActionsLists_.size() == 0)
                {
                    // Record that we are accepting these actions
                    AddActionList(plannedActions, impactsServiceAvailability);

                    if (!isOpenForBusiness_)
                    {
                        OpenForBusiness();
                    }

                    isMarkedForClose_ = ProxyActionsList::IsClose(plannedActions);
                    isMarkedForAbort_ = ProxyActionsList::IsAbort(plannedActions);

                    return true;
                }
                else
                {
                    bool isAbort = ProxyActionsList::IsAbort(plannedActions);
                    if ((!isMarkedForClose_ && isOpenForBusiness_) || isAbort)
                    {
                        isMarkedForClose_ = false;
                        CloseForBusiness(isAbort);
                        cancelNeeded = true;
                    }
                }
            }
        }
    }

    return false;
}

void FailoverUnitProxy::DoneExecutingActionsList(ProxyActionsListTypes::Enum const & completedActions)
{
    AsyncOperationSPtr completeDrainAsyncOperation;

    {
        // While removing the action list update the value of the isServiceAvailabilityImpacted_ flag
        // if the currently executing action list is the last action list that impacts the service availability
        AcquireExclusiveLock grab(lock_);

        int actionListsThatImpactAvailabilityCount = 0;
        bool found = false;
        size_t completedActionListIndex = 0;
        for(size_t i = 0; i < currentlyExecutingActionsLists_.size(); i++)
        {
            if (currentlyExecutingActionsLists_[i].ImpactsServiceAvailability == true)
            {
                actionListsThatImpactAvailabilityCount++;
            }

            if (currentlyExecutingActionsLists_[i].Name == completedActions)
            {
                found = true;
                completedActionListIndex = i;
            }
        }

        ASSERT_IF(
            !found,
            "DoneExecutingActionsList called for an action list that wasn't found in the list of outstanding actions lists.");

        ActionListInfo completingActionList = currentlyExecutingActionsLists_[completedActionListIndex];
        currentlyExecutingActionsLists_.erase(currentlyExecutingActionsLists_.begin() + completedActionListIndex);

        if(drainAsyncOperation_ && currentlyExecutingActionsLists_.size() == 0)
        {
            completeDrainAsyncOperation = move(drainAsyncOperation_);
            OpenForBusiness();
        }

        if (actionListsThatImpactAvailabilityCount == 1 && completingActionList.ImpactsServiceAvailability)
        {
            isServiceAvailabilityImpacted_ = false;
        }
    }

    // Complete the drain async operation outside lock
    if (completeDrainAsyncOperation)
    {
        completeDrainAsyncOperation->TryComplete(completeDrainAsyncOperation, ErrorCodeValue::Success);
    }
}

void FailoverUnitProxy::ProcessReplicaEndpointUpdatedReply(Reliability::ReplicaDescription const & msgReplica)
{
    if (messageStage_ != ProxyMessageStage::RAProxyEndpointUpdated)
    {
        return;
    }

    if (replicaDescription_.ReplicaId != msgReplica.ReplicaId ||
        replicaDescription_.InstanceId != msgReplica.InstanceId ||
        replicaDescription_.CurrentConfigurationRole != msgReplica.CurrentConfigurationRole ||
        replicaDescription_.ServiceLocation != msgReplica.ServiceLocation)
    {
        return;
    }

    messageStage_ = ProxyMessageStage::None;
}

void FailoverUnitProxy::ProcessReadWriteStatusRevokedNotificationReply(Reliability::ReplicaDescription const & msgReplica)
{
    if (messageStage_ != ProxyMessageStage::RAProxyReadWriteStatusRevoked)
    {
        return;
    }

    if (replicaDescription_.ReplicaId != msgReplica.ReplicaId ||
        replicaDescription_.InstanceId != msgReplica.InstanceId)
    {
        return;
    }

    messageStage_ = ProxyMessageStage::None;
}

void FailoverUnitProxy::UpdateServiceDescription(Reliability::ServiceDescription const & serviceDescription)
{    
    {
        AcquireExclusiveLock grab(lock_);

        if (serviceDescription.UpdateVersion <= serviceDescription_.UpdateVersion)
        {   
            return;
        }

        serviceDescription_ = serviceDescription;
    }

    if (statefulService_)
    {
        statefulService_->UpdateInitializationData(serviceDescription.InitializationData);
    }
}

void FailoverUnitProxy::UpdateReadWriteStatus()
{
    AcquireExclusiveLock grab(lock_);
    UpdateReadAndWriteStatus(grab);
}

ProxyErrorCode FailoverUnitProxy::FinalizeDemoteToSecondary()
{
    AcquireExclusiveLock grab(lock_);
    TESTASSERT_IF(catchupResult_ != CatchupResult::CatchupCompleted, "Catchup must have completed on finalize demote {0}", *this);
    TESTASSERT_IF(!AreServiceAndReplicatorRoleCurrent, "Replica and replicator role are inconsistent on finalize demote {0}", *this);
    TESTASSERT_IF(currentServiceRole_ != ReplicaRole::Secondary, "Incorrect replica role on finalize {0}", *this);
    return ProxyErrorCode::CreateRAPError(ErrorCodeValue::RAProxyDemoteCompleted);
}

AsyncOperationSPtr FailoverUnitProxy::BeginOpenInstance(
    __in IApplicationHost & applicationHost,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenInstanceAsyncOperation>(applicationHost, *this, callback, parent);
}

ProxyErrorCode FailoverUnitProxy::EndOpenInstance(AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation)
{    
    return OpenInstanceAsyncOperation::End(asyncOperation, serviceLocation);
}

AsyncOperationSPtr FailoverUnitProxy::BeginCloseInstance(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseInstanceAsyncOperation>(*this, callback, parent);
}

ProxyErrorCode FailoverUnitProxy::EndCloseInstance(AsyncOperationSPtr const & asyncOperation)
{
    return CloseInstanceAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr FailoverUnitProxy::BeginOpenReplica(
    __in IApplicationHost & applicationHost,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenReplicaAsyncOperation>(
        applicationHost,
        *this, 
        callback,
        parent);
}

ProxyErrorCode FailoverUnitProxy::EndOpenReplica(AsyncOperationSPtr const & asyncOperation)
{    
    return OpenReplicaAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr FailoverUnitProxy::BeginChangeReplicaRole(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ChangeReplicaRoleAsyncOperation>(
        *this,
        callback,
        parent);
}

ProxyErrorCode FailoverUnitProxy::EndChangeReplicaRole(AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation)
{
    return ChangeReplicaRoleAsyncOperation::End(asyncOperation, serviceLocation);
}

AsyncOperationSPtr FailoverUnitProxy::BeginCloseReplica(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseReplicaAsyncOperation>(*this, callback, parent);
}

ProxyErrorCode FailoverUnitProxy::EndCloseReplica(AsyncOperationSPtr const & asyncOperation)
{
    return CloseReplicaAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr FailoverUnitProxy::BeginOpenReplicator(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenReplicatorAsyncOperation>(*this, callback, parent);
}

ProxyErrorCode FailoverUnitProxy::EndOpenReplicator(AsyncOperationSPtr const & asyncOperation, __out std::wstring & replicationEndpoint)
{
    return OpenReplicatorAsyncOperation::End(asyncOperation, replicationEndpoint);
}

AsyncOperationSPtr FailoverUnitProxy::BeginChangeReplicatorRole(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ChangeReplicatorRoleAsyncOperation>(
        *this, 
        callback,
        parent);
}

ProxyErrorCode FailoverUnitProxy::EndChangeReplicatorRole(AsyncOperationSPtr const & asyncOperation)
{
    return ChangeReplicatorRoleAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr FailoverUnitProxy::BeginCloseReplicator(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseReplicatorAsyncOperation>(*this, callback, parent);
}

ProxyErrorCode FailoverUnitProxy::EndCloseReplicator(AsyncOperationSPtr const & asyncOperation)
{
    return CloseReplicatorAsyncOperation::End(asyncOperation);
}

void FailoverUnitProxy::Abort(bool keepFUPOpen)
{
    FailoverUnitProxyStates::Enum initialState = FailoverUnitProxyStates::Closed;

    {
        AcquireWriteLock grab(lock_);

        if(IsClosed)
        {
            return;
        }

        initialState = state_;
        state_ = FailoverUnitProxyStates::Closing;
        UpdateReadAndWriteStatus(grab);
    }
    
    AbortReplicator();
    AbortReplica();
    AbortInstance();
    AbortPartition();
    
    {
        AcquireWriteLock grab(lock_);
        if (keepFUPOpen)
        {
            isMarkedForAbort_ = false;
            state_ = initialState;
        }
        else
        {
            state_ = FailoverUnitProxyStates::Closed;
        }
    }
}

void FailoverUnitProxy::AbortPartition()
{
    Common::ComPointer<ComStatefulServicePartition> statefulServicePartition;
    Common::ComPointer<ComStatelessServicePartition> statelessServicePartition;

    {
        AcquireExclusiveLock grab(lock_);

        if (IsStatefulService)
        {
            statefulServicePartition = move(statefulServicePartition_);
        }
        else
        {
            statelessServicePartition = move(statelessServicePartition_);
        }
    }

    if (statefulServicePartition)
    {
        statefulServicePartition->ClosePartition();
        statefulServicePartition.Release();
    }

    if (statelessServicePartition)
    {
        statelessServicePartition->ClosePartition();
        statelessServicePartition.Release();
    }
}

void FailoverUnitProxy::AbortReplica()
{
    {
        AcquireExclusiveLock grab(lock_);

        RAPEventSource::Events->FailoverUnitProxyAbort(
                FailoverUnitId.Guid,
                RAPId,
                ReplicaTag);

        if(!statefulService_)
        {
            RAPEventSource::Events->FailoverUnitProxyAbortServiceIsNull(
                FailoverUnitId.Guid,
                RAPId,
                ReplicaTag);

            return;
        }

        ASSERT_IFNOT(serviceOperationManager_->TryStartOperation(ApiMonitoring::ApiName::Abort), "AbortReplica: Start abort should not fail");
    }

    statefulService_->Abort();

    {
        AcquireExclusiveLock grab(lock_);

        statefulService_ = nullptr;
        replicator_ = nullptr;
        TransitionServiceToClosed(grab);

        serviceOperationManager_->FinishOperation(ApiMonitoring::ApiName::Abort, ErrorCodeValue::Success);
    }
}

void FailoverUnitProxy::AbortInstance()
{
    {
        AcquireExclusiveLock grab(lock_);

        RAPEventSource::Events->FailoverUnitProxyAbort(
                FailoverUnitId.Guid,
                RAPId,
                InstanceTag);

        if(!statelessService_)
        {
            RAPEventSource::Events->FailoverUnitProxyAbortServiceIsNull(
                FailoverUnitId.Guid,
                RAPId,
                InstanceTag);

            return;
        }

        ASSERT_IFNOT(serviceOperationManager_->TryStartOperation(ApiMonitoring::ApiName::Abort), "AbortInstance: Start abort should not fail");
    }

    statelessService_->Abort();
    
    {
        AcquireExclusiveLock grab(lock_);

        statelessService_ = nullptr;
        TransitionServiceToClosed(grab);

        serviceOperationManager_->FinishOperation(ApiMonitoring::ApiName::Abort, ErrorCodeValue::Success);
    }
}

void FailoverUnitProxy::AbortReplicator()
{
    {
        AcquireExclusiveLock grab(lock_);

        RAPEventSource::Events->FailoverUnitProxyAbort(
                FailoverUnitId.Guid,
                RAPId,
                ReplicatorTag);

        if(!replicator_)
        {
            RAPEventSource::Events->FailoverUnitProxyAbortServiceIsNull(
                FailoverUnitId.Guid,
                RAPId,
                ReplicatorTag);

            return;
        }      

        if (replicatorState_ == ReplicatorStates::Closed)
        {
            TESTASSERT_IF(currentReplicatorRole_ != ReplicaRole::Unknown, "Replicator role is incorrect for state {0}", *this);
            // We hold the reference but replicator did not get a change to be opened
            replicator_ = nullptr;            
            return;
        }

        ASSERT_IFNOT(replicatorOperationManager_->TryStartOperation(ApiMonitoring::ApiName::Abort), "AbortReplicator: Start abort should not fail");

        replicatorState_ = ReplicatorStates::Closing;
    }

    replicator_->Abort();

    bool unregisterForCleanup = false;

    {
        AcquireExclusiveLock grab(lock_);

        TransitionReplicatorToClosed(grab);
        replicator_ = nullptr;
        
        remoteReplicas_.clear();
        configurationReplicas_.clear();
        unregisterForCleanup = true;

        replicatorOperationManager_->FinishOperation(ApiMonitoring::ApiName::Abort, ErrorCodeValue::Success);

    }

    if (unregisterForCleanup)
    {
        UnregisterFailoverUnitProxyForCleanupEvent();
    }
}

bool FailoverUnitProxy::IsConfigurationMessageBodyStaleCallerHoldsLock(
    std::vector<Reliability::ReplicaDescription> const & replicaDescriptions) const
{
    return ConfigurationUtility().IsConfigurationMessageBodyStale(replicaDescription_.ReplicaId, replicaDescriptions, configurationReplicas_);
}

bool FailoverUnitProxy::CheckConfigurationMessageBodyForUpdatesCallerHoldsLock(
    vector<Reliability::ReplicaDescription> const & replicaDescriptions,
    bool shouldApply)
{
    return ConfigurationUtility().CheckConfigurationMessageBodyForUpdates(replicaDescription_.ReplicaId, replicaDescriptions, shouldApply, configurationReplicas_);
}

ProxyErrorCode FailoverUnitProxy::UpdateConfiguration(
    ProxyRequestMessageBody const & msgBody,
    UpdateConfigurationReason::Enum reason)
{
    if (reason == UpdateConfigurationReason::PreWriteStatusRevokeCatchup && !IsPreWriteStatusCatchupEnabled)
    {
        return ProxyErrorCode::Success();
    }

    ScopedHeap heap;
    ConfigurationUtility configurationUtility;

    vector<Reliability::ReplicaDescription> const & replicaDescriptions = msgBody.RemoteReplicaDescriptions;

    vector<::FABRIC_REPLICA_INFORMATION> ccReplicas;
    vector<::FABRIC_REPLICA_INFORMATION> pcReplicas;

    vector<Reliability::ReplicaDescription const *> tombstoneReplicas;

    bool runUpdateCatchupConfiguration = false;

    int ccCount = 0;
    int pcCount = 0;

    int ccNonDroppedCount = 0;
    int pcNonDroppedCount = 0;

    UserApiInvoker updateCatchupInvoker(*this, InterfaceName::IReplicator, ApiName::UpdateCatchupConfiguration);
    UserApiInvoker updateCurrentInvoker(*this, InterfaceName::IReplicator, ApiName::UpdateCurrentConfiguration);
    UserApiInvoker *invoker = nullptr;

    {
        AcquireExclusiveLock grab(lock_);

        switch (reason)
        {
        case UpdateConfigurationReason::Default:
            runUpdateCatchupConfiguration =
                configurationStage_ == ProxyConfigurationStage::Catchup ||
                configurationStage_ == ProxyConfigurationStage::CatchupPending ||
                configurationStage_ == ProxyConfigurationStage::PreWriteStatusRevokeCatchup ||
                configurationStage_ == ProxyConfigurationStage::PreWriteStatusRevokeCatchupPending;
            break;

        case UpdateConfigurationReason::PreWriteStatusRevokeCatchup:
        case UpdateConfigurationReason::Catchup:
            runUpdateCatchupConfiguration = true;
            break;
            
        case UpdateConfigurationReason::Current:
            runUpdateCatchupConfiguration = false;
            break;

        default:
            Assert::CodingError("unknown reason {0}", static_cast<int>(reason));
        }

        if (runUpdateCatchupConfiguration)
        {
            invoker = &updateCatchupInvoker;
        }
        else
        {
            invoker = &updateCurrentInvoker;
        }

        invoker->TraceBeforeStart(grab, TraceCorrelatedEvent<ProxyRequestMessageBody>(msgBody));

        ASSERT_IFNOT(IsStatefulServiceFailoverUnitProxy(), "Invalid FUP state during UpdateConfiguration");

        if(replicaState_ != FailoverUnitProxyStates::Opened ||
            replicatorState_ != ReplicatorStates::Opened ||
            currentReplicatorRole_ != ReplicaRole::Primary)
        {
            // If replicator role is not Primary, then ignore UpdateConfiguration since its only valid on Primary
            return ProxyErrorCode::RAProxyOperationIncompatibleWithCurrentFupState();
        }

        ASSERT_IFNOT(replicator_, "Invalid replicator state during UpdateConfiguration");

        if (CurrentConfigurationEpoch > msgBody.FailoverUnitDescription.CurrentConfigurationEpoch)
        {
            return ProxyErrorCode::RAProxyOperationIncompatibleWithCurrentFupState();
        }

        if (IsConfigurationMessageBodyStaleCallerHoldsLock(replicaDescriptions))
        {
            // Drop this stale message
            // Safe to drop using this error code as RAP will not send the reply for it now
            return ProxyErrorCode::RAProxyOperationIncompatibleWithCurrentFupState();
        }

        // Used to indicate if there were any changes to the configurationReplicas_ list
        bool modified = false;

        if (!replicatorOperationManager_->CheckIfOperationCanBeStarted(invoker->Api))
        {
            // No use of sending back a reply to the RA here
            return ProxyErrorCode::RAProxyOperationIncompatibleWithCurrentFupState();
        }

        // STEP 0. Get the replica counts
        configurationUtility.GetReplicaCountForPCAndCC(replicaDescriptions, ccCount, ccNonDroppedCount, pcCount, pcNonDroppedCount);
        
        // STEP 1. Update the current configuration list based on the message
        // UpdateConfiguration contains replicas that are either in PC/CC
        modified = CheckConfigurationMessageBodyForUpdatesCallerHoldsLock(replicaDescriptions, true /*shouldApply*/);

        // STEP 2. Create PC, CC list for replicator
        configurationUtility.CreateReplicatorConfigurationList(
            configurationReplicas_,   
            failoverUnitDescription_.PreviousConfigurationEpoch.IsValid() && failoverUnitDescription_.IsPrimaryChangeBetweenPCAndCC,
            runUpdateCatchupConfiguration, 
            heap, 
            ccReplicas, 
            pcReplicas, 
            tombstoneReplicas);

        /*
            STEP 3. Check if replicator view needs to be updated
            The replicator view must be updated if the configuration has changed.

            If the UC is being invoked as part of a catchup action and catchup has not happend then UC must be called
            Similarly if the UC is being invoked as part of an action list that involves UpdateCurrent (SFEndReconfig etc) and current has not been called then update current

            UC failure is handled here as well because the configurationStage will stay at Pending 
        */
        bool isApiCallNeeded = modified;

        if (!modified)
        {
            switch (reason)
            {
            case UpdateConfigurationReason::Default:
                isApiCallNeeded = configurationStage_ == ProxyConfigurationStage::CatchupPending ||
                    configurationStage_ == ProxyConfigurationStage::PreWriteStatusRevokeCatchupPending ||
                    configurationStage_ == ProxyConfigurationStage::CurrentPending;
                break;

            case UpdateConfigurationReason::Catchup:
                isApiCallNeeded = configurationStage_ == ProxyConfigurationStage::CatchupPending || configurationStage_ == ProxyConfigurationStage::PreWriteStatusRevokeCatchup;
                break;

            case UpdateConfigurationReason::Current:
                isApiCallNeeded = configurationStage_ == ProxyConfigurationStage::CatchupPending ||
                    configurationStage_ == ProxyConfigurationStage::Catchup ||
                    ProxyConfigurationStage::CurrentPending;
                break;

            case UpdateConfigurationReason::PreWriteStatusRevokeCatchup:
                isApiCallNeeded = configurationStage_ == ProxyConfigurationStage::PreWriteStatusRevokeCatchupPending;
                break;

            default:
                Assert::CodingError("Unknown reason {0}", static_cast<int>(reason));
            }
        }

        if (!isApiCallNeeded)
        {
            return ProxyErrorCode::Success();
        }

        /*
            STEP 4. Update the configuration stage

            If the reason for API call was Catchup or PreWriteStatusUpdateCatchup or Current then this is simple to do

            If the reason for API call was Other (i.e. it is a normal UC call) then move the configuration stage back to 'pending' 
            so that a retry will cause the UC to be called even if it was not modified (in case of a previous failure)
        */
        switch (reason)
        {
        case UpdateConfigurationReason::Current:
            configurationStage_ = ProxyConfigurationStage::CurrentPending;
            break;

        case UpdateConfigurationReason::Catchup:
            configurationStage_ = ProxyConfigurationStage::CatchupPending;
            break;

        case UpdateConfigurationReason::PreWriteStatusRevokeCatchup:
            configurationStage_ = ProxyConfigurationStage::PreWriteStatusRevokeCatchupPending;
            break;

        case UpdateConfigurationReason::Default:
            if (configurationStage_ == ProxyConfigurationStage::PreWriteStatusRevokeCatchup)
            {
                configurationStage_ = ProxyConfigurationStage::PreWriteStatusRevokeCatchupPending;
            }
            else if (configurationStage_ == ProxyConfigurationStage::Current)
            {
                configurationStage_ = ProxyConfigurationStage::CurrentPending;
            }
            else if (configurationStage_ == ProxyConfigurationStage::Catchup)
            {
                configurationStage_ = ProxyConfigurationStage::CatchupPending;
            }
            break;

        default:
            Assert::CodingError("unknown reason {0}", static_cast<int>(reason));
        }

        UpdateReadAndWriteStatus(grab);

        auto rv = invoker->TryStartUserApi(grab);
        ASSERT_IF(!rv, "CheckIfOperation returned true under the same lock");
    }

    // Create CC configuration set
    ::FABRIC_REPLICA_SET_CONFIGURATION currentConfiguration;
    GetReplicaSetConfiguration(currentConfiguration, ccReplicas, ccCount, ccNonDroppedCount);

    ErrorCode errorCode;
    if (runUpdateCatchupConfiguration)
    {
        // Create PC configuration set
        ::FABRIC_REPLICA_SET_CONFIGURATION previousConfiguration;
        GetReplicaSetConfiguration(previousConfiguration, pcReplicas, pcCount, pcNonDroppedCount);

        errorCode = replicator_->UpdateCatchUpReplicaSetConfiguration(&currentConfiguration, &previousConfiguration);
    }
    else
    {
        errorCode = replicator_->UpdateCurrentReplicaSetConfiguration(&currentConfiguration);
    }

    {       
        AcquireExclusiveLock grab(lock_);

        invoker->FinishUserApi(grab, errorCode);

        if (errorCode.IsSuccess())
        {
            // Check state transition
            switch (configurationStage_)
            {
                case ProxyConfigurationStage::CatchupPending:
                    if (reason == UpdateConfigurationReason::Catchup || reason == UpdateConfigurationReason::Default) configurationStage_ = ProxyConfigurationStage::Catchup;
                    break;
                case ProxyConfigurationStage::CurrentPending:
                    if (reason == UpdateConfigurationReason::Current || reason == UpdateConfigurationReason::Default) configurationStage_ = ProxyConfigurationStage::Current;
                    break;
                case ProxyConfigurationStage::PreWriteStatusRevokeCatchupPending:
                    if (reason == UpdateConfigurationReason::PreWriteStatusRevokeCatchup || reason == UpdateConfigurationReason::Default) configurationStage_ = ProxyConfigurationStage::PreWriteStatusRevokeCatchup;
                    break;
                case ProxyConfigurationStage::Catchup:
                case ProxyConfigurationStage::Current:
                case ProxyConfigurationStage::PreWriteStatusRevokeCatchup:
                    break;
                default:
                    Assert::CodingError("FailoverUnitProxy::UpdateConfiguration unknown configuration stage.");
            }

            if (configurationStage_ == ProxyConfigurationStage::Current)
            {
                catchupResult_ = CatchupResult::CatchupCompleted;

                UpdateReadAndWriteStatus(grab);            
            }

            // Tombstone replicas in remote list
            for(auto iter = tombstoneReplicas.begin(); iter != tombstoneReplicas.end(); ++iter)
            {
                Reliability::ReplicaDescription const & replicaDesc = **iter;
                auto entry = remoteReplicas_.find(replicaDesc.FederationNodeId);
                if (entry != remoteReplicas_.end())
                {
                    ReplicaProxy & replicaProxy = entry->second;
                    if(replicaProxy.ReplicaDescription.InstanceId == replicaDesc.InstanceId && 
                        replicaProxy.FailoverUnitProxyState == ReplicaProxyStates::Ready)
                    {
                        ASSERT_IF(replicaProxy.BuildIdleOperation.IsRunning, "Pending build operation while Ready");
                        replicaProxy.State = ReplicaProxyStates::Dropped;
                    }
                }
            }
        }

        // Tracing at noise level after this call as well (before operation is a misnomer)
        invoker->TraceBeforeStart(grab, TraceCorrelatedEvent<ProxyRequestMessageBody>(msgBody));
    }

    return invoker->GetError(move(errorCode));
}

void FailoverUnitProxy::GetReplicaSetConfiguration(
    ::FABRIC_REPLICA_SET_CONFIGURATION & replicaSetConfiguration,
    vector<::FABRIC_REPLICA_INFORMATION> & replicas,
    int setCount,
    int setNonDroppedCount)
{
    int writeQuorum = setCount / 2 + 1;
    int writeQuorumSize = (setNonDroppedCount < writeQuorum) ? setNonDroppedCount : writeQuorum;

    // Set previous configuration
    ::FABRIC_REPLICA_INFORMATION * activeReplicas = NULL;
    ULONG activeReplicaCount = (ULONG)replicas.size();
    if (activeReplicaCount > 0)
    {
        activeReplicas = &replicas.front();
    }

    replicaSetConfiguration.ReplicaCount = activeReplicaCount;
    replicaSetConfiguration.WriteQuorum = static_cast<ULONG>(writeQuorumSize);
    replicaSetConfiguration.Replicas = activeReplicas;
    replicaSetConfiguration.Reserved = NULL;
}

AsyncOperationSPtr FailoverUnitProxy::BeginReplicatorBuildIdleReplica(
    Reliability::ReplicaDescription const & idleReplicaDescription,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{   
    return AsyncOperation::CreateAndStart<ReplicatorBuildIdleReplicaAsyncOperation>(idleReplicaDescription, *this, callback, parent);
}

ProxyErrorCode FailoverUnitProxy::EndReplicatorBuildIdleReplica(AsyncOperationSPtr const & asyncOperation)
{ 
    // The RA doesn't care about continuous failures to build a replica yet so dont worry about the errors
    return ProxyErrorCode::CreateRAPError(ReplicatorBuildIdleReplicaAsyncOperation::End(asyncOperation));
}

ProxyErrorCode FailoverUnitProxy::ReplicatorRemoveIdleReplica(Reliability::ReplicaDescription const & idleReplicaDescription)
{
    UserApiInvoker invoker(*this, InterfaceName::IReplicator, ApiName::RemoveReplica);

    AsyncOperationSPtr buildIdleReplicaAsyncOperation = nullptr;
    bool isRemove = false;

    {
        AcquireExclusiveLock grab(lock_);

        invoker.TraceBeforeStart(grab, TraceCorrelatedEvent<Reliability::ReplicaDescription>(idleReplicaDescription));
       
        ASSERT_IFNOT(IsStatefulServiceFailoverUnitProxy() && replicaState_ == FailoverUnitProxyStates::Opened, "Cannot remove idle replica except on an open FailoverUnit proxy hosting a stateful service.");
        ASSERT_IFNOT(replicator_ && replicatorState_ == ReplicatorStates::Opened, "Cannot remove idle replica without an opened replicator.");

        auto entry = remoteReplicas_.find(idleReplicaDescription.FederationNodeId);

        if (entry == remoteReplicas_.end())
        {
            // Replica isn't in the list of remote replicas, the build must not have been processed yet
            // Create a tombstone (dropped entry) for it; if a build ever arrives the tomstone will cause it
            // to be (correctly) suppressed
            
            ReplicaProxy replicaProxy(idleReplicaDescription);
            replicaProxy.State = ReplicaProxyStates::Dropped;

            remoteReplicas_.insert(std::pair<Federation::NodeId, ReplicaProxy>(idleReplicaDescription.FederationNodeId, std::move(replicaProxy)));
            entry = remoteReplicas_.find(idleReplicaDescription.FederationNodeId);
        }
        else
        {
            if (entry->second.ReplicaDescription.InstanceId > idleReplicaDescription.InstanceId)
            {
                return ProxyErrorCode::RAProxyOperationIncompatibleWithCurrentFupState();
            }

            // Ensure we have the correct instance id for this replica
            entry->second.ReplicaDescription = idleReplicaDescription;
        }

        ReplicaProxy & thisReplica = entry->second;

        switch (thisReplica.ReplicatorState)
        {
            case ReplicaProxyStates::Ready:
            case ReplicaProxyStates::InBuild:
            case ReplicaProxyStates::InDrop:
                break;
            case ReplicaProxyStates::Dropped:
                // Drop already happened, report success
                thisReplica.State = ReplicaProxyStates::Dropped;
                return ProxyErrorCode::Success();
        }

        // Grab the async result to cancel
        buildIdleReplicaAsyncOperation = thisReplica.BuildIdleOperation.AsyncOperation;
        isRemove = thisReplica.ReplicatorState != ReplicaProxyStates::InBuild && !buildIdleReplicaAsyncOperation;
        if (isRemove)
        {
            // Only if the build is not pending we can proceed with Remove, otherwise we will wait for the retry
            if (!invoker.TryStartUserApi(grab))
            {
                return ProxyErrorCode::RAProxyOperationIncompatibleWithCurrentFupState();
            }

            thisReplica.State = ReplicaProxyStates::InDrop;
        }
    }

    if (!isRemove)
    {
        // Cancel the outstanding build operation (if any)
        if (buildIdleReplicaAsyncOperation)
        {
            buildIdleReplicaAsyncOperation->Cancel();
        }

        return ProxyErrorCode::RAProxyOperationIncompatibleWithCurrentFupState();
    }

    ErrorCode errorCode = replicator_->RemoveIdleReplica(idleReplicaDescription.ReplicaId);

    {
        AcquireExclusiveLock grab(lock_);

        ErrorCodeValue::Enum retValue = errorCode.ReadValue();
        
        if (retValue == ErrorCodeValue::Success || retValue == ErrorCodeValue::REReplicaDoesNotExist)
        {
            auto entry = remoteReplicas_.find(idleReplicaDescription.FederationNodeId);
            ASSERT_IF(entry == remoteReplicas_.end(), "Replica wasn't found in the list of remote replicas after remove completed.");

            ReplicaProxy & thisReplica = entry->second;

            // Mark as dropped if we successfully removed it
            thisReplica.State = ReplicaProxyStates::Dropped;

            errorCode = ErrorCodeValue::Success;
        }

        invoker.FinishUserApi(grab, errorCode);
    }

    return invoker.GetError(move(errorCode));
}

void FailoverUnitProxy::RegisterFailoverUnitProxyForCleanupEvent()
{
    if (serviceDescription_.TargetReplicaSetSize > 1)
    {
        reconfigurationAgentProxy_.LocalFailoverUnitProxyMapObj.
            RegisterFailoverUnitProxyForCleanupEvent(failoverUnitDescription_.FailoverUnitId);
    }
}

void FailoverUnitProxy::UnregisterFailoverUnitProxyForCleanupEvent()
{
    if (serviceDescription_.TargetReplicaSetSize > 1)
    {
        reconfigurationAgentProxy_.LocalFailoverUnitProxyMapObj.
            UnregisterFailoverUnitProxyForCleanupEvent(failoverUnitDescription_.FailoverUnitId);
    }
}

void FailoverUnitProxy::CleanupEventHandler()
{
    {
        AcquireExclusiveLock grab(lock_);

        if (state_ == FailoverUnitProxyStates::Closed)
        {
            return;
        }

        for (auto iter = remoteReplicas_.begin(); iter != remoteReplicas_.end();)
        {
            if ((iter->second.FailoverUnitProxyState == ReplicaProxyStates::Dropped) &&
                ((iter->second.StateUpdateTime + FailoverConfig::GetConfig().FailoverUnitProxyCleanupInterval) < DateTime::Now()))
            {
                iter = remoteReplicas_.erase(iter);
                continue;
            }

            ++iter;
        }
    }
}

AsyncOperationSPtr FailoverUnitProxy::BeginReplicatorOnDataLoss(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{   
    return AsyncOperation::CreateAndStart<ReplicatorOnDataLossAsyncOperation>(*this, callback, parent);
}

ProxyErrorCode FailoverUnitProxy::EndReplicatorOnDataLoss(AsyncOperationSPtr const & asyncOperation, int64 & lastLSN)
{
    return ReplicatorOnDataLossAsyncOperation::End(asyncOperation, lastLSN);
}

AsyncOperationSPtr FailoverUnitProxy::BeginReplicatorUpdateEpoch(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{   
    return AsyncOperation::CreateAndStart<ReplicatorUpdateEpochAsyncOperation>( *this, callback, parent);
}

ProxyErrorCode FailoverUnitProxy::EndReplicatorUpdateEpoch(AsyncOperationSPtr const & asyncOperation)
{
    return ReplicatorUpdateEpochAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr FailoverUnitProxy::BeginReplicatorCatchupReplicaSet(
    CatchupType::Enum type,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ReplicatorCatchupReplicaSetAsyncOperation>(type, *this, callback, parent);
}

ProxyErrorCode FailoverUnitProxy::EndReplicatorCatchupReplicaSet(AsyncOperationSPtr const & asyncOperation)
{
    return ReplicatorCatchupReplicaSetAsyncOperation::End(asyncOperation);
}

bool FailoverUnitProxy::get_IsCatchupPending() const
{
    {
        // Is catchup running right now?
        if(replicatorOperationManager_->IsOperationRunning(ApiMonitoring::ApiName::CatchupReplicaSet))
        {
            return true;
        }

        // Is there an action batch running that includes catchup?
        auto pendingCatchupActionList = std::find_if(
            currentlyExecutingActionsLists_.begin(),
            currentlyExecutingActionsLists_.end(),
            [] (ActionListInfo const & actionList)
            {
                return
                    actionList.Name == ProxyActionsListTypes::StatefulServiceDemoteToSecondary;
            }
        );

        return (pendingCatchupActionList != currentlyExecutingActionsLists_.end());
    }
}

ProxyErrorCode FailoverUnitProxy::CancelReplicatorCatchupReplicaSet()
{
    {
        AcquireExclusiveLock grab(lock_);
        isCatchupCancel_ = true;
    }

    replicatorOperationManager_->CancelOrMarkOperationForCancel(ApiMonitoring::ApiName::CatchupReplicaSet);

    {
        AcquireExclusiveLock grab(lock_);

        if (!IsCatchupPending)
        {
            if (currentReplicatorRole_ == currentServiceRole_)
            {
                if (currentServiceRole_ == ReplicaRole::Secondary)
                {
                    return ProxyErrorCode::CreateRAPError(ErrorCodeValue::RAProxyDemoteCompleted);
                }
                else
                {
                    return ProxyErrorCode::CreateRAPError(ErrorCodeValue::Success);
                }
            }
        }
    }

    return ProxyErrorCode::RAProxyOperationIncompatibleWithCurrentFupState();
}

void FailoverUnitProxy::DoneCancelReplicatorCatchupReplicaSet()
{
    isCatchupCancel_ = false;
    replicatorOperationManager_->RemoveOperationForCancel(ApiMonitoring::ApiName::CatchupReplicaSet);
}

ProxyErrorCode FailoverUnitProxy::ReplicatorGetStatus(__out FABRIC_SEQUENCE_NUMBER & firstLsn, __out FABRIC_SEQUENCE_NUMBER & lastLsn)
{
    UserApiInvoker invoker(*this, InterfaceName::IReplicator, ApiName::GetStatus);

    {
        AcquireExclusiveLock grab(lock_);

        invoker.TraceBeforeStart(grab);

        ASSERT_IFNOT(IsStatefulServiceFailoverUnitProxy() && replicaState_ == FailoverUnitProxyStates::Opened, "Cannot get replicator status except on an open FailoverUnit proxy hosting a stateful service.");
        ASSERT_IFNOT(replicator_ && replicatorState_ == ReplicatorStates::Opened, "Cannot get replicator status without an opened replicator.");

        if (!invoker.TryStartUserApi(grab))
        {
            return ProxyErrorCode::RAProxyOperationIncompatibleWithCurrentFupState();
        }
    }

    ErrorCode errorCode = replicator_->GetStatus(firstLsn, lastLsn);
    
    {
        AcquireExclusiveLock grab(lock_);

        invoker.FinishUserApi(grab, errorCode);
    }
    
    return invoker.GetError(move(errorCode));
}

ProxyErrorCode FailoverUnitProxy::ReplicatorGetQuery(__out ServiceModel::ReplicatorStatusQueryResultSPtr & result)
{
    UserApiInvoker invoker(*this, InterfaceName::IReplicator, ApiName::GetQuery);

    {
        AcquireExclusiveLock grab(lock_);

        invoker.TraceBeforeStart(grab);

        if (!replicator_ || replicatorState_ != ReplicatorStates::Opened)
        {
            return ProxyErrorCode::CreateRAPError(ErrorCodeValue::NotReady);
        }

        if (!invoker.TryStartUserApi(grab))
        {
            return ProxyErrorCode::CreateRAPError(ErrorCodeValue::InvalidOperation);
        }
    }

    ErrorCode errorCode = replicator_->GetReplicatorQueryResult(result);
    
    {
        AcquireExclusiveLock grab(lock_);
        invoker.FinishUserApi(grab, errorCode);
    }

    return invoker.GetError(move(errorCode));
}

Common::ErrorCode FailoverUnitProxy::ReplicaGetQuery(__out ServiceModel::ReplicaStatusQueryResultSPtr & result) const
{
    if (statefulService_)
    {
        return statefulService_->GetQueryResult(result);
    }
    else
    {
        return ErrorCodeValue::InvalidState;
    }
}

void FailoverUnitProxy::ProcessUpdateConfigurationMessage(
    ProxyRequestMessageBody const & msgBody)
{
    // Preserve the replication endpoint and service location already on the FUP since, to handle
    // standby to active transition
    // TODO: This should not be needed on every single UC message
    auto replicaDesc = msgBody.LocalReplicaDescription;
    replicaDesc.ReplicationEndpoint = ReplicaDescription.ReplicationEndpoint;
    replicaDesc.ServiceLocation = ReplicaDescription.ServiceLocation;
    ReplicaDescription = replicaDesc;

    /*
        It is important to clear the catchup result here. Consider the scenario below:
        1. Process add primary on RAP with data loss
        2. This will leave the RAP state for catchupResult_ marked as "DataLoss" after the on data loss async op completes.
        3. new replica gets built.RA sends UC with Catchup to promote I / S.Let this be M1.
        4. Let RA retry UC with catchup.Let this be M2.
        5. M1 arrives on RAP and sees that the incoming epoch is higher and updates the cc epoch and enqueues the action list.It releases the lock on the FUP
        6. M2 arrives and acquires the lock sees the CC epoch is matching and the result is DataLoss and assumes M2 is a duplicate and sends back the reply with data loss detected.
        7. RA drops this and test asserts

        This requires a very tiny window where M1 releases the lock and M2 observes the FUP state.The first action that M1 performs after this is acquire the lock again and fix up the catchupResult_ to indicate that the reconfig is not complete.

        The catchup result must only be cleared by the first uc message otherwise
        detecting the duplicate completed message is not possible because any retry would clear this state
    */
    if (FailoverUnitDescription.CurrentConfigurationEpoch != msgBody.FailoverUnitDescription.CurrentConfigurationEpoch)
    {
        catchupResult_ = CatchupResult::NotStarted;
        FailoverUnitDescription.PreviousConfigurationEpoch = msgBody.FailoverUnitDescription.PreviousConfigurationEpoch;
        FailoverUnitDescription.CurrentConfigurationEpoch = msgBody.FailoverUnitDescription.CurrentConfigurationEpoch;
    }
}

void FailoverUnitProxy::OnReconfigurationStarting()
{
    AcquireExclusiveLock grab(lock_);

    catchupResult_ = CatchupResult::NotStarted;
    UpdateReadAndWriteStatus(grab);
}

void FailoverUnitProxy::OnReconfigurationEnding()
{
    AcquireExclusiveLock grab(lock_);

    if (catchupResult_ == CatchupResult::NotStarted)
    {
        // Must be the open of a primary replica otherwise either the completion of catchup or the completion of data loss will update this
        TESTASSERT_IF(CurrentConfigurationRole != ReplicaRole::Primary, "Must be primary here {0}", *this);
        TESTASSERT_IF(!configurationReplicas_.empty(), "During add primary must have no replicas {0}", *this);
        catchupResult_ = CatchupResult::CatchupCompleted;
    }

    UpdateReadAndWriteStatus(grab);
}

void FailoverUnitProxy::OpenPartition()
{
    AcquireExclusiveLock grab(lock_);

    if (IsStatefulService)
    {
        if (statefulServicePartition_.GetRawPointer() != nullptr)
        {
            statefulServicePartition_->AssertIsValid(FailoverUnitId.Guid, replicaDescription_.ReplicaId, *this);
            return;
        }

        weak_ptr<FailoverUnitProxy> failoverUnitProxyWPtr = weak_ptr<FailoverUnitProxy>(shared_from_this());
        statefulServicePartition_ = make_com<ComStatefulServicePartition>(
            FailoverUnitId.Guid,
            replicaDescription_.ReplicaId,
            failoverUnitDescription_.ConsistencyUnitDescription,
            serviceDescription_.HasPersistedState,
            move(failoverUnitProxyWPtr));
    }
    else
    {
        if (statelessServicePartition_.GetRawPointer() != nullptr)
        {
            statelessServicePartition_->AssertIsValid(FailoverUnitId.Guid, replicaDescription_.ReplicaId, *this);
            return;
        }

        weak_ptr<FailoverUnitProxy> failoverUnitProxyWPtr = weak_ptr<FailoverUnitProxy>(shared_from_this());
        statelessServicePartition_ = make_com<ComStatelessServicePartition>(
            failoverUnitDescription_.FailoverUnitId.Guid,
            failoverUnitDescription_.ConsistencyUnitDescription,
            replicaDescription_.ReplicaId,
            move(failoverUnitProxyWPtr));
    }
}

void FailoverUnitProxy::AssertCatchupNotStartedCallerHoldsLock() const
{
    TESTASSERT_IF(catchupResult_ != CatchupResult::NotStarted, "Catchup must not be started {0}", *this);
}

void FailoverUnitProxy::UpdateReadAndWriteStatus(Common::AcquireExclusiveLock & grab)
{
    if (!statefulServicePartition_)
    {
        return;
    }

    // Compute the read/write status based on the lifecycle and reconfiguration state
    auto readAndWriteStatus = ReadWriteStatusCalculatorObj->ComputeReadAndWriteStatus(grab, *this);

    if (readWriteStatusState_.TryUpdate(readAndWriteStatus.first, readAndWriteStatus.second))
    {
        statefulServicePartition_->SetReadWriteStatus(move(readWriteStatusState_.Current));
        
        RAPEventSource::Events->UpdateAccessStatus(
            FailoverUnitId.Guid,
            RAPId,
            replicaDescription_.ReplicaId,
            readAndWriteStatus.first,
            readAndWriteStatus.second);
    }
}

bool FailoverUnitProxy::HasMinReplicaSetAndWriteQuorum(Common::AcquireExclusiveLock & , bool includePCCheck) const
{
    if(this->replicatorState_ != FailoverUnitProxyStates::Opened)
    {
        return false;
    }

    int ccFailoverReplicaSetSize = 1;
    int ccReplicatorReplicaSetSize = 1;
    int pcFailoverReplicaSetSize = 1;
    int pcReplicatorReplicaSetSize = 1;
        
    for (auto iter = configurationReplicas_.begin(); iter != configurationReplicas_.end(); ++iter)
    {
        /*
            This method can also be called during demote
            At this point the configuration will have a replica in CC which is primary
            That replica also needs to be considered

            For the PC there can be no primary replica because the configuration only contains remote replicas
            and the local replica (primary) is always carrying out the reconfiguration
        */
        if(iter->second.CurrentConfigurationRole == ReplicaRole::Secondary || iter->second.CurrentConfigurationRole == ReplicaRole::Primary)
        {
            ++ccFailoverReplicaSetSize;
            
            if(iter->second.IsUp && iter->second.State == Reliability::ReplicaStates::Ready)
            {
                ++ccReplicatorReplicaSetSize;
            }
        }

        if(iter->second.PreviousConfigurationRole == ReplicaRole::Secondary)
        {
            ++pcFailoverReplicaSetSize;
            
            if(iter->second.IsUp && iter->second.State == Reliability::ReplicaStates::Ready)
            {
                ++pcReplicatorReplicaSetSize;
            }
        }        
    }
    
    int ccWriteQuorum = ccFailoverReplicaSetSize / 2 + 1;
    int pcWriteQuorum = pcFailoverReplicaSetSize / 2 + 1;

    bool quorumCheck = 
        ccFailoverReplicaSetSize >= serviceDescription_.MinReplicaSetSize && 
        ccReplicatorReplicaSetSize >= ccWriteQuorum;

    if (includePCCheck)
    {
        quorumCheck &= pcReplicatorReplicaSetSize >= pcWriteQuorum;
    }

    return quorumCheck;
}

bool FailoverUnitProxy::TryMarkForAbort()
{
    {
        if (!isMarkedForAbort_)
        {
            isMarkedForClose_ = false;
            isMarkedForAbort_ = true;
            OpenForBusiness();

            return true;
        }
    }

    return false;
}

bool FailoverUnitProxy::TryDelete()
{
    if (state_ != FailoverUnitProxyStates::Closed)
    {
        return false;
    }

    return (Common::Stopwatch::Now() - state_.LastTransitionTime) > FailoverConfig::GetConfig().FailoverUnitProxyCleanupInterval;
}

AsyncOperationSPtr FailoverUnitProxy::BeginMarkForCloseAndDrainOperations(
    bool isAbort,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    bool cancel = false;
    AsyncOperationSPtr cancelDrainAsyncOperation;
    AsyncOperationSPtr drainAsyncOperation;

    {
        AcquireExclusiveLock grab(lock_);

        if (drainAsyncOperation_)
        {
            // Cancel any pending drainAsyncOperation
            cancelDrainAsyncOperation = move(drainAsyncOperation_);
        }

        drainAsyncOperation_ = AsyncOperation::CreateAndStart<WaitForDrainAsyncOperation>(callback, parent);
        drainAsyncOperation = drainAsyncOperation_;

        if (isAbort)
        {
            if (!isMarkedForAbort_)
            {
                isMarkedForClose_ = false;
                isMarkedForAbort_ = true;
                CloseForBusiness(true/*isAbort*/);
                cancel = true;
            }
        }
        else
        {
            if (!isMarkedForClose_ && !isMarkedForAbort_)
            {
                isMarkedForClose_ = true;

                CloseForBusiness(false/*isAbort*/);
                cancel = true;
            }
        }
    }

    if (cancelDrainAsyncOperation)
    {
        cancelDrainAsyncOperation->Cancel();
    }

    if (cancel)
    {
        CancelOperations();
    }

    bool shouldComplete = false;

    {
        // Check if the operations are already drained
        AcquireExclusiveLock grab(lock_);
        if (currentlyExecutingActionsLists_.size() == 0)
        {
            shouldComplete = true;
            OpenForBusiness();
        }
    }

    if (shouldComplete)
    {
        drainAsyncOperation->TryComplete(drainAsyncOperation, ErrorCodeValue::Success);
    }

    return drainAsyncOperation;
}

Common::ErrorCode FailoverUnitProxy::EndMarkForCloseAndDrainOperations(
    AsyncOperationSPtr const & asyncOperation)
{
    auto casted = AsyncOperation::End<WaitForDrainAsyncOperation>(asyncOperation);

    ErrorCode error = casted->Error;

    if (error.IsError(ErrorCodeValue::OperationCanceled))
    {
        return error;
    }

    AsyncOperationSPtr asyncOp;
    {
        Common::AcquireExclusiveLock grab(lock_);
        ASSERT_IFNOT(currentlyExecutingActionsLists_.empty(), "EndMarkForCloseAndDrainOperations: {0} operations left", currentlyExecutingActionsLists_.size());
        drainAsyncOperation_.reset();
    }

    return error;
}

void FailoverUnitProxy::CancelOperations()
{
    wstring actionListString;

    {
        AcquireExclusiveLock grab(lock_);
        if (currentlyExecutingActionsLists_.size() == 0)
        {
            return;
        }

        actionListString = wformatString(currentlyExecutingActionsLists_);
    }

    RAPEventSource::Events->FailoverUnitProxyCancel(
            FailoverUnitId.Guid,
            RAPId,
            actionListString);

    CancelBuildIdleReplicaOperations();

    if (replicatorOperationManager_)
    {
        replicatorOperationManager_->CancelOperations();
    }

    if (serviceOperationManager_)
    {
        serviceOperationManager_->CancelOperations();
    }
}

void FailoverUnitProxy::CancelBuildIdleReplicaOperations()
{
    vector<AsyncOperationSPtr> outstandingBuildIdleReplicaAsyncOperations;

    // Build a list of outstanding build idle replica calls
    {
        AcquireExclusiveLock grab(lock_);
        for (auto iter = remoteReplicas_.begin(); iter != remoteReplicas_.end(); ++iter)
        {
            if(iter->second.BuildIdleOperation.AsyncOperation)
            {
                outstandingBuildIdleReplicaAsyncOperations.push_back(iter->second.BuildIdleOperation.AsyncOperation);
            }
        }
    }

    // Loop to cancel
    for_each(
        outstandingBuildIdleReplicaAsyncOperations.begin(),
        outstandingBuildIdleReplicaAsyncOperations.end(),
        [] (AsyncOperationSPtr const & operation)
        {
            operation->Cancel();
        }
    );
    outstandingBuildIdleReplicaAsyncOperations.clear();
}

void FailoverUnitProxy::CloseForBusiness(bool isAbort)
{
    isOpenForBusiness_ = false;

    if (serviceOperationManager_)
    {
        serviceOperationManager_->CloseForBusiness(isAbort);
    }

    if (replicatorOperationManager_)
    {
        replicatorOperationManager_->CloseForBusiness(isAbort);
    }
}

void FailoverUnitProxy::OpenForBusiness()
{
    isOpenForBusiness_ = true;

    if (serviceOperationManager_)
    {
        serviceOperationManager_->OpenForBusiness();
    }
 
    if(replicatorOperationManager_)
    {
        replicatorOperationManager_->OpenForBusiness();
    }
}

Common::AsyncOperationSPtr FailoverUnitProxy::BeginClose(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, callback, parent);
}

ErrorCode FailoverUnitProxy::EndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void FailoverUnitProxy::WriteTo(TextWriter& writer, FormatOptions const &options) const
{
    UNREFERENCED_PARAMETER(options);

    writer.Write("{0} {1} [ReplicatorState = {2}] [ReplicaState = {3}] {4} {5} {6} {7} {8}",
        failoverUnitDescription_,
        state_,
        replicatorState_,
        replicaState_,
        IsStatefulServiceFailoverUnitProxy() ? "SF" : 
            (IsStatelessServiceFailoverUnitProxy() ? "SL" : ""),        
        catchupResult_,
        isCatchupCancel_ ? L"1" : L"0",
        configurationStage_,
        messageStage_);
        
    writer.Write(" ({0}) ({1})",
        serviceOperationManager_,
        replicatorOperationManager_);

     writer.WriteLine(" CRR:{0} CSR:{1} CRS:{2} ROM:{3}",
        currentReplicatorRole_,
        currentServiceRole_,
        currentReplicaState_,
        replicaOpenMode_);

     writer.Write("{0}",
        replicaDescription_);

    for (auto replica = this->configurationReplicas_.begin(); replica != configurationReplicas_.end(); ++replica)
    {
        writer.Write("{0}", replica->second);
    }

    writer.WriteLine();

    for (auto replica = this->remoteReplicas_.begin(); replica != remoteReplicas_.end(); ++replica)
    {
        writer.Write("{0}", replica->second);
    }
}

wstring FailoverUnitProxy::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}

string FailoverUnitProxy::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "{0} {1} [ReplicatorState = {2}] [ReplicaState = {3}] {4} {5} {6} {7} ({8}) ({9}) CRR:{10} CSR:{11} CRS:{12} ROM:{13}\r\n{14}{15}\r\n{16}";
    size_t index = 0;

    traceEvent.AddEventField<Reliability::FailoverUnitDescription>(format, name + ".ftDesc", index);
    traceEvent.AddEventField<FailoverUnitProxyStates::Trace>(format, name + ".state", index);
    traceEvent.AddEventField<ReplicatorStates::Trace>(format, name + ".replicatorState", index);
    traceEvent.AddEventField<FailoverUnitProxyStates::Trace>(format, name + ".replicaState2", index);
    traceEvent.AddEventField<CatchupResult::Trace>(format, name + ".catchupResult", index);
    traceEvent.AddEventField<char>(format, name + ".isCatchupCancel", index);
    traceEvent.AddEventField<Reliability::ReconfigurationAgentComponent::ProxyConfigurationStage::Trace>(format, name + ".configurationStage", index);
    traceEvent.AddEventField<Reliability::ReconfigurationAgentComponent::ProxyMessageStage::Trace>(format, name + ".proxyMessageStage", index);

    traceEvent.AddEventField<string>(format, name + ".serviceOpMgr", index);
    traceEvent.AddEventField<string>(format, name + ".replicatorOpMgr", index);
    traceEvent.AddEventField<Reliability::ReplicaRole::Trace>(format, name + ".curReplicatorRole", index);
    traceEvent.AddEventField<Reliability::ReplicaRole::Trace>(format, name + ".curServiceRole", index);
    traceEvent.AddEventField<ReplicaStates::Trace>(format, name + ".replicaState", index);
    traceEvent.AddEventField<ReplicaOpenMode::Trace>(format, name + ".replicaOpenMode", index);
    traceEvent.AddEventField<Reliability::ReplicaDescription>(format, name + ".replicaDesc", index);

    traceEvent.AddEventField<std::map<Federation::NodeId, Reliability::ReplicaDescription>>(format, name + ".configReplicas", index);
    traceEvent.AddEventField<std::map<Federation::NodeId, Reliability::ReconfigurationAgentComponent::ReplicaProxy>>(format, name + ".remoteReplicas", index);

    return format;
}

void FailoverUnitProxy::FillEventData(TraceEventContext & context) const
{
    context.Write(failoverUnitDescription_);
    context.WriteCopy(static_cast<uint>(state_.State));
    context.WriteCopy(static_cast<uint>(replicatorState_));
    context.WriteCopy(static_cast<uint>(replicaState_));
    context.WriteCopy(static_cast<uint>(catchupResult_));
    context.WriteCopy(static_cast<char>(isCatchupCancel_));
    context.WriteCopy(static_cast<uint>(configurationStage_));
    context.WriteCopy(static_cast<uint>(messageStage_));

    context.WriteCopy(serviceOperationManager_ == nullptr ? string() : serviceOperationManager_->ToString());
    context.WriteCopy(replicatorOperationManager_ == nullptr ? string() : replicatorOperationManager_->ToString());
    context.WriteCopy(static_cast<uint>(currentReplicatorRole_));
    context.WriteCopy(static_cast<uint>(currentServiceRole_));
    context.WriteCopy(static_cast<uint>(currentReplicaState_));
    context.WriteCopy(static_cast<uint>(replicaOpenMode_));
    context.Write(replicaDescription_);

    context.Write(configurationReplicas_);
    context.Write(remoteReplicas_);
}

ErrorCode FailoverUnitProxy::ReportLoad(std::vector<LoadBalancingComponent::LoadMetric> && loadMetrics)
{
    Reliability::FailoverUnitId failoverUnitId;
    bool isStateful;
    ReplicaRole::Enum role;
    wstring serviceName;

    {
        AcquireExclusiveLock grab(lock_);
        if(replicaState_ == FailoverUnitProxyStates::Closed)
        {
            RAPEventSource::Events->FailoverUnitProxyInvalidStateForReportLoad(
                FailoverUnitId.Guid,
                RAPId,
                replicaState_);

            return ErrorCodeValue::ObjectClosed;
        }

        failoverUnitId = failoverUnitDescription_.FailoverUnitId;
        serviceName = serviceDescription_.Name;
        isStateful = serviceDescription_.IsStateful;
        role = replicaDescription_.CurrentConfigurationRole;

        reportedLoadStore_.AddLoad(loadMetrics);
    }

    reconfigurationAgentProxy_.ReportLoad(failoverUnitId, move(serviceName), isStateful, role, move(loadMetrics));
    return ErrorCodeValue::Success;
}

ErrorCode FailoverUnitProxy::ReportReplicatorHealth(
    Common::SystemHealthReportCode::Enum reportCode,
    std::wstring const & dynamicProperty,
    std::wstring const & extraDescription,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    Common::TimeSpan const & timeToLive)
{
    {
        AcquireReadLock grab(lock_);
        if(this->replicatorState_ == FailoverUnitProxyStates::Closed ||
           this->replicatorState_ == FailoverUnitProxyStates::Opening)
        {
            // Do not allow health reporting if replicator is already closed or is still opening.
            // Both of them can lead to stale health reports from the old replicator being reported on the new instance ID
            // that we already have
            return ErrorCodeValue::ObjectClosed;
        }
    }

    ServiceModel::AttributeList attributes;
    attributes.AddAttribute(*HealthAttributeNames::NodeId, wformatString(replicaDescription_.FederationNodeId));
    attributes.AddAttribute(*HealthAttributeNames::NodeInstanceId, wformatString(replicaDescription_.FederationNodeInstance.InstanceId));

    auto entity = ServiceModel::StatefulReplicaEntityHealthInformation::CreateStatefulReplicaEntityHealthInformation(
        failoverUnitDescription_.FailoverUnitId.Guid,
        replicaDescription_.ReplicaId,
        replicaDescription_.InstanceId);

    auto report = ServiceModel::HealthReport::CreateSystemHealthReport(
        reportCode,
        move(entity),
        dynamicProperty,
        extraDescription,
        sequenceNumber,
        timeToLive,
        move(attributes));

    reconfigurationAgentProxy_.LocalHealthReportingComponentObj.ReportHealth(move(report), nullptr);
    return ErrorCodeValue::Success;
}

ErrorCode FailoverUnitProxy::ReportFault(FaultType::Enum faultType)
{
    Reliability::FailoverUnitDescription failoverUnitDescription;
    Reliability::ReplicaDescription replicaDescription;

    {
        AcquireExclusiveLock grab(lock_);
        if(replicaState_ == FailoverUnitProxyStates::Closed)
        {
            RAPEventSource::Events->FailoverUnitProxyInvalidStateForReportFault(
                FailoverUnitId.Guid,
                RAPId,
                replicaState_);

            return ErrorCodeValue::ObjectClosed;
        }

        if (faultType == FaultType::Transient)
        {
            if(messageStage_ == ProxyMessageStage::RAProxyReportFaultTransient ||
               messageStage_ == ProxyMessageStage::RAProxyReportFaultPermanent)
            {
                // Already send reportfault
                return ErrorCodeValue::Success;
            }
            else
            {
                messageStage_ = ProxyMessageStage::RAProxyReportFaultTransient;
            }
        }
        else if (faultType == FaultType::Permanent)
        {
            if(messageStage_ == ProxyMessageStage::RAProxyReportFaultPermanent)
            {
                // Already send reportfault with permanent
                return ErrorCodeValue::Success;
            }
            else
            {
                messageStage_ = ProxyMessageStage::RAProxyReportFaultPermanent;
            }
        }
        else
        {
            return ErrorCodeValue::InvalidArgument;
        }

        failoverUnitDescription = failoverUnitDescription_;
        replicaDescription = replicaDescription_;
    }

    ActivityDescription reasonActivityDescription(ActivityId(), ActivityType::Enum::ServiceReportFaultEvent);
    RAPEventSource::Events->ApiReportFault(
        failoverUnitDescription.FailoverUnitId.Guid, 
        replicaDescription.FederationNodeId.ToString(),
        replicaDescription.ReplicaId,
        replicaDescription.InstanceId,
        faultType,
        reasonActivityDescription);

    ReportFaultMessageBody msgBody(failoverUnitDescription, replicaDescription, faultType, reasonActivityDescription);
    Transport::MessageUPtr outMsg = RAMessage::GetReportFault().CreateMessage<ReportFaultMessageBody>(msgBody, msgBody.FailoverUnitDescription.FailoverUnitId.Guid);

    ProxyOutgoingMessageUPtr outgoingMessage = make_unique<ProxyOutgoingMessage>(std::move(outMsg), weak_ptr<FailoverUnitProxy>(shared_from_this()));

    reconfigurationAgentProxy_.SendMessageToRA(std::move(outgoingMessage));

    return ErrorCodeValue::Success;
}

ErrorCode FailoverUnitProxy::ReportHealth(
    ServiceModel::HealthReport && healthReport,
    ServiceModel::HealthReportSendOptionsUPtr && sendOptions)
{
    return reconfigurationAgentProxy_.ThrottledHealthReportingClientObj.AddReport(move(healthReport), move(sendOptions));
}

void FailoverUnitProxy::PublishEndpoint(Transport::MessageUPtr && outMsg)
{
    ProxyOutgoingMessageUPtr outgoingMessage = make_unique<ProxyOutgoingMessage>(std::move(outMsg), weak_ptr<FailoverUnitProxy>(shared_from_this()));

    reconfigurationAgentProxy_.SendMessageToRA(std::move(outgoingMessage));
}

ApiMonitoring::ApiCallDescriptionSPtr FailoverUnitProxy::CreateApiCallDescription(
    ApiMonitoring::ApiNameDescription && nameDescription,
    Reliability::ReplicaDescription const & replicaDescription,
    bool isHealthReportEnabled,
    bool traceServiceType)
{
    MonitoringData data(
        FailoverUnitId.Guid,
        replicaDescription.ReplicaId,
        replicaDescription.InstanceId,
        std::move(nameDescription),
        Stopwatch::Now());

    if (traceServiceType)
    {
        data.ServiceType = serviceDescription_.Type.ServiceTypeName;
    }

    auto slowTimeout = FailoverConfig::GetConfig().BuildReplicaTimeLimit;
    auto traceInterval = TimeSpan::MaxValue;
    if (data.Api.Api != ApiName::BuildReplica)
    {
        slowTimeout = isServiceAvailabilityImpacted_ ?
            FailoverConfig::GetConfig().ServiceReconfigurationApiHealthDuration :
            FailoverConfig::GetConfig().ServiceApiHealthDuration;
		traceInterval = FailoverConfig::GetConfig().PeriodicApiSlowTraceInterval;
    }

    MonitoringParameters parameters(isHealthReportEnabled, true, true, slowTimeout, traceInterval);

    return make_shared<ApiCallDescription>(move(data), parameters);
}

void FailoverUnitProxy::StartOperationMonitoring(
    ApiMonitoring::ApiCallDescriptionSPtr const & description)
{
    ASSERT_IF(state_ == FailoverUnitProxyStates::Closed, "FailoverUnitProxy::StartOperationMonitoring fup is closed");

    reconfigurationAgentProxy_.LocalHealthReportingComponentObj.StartMonitoring(description);
}

void FailoverUnitProxy::StopOperationMonitoring(
    ApiMonitoring::ApiCallDescriptionSPtr const & description,
    Common::ErrorCode const & error)
{
    ASSERT_IF(state_ == FailoverUnitProxyStates::Closed, "FailoverUnitProxy::StopOperationMonitoring fup is closed");
    reconfigurationAgentProxy_.LocalHealthReportingComponentObj.StopMonitoring(description, error);
}

bool FailoverUnitProxy::ShouldResendMessage(Transport::MessageUPtr const& message)
{
    Reliability::FailoverUnitDescription failoverUnitDescription;
    Reliability::ReplicaDescription replicaDescription;
    FailoverUnitProxyStates::Enum state;
    {
        AcquireExclusiveLock grab(lock_);
        if(state_ == FailoverUnitProxyStates::Closed)
        {
            return false;
        }

        failoverUnitDescription = failoverUnitDescription_;
        replicaDescription = replicaDescription_;
        state = state_;

        std::wstring action = message->Action;

        if (action == RAMessage::GetReportFault().Action)
        {
            ReportFaultMessageBody body;
            bool isBodyValid = message->GetBody(body);
            ASSERT_IF(!isBodyValid, "Since the message is locally generated and held in memory the deserialization can never result in an invalid state");

            bool shouldResend = body.LocalReplicaDescription.ReplicaId == replicaDescription.ReplicaId &&
                                body.LocalReplicaDescription.InstanceId == replicaDescription.InstanceId &&
                                state == FailoverUnitProxyStates::Opened;

            if (!shouldResend)
            {
                messageStage_ = ProxyMessageStage::None;
            }

            return shouldResend;
        }
        else if (action == RAMessage::GetReplicaEndpointUpdated().Action)
        {
            // Always resend if the proxy is opened
            // This is set to false once an ack is received
            return messageStage_ == ProxyMessageStage::RAProxyEndpointUpdated;
        }
        else if (action == RAMessage::GetReadWriteStatusRevokedNotification().Action)
        {
            return messageStage_ == ProxyMessageStage::RAProxyReadWriteStatusRevoked;
        }
        else
        {
            Assert::CodingError("FailoverUnitProxy::ShouldResendMessage action {0} is not supported", action);
        }
    }
}

ProxyOutgoingMessageUPtr FailoverUnitProxy::ComposeReadWriteStatusRevokedNotification(Common::AcquireExclusiveLock & )
{
    /*
        This is not enabled for volatile services as telling the fm that a volatile replica is down
        implies that the volatile replica is dropped and new placements are allowed on that node
    */
    if (!serviceDescription_.HasPersistedState || !FailoverConfig::GetConfig().EnableReplicaDownOnReadWriteStatusRevoke)
    {
        return ProxyOutgoingMessageUPtr();
    }

    if (messageStage_ == ProxyMessageStage::RAProxyReadWriteStatusRevoked)
    {
        return ProxyOutgoingMessageUPtr();
    }

    messageStage_ = ProxyMessageStage::RAProxyReadWriteStatusRevoked;
    
    ReplicaMessageBody msgBody(failoverUnitDescription_, replicaDescription_, serviceDescription_);

    auto outMsg = RAMessage::GetReadWriteStatusRevokedNotification().CreateMessage<ReplicaMessageBody>(msgBody, msgBody.FailoverUnitDescription.FailoverUnitId.Guid);

    return make_unique<ProxyOutgoingMessage>(std::move(outMsg), weak_ptr<FailoverUnitProxy>(shared_from_this()));
}

void FailoverUnitProxy::SendReadWriteStatusRevokedNotification(ProxyOutgoingMessageUPtr && msg)
{
    if (msg == nullptr)
    {
        return;
    }

    reconfigurationAgentProxy_.SendMessageToRA(std::move(msg));
}

void FailoverUnitProxy::TraceBeforeOperation(
    Common::AcquireExclusiveLock &,
    ApiMonitoring::InterfaceName::Enum iface,
    ApiMonitoring::ApiName::Enum api) const
{
    RAPEventSource::Events->FUPPreTrace(
        FailoverUnitId.Guid,
        reconfigurationAgentProxy_.Id,
        iface,
        api,
        *this);
}

void FailoverUnitProxy::TraceBeforeOperation(
    Common::AcquireExclusiveLock &,
    ApiMonitoring::InterfaceName::Enum iface,
    ApiMonitoring::ApiName::Enum api,
    Common::TraceCorrelatedEventBase const & input) const
{
    RAPEventSource::Events->FUPPreTraceWithData(
        FailoverUnitId.Guid,
        reconfigurationAgentProxy_.Id,
        iface,
        api,
        input,
        *this);
}

void FailoverUnitProxy::TransitionReplicatorToClosed(Common::AcquireExclusiveLock &)
{
    currentReplicatorRole_ = ReplicaRole::Unknown;
    replicatorState_ = ReplicatorStates::Closed;
}

void FailoverUnitProxy::TransitionServiceToClosed(Common::AcquireExclusiveLock &)
{
    replicaState_ = FailoverUnitProxyStates::Closed;
    currentServiceRole_ = ReplicaRole::Unknown;
}

ErrorCode FailoverUnitProxy::IsQueryAllowed(bool& isReplicatorQueryRequired) const
{
    isReplicatorQueryRequired = false;

    if (IsClosed)
    {
        // Object is closed, cannot query
        return ErrorCodeValue::ObjectClosed;
    }

    if (IsStatefulService)
    {
        // Stateful service checks
        
        if (!statefulServicePartition_ || !replicatorOperationManager_)
        {
            // If the service/replicator objects have not been created yet, cannot query
            return ErrorCodeValue::NotReady;        
        }
        
        isReplicatorQueryRequired = DoesReplicatorSupportQueries;
    }
    else
    {
        // Stateless service checks

        if (!statelessServicePartition_)
        {
            // If the service object has not been created yet, cannot query
            return ErrorCodeValue::NotReady;        
        }
    }
    
    return ErrorCodeValue::Success;
}

DeployedServiceReplicaDetailQueryResult FailoverUnitProxy::GetQueryResult() const
{
    FABRIC_QUERY_SERVICE_OPERATION_NAME serviceOperationName = FABRIC_QUERY_SERVICE_OPERATION_NAME_INVALID;
    DateTime serviceOperationStartTime = DateTime::Zero;

    // Read properties common for SL/SV
    if (serviceOperationManager_)
    {
        serviceOperationManager_->GetQueryInformation(serviceOperationName, serviceOperationStartTime);
    }

    auto loads = reportedLoadStore_.GetForQuery();

    wstring serviceName = ServiceModel::SystemServiceApplicationNameHelper::GetPublicServiceName(serviceDescription_.Name);

    if (IsStatefulService)
    {
        FABRIC_QUERY_REPLICATOR_OPERATION_NAME replicatorOperationName = FABRIC_QUERY_REPLICATOR_OPERATION_NAME_INVALID;
        if (replicatorOperationManager_)
        {
            replicatorOperationName = replicatorOperationManager_->GetNameForQuery();
        }

        ASSERT_IFNOT(statefulServicePartition_, "FailoverUnitProxy::GetQueryResult, stateful partition object is null"); 

        auto readStatus = statefulServicePartition_->GetReadStatusForQuery();
        auto writeStatus = statefulServicePartition_->GetWriteStatusForQuery();

        auto queryResult = DeployedServiceReplicaDetailQueryResult(
            serviceName,
            FailoverUnitId.Guid,
            replicaDescription_.ReplicaId,
            serviceOperationName,
            serviceOperationStartTime,
            replicatorOperationName,
            readStatus,
            writeStatus,
            move(loads));

        ReplicaStatusQueryResultSPtr replicaQueryResult;
        auto error = this->ReplicaGetQuery(replicaQueryResult);
        if (error.IsSuccess())
        {
            queryResult.SetReplicaStatusQueryResult(move(replicaQueryResult));
        }

        return queryResult;
    }
    else
    {
        ASSERT_IFNOT(statelessServicePartition_, "FailoverUnitProxy::GetQueryResult, stateless partition object is null"); 
    
        return DeployedServiceReplicaDetailQueryResult(
            serviceName,
            FailoverUnitId.Guid,
            replicaDescription_.ReplicaId,
            serviceOperationName,
            serviceOperationStartTime,
            move(loads));
    }
}

void FailoverUnitProxy::ReportedLoadStore::AddLoad(vector<LoadBalancingComponent::LoadMetric> const & loads)
{
    auto now = DateTime::Now();

    for(auto it = loads.cbegin(); it != loads.cend(); ++it)
    {
        Data data(it->Value, now);
        loads_[it->Name] = data;
    }
}

void FailoverUnitProxy::ReportedLoadStore::OnFTChangeRole()
{
    // PLB will no longer consider the reports from the old role
    Clear();
}

void FailoverUnitProxy::ReportedLoadStore::OnFTOpen()
{
    Clear();
}

void FailoverUnitProxy::ReportedLoadStore::Clear()
{
    loads_.clear();
}

vector<LoadMetricReport> FailoverUnitProxy::ReportedLoadStore::GetForQuery() const
{
    vector<LoadMetricReport> rv;

    for(auto it = loads_.cbegin(); it != loads_.cend(); ++it)
    {
        rv.push_back(it->second.ToLoadValue(it->first));
    }

    return rv;
}

FailoverUnitProxy::ReportedLoadStore::Data::Data(uint value, Common::DateTime timeStamp)
: value_(value),
  timeStamp_(timeStamp)
{
}

FailoverUnitProxy::ReportedLoadStore::Data::Data()
: value_(0),
  timeStamp_(DateTime::Zero)
{
}

LoadMetricReport FailoverUnitProxy::ReportedLoadStore::Data::ToLoadValue(wstring const & metricName) const
{
    return LoadMetricReport(metricName, value_, value_, timeStamp_);
}
