// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Infrastructure;

FabricUpgrade::FabricUpgrade(
    std::wstring const & activityId, 
    Common::FabricCodeVersion const & nodeVersion,
    ServiceModel::FabricUpgradeSpecification const & upgradeSpecification, 
    bool isNodeUp, 
    ReconfigurationAgent & ra)
: IUpgrade(ra, upgradeSpecification.InstanceId),
  activityId_(activityId),
  upgradeSpecification_(upgradeSpecification),
  isNodeUp_(isNodeUp),
  shouldRestartReplica_(false),
  nodeVersion_(nodeVersion)
{
}

UpgradeStateName::Enum FabricUpgrade::GetStartState(RollbackSnapshotUPtr && rollbackSnapshot) 
{
    if (rollbackSnapshot != nullptr)
    {
        ASSERT_IF(rollbackSnapshot->State == UpgradeStateName::Completed, "Rollback is not possible from completed");

        switch (rollbackSnapshot->State)
        {
        case UpgradeStateName::FabricUpgrade_Upgrade:
        case UpgradeStateName::FabricUpgrade_UpgradeFailed:               
        case UpgradeStateName::FabricUpgrade_UpdateEntitiesOnCodeUpgrade:
            /*
                If it is a rollback then staleness checks have already been performed

                A rollback will need to reopen down replicas if the previous upgrade was in the upgrade state and replicas had been closed
            */
            return rollbackSnapshot->WereReplicasClosed ? UpgradeStateName::FabricUpgrade_RollbackPreviousUpgrade : UpgradeStateName::FabricUpgrade_Download;

        case UpgradeStateName::FabricUpgrade_Download:
        case UpgradeStateName::FabricUpgrade_DownloadFailed:
        case UpgradeStateName::FabricUpgrade_Validate:
        case UpgradeStateName::FabricUpgrade_ValidateFailed:
            return UpgradeStateName::FabricUpgrade_Download;

        case UpgradeStateName::FabricUpgrade_CloseReplicas:
        case UpgradeStateName::FabricUpgrade_RollbackPreviousUpgrade:
            Assert::CodingError("Rollback from {0} is not possible", rollbackSnapshot->State);

        default:
            Assert::CodingError("Unexpected Rollback state {0} in fabric upgrade", rollbackSnapshot->State);
        }
    }

    return UpgradeStateName::FabricUpgrade_Download;
}

