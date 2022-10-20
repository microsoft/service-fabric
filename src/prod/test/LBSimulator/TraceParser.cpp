// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FM.h"
#include "TraceParser.h"
#include "Utility.h"
#include "LBSimulatorConfig.h"
#include "RegexUtility.h"

using namespace std;
using namespace Common;
using namespace LBSimulator;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::LoadBalancingComponent;

wstring const TraceParser::PLBConstructTrace = L"PLB.PLBConstruct";
wstring const TraceParser::UpdateNode = L"PLB.UpdateNode";
wstring const TraceParser::UpdateApplication = L"PLB.UpdateApplication";
wstring const TraceParser::UpdateServiceType = L"PLB.UpdateServiceType";
wstring const TraceParser::UpdateService = L"PLB.UpdateService";
wstring const TraceParser::UpdateServicePackage = L"PLB.UpdateServicePackage";
wstring const TraceParser::UpdateFailoverUnitTrace = L"PLB.UpdateFailoverUnit";
wstring const TraceParser::UpdateLoadOnNode = L"PLB.UpdateLoadOnNode";
wstring const TraceParser::UpdateLoad = L"PLB.UpdateLoad";
wstring const TraceParser::PLBProcessPendingUpdatesTrace = L"PLB.PLBProcessPendingUpdates";
wstring const TraceParser::ReplicaFlagsString = L"DRPNZLMIJKEVT";
wstring const TraceParser::FailoverUnitFlagString = L"SPEDWOB";

TraceParser::TraceParser(Reliability::LoadBalancingComponent::PLBConfig & plbConfig)
    : config_(plbConfig), guidToServiceName_()
{
}

void TraceParser::Parse(wstring const& fileName)
{
    wstring fileTextW;
    Utility::LoadFile(fileName, fileTextW);
    size_t pos = 0, oldPos = 0;
    wstring lineBuf = Utility::ReadLine(fileTextW, pos);

    bool foundConstructTrace = false;
    // Find first PLB construct trace
    while (pos != std::wstring::npos && oldPos != pos)
    {
        StringUtility::TrimWhitespaces(lineBuf);
        if (StringUtility::Contains<wstring>(lineBuf, TraceParser::PLBConstructTrace))
        {
            foundConstructTrace = true;
            break;
        }

        oldPos = pos;
        lineBuf = Utility::ReadLine(fileTextW, pos);
    }

    // Check if we have PLB construct trace
    if (foundConstructTrace == false)
    {
        return;
    }

    oldPos = pos;
    lineBuf = Utility::ReadLine(fileTextW, pos);

    while (pos != std::wstring::npos &&
        oldPos != pos &&
        !StringUtility::Contains<wstring>(lineBuf, TraceParser::PLBProcessPendingUpdatesTrace))
    {
        if (StringUtility::Contains<wstring>(lineBuf, TraceParser::UpdateNode))
        {
            Reliability::LoadBalancingComponent::NodeDescription nodeDescription = ParseNodeUpdate(lineBuf);
            nodes_.push_back(move(nodeDescription));
        }
        else if (StringUtility::Contains<wstring>(lineBuf, TraceParser::UpdateApplication))
        {
            ApplicationDescription applicationDescription = ParseApplicationUpdate(lineBuf);
            applications_.push_back(move(applicationDescription));
        }
        else if (StringUtility::Contains<wstring>(lineBuf, TraceParser::UpdateServiceType))
        {
            ServiceTypeDescription serviceTypeDescription = ParseServiceTypeUpdate(lineBuf);
            serviceTypes_.push_back(move(serviceTypeDescription));
        }
        else if (StringUtility::Contains<wstring>(lineBuf, TraceParser::UpdateService) &&
            !StringUtility::Contains<wstring>(lineBuf, TraceParser::UpdateServiceType) &&
            !StringUtility::Contains<wstring>(lineBuf, TraceParser::UpdateServicePackage))
        {
            Reliability::LoadBalancingComponent::ServiceDescription serviceDescription = ParseServiceUpdate(lineBuf);
            services_.push_back(move(serviceDescription));
        }
        else if (StringUtility::Contains<wstring>(lineBuf, TraceParser::UpdateFailoverUnitTrace))
        {
            FailoverUnitDescription failoverUnitDescription = ParseFailoverUnitUpdate(lineBuf);
            failoverUnits_.push_back(move(failoverUnitDescription));
        }
        else if (StringUtility::Contains<wstring>(lineBuf, TraceParser::UpdateLoadOnNode))
        {
            LoadOrMoveCostDescription loadOrMoveCostDescription = ParseLoadOnNodeUpdate(lineBuf);
            loadAndMoveCosts_.push_back(move(loadOrMoveCostDescription));
        }
        else if (StringUtility::Contains<wstring>(lineBuf, TraceParser::UpdateLoad))
        {
            Reliability::LoadBalancingComponent::LoadOrMoveCostDescription loadOrMoveCostDescription = ParseLoadUpdate(lineBuf);
            loadAndMoveCosts_.push_back(move(loadOrMoveCostDescription));
        }

        oldPos = pos;
        lineBuf = Utility::ReadLine(fileTextW, pos);
    }

    
}

