// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;
using namespace Management::ClusterManager;
using namespace Management::ImageModel;

StringLiteral const TraceComponent("RolloutManager");

RolloutManager::RolloutManager(
    ClusterManagerReplica const & replica)
    : PartitionedReplicaTraceComponent(replica.PartitionedReplicaId)
    , recoveryTimer_()
    , timerLock_()
    , replicaSPtr_(replica.CreateComponentRoot())
    , replica_(replica)
    , state_(RolloutManagerState::NotActive)
    , stateLock_()
    , activeContexts_()
    , contextsLock_()
{
    WriteInfo(
        TraceComponent, 
        "{0} ctor: this = {1}",
        this->TraceId,
        static_cast<void*>(this));
    perfCounters_ = ClusterManagerPerformanceCounters::CreateInstance(wformatString("{0}:{1}",
        this->PartitionedReplicaId,
        DateTime::Now().Ticks));
}

RolloutManager::~RolloutManager()
{
    WriteInfo(
        TraceComponent, 
        "{0} ~dtor: this = {1}",
        this->TraceId,
        static_cast<void*>(this));
}

Common::ErrorCode RolloutManager::Start()
{
    AcquireWriteLock lock(stateLock_);

    CMEvents::Trace->RolloutManagerStart(
        this->PartitionedReplicaId,
        state_);

    switch (state_)
    {
        case RolloutManagerState::NotActive:
        case RolloutManagerState::Active:

            state_ = RolloutManagerState::Recovery;

            this->SchedulePendingContextCheck(TimeSpan::Zero);

            return ErrorCodeValue::Success;

        case RolloutManagerState::Recovery:

            return ErrorCodeValue::Success;

        case RolloutManagerState::Closed:

            return ErrorCodeValue::ObjectClosed;

        default:

            Assert::TestAssert("{0} invalid state in Start(): {1}", this->TraceId, state_);

            state_ = RolloutManagerState::Closed;

            return ErrorCodeValue::ObjectClosed;
    }
}

void RolloutManager::SchedulePendingContextCheck(TimeSpan const delay)
{
    AcquireExclusiveLock lock(timerLock_);

    auto root = replica_.CreateAsyncOperationRoot();

    // Handle duplicate Start() or rapid Stop()/Start() calls
    //
    if (recoveryTimer_) { recoveryTimer_->Cancel(); }

    recoveryTimer_ = Timer::Create(TimerTagDefault, [this, root](TimerSPtr const & timer)
        {
            timer->Cancel();
            this->PendingContextCheckCallback();
        },
        true); // allow concurrency
    recoveryTimer_->Change(delay);
}

void RolloutManager::PendingContextCheckCallback()
{
    size_t pendingContextCount = 0;
    {
        AcquireExclusiveLock lock(contextsLock_);

        pendingContextCount = activeContexts_.size();
    }

    // At this point, the recovery state will prevent new requests from being accepted
    // so we can wait for existing requests to complete before starting a new recovery
    //
    if (pendingContextCount > 0)
    {
        TimeSpan retryDelay = replica_.GetRandomizedOperationRetryDelay(ErrorCodeValue::Success);

        WriteInfo(
            TraceComponent, 
            "{0} recovery waiting for {1} pending contexts: delay = {2}",
            this->TraceId,
            pendingContextCount,
            retryDelay);

        this->SchedulePendingContextCheck(retryDelay);
    }
    else
    {
        this->RecoveryCallback();
    }
}

void RolloutManager::ScheduleRecovery(ErrorCode const & error)
{
    TimeSpan delay = replica_.GetRandomizedOperationRetryDelay(error);

    WriteInfo(
        TraceComponent, 
        "{0} retry recovery due to {1}: delay = {2}",
        this->TraceId, 
        error,
        delay);

    AcquireExclusiveLock lock(timerLock_);

    auto root = replica_.CreateAsyncOperationRoot();
    recoveryTimer_ = Timer::Create(TimerTagDefault, [this, root](TimerSPtr const & timer)
        {
            timer->Cancel();
            this->RecoveryCallback();
        },
        true); // allow concurrency
    recoveryTimer_->Change(delay);
}