UpgradeStateDescription const & FabricUpgrade::GetStateDescription(UpgradeStateName::Enum state) const
{
    /*
        Transient state where computation happens to decide whether the previous upgrade caused replicas to be closed
        and whether replicas need to be opened
    */
    static UpgradeStateDescription const rollbackPreviousUpgrade(
        UpgradeStateName::FabricUpgrade_RollbackPreviousUpgrade,
        UpgradeStateCancelBehaviorType::NonCancellableWithNoRollback);

    /*
        For the states below (upto the sendCloseMessage) no change to RA FT state has happened
        The hosting async API (if any) can be cancelled immediately and a rollback can be started
    */
    static UpgradeStateDescription const download(
        UpgradeStateName::FabricUpgrade_Download,
        UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback,
        true /*isAsyncApiState */);

    static UpgradeStateDescription const downloadFailed(
        UpgradeStateName::FabricUpgrade_DownloadFailed, 
        [](FailoverConfig const & cfg) { return cfg.FabricUpgradeDownloadRetryInterval; }, 
        UpgradeStateName::FabricUpgrade_Download,
        UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback);

    static UpgradeStateDescription const validate(
        UpgradeStateName::FabricUpgrade_Validate,
        UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback,
        true /* isAsyncApistate */);

    static UpgradeStateDescription const validateFailed(
        UpgradeStateName::FabricUpgrade_ValidateFailed, 
        [](FailoverConfig const & cfg) { return cfg.FabricUpgradeValidateRetryInterval; }, 
        UpgradeStateName::FabricUpgrade_Validate,
        UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback);

    /*
        For the states below (upto updateEntitiesOnCodeUpgrade) rollback/cancel is not allowed

        This is because during these states the FT is being closed. This is happening for an upgrade from
        v1 -> v2 which requires node restart. If v1 -> v2 is rolling back then the node does not need to go down

        However, some FT could already have been closed. Some FT could still be closing. These would need to be opened 
        (if they are persisted)

        The RA state machine for FT does not handle queued open while closing. Thus, if the node is in this state
        then the rollback and cancel is discarded until all the FTs transition to closed state.
    */
    static UpgradeStateDescription const closeReplicas(
        UpgradeStateName::FabricUpgrade_CloseReplicas,
        UpgradeStateCancelBehaviorType::NonCancellableWithNoRollback,
        true /* isAsyncApiState */);

    /*
        The FTs are being updated to indicate that a code upgrade is being performed
        Immediate rollback is not possible as the state update should complete
        After this update the rollback can start (and it will open the replicas as appropriate)
    */
    static UpgradeStateDescription const updateEntitiesOnCodeUpgrade(
        UpgradeStateName::FabricUpgrade_UpdateEntitiesOnCodeUpgrade,
        UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback);

    /*
        Upgrade API has been called on hosting. At this point rollback can be processed immediately 
        (by cancelling the async op on hosting and starting the rollback)
    */
    static UpgradeStateDescription const upgrade(
        UpgradeStateName::FabricUpgrade_Upgrade,
        UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback,
        true /* isAsyncApiState */);

    static UpgradeStateDescription const upgradeFailed(
        UpgradeStateName::FabricUpgrade_UpgradeFailed, 
        [](FailoverConfig const & cfg) { return cfg.FabricUpgradeUpgradeRetryInterval; }, 
        UpgradeStateName::FabricUpgrade_Upgrade,
        UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback);

    switch (state)
    {
    case UpgradeStateName::FabricUpgrade_RollbackPreviousUpgrade:
        return rollbackPreviousUpgrade;

    case UpgradeStateName::FabricUpgrade_Download:
        return download;

    case UpgradeStateName::FabricUpgrade_DownloadFailed:
        return downloadFailed;

    case UpgradeStateName::FabricUpgrade_Validate:
        return validate;

    case UpgradeStateName::FabricUpgrade_ValidateFailed:
        return validateFailed;
                   
    case UpgradeStateName::FabricUpgrade_CloseReplicas:
        return closeReplicas;

    case UpgradeStateName::FabricUpgrade_UpdateEntitiesOnCodeUpgrade:
        return updateEntitiesOnCodeUpgrade;

    case UpgradeStateName::FabricUpgrade_Upgrade:
        return upgrade;

    case UpgradeStateName::FabricUpgrade_UpgradeFailed:
        return upgradeFailed;

    default:
        Assert::CodingError("Unknown fabric upgrade state: {0}", state);
    };
}

RollbackSnapshotUPtr FabricUpgrade::CreateRollbackSnapshot(UpgradeStateName::Enum state) const
{
    return make_unique<RollbackSnapshot>(state, shouldRestartReplica_);
}

void FabricUpgrade::SendReply()
{
    Common::FabricVersionInstance fabricVersionInstance(UpgradeSpecification.Version, UpgradeSpecification.InstanceId);

    if (ra_.Config.IsVersionInstanceCheckEnabled && fabricVersionInstance != ra_.Reliability.NodeConfig->NodeVersion)
    {
        auto msg = Common::wformatString("Failfast as NodeConfig.NodeVersion {0} and upgrade.Instance {1} don't match.", ra_.Reliability.NodeConfig->NodeVersion, fabricVersionInstance);
        ra_.ProcessTerminationService.ExitProcess(msg);
    }

    if (isNodeUp_)
    {
        ra_.Reliability.SendNodeUpToFM();
    }
    else
    {
        ra_.FMTransportObj.SendFabricUpgradeReplyMessage(GetActivityId(), NodeFabricUpgradeReplyMessageBody(fabricVersionInstance));
    }
}

