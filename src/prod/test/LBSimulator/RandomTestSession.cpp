// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RandomTestSession.h"
#include "TestDispatcher.h"
#include "Service.h"
#include "Utility.h"

using namespace std;
using namespace Common;
using namespace LBSimulator;


RandomTestSession::RandomTestSession(int iterations, wstring const& label, bool autoMode, TestDispatcher& dispatcher)
    : random_(LBSimulatorConfigObj.RandomSeed),
    targetIterations_(iterations),
    iterations_(0),
    nodes_(LBSimulatorConfigObj.MaxNodes, false),
    services_(LBSimulatorConfigObj.MaxServices),
    TestSession(label, autoMode, dispatcher)
{
    faultDomains_.reserve(LBSimulatorConfigObj.Level1FaultDomainCount * LBSimulatorConfigObj.Level2FaultDomainCount);
    for (int i = 0; i < LBSimulatorConfigObj.Level1FaultDomainCount; i++)
    {
        for (int j = 0; j < LBSimulatorConfigObj.Level2FaultDomainCount; j++)
        {
            faultDomains_.push_back(Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", wformatString("{0}/{1}", i, j)));
        }
    }

    upgradeDomains_.reserve(LBSimulatorConfigObj.UpgradeDomainCount);
    for (int i = 0; i < LBSimulatorConfigObj.UpgradeDomainCount; i++)
    {
        upgradeDomains_.push_back(wformatString("ud{0}", i));
    }

    metrics_.reserve(LBSimulatorConfigObj.MetricCount);
    for (int i = 0; i < LBSimulatorConfigObj.MetricCount; i++)
    {
        // insert the scale of each metrics
        if (random_.Next(2) == 0)
        {
            // no limitation
            metrics_.push_back(-1);
        }
        else
        {
            metrics_.push_back(random_.Next(100));
        }
    }
}

wstring RandomTestSession::GetInput()
{
    if (iterations_ == 0)
    {
        // initial setup
        watch_.Start();
        initialized_ = true;
    }

    wstring text = wformatString("Iterations left: {0}, time elapsed: {1}", targetIterations_ - iterations_, TimeSpan::FromTicks(watch_.ElapsedTicks));
    TraceConsoleSink::Write(0x0b, text);

    if (iterations_ == 0)
    {
        // Create one node for each FD/UD combination
        GenerateInitialNodeDynamic();
    }
    else
    {
        for (int i = 0; i < random_.Next(1, LBSimulatorConfigObj.MaxDynamism + 1); i++)
        {
            ExecuteNodeDynamic();
        }
    }

    if (iterations_ == 0)
    {
        for (int i = 0; i < LBSimulatorConfigObj.InitialServices; i++)
        {
            ExecuteServiceDynamic();
        }
    }
    else if (iterations_ % LBSimulatorConfigObj.ServiceDynamicIterations == 0)
    {
        ExecuteServiceDynamic();
    }

    ExecuteLoadDynamic();

    AddInput(TestDispatcher::VerifyCommand);

    if (++iterations_ >= targetIterations_)
    {
        watch_.Stop();
        CloseAllNode();
        AddInput(TestSession::QuitCommand);
    }

    return wstring();
}

void RandomTestSession::ExecuteNodeDynamic()
{
    int randomIndex = random_.Next(static_cast<int>(nodes_.size()));

    if (!nodes_[randomIndex])
    {
        wstring faultDomainId;
        if (!faultDomains_.empty())
        {
            faultDomainId = faultDomains_[random_.Next(static_cast<int>(faultDomains_.size()))].ToString();
        }

        wstring upgradeDomainId;
        if (!upgradeDomains_.empty())
        {
            upgradeDomainId = upgradeDomains_[random_.Next(static_cast<int>(upgradeDomains_.size()))];
        }

        wstring capacities = GetNodeCapacities();

        AddInput(wformatString("{0}{1} {2} {3} {4}", 
            TestDispatcher::CreateNodeCommand, 
            randomIndex, 
            faultDomainId.empty() ? Utility::EmptyCharacter : faultDomainId, 
            upgradeDomainId.empty() ? Utility::EmptyCharacter : upgradeDomainId, 
            capacities.empty() ? Utility::EmptyCharacter : capacities));

        nodes_[randomIndex] = true;
    }
    else
    {
        AddInput(wformatString("{0}{1}", TestDispatcher::DeleteNodeCommand, randomIndex));
        nodes_[randomIndex] = false;
    }
}

void RandomTestSession::GenerateInitialNodeDynamic()
{
    int initialIndex = static_cast<int>(nodes_.size());

    ASSERT_IF(nodes_[initialIndex], "Error, node position is already taken during initial node dynamic, node index {0}", initialIndex);

    for (size_t fdIdx = 0; fdIdx < faultDomains_.size(); fdIdx++)
    {
        for (size_t udIdx = 0; udIdx < upgradeDomains_.size(); udIdx++)
        {
            wstring faultDomainId = faultDomains_[fdIdx].ToString();
            wstring upgradeDomainId = upgradeDomains_[udIdx];
            wstring capacities = GetNodeCapacities();

            AddInput(wformatString("{0}{1} {2} {3} {4}", 
                TestDispatcher::CreateNodeCommand, 
                initialIndex, 
                faultDomainId.empty() ? Utility::EmptyCharacter : faultDomainId, 
                upgradeDomainId.empty() ? Utility::EmptyCharacter : upgradeDomainId, 
                capacities.empty() ? Utility::EmptyCharacter : capacities));

            nodes_[initialIndex] = true;
            initialIndex++;
        }
    }
}

wstring RandomTestSession::GetNodeCapacities()
{
    wstring capacities;

    for (size_t i = 0; i < metrics_.size(); i++)
    {
        if (metrics_[i] >= 0)
        {
            if (!capacities.empty())
            {
                capacities.append(Utility::ItemDelimiter);
            }

            capacities.append(wformatString("Metric{0:02}", i));
            capacities.append(Utility::PairDelimiter);
            int upperBound = metrics_[i] * 3/*each service can have 3 metrics*/ * 3/*each partition can have 3 replicas*/ * 
                LBSimulatorConfigObj.MaxServices * LBSimulatorConfigObj.MaxPartitions / LBSimulatorConfigObj.MaxNodes / LBSimulatorConfigObj.MetricCount;
            // We don't want the capacity too small
            int lowerBound = upperBound / 2;
            capacities.append(wformatString("{0}", random_.Next(lowerBound, upperBound)));
        }
    }

    return capacities;
}

void RandomTestSession::ExecuteServiceDynamic()
{
    int randomIndex = random_.Next(static_cast<int>(services_.size()));

    if (randomIndex > 1 && static_cast<double>(randomIndex) / static_cast<double>(services_.size()) < LBSimulatorConfigObj.AffinitySimulationThreshold)
    {
        ExecuteServiceAffinityDynamic(randomIndex);
        return;
    }

    if (!services_[randomIndex].IsUp)
    {
        //check if any services is affinitized to this service
        bool affinitized = false;
        for (size_t i = 0; i < services_.size(); ++i)
        {
            if (services_[i].IsUp && services_[i].AffinitizedService == randomIndex)
            {
                affinitized = true;
            }
        }

        wstring blockListStr = GetRandomBlockList();

        set<int> metricList;
        for (int i = 0; i < random_.Next(4); i ++) // number of metrics: 0~3
        {
            metricList.insert(random_.Next(LBSimulatorConfigObj.MetricCount)); // metric index: 0~MetricCount-1
        }

        ASSERT_IFNOT(LBSimulatorConfigObj.MaxReplicas < LBSimulatorConfigObj.MaxNodes, "Error, max replica count should be less than max node count");

        int replicaCount = random_.Next(1, LBSimulatorConfigObj.MaxReplicas + 1);  // 1~MaxReplicas

        AddInput(wformatString("{0} {1} {2} {3} {4} {5} {6} {7}", 
            TestDispatcher::CreateServiceCommand, 
            randomIndex,
            random_.Next(2) == 0 ? "false" : "true", // isStateful
            affinitized ? 1 : random_.Next(1, LBSimulatorConfigObj.MaxPartitions + 1), // partitionCount: 1~MaxPartitions
            replicaCount,
            Utility::EmptyCharacter, //affinitizedService
            blockListStr.empty() ? Utility::EmptyCharacter : blockListStr,
            metricList.empty() ? Utility::EmptyCharacter : GetMetricList(metricList)
            ));

        services_[randomIndex] = ServiceData(true, -1, move(metricList));
    }
    else
    {
        AddInput(wformatString("{0} {1}", TestDispatcher::DeleteServiceCommand, randomIndex));
        services_[randomIndex] = ServiceData(false, -1, set<int>());
    }
}

void RandomTestSession::ExecuteServiceAffinityDynamic(int randomIndex)
{
    randomIndex;
    size_t numAffinitizedService = 2; //only deal with 2 services now //static_cast<size_t>(random_.Next(2, randomIndex));
    vector<int> affinitizedServicesIndexVec;

    size_t loopCount = 0;
    while (loopCount < services_.size() && affinitizedServicesIndexVec.size() < numAffinitizedService)
    {
        loopCount++;
        int serviceIndex = random_.Next(static_cast<int>(services_.size()));
        if (!services_[serviceIndex].IsUp && 
            find(affinitizedServicesIndexVec.begin(), affinitizedServicesIndexVec.end(), serviceIndex) == affinitizedServicesIndexVec.end())
        {
            if (!affinitizedServicesIndexVec.empty())
            {
                auto itChildService = find_if(services_.begin(), services_.end(), [&](ServiceData const& s)
                {
                    return s.AffinitizedService == serviceIndex;
                });

                if (itChildService != services_.end())
                {
                    // skip if it is already a parent service
                    continue;
                }
            }

            affinitizedServicesIndexVec.push_back(serviceIndex);
        }
    }

    wstring blockListStr = GetRandomBlockList();

    int affinitizedServiceIndex = -1;
    for (size_t i = 0; i < affinitizedServicesIndexVec.size(); i++)
    {
        if (i >= 1)
        {
            affinitizedServiceIndex = affinitizedServicesIndexVec[i-1];
        }

        int replicaCount = random_.Next(1, LBSimulatorConfigObj.MaxReplicas + 1);  // 1~MaxReplicas

        set<int> metricList;
        for (int m = 0; m < random_.Next(4); m ++) // number of metrics: 0~3
        {
            metricList.insert(random_.Next(LBSimulatorConfigObj.MetricCount)); // metric index: 0~MetricCount-1
        }

        AddInput(wformatString("{0} {1} {2} {3} {4} {5} {6} {7}", 
            TestDispatcher::CreateServiceCommand, 
            affinitizedServicesIndexVec[i],
            random_.Next(2) == 0 ? "false" : "true", // isStateful
            1, // partitionCount equal to 1 for affinity constraint
            replicaCount,
            affinitizedServiceIndex,
            blockListStr.empty() ? Utility::EmptyCharacter : blockListStr,
            metricList.empty() ? Utility::EmptyCharacter : GetMetricList(metricList)
            ));

        services_[affinitizedServicesIndexVec[i]] = ServiceData(true, affinitizedServiceIndex, move(metricList));
    }
}

void RandomTestSession::ExecuteLoadDynamic()
{
    for (size_t serviceIndex = 0; serviceIndex < services_.size(); ++serviceIndex)
    {
        if (!services_[serviceIndex].IsUp || random_.NextDouble() >= LBSimulatorConfigObj.ReportLoadProbability)
        {
            continue;
        }

        for each (int metricIndex in services_[serviceIndex].Metrics)
        {
            double scale = 1;
            if (metrics_[metricIndex] >= 0)
            {
                scale = metrics_[metricIndex];
            }

            if (random_.NextDouble() >= 0.5)
            {
                // uniform
                AddInput(wformatString("{0} {1} {2} U{3}{4}{5}{6}", 
                    TestDispatcher::ReportBatchLoadCommand,
                    serviceIndex,
                    wformatString("Metric{0:02}", metricIndex),
                    Utility::PairDelimiter,
                    0,
                    Utility::PairDelimiter,
                    scale));
            }
            else
            {
                // power law
                AddInput(wformatString("{0} {1} {2} P{3}{4}{5}{6}{7}{8}", 
                    TestDispatcher::ReportBatchLoadCommand,
                    serviceIndex,
                    wformatString("Metric{0:02}", metricIndex),
                    Utility::PairDelimiter,
                    5,
                    Utility::PairDelimiter,
                    scale / 1000.0,
                    Utility::PairDelimiter,
                    0));
            }
        }
    }
}

wstring RandomTestSession::GetRandomBlockList()
{
    set<int> blockList;
    for (int i = 0; i < random_.Next(LBSimulatorConfigObj.MaxNodes + 1); i ++) // number of nodes in block list: 0~MaxNode
    {
        blockList.insert(random_.Next(LBSimulatorConfigObj.MaxNodes)); // block node index: 0~MaxNode-1
    }

    wstring blockListStr;
    if (!blockList.empty())
    {
        for each (int index in blockList)
        {
            if (!blockListStr.empty())
            {
                blockListStr.append(Utility::ItemDelimiter);
            }

            blockListStr.append(wformatString("{0}", index));
        }
    }

    return blockListStr;
}

wstring RandomTestSession::GetMetricList(set<int> const& metricList)
{
    wstring metricListStr;
    for each (int index in metricList)
    {
        if (!metricListStr.empty())
        {
            metricListStr.append(Utility::ItemDelimiter);
        }

        double weight = random_.NextDouble();
        uint primaryDefaultLoad = random_.Next(metrics_[index]);
        uint secondaryDefaultLoad = random_.Next(metrics_[index]);
        metricListStr.append(wformatString("Metric{0:02}/{1}/{2}/{3}", index, weight, primaryDefaultLoad, secondaryDefaultLoad));
    }

    return metricListStr;
}

void RandomTestSession::CloseAllNode()
{
    for (size_t i = 0; i < nodes_.size(); i++)
    {
        if (nodes_[i])
        {
            AddInput(wformatString("{0}{1}", TestDispatcher::DeleteNodeCommand, i));
            nodes_[i] = false;
        }
    }
}
