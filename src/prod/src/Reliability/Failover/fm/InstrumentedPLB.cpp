// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "InstrumentedPLB.h"

using namespace std;
using namespace Common;
using namespace Reliability::FailoverManagerComponent;
using namespace Reliability::LoadBalancingComponent;

InstrumentedPLB::InstrumentedPLB(
    FailoverManager const& fm,
    IPlacementAndLoadBalancingUPtr && plb)
    : fm_(fm), plb_(move(plb))
{
}

InstrumentedPLB::~InstrumentedPLB()
{
}

InstrumentedPLBUPtr InstrumentedPLB::CreateInstrumentedPLB(
    FailoverManager const& fm,
    IPlacementAndLoadBalancingUPtr && plb)
{
    return make_unique<InstrumentedPLB>(fm, move(plb));
}

void InstrumentedPLB::SetMovementEnabled(bool constraintCheckEnabled, bool balancingEnabled)
{
    Stopwatch sw;
    sw.Start();

    plb_->SetMovementEnabled(constraintCheckEnabled, balancingEnabled);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbSetMovementEnabledBase.Increment();
    fm_.FailoverUnitCounters->PlbSetMovementEnabled.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::SetMovementEnabled, sw.ElapsedMilliseconds);
    }
}

void InstrumentedPLB::UpdateNode(Reliability::LoadBalancingComponent::NodeDescription && nodeDescription)
{
    plb_->UpdateNode(move(nodeDescription));
}

void InstrumentedPLB::UpdateNode(Reliability::LoadBalancingComponent::NodeDescription && nodeDescription, int64 & plbElapsedMilliseconds)
{
    Stopwatch sw;
    sw.Start();

    UpdateNode(move(nodeDescription));

    sw.Stop();
    plbElapsedMilliseconds = sw.ElapsedMilliseconds;

    fm_.FailoverUnitCounters->PlbUpdateNodeBase.Increment();
    fm_.FailoverUnitCounters->PlbUpdateNode.IncrementBy(static_cast<PerformanceCounterValue>(plbElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::UpdateNode, plbElapsedMilliseconds);
    }
}

void InstrumentedPLB::UpdateServiceType(ServiceTypeDescription && serviceTypeDescription)
{
    plb_->UpdateServiceType(move(serviceTypeDescription));
}

void InstrumentedPLB::UpdateServiceType(ServiceTypeDescription && serviceTypeDescription, __out int64 & plbElapsedMilliseconds)
{
    Stopwatch sw;
    sw.Start();

    UpdateServiceType(move(serviceTypeDescription));

    sw.Stop();

    plbElapsedMilliseconds = sw.ElapsedMilliseconds;

    fm_.FailoverUnitCounters->PlbUpdateServiceTypeBase.Increment();
    fm_.FailoverUnitCounters->PlbUpdateServiceType.IncrementBy(static_cast<PerformanceCounterValue>(plbElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::UpdateServiceType, plbElapsedMilliseconds);
    }
}


void InstrumentedPLB::DeleteServiceType(wstring const& serviceTypeName)
{
    plb_->DeleteServiceType(serviceTypeName);
}

void InstrumentedPLB::DeleteServiceType(wstring const& serviceTypeName, __out int64 & plbElapsedMilliseconds)
{
    Stopwatch sw;
    sw.Start();

    DeleteServiceType(serviceTypeName);

    sw.Stop();

    plbElapsedMilliseconds = sw.ElapsedMilliseconds;

    fm_.FailoverUnitCounters->PlbDeleteServiceTypeBase.Increment();
    fm_.FailoverUnitCounters->PlbDeleteServiceType.IncrementBy(static_cast<PerformanceCounterValue>(plbElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::DeleteServiceType, plbElapsedMilliseconds);
    }
}

Common::ErrorCode InstrumentedPLB::UpdateApplication(ApplicationDescription && applicationDescription, bool forceUpdate)
{
    Common::ErrorCode error = plb_->UpdateApplication(move(applicationDescription), forceUpdate);

    return error;
}

