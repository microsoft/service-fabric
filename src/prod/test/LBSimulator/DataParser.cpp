// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FM.h"
#include "DataParser.h"
#include "Utility.h"
#include "LBSimulatorConfig.h"

using namespace std;
using namespace Common;
using namespace LBSimulator;

wstring const DataParser::SectionNodeTypes = L"NodeTypes";
wstring const DataParser::SectionNode = L"Node";
wstring const DataParser::SectionService = L"Service";
wstring const DataParser::SectionServiceInsert = L"SrvcInsert";
wstring const DataParser::SectionFailoverUnit = L"FailoverUnit";
wstring const DataParser::SectionPlacement = L"Placement";
wstring const DataParser::VectorStartDelimiter = L"{";
wstring const DataParser::VectorDelimiter = L"}";

wstring const DataParser::nodeTypeLabelArray[] = { L"NODETYPE", L"PROPERTIES" };
vector<wstring> const DataParser::nodeTypeLabels = vector<wstring>({nodeTypeLabelArray, nodeTypeLabelArray + 2});
wstring const DataParser::nodeLabelArray[] = { L"INFO", L"NODENAME", L"INSTANCES",
    L"NODETYPE", L"NODESTATUS", L"CAPACITYRATIOS", L"METRICCAPACITY", L"FAULTDOMAIN", L"UPGRADEDOMAIN"};
vector<wstring> const DataParser::nodeLabels = vector<wstring>({nodeLabelArray, nodeLabelArray + 9});
wstring const DataParser::serviceLabelArray[] = { L"INFO", L"URI", L"AFF", L"DESC", L"BLOCKLIST", L"CONSTRAINTS", L"METRICS"};
vector<wstring> const DataParser::serviceLabels = vector<wstring>({ serviceLabelArray, serviceLabelArray + 7 });
wstring const DataParser::failoverUnitLabelArray[] = { L"INFO", L"PARENT", L"TYPE", L"METRICS", L"SERVICE", L"GUID", L"INDEX"};
vector<wstring> const DataParser::failoverUnitLabels = vector<wstring>({ failoverUnitLabelArray, failoverUnitLabelArray + 7 });
wstring const DataParser::placementLabelArray[] = { L"NODE", L"REPLICAS"};
vector<wstring> const DataParser::placementLabels = vector<wstring>({ placementLabelArray, placementLabelArray + 2 });

DataParser::DataParser(FM & fm, Reliability::LoadBalancingComponent::PLBConfig & plbConfig)
    : fm_(fm), config_(plbConfig)
{
    stateMachine_.clear();
    currentState_ = L"START";

    //start transitions
    stateMachine_.insert(std::make_pair(L"START", SectionNodeTypes));
    stateMachine_.insert(std::make_pair(L"START", SectionNode));
    stateMachine_.insert(std::make_pair(L"START", Utility::SectionConfig));
    //self transitions
    stateMachine_.insert(std::make_pair(SectionNodeTypes, SectionNodeTypes));
    stateMachine_.insert(std::make_pair(SectionNode, SectionNode));
    stateMachine_.insert(std::make_pair(Utility::SectionConfig, Utility::SectionConfig));
    stateMachine_.insert(std::make_pair(SectionService, SectionService));
    stateMachine_.insert(std::make_pair(SectionFailoverUnit, SectionFailoverUnit));
    stateMachine_.insert(std::make_pair(SectionPlacement, SectionPlacement));
    //forward transitions
    stateMachine_.insert(std::make_pair(Utility::SectionConfig, SectionNodeTypes));
    stateMachine_.insert(std::make_pair(Utility::SectionConfig, SectionNode));
    stateMachine_.insert(std::make_pair(SectionNodeTypes, SectionNode));
    stateMachine_.insert(std::make_pair(SectionNode, SectionService));
    stateMachine_.insert(std::make_pair(SectionNode, SectionServiceInsert));
    stateMachine_.insert(std::make_pair(SectionService, SectionFailoverUnit));
    stateMachine_.insert(std::make_pair(SectionFailoverUnit, SectionPlacement));
    stateMachine_.insert(std::make_pair(SectionPlacement, SectionServiceInsert));

    //end transitions
    stateMachine_.insert(std::make_pair(SectionPlacement, L"END"));
    stateMachine_.insert(std::make_pair(SectionServiceInsert, L"END"));
}

