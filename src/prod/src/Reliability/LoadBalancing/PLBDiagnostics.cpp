// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PLBDiagnostics.h"
#include "PLBConfig.h"
#include "PartitionEntry.h"
#include "ServiceEntry.h"
#include "NodeEntry.h"
#include "ServiceModel/SystemServiceApplicationNameHelper.h"
#include "client/INotificationClientSettings.h"
#include "client/FabricClientInternalSettingsHolder.h"
#include "client/HealthReportingComponent.h"
#include "client/HealthReportingTransport.h"


using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;
using namespace ServiceModel;


PLBDiagnostics::PLBDiagnostics(Client::HealthReportingComponentSPtr healthClient,
    bool const & verbosePlacementHealthReportingEnabled,
    std::vector<Node> const & plbNodes,
    ServiceDomainTable const & serviceDomainTable,
    PLBEventSource const& trace)
    : healthClient_(healthClient),
    verbosePlacementHealthReportingEnabled_(verbosePlacementHealthReportingEnabled),
    plbNodes_(plbNodes),
    plbServiceDomainTable_(serviceDomainTable),
    trace_(trace),
    balancingViolationHealthReports_(),
    constraintViolationHealthReports_(),
    unfixableConstraintViolationHealthReports_(),
    unplacedReplicaHealthReports_(),
    unswappableUpgradePartitionHealthReports_(),
    placementQueue_(),
    upgradeSwapQueue_(),
    constraintDiagnosticsReplicas_(),
    placementDiagnosticsTableSPtr_(make_shared<PlacementDiagnosticsTable>()),
    constraintDiagnosticsTableSPtr_(make_shared<PlacementDiagnosticsTable>()),
    cachedConstraintDiagnosticsTableSPtr_(make_shared<PlacementDiagnosticsTable>()),
    queryableDiagnosticsTableSPtr_(make_shared<PlacementDiagnosticsTable>()),
    upgradeSwapDiagnosticsTableSPtr_(make_shared<PartitionDiagnosticsTable>()),
    queryableUpgradeSwapDiagnosticsTableSPtr_(make_shared<PartitionDiagnosticsTable>()),
    fmConsecutiveDroppedPLBMovementsTableSPtr_(make_shared<FMConsecutiveDroppedPLBMovementsTable>()),
    schedulingDiagnosticsSPtr_(make_shared<SchedulingDiagnostics>())
{
}

void PLBDiagnostics::ExecuteUnderPDTLock(std::function<void()> lockedOperation)
{
    AcquireWriteLock grab(placementDiagnosticsTableLock_);

    lockedOperation();
}

void PLBDiagnostics::ExecuteUnderUSPDTLock(std::function<void()> lockedOperation)
{
    AcquireWriteLock grab(upgradeSwapDiagnosticsTableLock_);

    lockedOperation();
}

bool PLBDiagnostics::ReportAsHealthWarning(PlacementReplica const* itReplica, IConstraintUPtr const & it)
{
    switch (PLBConfig::GetConfig().ConstraintViolationReportingPolicy)
    {
    case 0:
        return true;

    case 1:
        return it->Priority == 0;

    case 2:
        return !((it->Priority > 0) && (it->Type == IConstraint::FaultDomain || it->Type == IConstraint::UpgradeDomain));

    case 3:
    {
        bool isWarning = true;
        //TODO: Implement Smarter Logic Here Eventually -- Such as if the cluster is under FabricUpgrade
        if ((it->Priority > 0) && (it->Type == IConstraint::FaultDomain || it->Type == IConstraint::UpgradeDomain) && itReplica->Partition->IsInUpgrade)
        {
            isWarning = false;
        }
        return isWarning;
    }

    default:
        return false;
    }
}

void PLBDiagnostics::TrackDroppedPLBMovement(Common::Guid  const & partitionId)
{
    auto const & config = PLBConfig::GetConfig();
    TimeSpan const timeSpan = config.PLBHealthEventTTL;

    AcquireWriteLock grab(droppedMovementTableLock_);

    if (fmConsecutiveDroppedPLBMovementsTableSPtr_->count(partitionId))
    {
        auto & numConsecutiveDrops = fmConsecutiveDroppedPLBMovementsTableSPtr_->at(partitionId);
        ++numConsecutiveDrops;
    }
    else
    {
        fmConsecutiveDroppedPLBMovementsTableSPtr_->insert(std::pair<Common::Guid, size_t>(partitionId, 1));
    }

    if (healthClient_
        && (config.ConsecutiveDroppedMovementsHealthReportLimit > 0)
        && (fmConsecutiveDroppedPLBMovementsTableSPtr_->at(partitionId) > config.ConsecutiveDroppedMovementsHealthReportLimit))
    {

        auto healthInfo = ServiceModel::EntityHealthInformation::CreatePartitionEntityHealthInformation(partitionId);
        auto attributeList = ServiceModel::AttributeList();
        auto violationString = wformatString("Consecutively_Via_FM");
        auto healthDetails = HMResource::GetResources().PLBMovementsDroppedByFMDescription;

        ServiceModel::HealthReport consecutiveDroppedMovementsHealthReport = ServiceModel::HealthReport::CreateSystemHealthReport(
            config.UseCRMPublicName ? SystemHealthReportCode::CRM_MovementsDropped : SystemHealthReportCode::PLB_MovementsDropped,
            move(healthInfo),
            move(violationString),
            move(healthDetails),
            timeSpan,
            move(attributeList));

        consecutiveDroppedMovementsHealthReports_.push_back(move(consecutiveDroppedMovementsHealthReport));
    }
}

void PLBDiagnostics::TrackExecutedPLBMovement(Common::Guid const & partitionId)
{
    auto const & config = PLBConfig::GetConfig();
    TimeSpan const timeSpan = config.PLBHealthEventTTL;

    AcquireWriteLock grab(droppedMovementTableLock_);

    if (fmConsecutiveDroppedPLBMovementsTableSPtr_->count(partitionId) == 1)
    {
        fmConsecutiveDroppedPLBMovementsTableSPtr_->erase(partitionId);
    }
}

bool PLBDiagnostics::IsPLBMovementBeingDropped(Common::Guid const & partitionId)
{
    auto const & config = PLBConfig::GetConfig();
    AcquireReadLock grab(droppedMovementTableLock_);

    return fmConsecutiveDroppedPLBMovementsTableSPtr_->count(partitionId) ? (fmConsecutiveDroppedPLBMovementsTableSPtr_->at(partitionId) > config.ConsecutiveDroppedMovementsHealthReportLimit) : false;
}

void PLBDiagnostics::TrackNodeCapacityViolation(std::function< Common::ErrorCode (Federation::NodeId, ServiceModel::NodeLoadInformationQueryResult &)> nodeLoadPopulator)
{
    TimeSpan const timeSpan = PLBConfig::GetConfig().PLBHealthEventTTL;
    auto const & config = PLBConfig::GetConfig();

    if (healthClient_)
    {
        vector<ServiceModel::HealthReport> nodeViolationReports;

        for (auto const& it : plbNodes_)
        {
            ServiceModel::NodeLoadInformationQueryResult nodeViolationData;

           if (it.NodeDescriptionObj.IsUp && 
                nodeLoadPopulator(it.NodeDescriptionObj.NodeId, nodeViolationData).IsSuccess())
            {
                auto const& nodeName = it.NodeDescriptionObj.NodeName;
                auto nodeInstanceID = it.NodeDescriptionObj.NodeInstance.InstanceId;

                for (auto itMetricInfo = nodeViolationData.NodeLoadMetricInformation.begin(); itMetricInfo != nodeViolationData.NodeLoadMetricInformation.end(); ++itMetricInfo)
                {
                    auto healthInfo = ServiceModel::EntityHealthInformation::CreateNodeEntityHealthInformation(it.NodeDescriptionObj.NodeId.IdValue, nodeName, nodeInstanceID);
                    auto attributeList = ServiceModel::AttributeList();
                    attributeList.AddAttribute(*ServiceModel::HealthAttributeNames::NodeName, nodeName);

                    nodeViolationReports.push_back(ServiceModel::HealthReport::CreateSystemHealthReport(
                        config.UseCRMPublicName ? SystemHealthReportCode::CRM_NodeCapacityViolation : SystemHealthReportCode::PLB_NodeCapacityViolation,
                        move(healthInfo),
                        itMetricInfo->Name,
                        wformatString("[Node, Metric, Capacity, Load] = [{0}, {1}, {2}, {3}]", nodeName, itMetricInfo->Name, itMetricInfo->NodeCapacity, itMetricInfo->NodeLoad),
                        timeSpan,
                        move(attributeList)));
                }
            }
            else
            {
                break;
            }
        }

        //Emit NodeCapacity Health Warnings
         healthClient_->AddHealthReports(move(nodeViolationReports));
    }

}