Common::ErrorCode InstrumentedPLB::UpdateApplication(ApplicationDescription && applicationDescription, int64 & plbElapsedMilliseconds, bool forceUpdate)
{
    Stopwatch sw;
    sw.Start();

    Common::ErrorCode error = UpdateApplication(move(applicationDescription), forceUpdate);

    sw.Stop();

    plbElapsedMilliseconds = sw.ElapsedMilliseconds;

    fm_.FailoverUnitCounters->PlbUpdateApplicationBase.Increment();
    fm_.FailoverUnitCounters->PlbUpdateApplication.IncrementBy(static_cast<PerformanceCounterValue>(plbElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::UpdateApplication, plbElapsedMilliseconds);
    }

    return error;
}

void InstrumentedPLB::DeleteApplication(std::wstring const& applicationName, bool forceUpdate)
{
    plb_->DeleteApplication(applicationName, forceUpdate);
}

void InstrumentedPLB::DeleteApplication(std::wstring const& applicationName, int64 & plbElapsedMilliseconds, bool forceUpdate)
{
    Stopwatch sw;
    sw.Start();

    DeleteApplication(applicationName, forceUpdate);

    sw.Stop();

    plbElapsedMilliseconds = sw.ElapsedMilliseconds;

    fm_.FailoverUnitCounters->PlbDeleteApplicationBase.Increment();
    fm_.FailoverUnitCounters->PlbDeleteApplication.IncrementBy(static_cast<PerformanceCounterValue>(plbElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::DeleteApplication, plbElapsedMilliseconds);
    }
}

ErrorCode InstrumentedPLB::UpdateService(Reliability::LoadBalancingComponent::ServiceDescription && serviceDescription, bool forceUpdate)
{
    ErrorCode ret = plb_->UpdateService(move(serviceDescription), forceUpdate);
    return ret;
}

ErrorCode InstrumentedPLB::UpdateService(Reliability::LoadBalancingComponent::ServiceDescription && serviceDescription, int64 & plbElapsedMilliseconds, bool forceUpdate)
{
    Stopwatch sw;
    sw.Start();

    ErrorCode ret = plb_->UpdateService(move(serviceDescription), forceUpdate);

    sw.Stop();

    plbElapsedMilliseconds = sw.ElapsedMilliseconds;

    fm_.FailoverUnitCounters->PlbUpdateServiceBase.Increment();
    fm_.FailoverUnitCounters->PlbUpdateService.IncrementBy(static_cast<PerformanceCounterValue>(plbElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::UpdateService, plbElapsedMilliseconds);
    }

    return ret;
}

void InstrumentedPLB::DeleteService(std::wstring const& serviceName)
{
    plb_->DeleteService(serviceName);
}


void InstrumentedPLB::DeleteService(std::wstring const& serviceName, int64 & plbElapsedMilliseconds)
{
    Stopwatch sw;
    sw.Start();

    plb_->DeleteService(serviceName);

    sw.Stop();

    plbElapsedMilliseconds = sw.ElapsedMilliseconds;

    fm_.FailoverUnitCounters->PlbDeleteServiceBase.Increment();
    fm_.FailoverUnitCounters->PlbDeleteService.IncrementBy(static_cast<PerformanceCounterValue>(plbElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::DeleteService, plbElapsedMilliseconds);
    }
}

void InstrumentedPLB::UpdateFailoverUnit(Reliability::LoadBalancingComponent::FailoverUnitDescription && failoverUnitDescription)
{
    plb_->UpdateFailoverUnit(move(failoverUnitDescription));
}

void InstrumentedPLB::UpdateFailoverUnit(Reliability::LoadBalancingComponent::FailoverUnitDescription && failoverUnitDescription, int64 & plbElapsedMilliseconds)
{
    Stopwatch sw;
    sw.Start();

    UpdateFailoverUnit(move(failoverUnitDescription));

    sw.Stop();

    plbElapsedMilliseconds = sw.ElapsedMilliseconds;

    fm_.FailoverUnitCounters->PlbUpdateFailoverUnitBase.Increment();
    fm_.FailoverUnitCounters->PlbUpdateFailoverUnit.IncrementBy(static_cast<PerformanceCounterValue>(plbElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::UpdateFailoverUnit, plbElapsedMilliseconds);
    }
}