bool DataParser::ValidateTransition(wstring state)
{
    if (stateMachine_.count(make_pair(currentState_, state)) != 0)
    {
        currentState_ = state;
        return true;
    }

    return false;
}

void DataParser::Parse(wstring const& fileName)
{
    wstring fileTextW;
    Utility::LoadFile(fileName, fileTextW);
    currentState_ = L"START";
    size_t pos = 0;
    size_t oldPos;
    while (pos != std::wstring::npos)
    {
        oldPos = pos;
        wstring lineBuf = Utility::ReadLine(fileTextW, pos);
        if (oldPos == pos)
        {
            break;
        }

        if (StringUtility::StartsWith(lineBuf, Utility::CommentInit))
        {
            continue;
        }

        StringUtility::TrimWhitespaces(lineBuf);
        if (lineBuf.empty())
        {
            continue;
        }

        if (StringUtility::StartsWith(lineBuf, Utility::SectionStart))
        {
            ParseSections(lineBuf, fileTextW, pos);
        }
    }
}

void DataParser::ParseNodeTypesSection(wstring const & fileTextW, size_t & pos)
{
    size_t oldPos;
    while (pos != std::wstring::npos)
    {
        oldPos = pos;
        wstring lineBuf = Utility::ReadLine(fileTextW, pos);
        if (oldPos == pos || StringUtility::Contains(lineBuf, Utility::SectionEnd))
        {
            return;
        }

        //list of properties we are trying to capture....
        wstring nodeTypeStr;
        map<wstring, wstring> currentNodeTypeProperties;
        vector<pair<wstring, wstring>> nodeTypeVec;
        Utility::Tokenize(lineBuf, nodeTypeVec);
        map<wstring, wstring> nodeTypeLabelledVec;
        Utility::Label(nodeTypeVec, nodeTypeLabelledVec, LBSimulator::DataParser::nodeTypeLabels);
        StringCollection nodeProperties;
        StringUtility::Split<wstring>(nodeTypeLabelledVec[L"PROPERTIES"], nodeProperties, Utility::ItemDelimiter);
        for each(auto nodeProperty in nodeProperties)
        {
            StringCollection nodePropertyPair;
            StringUtility::Split<wstring>(nodeProperty, nodePropertyPair, Utility::KeyValueDelimiter);
            ASSERT_IF(nodePropertyPair.size() != 2, "node pair {0} has wrong number of items", nodeProperty);
            ASSERT_IF(nodePropertyPair[0] == Reliability::LoadBalancingComponent::Constants::ImplicitNodeName,
                "NodeName Cannot be defined in the type", nodeProperty);
            currentNodeTypeProperties.insert(std::make_pair(nodePropertyPair[0], nodePropertyPair[1]));
        }
        if (currentNodeTypeProperties.count(StringUtility::ToWString(
            Reliability::LoadBalancingComponent::Constants::ImplicitNodeType)) == 0)
        {
            currentNodeTypeProperties.insert(std::make_pair(
                StringUtility::ToWString(Reliability::LoadBalancingComponent::Constants::ImplicitNodeType),
                nodeTypeLabelledVec[L"NODETYPE"]));
        }
        fm_.AddNodeProperty(nodeTypeLabelledVec[L"NODETYPE"], currentNodeTypeProperties);
    }
}

void DataParser::ParseNodeSection(wstring const & fileTextW, size_t & pos)
{
    //TODO::Add more flexibilty to how structures are accepted.
    int index = 0;
    size_t oldPos;
    while (pos != std::wstring::npos)
    {
        oldPos = pos;
        wstring lineBuf = Utility::ReadLine(fileTextW, pos);
        if (oldPos == pos || StringUtility::Contains(lineBuf, Utility::SectionEnd))
        {
            return;
        }

        vector<pair<wstring, wstring>> nodeVec;
        Utility::Tokenize(lineBuf, nodeVec);
        map<wstring, wstring> nodeLabelledVec;
        Utility::Label(nodeVec, nodeLabelledVec, LBSimulator::DataParser::nodeLabels);
        CreateNode(nodeLabelledVec, index++);
    }
}