NodeDescription TraceParser::ParseNodeUpdate(std::wstring const & updateNodeTrace)
{
    Federation::NodeInstance nodeInstance;
    bool isUp = false;
    map<std::wstring, std::wstring> nodeProperties;
    Reliability::NodeDeactivationIntent::Enum deactivationIntent = Reliability::NodeDeactivationIntent::Enum::None;
    Reliability::NodeDeactivationStatus::Enum deactivationStatus = Reliability::NodeDeactivationStatus::Enum::None;
    NodeDescription::DomainId faultDomainId;
    std::wstring upgradeDomainId;
    std::map<std::wstring, uint> capacityRatios;
    std::map<std::wstring, uint> capacities;

    wstring nodeInstanceString = RegexUtility::ExtractStringInBetween(updateNodeTrace, L"Updating node with ", L" ");

    if (!NodeInstance::TryParse(nodeInstanceString, nodeInstance))
    {
        Assert::CodingError("Can't convert NodeInstance string {0} to NodeInstance while parsing node update trace", nodeInstanceString.c_str());
    }

    wstring nodePropertiesString = RegexUtility::ExtractStringInBetween(updateNodeTrace, L"\\(\\(", L"\\)\\)");

    StringCollection nodeProperitesStringCollection;
    StringUtility::Split<wstring>(nodePropertiesString, nodeProperitesStringCollection, L" ");
    for (auto iter = nodeProperitesStringCollection.begin(); iter != nodeProperitesStringCollection.end(); ++iter)
    {
        wstring propertyName = iter->substr(0, iter->find(':'));
        wstring propertyValue = iter->substr(iter->find(':') + 1, wstring::npos);
        nodeProperties.insert(make_pair(propertyName, propertyValue));
    }

    wstring nodesAtributes = RegexUtility::ExtractStringInBetween(updateNodeTrace, L"\\) ", L" \\(.* \\(");

    StringCollection nodeAtributesCollection;
    StringUtility::Split<wstring>(nodesAtributes, nodeAtributesCollection, L" ");

    if (nodeAtributesCollection.size() != 5)
    {
        Assert::CodingError("Wrong number of arguments while parsing node update trace");
    }

    wstring faultDomainString = RegexUtility::ExtractStringInBetween(nodeAtributesCollection[0], L"\\(", L"\\)");
    faultDomainId = faultDomainString.empty() ?
        NodeDescription::DomainId() : Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", faultDomainString).Segments;

    upgradeDomainId = nodeAtributesCollection[1];

    wstring isUpString = nodeAtributesCollection[2];
    if (!isUpString.compare(L"true"))
    {
        isUp = true;
    }
    else if (!isUpString.compare(L"false"))
    {
        isUp = false;
    }
    else
    {
        Assert::CodingError("Can't convert isUp node while parsing node update trace");
    }

    wstring deactivationIntentString = nodeAtributesCollection[3];
    if (!deactivationIntentString.compare(L"None"))
    {
        deactivationIntent = Reliability::NodeDeactivationIntent::Enum::None;
    }
    else if (!deactivationIntentString.compare(L"Pause"))
    {
        deactivationIntent = Reliability::NodeDeactivationIntent::Enum::Pause;
    }
    else if (!deactivationIntentString.compare(L"Restart"))
    {
        deactivationIntent = Reliability::NodeDeactivationIntent::Enum::Restart;
    }
    else if (!deactivationIntentString.compare(L"RemoveData"))
    {
        deactivationIntent = Reliability::NodeDeactivationIntent::Enum::RemoveData;
    }
    else if (!deactivationIntentString.compare(L"RemoveNode"))
    {
        deactivationIntent = Reliability::NodeDeactivationIntent::Enum::RemoveNode;
    }
    else
    {
        Assert::CodingError("Can't convert node deactivation intent while parsing node update trace");
    }

    wstring deactivationStatusString = nodeAtributesCollection[4];
    if (!deactivationStatusString.compare(L"DeactivationSafetyCheckInProgress"))
    {
        deactivationStatus = Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress;
    }
    else if (!deactivationStatusString.compare(L"DeactivationSafetyCheckComplete"))
    {
        deactivationStatus = Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckComplete;
    }
    else if (!deactivationStatusString.compare(L"DeactivationComplete"))
    {
        deactivationStatus = Reliability::NodeDeactivationStatus::Enum::DeactivationComplete;
    }
    else if (!deactivationStatusString.compare(L"ActivationInProgress"))
    {
        deactivationStatus = Reliability::NodeDeactivationStatus::Enum::ActivationInProgress;
    }
    else if (!deactivationStatusString.compare(L"None"))
    {
        deactivationStatus = Reliability::NodeDeactivationStatus::Enum::None;
    }
    else
    {
        Assert::CodingError("Can't convert node deactivation status while parsing node update trace");
    }

    wstring nodesCapacitiesRations = RegexUtility::ExtractStringInBetween(updateNodeTrace, L"(?:.*) \\(", L"\\) \\((?:.*)\\)[ 0-9]*$");
    capacityRatios = CreateCapacities(nodesCapacitiesRations);

    wstring nodesCapacities = RegexUtility::ExtractStringInBetween(updateNodeTrace, L"(?:.*) \\(", L"\\)[ 0-9]*$");
    capacities = CreateCapacities(nodesCapacities);

    for (auto capacityIter = capacities.begin(); capacityIter != capacities.end(); ++capacityIter)
    {
        if (capacityIter->first == ServiceModel::Constants::SystemMetricNameCpuCores)
        {
            // it will be corrected again at the PLB creation
            capacityIter->second = capacityIter->second / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
        }
    }

    NodeDescription nodeDescription(nodeInstance, isUp, deactivationIntent, deactivationStatus, move(nodeProperties),
        move(faultDomainId), move(upgradeDomainId), move(capacityRatios), move(capacities));

    return nodeDescription;
}