// This function will retry recovery of pending contexts indefinitely (or until stopped) on errors. 
// Since the store layer will return reconfiguration-related errors until OnChangeRole()
// completes, this function cannot be called on the OnChangeRole() thread or
// that thread will deadlock.
//
void RolloutManager::RecoveryCallback()
{
    {
        AcquireWriteLock lock(stateLock_);

        CMEvents::Trace->RolloutManagerRecovery(
            this->PartitionedReplicaId,
            state_);

        switch (state_)
        {
            case RolloutManagerState::Recovery:
                // proceed with recovery

                break;

            case RolloutManagerState::NotActive:
            case RolloutManagerState::Closed:
                // cancel recovery
                
                return;

            default:
                Assert::TestAssert("{0} invalid state in RecoveryCallback(): {1}", this->TraceId, state_);

                state_ = RolloutManagerState::Closed;

                return;
        }   // end switch (state)
    }   // end state lock

    size_t appTypeCount = 0; 
    size_t appCount = 0; 
    size_t serviceCount = 0;
    size_t applicationUpgradeCount = 0;
    size_t fabricProvisionCount = 0;
    size_t fabricUpgradeCount = 0;
    size_t infraTaskCount = 0;
    size_t composeDeploymentCount = 0;
    size_t composeUpgradeCount = 0;
    size_t singleInstanceDeploymentCount = 0;
    size_t singleInstanceDeploymentUpgradeCount = 0;

    auto storeTx = StoreTransaction::Create(replica_.ReplicatedStore, replica_.PartitionedReplicaId);

    auto activityId = storeTx.ActivityId.GetNestedActivity();

    vector<ApplicationTypeContext> allAppTypeContexts;
    ErrorCode error = this->RecoverRolloutContexts<ApplicationTypeContext>(
        storeTx,
        Constants::StoreType_ApplicationTypeContext,
        activityId,
        appTypeCount,
        allAppTypeContexts);

    vector<ApplicationContext> allApplicationContexts;
    if (error.IsSuccess())
    {
        error = this->RecoverRolloutContexts<ApplicationContext>(
            storeTx, 
            Constants::StoreType_ApplicationContext, 
            activityId,
            appCount,
            allApplicationContexts);
    }
    
    if (error.IsSuccess())
    {
        vector<ServiceContext> allServiceContexts;

        error = this->RecoverRolloutContexts<ServiceContext>(
            storeTx, 
            Constants::StoreType_ServiceContext, 
            activityId,
            serviceCount,
            allServiceContexts);
    }

    if (error.IsSuccess())
    {
        error = this->RecoverRolloutContexts<ApplicationUpgradeContext>(
            storeTx, 
            Constants::StoreType_ApplicationUpgradeContext, 
            activityId,
            applicationUpgradeCount);
    }

    if (error.IsSuccess())
    {
        error = this->RecoverRolloutContexts<FabricProvisionContext>(
            storeTx, 
            Constants::StoreType_FabricProvisionContext, 
            activityId,
            fabricProvisionCount);
    }

    if (error.IsSuccess())
    {
        error = this->RecoverRolloutContexts<FabricUpgradeContext>(
            storeTx, 
            Constants::StoreType_FabricUpgradeContext, 
            activityId,
            fabricUpgradeCount);
    }

    if (error.IsSuccess())
    {
        error = this->RecoverRolloutContexts<InfrastructureTaskContext>(
            storeTx, 
            Constants::StoreType_InfrastructureTaskContext, 
            activityId,
            infraTaskCount);
    }

    if (error.IsSuccess())
    {
        error = this->RecoverRolloutContexts<ComposeDeploymentContext>(
            storeTx, 
            Constants::StoreType_ComposeDeploymentContext, 
            activityId,
            composeDeploymentCount);
    }

    if (error.IsSuccess())
    {
        error = this->RecoverRolloutContexts<ComposeDeploymentUpgradeContext>(
            storeTx, 
            Constants::StoreType_ComposeDeploymentUpgradeContext, 
            activityId,
            composeUpgradeCount);
    }

    if (error.IsSuccess())
    {
        error = this->RecoverRolloutContexts<SingleInstanceDeploymentContext>(
            storeTx, 
            Constants::StoreType_SingleInstanceDeploymentContext, 
            activityId,
            singleInstanceDeploymentCount);
    }

    if (error.IsSuccess())
    {
        error = this->RecoverRolloutContexts<SingleInstanceDeploymentUpgradeContext>(
            storeTx, 
            Constants::StoreType_SingleInstanceDeploymentUpgradeContext, 
            activityId,
            singleInstanceDeploymentUpgradeCount);
    }

    if (error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} recovered pending contexts [AppProv = {1}, Apps = {2}, Svcs = {3}, AppUpgrades = {4} FabProv = {5} FabUpgrades = {6} InfraTasks = {7} composeDeployments = {8} composeUpgrades = {9} singleInstances = {10} singleInstanceUpgrades = {11}]", 
            storeTx.ReplicaActivityId, 
            appTypeCount,
            appCount,
            serviceCount,
            applicationUpgradeCount,
            fabricProvisionCount,
            fabricUpgradeCount,
            infraTaskCount,
            composeDeploymentCount,
            composeUpgradeCount,
            singleInstanceDeploymentCount,
            singleInstanceDeploymentUpgradeCount);

        storeTx.CommitReadOnly();

        auto nextActivityId = activityId.GetNestedActivity();

        auto operation = AsyncOperation::CreateAndStart<MigrateDataAsyncOperation>(
            *this,
            nextActivityId,
            move(allAppTypeContexts),
            move(allApplicationContexts),
            ManagementConfig::GetConfig().MaxCommunicationTimeout,
            [this, nextActivityId](AsyncOperationSPtr const & operation) { this->OnMigrateDataComplete(nextActivityId, operation, false); },
            replica_.CreateAsyncOperationRoot());
        this->OnMigrateDataComplete(nextActivityId, operation, true);
    }
    else
    {
        storeTx.Rollback();

        this->ScheduleRecovery(error);
    }
}

void RolloutManager::OnMigrateDataComplete(
    ActivityId const & activityId,
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = MigrateDataAsyncOperation::End(operation);

    if (error.IsSuccess())
    {
        auto inner = AsyncOperation::CreateAndStart<ClusterBaselineAsyncOperation>(
            *this,
            activityId.GetNestedActivity(),
            ManagementConfig::GetConfig().AutomaticBaselineTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnBaselineComplete(operation, false); },
            operation->Parent);
        this->OnBaselineComplete(inner, true);
    }
    else
    {
        this->ScheduleRecovery(error);
    }
}

void RolloutManager::OnBaselineComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = ClusterBaselineAsyncOperation::End(operation);

    if (error.IsSuccess())
    {
        this->RecoveryComplete();
    }
    else
    {
        this->ScheduleRecovery(error);
    }
}

void RolloutManager::RecoveryComplete()
{
    AcquireWriteLock lock(stateLock_);

    switch (state_)
    {
        case RolloutManagerState::Recovery:

            CMEvents::Trace->RolloutManagerActive(
                this->PartitionedReplicaId);

            state_ = RolloutManagerState::Active;

            break;

        case RolloutManagerState::NotActive:
        case RolloutManagerState::Closed:

            WriteInfo(
                TraceComponent, 
                "{0} canceled recovery ({1})",
                this->TraceId, 
                state_);

            break;

        default:
            Assert::TestAssert("{0} invalid RolloutManager state {1}", this->TraceId, state_); 

            state_ = RolloutManagerState::Closed;

            break;
    };
}

void RolloutManager::Stop()
{
    AcquireWriteLock lock(stateLock_);

    if (state_ != RolloutManagerState::Closed)
    {
        state_ = RolloutManagerState::NotActive;
    }
}

void RolloutManager::Close()
{
    {
        AcquireWriteLock lock(stateLock_);

        state_ = RolloutManagerState::Closed;
    }

    // Pending contexts will keep the replica alive
    replicaSPtr_.reset();
}