void InstrumentedPLB::DeleteFailoverUnit(wstring && serviceName, Common::Guid failoverUnitId)
{
    plb_->DeleteFailoverUnit(move(serviceName), failoverUnitId);
}

void InstrumentedPLB::DeleteFailoverUnit(wstring && serviceName, Common::Guid failoverUnitId, int64 & plbElapsedMilliseconds)
{
    Stopwatch sw;
    sw.Start();

    DeleteFailoverUnit(move(serviceName), failoverUnitId);

    sw.Stop();

    plbElapsedMilliseconds = sw.ElapsedMilliseconds;

    fm_.FailoverUnitCounters->PlbDeleteFailoverUnitBase.Increment();
    fm_.FailoverUnitCounters->PlbDeleteFailoverUnit.IncrementBy(static_cast<PerformanceCounterValue>(plbElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::DeleteFailoverUnit, plbElapsedMilliseconds);
    }
}

void InstrumentedPLB::UpdateLoadOrMoveCost(LoadOrMoveCostDescription && loadOrMoveCost)
{
    Stopwatch sw;
    sw.Start();

    plb_->UpdateLoadOrMoveCost(move(loadOrMoveCost));

    sw.Stop();

    fm_.FailoverUnitCounters->PlbUpdateLoadOrMoveCostBase.Increment();
    fm_.FailoverUnitCounters->PlbUpdateLoadOrMoveCost.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::UpdateLoadOrMoveCost, sw.ElapsedMilliseconds);
    }
}

void InstrumentedPLB::UpdateClusterUpgrade(bool isUpgradeInProgress, std::set<std::wstring> && completedUDs)
{
    Stopwatch sw;
    sw.Start();

    plb_->UpdateClusterUpgrade(isUpgradeInProgress, move(completedUDs));

    sw.Stop();

    fm_.FailoverUnitCounters->PlbUpdateClusterUpgradeBase.Increment();
    fm_.FailoverUnitCounters->PlbUpdateClusterUpgrade.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::UpdateClusterUpgrade, sw.ElapsedMilliseconds);
    }
}

Common::ErrorCode InstrumentedPLB::ResetPartitionLoad(Reliability::FailoverUnitId const & failoverUnitId, wstring const & serviceName, bool isStateful)
{
    Stopwatch sw;
    sw.Start();

    Common::ErrorCode ret = plb_->ResetPartitionLoad(failoverUnitId, serviceName, isStateful);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbResetPartitionLoadBase.Increment();
    fm_.FailoverUnitCounters->PlbResetPartitionLoad.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::ResetPartitionLoad, sw.ElapsedMilliseconds);
    }

    return ret;
}

int InstrumentedPLB::CompareNodeForPromotion(wstring const& serviceName, Common::Guid failoverUnitId, Federation::NodeId node1, Federation::NodeId node2)
{
    Stopwatch sw;
    sw.Start();

    int ret = plb_->CompareNodeForPromotion(serviceName, failoverUnitId, node1, node2);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbCompareNodeForPromotionBase.Increment();
    fm_.FailoverUnitCounters->PlbCompareNodeForPromotion.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::CompareNodeForPromotion, sw.ElapsedMilliseconds);
    }

    return ret;
}

void InstrumentedPLB::Dispose()
{
    Stopwatch sw;
    sw.Start();

    plb_->Dispose();

    sw.Stop();

    fm_.FailoverUnitCounters->PlbDisposeBase.Increment();
    fm_.FailoverUnitCounters->PlbDispose.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::Dispose, sw.ElapsedMilliseconds);
    }
}