UpgradeStateName::Enum FabricUpgrade::EnterState(UpgradeStateName::Enum state, AsyncStateActionCompleteCallback callback)
{
    switch (state)
    {
    case UpgradeStateName::FabricUpgrade_RollbackPreviousUpgrade:
        return Rollback(callback);

    case UpgradeStateName::FabricUpgrade_UpdateEntitiesOnCodeUpgrade:
        return UpdateEntitiesOnCodeUpgrade(callback);

    default:
        Assert::CodingError("Unknown state {0}", state);
    }
}

AsyncOperationSPtr FabricUpgrade::EnterAsyncOperationState(
    UpgradeStateName::Enum state,
    AsyncCallback const & asyncCallback)
{
    switch (state)
    {
    case UpgradeStateName::FabricUpgrade_Download:
        return Download(asyncCallback);

    case UpgradeStateName::FabricUpgrade_Validate:
        return Validate(asyncCallback);

    case UpgradeStateName::FabricUpgrade_Upgrade:
        return Upgrade(asyncCallback);

    case UpgradeStateName::FabricUpgrade_CloseReplicas:
        return CloseReplicas(asyncCallback);

    default:
        Assert::CodingError("Unknown fabric upgrade state: {0}", state);
    }
}

UpgradeStateName::Enum FabricUpgrade::ExitAsyncOperationState(
    UpgradeStateName::Enum state,
    AsyncOperationSPtr const & asyncOp)
{
    switch (state)
    {
    case UpgradeStateName::FabricUpgrade_Download:
        return EndDownload(asyncOp);

    case UpgradeStateName::FabricUpgrade_Validate:
        return EndValidate(asyncOp);

    case UpgradeStateName::FabricUpgrade_Upgrade:
        return EndUpgrade(asyncOp);

    case UpgradeStateName::FabricUpgrade_CloseReplicas:
        return EndCloseReplicas(asyncOp);

    default:
        Assert::CodingError("Unknown fabric upgrade state: {0}", state);
    }
}

