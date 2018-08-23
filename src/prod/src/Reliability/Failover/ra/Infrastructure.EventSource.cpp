// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"
#include "Diagnostics.IEventData.h"
#include "Diagnostics.FailoverUnitEventData.h"
#include "Diagnostics.ResourceUsageReportEventData.h"

using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Diagnostics;

Global<RAEventSource> RAEventSource::Events = make_global<RAEventSource>();

void RAEventSource::TraceEventData(
    Diagnostics::IEventData const && eventData,
    std::wstring const & nodeName)
{
    switch (eventData.TraceEventType)
    {
    case TraceEventType::ReconfigurationComplete :
    {    
        const ReconfigurationCompleteEventData & reconfigComplete = dynamic_cast<const ReconfigurationCompleteEventData&>(eventData);

        // Eventually we need to deprecate the Query channel one in favor of Operational channel.
        ReconfigurationCompleted(reconfigComplete.FtId,
                                 nodeName,
                                 reconfigComplete.NodeInstance,
                                 reconfigComplete.ServiceType,
                                 reconfigComplete.CcEpoch,
                                 reconfigComplete.Type,
                                 reconfigComplete.Result,
                                 reconfigComplete.PerfData);

        ReconfigurationCompletedOperational(Guid::NewGuid(),
                                reconfigComplete.FtId,
                                nodeName,
                                reconfigComplete.NodeInstance,
                                reconfigComplete.ServiceType,
                                reconfigComplete.CcEpoch,
                                reconfigComplete.Type,
                                reconfigComplete.Result,
                                reconfigComplete.PerfData);
        break;
    }
    case TraceEventType::ReconfigurationSlow :
    {
        const ReconfigurationSlowEventData & reconfigSlow = dynamic_cast<const ReconfigurationSlowEventData&>(eventData);
        ReconfigurationSlow(reconfigSlow.FtId, reconfigSlow.NodeInstance, reconfigSlow.TraceDescription, reconfigSlow.ReconfigurationPhase);
        break;
    }
    case TraceEventType::ResourceUsageReport :
    {
        const ResourceUsageReportEventData & resourceUsage = dynamic_cast<const ResourceUsageReportEventData&>(eventData);
        ResourceUsageReportExclusiveHost(resourceUsage.FtId, resourceUsage.ReplicaId, ReplicaRole::Trace(resourceUsage.ReplicaRole), resourceUsage.NodeInstance, resourceUsage.CpuUsage, resourceUsage.MemoryUsage, resourceUsage.CpuUsageRaw, resourceUsage.MemoryUsageRaw);
        break;
    }
    case TraceEventType::ReplicaStateChange :
    {
        const ReplicaStateChangeEventData & replicaStateChange = dynamic_cast<const ReplicaStateChangeEventData&>(eventData);
        ReplicaStateChange(
            replicaStateChange.NodeInstance,
            replicaStateChange.FtId,
            replicaStateChange.ReplicaId,
            replicaStateChange.CurrentConfigurationEpoch,
            replicaStateChange.ReplicaState,
            replicaStateChange.ReplicaRole,
            replicaStateChange.Reason);
        break;
    }
    default:
        ASSERT("Invalid TraceEventType");
        break;
    }
}