Common::ErrorCode InstrumentedPLB::GetClusterLoadInformationQueryResult(ServiceModel::ClusterLoadInformationQueryResult & queryResult)
{
    Stopwatch sw;
    sw.Start();

    Common::ErrorCode ret = plb_->GetClusterLoadInformationQueryResult(queryResult);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbGetClusterLoadInformationQueryResultBase.Increment();
    fm_.FailoverUnitCounters->PlbGetClusterLoadInformationQueryResult.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::GetClusterLoadInformationQueryResult, sw.ElapsedMilliseconds);
    }

    return ret;
}

Common::ErrorCode InstrumentedPLB::GetNodeLoadInformationQueryResult(Federation::NodeId nodeId, ServiceModel::NodeLoadInformationQueryResult & queryResult, bool onlyViolations) const
{
    Stopwatch sw;
    sw.Start();

    Common::ErrorCode ret = plb_->GetNodeLoadInformationQueryResult(nodeId, queryResult, onlyViolations);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbGetNodeLoadInformationQueryResultBase.Increment();
    fm_.FailoverUnitCounters->PlbGetNodeLoadInformationQueryResult.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::GetNodeLoadInformationQueryResult, sw.ElapsedMilliseconds);
    }

    return ret;
}

ServiceModel::UnplacedReplicaInformationQueryResult InstrumentedPLB::GetUnplacedReplicaInformationQueryResult(std::wstring const& serviceName, Common::Guid const& partitionId, bool onlyQueryPrimaries)
{
    Stopwatch sw;
    sw.Start();

    ServiceModel::UnplacedReplicaInformationQueryResult ret = plb_->GetUnplacedReplicaInformationQueryResult(serviceName, partitionId, onlyQueryPrimaries);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbGetUnplacedReplicaInformationQueryResultBase.Increment();
    fm_.FailoverUnitCounters->PlbGetUnplacedReplicaInformationQueryResult.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::GetUnbalancedReplicaInformationQueryResult, sw.ElapsedMilliseconds);
    }

    return ret;
}

Common::ErrorCode InstrumentedPLB::GetApplicationLoadInformationResult(std::wstring const & applicationName, ServiceModel::ApplicationLoadInformationQueryResult & result)
{
    Stopwatch sw;
    sw.Start();

    Common::ErrorCode ret = plb_->GetApplicationLoadInformationResult(applicationName, result);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbGetApplicationLoadInformationResultBase.Increment();
    fm_.FailoverUnitCounters->PlbGetApplicationLoadInformationResult.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::GetApplicationLoadInformationResult, sw.ElapsedMilliseconds);
    }

    return ret;
}

Common::ErrorCode InstrumentedPLB::TriggerPromoteToPrimary(std::wstring const& serviceName, Guid const& failoverUnitId, Federation::NodeId newPrimary)
{
    Stopwatch sw;
    sw.Start();

    Common::ErrorCode ret = plb_->TriggerPromoteToPrimary(serviceName, failoverUnitId, newPrimary);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbTriggerPromoteToPrimaryBase.Increment();
    fm_.FailoverUnitCounters->PlbTriggerPromoteToPrimary.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::TriggerPromoteToPrimary, sw.ElapsedMilliseconds);
    }

    return ret;
}

Common::ErrorCode InstrumentedPLB::TriggerSwapPrimary(
    std::wstring const& serviceName,
    Guid const& failoverUnitId,
    Federation::NodeId currentPrimary,
    Federation::NodeId & newPrimary,
    bool force /* = false */,
    bool chooseRandom /* = false */)
{
    Stopwatch sw;
    sw.Start();

    Common::ErrorCode ret = plb_->TriggerSwapPrimary(serviceName, failoverUnitId, currentPrimary, newPrimary, force, chooseRandom);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbTriggerSwapPrimaryBase.Increment();
    fm_.FailoverUnitCounters->PlbTriggerSwapPrimary.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::TriggerSwapPrimary, sw.ElapsedMilliseconds);
    }

    return ret;
}