void PLBDiagnostics::TrackConstraintViolation(PlacementReplica const* itReplica, IConstraintUPtr const & it, std::shared_ptr<IConstraintDiagnosticsData> diagnosticsDataSPtr)
{
    TimeSpan const timeSpan = PLBConfig::GetConfig().PLBHealthEventTTL;
    auto const & config = PLBConfig::GetConfig();

    if (healthClient_ && (it)->Type != IConstraint::NodeCapacity) // NodeCapacity is handled in PLB::BeginRefresh as it's not PartitionBased and it's more convenient to access the required data for the health reports from there
    {

        auto reportingName = SystemServiceApplicationNameHelper::IsSystemServiceName((itReplica)->Partition->Service->Name) ? SystemServiceApplicationNameHelper::GetPublicServiceName((itReplica)->Partition->Service->Name) : (itReplica)->Partition->Service->Name;
        auto healthInfo = ServiceModel::EntityHealthInformation::CreatePartitionEntityHealthInformation((itReplica)->Partition->PartitionId);
        auto attributeList = ServiceModel::AttributeList();
        auto violationString = StringUtility::ToWString((it)->Type);
        auto healthDetails = wformatString("{0} {2} Partition {1} is violating the Constraint: {3}",
            reportingName,
            (itReplica)->Partition->PartitionId,
            StringUtility::ToWString((itReplica)->Role),
            ConstraintDetails((it)->Type, (itReplica), diagnosticsDataSPtr));

        attributeList.AddAttribute(*ServiceModel::HealthAttributeNames::ServiceName, move(reportingName));

        auto reportCode1 = config.UseCRMPublicName ? SystemHealthReportCode::CRM_ReplicaConstraintViolation : SystemHealthReportCode::PLB_ReplicaConstraintViolation;
        auto reportCode2 = config.UseCRMPublicName ? SystemHealthReportCode::CRM_ReplicaSoftConstraintViolation : SystemHealthReportCode::PLB_ReplicaSoftConstraintViolation;

        ServiceModel::HealthReport constraintViolationHealthReport = ServiceModel::HealthReport::CreateSystemHealthReport(
            ReportAsHealthWarning(itReplica, it) ? reportCode1 : reportCode2,
            move(healthInfo),
            move(violationString),
            move(healthDetails),
            timeSpan,
            move(attributeList));

        constraintViolationHealthReports_.push_back(move(constraintViolationHealthReport));
    }
}

std::wstring PLBDiagnostics::ServiceDomainMetricsString(DomainId const & domainId, bool forProperty)
{
    std::wstring metricsString;
    StringWriter writer(metricsString);

    writer.Write("{ ");

    if (plbServiceDomainTable_.count(ServiceDomain::GetDomainIdPrefix(domainId)))
    {
        auto& metrics = plbServiceDomainTable_.at(ServiceDomain::GetDomainIdPrefix(domainId)).Metrics;

        if (forProperty)
        {
            writer.Write((metrics.begin())->first);

            if (metrics.size() > 1)
            {
                writer.Write(", ...");
            }
        }
        else
        {
            for (auto iter = metrics.begin(); (iter != metrics.end()) && ( std::distance(metrics.begin(), iter) < PLBConfig::GetConfig().DetailedMetricListLimit); ++iter)
            {
                writer.Write("{0}", iter->first);
                if (std::distance(iter, metrics.end()) != 1)
                {
                    writer.Write(" ");
                }
            }

            if (plbServiceDomainTable_.size() > PLBConfig::GetConfig().DetailedMetricListLimit)
            {
                writer.Write("...");
            }
        }
    }

    writer.Write(" }");
    return metricsString;
}

void PLBDiagnostics::TrackBalancingFailure(CandidateSolution & oldSolution, CandidateSolution & newSolution, DomainId const & domainId)
{
    auto SDDiagPtr =  SchedulersDiagnostics->GetLatestStageDiagnostics(domainId);

    PLBConfig const& config = PLBConfig::GetConfig();
    TimeSpan const timeSpan = config.PLBHealthEventTTL;
    double scoreImprovement = oldSolution.AvgStdDev - newSolution.AvgStdDev;

    if (healthClient_)
    {
        //auto reportingName = SystemServiceApplicationNameHelper::IsSystemServiceName((itReplica)->Partition->Service->Name) ? SystemServiceApplicationNameHelper::GetPublicServiceName((itReplica)->Partition->Service->Name) : (itReplica)->Partition->Service->Name;
        auto healthInfo = ServiceModel::EntityHealthInformation::CreateClusterEntityHealthInformation();
        auto attributeList = ServiceModel::AttributeList();
        auto metricsString = SDDiagPtr->metricProperty_;
        auto healthDetails = wformatString("ScoreImprovement: {0} -- OldAvgStdDev: {1} -- NewAvgStdDev: {2} -- ScoreImprovementThreshold: {3} -- Valid Moves Generated: {4} -- Metric List {5}: ",
        scoreImprovement,  oldSolution.AvgStdDev, newSolution.AvgStdDev, config.ScoreImprovementThreshold, newSolution.ValidMoveCount, SDDiagPtr->metricString_);

        ServiceModel::HealthReport balancingFailureHealthReport = ServiceModel::HealthReport::CreateSystemHealthReport(
            config.UseCRMPublicName ? SystemHealthReportCode::CRM_BalancingUnsuccessful : SystemHealthReportCode::PLB_BalancingUnsuccessful,
            move(healthInfo),
            move(metricsString),
            move(healthDetails),
            timeSpan,
            move(attributeList));

        balancingViolationHealthReports_.push_back(move(balancingFailureHealthReport));
    }
}


bool PLBDiagnostics::TrackEliminatedNodes(PlacementReplica const* r)
{
    auto rHash = r->ReplicaHash();
    //Need to add +1 to account for the fact that this quantity would ordinarily have been evaluated after being incremented in TrackUnplacedReplica
    return (VerbosePlacementHealthReportingEnabled && (healthClient_) && placementDiagnosticsTableSPtr_->count(r->Partition->Service->Name)
            && (placementDiagnosticsTableSPtr_->at(r->Partition->Service->Name).count(rHash)
                ?   (
                    placementDiagnosticsTableSPtr_->at(r->Partition->Service->Name).at(rHash).first + 1 > PLBConfig::GetConfig().VerboseHealthReportLimit
                    &&
                    placementDiagnosticsTableSPtr_->at(r->Partition->Service->Name).at(rHash).first + 1 >= PLBConfig::GetConfig().DetailedVerboseHealthReportLimit
                    )
                : false));
}

bool PLBDiagnostics::TrackConstraintFixEliminatedNodes(PlacementReplica const* r, IConstraint::Enum it)
{
    //DetailedConstraintViolationHealthReportLimit
    auto rHash = r->ReplicaHash(true) + StringUtility::ToWString(it);

    //Dont need to add +1 to account for the fact that this quantity would ordinarily have been evaluated after being incremented in TrackUpgradeUnswappedPartition
    return (VerbosePlacementHealthReportingEnabled && (healthClient_) && cachedConstraintDiagnosticsTableSPtr_->count(r->Partition->Service->Name)
            && (cachedConstraintDiagnosticsTableSPtr_->at(r->Partition->Service->Name).count(rHash)
                ?   (
                    cachedConstraintDiagnosticsTableSPtr_->at(r->Partition->Service->Name).at(rHash).first > PLBConfig::GetConfig().ConstraintViolationHealthReportLimit
                    &&
                    cachedConstraintDiagnosticsTableSPtr_->at(r->Partition->Service->Name).at(rHash).first >= PLBConfig::GetConfig().DetailedConstraintViolationHealthReportLimit
                    )
                : false));
}

bool PLBDiagnostics::TrackUpgradeSwapEliminatedNodes(PartitionEntry const* p)
{
    auto pHash = p->PartitionId;
    //Need to add +1 to account for the fact that this quantity would ordinarily have been evaluated after being incremented in TrackUpgradeUnswappedPartition
    return (VerbosePlacementHealthReportingEnabled && (healthClient_) && upgradeSwapDiagnosticsTableSPtr_->count(p->Service->Name)
            && (upgradeSwapDiagnosticsTableSPtr_->at(p->Service->Name).count(pHash)
                ?   (
                    upgradeSwapDiagnosticsTableSPtr_->at(p->Service->Name).at(pHash).first + 1 > PLBConfig::GetConfig().VerboseHealthReportLimit
                    &&
                    upgradeSwapDiagnosticsTableSPtr_->at(p->Service->Name).at(pHash).first + 1 >= PLBConfig::GetConfig().DetailedVerboseHealthReportLimit
                    )
                : false));
}



