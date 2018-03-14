// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"
#include "ResourceMonitor.ResourceComponent.h"
#include "Diagnostics.IEventWriter.h"
#include "Diagnostics.ResourceUsageReportEventData.h"
#include "ServiceModel/management/ResourceMonitor/public.h"
#include "Management/ResourceMonitor/config/ResourceMonitorServiceConfig.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Management::ResourceMonitor;

Reliability::ReconfigurationAgentComponent::ResourceMonitor::ResourceComponent::ResourceComponent(ReconfigurationAgent & ra):
    ra_(ra),
    lastTraceTime_(ra_.Clock.Now())
{
}

bool Reliability::ReconfigurationAgentComponent::ResourceMonitor::ResourceComponent::ProcessResoureUsageMessage(Management::ResourceMonitor::ResourceUsageReport const & resourceUsageReport)
{
    std::vector<LoadBalancingComponent::LoadOrMoveCostDescription> loadReportVector;
    bool shouldTrace = false;
    auto now = ra_.Clock.Now();
    if (lastTraceTime_ + ::ResourceMonitor::ResourceMonitorServiceConfig::GetConfig().ResourceTracingInterval <= now)
    {
        shouldTrace = true;
        lastTraceTime_ = now;
    }

    ExecuteUnderReadLockOverMap<FailoverUnit>(
        ra_.LocalFailoverUnitMapObj,
        [&](ReadOnlyLockedFailoverUnitPtr & lockedFT)
    {
        if (lockedFT.IsEntityDeleted || lockedFT.get_Current() == nullptr || lockedFT->IsClosed)
        {
            return;
        }

        //PLB can only associate load on a partition basis so we can only report this for exclusive processes
        if (lockedFT->ServiceDescription.PackageActivationMode == ServiceModel::ServicePackageActivationMode::ExclusiveProcess)
        {
            auto itResourceUsage = resourceUsageReport.ResourceUsageReports.find(lockedFT->FailoverUnitId);
            if (itResourceUsage != resourceUsageReport.ResourceUsageReports.end())
            {
                LoadBalancingComponent::LoadOrMoveCostDescription loadDescription;
                //PLB handles only reports for replicas that are primary/secondary from PLB point of view
                bool isReportValid = LoadBalancingComponent::LoadOrMoveCostDescription::CreateLoadReportForResources(lockedFT->FailoverUnitId.Guid, 
                    lockedFT->ServiceDescription.Name, lockedFT->IsStateful,
                    ReplicaRole::ConvertToPLBReplicaRole(lockedFT->IsStateful, lockedFT->LocalReplica.CurrentConfigurationRole),
                    ra_.NodeId, itResourceUsage->second.CpuUsage, itResourceUsage->second.MemoryUsage, loadDescription);

                if (isReportValid)
                {
                    if (shouldTrace)
                    {
                        Diagnostics::ResourceUsageReportEventData resourceEvent(lockedFT->FailoverUnitId.Guid, ra_.NodeInstance,
                            lockedFT->LocalReplica.CurrentConfigurationRole, lockedFT->LocalReplica.ReplicaId,
                            itResourceUsage->second.CpuUsage, itResourceUsage->second.MemoryUsage,
                            itResourceUsage->second.CpuUsageRaw, itResourceUsage->second.MemoryUsageRaw);
                        ra_.EventWriter.Trace(move(resourceEvent));
                    }

                    loadReportVector.push_back(move(loadDescription));
                }
            }
        }

    });

    if (loadReportVector.size() > 0)
    {
        ra_.ProcessLoadReport(ReportLoadMessageBody(move(loadReportVector), Stopwatch::Now()));
    }

    return true;
}