Common::ErrorCode InstrumentedPLB::TriggerMoveSecondary(
    std::wstring const& serviceName,
    Guid const& failoverUnitId, Federation::NodeId currentSecondary,
    Federation::NodeId & newSecondary,
    bool force /* = false */,
    bool chooseRandom /*= false*/)
{
    Stopwatch sw;
    sw.Start();

    Common::ErrorCode ret = plb_->TriggerMoveSecondary(serviceName, failoverUnitId, currentSecondary, newSecondary, force, chooseRandom);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbTriggerMoveSecondaryBase.Increment();
    fm_.FailoverUnitCounters->PlbTriggerMoveSecondary.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::TriggerMoveSecondary, sw.ElapsedMilliseconds);
    }

    return ret;
}

void InstrumentedPLB::OnFMBusy()
{
    Stopwatch sw;
    sw.Start();

    plb_->OnFMBusy();

    sw.Stop();

    fm_.FailoverUnitCounters->PlbOnFMBusyBase.Increment();
    fm_.FailoverUnitCounters->PlbOnFMBusy.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::OnFMBusy, sw.ElapsedMilliseconds);
    }
}

void InstrumentedPLB::OnDroppedPLBMovement(Common::Guid const& failoverUnitId, Reliability::PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId)
{
    Stopwatch sw;
    sw.Start();

    plb_->OnDroppedPLBMovement(failoverUnitId, reason, decisionId);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbOnDroppedPLBMovementBase.Increment();
    fm_.FailoverUnitCounters->PlbOnDroppedPLBMovement.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::OnDroppedPLBMovement, sw.ElapsedMilliseconds);
    }
}

void InstrumentedPLB::OnDroppedPLBMovements(std::map<Common::Guid, FailoverUnitMovement> && droppedMovements, Reliability::PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId)
{
    size_t numberofDroppedMovements = droppedMovements.size();
    Stopwatch sw;
    sw.Start();

    plb_->OnDroppedPLBMovements(move(droppedMovements), reason, decisionId);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbOnDroppedPLBMovementsBase.IncrementBy(numberofDroppedMovements);
    fm_.FailoverUnitCounters->PlbOnDroppedPLBMovements.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::OnDroppedPLBMovements, sw.ElapsedMilliseconds);
    }
}

void InstrumentedPLB::OnExecutePLBMovement(Common::Guid const & partitionId)
{
    Stopwatch sw;
    sw.Start();

    plb_->OnExecutePLBMovement(partitionId);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbOnExecutePLBMovementBase.Increment();
    fm_.FailoverUnitCounters->PlbOnExecutePLBMovement.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::OnExecutePLBMovement, sw.ElapsedMilliseconds);
    }
}

Common::ErrorCode InstrumentedPLB::ToggleVerboseServicePlacementHealthReporting(bool enabled)
{
    Stopwatch sw;
    sw.Start();

    Common::ErrorCode ret = plb_->ToggleVerboseServicePlacementHealthReporting(enabled);

    sw.Stop();

    fm_.FailoverUnitCounters->PlbToggleVerboseServicePlacementHealthReportingBase.Increment();
    fm_.FailoverUnitCounters->PlbToggleVerboseServicePlacementHealthReporting.IncrementBy(static_cast<PerformanceCounterValue>(sw.ElapsedMilliseconds));

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::ToggleVerboseServicePlacementHealthReporting, sw.ElapsedMilliseconds);
    }

    return ret;
}

void InstrumentedPLB::OnSafetyCheckAcknowledged(ServiceModel::ApplicationIdentifier const & appId)
{
    Stopwatch sw;
    sw.Start();

    plb_->OnSafetyCheckAcknowledged(appId);

    sw.Stop();
    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::OnSafetyCheckAcknowledged, sw.ElapsedMilliseconds);
    }
}

void InstrumentedPLB::UpdateAvailableImagesPerNode(std::wstring const& nodeId, std::vector<std::wstring>const& images)
{
    Stopwatch sw;
    sw.Start();

    plb_->UpdateAvailableImagesPerNode(nodeId, images);

    sw.Stop();

    if (sw.Elapsed > FailoverConfig::GetConfig().PlbUpdateTimeLimit)
    {
        fm_.Events.PlbFunctionCallSlow(PlbApiCallName::UpdateAvailableImagesPerNode, sw.ElapsedMilliseconds);
    }
}