bool PLBDiagnostics::TrackUpgradeUnswappedPartition(PartitionEntry const* p, std::wstring & traceMessage, std::wstring & nodeDetailMessage)
{
    // We don't take a lock in this method against the double call to the Toggle method to clear
    // the diagnostic table because this lock is taken in the searcher method for better performance

    //Returns whether to do a detailed trace

    auto pHash = p->PartitionId;

    if (upgradeSwapDiagnosticsTableSPtr_->count(p->Service->Name))
    {
        //If the particular partition/role is present
        if (upgradeSwapDiagnosticsTableSPtr_->at(p->Service->Name).count(pHash))
        {
            auto& diagPair = upgradeSwapDiagnosticsTableSPtr_->at(p->Service->Name).at(pHash);
            diagPair.first++;
            diagPair.second = (diagPair.first >= PLBConfig::GetConfig().DetailedVerboseHealthReportLimit) ? (traceMessage + nodeDetailMessage) : traceMessage;

        }
        else
        {
            std::pair<size_t, std::wstring> newPRdiagnosticPair(1, traceMessage);
            upgradeSwapDiagnosticsTableSPtr_->at(p->Service->Name).insert(std::pair<PartitionKeyType, std::pair<size_t, std::wstring>>(pHash, newPRdiagnosticPair));
        }
    }
    else
    {
        //Replica to Diagnostic Info map
        std::map<PartitionKeyType, std::pair<size_t, std::wstring>> partitionToDiagnosticInfo;
        std::pair<size_t, std::wstring> diagnosticPair(1, traceMessage);

        partitionToDiagnosticInfo.insert(std::pair<PartitionKeyType, std::pair<size_t, std::wstring>>(pHash, diagnosticPair));
        upgradeSwapDiagnosticsTableSPtr_->insert(std::pair<std::wstring, std::map<PartitionKeyType, std::pair<size_t, std::wstring>>>(p->Service->Name, partitionToDiagnosticInfo));
    }

    if (VerbosePlacementHealthReportingEnabled && (healthClient_) && (upgradeSwapDiagnosticsTableSPtr_->at(p->Service->Name).count(pHash) ? (upgradeSwapDiagnosticsTableSPtr_->at(p->Service->Name).at(pHash).first > PLBConfig::GetConfig().VerboseHealthReportLimit) : false))
    {
        auto& countAlias = upgradeSwapDiagnosticsTableSPtr_->at(p->Service->Name).at(pHash).first;
        TimeSpan const timeSpan = PLBConfig::GetConfig().PLBHealthEventTTL;
        auto const & config = PLBConfig::GetConfig();
        auto reportingName = SystemServiceApplicationNameHelper::IsSystemServiceName(p->Service->Name) ? SystemServiceApplicationNameHelper::GetPublicServiceName(p->Service->Name) : p->Service->Name;
        auto healthInfo = ServiceModel::EntityHealthInformation::CreatePartitionEntityHealthInformation(p->PartitionId);
        auto healthString = wformatString("{0}", p->PartitionId);
        traceMessage = Environment::NewLine + wformatString("{0} Partition {1} could not be swapped during upgrade, due to the following constraints and properties: {2}", p->Service->Name, p->PartitionId, (countAlias >= PLBConfig::GetConfig().DetailedVerboseHealthReportLimit) ? (traceMessage + nodeDetailMessage) : traceMessage);
        auto attributeList = ServiceModel::AttributeList();
        attributeList.AddAttribute(*ServiceModel::HealthAttributeNames::ServiceName, reportingName);

        ServiceModel::HealthReport servicePlacementHealthReport = ServiceModel::HealthReport::CreateSystemHealthReport(
            config.UseCRMPublicName ? SystemHealthReportCode::CRM_UpgradePrimarySwapViolation : SystemHealthReportCode::PLB_UpgradePrimarySwapViolation,
            move(healthInfo),
            move(healthString),
            move(traceMessage),
            timeSpan,
            move(attributeList));

        unswappableUpgradePartitionHealthReports_.push_back(move(servicePlacementHealthReport));
        //Only do detailed tracing a small fraction of the time to save space
        return (countAlias % PLBConfig::GetConfig().DetailedVerboseHealthReportLimit == 0);
    }

    return false;
}

bool PLBDiagnostics::TrackUnplacedReplica(PlacementReplica const* r, std::wstring & traceMessage, std::wstring & nodeDetailMessage)
{
    // We don't take a lock in this method against the double call to the Toggle method to clear
    // the diagnostic table because this lock is taken in the searcher method for better performance

    //This method Returns whether to do a detailed trace

    auto rHash = r->ReplicaHash();

    //If the element is present
    if (placementDiagnosticsTableSPtr_->count(r->Partition->Service->Name))
    {
        //If the particular partition/role is present
        if (placementDiagnosticsTableSPtr_->at(r->Partition->Service->Name).count(rHash))
        {
            placementDiagnosticsTableSPtr_->at(r->Partition->Service->Name).at(rHash).first++;
            placementDiagnosticsTableSPtr_->at(r->Partition->Service->Name).at(rHash).second = traceMessage;
        }
        else
        {
            std::pair<size_t, std::wstring> newPRdiagnosticPair(1, traceMessage);
            placementDiagnosticsTableSPtr_->at(r->Partition->Service->Name).insert(std::pair<ReplicaKeyType, std::pair<size_t, std::wstring>>(rHash, newPRdiagnosticPair));
        }
    }
    else
    {
        //Replica to Diagnostic Info map
        std::map<ReplicaKeyType, std::pair<size_t, std::wstring>> replicaToDiagnosticInfo;
        std::pair<size_t, std::wstring> diagnosticPair(1, traceMessage);

        replicaToDiagnosticInfo.insert(std::pair<ReplicaKeyType, std::pair<size_t, std::wstring>>(rHash, diagnosticPair));
        placementDiagnosticsTableSPtr_->insert(std::pair<std::wstring, std::map<ReplicaKeyType, std::pair<size_t, std::wstring>>>(r->Partition->Service->Name, replicaToDiagnosticInfo));
    }

    if (VerbosePlacementHealthReportingEnabled && (healthClient_) && (placementDiagnosticsTableSPtr_->at(r->Partition->Service->Name).count(rHash) ? (placementDiagnosticsTableSPtr_->at(r->Partition->Service->Name).at(rHash).first > PLBConfig::GetConfig().VerboseHealthReportLimit) : false))
    {
        auto& countAlias = placementDiagnosticsTableSPtr_->at(r->Partition->Service->Name).at(rHash).first;
        TimeSpan const timeSpan = PLBConfig::GetConfig().PLBHealthEventTTL;
        auto const & config = PLBConfig::GetConfig();
        auto reportingName = SystemServiceApplicationNameHelper::IsSystemServiceName(r->Partition->Service->Name) ? SystemServiceApplicationNameHelper::GetPublicServiceName(r->Partition->Service->Name) : r->Partition->Service->Name;
        auto healthInfo = ServiceModel::EntityHealthInformation::CreatePartitionEntityHealthInformation(r->Partition->PartitionId);
        auto healthString = wformatString("{0}_{1}", StringUtility::ToWString(r->Role), r->Partition->PartitionId);
        traceMessage = Environment::NewLine +
                        wformatString("{0} replica could not be placed due to the following constraints and properties: {1}",
                        StringUtility::ToWString(r->Role),
                        (countAlias >= PLBConfig::GetConfig().DetailedVerboseHealthReportLimit) ? (traceMessage + nodeDetailMessage) : traceMessage);
        auto attributeList = ServiceModel::AttributeList();
        attributeList.AddAttribute(*ServiceModel::HealthAttributeNames::ServiceName, reportingName);

        ServiceModel::HealthReport servicePlacementHealthReport = ServiceModel::HealthReport::CreateSystemHealthReport(
            config.UseCRMPublicName ? SystemHealthReportCode::CRM_UnplacedReplicaViolation : SystemHealthReportCode::PLB_UnplacedReplicaViolation,
            move(healthInfo),
            move(healthString),
            move(traceMessage),
            timeSpan,
            move(attributeList));

        unplacedReplicaHealthReports_.push_back(move(servicePlacementHealthReport));
        //Only do detailed tracing a small fraction of the time to save space
        return (countAlias % PLBConfig::GetConfig().DetailedVerboseHealthReportLimit == 0);
    }
    return false;
}

bool PLBDiagnostics::TrackUnfixableConstraint(ConstraintViolation const & constraintViolation, std::wstring & traceMessage, std::wstring & nodeDetailMessage)
{
    // modify std::vector<PlacementReplica const*> constraintDiagnosticsReplicas_ so that it has both the replica and the constraint
    //Use the ConstraintViolation Class in DiagnosticSubspace.h

    wstring const & cHash = constraintViolation.ViolationHash;
    wstring const & sName = constraintViolation.Replica->Partition->Service->Name;
    size_t fixAttempts = 1;

    if (cachedConstraintDiagnosticsTableSPtr_->count(sName))
    {
        if (cachedConstraintDiagnosticsTableSPtr_->at(sName).count(cHash))
        {
            fixAttempts = cachedConstraintDiagnosticsTableSPtr_->at(sName).at(cHash).first;
        }
    }

    // fixAttempts will be 1 if config ConstraintViolationHealthReportLimit is 0
    // As below we check % ConstraintViolationHealthReportLimit => it can lead to division by 0 error
    TESTASSERT_IF(fixAttempts == 1, "Coding Error: Number of Cached Constraint Fix Attempts is inconsistent.");

    if ((healthClient_) && (cachedConstraintDiagnosticsTableSPtr_->at(sName).count(cHash) ? (cachedConstraintDiagnosticsTableSPtr_->at(sName).at(cHash).first > PLBConfig::GetConfig().ConstraintViolationHealthReportLimit) : false))
    {
        TimeSpan const timeSpan = PLBConfig::GetConfig().PLBHealthEventTTL;
        auto const & config = PLBConfig::GetConfig();
        auto reportingName = SystemServiceApplicationNameHelper::IsSystemServiceName(sName) ? SystemServiceApplicationNameHelper::GetPublicServiceName(sName) : sName;

        std::wstring cvNodeName = L"Unknown";

        cvNodeName = plbNodes_[constraintViolation.Replica->Node->NodeIndex].NodeDescriptionObj.NodeName;

        TESTASSERT_IF(cvNodeName ==  L"Unknown", "NodeName for a constraint Violating replica is unknown due to a diagnostics failure.");

        auto healthInfo = ServiceModel::EntityHealthInformation::CreatePartitionEntityHealthInformation(constraintViolation.Replica->Partition->PartitionId);
        auto healthString = wformatString("{0}_{1}_{2}_{3}", constraintViolation.Constraint, constraintViolation.Replica->Partition->PartitionId, StringUtility::ToWString(constraintViolation.Replica->Role), cvNodeName);

        traceMessage = Environment::NewLine + wformatString("{0} {1} Partition {2} constraint violation could not be fixed, possibly, due to the following constraints and properties: {3}", sName, StringUtility::ToWString(constraintViolation.Replica->Role), constraintViolation.Replica->Partition->PartitionId, (fixAttempts >= PLBConfig::GetConfig().DetailedConstraintViolationHealthReportLimit) ? (traceMessage + nodeDetailMessage) : traceMessage);

        auto attributeList = ServiceModel::AttributeList();
        attributeList.AddAttribute(*ServiceModel::HealthAttributeNames::ServiceName, move(reportingName));

        if (cvNodeName !=  L"Unknown")
        {
            attributeList.AddAttribute(*ServiceModel::HealthAttributeNames::NodeId, move(cvNodeName));
        }

        ServiceModel::HealthReport unfixableConstraintHealthReport = ServiceModel::HealthReport::CreateSystemHealthReport(
            config.UseCRMPublicName ? SystemHealthReportCode::CRM_ConstraintFixUnsuccessful : SystemHealthReportCode::PLB_ConstraintFixUnsuccessful,
            move(healthInfo),
            move(healthString),
            move(traceMessage),
            timeSpan,
            move(attributeList));

        unfixableConstraintViolationHealthReports_.push_back(move(unfixableConstraintHealthReport));

        return !(fixAttempts % PLBConfig::GetConfig().ConstraintViolationHealthReportLimit);
    }

    return false;
}

