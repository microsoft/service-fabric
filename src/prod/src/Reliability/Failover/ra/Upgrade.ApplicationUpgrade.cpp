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
using namespace ServiceModel;
using namespace Hosting2;
using namespace Infrastructure;

ApplicationUpgrade::ApplicationUpgrade(
    wstring && activityId, 
    ApplicationUpgradeSpecification const & upgradeSpec,
    ReconfigurationAgent & ra)
: IUpgrade(ra, upgradeSpec.InstanceId),
  activityId_(move(activityId)),
  upgradeSpec_(upgradeSpec)
{
}

UpgradeStateName::Enum ApplicationUpgrade::GetStartState(RollbackSnapshotUPtr && rollbackSnapshot)
{
    ASSERT_IF(rollbackSnapshot != nullptr, "No rollback for app upgrade");
    return UpgradeStateName::ApplicationUpgrade_EnumerateFTs;
}

UpgradeStateDescription const & ApplicationUpgrade::GetStateDescription(UpgradeStateName::Enum state) const
{
    static UpgradeStateDescription const enumerateFTs(
        UpgradeStateName::ApplicationUpgrade_EnumerateFTs, 
        UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback);
    
    static UpgradeStateDescription const download(
        UpgradeStateName::ApplicationUpgrade_Download, 
        UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback,
        true /* isAsyncApiState */);
    
    static UpgradeStateDescription const downloadFailed(
        UpgradeStateName::ApplicationUpgrade_DownloadFailed, 
        [](FailoverConfig const & cfg) { return cfg.RAUpgradeProgressCheckInterval; },
        UpgradeStateName::ApplicationUpgrade_Download,
        UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback);

    static UpgradeStateDescription const analyze(
        UpgradeStateName::ApplicationUpgrade_Analyze, 
        UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback);

    static UpgradeStateDescription const analyzeFailed(
        UpgradeStateName::ApplicationUpgrade_AnalyzeFailed, 
        [](FailoverConfig const & cfg) { return cfg.RAUpgradeProgressCheckInterval; },
        UpgradeStateName::ApplicationUpgrade_Analyze,
        UpgradeStateCancelBehaviorType::CancellableWithImmediateRollback);

    static UpgradeStateDescription const upgrade(
        UpgradeStateName::ApplicationUpgrade_Upgrade, 
        UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback,
        true /* isAsyncApiState */);

    static UpgradeStateDescription const upgradeFailed(
        UpgradeStateName::ApplicationUpgrade_UpgradeFailed, 
        [](FailoverConfig const & cfg) { return cfg.RAUpgradeProgressCheckInterval; }, 
        UpgradeStateName::ApplicationUpgrade_Upgrade,
        UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback);

    static UpgradeStateDescription const updateVersionsAndCloseIfNeeded(
        UpgradeStateName::ApplicationUpgrade_UpdateVersionsAndCloseIfNeeded, 
        UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback);

    static UpgradeStateDescription const waitForCloseCompletion(
        UpgradeStateName::ApplicationUpgrade_WaitForCloseCompletion, 
        UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback,
        true /* is Async API */);

    static UpgradeStateDescription const waitForDropCompletion(
        UpgradeStateName::ApplicationUpgrade_WaitForDropCompletion,
        UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback,
        true /* is Async API */);

    static UpgradeStateDescription const waitForReplicaDownCompletionCheck(
        UpgradeStateName::ApplicationUpgrade_WaitForReplicaDownCompletionCheck, 
        [](FailoverConfig const & cfg) { return cfg.RAUpgradeProgressCheckInterval; }, 
        UpgradeStateName::ApplicationUpgrade_ReplicaDownCompletionCheck,
        UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback);

    static UpgradeStateDescription const replicaDownCompletionCheck(
        UpgradeStateName::ApplicationUpgrade_ReplicaDownCompletionCheck,
        UpgradeStateCancelBehaviorType::NonCancellableWithImmediateRollback);

    static UpgradeStateDescription const finalizeUpgrade(
        UpgradeStateName::ApplicationUpgrade_Finalize,
        UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback);

    switch (state)
    {
    case UpgradeStateName::ApplicationUpgrade_EnumerateFTs:
        return enumerateFTs;

    case UpgradeStateName::ApplicationUpgrade_Download:
        return download;

    case UpgradeStateName::ApplicationUpgrade_DownloadFailed:
        return downloadFailed;

    case UpgradeStateName::ApplicationUpgrade_Analyze:
        return analyze;

    case UpgradeStateName::ApplicationUpgrade_AnalyzeFailed:
        return analyzeFailed;

    case UpgradeStateName::ApplicationUpgrade_Upgrade:
        return upgrade;

    case UpgradeStateName::ApplicationUpgrade_UpgradeFailed:
        return upgradeFailed;

    case UpgradeStateName::ApplicationUpgrade_UpdateVersionsAndCloseIfNeeded:
        return updateVersionsAndCloseIfNeeded;

    case UpgradeStateName::ApplicationUpgrade_WaitForCloseCompletion:
        return waitForCloseCompletion;
    
    case UpgradeStateName::ApplicationUpgrade_WaitForDropCompletion:
        return waitForDropCompletion;

    case UpgradeStateName::ApplicationUpgrade_WaitForReplicaDownCompletionCheck:
        return waitForReplicaDownCompletionCheck;
    
    case UpgradeStateName::ApplicationUpgrade_ReplicaDownCompletionCheck:
        return replicaDownCompletionCheck;

    case UpgradeStateName::ApplicationUpgrade_Finalize:
        return finalizeUpgrade;

    default:
        Assert::CodingError("Unknown application upgradestate: {0}", state);
    };
}