ApplicationDescription TraceParser::ParseApplicationUpdate(wstring const & updateApplicationTrace)
{
    wstring applicationName;
    map<wstring, ApplicationCapacitiesDescription> capacities;
    uint64 applicationId;
    int minimumNodes;
    int scaleoutCount;
    bool upgradeInProgress;
    set<std::wstring> completedUDs;

    applicationName = RegexUtility::ExtractStringInBetween(updateApplicationTrace, L"Updating application with ", L" ");
    applicationId = RegexUtility::ExtractValueByKey<uint64>(updateApplicationTrace, L"id");

    wstring capacitiesString = RegexUtility::ExtractStringByKey(updateApplicationTrace, L"capacities");

    minimumNodes = RegexUtility::ExtractValueByKey<int>(updateApplicationTrace, L"minimumNodes");
    scaleoutCount = RegexUtility::ExtractValueByKey<int>(updateApplicationTrace, L"scaleoutCount");
    upgradeInProgress = RegexUtility::ExtractBoolInBetween(updateApplicationTrace, L"upgradeInProgress: ", L",");

    if (upgradeInProgress)
    {
        wstring udsString = L"";
        udsString = RegexUtility::ExtractStringInBetween(updateApplicationTrace, L"upgradeCompletedUDs: ", L"$");
        StringCollection uds;
        StringUtility::Split<wstring>(udsString, uds, L" ");
        for (auto it = uds.begin(); it != uds.end(); ++it)
        {
            completedUDs.insert(*it);
        }
    }

    ApplicationDescription applicationDescription(
        move(applicationName),
        move(capacities),
        minimumNodes,
        scaleoutCount,
        std::map<ServiceModel::ServicePackageIdentifier, Reliability::LoadBalancingComponent::ServicePackageDescription> (),
        std::map<ServiceModel::ServicePackageIdentifier, Reliability::LoadBalancingComponent::ServicePackageDescription> (),
        upgradeInProgress,
        move(completedUDs));

    applicationDescription.ApplicationId = applicationId;

    return applicationDescription;
}