void DataParser::CreateNode(map<wstring,wstring> & nodeLabelledVec, size_t index)
{
    //list of properties we are trying to capture....
    Uri fdId;
    wstring udId;
    map<wstring, wstring> currentNodeProperties;
    map<wstring, uint> capacities;
    fm_.GetNodeProperties(nodeLabelledVec[L"NODETYPE"], currentNodeProperties);
    currentNodeProperties[StringUtility::ToWString(
        Reliability::LoadBalancingComponent::Constants::ImplicitNodeName)] = nodeLabelledVec[L"NODENAME"];
    bool isUp = StringUtility::ContainsCaseInsensitive(nodeLabelledVec[L"NODESTATUS"], StringUtility::ToWString(L"Up"));
    StringCollection capacityVec;
    StringUtility::Split<wstring>(nodeLabelledVec[L"METRICCAPACITY"], capacityVec, Utility::ItemDelimiter);
    for each (wstring const& capacityStr in capacityVec)
    {
        wstring tempCapacityStr = capacityStr;
        StringUtility::TrimWhitespaces(tempCapacityStr);
        if (tempCapacityStr == L"") { continue; }
        StringCollection nodeMetric;
        StringUtility::Split<wstring>(capacityStr, nodeMetric, Utility::PairDelimiter);
        ASSERT_IF(nodeMetric.size() > 3 || nodeMetric.size() < 2, "Capacity string {0} has wrong number of items", capacityStr);
        capacities.insert(make_pair(nodeMetric[0], static_cast<uint>(Int32_Parse(nodeMetric[1]))));
    }

    bool result = Uri::TryParse(nodeLabelledVec[L"FAULTDOMAIN"], fdId);
    ASSERT_IF(!result, "Invalid domain id {0}", nodeLabelledVec[L"FAULTDOMAIN"]);
    udId = nodeLabelledVec[L"UPGRADEDOMAIN"];
    //if (!isUp) { return; }
    fm_.CreateNode(Node(int(index), move(capacities), move(fdId), move(udId), move(currentNodeProperties), isUp));
}

void DataParser::ParseServiceSection(wstring const & fileTextW, size_t & pos, bool insertNew = false)
{
    int index = 0;
    size_t oldPos;
    while (pos != std::wstring::npos)
    {
        oldPos = pos;
        wstring lineBuf = Utility::ReadLine(fileTextW, pos);
        if (oldPos == pos || StringUtility::Contains(lineBuf, Utility::SectionEnd))
        {
            return;
        }

        vector<pair<wstring, wstring>> serviceVec;
        Utility::Tokenize(lineBuf, serviceVec);
        map<wstring, wstring> serviceLabelledVec;
        Utility::Label(serviceVec, serviceLabelledVec, LBSimulator::DataParser::serviceLabels);
        CreateService(serviceLabelledVec, index++, insertNew);
    }
}