RollbackSnapshotUPtr ApplicationUpgrade::CreateRollbackSnapshot(UpgradeStateName::Enum state) const
{
    UNREFERENCED_PARAMETER(state);
    return RollbackSnapshotUPtr();
}

UpgradeStateName::Enum ApplicationUpgrade::EnterState(UpgradeStateName::Enum state, AsyncStateActionCompleteCallback callback)
{
    switch (state)
    {
    case UpgradeStateName::ApplicationUpgrade_EnumerateFTs:
        return EnumerateFTs(callback);

    case UpgradeStateName::ApplicationUpgrade_Analyze:
        return Analyze();

    case UpgradeStateName::ApplicationUpgrade_UpdateVersionsAndCloseIfNeeded:
        return UpdateVersionsAndCloseIfNeeded(callback);

    case UpgradeStateName::ApplicationUpgrade_ReplicaDownCompletionCheck:
        return ReplicaDownCompletionCheck(callback);

    case UpgradeStateName::ApplicationUpgrade_Finalize:
        return Finalize(callback);

    default:
        Assert::CodingError("Unknown application upgradestate: {0}", state);
    };    
}

AsyncOperationSPtr ApplicationUpgrade::EnterAsyncOperationState(
    UpgradeStateName::Enum state,
    AsyncCallback const & asyncCallback)
{
    switch (state)
    {
    case UpgradeStateName::ApplicationUpgrade_Download:
        return Download(asyncCallback);

    case UpgradeStateName::ApplicationUpgrade_Upgrade:
        return Upgrade(asyncCallback);

    case UpgradeStateName::ApplicationUpgrade_WaitForCloseCompletion:
        return WaitForCloseCompletion(asyncCallback);

    case UpgradeStateName::ApplicationUpgrade_WaitForDropCompletion:
        return WaitForDropCompletion(asyncCallback);

    default:
        Assert::CodingError("Unknown application upgradestate: {0}", state);
    }
}