bool PLBDiagnostics::UpdateConstraintFixFailureLedger(PlacementReplica const* itReplica, IConstraintUPtr const & it)
{
    bool needToRunDiagnostics = false;

    auto rHash = itReplica->ReplicaHash(true) + StringUtility::ToWString(it->Type);

    // Change the offset in test mode so that we can enable running diagnostics
    size_t offset = PLBConfig::GetConfig().IsTestMode ? 1 : 0;

    if (constraintDiagnosticsTableSPtr_->count(itReplica->Partition->Service->Name))
    {
        if (constraintDiagnosticsTableSPtr_->at(itReplica->Partition->Service->Name).count(rHash))
        {
            offset = constraintDiagnosticsTableSPtr_->at(itReplica->Partition->Service->Name).at(rHash).first;
        }
    }

    if (offset + 1 >= PLBConfig::GetConfig().ConstraintViolationHealthReportLimit)
    {
        needToRunDiagnostics = true;
        constraintDiagnosticsReplicas_.push_back(ConstraintViolation(itReplica, it->Type, it->Priority));
    }

    //Blank string for now. Already filed a work item to clean up the diagnostics data structures.
    std::pair<size_t, std::wstring> diagnosticPair(offset + 1, L"");

    if (cachedConstraintDiagnosticsTableSPtr_->count(itReplica->Partition->Service->Name))
    {
        cachedConstraintDiagnosticsTableSPtr_->at(itReplica->Partition->Service->Name).insert(std::pair<ReplicaKeyType, std::pair<size_t, std::wstring>>(rHash, diagnosticPair));
    }
    else
    {
        //Replica to Diagnostic Info map
        std::map<ReplicaKeyType, std::pair<size_t, std::wstring>> replicaToDiagnosticInfo;

        replicaToDiagnosticInfo.insert(std::pair<ReplicaKeyType, std::pair<size_t, std::wstring>>(rHash, diagnosticPair));
        cachedConstraintDiagnosticsTableSPtr_->insert(std::pair<std::wstring, std::map<ReplicaKeyType, std::pair<size_t, std::wstring>>>(itReplica->Partition->Service->Name, replicaToDiagnosticInfo));
    }

    return needToRunDiagnostics;
}

void PLBDiagnostics::CleanupConstraintDiagnosticsCaches()
{
    *constraintDiagnosticsTableSPtr_ = *cachedConstraintDiagnosticsTableSPtr_;
    cachedConstraintDiagnosticsTableSPtr_->clear();
    constraintDiagnosticsReplicas_.clear();
}

void PLBDiagnostics::ReportSearchForPlacementHealth()
{
    if (healthClient_)
    {
        if (!unplacedReplicaHealthReports_.empty())
        {
            auto addHealthError = healthClient_->AddHealthReports(move(unplacedReplicaHealthReports_));
            if (!addHealthError.IsSuccess())
            {
                Trace.HealthReportingFailure(addHealthError);
            }

            std::vector<ServiceModel::HealthReport> emptyVector;
            std::swap(unplacedReplicaHealthReports_, emptyVector);
        }

        if (!unswappableUpgradePartitionHealthReports_.empty())
        {
            auto addHealthError = healthClient_->AddHealthReports(move(unswappableUpgradePartitionHealthReports_));
            if (!addHealthError.IsSuccess())
            {
                Trace.HealthReportingFailure(addHealthError);
            }

            unswappableUpgradePartitionHealthReports_.clear();
        }
    }
}

void PLBDiagnostics::ReportConstraintHealth()
{
    if (!constraintViolationHealthReports_.empty() && healthClient_)
    {
        auto addHealthError = healthClient_->AddHealthReports(move(constraintViolationHealthReports_));
        if (!addHealthError.IsSuccess())
        {
            Trace.HealthReportingFailure(addHealthError);
        }

        constraintViolationHealthReports_.clear();
    }

    if (!unfixableConstraintViolationHealthReports_.empty() && healthClient_)
    {
        auto addHealthError = healthClient_->AddHealthReports(move(unfixableConstraintViolationHealthReports_));
        if (!addHealthError.IsSuccess())
        {
            Trace.HealthReportingFailure(addHealthError);
        }

        unfixableConstraintViolationHealthReports_.clear();
    }
}

void PLBDiagnostics::ReportBalancingHealth()
{
    if (!balancingViolationHealthReports_.empty() && healthClient_)
    {
        auto addHealthError = healthClient_->AddHealthReports(move(balancingViolationHealthReports_));
        if (!addHealthError.IsSuccess())
        {
            Trace.HealthReportingFailure(addHealthError);
        }

        balancingViolationHealthReports_.clear();
    }
}

void PLBDiagnostics::ReportDroppedPLBMovementsHealth()
{
    if (!consecutiveDroppedMovementsHealthReports_.empty() && healthClient_)
    {
        auto addHealthError = healthClient_->AddHealthReports(move(consecutiveDroppedMovementsHealthReports_));
        if (!addHealthError.IsSuccess())
        {
            Trace.HealthReportingFailure(addHealthError);
        }

        consecutiveDroppedMovementsHealthReports_.clear();
    }
}

void PLBDiagnostics::ReportServiceCorrelationError(ServiceDescription const& service)
{
    wstring reportingName = SystemServiceApplicationNameHelper::IsSystemServiceName(service.Name)
        ? SystemServiceApplicationNameHelper::GetPublicServiceName(service.Name)
        : service.Name;

    auto const & config = PLBConfig::GetConfig();

    auto healthDetails = wformatString("Updating service {0} to be correlated to service {1} will create affinity chain. Correlations are ignored.",
        reportingName,
        service.AffinitizedService);

    // Report health error:
    //  - Maximum TTL, never expires.
    //  - Will be cleared when successful update happens - ReportServiceDescriptionOK() will send the report.
    ReportServiceHealth(
        move(reportingName),
        service,
        config.UseCRMPublicName ? SystemHealthReportCode::CRM_ServiceCorrelationError : SystemHealthReportCode::PLB_ServiceCorrelationError,
        move(healthDetails),
        Common::TimeSpan::MaxValue);
}

void PLBDiagnostics::ReportServiceDescriptionOK(ServiceDescription const& service)
{
    wstring reportingName = SystemServiceApplicationNameHelper::IsSystemServiceName(service.Name)
        ? SystemServiceApplicationNameHelper::GetPublicServiceName(service.Name)
        : service.Name;

    auto const & config = PLBConfig::GetConfig();

    wstring healthDetails = wformatString("Service {0} was updated successfully.", reportingName);

    // Report that health is OK:
    //  - TTL as all other PLB reports, remove when expired.
    //  - Will clear previous correlation error (if any).
    ReportServiceHealth(
        move(reportingName),
        service,
        config.UseCRMPublicName ? SystemHealthReportCode::CRM_ServiceDescriptionOK : SystemHealthReportCode::PLB_ServiceDescriptionOK,
        move(healthDetails),
        PLBConfig::GetConfig().PLBHealthEventTTL);
}

void PLBDiagnostics::ReportServiceHealth(
    std::wstring && reportingName,
    ServiceDescription const & serviceDescription,
    Common::SystemHealthReportCode::Enum reportCode,
    std::wstring && healthDetails,
    Common::TimeSpan ttl)
{
    auto healthInfo = ServiceModel::EntityHealthInformation::CreateServiceEntityHealthInformation(reportingName, serviceDescription.ServiceInstance == 0 ? 1 : serviceDescription.ServiceInstance);

    ServiceModel::AttributeList attributeList;

    // Some services may not have application set (created before 4.5). Ignore the attribute in that case.
    if (serviceDescription.ApplicationName != L"")
    {
        attributeList.AddAttribute(*ServiceModel::HealthAttributeNames::ApplicationName, move(serviceDescription.ApplicationName));
    }

    // Obtaining service type name from qualified service type name
    ServiceModel::ServiceTypeIdentifier serviceTypeIdentifier;
    auto parseError = ServiceModel::ServiceTypeIdentifier::FromString(serviceDescription.ServiceTypeName, serviceTypeIdentifier);
    if (parseError.IsSuccess())
    {
        attributeList.AddAttribute(*ServiceModel::HealthAttributeNames::ServiceTypeName, serviceTypeIdentifier.ServiceTypeName);
    }
    else
    {
        Trace.HealthReportingFailure(parseError);
        return;
    }

    ServiceModel::HealthReport serviceHealthReport = HealthReport::CreateSystemHealthReport(
        reportCode,
        move(healthInfo),
        move(healthDetails),
        static_cast<FABRIC_SEQUENCE_NUMBER>(SequenceNumber::GetNext()),
        ttl,
        move(attributeList));

    if (healthClient_)
    {
        auto addHealthError = healthClient_->AddHealthReport(move(serviceHealthReport));
        if (!addHealthError.IsSuccess())
        {
            Trace.HealthReportingFailure(addHealthError);
        }
    }
}