ServiceTypeDescription TraceParser::ParseServiceTypeUpdate(wstring const & updateServiceTypeTrace)
{
    wstring serviceTypeName;
    set<NodeId> nodes;

    serviceTypeName = RegexUtility::ExtractStringInBetween(updateServiceTypeTrace, L"Updating service type with ", L" blockList");

    wstring nodesString = RegexUtility::ExtractStringInBetween(updateServiceTypeTrace, L"blockList:\\(", L"\\)");

    StringCollection nodesCollection;
    StringUtility::Split<wstring>(nodesString, nodesCollection, L" ");

    for (wstring const& nodeStr : nodesCollection)
    {
        NodeId id;
        if (!NodeId::TryParse(nodeStr, id))
        {
            Assert::CodingError("Can't convert NodeInstance string {0} to NodeInstance in UpdateServiceType trace", nodeStr.c_str());
        }
        nodes.insert(move(id));
    }

    ServiceTypeDescription serviceTypeDescription(move(serviceTypeName), move(nodes));
    return serviceTypeDescription;
}

Reliability::LoadBalancingComponent::ServiceDescription TraceParser::ParseServiceUpdate(std::wstring const & updateServiceTrace)
{
    wstring serviceName;
    wstring serviceTypeName;
    wstring applicationName;
    bool stateful;
    wstring placementConstraints;
    wstring affinitizedService;
    bool alignedAffinity;
    vector<ServiceMetric> metrics;
    uint defaultMoveCost;
    bool onEveryNode;
    int partitionCount;
    int targetReplicaSetSize;

    serviceName = RegexUtility::ExtractStringInBetween(updateServiceTrace, L"Updating service with ", L" ");
    serviceTypeName = RegexUtility::ExtractStringByKey(updateServiceTrace, L"type");
    applicationName = RegexUtility::ExtractStringInBetween(updateServiceTrace, L"application:", L"stateful");
    applicationName = applicationName.substr(0, applicationName.size() - 1);
    stateful = RegexUtility::ExtractBoolByKey(updateServiceTrace, L"stateful");
    placementConstraints = RegexUtility::ExtractStringInBetween(updateServiceTrace, L"placementConstraints:", L" affinity");
    affinitizedService = RegexUtility::ExtractStringInBetween(updateServiceTrace, L"affinity:", L"alignedAffinity");
    affinitizedService = affinitizedService.substr(0, affinitizedService.size() - 1);
    alignedAffinity = RegexUtility::ExtractBoolByKey(updateServiceTrace, L"alignedAffinity");

    wstring metricsString = RegexUtility::ExtractStringInBetween(updateServiceTrace, L"metrics:\\(", L"\\)");
    replace(metricsString.begin(), metricsString.end(), ' ', ',');
    metrics = CreateMetrics(metricsString);

    defaultMoveCost = RegexUtility::ExtractValueByKey<uint>(updateServiceTrace, L"defaultMoveCost");
    onEveryNode = RegexUtility::ExtractBoolByKey(updateServiceTrace, L"everyNode");
    partitionCount = RegexUtility::ExtractValueByKey<int>(updateServiceTrace, L"partitionCount");
    targetReplicaSetSize = RegexUtility::ExtractValueByKey<int>(updateServiceTrace, L"targetReplicaSetSize");

    Reliability::LoadBalancingComponent::ServiceDescription serviceDescription(move(serviceName), move(serviceTypeName), 
        move(applicationName), stateful, move(placementConstraints),move(affinitizedService), alignedAffinity, move(metrics), 
        defaultMoveCost, onEveryNode, partitionCount, targetReplicaSetSize);

    return serviceDescription;
}