UpgradeStateName::Enum ApplicationUpgrade::ExitAsyncOperationState(
    UpgradeStateName::Enum state,
    AsyncOperationSPtr const & asyncOp)
{
    switch (state)
    {
    case UpgradeStateName::ApplicationUpgrade_Download:
        return EndDownload(asyncOp);

    case UpgradeStateName::ApplicationUpgrade_Upgrade:
        return EndUpgrade(asyncOp);

    case UpgradeStateName::ApplicationUpgrade_WaitForCloseCompletion:
        return EndWaitForCloseCompletion(asyncOp);

    case UpgradeStateName::ApplicationUpgrade_WaitForDropCompletion:
        return EndWaitForDropCompletion(asyncOp);

    default:
        Assert::CodingError("Unknown application upgradestate: {0}", state);
    }
}

void ApplicationUpgrade::SendReply()
{
    vector<FailoverUnitInfo> replyFTInfoList, replyDroppedFTInfoList;

    NodeUpgradeReplyMessageBody body(UpgradeSpecification, move(replyFTInfoList), move(replyDroppedFTInfoList));
    ra_.FMTransportObj.SendApplicationUpgradeReplyMessage(GetActivityId(), body);
}

UpgradeStateName::Enum ApplicationUpgrade::EnumerateFTs(AsyncStateActionCompleteCallback callback) 
{
    auto failoverUnitsInLFUM = ra_.LocalFailoverUnitMapObj.GetAllFailoverUnitEntries(false);

    // If the LFUM is empty then No-Op
    if (failoverUnitsInLFUM.empty())
    {
        SendReply();

        return UpgradeStateName::Completed;
    }

    auto work = make_shared<MultipleEntityWork>(
        GetActivityId(),
        [this, callback] (MultipleEntityWork const & work, ReconfigurationAgent &)
        {
            OnEnumerationComplete(work, callback);
        });

    auto handler = [this](HandlerParameters & handlerParameters, ApplicationUpgradeEnumerateFTsJobItemContext & context)
    {
        return EnumerateFTProcessor(handlerParameters, context);
    };

    ra_.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(
        work,
        failoverUnitsInLFUM,
        [handler](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
        {
            ApplicationUpgradeEnumerateFTsJobItem::Parameters jobItemParameters(
                entry,
                work,
                handler,
                JobItemCheck::DefaultAndOpen,
                *JobItemDescription::ApplicationUpgradeEnumerateFTs,
                move(ApplicationUpgradeEnumerateFTsJobItemContext()));

            return make_shared<ApplicationUpgradeEnumerateFTsJobItem>(move(jobItemParameters));
        });

    // Async execution is taking place now
    return UpgradeStateName::Invalid;
}

bool ApplicationUpgrade::EnumerateFTProcessor(HandlerParameters & handlerParameters, ApplicationUpgradeEnumerateFTsJobItemContext & context) const
{
    handlerParameters.AssertFTExists();
    handlerParameters.AssertFTIsOpen();

    LockedFailoverUnitPtr & failoverUnit = handlerParameters.FailoverUnit;

    bool isBeingDeleted = UpgradeSpecification.IsServiceTypeBeingDeleted(failoverUnit->ServiceDescription.Type);
    if (isBeingDeleted)
    {
        context.MarkAsDeletedByUpgrade();
        return true;;
    }

    ServiceModel::ServicePackageVersionInstance packageVersionInstance;
    bool isFound = UpgradeSpecification.GetUpgradeVersionForServiceType(failoverUnit->ServiceDescription.Type, packageVersionInstance);
    if (!isFound)
    {
        // This FT is not being upgraded
        return false;
    }

    Replica & localReplica = failoverUnit->LocalReplica;
                    
    ASSERT_IF(
        packageVersionInstance.InstanceId < localReplica.PackageVersionInstance.InstanceId, 
        "Invalid service code package instance: {0} appdesc:{1}", 
        *(failoverUnit), UpgradeSpecification);

    if (packageVersionInstance.InstanceId > localReplica.PackageVersionInstance.InstanceId)
    {
        // this ft is valid for upgrade
        context.MarkAsPartOfUpgrade(failoverUnit->ServiceDescription.Type.ServicePackageId.ServicePackageName, packageVersionInstance.Version.RolloutVersionValue);
        return true;
    }
    else
    {
        return false;
    }
}

void ApplicationUpgrade::OnEnumerationComplete(MultipleEntityWork const & work, AsyncStateActionCompleteCallback callback)
{
    ASSERT_IF(!packagesAffectedByUpgrade_.empty(), "Cannot have already set the packages to be downloaded at this point");

    // Iterate over all the job items and identify the FTs affected by this ugprade
    for(auto it = work.JobItems.cbegin(); it != work.JobItems.cend(); ++it)
    {
        auto & casted = (*it)->As<ApplicationUpgradeEnumerateFTsJobItem>();

        casted.Context.AddToListIfAffectedByUpgrade(casted.EntrySPtr, ftsAffectedByUpgrade_, ftsToBeDeleted_, packagesAffectedByUpgrade_);
    }

    // Upgrade only needs to be processed if there are any FTs that need to be closed (or restarted)
    // In a scenario where the RA has no FTs of any package being upgraded 
    // even if it has FTs that belong to service types being deleted
    // It is safe to send the reply to the FM and mark the upgrade as complete
    // The FM will send DeleteReplica for the FTs belonging to service types being deleted in parallel
    if (ftsAffectedByUpgrade_.empty())
    {
        SendReply();

        // Inform the state machine about the completion of this async operation
        callback(UpgradeStateName::Completed);

        return;
    }

    // Continue the upgrade
    callback(UpgradeStateName::ApplicationUpgrade_Download);
}

AsyncOperationSPtr ApplicationUpgrade::Download(AsyncCallback const & callback)
{
    ASSERT_IF(packagesAffectedByUpgrade_.empty(), "Cannot be upgrading if there is nothing to download");

    auto downloadSpec = CreateFilteredDownloadSpecification();

    return ra_.HostingObj.BeginDownloadApplication(downloadSpec, 
        callback,
        ra_.Root.CreateAsyncOperationRoot());
}

ApplicationDownloadSpecification ApplicationUpgrade::CreateFilteredDownloadSpecification() const
{
    vector<ServicePackageDownloadSpecification> packageDownloadsSpec;

    for(auto it = packagesAffectedByUpgrade_.cbegin(); it != packagesAffectedByUpgrade_.cend(); ++it)
    {
        packageDownloadsSpec.push_back(ServicePackageDownloadSpecification(it->first, it->second));
    }

    return ApplicationDownloadSpecification(UpgradeSpecification.ApplicationId, UpgradeSpecification.AppVersion, UpgradeSpecification.AppName, move(packageDownloadsSpec));
}

ApplicationUpgradeSpecification ApplicationUpgrade::CreateFilteredUpgradeSpecification() const
{
    vector<ServiceTypeRemovalSpecification> removedTypes = upgradeSpec_.RemovedTypes;
    vector<ServicePackageUpgradeSpecification> packageUpgrades;

    // Only add affected packages
    for(auto it = upgradeSpec_.PackageUpgrades.cbegin(); it != upgradeSpec_.PackageUpgrades.cend(); ++it)
    {
        if (packagesAffectedByUpgrade_.find(it->ServicePackageName) != packagesAffectedByUpgrade_.cend())
        {
            packageUpgrades.push_back(*it);
        }
    }

    return ApplicationUpgradeSpecification(
        upgradeSpec_.ApplicationId,
        upgradeSpec_.AppVersion,
        upgradeSpec_.AppName,
        upgradeSpec_.InstanceId,
        upgradeSpec_.UpgradeType,
        upgradeSpec_.IsMonitored,
        upgradeSpec_.IsManual,
        move(packageUpgrades),
        move(removedTypes),
        map<ServicePackageIdentifier, ServicePackageResourceGovernanceDescription>());
}

UpgradeStateName::Enum ApplicationUpgrade::EndDownload(AsyncOperationSPtr const & asyncOp)
{
    auto error = ra_.HostingObj.EndDownloadApplication(asyncOp);

    return error.IsSuccess() ? UpgradeStateName::ApplicationUpgrade_Analyze : UpgradeStateName::ApplicationUpgrade_DownloadFailed;
}

UpgradeStateName::Enum ApplicationUpgrade::Analyze()
{
    affectedRuntimes_.clear();
    auto error = ra_.HostingObj.AnalyzeApplicationUpgrade(CreateFilteredUpgradeSpecification(), affectedRuntimes_);

    RAEventSource::Events->UpgradePrintAnalyzeResult(ra_.NodeInstanceIdStr, GetActivityId(), error, static_cast<uint64>(affectedRuntimes_.size()));

    if (!error.IsSuccess())
    {
        return UpgradeStateName::ApplicationUpgrade_AnalyzeFailed;
    }
    else
    {
        return UpgradeStateName::ApplicationUpgrade_UpdateVersionsAndCloseIfNeeded;
    }
}

UpgradeStateName::Enum ApplicationUpgrade::UpdateVersionsAndCloseIfNeeded(AsyncStateActionCompleteCallback callback)
{
    // there must be FTs
    ASSERT_IF(ftsAffectedByUpgrade_.empty(), "There must be fts");

    // Create a multiple FT work. It is required to capture the callback
    auto work = make_shared<MultipleEntityWork>(
        GetActivityId(),
        [this, callback] (MultipleEntityWork const & inner, ReconfigurationAgent &)
        {
            OnVersionsUpdated(inner, callback);
        });

    auto handler = [this](HandlerParameters & handlerParameters, ApplicationUpgradeUpdateVersionJobItemContext & context)
    {
        return UpdateVersionAndCloseIfNeeded(handlerParameters, context);
    };

    ra_.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(
        work,
        ftsAffectedByUpgrade_,
        [handler](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
    {
        ApplicationUpgradeUpdateVersionJobItem::Parameters jobItemParameters(
            entry,
            work,
            handler,
            JobItemCheck::DefaultAndOpen,
            *JobItemDescription::ApplicationUpgradeUpdateVersionsAndCloseIfNeeded,
            move(ApplicationUpgradeUpdateVersionJobItemContext()));

        return make_shared<ApplicationUpgradeUpdateVersionJobItem>(move(jobItemParameters));
    });

    // Async processing
    return UpgradeStateName::Invalid;
}

bool ApplicationUpgrade::UpdateVersionAndCloseIfNeeded(HandlerParameters & handlerParameters, ApplicationUpgradeUpdateVersionJobItemContext & context) const
{
    handlerParameters.AssertFTExists();
    handlerParameters.AssertFTIsOpen();

    // The FT only needs to be closed if it belongs to a runtime that hosting mentions will be affected by the upgrade
    LockedFailoverUnitPtr & failoverUnit = handlerParameters.FailoverUnit;

    // If the FT does not have a runtime then by the end of this function the version will be updated
    // and any runtime that gets added will be of the older version and ignored 
    bool closeNeeded = affectedRuntimes_.find(failoverUnit->RuntimeId) != affectedRuntimes_.end();

    failoverUnit->OnApplicationCodeUpgrade(UpgradeSpecification, handlerParameters.ExecutionContext);

    if (closeNeeded && failoverUnit->LocalReplica.IsUp)
    {
        // let us close this replica 
        // this will be picked up by the message retry manager
        auto const closeMode = ReplicaCloseMode::Close;
        if (!failoverUnit->IsLocalReplicaClosed(closeMode))
        {
            ra_.CloseLocalReplica(handlerParameters, closeMode, ActivityDescription::Empty);
            context.MarkAsClosedForUpgrade();
        }
    }

    // The job item was successfully processed
    return true;
}

void ApplicationUpgrade::OnVersionsUpdated(MultipleEntityWork const & work, AsyncStateActionCompleteCallback callback) 
{   
    for(auto it = work.JobItems.cbegin(); it != work.JobItems.cend(); ++it)
    {
        auto const & casted = (*it)->As<ApplicationUpgradeUpdateVersionJobItem>();

        if (casted.Context.WasClosedAsPartOfUpgrade)
        {
            ftsClosedByUpgrade_.push_back(casted.EntrySPtr);
        }
    }

    if (ftsClosedByUpgrade_.empty() && ftsToBeDeleted_.empty())
    {
        // There are no FTs that need to be closed as part of this upgrade
        // There are also no FTs that the RA needs to wait for to be deleted 
        // It is now safe to call Upgrade API on hosting
        callback(UpgradeStateName::ApplicationUpgrade_Upgrade);
    }
    else
    {
        callback(UpgradeStateName::ApplicationUpgrade_WaitForCloseCompletion);       
    }
}

AsyncOperationSPtr ApplicationUpgrade::WaitForCloseCompletion(Common::AsyncCallback const & callback)
{
    auto fts = ftsClosedByUpgrade_;

    // Start the close completion check
    MultipleReplicaCloseCompletionCheckAsyncOperation::Parameters parameters = CreateParameters();
    parameters.CloseMode = ReplicaCloseMode::Close;
    parameters.FTsToClose = move(fts);
    parameters.MaxWaitBeforeTerminationEntry = &ra_.Config.ApplicationUpgradeMaxReplicaCloseDurationEntry;
    parameters.Callback = [](wstring const & activityId, EntityEntryBaseList const & pendingCloseFT, ReconfigurationAgent & ra)
    {
        RAEventSource::Events->UpgradeCloseCompletionCheckResult(ra.NodeInstanceIdStr, activityId, pendingCloseFT.size(), pendingCloseFT, EntityEntryBaseList());
    };

    /*
        Unfortunately application upgrade is special. It cannot use the IsReplicaClosed method
        This is because the PLB can place replicas on a node during app upgrade and RA must honor the placement
        If RA does not honor the placement (i.e. it rejects the message with an error) PLB-FM-RA get into a tight loop
        with PLB continuously placing the replica on the node and RA continuously failiing it.

        RA could ignore the message from FM but that does not work because there is no per service type information entity
        that is marked as 'upgrading' which would deny the request to place the replica. This works for fabric upgrade and
        node deactivate because the AddInstance/CreateReplica/AddPrimary check node activation state/node upgrade state under a lock

        Now that the per service type entity does exist - this code can be added and then this logic can use the standard
        IsReplicaClosed method
    */
    parameters.IsReplicaClosedFunction = [](ReplicaCloseMode, FailoverUnit & ft)
    {
        return !ft.LocalReplicaClosePending.IsSet;
    };

    return AsyncOperation::CreateAndStart<MultipleReplicaCloseCompletionCheckAsyncOperation>(move(parameters), callback, ra_.Root.CreateAsyncOperationRoot());
}

UpgradeStateName::Enum ApplicationUpgrade::EndWaitForCloseCompletion(Common::AsyncOperationSPtr const & op)
{
    MultipleReplicaCloseCompletionCheckAsyncOperation::End(op);

    return UpgradeStateName::ApplicationUpgrade_WaitForDropCompletion;
}

AsyncOperationSPtr ApplicationUpgrade::WaitForDropCompletion(Common::AsyncCallback const & callback)
{
    auto fts = ftsToBeDeleted_;

    // Start the close completion check
    MultipleReplicaCloseCompletionCheckAsyncOperation::Parameters parameters = CreateParameters();
    parameters.CloseMode = ReplicaCloseMode::Drop;
    parameters.FTsToClose = move(fts);

    /*
        For drop wait for inifinity and do not terminate the host. 

        This can be optimized later as it has never been hit and with the new CM changes before the upgrade can go through
        the delete must complete and the delete requires all replicas
    */
    parameters.MaxWaitBeforeTerminationEntry = nullptr; 
    parameters.Callback = [](wstring const & activityId, EntityEntryBaseList const & pendingCloseFT, ReconfigurationAgent & ra)
    {
        RAEventSource::Events->UpgradeCloseCompletionCheckResult(ra.NodeInstanceIdStr, activityId, pendingCloseFT.size(), EntityEntryBaseList(), pendingCloseFT);
    };

    return AsyncOperation::CreateAndStart<MultipleReplicaCloseCompletionCheckAsyncOperation>(move(parameters), callback, ra_.Root.CreateAsyncOperationRoot());
}

UpgradeStateName::Enum ApplicationUpgrade::EndWaitForDropCompletion(Common::AsyncOperationSPtr const & op)
{
    MultipleReplicaCloseCompletionCheckAsyncOperation::End(op);

    return UpgradeStateName::ApplicationUpgrade_ReplicaDownCompletionCheck;
}

UpgradeStateName::Enum ApplicationUpgrade::ReplicaDownCompletionCheck(AsyncStateActionCompleteCallback callback)
{
    ASSERT_IF(ftsAffectedByUpgrade_.empty(), "upgrade must affect FTs");

    auto work = make_shared<MultipleEntityWork>(
        GetActivityId(),
        [this, callback] (MultipleEntityWork const & inner, ReconfigurationAgent &)
        {
            OnReplicaDownCompletionCheckComplete(inner, callback);
        });

    Infrastructure::RAJobQueueManager::EntryJobItemList jobItems;

    AddJobItemsForReplicaDownCompletionCheckWork(work, ftsClosedByUpgrade_, jobItems);

    AddJobItemsForReplicaDownCompletionCheckWork(work, ftsToBeDeleted_, jobItems);

    ra_.JobQueueManager.AddJobItemsAndStartMultipleFailoverUnitWork(work, jobItems);

    // async execution will occur
    return UpgradeStateName::Invalid;
}

void ApplicationUpgrade::AddJobItemsForReplicaDownCompletionCheckWork(
    MultipleEntityWorkSPtr const & work,
    EntityEntryBaseList const & fts,
    Infrastructure::RAJobQueueManager::EntryJobItemList & list)
{
    auto handler = [this](HandlerParameters & handlerParameters, ApplicationUpgradeReplicaDownCompletionCheckJobItemContext & context)
    {
        return ReplicaDownCompletionCheckProcessor(handlerParameters, context);
    };

    for (auto const & it : fts)
    {
        ApplicationUpgradeReplicaDownCompletionCheckJobItem::Parameters jobItemParameters(
            it,
            work,
            handler,
            JobItemCheck::Default,
            *JobItemDescription::ApplicationUpgradeReplicaDownCompletionCheck,
            move(ApplicationUpgradeReplicaDownCompletionCheckJobItemContext()));

        auto ji = make_shared<ApplicationUpgradeReplicaDownCompletionCheckJobItem>(move(jobItemParameters));

        auto element = Infrastructure::RAJobQueueManager::EntryJobItemListElement(it, ji);

        list.push_back(move(element));
    }
}

bool ApplicationUpgrade::ReplicaDownCompletionCheckProcessor(HandlerParameters & handlerParameters, ApplicationUpgradeReplicaDownCompletionCheckJobItemContext & context) const
{
    handlerParameters.AssertFTExists();
    ASSERT_IF(context.IsReplyReceived, "The default state of the context is to assume that reply is not received. if reply is received processor sets this to true");
    
    LockedFailoverUnitPtr & ft = handlerParameters.FailoverUnit;

    if (ft->FMMessageStage != FMMessageStage::ReplicaDown)
    {
        context.MarkAsReplyReceived();
    }

    return true;
}

void ApplicationUpgrade::OnReplicaDownCompletionCheckComplete(MultipleEntityWork const & work, AsyncStateActionCompleteCallback callback)
{
    vector<FailoverUnitId> stillPendingAckFTs;

    for(auto it = work.JobItems.cbegin(); it != work.JobItems.cend(); ++it)
    {
        auto const & casted = (*it)->As<ApplicationUpgradeReplicaDownCompletionCheckJobItem>();
        casted.Context.AddToListIfReplyNotReceived(casted.Id, stillPendingAckFTs);
    }

    RAEventSource::Events->UpgradeReplicaDownCompletionCheckResult(ra_.NodeInstanceIdStr, GetActivityId(), static_cast<uint64>(stillPendingAckFTs.size()), stillPendingAckFTs);

    if (stillPendingAckFTs.empty())
    {
        callback(UpgradeStateName::ApplicationUpgrade_Upgrade);
    }
    else
    {
        callback(UpgradeStateName::ApplicationUpgrade_WaitForReplicaDownCompletionCheck);
    }
}

AsyncOperationSPtr ApplicationUpgrade::Upgrade(AsyncCallback const & callback)
{
    return ra_.HostingObj.BeginUpgradeApplication(CreateFilteredUpgradeSpecification(), 
        callback,
        ra_.Root.CreateAsyncOperationRoot());
}

UpgradeStateName::Enum ApplicationUpgrade::EndUpgrade(AsyncOperationSPtr const & asyncOp)
{
    auto error = ra_.HostingObj.EndUpgradeApplication(asyncOp);

    return error.IsSuccess() ? UpgradeStateName::ApplicationUpgrade_Finalize : UpgradeStateName::ApplicationUpgrade_UpgradeFailed;
}

UpgradeStateName::Enum ApplicationUpgrade::Finalize(AsyncStateActionCompleteCallback callback)
{
    ASSERT_IF(ftsAffectedByUpgrade_.empty(), "Upgrade must affect FTs");

    auto work = make_shared<MultipleEntityWork>(
        GetActivityId(),
        [this, callback] (MultipleEntityWork const & inner, ReconfigurationAgent &)
        {
            OnUpgradeFinalized(inner, callback);
        });

    auto handler = [this](HandlerParameters & handlerParameters, JobItemContextBase & context)
    {
        return FinalizeUpgradeFTProcessor(handlerParameters, context);
    };

    ra_.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(
        work,
        ftsAffectedByUpgrade_,
        [handler](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
        {
            ApplicationUpgradeFinalizeFTJobItem::Parameters jobItemParameters(
                entry,
                work,
                handler,
                JobItemCheck::Default,
                *JobItemDescription::ApplicationUpgradeFinalizeFT,
                move(JobItemContextBase()));

            return make_shared<ApplicationUpgradeFinalizeFTJobItem>(move(jobItemParameters));
        });

    // Async operation
    return UpgradeStateName::Invalid;
}

bool ApplicationUpgrade::FinalizeUpgradeFTProcessor(HandlerParameters & handlerParameters, JobItemContextBase & ) const
{
    handlerParameters.AssertFTExists();
    handlerParameters.AssertHasWork();

    handlerParameters.RA.ReopenDownReplica(handlerParameters);

    return true;    
}

void ApplicationUpgrade::OnUpgradeFinalized(MultipleEntityWork const &, AsyncStateActionCompleteCallback callback)
{
    // Send Upgrade reply
    // Ask the reopen work manager to perform the reopen related work
    SendReply();
    callback(UpgradeStateName::Completed);       
}

void ApplicationUpgrade::WriteToEtw(uint16 contextSequenceId) const
{
    RAEventSource::Events->ApplicationUpgrade(contextSequenceId, GetActivityId(), UpgradeSpecification);
}

void ApplicationUpgrade::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << "ApplicationUpgrade(" << GetActivityId() << "). Specification = [" << UpgradeSpecification << "]";
}

MultipleReplicaCloseCompletionCheckAsyncOperation::Parameters ApplicationUpgrade::CreateParameters()
{
    MultipleReplicaCloseCompletionCheckAsyncOperation::Parameters parameters;

    parameters.ActivityId = GetActivityId();
    parameters.MonitoringIntervalEntry = &ra_.Config.RAUpgradeProgressCheckIntervalEntry;
    parameters.RA = &ra_;
    parameters.Clock = ra_.ClockSPtr;
    parameters.TerminateReason = Hosting::TerminateServiceHostReason::ApplicationUpgrade;

    return parameters;
}