void PLBDiagnostics::ClearDiagnosticsTables()
{
    {
        AcquireWriteLock grab(placementDiagnosticsTableLock_);
        placementDiagnosticsTableSPtr_->clear();
    }

    {
        AcquireWriteLock grab(upgradeSwapDiagnosticsTableLock_);
        upgradeSwapDiagnosticsTableSPtr_->clear();
    }

    {
        AcquireWriteLock grab(queryLock_);
        queryableDiagnosticsTableSPtr_->clear();
        queryableUpgradeSwapDiagnosticsTableSPtr_->clear();
    }

}

void PLBDiagnostics::CleanupSearchForPlacementDiagnostics()
{
    {
        AcquireWriteLock grabTable(placementDiagnosticsTableLock_);
        AcquireWriteLock hold(upgradeSwapDiagnosticsTableLock_);

        //This method cleans up recently placed replicas that were formerly labeled unplaced/unswapped when encountering placement/upgradeswap difficulties

        if (placementDiagnosticsTableSPtr_->empty())
        {
            //If the diagnostic table is empty, that means that none of the replicas that were placed  were previously unable to be placed, so we can discard them from the queue
            if (!placementQueue_.empty())
            {
                std::queue<std::pair<std::wstring, std::wstring>> emptyQueue;
                std::swap(placementQueue_, emptyQueue);
            }
        }
        else if (upgradeSwapDiagnosticsTableSPtr_->empty())
        {
            //If the diagnostic table is empty, that means that none of the replicas that were swapped  were previously unable to be swapped, so we can discard them from the queue
            if (!upgradeSwapQueue_.empty())
            {
                std::queue<std::pair<std::wstring, Common::Guid>> emptyQueue;
                std::swap(upgradeSwapQueue_, emptyQueue);
            }
        }
        else
        {
            //Remove Placed Partitions
            while (!placementQueue_.empty())
            {
                auto & rHash = placementQueue_.front().first;
                auto & name = placementQueue_.front().second;

                if (placementDiagnosticsTableSPtr_->count(name))
                {
                    if (placementDiagnosticsTableSPtr_->at(name).count(rHash))
                    {
                        placementDiagnosticsTableSPtr_->at(name).erase(rHash);

                        if (placementDiagnosticsTableSPtr_->at(name).empty())
                        {
                            placementDiagnosticsTableSPtr_->erase(name);
                        }
                    }
                }

                placementQueue_.pop();
            }

            //Remove Upgrade Swapped Partitions
            while (!upgradeSwapQueue_.empty())
            {
                auto & p = upgradeSwapQueue_.front();
                auto & pHash = p.second;
                std::wstring & name = p.first;

                if (upgradeSwapDiagnosticsTableSPtr_->count(name))
                {
                    if (upgradeSwapDiagnosticsTableSPtr_->at(name).count(pHash))
                    {
                        upgradeSwapDiagnosticsTableSPtr_->at(name).erase(pHash);

                        if (upgradeSwapDiagnosticsTableSPtr_->at(name).empty())
                        {
                            upgradeSwapDiagnosticsTableSPtr_->erase(name);
                        }
                    }
                }

                upgradeSwapQueue_.pop();
            }

            //Remove Deleted Services that have old records of unplaced replicas
            //Acquires a lock as InternalDeleteService Calls RemoveService
            {
                AcquireWriteLock grabService(serviceDiagnosticLock_);

                while (!removedServiceQueue_.empty())
                {
                    auto serviceName = removedServiceQueue_.front();
                    removedServiceQueue_.pop();

                    if (placementDiagnosticsTableSPtr_->count(serviceName))
                    {
                        placementDiagnosticsTableSPtr_->erase(serviceName);
                    }

                    if (upgradeSwapDiagnosticsTableSPtr_->count(serviceName))
                    {
                        upgradeSwapDiagnosticsTableSPtr_->erase(serviceName);
                    }
                }
            }


            //Remove Deleted Partitions that have old records of unplaced replicas
            //Acquires a lock as ProcessUpdateFailoverUnit modifies the FUQueue by calling RemoveFailoverUnit
            {
                AcquireWriteLock grabPartition(partitionDiagnosticLock_);

                while (!removedFUQueue_.empty())
                {
                    std::wstring serviceName = removedFUQueue_.front().first;
                    Common::Guid fUId = removedFUQueue_.front().second;
                    removedFUQueue_.pop();

                    if (placementDiagnosticsTableSPtr_->count(serviceName))
                    {
                        //Iterate through keys, removing the ones that have the GUID in it
                        ReplicaDiagnosticMap & replicaDiagMap = placementDiagnosticsTableSPtr_->at(serviceName);
                        for (auto mapIter = replicaDiagMap.begin(); mapIter != replicaDiagMap.end();)
                        {
                            //Though substring matching is slow, this is an edge case and this portion of this method isn't expected to be run very often.
                            //A quick and dirty alternative is to simply call RemoveService to clean up the entire service instead of just one failoverUnit
                            if (mapIter->first.find(fUId.ToString()) != wstring::npos)
                            {
                                mapIter = replicaDiagMap.erase(mapIter);
                            }
                            else
                            {
                                ++mapIter;
                            }
                        }

                        if (replicaDiagMap.empty())
                        {
                            placementDiagnosticsTableSPtr_->erase(serviceName);
                        }
                    }

                    if (upgradeSwapDiagnosticsTableSPtr_->count(serviceName))
                    {
                        //Iterate through keys, removing the ones that have the GUID in it
                        PartitionDiagnosticMap & partitionDiagMap = upgradeSwapDiagnosticsTableSPtr_->at(serviceName);

                        if (partitionDiagMap.count(fUId))
                        {
                            partitionDiagMap.erase(fUId);
                        }

                        if (partitionDiagMap.empty())
                        {
                            upgradeSwapDiagnosticsTableSPtr_->erase(serviceName);
                        }
                    }

                }
            }
        }

        //Copy the the diagnostics table over so it can be freely and safely queried for the rest of the PLBRefresh cycle
        {
            AcquireWriteLock grabQuery(queryLock_);

            *queryableDiagnosticsTableSPtr_ = *placementDiagnosticsTableSPtr_;
            *queryableUpgradeSwapDiagnosticsTableSPtr_ = *upgradeSwapDiagnosticsTableSPtr_;
        }
    }
}

void PLBDiagnostics::TrackPlacedReplica(PlacementReplica const* r)
{
    if ((r != nullptr) && (r->Partition != nullptr))
    placementQueue_.push(std::make_pair(r->ReplicaHash(), r->Partition->Service->Name));
}

void PLBDiagnostics::TrackUpgradeSwap(PartitionEntry const* p)
{
    if (p != nullptr)
    {
        upgradeSwapQueue_.push(std::make_pair(p->Service->Name, p->PartitionId));
    }
}

void PLBDiagnostics::RemoveService(std::wstring && serviceName)
{
    {
        AcquireWriteLock grab(serviceDiagnosticLock_);

        removedServiceQueue_.push(move(serviceName));
    }
}

void PLBDiagnostics::RemoveFailoverUnit(std::wstring && serviceName, Common::Guid const & fUId)
{
    {
        AcquireWriteLock grab(partitionDiagnosticLock_);

        removedFUQueue_.push(std::make_pair(move(serviceName), fUId));
    }
}