FailoverUnitDescription TraceParser::ParseFailoverUnitUpdate(wstring const & failoverUnitTrace)
{
    Guid guid;
    wstring serviceName;
    int64 version;
    vector<ReplicaDescription> replicas;
    int replicaDiff;
    int targetReplicaSetSize;
    Reliability::FailoverUnitFlags::Flags ftFlags;
    bool isReconfigurationInProgress = false;
    bool isInQuorumLost = false;

    wstring guidString = RegexUtility::ExtractStringInBetween(failoverUnitTrace, L"Updating failover unit with ", L" ");

    if (!Guid::TryParse(guidString, guid))
    {
        Assert::CodingError("Can't convert GUID string {0} to GUID while parsing failover unit update", guidString.c_str());
    }

    serviceName = RegexUtility::ExtractStringByKey(failoverUnitTrace, L"service");

    auto it = guidToServiceName_.find(Guid(guidString));
    if (it == guidToServiceName_.end())
    {
        guidToServiceName_.insert(make_pair(Guid(guidString), serviceName));
    }

    version = RegexUtility::ExtractValueByKey<int64>(failoverUnitTrace, L"version");
    replicaDiff = RegexUtility::ExtractValueByKey<int>(failoverUnitTrace, L"replicaDiff");
    targetReplicaSetSize = RegexUtility::ExtractValueByKey<int>(failoverUnitTrace, L"targetReplicaSetSize");

    wstring failoverUnitFlags = RegexUtility::ExtractStringByKey(failoverUnitTrace, L"flags");

    bool stateful = false;
    bool persisted = false;
    bool noData = false;
    bool toBeDeleted = false;
    bool swapPrimary = false;
    bool orphaned = false;
    bool upgrading = false;

    if (failoverUnitFlags.find(L"-") != wstring::npos)
    {
        ftFlags = Reliability::FailoverUnitFlags::Flags::None;
    }
    else
    {
        if (failoverUnitFlags.find(L"S") != wstring::npos)
        {
            stateful = true;
        }

        if (failoverUnitFlags.find(L"P") != wstring::npos)
        {
            persisted = true;
        }

        if (failoverUnitFlags.find(L"E") != wstring::npos)
        {
            noData = true;
        }

        if (failoverUnitFlags.find(L"D") != wstring::npos)
        {
            toBeDeleted = true;
        }

        if (failoverUnitFlags.find(L"W") != wstring::npos)
        {
            swapPrimary = true;
        }

        if (failoverUnitFlags.find(L"O") != wstring::npos)
        {
            orphaned = true;
        }

        if (failoverUnitFlags.find(L"B") != wstring::npos)
        {
            upgrading = true;
        }

        ftFlags = Reliability::FailoverUnitFlags::Flags(
            stateful,
            persisted,
            noData,
            toBeDeleted,
            swapPrimary,
            orphaned,
            upgrading);
    }

    wstring plbFlags = RegexUtility::ExtractStringByKey(failoverUnitTrace, L"plbFlags");

    bool quorumLost = false;
    bool reconfigurationInProgress = false;

    if (plbFlags.find(L"Q") != wstring::npos)
    {
        quorumLost = true;
    }

    if (plbFlags.find(L"C") != wstring::npos)
    {
        reconfigurationInProgress = true;
    }


    wstring replicasString = RegexUtility::ExtractStringInBetween(failoverUnitTrace, L"replicas:\\(", L"\\)");

    StringCollection replicasStringCollection;
    StringUtility::Split<wstring>(replicasString, replicasStringCollection, L" ");

    int cnt = 0;
    while (cnt < replicasStringCollection.size())
    {
        NodeInstance nodeInstance;
        ReplicaRole::Enum role = ReplicaRole::Primary;
        ReplicaStates::Enum state = ReplicaStates::Ready;
        bool isUp = true;
        ReplicaFlags::Enum flags = ReplicaFlags::Enum::None;

        wstring nodeInstanceString = replicasStringCollection[cnt++];
        if (!NodeInstance::TryParse(nodeInstanceString, nodeInstance))
        {
            Assert::CodingError("Can't convert NodeInstance string {0} to NodeInstance", nodeInstanceString.c_str());
        }

        if (cnt == replicasStringCollection.size())
        {
            Assert::CodingError("Invalid replica string: {0}", replicasString.c_str());
        }

        wstring replicaRole = replicasStringCollection[cnt++];

        if (!replicaRole.compare(L"Primary"))
        {
            role = ReplicaRole::Primary;
        }
        else if (!replicaRole.compare(L"Secondary"))
        {
            role = ReplicaRole::Secondary;
        }
        else if (!replicaRole.compare(L"None"))
        {
            role = ReplicaRole::None;
        }
        else if (!replicaRole.compare(L"StandBy"))
        {
            role = ReplicaRole::StandBy;
        }
        else if (!replicaRole.compare(L"Dropped"))
        {
            role = ReplicaRole::Dropped;
        }
        else
        {
            Assert::CodingError("Invalid replica role: {0}", replicaRole.c_str());
        }

        wstring replicaState = L"";
        wstring replicaFlags = L"";
        wstring replicaIsUp = L"";
        wstring replicaInTransition = L"";
        wstring fuInTransition = L"";

        NodeInstance checkIsNode;
        if (cnt != replicasStringCollection.size() && !NodeInstance::TryParse(replicasStringCollection[cnt], checkIsNode))
        {
            replicaState = replicasStringCollection[cnt++];
        }

        if (!replicaState.compare(L"SB"))
        {
            state = ReplicaStates::StandBy;
        }
        else if (!replicaState.compare(L"IB"))
        {
            state = ReplicaStates::InBuild;
        }
        else if (!replicaState.compare(L"DD"))
        {
            state = ReplicaStates::Dropped;
        }

        if (state == ReplicaStates::Ready)
        {
            replicaFlags = replicaState;
        }
        else if (cnt != replicasStringCollection.size() && !NodeInstance::TryParse(replicasStringCollection[cnt], checkIsNode))
        {
            replicaFlags = replicasStringCollection[cnt++];
        }

        if (replicaFlags.compare(L"Down") && replicaFlags.compare(L"InTransition") && replicaFlags.compare(L"FuNotInTransition"))
        {
            if (!CheckReplicaFlags(replicaFlags))
            {
                Assert::CodingError("Replica flags not correct", replicasString.c_str());
            }

            if (replicaFlags.find(L"D") != wstring::npos)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::ToBeDroppedByFM);
            }

            if (replicaFlags.find(L"R") != wstring::npos)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::ToBeDroppedByPLB);
            }

            if (replicaFlags.find(L"P") != wstring::npos)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::ToBePromoted);
            }

            if (replicaFlags.find(L"N") != wstring::npos)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::PendingRemove);
            }

            if (replicaFlags.find(L"Z") != wstring::npos)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::Deleted);
            }

            if (replicaFlags.find(L"L") != wstring::npos)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::PreferredPrimaryLocation);
            }

            if (replicaFlags.find(L"M") != wstring::npos)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::PreferredReplicaLocation);
            }

            if (replicaFlags.find(L"I") != wstring::npos)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::PrimaryToBeSwappedOut);
            }

            if (replicaFlags.find(L"J") != wstring::npos)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::PrimaryToBePlaced);
            }

            if (replicaFlags.find(L"K") != wstring::npos)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::ReplicaToBePlaced);
            }

            if (replicaFlags.find(L"E") != wstring::npos)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::EndpointAvailable);
            }

            if (replicaFlags.find(L"V") != wstring::npos)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::MoveInProgress);
            }

            if (replicaFlags.find(L"T") != wstring::npos)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::ToBeDroppedForNodeDeactivation);
            }
        }

        if (flags == ReplicaFlags::Enum::None)
        {
            replicaIsUp = replicaFlags;
        }
        else if (cnt != replicasStringCollection.size() && !NodeInstance::TryParse(replicasStringCollection[cnt], checkIsNode))
        {
            replicaIsUp = replicasStringCollection[cnt++];
        }

        if (!replicaIsUp.compare(L"Down"))
        {
            isUp = false;
        }

        if (isUp == true)
        {
            replicaInTransition = replicaIsUp;
        }
        else if (cnt != replicasStringCollection.size() && !NodeInstance::TryParse(replicasStringCollection[cnt], checkIsNode))
        {
            replicaInTransition = replicasStringCollection[cnt++];
        }

        if (replicaInTransition.compare(L"InTransition"))
        {
            fuInTransition = replicaInTransition;
        }
        else if (cnt != replicasStringCollection.size() && !NodeInstance::TryParse(replicasStringCollection[cnt], checkIsNode))
        {
            fuInTransition = replicasStringCollection[cnt++];
        }

        if (!fuInTransition.empty() && fuInTransition.compare(L"FuNotInTransition"))
        {
            Assert::CodingError("Can't convert Replica string {0} to ReplicaDescription in UpdateFailoverUnit trace", replicasString.c_str());
        }

        replicas.push_back(ReplicaDescription(
            nodeInstance,
            role,
            state,
            isUp,
            flags));
    }

    FailoverUnitDescription failoverUnitDescription(
        guid,
        move(serviceName),
        version, 
        move(replicas),
        replicaDiff,
        ftFlags,
        isReconfigurationInProgress,
        isInQuorumLost,
        targetReplicaSetSize);

    return failoverUnitDescription;
}