ErrorCode RolloutManager::Enqueue(std::shared_ptr<RolloutContext> && context)
{
    shared_ptr<RolloutContext> activeContext;
    {
        AcquireExclusiveLock lock(contextsLock_);

        if (!this->IsActive)
        {
            WriteInfo(
                TraceComponent,
                "{0} not active - skipping enqueue",
                this->TraceId);

            return ErrorCodeValue::NotPrimary;
        }

        if (this->TryAddActiveContextCallerHoldsLock(context, activeContext))
        {
            // Currently track failure timeout for all rollout context, but only fail applicationcontext & servicecontext
            activeContext->StartOperationRetryStopwatch();

            this->PostRolloutContextProcessing(*activeContext);

            return ErrorCodeValue::Success;
        }
    }

    if (context->IsConvertToForceDelete)
    {
        activeContext->ClientRequest = move(context->ClientRequest);
    }

    // Allow unprovision to interrupt and cancel a pending provision (if any)
    //
    if (context->ContextType == RolloutContextType::ApplicationType && context->IsDeleting)
    {
        auto casted = dynamic_cast<ApplicationTypeContext*>(activeContext.get());
        ASSERT_IF(casted == nullptr, "Failed to cast ApplicationTypeContext");
        this->Replica.AppTypeRequestTracker.TryCancel(casted->TypeName);
    }

    return ErrorCodeValue::CMRequestAlreadyProcessing;
}

void RolloutManager::ExternallyFailUpgrade(RolloutContext const & context)
{
    AcquireExclusiveLock lock(contextsLock_);

    auto const & mapKey = context.MapKey;

    auto iter = activeContexts_.find(mapKey);
    if (iter != activeContexts_.end())
    {
        iter->second->ExternalFail();
    }
}

void RolloutManager::PostRolloutContextProcessing(__in RolloutContext & context)
{
    auto root = replica_.CreateComponentRoot();
    context.SetRetryTimer([this, root, &context]() { this->ProcessRolloutContext(context); }, replica_.ContextProcessingDelay);
}

void RolloutManager::ScheduleRolloutContextProcessing(__in RolloutContext & context, TimeSpan const retryDelay)
{
    if (!this->IsActive)
    {
        WriteInfo(
            TraceComponent, 
            "{0} rollout manager no longer active",
            this->TraceId);

        this->RemoveActiveContext(context);
    }
    else
    {
        auto root = replica_.CreateComponentRoot();
        context.SetRetryTimer([this, root, &context]() { this->ProcessRolloutContext(context); }, retryDelay);
    }
}

