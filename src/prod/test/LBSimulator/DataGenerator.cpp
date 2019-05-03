// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FM.h"
#include "DataGenerator.h"
#include "Utility.h"
#include "LBSimulatorConfig.h"

using namespace std;
using namespace Common;
using namespace LBSimulator;

wstring const DataGenerator::MetricCountStr = L"MetricCount";
wstring const DataGenerator::NodeCountStr = L"NodeCount";
wstring const DataGenerator::FaultDomainsStr = L"FaultDomains";
wstring const DataGenerator::PartitionCountStr = L"PartitionCount";
wstring const DataGenerator::ReplicaCountPerPartitionStr = L"ReplicaCountPerPartition";
wstring const DataGenerator::NodeCapacityRangeStr = L"NodeCapacityRange";
wstring const DataGenerator::PrimaryLoadRangeStr = L"PrimaryLoadRange";
wstring const DataGenerator::SecondaryLoadRangeStr = L"SecondaryLoadRange";
wstring const DataGenerator::AffinitizedServicePairStr = L"AffinitizedServicePair";

wstring DataGenerator::GetMetricName(int metricIndex)
{
    return wformatString("Metric{0:02}", metricIndex);
}

DataGenerator::DataGenerator(FM & fm, int seed, Reliability::LoadBalancingComponent::PLBConfig & plbConfig) :
    fm_(fm),
    random_(seed),
    MetricCount(0),
    NodeCount(0),
    config_(plbConfig),
    AffinitizedServicePair(0),
    ReplicaCountPerAffinitizedService(1)
{
}

void DataGenerator::ReadIntVec(wstring & line, vector<int> & vecInt)
{
    StringUtility::Trim<wstring>(line, L" {}");
    StringCollection vecStr;
    StringUtility::Split<wstring>(line, vecStr, Utility::ItemDelimiter);

    for each (wstring const& strItem in vecStr)
    {
        vecInt.push_back(_wtoi(strItem.c_str()));
    }
}

void DataGenerator::ReadLoadInputVec(std::wstring & line, std::vector<LoadDistribution> & vecLoad)
{
    StringUtility::Trim<wstring>(line, L" {}");
    StringCollection vecStr;
    StringUtility::Split<wstring>(line, vecStr, Utility::ItemDelimiter);

    for each (wstring const & strItem in vecStr)
    {
        vecLoad.push_back(LoadDistribution::Parse(strItem, Utility::PairDelimiter));
    }
}

void DataGenerator::Parse(wstring const & fileName)
{
    wstring fileTextW;
    Utility::LoadFile(fileName, fileTextW);

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
        if (StringUtility::Contains(lineBuf, Utility::SectionConfig))
        {
            Utility::ParseConfigSection(fileTextW, pos, config_);
            continue;
        }

        StringCollection params;
        StringUtility::Split<wstring>(lineBuf, params, Utility::KeyValueDelimiter);
        ASSERT_IFNOT(params.size() == 2, "\"{0}\" format not correct", lineBuf);

        StringUtility::TrimWhitespaces(params[0]);
        StringUtility::TrimWhitespaces(params[1]);

        if (params[0] == MetricCountStr)
        {
            MetricCount = _wtoi(params[1].c_str());
        }
        else if (params[0] == NodeCountStr)
        {
            NodeCount = _wtoi(params[1].c_str());
        }
        else if (params[0] == FaultDomainsStr)
        {
            StringUtility::Trim<wstring>(params[1], L"{}");
            StringCollection vecStr;
            StringUtility::Split<wstring>(params[1], vecStr, Utility::ItemDelimiter);

            vector<Uri> faultDomains;
            for each (wstring const& strItem in vecStr)
            {
                faultDomains.push_back(Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", strItem));
            }

            FaultDomains = move(faultDomains);
        }
        else if (params[0] == PartitionCountStr)
        {
            ReadIntVec(params[1], PartitionCount);
        }
        else if (params[0] == ReplicaCountPerPartitionStr)
        {
            ReadIntVec(params[1], ReplicaCountPerPartition);
        }
        else if (params[0] == NodeCapacityRangeStr)
        {
            ReadLoadInputVec(params[1], NodeCapacityRange);
        }
        else if (params[0] == PrimaryLoadRangeStr)
        {
            ReadLoadInputVec(params[1], PrimaryLoadRange);
        }
        else if (params[0] == SecondaryLoadRangeStr)
        {
            ReadLoadInputVec(params[1], SecondaryLoadRange);
        }
        else if (params[0] == AffinitizedServicePairStr)
        {
            // params[1]: affinitized service pair count, replica per service
            vector<int> parameters;
            ReadIntVec(params[1], parameters);
            AffinitizedServicePair = parameters[0];
            ReplicaCountPerAffinitizedService = parameters[1];
        }
        else
        {
            Assert::CodingError("Invalid key {0}", params[0]);
        }
    }

    ASSERT_IFNOT(PartitionCount.size() == ReplicaCountPerPartition.size(), "Error");

    size_t count = static_cast<size_t>(MetricCount);
    ASSERT_IFNOT(count == PrimaryLoadRange.size() && count == SecondaryLoadRange.size(), "Error");

    if (NodeCapacityRange.size() > 0)
    {
        ASSERT_IFNOT(count == NodeCapacityRange.size(), "Error");
    }
}

void DataGenerator::Generate()
{
    GenerateNodes();
    GenerateServices();
    GenerateFailoverUnits();
    GeneratePlacements();
}

