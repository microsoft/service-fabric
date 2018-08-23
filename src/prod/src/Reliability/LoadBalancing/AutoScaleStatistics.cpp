// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServiceDescription.h"
#include "AutoScaleStatistics.h"
#include "Snapshot.h"
#include "PLBConfig.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;


std::string AutoScaleStatisticsEntry::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    string format = "PartitionCount={0}";
    size_t index = 0;

    traceEvent.AddEventField<int>(format, name + ".partitionCount", index);

    return format;
}

void AutoScaleStatisticsEntry::FillEventData(Common::TraceEventContext & context) const
{
    context.Write(partitionCount_);
}

void AutoScaleStatisticsEntry::WriteTo(Common::TextWriter & writer, Common::FormatOptions const &) const
{
    writer.WriteLine("PartitionCount={0}", partitionCount_);
}

void AutoScaleStatistics::AddService(ServiceDescription const& serviceDescription)
{
    if (serviceDescription.IsAutoScalingDefined)
    {
        if (serviceDescription.AutoScalingPolicy.Trigger->Kind == ScalingTriggerKind::AveragePartitionLoad)
        {
            AveragePartitionLoadScalingTriggerSPtr trigger = static_pointer_cast<AveragePartitionLoadScalingTrigger>(serviceDescription.AutoScalingPolicy.Trigger);

            if (trigger->MetricName == *ServiceModel::Constants::SystemMetricNameCpuCores)
            {
                cpuCoresStats_.PartitionCount += serviceDescription.PartitionCount;
            }
            else if (trigger->MetricName == *ServiceModel::Constants::SystemMetricNameMemoryInMB)
            {
                memoryInMBStats_.PartitionCount += serviceDescription.PartitionCount;
            }
            else
            {
                customMetricStats_.PartitionCount += serviceDescription.PartitionCount;
            }
        }
    }
}

void AutoScaleStatistics::DeleteService(ServiceDescription const& serviceDescription)
{
    if (serviceDescription.IsAutoScalingDefined)
    {
        if (serviceDescription.AutoScalingPolicy.Trigger->Kind == ScalingTriggerKind::AveragePartitionLoad)
        {
            AveragePartitionLoadScalingTriggerSPtr trigger = static_pointer_cast<AveragePartitionLoadScalingTrigger>(serviceDescription.AutoScalingPolicy.Trigger);

            if (trigger->MetricName == *ServiceModel::Constants::SystemMetricNameCpuCores)
            {
                cpuCoresStats_.PartitionCount -= serviceDescription.PartitionCount;
            }
            else if (trigger->MetricName == *ServiceModel::Constants::SystemMetricNameMemoryInMB)
            {
                memoryInMBStats_.PartitionCount -= serviceDescription.PartitionCount;
            }
            else
            {
                customMetricStats_.PartitionCount -= serviceDescription.PartitionCount;
            }
        }
    }
}

void AutoScaleStatistics::UpdateService(ServiceDescription const & service1, ServiceDescription const & service2)
{
    DeleteService(service1);
    AddService(service2);
}

string AutoScaleStatistics::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "CpuCores=({0})\r\nMemoryInMB=({1})\r\nCustomMetric=({2})";
    size_t index = 0;

    traceEvent.AddEventField<AutoScaleStatisticsEntry>(format, name + ".cpuCores", index);
    traceEvent.AddEventField<AutoScaleStatisticsEntry>(format, name + ".memoryInMB", index);
    traceEvent.AddEventField<AutoScaleStatisticsEntry>(format, name + ".customMetric", index);

    return format;
}

void AutoScaleStatistics::FillEventData(Common::TraceEventContext & context) const
{

    context.Write(cpuCoresStats_);
    context.Write(memoryInMBStats_);
    context.Write(customMetricStats_);
}

void AutoScaleStatistics::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.WriteLine("CpuCores={0}  MemoryInMB={1} CustomMetric={2}",
        cpuCoresStats_,
        memoryInMBStats_,
        customMetricStats_);
}