std::vector<wstring> PLBDiagnostics::GetUnplacedReplicaInformation(std::wstring const& serviceName, Common::Guid const& partitionId, bool onlyQueryPrimaries)
{
    std::vector<wstring> unplacedReasons;

    //We take the queryLock_ below and hence should only access queryable structures, not functional ones
    {
        AcquireReadLock grab(queryLock_);

        if (!onlyQueryPrimaries && (partitionId == Guid::Empty()))
        {
            if (queryableDiagnosticsTableSPtr_->count(serviceName))
            {
                //Iterate through unplaced replicas, adding all the strings
                ReplicaDiagnosticMap & replicaDiagMap = queryableDiagnosticsTableSPtr_->at(serviceName);
                for (auto mapIter = replicaDiagMap.begin(); mapIter != replicaDiagMap.end(); ++mapIter)
                {
                    unplacedReasons.push_back(wformatString("\n{0}\n{1} Failed Placement Attempts\n{2}", mapIter->first, mapIter->second.first, mapIter->second.second));
                }
            }
            else
            {
                //No Unplaced replicas
                unplacedReasons.push_back(wformatString("There were no unplaced replicas for {0}", serviceName));
            }
        }
        else if (onlyQueryPrimaries && (partitionId == Guid::Empty()))
        {
            if (queryableDiagnosticsTableSPtr_->count(serviceName))
            {
                //Iterate through unplaced replicas, recording the ones that are primaries
                ReplicaDiagnosticMap & replicaDiagMap = queryableDiagnosticsTableSPtr_->at(serviceName);
                for (auto mapIter = replicaDiagMap.begin(); mapIter != replicaDiagMap.end(); ++mapIter)
                {
                    if (mapIter->first.find(L"Primary") != wstring::npos)
                    {
                        unplacedReasons.push_back(wformatString("\n{0}\n{1} Failed Placement Attempts\n{2}", mapIter->first, mapIter->second.first, mapIter->second.second));
                    }
                }
            }
            else
            {
                //No Unplaced replicas
                unplacedReasons.push_back(wformatString("There were no unplaced primary replicas for {0}", serviceName));
            }
        }
        else if (onlyQueryPrimaries && (partitionId != Guid::Empty()))
        {
            if (queryableDiagnosticsTableSPtr_->count(serviceName))
            {
                //Iterate through unplaced replicas, recordings the ones that are primaries and have the right partitionId
                ReplicaDiagnosticMap & replicaDiagMap = queryableDiagnosticsTableSPtr_->at(serviceName);
                for (auto mapIter = replicaDiagMap.begin(); mapIter != replicaDiagMap.end(); ++mapIter)
                {
                    if ((mapIter->first.find(L"Primary") != wstring::npos) && (mapIter->first.find(partitionId.ToString()) != wstring::npos))
                    {
                        unplacedReasons.push_back(wformatString("\n{0}\n{1} Failed Placement Attempts\n{2}", mapIter->first, mapIter->second.first, mapIter->second.second));
                    }
                }

                if (unplacedReasons.empty())
                {
                    unplacedReasons.push_back(wformatString("There were no unplaced primary replicas for {0} partition {1}", serviceName, partitionId));
                }
            }
            else
            {
                //No Unplaced replicas
                unplacedReasons.push_back(wformatString("There were no unplaced replicas for {0}", serviceName));
            }
        }
        else if (!onlyQueryPrimaries && (partitionId != Guid::Empty()))
        {
            if (queryableDiagnosticsTableSPtr_->count(serviceName))
            {
                //Iterate through unplaced replicas that have the right partitionId
                ReplicaDiagnosticMap & replicaDiagMap = queryableDiagnosticsTableSPtr_->at(serviceName);
                for (auto mapIter = replicaDiagMap.begin(); mapIter != replicaDiagMap.end(); ++mapIter)
                {
                    if ((mapIter->first.find(partitionId.ToString()) != wstring::npos))
                    {
                        unplacedReasons.push_back(wformatString("\n{0}\n{1} Failed Placement Attempts\n{2}", mapIter->first, mapIter->second.first, mapIter->second.second));
                    }
                }

                if (unplacedReasons.empty())
                {
                    unplacedReasons.push_back(wformatString("There were no unplaced replicas for {0} partition {1}", serviceName, partitionId));
                }
            }
            else
            {
                //No Unplaced replicas
                unplacedReasons.push_back(wformatString("There were no unplaced replicas for {0}", serviceName));
            }
        }
    }

    return unplacedReasons;
}

std::wstring PLBDiagnostics::ShortFriendlyConstraintName(IConstraint::Enum type)
{
    switch (type)
    {
        case IConstraint::ReplicaExclusionStatic:
            return wformatString("Existing Secondary Replicas");
            break;
        case IConstraint::ReplicaExclusionDynamic:
            return wformatString("Existing Primary Replica");
            break;
        case IConstraint::PlacementConstraint:
            return wformatString("PlacementConstraint");
            break;
        case IConstraint::NodeCapacity:
            return wformatString("NodeCapacity");
            break;
        case IConstraint::Affinity:
            return wformatString("Affinity");
            break;
        case IConstraint::FaultDomain:
            return wformatString("FaultDomain");
            break;
        case IConstraint::UpgradeDomain:
            return wformatString("UpgradeDomain");
            break;
        case IConstraint::PreferredLocation:
            return wformatString("PreferredLocation");
            break;
        case IConstraint::ScaleoutCount:
            return wformatString("ScaleoutCount");
            break;
        case IConstraint::ApplicationCapacity:
            return wformatString("ApplicationCapacity");
            break;
        case IConstraint::Throttling:
            return L"Throttling";
            break;
        default:
            return wformatString("Unknown Constraint Type");
    }
}

std::wstring PLBDiagnostics::FriendlyConstraintName(IConstraint::Enum type, PlacementReplica const* r, PartitionEntry const* p)
{
    TESTASSERT_IF((p == nullptr) && (r == nullptr), "Coding Error: Pointers should not both be null.");
    TESTASSERT_IF( (r != nullptr) && (p != nullptr) && (p != (r->Partition)) , "Coding Error: Pointers should match.");

    if (r != nullptr)
    {
        p = (r->Partition);
    }

    //By now, atleast p exists, and if r exists, then p also exists and matches it

    auto fdType = ((p) && (p->Service)) ? p->Service->FDDistribution : Service::Type::Ignore;

    switch (type)
    {
        case IConstraint::ReplicaExclusionStatic:
            return wformatString("Existing Secondary Replicas -- Nodes with Partition's Existing {0}", ((r == nullptr) || (r->IsSecondary)) ? L"Secondary Replicas/Instances" : L"Secondary Replicas" );
            break;
        case IConstraint::ReplicaExclusionDynamic:
            return wformatString("Existing Primary Replica -- Nodes with Partition's {0}", ((r == nullptr) || (r->IsPrimary)) ? L"Potential Secondary Replicas" : L"Existing Primary Replica or Secondary Replicas");
            break;
        case IConstraint::PlacementConstraint:
            return wformatString("PlacementConstraint -- Nodes that satisfy Service's PlacementConstraint");
            break;
        case IConstraint::NodeCapacity:
            return wformatString("NodeCapacity -- Nodes with sufficient remaining Metric Capacities");
            break;
        case IConstraint::Affinity:
            return wformatString("Affinity -- Nodes with Affinitized Service's Replicas");
            break;
        case IConstraint::FaultDomain:
            return wformatString("FaultDomain -- FaultDomains {0}", (fdType ==  Service::Type::Packing ) ? L"must have Similar Numbers of Partition's Replicas" : ((fdType ==  Service::Type::Nonpacking ) ? L"must have Equal Numbers of Partition's Replicas" : L"are being Ignored" ) );
            break;
        case IConstraint::UpgradeDomain:
            return wformatString("UpgradeDomain -- UpgradeDomains must have Similar Numbers of Partition's Replicas");
            break;
        case IConstraint::PreferredLocation:
            return wformatString("PreferredLocation -- Standby Replica of Partition must Exist");
            break;
        case IConstraint::ScaleoutCount:
            return wformatString("ApplicationCapacity -- Nodes that satisfy the Application-Specific Restriction to Scale to Limited Number of Nodes");
            break;
        case IConstraint::ApplicationCapacity:
            return wformatString("ApplicationCapacity -- Nodes with sufficient remaining Application-Specific Capacities");
            break;
        case IConstraint::Throttling:
            return L"Throttling";
            break;
        default:
            return wformatString("Unknown Constraint Type");
    }
}