void DataGenerator::GenerateNodes()
{
    int faultDomainSize = static_cast<int>(FaultDomains.size());
    for (int nodeIndex = 0; nodeIndex < NodeCount; nodeIndex++)
    {
        map<wstring, uint> capacities;
        for (int metricIndex = 0; metricIndex < MetricCount; metricIndex++)
        {
            if (NodeCapacityRange.size() > 0)
            {
                uint capacity = NodeCapacityRange[metricIndex].GetRandom(random_);
                ASSERT_IF(capacity <= 0, "Capacity should be > 0");
                capacities.insert(make_pair(GetMetricName(metricIndex), capacity));
            }
        }

        fm_.CreateNode(Node(nodeIndex, move(capacities), Uri(FaultDomains[random_.Next(faultDomainSize)]),
            wstring(), map<wstring, wstring>()));
    }
}

void DataGenerator::GenerateServices()
{
    for (size_t serviceIndex = 0; serviceIndex < PartitionCount.size(); serviceIndex++)
    {
        GenerateService(static_cast<int>(serviceIndex), PartitionCount[serviceIndex], ReplicaCountPerPartition[serviceIndex], L"");
    }

    int nextServiceIndex = static_cast<int>(PartitionCount.size());
    for (int i = 0; i < AffinitizedServicePair; i++)
    {
        GenerateService(nextServiceIndex, 1, ReplicaCountPerAffinitizedService, L"");
        GenerateService(nextServiceIndex + 1, 1, ReplicaCountPerAffinitizedService, L"Service_" + StringUtility::ToWString(nextServiceIndex));
        nextServiceIndex += 2;
    }
}

void DataGenerator::GenerateService(int serviceIndex, int partitionCount, int replicaCount, wstring affinitizedService)
{
    set<int> blockList;
    for (int i = 0; i < random_.Next(NodeCount + 1 - replicaCount); i ++) // number of nodes in block list: 0~MaxNode-replicaCount
    {
        blockList.insert(random_.Next(NodeCount)); // block node index: 0~MaxNode-1
    }

    vector<Service::Metric> metrics;
    for (int metricIndex = 0; metricIndex < MetricCount; metricIndex++)
    {
        metrics.push_back(Service::Metric(
            GetMetricName(metricIndex),
            1.0,
            0,
            0
            ));
    }

    fm_.CreateService(LBSimulator::Service(
        serviceIndex,
        L"Service_" + StringUtility::ToWString(serviceIndex),
        true,
        partitionCount,
        replicaCount,
        affinitizedService,
        move(blockList),
        move(metrics),
        LBSimulatorConfig::GetConfig().DefaultMoveCost,
        wstring()
        ), false);
}

void DataGenerator::GenerateFailoverUnits()
{
    int totalFailoverUnitIndex(0);
    for (int serviceIndex = 0; serviceIndex < static_cast<int>(PartitionCount.size()); serviceIndex++)
    {
        for (int failoverUnitIndex = 0; failoverUnitIndex < PartitionCount[serviceIndex]; failoverUnitIndex++)
        {
            GenerateFailoverUnit(totalFailoverUnitIndex, serviceIndex, ReplicaCountPerPartition[serviceIndex]);
            ++totalFailoverUnitIndex;
        }
    }

    int nextServiceIndex = static_cast<int>(PartitionCount.size());
    for (int i = 0; i < AffinitizedServicePair; i++)
    {
        GenerateFailoverUnit(totalFailoverUnitIndex++, nextServiceIndex++, ReplicaCountPerAffinitizedService);
        GenerateFailoverUnit(totalFailoverUnitIndex++, nextServiceIndex++, ReplicaCountPerAffinitizedService);
    }
}

void DataGenerator::GenerateFailoverUnit(int failoverUnitIndex, int serviceIndex, int replicaCount)
{
    map<wstring, uint> primary;
    map<wstring, uint> secondary;
    for (int metricIndex = 0; metricIndex < MetricCount; metricIndex++)
    {
        primary.insert(make_pair(GetMetricName(metricIndex), static_cast<uint>(PrimaryLoadRange[metricIndex].GetRandom(random_))));
        secondary.insert(make_pair(GetMetricName(metricIndex), static_cast<uint>(SecondaryLoadRange[metricIndex].GetRandom(random_))));
    }
    wstring serviceName = StringUtility::ToWString(L"Service_" + StringUtility::ToWString(serviceIndex));
    fm_.AddFailoverUnit(
        FailoverUnit(Common::Guid(failoverUnitIndex, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), serviceIndex, serviceName, replicaCount, true, move(primary), move(secondary))
        );
}

void DataGenerator::GeneratePlacements()
{
    int totalFailoverUnitIndex(0);
    for (int serviceIndex = 0; serviceIndex < static_cast<int>(PartitionCount.size()); serviceIndex++)
    {
        for (int partitionIndex = 0; partitionIndex < PartitionCount[serviceIndex]; partitionIndex++)
        {
            set<int> nodeIndexSet;
            for (int replicaIndex = 0; replicaIndex < ReplicaCountPerPartition[serviceIndex]; replicaIndex++)
            {
                int role = replicaIndex == 0 ? 0 : 1;

                int nodeIndex;
                do
                {
                    nodeIndex = random_.Next(NodeCount);
                }
                while (nodeIndexSet.find(nodeIndex) != nodeIndexSet.end());
                nodeIndexSet.insert(nodeIndex);
                fm_.AddReplica(
                    Common::Guid(totalFailoverUnitIndex, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), 
                    FailoverUnit::Replica(nodeIndex, 0, role, false, false, false, false, false)
                    );
            }
            totalFailoverUnitIndex++;
        }
    }
}