LoadOrMoveCostDescription TraceParser::ParseLoadOnNodeUpdate(std::wstring const & loadOnNodeUpdateTrace)
{
    Guid partitionGuid;
    wstring serviceName;
    wstring replicaRole;
    wstring metricName;
    uint load;
    NodeId nodeId;

    wstring partitionGuidString = RegexUtility::ExtractStringInBetween(loadOnNodeUpdateTrace, L"UpdateLoadOnNode@", L"\\s");

    if (!Guid::TryParse(partitionGuidString, partitionGuid))
    {
        Assert::CodingError("Can't convert GUID string {0} to GUID while parsing update load on node", partitionGuidString.c_str());
    }

    auto it = guidToServiceName_.find(partitionGuid);
    if (it == guidToServiceName_.end())
    {
        Assert::CodingError("Can't map GUID {0} to service name while parsing update load on node", partitionGuid.ToString());
    }

    serviceName = it->second;

    replicaRole = RegexUtility::ExtractStringInBetween(loadOnNodeUpdateTrace, L"Updating load for role ", L" with");

    wstring metricString = RegexUtility::ExtractStringInBetween(loadOnNodeUpdateTrace, L"Updating load for role [^\\s]* with ", L" ");

    metricName = metricString.substr(0, metricString.find_last_of(L":"));
    load = RegexUtility::ExtractValueByKey<uint>(metricString, metricName);

    wstring nodeIdString = RegexUtility::ExtractStringInBetween(loadOnNodeUpdateTrace, L"on node ", L"$");

    if (!NodeId::TryParse(nodeIdString, nodeId))
    {
        Assert::CodingError("Can't convert NodeId {0} while parsing update load on node", nodeIdString.c_str());
    }

    LoadOrMoveCostDescription loadDescription(partitionGuid, move(serviceName), true);
    StopwatchTime now = Stopwatch::Now();
    if (replicaRole == L"Primary")
    {
        vector<LoadMetric> primaryEntries;
        primaryEntries.push_back(LoadMetric(wstring(metricName), load));
        loadDescription.MergeLoads(ReplicaRole::Primary, move(primaryEntries), now, false);
    }
    else
    {
        vector<LoadMetric> secondaryEntries;
        secondaryEntries.push_back(LoadMetric(wstring(metricName), load));
        loadDescription.MergeLoads(ReplicaRole::Secondary, move(secondaryEntries), now, true, nodeId);
    }
    
    return loadDescription;
}

