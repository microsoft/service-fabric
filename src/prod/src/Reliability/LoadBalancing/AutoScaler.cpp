// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "AutoScaler.h"
#include "PlacementAndLoadBalancing.h"
#include "ServiceDomain.h"
#include "PLBConfig.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

AutoScaler::AutoScaler()
{

}

AutoScaler::~AutoScaler()
{

}

void AutoScaler::Refresh(Common::StopwatchTime timestamp, ServiceDomain & serviceDomain, size_t upNodeCount)
{
    RefreshFTs(timestamp, serviceDomain, upNodeCount);
    RefreshServices(timestamp, serviceDomain);
}

void AutoScaler::RefreshFTs(StopwatchTime timestamp, ServiceDomain & serviceDomain, size_t upNodeCount)
{
    while (!pendingAutoScaleSet_.empty())
    {
        auto itNext = pendingAutoScaleSet_.begin();

        if (itNext->expiry > timestamp)
            break;

        auto itFT = serviceDomain.failoverUnitTable_.find(itNext->FUid);
        if (itFT == serviceDomain.failoverUnitTable_.end())
        {
            ongoingTargetUpdates_.erase(itNext->FUid);
            pendingAutoScaleSet_.erase(itNext);
            continue;
        }
        FailoverUnit & failoverUnit = itFT->second;
        auto serviceIt = serviceDomain.Services.find(failoverUnit.FuDescription.ServiceId);
        if (serviceIt == serviceDomain.Services.end())
        {
            ongoingTargetUpdates_.erase(itNext->FUid);
            pendingAutoScaleSet_.erase(itNext);
            continue;
        }
        Service const& service = serviceIt->second;

        // If AS is removed, or if it is changed not to scale by changing instance count:
        if (!service.ServiceDesc.IsAutoScalingDefined || !service.ServiceDesc.AutoScalingPolicy.IsPartitionScaled())
        {
            ongoingTargetUpdates_.erase(itNext->FUid);
            pendingAutoScaleSet_.erase(itNext);
            continue;
        }
        if (service.ServiceDesc.AutoScalingPolicy.Trigger->Kind == ScalingTriggerKind::AveragePartitionLoad)
        {
            if (service.ServiceDesc.AutoScalingPolicy.Mechanism->Kind != ScalingMechanismKind::PartitionInstanceCount)
            {
                TRACE_AND_TESTASSERT(
                    serviceDomain.plb_.WriteWarning,
                    "AutoScaler",
                    service.ServiceDesc.Name,
                    "Scaling mechanism {0} is used with AverageServiceLoad scaling trigger.",
                    service.ServiceDesc.AutoScalingPolicy.Mechanism->Kind);
                ongoingTargetUpdates_.erase(itNext->FUid);
                pendingAutoScaleSet_.erase(itNext);
                continue;
            }
            AveragePartitionLoadScalingTriggerSPtr trigger = static_pointer_cast<AveragePartitionLoadScalingTrigger>(service.ServiceDesc.AutoScalingPolicy.Trigger);
            InstanceCountScalingMechanismSPtr mechanism = static_pointer_cast<InstanceCountScalingMechanism>(service.ServiceDesc.AutoScalingPolicy.Mechanism);

            // Next autoScale check should not be scheduled immediately (if ScaleInterval is not set, or set to zero)
            auto nextExpiry = timestamp + max(TimeSpan::FromSeconds(trigger->ScaleIntervalInSeconds), TimeSpan::FromSeconds(1));

            auto itOngoingUpdates = ongoingTargetUpdates_.find(itNext->FUid);
            if (itOngoingUpdates != ongoingTargetUpdates_.end())
            {
                serviceDomain.plb_.Trace.AutoScaler(wformatString(
                    "Skipping partition {0} because it has pending update.",
                    itNext->FUid));
                failoverUnit.NextScalingCheck = nextExpiry;
                //remove this entry and add a new one
                pendingAutoScaleSet_.erase(itNext);
                pendingAutoScaleSet_.insert(FailoverUnitAndExpiry(failoverUnit.FUId, nextExpiry));
                continue;
            }

            if (failoverUnit.IsLoadAvailable(trigger->MetricName))
            {
                // Perform auto scaling only if there are load reports.
                wstring const & metricName = trigger->MetricName;
                uint metricLoad;
                if (metricName == *ServiceModel::Constants::SystemMetricNameCpuCores || metricName == *ServiceModel::Constants::SystemMetricNameMemoryInMB)
                {
                    metricLoad = failoverUnit.GetResourceAverageLoad(metricName);
                }
                else
                {
                    int metricIndex = service.GetCustomLoadIndex(metricName);
                    if (metricIndex == -1)
                    {
                        TRACE_AND_TESTASSERT(
                            serviceDomain.plb_.WriteWarning,
                            "AutoScaler",
                            service.ServiceDesc.Name,
                            "Metric {0} does not exist when calculating average load for service {1}.",
                            metricName,
                            service.ServiceDesc.Name);
                        ongoingTargetUpdates_.erase(itNext->FUid);
                        pendingAutoScaleSet_.erase(itNext);
                        continue;
                    }
                    metricLoad = failoverUnit.GetSecondaryLoadForAutoScaling(metricIndex);
                }
                int newTargetInstanceCount = failoverUnit.TargetReplicaSetSize;

                auto highLimit = trigger->UpperLoadThreshold;
                auto lowLimit = trigger->LowerLoadThreshold;

                //adjust for cpu
                if (metricName == *ServiceModel::Constants::SystemMetricNameCpuCores)
                {
                    highLimit *= ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
                    lowLimit *= ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
                }

                if (metricLoad > highLimit)
                {                    
                    if (failoverUnit.FuDescription.ReplicaDifference > 0)
                    {
                        serviceDomain.plb_.Trace.AutoScaler(wformatString(
                            "Skipping partition {0} because it has replica diff {1}",
                            itNext->FUid,
                            failoverUnit.FuDescription.ReplicaDifference));
                        failoverUnit.NextScalingCheck = nextExpiry;
                        //remove this entry and add a new one
                        pendingAutoScaleSet_.erase(itNext);
                        pendingAutoScaleSet_.insert(FailoverUnitAndExpiry(failoverUnit.FUId, nextExpiry));
                        continue;
                    }
                    
                    auto maxInstanceCount = mechanism->MaximumInstanceCount;
                    //special case if the maximum is unlimited
                    if (maxInstanceCount == -1)
                    {
                        //-1 should scaleup to all valid nodes, but shouldn't set target below minimum instance count
                        maxInstanceCount = max((int)upNodeCount - (int)service.BlockList.Count, mechanism->MinimumInstanceCount);
                    }
                    newTargetInstanceCount = min(maxInstanceCount, failoverUnit.TargetReplicaSetSize + mechanism->ScaleIncrement);
                }
                if (metricLoad < lowLimit)
                {                    
                    if (failoverUnit.FuDescription.ReplicaDifference < 0)
                    {
                        serviceDomain.plb_.Trace.AutoScaler(wformatString(
                            "Skipping partition {0} because it has replica diff {1}",
                            itNext->FUid,
                            failoverUnit.FuDescription.ReplicaDifference));
                        failoverUnit.NextScalingCheck = nextExpiry;
                        //remove this entry and add a new one
                        pendingAutoScaleSet_.erase(itNext);
                        pendingAutoScaleSet_.insert(FailoverUnitAndExpiry(failoverUnit.FUId, nextExpiry));
                        continue;
                    }
                    
                    newTargetInstanceCount = max(mechanism->MinimumInstanceCount, failoverUnit.TargetReplicaSetSize - mechanism->ScaleIncrement);
                }
                if (newTargetInstanceCount != failoverUnit.TargetReplicaSetSize)
                {
                    ongoingTargetUpdates_[itNext->FUid] = newTargetInstanceCount;
                }

                serviceDomain.plb_.Trace.AutoScaler(wformatString(
                    "For partition {0} old target {1} new target {2} metricLoad {3}.",
                    itNext->FUid, failoverUnit.TargetReplicaSetSize, newTargetInstanceCount, metricLoad));
            }
            else
            {
                serviceDomain.plb_.Trace.AutoScaler(wformatString(
                    "Skipping partition {0} because it has no load updates. Reconsidering at {2}.",
                    itNext->FUid));
            }
            //keep this updated
            failoverUnit.NextScalingCheck = nextExpiry;
            //remove this entry and add a new one
            pendingAutoScaleSet_.erase(itNext);
            pendingAutoScaleSet_.insert(FailoverUnitAndExpiry(failoverUnit.FUId, nextExpiry));
        }
    }

    //go over all of the ongoing updates that should be made and send them out to FM
    for (auto & ongoing : ongoingTargetUpdates_)
    {
        serviceDomain.plb_.TriggerUpdateTargetReplicaCount(ongoing.first, ongoing.second);
    }
}