void DataParser::CreateService(map<wstring, wstring> & serviceLabelledVec,size_t serviceIndex, bool generateFTAndReplicas)
{
    StringCollection descriptionVec;
    StringUtility::Split<wstring>(serviceLabelledVec[L"DESC"], descriptionVec, Utility::ItemDelimiter);
    ASSERT_IF(descriptionVec.size() != 4, "Service vector parameter list {0} has wrong number of items", descriptionVec);

    set<int> blockList;
    StringCollection blockListVec;
    StringUtility::Split<wstring>(serviceLabelledVec[L"BLOCKLIST"], blockListVec, Utility::ItemDelimiter);
    for each (wstring const& node in blockListVec)
    {
        blockList.insert(_wtoi(node.c_str()));
    }

    vector<Service::Metric> metrics;
    StringCollection metricVec;
    StringUtility::Split<wstring>(serviceLabelledVec[L"METRICS"], metricVec, Utility::ItemDelimiter);
    for each (wstring const& metricStr in metricVec)
    {
        StringCollection metricMemberVec;
        StringUtility::Split<wstring>(metricStr, metricMemberVec, Utility::PairDelimiter);
        ASSERT_IF(metricMemberVec.size() != 4, "Service metric vector {0} has wrong number of items", metricStr);
        wstring metricName = metricMemberVec[0];
        double metricWeight = 0.0;
        if (StringUtility::AreEqualCaseInsensitive(metricMemberVec[1].c_str(), L"Medium"))
        {
            metricWeight = Reliability::LoadBalancingComponent::Constants::MetricWeightMedium;
        }
        else if (StringUtility::AreEqualCaseInsensitive(metricMemberVec[1].c_str(), L"High"))
        {
            metricWeight = Reliability::LoadBalancingComponent::Constants::MetricWeightHigh;
        }
        else if (StringUtility::AreEqualCaseInsensitive(metricMemberVec[1].c_str(), L"Low"))
        {
            metricWeight = Reliability::LoadBalancingComponent::Constants::MetricWeightLow;
        }
        else if (StringUtility::AreEqualCaseInsensitive(metricMemberVec[1].c_str(), L"Zero"))
        {
            metricWeight = Reliability::LoadBalancingComponent::Constants::MetricWeightZero;
        }
        else
        {
            ASSERT(FALSE);
        }
        uint primaryDefaultLoad = _wtoi(metricMemberVec[2].c_str());
        uint secondaryDefaultLoad = _wtoi(metricMemberVec[3].c_str());
        metrics.push_back(Service::Metric(move(metricName), metricWeight, primaryDefaultLoad, secondaryDefaultLoad));
    }

    wstring serviceName = (serviceLabelledVec[L"URI"] != L"") ? serviceLabelledVec[L"URI"] : L"Service_" + StringUtility::ToWString(serviceIndex);
    bool isServiceStateful = false;

    ASSERT_IF((!StringUtility::ContainsCaseInsensitive(descriptionVec[0], StringUtility::ToWString(L"Stateful"))
        && !StringUtility::ContainsCaseInsensitive(descriptionVec[0], StringUtility::ToWString(L"Stateless"))),
        "Invalid Service Type")
    if (StringUtility::ContainsCaseInsensitive(descriptionVec[0], StringUtility::ToWString(L"Stateful")))
    {
        isServiceStateful = true;
    }
    else if (StringUtility::ContainsCaseInsensitive(descriptionVec[0],
        StringUtility::ToWString(L"Stateless"))) {
        isServiceStateful = false;
    }
    uint partitionCount;
    if (StringUtility::AreEqualCaseInsensitive(descriptionVec[1].c_str(), L"-")) {
        partitionCount = 1;
    }
    else if (StringUtility::ContainsCaseInsensitive(descriptionVec[1], StringUtility::ToWString(L"Singleton"))) {
        partitionCount = 1;
    }
    else {
        partitionCount = _wtoi(descriptionVec[1].c_str());
    }
    //ASSERT_IF(partitionCount == 0, "Empty Service Created");
    int minReplicaSetSize = (StringUtility::ContainsCaseInsensitive(descriptionVec[2], wstring(L"-")))
        ? 0 : _wtoi(descriptionVec[1].c_str());
    uint targetReplicaSetSize = (StringUtility::ContainsCaseInsensitive(descriptionVec[3], wstring(L"-")))
        ? 0 : _wtoi(descriptionVec[3].c_str());

    wstring affinitizedService;
    if (serviceLabelledVec[L"AFF"] == L"-1" )
    {
        affinitizedService = L"";
    }
    else
    {
        affinitizedService = serviceLabelledVec[L"AFF"];
    }

    wstring placementConstraints;
    if (serviceLabelledVec[L"CONSTRAINTS"] == L"" || serviceLabelledVec[L"CONSTRAINTS"] == L"-1")
    {
        placementConstraints = L"";
    }
    else
    {
        placementConstraints = serviceLabelledVec[L"CONSTRAINTS"];
    }

    if (!generateFTAndReplicas)
    {
        fm_.CreateService(Service(
            int(serviceIndex++),
            serviceName,
            isServiceStateful,
            partitionCount,
            targetReplicaSetSize,
            affinitizedService,
            move(blockList),
            move(metrics),
            LBSimulatorConfig::GetConfig().DefaultMoveCost,
            placementConstraints,
            minReplicaSetSize
            ), false);
    }
    else
    {
        //TODO:: enable this as well...
        fm_.CreateNewService(Service(
            int(serviceIndex++),
            serviceName,
            isServiceStateful,
            partitionCount,
            targetReplicaSetSize,
            affinitizedService,
            move(blockList),
            move(metrics),
            LBSimulatorConfig::GetConfig().DefaultMoveCost,
            placementConstraints,
            minReplicaSetSize
            ));
    }
}