LoadOrMoveCostDescription TraceParser::ParseLoadUpdate(std::wstring const & loadUpdateTrace)
{
    Guid partitionGuid;
    wstring serviceName;
    wstring metricName;
    uint load;
    ReplicaRole::Enum replicaRole = ReplicaRole::Enum::None;

    wstring partitionGuidString = RegexUtility::ExtractStringInBetween(loadUpdateTrace, L"UpdateLoad@", L"\\s");

    if (!Guid::TryParse(partitionGuidString, partitionGuid))
    {
        Assert::CodingError("Can't convert GUID string {0} to GUID while parsing update load", partitionGuidString.c_str());
    }

    auto it = guidToServiceName_.find(partitionGuid);
    if (it == guidToServiceName_.end())
    {
        Assert::CodingError("Can't map GUID {0} to service name while parsing update load", partitionGuid.ToString());
    }

    serviceName = it->second;

    wstring replicaRoleString = RegexUtility::ExtractStringInBetween(loadUpdateTrace, L"Updating load for role ", L" with");

    if (!replicaRoleString.compare(L"Primary"))
    {
        replicaRole = ReplicaRole::Enum::Primary;
    }
    else if (!replicaRoleString.compare(L"Secondary"))
    {
        replicaRole = ReplicaRole::Enum::Secondary;
    }
    else
    {
        Assert::CodingError("Can't convert ReplicaRole string {0} while parsing update load", replicaRoleString.c_str());
    }

    wstring metricString = RegexUtility::ExtractStringInBetween(loadUpdateTrace, L" with ", L"$");

    metricName = metricString.substr(0, metricString.find_last_of(L":"));
    load = RegexUtility::ExtractValueByKey<uint>(loadUpdateTrace, metricName);

    vector<LoadMetric> entries;
    entries.push_back(LoadMetric(wstring(metricName), load));
    LoadOrMoveCostDescription loadDescription(partitionGuid, move(serviceName), true);
    StopwatchTime now = Stopwatch::Now();

    if (replicaRole == ReplicaRole::Primary)
    {
        loadDescription.MergeLoads(ReplicaRole::Primary, move(entries), now);
    }
    else
    {
        loadDescription.MergeLoads(ReplicaRole::Secondary, move(entries), now, false);
    }

    return loadDescription;
}