void AutoScaler::ProcessFailoverUnitTargetUpdate(Common::Guid const & failoverUnitId, int targetReplicaSetSize)
{
    auto itUpdate = ongoingTargetUpdates_.find(failoverUnitId);
    if (itUpdate != ongoingTargetUpdates_.end())
    {
        //if we had another update that changed target yet again we should keep sending it
        //else just remove it as it has been acked by FM
        if (itUpdate->second == targetReplicaSetSize)
        {
            ongoingTargetUpdates_.erase(itUpdate);
        }
    }
}

void AutoScaler::AddService(uint64_t serviceId, Common::StopwatchTime expiry)
{
    if (expiry != StopwatchTime::Zero)
    {
        pendingAutoScaleServices_.insert(ServiceAndExpiry(serviceId, expiry));
    }
}

void AutoScaler::RemoveService(uint64_t serviceId, Common::StopwatchTime expiry)
{
    pendingAutoScaleServices_.erase(ServiceAndExpiry(serviceId, expiry));
    ongoingServiceUpdates_.erase(serviceId);
}

void AutoScaler::UpdateServicePartitionCount(uint64_t serviceId, int change)
{
    // Only if partition count has changed, remove the service from pending.
    if (change != 0)
    {
        ongoingServiceUpdates_.erase(serviceId);
    }
}