void RolloutManager::ProcessRolloutContext(__in RolloutContext & context)
{
    // Root the hierarchy long enough for any async operations to start
    auto snap = replicaSPtr_;
    if (snap)
    {
        WriteNoise(
            TraceComponent,
            "{0} processing: {1} context timeout = {2}",
            context.TraceId,
            context,
            context.OperationTimeout);
    }
    else
    {
        WriteNoise(
            TraceComponent,
            "{0} replica closed - cancel processing of: {1}",
            context.TraceId,
            context);

        return;
    }

    switch(context.Status)
    {
        case RolloutStatus::Unknown:
            ProcessUnknown(context);
            break;

        case RolloutStatus::Pending:
        {
            switch(context.ContextType)
            {
                case RolloutContextType::ApplicationType:
                {
                    auto appTypeContext = dynamic_cast<ApplicationTypeContext*>(&context);
                    if (appTypeContext->ApplicationTypeDefinitionKind == ApplicationTypeDefinitionKind::Enum::ServiceFabricApplicationPackage)
                    {
                        ProcessPendingApplicationType(*appTypeContext);
                    }
                    else
                    {
                        ProcessCompleted(*appTypeContext);
                    }
                    break;
                }

                case RolloutContextType::Application:
                {
                    auto applicationContext = dynamic_cast<ApplicationContext*>(&context);
                    ASSERT_IF(applicationContext == nullptr, "Failed to cast ApplicationContext");

                    if (applicationContext->IsUpgrading || applicationContext->ApplicationDefinitionKind != ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
                    {
                        // The recovered corresponding ApplicationUpgradeContext will run the upgrade logic
                        // so no-op the recovered ApplicationContext
                        //
                        ProcessCompleted(context);
                    }
                    else
                    {
                        ProcessPendingApplication(*applicationContext);
                    }
                    break;
                }

                case RolloutContextType::ComposeDeployment:
                {
                    auto containerApplicationContext = dynamic_cast<ComposeDeploymentContext *>(&context);
                    ASSERT_IF(containerApplicationContext == nullptr, "Failed to cast ComposeDeploymentContext");
                    
                    if (containerApplicationContext->IsUpgrading)
                    {
                        // The recovered corresponding ApplicationUpgradeContext will run the upgrade logic
                        // so no-op the recovered ApplicationContext
                        //
                        ProcessCompleted(*containerApplicationContext);
                    }
                    else
                    {
                        ProcessPendingComposeDeployment(*containerApplicationContext);
                    }
                    break;
                }

                case RolloutContextType::SingleInstanceDeployment:
                {
                    auto singleInstanceDeploymentContext = dynamic_cast<SingleInstanceDeploymentContext *>(&context);
                    if (singleInstanceDeploymentContext->IsUpgrading)
                    {
                        ProcessCompleted(*singleInstanceDeploymentContext);
                    }
                    else
                    {
                        ProcessPendingSingleInstanceDeployment(*singleInstanceDeploymentContext);
                    }
                    break;
                }

                case RolloutContextType::ComposeDeploymentUpgrade:
                {
                    auto composeDeploymentUpgradeContext = dynamic_cast<ComposeDeploymentUpgradeContext *>(&context);
                    ASSERT_IF(composeDeploymentUpgradeContext == nullptr, "Failed to cast ComposeDeploymentUpgradeContext");
                    ProcessPendingComposeDeploymentUpgrade(*composeDeploymentUpgradeContext);
                    break;
                }

                case RolloutContextType::SingleInstanceDeploymentUpgrade:
                {
                    auto singleInstanceDeploymentUpgradeContext = dynamic_cast<SingleInstanceDeploymentUpgradeContext *>(&context);
                    ProcessPendingSingleInstanceDeploymentUpgrade(*singleInstanceDeploymentUpgradeContext);
                    break;
                }

                case RolloutContextType::Service:
                {
                    auto serviceContext = dynamic_cast<ServiceContext*>(&context);
                    ASSERT_IF(serviceContext == nullptr, "Failed to cast ServiceContext");
                    ProcessPendingService(*serviceContext);
                    break;
                }
                    
                case RolloutContextType::ApplicationUpgrade:
                {
                    auto upgradeContext = dynamic_cast<ApplicationUpgradeContext*>(&context);
                    ASSERT_IF(upgradeContext == nullptr, "Failed to cast ApplicationUpgradeContext");
                    if (upgradeContext->ApplicationDefinitionKind == ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
                    {
                        //
                        // Only application upgrades for Service fabric application description are driven via the upgrade context
                        //
                        ProcessPendingApplicationUpgrade(*upgradeContext);
                    }
                    else
                    {
                        ProcessCompleted(*upgradeContext);
                    }
                    break;
                }

                case RolloutContextType::ApplicationUpdate:
                {
                    auto applicationUpdateContext = dynamic_cast<ApplicationUpdateContext*>(&context);
                    ASSERT_IF(applicationUpdateContext == nullptr, "Failed to cast ApplicationUpdateContext");
                    ProcessPendingApplicationUpdate(*applicationUpdateContext, false);
                    break;
                }

                case RolloutContextType::FabricProvision:
                {
                    auto provisionContext = dynamic_cast<FabricProvisionContext*>(&context);
                    ASSERT_IF(provisionContext == nullptr, "Failed to cast FabricProvisionContext");
                    ProcessPendingFabricProvision(*provisionContext);
                    break;
                }

                case RolloutContextType::FabricUpgrade:
                {
                    auto upgradeContext = dynamic_cast<FabricUpgradeContext*>(&context);
                    ASSERT_IF(upgradeContext == nullptr, "Failed to cast FabricUpgradeContext");
                    ProcessPendingFabricUpgrade(*upgradeContext);
                    break;
                }

                case RolloutContextType::InfrastructureTask:
                {
                    auto infraTaskContext = dynamic_cast<InfrastructureTaskContext*>(&context);
                    ASSERT_IF(infraTaskContext == nullptr, "Failed to cast InfrastructureTaskContext");
                    ProcessPendingInfrastructureTask(*infraTaskContext);
                    break;
                }

                default:
                    this->OnUnknownContextType(context);
                    break;
            } // switch context type
            break;
        } // case status pending

        case RolloutStatus::Completed:
            ProcessCompleted(context);
            break;

        case RolloutStatus::Failed:

            switch(context.ContextType)
            {
                // Failed create: rollback by deleting context
                // Failed delete: continue trying to delete context

                case RolloutContextType::ApplicationType:
                {
                    auto castedContext = dynamic_cast<ApplicationTypeContext*>(&context);
                    ASSERT_IF(castedContext == nullptr, "Failed to cast ApplicationTypeContext");
                    if (castedContext->IsAsync)
                    {
                        // No-op: Leave the context in the failed state for 
                        // status querying. The failed status was already persisted 
                        // by ProcessApplicationTypeContextAsyncOperation.
                        //
                        this->RemoveActiveContext(context);

                        break;
                    }

                    /// FALL THROUGH to ProcessFailed()
                }
                case RolloutContextType::Application:
                case RolloutContextType::Service:
                    ProcessFailed(context, RolloutStatus::DeletePending);
                    break;

                case RolloutContextType::ComposeDeployment:
                {
                    auto dockerComposeDeploymentContext = dynamic_cast<ComposeDeploymentContext*>(&context);
                    ASSERT_IF(dockerComposeDeploymentContext == nullptr, "Failed to cast ComposeDeploymentContext");
                    ProcessFailedComposeDeployment(*dockerComposeDeploymentContext);
                    break;
                }

                case RolloutContextType::ComposeDeploymentUpgrade:
                {
                    auto composeDeploymentUpgradeContext = dynamic_cast<ComposeDeploymentUpgradeContext *>(&context);
                    ASSERT_IF(composeDeploymentUpgradeContext == nullptr, "Failed to cast ComposeDeploymentUpgradeContext");
                    ClearPendingComposeDeploymentUpgrade(*composeDeploymentUpgradeContext);
                    break;
                }

                case RolloutContextType::SingleInstanceDeployment:
                {
                    auto singleInstanceDeploymentContext = dynamic_cast<SingleInstanceDeploymentContext*>(&context);
                    ProcessFailedSingleInstanceDeployment(*singleInstanceDeploymentContext);
                    break;
                }

                case RolloutContextType::SingleInstanceDeploymentUpgrade:
                {
                    auto singleInstanceDeploymentUpgradeContext = dynamic_cast<SingleInstanceDeploymentUpgradeContext*>(&context);
                    ASSERT_IF(singleInstanceDeploymentUpgradeContext == nullptr, "Failed to cast SingleInstanceDeploymentUpgradeContext");
                    ClearPendingSingleInstanceDeploymentUpgrade(*singleInstanceDeploymentUpgradeContext);
                    break;
                }

                // Failed application update: perform an update with old values
                case RolloutContextType::ApplicationUpdate:
                {
                    auto applicationUpdateContext = dynamic_cast<ApplicationUpdateContext*>(&context);
                    ASSERT_IF(applicationUpdateContext == nullptr, "Failed to cast ApplicationUpdateContext");
                    ProcessPendingApplicationUpdate(*applicationUpdateContext, true);
                    break;
                }

                // Failed application upgrade: clear application upgrading status 
                //
                case RolloutContextType::ApplicationUpgrade:
                {
                    auto upgradeContext = dynamic_cast<ApplicationUpgradeContext*>(&context);
                    ASSERT_IF(upgradeContext == nullptr, "Failed to cast ApplicationUpgradeContext");
                    if (upgradeContext->ApplicationDefinitionKind == ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
                    {
                        ClearPendingApplicationUpgrade(*upgradeContext);
                        break;
                    }
                    else
                    {
                        //
                        // Intentional fall-through: cleanup will happen via the deployment context.
                        //
                        this->RemoveActiveContext(context);
                    }
                }

                // Failed Fabric operations should not be deleted since they
                // contain some state about the last successful Fabric operation
                // and can be "restarted" by a new client request.
                //
                // Failed infrastructure tasks can also be restarted.
                //
                case RolloutContextType::FabricProvision:
                case RolloutContextType::FabricUpgrade:
                case RolloutContextType::InfrastructureTask:
                    ProcessFailed(context, RolloutStatus::Failed);
                    break;

                default:
                    this->OnUnknownContextType(context);
                    break;
            } // switch context type
            break;

        case RolloutStatus::DeletePending:
        {
            switch(context.ContextType)
            {
                case RolloutContextType::ApplicationType:
                {
                    auto appTypeContext = dynamic_cast<ApplicationTypeContext*>(&context);
                    ASSERT_IF(appTypeContext == nullptr, "Failed to cast ApplicationTypeContext");
                    DeletePendingApplicationType(*appTypeContext);
                    break;
                }

                case RolloutContextType::Application:
                {
                    auto applicationContext = dynamic_cast<ApplicationContext*>(&context);
                    ASSERT_IF(applicationContext == nullptr, "Failed to cast ApplicationContext");
                    DeletePendingApplication(*applicationContext);
                    break;
                }

                case RolloutContextType::Service:
                {
                    auto serviceContext = dynamic_cast<ServiceContext*>(&context);
                    ASSERT_IF(serviceContext == nullptr, "Failed to cast ServiceContext");
                    DeletePendingService(*serviceContext);
                    break;
                }

                case RolloutContextType::ComposeDeployment:
                {
                    auto composeDeploymentContext = dynamic_cast<ComposeDeploymentContext*>(&context);
                    ASSERT_IF(composeDeploymentContext == nullptr, "Failed to cast ComposeDeploymentContext");
                    DeletePendingComposeDeployment(*composeDeploymentContext);
                    break;
                }

                case RolloutContextType::SingleInstanceDeployment:
                {
                    auto singleInstanceDeploymentContext = dynamic_cast<SingleInstanceDeploymentContext*>(&context);
                    DeletePendingSingleInstanceDeployment(*singleInstanceDeploymentContext);
                    break;
                }
                default:
                    this->OnUnknownContextType(context);
                    break;
            } // switch context type
            break;
        } // case status deleting

        case RolloutStatus::Replacing:
        {
            switch(context.ContextType)
            {
                case RolloutContextType::SingleInstanceDeployment:
                {
                    auto singleInstanceDeploymentContext = dynamic_cast<SingleInstanceDeploymentContext *>(&context);
                    ProcessReplacingSingleInstanceDeployment(*singleInstanceDeploymentContext);
                    break;
                }
            }
        }

        default:
            Assert::TestAssert("{0}: Unknown context status {1}", context.Status);
    }
}

void RolloutManager::FinishProcessingRolloutContext(
    __in RolloutContext & context, 
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = ProcessRolloutContextAsyncOperationBase::End(operation);

    if (error.IsSuccess())
    {
        ProcessCompleted(context);
    }
    else
    {
        OnProcessingError(context, error);
    }
}

void RolloutManager::ProcessPendingApplicationType(__in ApplicationTypeContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessApplicationTypeContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ProcessPendingApplication(__in ApplicationContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessApplicationContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ProcessPendingComposeDeployment(__in ComposeDeploymentContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessComposeDeploymentContextAsyncOperation>(
        *this,
        context,
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ProcessPendingSingleInstanceDeployment(__in SingleInstanceDeploymentContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessSingleInstanceDeploymentContextAsyncOperation>(
        *this,
        context,
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ProcessPendingSingleInstanceDeploymentUpgrade(__in SingleInstanceDeploymentUpgradeContext &context)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation>(
        *this,
        context,
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ProcessPendingComposeDeploymentUpgrade(__in ComposeDeploymentUpgradeContext &context)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessComposeDeploymentUpgradeAsyncOperation>(
        *this,
        context,
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ProcessPendingApplicationUpdate(__in ApplicationUpdateContext & context, bool failed)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessApplicationContextUpdateAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot(),
        !failed);   // If !failed - we're rolling forward

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ProcessPendingService(__in ServiceContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessServiceContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ProcessPendingApplicationUpgrade(__in ApplicationUpgradeContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessApplicationUpgradeContextAsyncOperation>(
        *this,
        context,
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ProcessPendingFabricProvision(__in FabricProvisionContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessFabricProvisionContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ProcessPendingFabricUpgrade(__in FabricUpgradeContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessFabricUpgradeContextAsyncOperation>(
        *this,
        context,
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ProcessPendingInfrastructureTask(__in InfrastructureTaskContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessInfrastructureTaskContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::DeletePendingApplicationType(__in ApplicationTypeContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<DeleteApplicationTypeContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::DeletePendingApplication(__in ApplicationContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<DeleteApplicationContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::DeletePendingComposeDeployment(__in ComposeDeploymentContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<DeleteComposeDeploymentContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::DeletePendingSingleInstanceDeployment(__in SingleInstanceDeploymentContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<DeleteSingleInstanceDeploymentContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::DeletePendingService(__in ServiceContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<DeleteServiceContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ClearPendingApplicationUpgrade(__in ApplicationUpgradeContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<ClearApplicationUpgradeContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ClearPendingComposeDeploymentUpgrade(__in ComposeDeploymentUpgradeContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<ClearComposeDeploymentUpgradeContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ClearPendingSingleInstanceDeploymentUpgrade(__in SingleInstanceDeploymentUpgradeContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<ClearSingleInstanceDeploymentUpgradeContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());
    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ProcessFailedComposeDeployment(__in ComposeDeploymentContext &context)
{
    //
    // The delete operation uses the context state - Failed vs DeletePending to distinguish cleanup(rollback) vs actual delete
    //
    auto operation = AsyncOperation::CreateAndStart<DeleteComposeDeploymentContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->OnProcessFailedComposeDeploymentComplete(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->OnProcessFailedComposeDeploymentComplete(context, operation, true);
}

void RolloutManager::OnProcessFailedComposeDeploymentComplete(
    RolloutContext & context, 
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = ProcessRolloutContextAsyncOperationBase::End(operation);

    if (error.IsSuccess())
    {
        this->RemoveActiveContext(context);
    }
    else
    {
        //
        // schedule retry.
        //
        TimeSpan retryDelay = replica_.GetRandomizedOperationRetryDelay(error);
        ScheduleRolloutContextProcessing(context, retryDelay);
    }
}

void RolloutManager::ProcessFailedSingleInstanceDeployment(__in SingleInstanceDeploymentContext &context)
{
    //
    // The delete operation uses the context state - Failed vs DeletePending to distinguish cleanup(rollback) vs actual delete
    //
    auto operation = AsyncOperation::CreateAndStart<DeleteSingleInstanceDeploymentContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->OnProcessFailedSingleInstanceDeploymentComplete(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->OnProcessFailedSingleInstanceDeploymentComplete(context, operation, true);
    context;
}

void RolloutManager::OnProcessFailedSingleInstanceDeploymentComplete(
    RolloutContext & context, 
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = ProcessRolloutContextAsyncOperationBase::End(operation);

    if (error.IsSuccess())
    {
        this->RemoveActiveContext(context);
    }
    else
    {
        //
        // schedule retry.
        //
        TimeSpan retryDelay = replica_.GetRandomizedOperationRetryDelay(error);
        ScheduleRolloutContextProcessing(context, retryDelay);
    }
}

void RolloutManager::ProcessReplacingSingleInstanceDeployment(__in SingleInstanceDeploymentContext & context)
{
    auto operation = AsyncOperation::CreateAndStart<ReplaceSingleInstanceDeploymentContextAsyncOperation>(
        *this,
        context,
        GetOperationTimeout(context),
        [this, &context](AsyncOperationSPtr const & operation) { this->FinishProcessingRolloutContext(context, operation, false); },
        replica_.CreateAsyncOperationRoot());

    this->FinishProcessingRolloutContext(context, operation, true);
}

void RolloutManager::ProcessUnknown(__in RolloutContext & context)
{
    ErrorCode error = Refresh(context);

    if (error.IsSuccess())
    {
        PostRolloutContextProcessing(context);
    }
    else
    {
        OnProcessingError(context, error);
    }
}

void RolloutManager::ProcessCompleted(__in RolloutContext & context)
{
    // Keep in queue for another round of rollout.
    // This flag is saved for "replacement" context.
    if (context.ShouldKeepInQueue())
    {
        Refresh(context);
        ScheduleRolloutContextProcessing(context, TimeSpan::Zero);
    }
    else
    {
        // This is a no-op if the context has been recovered
        // off persistent storage
        //
        context.CompleteClientRequest();

        this->RemoveActiveContext(context);
    }
}

void RolloutManager::ProcessFailed(__in RolloutContext & context, RolloutStatus::Enum status)
{
    // Do not complete the client request until after clean-up of the context
    // is successful, which will result in ProcessCompleted() getting called.
    // This is to avoid having the client observe state (upon retry) before
    // clean-up has finished

    auto storeTx = StoreTransaction::Create(replica_.ReplicatedStore, replica_.PartitionedReplicaId, context.ActivityId);

    ErrorCode error = context.UpdateStatus(storeTx, status);

    if (error.IsSuccess())
    {
        WriteNoise(
            TraceComponent,
            "{0} setting failed context ({1}): {2}",
            context.TraceId,
            status,
            context);

        auto operation = StoreTransaction::BeginCommit(
            move(storeTx),
            context,
            [this, &context, status](AsyncOperationSPtr const & operation) { this->OnProcessFailedCommitComplete(context, status, operation, false); },
            replica_.CreateAsyncOperationRoot());
        this->OnProcessFailedCommitComplete(context, status, operation, true);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} could not set {1} status for failed context {2} due to {3}",
            context.TraceId,
            status,
            context,
            error);

        TimeSpan retryDelay = replica_.GetRandomizedOperationRetryDelay(error);
        ScheduleRolloutContextProcessing(context, retryDelay);
    }
}

void RolloutManager::OnProcessFailedCommitComplete(
    __in RolloutContext & context, 
    RolloutStatus::Enum status,
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} could not commit {1} status for failed context {2} due to {3}",
            context.TraceId,
            status,
            context,
            error);
    }

    if (error.IsSuccess() && status == RolloutStatus::Failed)
    {
        // Complete the client request and remove the context from processing.
        // Leave it in the failed state.
        //
        ProcessCompleted(context);
    }
    else
    {
        // Continue with the next phase of processing or retry.
        //
        TimeSpan retryDelay = replica_.GetRandomizedOperationRetryDelay(error);
        ScheduleRolloutContextProcessing(context, retryDelay);
    }
}

TimeSpan RolloutManager::GetOperationTimeout(RolloutContext const & context)
{
    auto minTimeout = ManagementConfig::GetConfig().MinOperationTimeout;
    auto retryBuffer = TimeSpan::FromTicks(minTimeout.Ticks * context.GetTimeoutCount());

    auto maxRetryBuffer = ManagementConfig::GetConfig().MaxTimeoutRetryBuffer;
    if (retryBuffer > maxRetryBuffer) { retryBuffer = maxRetryBuffer; }

    auto timeout = TimeSpan::FromTicks(context.OperationTimeout.Ticks + retryBuffer.Ticks);

    auto maxTimeout = ManagementConfig::GetConfig().MaxOperationTimeout;
    if (timeout > maxTimeout) { timeout = maxTimeout; }

    return timeout;
}

TimeSpan RolloutManager::GetOperationRetry(RolloutContext const & context, Common::ErrorCode const & error)
{
    auto minRetry = ClusterManagerReplica::GetRandomizedOperationRetryDelay(error);
    auto exponentialRetry = TimeSpan::FromTicks(minRetry.Ticks * context.GetTimeoutCount());
    auto maxExponentialRetry = ManagementConfig::GetConfig().MaxExponentialOperationRetryDelay;
    
    return (exponentialRetry < maxExponentialRetry) ? exponentialRetry : maxExponentialRetry;
}

void RolloutManager::OnProcessingError(__in RolloutContext & context, ErrorCode const & error)
{
    if (context.ShouldTerminateProcessing())
    {
        context.OperationRetryStopwatch.Reset();
        WriteInfo(
            TraceComponent,
            "{0} cancelling rollout for {1} due to retry timeout {2}",
            context.TraceId,
            context,
            error);
 
        auto storeTx = StoreTransaction::Create(replica_.ReplicatedStore, replica_.PartitionedReplicaId, context.ActivityId);

        ErrorCode innerError = context.Fail(storeTx);
 
        if(innerError.IsSuccess())
        {
            auto operation = StoreTransaction::BeginCommit(
                move(storeTx),
                context,
                [this, &context] (AsyncOperationSPtr const & operation) { this->OnRetryTimeoutCommitComplete(context, operation, false); },
                replica_.CreateAsyncOperationRoot());
            this->OnRetryTimeoutCommitComplete(context, operation, true);
        }
        else
        {
            TimeSpan retryDelay = replica_.GetRandomizedOperationRetryDelay(error);

            WriteNoise(
                TraceComponent,
                "{0} could not update failed status for {1} due to {2}: retrying in {3}",
                context.TraceId,
                context,
                innerError,
                retryDelay);

            // Try to read context back from disk. If this fails, then
            // let the system attempt one more refresh after scheduling.
            //
            Refresh(context);
            ScheduleRolloutContextProcessing(context, retryDelay);
        }   
    }
    else if (IsReconfiguration(error))
    {
        WriteInfo(
            TraceComponent,
            "{0} cancelling rollout for {1} due to reconfiguration {2}",
            context.TraceId,
            context,
            error);

        // Cancel client request so it can be retried at the new primary 
        // immediately rather than making the client wait for timeout
        //
        context.CompleteClientRequest(error);
        this->RemoveActiveContext(context);
    }
    else if (IsRetryable(error))
    {
        TimeSpan retryDelay = replica_.GetRandomizedOperationRetryDelay(error);

        if (error.IsError(ErrorCodeValue::Timeout) || error.IsError(ErrorCodeValue::NamingServiceTooBusy))
        {
            context.IncrementTimeoutCount();
        }

        WriteInfo(
            TraceComponent,
            "{0} scheduling retry for {1} in {2} due to {3}: timeouts={4}",
            context.TraceId,
            context,
            retryDelay,
            error,
            context.GetTimeoutCount());

        // Ignore error and try to refresh one more time after scheduling if this fails
        Refresh(context);
        ScheduleRolloutContextProcessing(context, retryDelay);
    }
    else if (error.IsError(ErrorCodeValue::CMRequestAborted))
    {
        // Stop processing the context, but do not fail the operation or
        // return a reply to the client since the context is still in the
        // pending state. It is up to the CM to restart processing as
        // appropriate.
        //
        // Currently, the only cases for this are aborting an application/cluster
        // upgrade or infrastructure task when primary recovery is blocked behind 
        // the pending context.
        //
        this->RemoveActiveContext(context);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} failing operation {1} due to {2}",
            context.TraceId,
            context,
            error);

        context.SetCompletionError(error);

        auto storeTx = StoreTransaction::Create(replica_.ReplicatedStore, replica_.PartitionedReplicaId, context.ActivityId);

        ErrorCode innerError = context.Fail(storeTx);

        // Do not persist the fail status to avoid unnecessary replication. 
        // Let processing of the fail status persist the new status (e.g. DeletePending)
        // if necessary. Also, failed contexts are not recovered for processing on 
        // primary failover, so the "Failed" status should only be persisted after
        // all required failed processing is complete.
        //
        if (innerError.IsSuccess())
        {
            PostRolloutContextProcessing(context);
        }
        else
        {
            TimeSpan retryDelay = replica_.GetRandomizedOperationRetryDelay(error);

            WriteNoise(
                TraceComponent,
                "{0} could not update failed status for {1} due to {2}: retrying in {3}",
                context.TraceId,
                context,
                innerError,
                retryDelay);

            // Try to read context back from disk. If this fails, then
            // let the system attempt one more refresh after scheduling.
            //
            Refresh(context);
            ScheduleRolloutContextProcessing(context, retryDelay);
        }
    }
}

void RolloutManager::OnRetryTimeoutCommitComplete(
    __in RolloutContext & context,
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode error = StoreTransaction::EndCommit(operation);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} could not commit for failed context {1} due to {2}",
            context.TraceId,
            context,
            error);

        // Continue with the next phase of processing or retry.
        //
        TimeSpan retryDelay = replica_.GetRandomizedOperationRetryDelay(error);
        ScheduleRolloutContextProcessing(context, retryDelay);
    }
    else
    {
        context.SetCompletionError(replica_.TraceAndGetErrorDetails( ErrorCodeValue::CMOperationFailed, wformatString("{0} RetryTimeout: {1}", GET_RC( Operation_Failed ), ManagementConfig::GetConfig().RolloutFailureTimeout))); 
        ProcessCompleted(context);
    }
}


ErrorCode RolloutManager::Refresh(__in RolloutContext & context) const
{
    auto storeTx = StoreTransaction::Create(replica_.ReplicatedStore, replica_.PartitionedReplicaId, context.ActivityId);

    ErrorCode error = context.Refresh(storeTx);

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to refresh {1}: error = {2}",
            context.TraceId,
            context,
            error);

        storeTx.Rollback();
    }

    return error;
}

void RolloutManager::OnUnknownContextType(__in RolloutContext & context)
{
    Assert::TestAssert("{0}: Unknown context type {1}", this->TraceId, context.ContextType);
}

bool RolloutManager::IsReconfiguration(ErrorCode const & error)
{
    switch (error.ReadValue())
    {
        case ErrorCodeValue::NotPrimary:
            return (!replica_.IsActivePrimary);

        case ErrorCodeValue::ObjectClosed:
        case ErrorCodeValue::InvalidState:  // returned by replicator
            return true;

        default:
            return false;
    }
}

bool RolloutManager::IsRetryable(ErrorCode const & error)
{
    switch (error.ReadValue())
    {
        case ErrorCodeValue::Timeout:
        case ErrorCodeValue::OperationCanceled:
        case ErrorCodeValue::GatewayUnreachable:

        case ErrorCodeValue::NoWriteQuorum:
        case ErrorCodeValue::ReconfigurationPending:
        // DeleteApplication/UpgradeApplication may need to wait for pending Create/Delete service requests
        // before proceeding
        //
        case ErrorCodeValue::CMRequestAlreadyProcessing:

        //case ErrorCodeValue::StoreRecordAlreadyExists: // same as StoreWriteConflict
        case ErrorCodeValue::StoreOperationCanceled:
        case ErrorCodeValue::StoreWriteConflict:
        case ErrorCodeValue::StoreSequenceNumberCheckFailed:
        case ErrorCodeValue::StoreUnexpectedError:
        case ErrorCodeValue::StoreTransactionNotActive:

        case ErrorCodeValue::CMImageBuilderRetryableError:
        case ErrorCodeValue::AcquireFileLockFailed:

        // The queue with operations to be sent to Naming is full
        case ErrorCodeValue::JobQueueFull:

        // HM health report queue is full
        case ErrorCodeValue::ServiceTooBusy:
            return true;

        default:
            return false;
    }
}

bool RolloutManager::TryAddActiveContextCallerHoldsLock(
    shared_ptr<RolloutContext> const & context,
    __out shared_ptr<RolloutContext> & activeContext)
{
    auto const & mapKey = context->MapKey;

    // Do not accept duplicate contexts for processing.
    //
    auto iter = activeContexts_.find(mapKey);
    if (iter == activeContexts_.end())
    {
        activeContext = context;

        activeContexts_.insert(make_pair(mapKey, context));

        return true;
    }
    else
    {
        activeContext = activeContexts_[mapKey];

        return false;
    }
}

void RolloutManager::RemoveActiveContext(__in RolloutContext & context)
{
    AcquireExclusiveLock lock(contextsLock_);

    auto iter = activeContexts_.find(context.MapKey);
    if (iter != activeContexts_.end())
    {
        activeContexts_.erase(iter);
    }
}

// ******************
// Template functions
// ******************

template ErrorCode RolloutManager::RecoverRolloutContexts<ApplicationTypeContext>(StoreTransaction const &, wstring const &, __inout ActivityId &, __out size_t &);
template ErrorCode RolloutManager::RecoverRolloutContexts<ApplicationContext>(StoreTransaction const &, wstring const &, __inout ActivityId &, __out size_t &);
template ErrorCode RolloutManager::RecoverRolloutContexts<ServiceContext>(StoreTransaction const &, wstring const &, __inout ActivityId &, __out size_t &);
template ErrorCode RolloutManager::RecoverRolloutContexts<ApplicationUpgradeContext>(StoreTransaction const &, wstring const &, __inout ActivityId &, __out size_t &);
template ErrorCode RolloutManager::RecoverRolloutContexts<FabricProvisionContext>(StoreTransaction const &, wstring const &, __inout ActivityId &, __out size_t &);
template ErrorCode RolloutManager::RecoverRolloutContexts<FabricUpgradeContext>(StoreTransaction const &, wstring const &, __inout ActivityId &, __out size_t &);
template ErrorCode RolloutManager::RecoverRolloutContexts<InfrastructureTaskContext>(StoreTransaction const &, wstring const &, __inout ActivityId &, __out size_t &);

template ErrorCode RolloutManager::RecoverRolloutContexts<ApplicationTypeContext>(StoreTransaction const &, wstring const &, __inout ActivityId &, __out size_t &, __out vector<ApplicationTypeContext> &);
template ErrorCode RolloutManager::RecoverRolloutContexts<ApplicationContext>(StoreTransaction const &, wstring const &, __inout ActivityId &, __out size_t &, __out vector<ApplicationContext> &);
template ErrorCode RolloutManager::RecoverRolloutContexts<ServiceContext>(StoreTransaction const &, wstring const &, __inout ActivityId &, __out size_t &, __out vector<ServiceContext> &);

template <class T>
ErrorCode RolloutManager::RecoverRolloutContexts(
    StoreTransaction const & storeTx, 
    std::wstring const & type, 
    __inout ActivityId & activityId,
    __out size_t & recoveryCount)
{
    vector<T> unused;
    return RecoverRolloutContexts(storeTx, type, activityId, recoveryCount, unused);
}

template <class T>
ErrorCode RolloutManager::RecoverRolloutContexts(
    StoreTransaction const & storeTx, 
    std::wstring const & type, 
    __inout ActivityId & activityId,
    __out size_t & recoveryCount,
    __out vector<T> & allContexts)
{
    size_t count = 0;
    vector<T> typedContexts;
    ErrorCode error = storeTx.ReadPrefix(type, L"", typedContexts);

    if (error.IsSuccess())
    {
        // Take the lock once instead of releasing/re-acquiring for
        // each recovered context
        //
        AcquireExclusiveLock lock(contextsLock_);

        for (auto & typedContext : typedContexts)
        {
            if (!typedContext.IsComplete && !typedContext.IsFailed)
            {
                shared_ptr<T> context(new T(move(typedContext)));
                context->ReInitializeContext(*replicaSPtr_);

                // Give each recovered context a different activityId - correlated
                // with the recovery transaction's activityId
                // 
                activityId = activityId.GetNestedActivity();
                context->ReInitializeTracing(Store::ReplicaActivityId(storeTx.ReplicaActivityId.PartitionedReplicaId, activityId));

                shared_ptr<RolloutContext> activeContext;
                if (this->TryAddActiveContextCallerHoldsLock(context, activeContext))
                {
                    ++count;

                    WriteNoise(
                        TraceComponent, 
                        "{0} recovering {1}",
                        activeContext->TraceId, 
                        *activeContext); 

                    this->PostRolloutContextProcessing(*activeContext);
                }
            }
        }

        recoveryCount = count;
        allContexts.swap(typedContexts);
    }

    return error;
}