void DataParser::ParseFailoverUnitSection(wstring const & fileTextW, size_t & pos)
{
    int failoverUnitIndex(0);
    //int tempTotalPartition(0);
    Service const*  service = nullptr;

    size_t oldPos;
    while (pos != std::wstring::npos)
    {
        oldPos = pos;
        wstring lineBuf = Utility::ReadLine(fileTextW, pos);
        if (oldPos == pos || StringUtility::Contains(lineBuf, Utility::SectionEnd))
        {
            return;
        }
        //list of properties we are trying to capture....
        map<wstring, uint> primary;
        map<wstring, uint> secondary;

        vector<pair<wstring, wstring>> failoverUnitVec;
        Utility::Tokenize(lineBuf, failoverUnitVec);
        map<wstring, wstring> failoverUnitLabelledVec;
        Utility::Label(failoverUnitVec, failoverUnitLabelledVec, LBSimulator::DataParser::failoverUnitLabels);

        int parentServiceIndex = _wtoi(failoverUnitLabelledVec[L"PARENT"].c_str());
        service = &const_cast<FM const&>(fm_).GetService(parentServiceIndex);
        if (service->StartFailoverUnitIndex == -1 || service->StartFailoverUnitIndex > failoverUnitIndex
            || (service->EndFailoverUnitIndex < failoverUnitIndex))
        {
            //this is a temp fix...
            fm_.SetServiceStartFailoverUnitIndex(parentServiceIndex, failoverUnitIndex);
        }
        StringCollection failoverUnitLoadVec;
        StringUtility::Split<wstring>(failoverUnitLabelledVec[L"METRICS"], failoverUnitLoadVec, Utility::ItemDelimiter);

        for each (wstring const& failoverUnitLoad in failoverUnitLoadVec)
        {
            StringCollection failoverUnitPair;
            StringUtility::Split<wstring>(failoverUnitLoad, failoverUnitPair, Utility::KeyValueDelimiter);
            ASSERT_IF(failoverUnitPair.size() != 2,
                "failoverUnit load input {0} has wrong number of values", failoverUnitLoad);

            StringCollection valuePair;
            StringUtility::Split<wstring>(failoverUnitPair[1], valuePair, Utility::PairDelimiter);
            ASSERT_IF(valuePair.size() != 2, "failoverUnit load input {0} has wrong number of values", failoverUnitLoad);

            if (static_cast<int>(Common::Int64_Parse(valuePair[0])) < 0 )
            {
                //primaryload gets a random value from 0 to x, where -x is the parameter
                int primaryRandomLoad = rand() % (-1*Common::Int64_Parse(valuePair[0]));
                int secondaryRandomLoad = 0;
                if (primaryRandomLoad > 0 && static_cast<int>(Common::Int64_Parse(valuePair[1])) < 0)
                {
                    //secondaryload is treated similarly, but must be less than primaryload
                    secondaryRandomLoad = ( rand() % (-1 * Common::Int64_Parse(valuePair[0])) ) % primaryRandomLoad;
                }
                primary.insert(make_pair(failoverUnitPair[0], primaryRandomLoad));
                secondary.insert(make_pair(failoverUnitPair[0], secondaryRandomLoad));
            }
            else
            {
                primary.insert(make_pair(failoverUnitPair[0], static_cast<uint>(Common::Int64_Parse(valuePair[0]))));
                secondary.insert(make_pair(failoverUnitPair[0], static_cast<uint>(Common::Int64_Parse(valuePair[1]))));
            }
        }

        int currentFUIndex = failoverUnitLabelledVec[L"INDEX"].size() != 0 ? Common::Int32_Parse(failoverUnitLabelledVec[L"INDEX"]) : failoverUnitIndex;
        Common::Guid failoverUnitId = failoverUnitLabelledVec[L"GUID"].size() == 36 ? Common::Guid(failoverUnitLabelledVec[L"GUID"]) : Common::Guid(currentFUIndex,0,0,0,0,0,0,0,0,0,0);
        failoverUnitIndexMap_.insert(make_pair(currentFUIndex, failoverUnitId));
        fm_.AddFailoverUnit(FailoverUnit(failoverUnitId, service->Index, service->ServiceName,
            service->TargetReplicaSetSize, service->IsStateful, move(primary), move(secondary)));

        failoverUnitIndex++;
    }
}