void AutoScaler::UpdateServiceAutoScalingOperationFailed(uint64_t serviceId)
{
    ongoingServiceUpdates_.erase(serviceId);
}

void AutoScaler::RefreshServices(Common::StopwatchTime timestamp, ServiceDomain & serviceDomain)
{
    while (!pendingAutoScaleServices_.empty())
    {
        auto itNext = pendingAutoScaleServices_.begin();

        if (itNext->Expiry > timestamp)
            break;

        auto itService = serviceDomain.Services.find(itNext->ServiceId);
        if (itService == serviceDomain.Services.end())
        {
            // Service got deleted behind our back - should have been deleted from here as well.
            Common::Assert::TestAssert(
                "AutoScaler: Service {0} not present in ServiceDomain",
                itNext->ServiceId);
            ongoingServiceUpdates_.erase(itNext->ServiceId);
            pendingAutoScaleServices_.erase(itNext);
            continue;
        }
        Service & service = itService->second;

        // If auto scaling is not defined, or if it is not scaled by changing partition count.
        if (!service.ServiceDesc.IsAutoScalingDefined || !service.ServiceDesc.AutoScalingPolicy.IsServiceScaled())
        {
            ongoingServiceUpdates_.erase(itNext->ServiceId);
            pendingAutoScaleServices_.erase(itNext);
            continue;
        }

        if (service.ServiceDesc.AutoScalingPolicy.Trigger->Kind == ScalingTriggerKind::AverageServiceLoad)
        {
            if (service.ServiceDesc.AutoScalingPolicy.Mechanism->Kind != ScalingMechanismKind::AddRemoveIncrementalNamedPartition)
            {
                TRACE_AND_TESTASSERT(
                    serviceDomain.plb_.WriteWarning,
                    "AutoScaler",
                    service.ServiceDesc.Name,
                    "Scaling mechanism {0} is used with AverageServiceLoad scaling trigger.",
                    service.ServiceDesc.AutoScalingPolicy.Mechanism->Kind);
                ongoingServiceUpdates_.erase(itNext->ServiceId);
                pendingAutoScaleServices_.erase(itNext);
                continue;
            }
            AverageServiceLoadScalingTriggerSPtr trigger = static_pointer_cast<AverageServiceLoadScalingTrigger>(service.ServiceDesc.AutoScalingPolicy.Trigger);
            AddRemoveIncrementalNamedPartitionScalingMechanismSPtr mechanism = static_pointer_cast<AddRemoveIncrementalNamedPartitionScalingMechanism>(service.ServiceDesc.AutoScalingPolicy.Mechanism);

            // Next autoScale check should not be scheduled immediately (if ScaleInterval is not set, or set to zero)
            auto nextExpiry = timestamp + max(service.ServiceDesc.AutoScalingPolicy.GetScalingInterval(), TimeSpan::FromSeconds(1));

            auto itOngoingUpdates = ongoingServiceUpdates_.find(itNext->ServiceId);
            if (itOngoingUpdates != ongoingServiceUpdates_.end())
            {
                // FM still did not process this service. Wait for it.
                serviceDomain.plb_.Trace.AutoScaler(wformatString(
                    "Skipping service {0}({1}) because it has pending update. Reconsidering at {2}.",
                    service.ServiceDesc.Name,
                    itNext->ServiceId,
                    nextExpiry));
                service.NextAutoScaleCheck = nextExpiry;
                pendingAutoScaleServices_.erase(itNext);
                pendingAutoScaleServices_.insert(ServiceAndExpiry(service.ServiceDesc.ServiceId, nextExpiry));
                continue;
            }

            wstring const & metricName = trigger->MetricName;

            if (!service.IsLoadAvailableForPartitions(metricName, serviceDomain))
            {
                serviceDomain.plb_.Trace.AutoScaler(wformatString(
                    "Skipping auto scaling od service {0} bacause load for metric {1} is not available.",
                    service.ServiceDesc.Name,
                    metricName));
                service.NextAutoScaleCheck = nextExpiry;
                pendingAutoScaleServices_.erase(itNext);
                pendingAutoScaleServices_.insert(ServiceAndExpiry(service.ServiceDesc.ServiceId, nextExpiry));
                continue;
            }

            double averageMetricLoad;
            if (!service.GetAverageLoadPerPartitions(metricName, serviceDomain, trigger->UseOnlyPrimaryLoad, averageMetricLoad))
            {
                TRACE_AND_TESTASSERT(
                    serviceDomain.plb_.WriteWarning,
                    "AutoScaler",
                    service.ServiceDesc.Name,
                    "Metric {0} does not exist when calculating average load for service {1}.",
                    metricName,
                    service.ServiceDesc.Name);
                service.NextAutoScaleCheck = nextExpiry;
                pendingAutoScaleServices_.erase(itNext);
                pendingAutoScaleServices_.insert(ServiceAndExpiry(service.ServiceDesc.ServiceId, nextExpiry));
                continue;
            }

            auto highLimit = trigger->UpperLoadThreshold;
            auto lowLimit = trigger->LowerLoadThreshold;
            //adjust for cpu
            if (metricName == *ServiceModel::Constants::SystemMetricNameCpuCores)
            {
                highLimit *= ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
                lowLimit *= ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
            }

            int change = 0;
            int currentPartitionCount = service.ServiceDesc.PartitionCount;
            int newPartitionCount = currentPartitionCount;

            if (averageMetricLoad > highLimit)
            {
                // Scale the service up
                newPartitionCount = min(
                    currentPartitionCount + mechanism->ScaleIncrement,
                    mechanism->MaximumPartitionCount);
            }
            else if (averageMetricLoad < lowLimit)
            {
                // Scale the service down
                newPartitionCount = max(
                    currentPartitionCount - mechanism->ScaleIncrement,
                    mechanism->MinimumPartitionCount);
            }

            change = newPartitionCount - currentPartitionCount;
            if (change != 0)
            {
                // Put in ongoing set, notify PLB so that it can scale without lock
                ongoingServiceUpdates_.insert(make_pair(itNext->ServiceId, change));
                serviceDomain.plb_.PendingAutoScalingRepartitions.push_back(
                    AutoScalingRepartitionInfo(
                        service.ServiceDesc.ServiceId,
                        service.ServiceDesc.Name,
                        service.ServiceDesc.IsStateful,
                        service.ServiceDesc.PartitionCount,
                        change));
            }

            serviceDomain.plb_.Trace.AutoScaler(wformatString(
                "AutoScaling service {0}: currentPartitionCount={1} averageLoad={2} change={3}",
                service.ServiceDesc.Name,
                currentPartitionCount,
                averageMetricLoad,
                change));

            service.NextAutoScaleCheck = nextExpiry;
            //remove this entry and add a new one
            pendingAutoScaleServices_.erase(itNext);
            pendingAutoScaleServices_.insert(ServiceAndExpiry(service.ServiceDesc.ServiceId, nextExpiry));
        }
    }
}