map<wstring, uint> TraceParser::CreateCapacities(wstring capacityStr)
{
    map<wstring, uint> capacities;
    if (!capacityStr.empty())
    {
        StringCollection capacityCollection;
        StringUtility::Split<wstring>(capacityStr, capacityCollection, L" ");
        for (int i = 0; i < capacityCollection.size(); ++i)
        {
            StringCollection capacityPair;
            StringUtility::Split<wstring>(capacityCollection[i], capacityPair, L":");

            wstring metricName;
            for (int j = 0; j < capacityPair.size() - 1; j++)
            {
                if (j > 0)
                {
                    metricName += L":";
                }

                metricName += capacityPair[j];
            }
            capacities.insert(make_pair(metricName, _wtoi(capacityPair[capacityPair.size() - 1].c_str())));
        }
    }

    return capacities;
}

vector<ServiceMetric> TraceParser::CreateMetrics(wstring metrics)
{
    StringCollection metricCollection;
    StringUtility::Split<wstring>(metrics, metricCollection, L",");

    vector<ServiceMetric> metricList;
    for (wstring const& metricStr : metricCollection)
    {
        StringCollection metricProperties;
        StringUtility::Split<wstring>(metricStr, metricProperties, L"/");

        if (metricProperties.size() != 4)
        {
            Assert::CodingError("Can't convert metrics string");
        }

        metricList.push_back(ServiceMetric(move(metricProperties[0]), Common::Double_Parse(metricProperties[1]),
            _wtoi(metricProperties[2].c_str()), _wtoi(metricProperties[3].c_str())));
    }

    return metricList;
}

bool TraceParser::CheckReplicaFlags(std::wstring const& flags)
{
    for (int i = 0; i < flags.size(); i++)
    {
        if (ReplicaFlagsString.find(flags[i]) == wstring::npos)
        {
            return false;
        }
    }

    return true;
}

bool TraceParser::CheckFailoverUnitFlags(std::wstring const& flags)
{
    for (int i = 0; i < flags.size(); i++)
    {
        if (FailoverUnitFlagString.find(flags[i]) == wstring::npos)
        {
            return false;
        }
    }

    return true;
}