UpgradeStateName::Enum FabricUpgrade::Rollback(AsyncStateActionCompleteCallback callback)
{
    auto fts = ra_.LocalFailoverUnitMapObj.GetAllFailoverUnitEntries(false);

    ra_.OnFabricUpgradeRollback();

    if (fts.empty())
    {
        return UpgradeStateName::FabricUpgrade_Download;
    }

    auto work = make_shared<MultipleEntityWork>(
        GetActivityId(),
        [this, callback](MultipleEntityWork const & inner, ReconfigurationAgent &)
    {
        OnRollbackCompleted(inner, callback);
    });

    auto handler = [this](HandlerParameters & handlerParameters, JobItemContextBase & context)
    {
        return RollbackFTProcessor(handlerParameters, context);
    };

    ra_.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(
        work,
        fts,
        [handler](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
        {
            FabricUpgradeRollbackJobItem::Parameters jobItemParameters(
                entry,
                work,
                handler,
                JobItemCheck::DefaultAndOpen,
                *JobItemDescription::FabricUpgradeRollback,
                move(JobItemContextBase()));

            return make_shared<FabricUpgradeRollbackJobItem>(move(jobItemParameters));
        });

    return UpgradeStateName::Invalid;
}

bool FabricUpgrade::RollbackFTProcessor(HandlerParameters & handlerParameters, JobItemContextBase &) const
{
    handlerParameters.AssertFTExists();
    handlerParameters.AssertFTIsOpen();

    ra_.ReopenDownReplica(handlerParameters);

    return true;
}

void FabricUpgrade::OnRollbackCompleted(MultipleEntityWork const &, AsyncStateActionCompleteCallback callback)
{
    callback(UpgradeStateName::FabricUpgrade_Download);
}

AsyncOperationSPtr FabricUpgrade::Download(AsyncCallback const & callback)
{
    return ra_.HostingObj.BeginDownloadFabric(UpgradeSpecification.Version, 
        callback,
        ra_.Root.CreateAsyncOperationRoot());
}

UpgradeStateName::Enum FabricUpgrade::EndDownload(AsyncOperationSPtr const & operation)
{
    auto error = ra_.HostingObj.EndDownloadFabric(operation);

    if (!error.IsSuccess())
    {
        return UpgradeStateName::FabricUpgrade_DownloadFailed;
    }
    else if (isNodeUp_)
    {
        return UpgradeStateName::FabricUpgrade_UpdateEntitiesOnCodeUpgrade;
    }
    else
    {
        // determine whether this upgrade requires replicas to be closed
        return UpgradeStateName::FabricUpgrade_Validate;
    }
}

AsyncOperationSPtr FabricUpgrade::Validate(AsyncCallback const & callback)
{
    return ra_.HostingObj.BeginValidateFabricUpgrade(UpgradeSpecification, 
        callback,
        ra_.Root.CreateAsyncOperationRoot());
}

UpgradeStateName::Enum FabricUpgrade::EndValidate(AsyncOperationSPtr const & op)
{
    auto error = ra_.HostingObj.EndValidateFabricUpgrade(shouldRestartReplica_, op);

    if (!error.IsSuccess())
    {
        return UpgradeStateName::FabricUpgrade_ValidateFailed;
    }
    else 
    {
        RAEventSource::Events->FabricUpgradeValidateDecision(ra_.NodeInstanceIdStr, GetActivityId(), shouldRestartReplica_);
        return shouldRestartReplica_ ? UpgradeStateName::FabricUpgrade_CloseReplicas : UpgradeStateName::FabricUpgrade_Upgrade;
    }
}

Common::AsyncOperationSPtr FabricUpgrade::CloseReplicas(Common::AsyncCallback const & callback)
{
    // Tell the RA to not accept any more replica creates
    // This must happen before the snapshot of the LFUM takes place
    ra_.OnFabricUpgradeStart();

    auto fts = ra_.LocalFailoverUnitMapObj.GetAllFailoverUnitEntries(false);

    // Start the close completion check
    MultipleReplicaCloseAsyncOperation::Parameters parameters;
    parameters.Clock = ra_.ClockSPtr;
    parameters.ActivityId = GetActivityId();
    parameters.Callback = [](wstring const & activityId, Infrastructure::EntityEntryBaseList const & pendingCloseFT, ReconfigurationAgent & ra)
    {
        RAEventSource::Events->UpgradeCloseCompletionCheckResult(ra.NodeInstanceIdStr, activityId, pendingCloseFT.size(), pendingCloseFT, EntityEntryBaseList());
    };
    parameters.CloseMode = ReplicaCloseMode::Close;
    parameters.FTsToClose = move(fts);
    parameters.JobItemCheck = JobItemCheck::DefaultAndOpen;
    parameters.MaxWaitBeforeTerminationEntry = &ra_.Config.FabricUpgradeMaxReplicaCloseDurationEntry;
    parameters.MonitoringIntervalEntry = &ra_.Config.FabricUpgradeReplicaCloseCompleteCheckIntervalEntry;
    parameters.RA = &ra_;
    parameters.TerminateReason = Hosting::TerminateServiceHostReason::FabricUpgrade;

    return AsyncOperation::CreateAndStart<MultipleReplicaCloseAsyncOperation>(move(parameters), callback, ra_.Root.CreateAsyncOperationRoot());
}

UpgradeStateName::Enum FabricUpgrade::EndCloseReplicas(Common::AsyncOperationSPtr const & op)
{
    auto result = MultipleReplicaCloseCompletionCheckAsyncOperation::End(op);
    TESTASSERT_IF(!result.IsSuccess(), "Fabric upgrade close failed");

    return UpgradeStateName::FabricUpgrade_UpdateEntitiesOnCodeUpgrade;
}

UpgradeStateName::Enum FabricUpgrade::UpdateEntitiesOnCodeUpgrade(AsyncStateActionCompleteCallback callback)
{
    if (upgradeSpecification_.Version.CodeVersion == nodeVersion_)
    {
        return UpgradeStateName::FabricUpgrade_Upgrade;
    }

    FabricCodeVersionProperties newVersionProperties = FabricCodeVersionClassifier().Classify(upgradeSpecification_.Version.CodeVersion);

    auto fts = ra_.LocalFailoverUnitMapObj.GetAllFailoverUnitEntries(false);

    auto work = make_shared<MultipleFailoverUnitWorkWithInput<FabricCodeVersionProperties>>(
        GetActivityId(),
        [this, callback](MultipleEntityWork const & inner, ReconfigurationAgent &)
        {
            OnUpdateEntitiesOnCodeUpgradeCompleted(inner, callback);
        },
        move(newVersionProperties));

    auto handler = [this](HandlerParameters & handlerParameters, JobItemContextBase & context)
    {
        return UpdateEntitiesOnCodeUpgradeProcessor(handlerParameters, context);
    };

    ra_.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(
        work,
        fts,
        [handler](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
        {
            UpdateEntitiesOnCodeUpgradeJobItem::Parameters jobItemParameters(
                entry,
                work,
                handler,
                static_cast<JobItemCheck::Enum>(JobItemCheck::RAIsOpen | JobItemCheck::FTIsNotNull),
                *JobItemDescription::FabricUpgradeUpdateEntity);

            return make_shared<UpdateEntitiesOnCodeUpgradeJobItem>(move(jobItemParameters));
        });

    // async execution will occur
    return UpgradeStateName::Invalid;
}

bool FabricUpgrade::UpdateEntitiesOnCodeUpgradeProcessor(HandlerParameters & handlerParameters, JobItemContextBase &) const
{
    auto & ft = handlerParameters.FailoverUnit;
    handlerParameters.AssertHasWork();
    handlerParameters.AssertFTExists();

    auto const & versionProperties = handlerParameters.Work->GetInput<FabricCodeVersionProperties>();

    ft.EnableUpdate();
    ft->OnFabricCodeUpgrade(versionProperties);
    return true;
}

void FabricUpgrade::OnUpdateEntitiesOnCodeUpgradeCompleted(MultipleEntityWork const & , AsyncStateActionCompleteCallback callback)
{
    callback(UpgradeStateName::FabricUpgrade_Upgrade);
}

AsyncOperationSPtr FabricUpgrade::Upgrade(AsyncCallback const & callback)
{
    return ra_.HostingObj.BeginFabricUpgrade(UpgradeSpecification, 
        shouldRestartReplica_, 
        callback,
        ra_.Root.CreateAsyncOperationRoot());
}

UpgradeStateName::Enum FabricUpgrade::EndUpgrade(AsyncOperationSPtr const & operation)
{
    auto error = ra_.HostingObj.EndFabricUpgrade(operation);

    if (error.IsSuccess())
    {
        SendReply();
        return UpgradeStateName::Completed;
    }
    else 
    {
        return UpgradeStateName::FabricUpgrade_UpgradeFailed;
    }
}

void FabricUpgrade::WriteToEtw(uint16 contextSequenceId) const
{
    RAEventSource::Events->FabricUpgrade(contextSequenceId, GetActivityId(), isNodeUp_, UpgradeSpecification);
}

void FabricUpgrade::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << "FabricUpgrade. ActivityId = (" << GetActivityId() << ")IsNodeUp = (" << isNodeUp_ << "). Specification = [" << UpgradeSpecification << "]";
}