void AutoScaler::AddFailoverUnit(Guid failoverUnitId, StopwatchTime expiry)
{
    //if there is a scaling policy defined
    if (expiry != StopwatchTime::Zero)
    {
        pendingAutoScaleSet_.insert(FailoverUnitAndExpiry(failoverUnitId, expiry));
    }
}
void AutoScaler::RemoveFailoverUnit(Guid failoverUnitId, StopwatchTime expiry)
{
    pendingAutoScaleSet_.erase(FailoverUnitAndExpiry(failoverUnitId, expiry));
    ongoingTargetUpdates_.erase(failoverUnitId);
}

void AutoScaler::MergeAutoScaler(AutoScaler && other)
{
    //we can be sure at this point that we do not have any overlapping FTs/services in these two domains
    pendingAutoScaleSet_.insert(other.pendingAutoScaleSet_.begin(), other.pendingAutoScaleSet_.end());

    ongoingTargetUpdates_.insert(std::make_move_iterator(other.ongoingTargetUpdates_.begin()),
        std::make_move_iterator(other.ongoingTargetUpdates_.end()));

    pendingAutoScaleServices_.insert(other.pendingAutoScaleServices_.begin(), other.pendingAutoScaleServices_.end());

    ongoingServiceUpdates_.insert(std::make_move_iterator(other.ongoingServiceUpdates_.begin()),
        std::make_move_iterator(other.ongoingServiceUpdates_.end()));
}