void DataParser::ParsePlacementSection(wstring const & fileTextW, size_t & pos)
{
    size_t oldPos;
    int tempNodeIndex(0);
    while (pos != std::wstring::npos)
    {
        oldPos = pos;
        wstring lineBuf = Utility::ReadLine(fileTextW, pos);
        if (oldPos == pos || StringUtility::Contains(lineBuf, Utility::SectionEnd))
        {
            return;
        }

        vector<pair<wstring, wstring>> placementVec;
        Utility::Tokenize(lineBuf, placementVec);
        map<wstring, wstring> placementLabelledVec;
        Utility::Label(placementVec, placementLabelledVec, LBSimulator::DataParser::placementLabels);
        StringCollection replicaVec;
        StringUtility::Split<wstring>(placementLabelledVec[L"REPLICAS"], replicaVec, Utility::ItemDelimiter);
        for each (wstring const& replicaStr in replicaVec)
        {
            int nodeIndex, role;

            StringCollection replicaEntry; //FailoverUnit:Role/Status
            StringUtility::Split<wstring>(replicaStr, replicaEntry, Utility::KeyValueDelimiter);
            StringUtility::TrimSpaces(replicaEntry[0]);
            int failoverUnitIndex = _wtoi(replicaEntry[0].c_str());
            if (failoverUnitIndex == 0 && !StringUtility::Contains(replicaEntry[0], StringUtility::ToWString(L"0")))
            {
                continue;
            }
            nodeIndex = tempNodeIndex;
            StringCollection replicaInfo;//Role/Status
            StringUtility::Split<wstring>(replicaEntry[1], replicaInfo, Utility::PairDelimiter);
            if (StringUtility::ContainsCaseInsensitive(replicaInfo[1], StringUtility::ToWString(L"Standby")))
            {
                role = LBSimulator::FailoverUnit::ReplicaRole::Standby;
            }
            else if (StringUtility::ContainsCaseInsensitive(replicaInfo[0], StringUtility::ToWString(L"Primary")))
            {
                role = LBSimulator::FailoverUnit::ReplicaRole::Primary;
            }
            else if (StringUtility::ContainsCaseInsensitive(replicaInfo[0], StringUtility::ToWString(L"ActiveSecondary")))
            {
                role = LBSimulator::FailoverUnit::ReplicaRole::Secondary;
            }
            else if (StringUtility::ContainsCaseInsensitive(replicaInfo[0], StringUtility::ToWString(L"Instance")))
            {
                //stateless instance is also imported as secondary...
                role = LBSimulator::FailoverUnit::ReplicaRole::Secondary;
            }
            else if (StringUtility::ContainsCaseInsensitive(replicaInfo[0], StringUtility::ToWString(L"IdleSecondary")))
            {
                //idlesecondary is also imported as secondary...
                role = LBSimulator::FailoverUnit::ReplicaRole::Secondary;
            }
            else if (StringUtility::ContainsCaseInsensitive(replicaInfo[0], StringUtility::ToWString(L"ToBeDropped")))
            {
                role = LBSimulator::FailoverUnit::ReplicaRole::ToBeDropped;
            }
            else
            {
                continue;
            }
            if (!StringUtility::ContainsCaseInsensitive(replicaEntry[1], StringUtility::ToWString(L"Down")))
            {

                fm_.AddReplica(failoverUnitIndexMap_[failoverUnitIndex], FailoverUnit::Replica(nodeIndex, 0, role));
            }
        }

        tempNodeIndex++;
    }
}

void DataParser::ParseSections(wstring const & lineBuf, wstring const & fileTextW, size_t & pos)
{
    fm_.ForcePLBUpdate();
    if (StringUtility::Contains(lineBuf, Utility::SectionConfig))
    {
        if (!ValidateTransition(Utility::SectionConfig)) { return; }
        Utility::ParseConfigSection(fileTextW, pos, config_);
    }
    else if (StringUtility::Contains(lineBuf, SectionNodeTypes))
    {
        if (!ValidateTransition(SectionNodeTypes)) { return; }
        ParseNodeTypesSection(fileTextW, pos);
    }
    else if (StringUtility::Contains(lineBuf, SectionNode))
    {
        if (!ValidateTransition(SectionNode)) { return; }
        ParseNodeSection(fileTextW, pos);
    }
    else if (StringUtility::Contains(lineBuf, SectionService))
    {
        if (!ValidateTransition(SectionService))  { return; }
        ParseServiceSection(fileTextW, pos);
    }
    else if (StringUtility::Contains(lineBuf, SectionServiceInsert))
    {
        if (!ValidateTransition(SectionServiceInsert)) { return; }
        ParseServiceSection(fileTextW, pos, true);
    }
    else if (StringUtility::Contains(lineBuf, SectionFailoverUnit))
    {
        if (!ValidateTransition(SectionFailoverUnit)) { return; }
        ParseFailoverUnitSection(fileTextW, pos);
    }
    else if (StringUtility::Contains(lineBuf, SectionPlacement))
    {
        if (!ValidateTransition(SectionPlacement)) { return; }
        ParsePlacementSection(fileTextW, pos);
    }
}