std::wstring PLBDiagnostics::ConstraintDetails(IConstraint::Enum type, PlacementReplica const* r, std::shared_ptr<IConstraintDiagnosticsData> diagnosticsDataSPtr)
{


    switch (type)
    {
    case IConstraint::ReplicaExclusionStatic:
        return wformatString("Existing Secondary Replicas:");
        break;
    case IConstraint::ReplicaExclusionDynamic:
        return wformatString("Existing Primary Replica:");
        break;
    case IConstraint::PlacementConstraint:
    {
        std::wstring pConstraints = ((r->Partition) && (r->Partition->Service)) ? r->Partition->Service->DiagnosticsData.placementConstraints_ : L"Not Applicable or Unknown";
        return wformatString("PlacementConstraint: {0}", pConstraints);
    }
        break;
    case IConstraint::NodeCapacity:
        return wformatString("NodeCapacity");
        break;
    case IConstraint::Affinity:
    {
        if (nullptr == r->Partition)
        {
            return  wformatString("Affinity: empty partition");
        }
        else if (nullptr == r->Partition->Service)
        {
            return  wformatString("Affinity: null service for partition {0}", r->Partition->PartitionId);
        }
        else if (nullptr == r->Partition->Service->DependedService)
        {
            return  wformatString("Affinity: parent service {0}", r->Partition->Service->Name);
        }
        else
        {
            // Has valid parent service, this is child partition
            std::wstring affinityName = r->Partition->Service->DependedService->Name;
            std::vector<int> parentNodeIndexes;
            std::wstring parentNodeString;

            ServiceEntry const* childService = r->Partition->Service;
            ServiceEntry const* parentService = r->Partition->Service->DependedService;
            Common::Guid parentPartitionId;

            std::vector<PartitionEntry const*> const& parentPartitions = r->Partition->Service->DependedService->Partitions;
            if (parentPartitions.empty())
            {
                return wformatString("Aligned Affinity {0}, Parent Service {1} doesn't have partition\n Parent/Child ReplicaCounts {2}/{3} \n Child Nodes: {4} ",
                    childService->AlignedAffinity, affinityName, parentService->TargetReplicaSetSize, childService->TargetReplicaSetSize, diagnosticsDataSPtr->RelevantNodes());
            }
            else
            {
                PartitionEntry const* parentPartition = parentPartitions[0];
                // parent should be a singleton
                parentPartition->ForEachExistingReplica([&parentNodeIndexes](PlacementReplica const * rep) -> void {parentNodeIndexes.push_back(rep->Node->NodeIndex); });
                parentPartitionId = parentPartition->PartitionId;

                for (auto j : parentNodeIndexes)
                {
                    auto const & ndRef = (*(diagnosticsDataSPtr->plbNodesPtr_))[j];
                    parentNodeString += Common::wformatString("Node {0}, {1}, ", ndRef.NodeDescriptionObj.NodeId, ndRef.NodeDescriptionObj.NodeName);
                }

                return wformatString("Aligned Affinity {0}, Parent Service {1} \n Parent PartitionId {2} \n Parent/Child ReplicaCounts {3}/{4} \n Parent Nodes: {5} \n Child Nodes: {6} ",
                    childService->AlignedAffinity, affinityName, parentPartitionId,
                    parentService->TargetReplicaSetSize, childService->TargetReplicaSetSize, parentNodeString, diagnosticsDataSPtr->RelevantNodes());
            }
        }
    }
        break;
    case IConstraint::FaultDomain:
    {
        std::wstring fDomainNodeName = L"NodeID not found";
        std::wstring fDomainID = L"FaultDomain ID not found";
        std::wstring distribution = ((r->Partition) && (r->Partition->Service)) ? StringUtility::ToWString(r->Partition->Service->FDDistribution) : L"Distribution Policy Information not found";

        if (r->Node)
        {
            //Implicitly also checks to see if the plbNodes_ is empty because the LB simulator currently doesn't use it -- defaults to NodeID for LBSim
            if (plbNodes_.size() > r->Node->NodeIndex)
            {
                auto const& nodeDescObj = plbNodes_[r->Node->NodeIndex].NodeDescriptionObj;
                fDomainNodeName = nodeDescObj.NodeName;
                fDomainID = L"";
                for (auto const& wsIter : nodeDescObj.FaultDomainId) {fDomainID += wsIter;}
            }
            else
            {

                fDomainNodeName = StringUtility::ToWString(r->Node->NodeId);
            }
        }

        return wformatString("FaultDomain Details: FaultDomain ID -- {0}. Replica on NodeName -- {1}. Distribution Policy -- {2}", fDomainID, fDomainNodeName, distribution);
    }
        break;
    case IConstraint::UpgradeDomain:
    {
        std::wstring uDomainNodeName = L"NodeID not found";
        std::wstring uDomainID = L"UpgradeDomain ID not found";
        std::wstring isUpgrade = L"Partition Upgrade Status Unknown";

        if (r->Node)
        {
            //Implicitly also checks to see if the plbNodes_ is empty because the LB simulator currently doesn't use it -- defaults to NodeID for LBSim
            if (plbNodes_.size() > r->Node->NodeIndex)
            {
                auto const& nodeDescObj = plbNodes_[r->Node->NodeIndex].NodeDescriptionObj;
                uDomainNodeName = nodeDescObj.NodeName;
                uDomainID = nodeDescObj.UpgradeDomainId;
            }
            else
            {
                uDomainNodeName = StringUtility::ToWString(r->Node->NodeId);
            }
        }

        //return with a cleaner string if
        if (r->Partition)
        {
            if (r->Partition->IsInUpgrade)
            {
                std::wstring primaryUpgrade = (r->Partition->PrimaryUpgradeLocation) ?
                    StringUtility::ToWString(r->Partition->PrimaryUpgradeLocation->NodeId.IdValue) :
                    L"Not Applicable or Unknown";

                std::wstring secondaryUpgrade = r->Partition->SecondaryUpgradeLocations.empty() ? L"Not Applicable or Unknown" : L"";

                if (!r->Partition->SecondaryUpgradeLocations.empty())
                {
                    for (auto it = r->Partition->SecondaryUpgradeLocations.begin(); it != r->Partition->SecondaryUpgradeLocations.end(); ++it)
                    {
                        if (it != r->Partition->SecondaryUpgradeLocations.begin())
                        {
                            secondaryUpgrade.append(L" ");
                        }

                        secondaryUpgrade.append(wformatString("{0}", (*it)->NodeId.IdValue));
                    }
                }

                isUpgrade = L"true";

                //Implicitly also checks to see if the plbNodes_ is empty because the LB simulator currently doesn't use it -- defaults to NodeID for LBSim
                if (plbNodes_.size() > r->Node->NodeIndex)
                {
                    if ((r->Partition->PrimaryUpgradeLocation) && (!r->Partition->SecondaryUpgradeLocations.empty()))
                    {
                        if (plbNodes_.size() > r->Partition->PrimaryUpgradeLocation->NodeIndex)
                        {
                            auto const& primaryNodeDescObj = plbNodes_[r->Partition->PrimaryUpgradeLocation->NodeIndex].NodeDescriptionObj;
                            primaryUpgrade = primaryNodeDescObj.NodeName;
                        }

                        wstring tempSecondaryUpgrade = L"";
                        for (auto it = r->Partition->SecondaryUpgradeLocations.begin(); it != r->Partition->SecondaryUpgradeLocations.end(); ++it)
                        {
                            if (it != r->Partition->SecondaryUpgradeLocations.begin())
                            {
                                tempSecondaryUpgrade.append(L" ");
                            }

                            if (plbNodes_.size() > ((*it)->NodeIndex))
                            {
                                tempSecondaryUpgrade.append(plbNodes_[((*it)->NodeIndex)].NodeDescriptionObj.NodeName);
                            }
                        }

                        if (tempSecondaryUpgrade != L"")
                        {
                            secondaryUpgrade = tempSecondaryUpgrade;
                        }
                    }
                }

                return wformatString("UpgradeDomain Details: UpgradeDomain ID -- {0}, Replica on NodeName -- {1} Currently Upgrading -- {2}, Primary Upgrade Location -- {3}, Secondary Upgrade Location -- {4}",
                    uDomainID,
                    uDomainNodeName,
                    isUpgrade,
                    primaryUpgrade,
                    secondaryUpgrade);
            }
            else
            {
                isUpgrade = L"false";
            }
        }

        return wformatString("UpgradeDomain Details: UpgradeDomain ID -- {0}, Replica on NodeName -- {1} Currently Upgrading -- {2}",
            uDomainID,
            uDomainNodeName,
            isUpgrade);

    }
        break;
    case IConstraint::PreferredLocation:
        return wformatString("PreferredLocation:");
        break;
    case IConstraint::ScaleoutCount:
        return wformatString("ScaleoutCount");
        break;
    case IConstraint::ApplicationCapacity:
        return wformatString("ApplicationCapacity");
        break;
    case IConstraint::Throttling:
        return wformatString("Throttling");
    default:
        return wformatString("Unknown Constraint Type");
    }
}

std::wstring PLBDiagnostics::ReplicaDetailString(PlacementReplica const* r)
{
    return L" " + Environment::NewLine
        + (L"TargetReplicaSetSize: " + StringUtility::ToWString(r->Partition->TargetReplicaSetSize) + Environment::NewLine)
        + ((!(r->Partition->Service->DiagnosticsData.placementConstraints_.empty())) ? (L"Placement Constraint: " + r->Partition->Service->DiagnosticsData.placementConstraints_ + Environment::NewLine) : L"Placement Constraint: N/A" + Environment::NewLine)
        + ((r->Partition->Service->DependedService) ? (L"Parent Service: " + r->Partition->Service->DependedService->Name + Environment::NewLine + Environment::NewLine) : L"Parent Service: N/A" + Environment::NewLine + Environment::NewLine)
        + L"Constraint Elimination Sequence:" + Environment::NewLine;
}

std::wstring PLBDiagnostics::PartitionDetailString(PartitionEntry const* p)
{
    return L" " + Environment::NewLine
        + (L"TargetReplicaSetSize: " + StringUtility::ToWString(p->TargetReplicaSetSize) + Environment::NewLine)
        + ((!(p->Service->DiagnosticsData.placementConstraints_.empty())) ? (L"Placement Constraint: " + p->Service->DiagnosticsData.placementConstraints_ + Environment::NewLine) : L"Placement Constraint: N/A" + Environment::NewLine)
        + ((p->Service->DependedService) ? (L"Parent Service: " + p->Service->DependedService->Name + Environment::NewLine + Environment::NewLine) : L"Parent Service: N/A" + Environment::NewLine + Environment::NewLine)
        + L"Constraint Elimination Sequence:" + Environment::NewLine;
}

std::wstring PLBDiagnostics::NodeSetDetailString(NodeSet nodes, NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr /*= nullptr*/)
{
    std::wstring detailString = L"";
    std::wstring cache = L"";
    std::wstring fDomainID = L"";
    auto & nodeTable = plbNodes_;
    int counter = 0;

    nodes.ForEach([&](NodeEntry const * n) -> bool
    {
        if (nodeTable.size() > n->NodeIndex)
        {
            auto& nodeDescObj = nodeTable[n->NodeIndex].NodeDescriptionObj;

            fDomainID = L"";
            for (auto const& wsIter : nodeDescObj.FaultDomainId) {fDomainID += wsIter;}
            cache = StringUtility::ToWString(nodeDescObj.NodeProperties);
            detailString += cache.substr(1, cache.size() - 2) + L" UpgradeDomain: ud:/" +nodeDescObj.UpgradeDomainId + L" Deactivation Intent/Status: " + StringUtility::ToWString(nodeDescObj.DeactivationIntent) + L"/" + StringUtility::ToWString(nodeDescObj.DeactivationStatus) + Environment::NewLine;
            if (nodeToConstraintDiagnosticsDataMapSPtr != nullptr && nodeToConstraintDiagnosticsDataMapSPtr->count(n->NodeId))
            {
                detailString += (nodeToConstraintDiagnosticsDataMapSPtr->at(n->NodeId))->DiagnosticsMessage() + Environment::NewLine + L"--"+ Environment::NewLine;
            }

            ++counter;
        }
        else
        {
            detailString += L"Trace Mismatch for NodeId: " + StringUtility::ToWString(n->NodeId) + Environment::NewLine;
        }

        return counter < PLBConfig::GetConfig().DetailedNodeListLimit;
    });

    return detailString;
}

void PLBDiagnostics::GenerateUnplacedTraceandNodeDetailMessages(
    std::vector<IConstraintUPtr> const & constraints,
    std::vector<DiagnosticSubspace> & subspaces,
    PlacementReplica const* r,
    size_t totalNodeCount,
    std::wstring & traceMessage,
    std::wstring & nodeDetailMessage,
    bool trackEliminatedNodes,
    bool plbMovementsBeingDropped)
{
    size_t runningCount = 0;
    traceMessage = PLBDiagnostics::PartitionDetailString(r->Partition);
    nodeDetailMessage = trackEliminatedNodes ? (Environment::NewLine + L"Nodes Eliminated By Constraints:" + Environment::NewLine + Environment::NewLine) : L"";

    auto conIter = constraints.begin();

    for (auto subIter = subspaces.begin(); subIter != subspaces.end(); ++subIter)
    {
        if ((subIter->Count != 0) && (subIter->Count != std::numeric_limits<size_t>::max()))
        {
            std::wstring constraintName;
            std::wstring friendlyConstraintName;

            runningCount += subIter->Count;

            if ((*conIter)->Type != IConstraint::PlacementConstraint || ((*conIter)->Type == IConstraint::PlacementConstraint) && r->Partition->Service->DiagnosticsData.placementConstraints_.empty())
            {

                if ((*conIter)->Type != IConstraint::PlacementConstraint)
                {
                    if (((*conIter)->Type == IConstraint::ReplicaExclusionDynamic) && plbMovementsBeingDropped)
                    {
                        constraintName = L"RejectedMovement";

                        if (trackEliminatedNodes)
                        {
                            friendlyConstraintName = L"RejectedMovement -- Replica Placements Targeting these Nodes are not being Executed, possibly due to Nodes in Disabling status";
                        }
                    }
                    else
                    {
                        constraintName = ShortFriendlyConstraintName((*conIter)->Type);

                        if (trackEliminatedNodes)
                        {
                            friendlyConstraintName = FriendlyConstraintName((*conIter)->Type, r, nullptr);
                        }
                    }
                }
                else
                {
                    constraintName = L"ServiceTypeDisabled/NodesBlockListed";

                    if (trackEliminatedNodes)
                    {
                        friendlyConstraintName = L"ServiceTypeDisabled/NodesBlockListed -- Nodes must not have ServiceType Disabled or be BlockListed due to Node's Pause/Deactivate Status";
                    }
                }

                traceMessage += wformatString("{0} eliminated {1} possible node(s) for placement -- {2}/{3} node(s) remain.",
                    constraintName,
                    subIter->Count,
                    totalNodeCount - runningCount,
                    totalNodeCount)
                    + Environment::NewLine;
            }
            else
            {
                if (r->IsPrimary)
                {
                    constraintName = L"{" + StringUtility::ToWString(r->Partition->Service->PrimaryReplicaBlockList.Count) +
                                               L" Primary PlacementPolicy Nodes}, {" +
                                               StringUtility::ToWString(r->Partition->Service->ServiceBlockListCount) +
                                               L" PlacementConstraint Nodes}, {BlockList Nodes}";

                    traceMessage += wformatString("({0})'s set union eliminated {1} possible node(s) for placement -- {2}/{3} node(s) remain.",
                        constraintName,
                        subIter->Count,
                        totalNodeCount - runningCount,
                        totalNodeCount)
                        + Environment::NewLine;
                    //For the detailed traces and health alert formatting
                    constraintName = L"Set Union of " + constraintName;
                }
                else
                {
                    constraintName = L"PlacementConstraint + ServiceTypeDisabled/NodesBlockListed";

                    if (trackEliminatedNodes)
                    {
                        friendlyConstraintName = L"PlacementConstraint + ServiceTypeDisabled/NodesBlockListed -- PlacementProperties must Satisfy Service's PlacementConstraint, and Nodes must not have had the ServiceType Disabled or be BlockListed due to Node's Pause/Deactivate Status";
                    }

                    size_t appropriateBlockListCount = r->Partition->Service->ServiceBlockListCount;
                    traceMessage += wformatString("{0} eliminated {1} possible node(s) for placement -- {2}/{3} node(s) remain (total of {4} nodes blocklisted for this service).",
                        constraintName,
                        subIter->Count,
                        totalNodeCount - runningCount,
                        totalNodeCount,
                        appropriateBlockListCount)
                        + Environment::NewLine;
                }
            }

            if (trackEliminatedNodes)
            {
                //Node Details
                nodeDetailMessage += (friendlyConstraintName.empty() ? constraintName : friendlyConstraintName) + L":" + Environment::NewLine + L"--" + Environment::NewLine + subIter->NodeDetail + Environment::NewLine;
            }
        }

        ++conIter;
    }
}

void PLBDiagnostics::GenerateUnswappedUpgradeTraceMessage(
   std::vector<IConstraintUPtr> const & constraints,
   std::vector<DiagnosticSubspace> & subspaces,
   PartitionEntry const* p,
   size_t totalNodeCount,
   std::wstring & traceMessage,
   std::wstring & nodeDetailMessage,
   bool trackEliminatedNodes,
   bool plbMovementsBeingDropped)
{
    size_t runningCount = 0;
    traceMessage = PLBDiagnostics::PartitionDetailString(p);
    nodeDetailMessage = trackEliminatedNodes ? (Environment::NewLine + L"Nodes Eliminated By Constraints:" + Environment::NewLine + Environment::NewLine) : L"";

    auto conIter = constraints.begin();

    for (auto subIter = subspaces.begin(); subIter != subspaces.end(); ++subIter)
    {
        if ((subIter->Count != 0) && (subIter->Count != std::numeric_limits<size_t>::max()))
        {
            std::wstring constraintName;
            std::wstring friendlyConstraintName;

            runningCount += subIter->Count;

            if ((*conIter)->Type != IConstraint::PlacementConstraint || ((*conIter)->Type == IConstraint::PlacementConstraint) && p->Service->DiagnosticsData.placementConstraints_.empty())
            {

                if ((*conIter)->Type != IConstraint::PlacementConstraint)
                {
                    if ((*conIter)->Type == IConstraint::ReplicaExclusionStatic)
                    {
                        constraintName = L"IsInTransition";

                        if (trackEliminatedNodes)
                        {
                            friendlyConstraintName =  L"IsInTransition -- Nodes must not have Replicas of this partition currently in Transition";
                        }
                    }
                    else
                    {
                        if (((*conIter)->Type == IConstraint::ReplicaExclusionDynamic) && plbMovementsBeingDropped)
                        {
                            constraintName = L"RejectedMovement";

                            if (trackEliminatedNodes)
                            {
                                friendlyConstraintName = L"RejectedMovement -- Replica Placements Targeting these Nodes are not being Executed, possibly due to Nodes in Disabling status";
                            }
                        }
                        else
                        {
                            constraintName = ShortFriendlyConstraintName((*conIter)->Type);

                            if (trackEliminatedNodes)
                            {
                                friendlyConstraintName = FriendlyConstraintName((*conIter)->Type, nullptr, p);
                            }
                        }
                    }
                }
                else
                {
                    constraintName = L"ServiceTypeDisabled/NodesBlockListed";

                    if (trackEliminatedNodes)
                    {
                        friendlyConstraintName = L"ServiceTypeDisabled/NodesBlockListed -- Nodes must not have ServiceType Disabled or be BlockListed due to Node's Pause/Deactivate Status";
                    }
                }

                traceMessage += wformatString("{0} eliminated {1} possible node(s) for placement -- {2}/{3} node(s) remain.",
                    constraintName,
                    subIter->Count,
                    totalNodeCount - runningCount,
                    totalNodeCount)
                    + Environment::NewLine;
            }
            else
            {
                constraintName = L"{" + StringUtility::ToWString(p->Service->PrimaryReplicaBlockList.Count) +
                                           L" Primary PlacementPolicy Nodes}, {" +
                                           StringUtility::ToWString(p->Service->ServiceBlockListCount) +
                                           L" PlacementConstraint Nodes}, {BlockList Nodes}";

                traceMessage += wformatString("({0})'s set union eliminated {1} possible node(s) for placement -- {2}/{3} node(s) remain.",
                    constraintName,
                    subIter->Count,
                    totalNodeCount - runningCount,
                    totalNodeCount)
                    + Environment::NewLine;
                //For the detailed traces and health alert formatting
                constraintName = L"Set Union of " + constraintName;
            }

            if (trackEliminatedNodes)
            {
                //Node Details
                nodeDetailMessage += (friendlyConstraintName.empty() ? constraintName : friendlyConstraintName) + L":" + Environment::NewLine + L"--" + Environment::NewLine + subIter->NodeDetail + Environment::NewLine;
            }
        }

        ++conIter;
    }
}
