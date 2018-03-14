// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;

LocalLoadReportingComponent::LocalLoadReportingComponent(
    Common::ComponentRoot const & root,
    Transport::IpcClient & ipcClient)
        :failoverUnitLoadMap_(),
        lock_(),
        open_(false),
        reportTimerSPtr_(),    
        ipcClient_(ipcClient),
        Common::RootedObject(root)
{
}

LocalLoadReportingComponent::~LocalLoadReportingComponent()
{
    ASSERT_IF(reportTimerSPtr_, "LocalLoadReportingComponent::~LocalLoadReportingComponent timer is set");
}

void LocalLoadReportingComponent::Open(ReconfigurationAgentProxyId const & rapId)
{
    {
        AcquireWriteLock grab(lock_);

        id_ = rapId;

        RAPEventSource::Events->LoadReportingOpen(id_, failoverUnitLoadMap_.size());

        ASSERT_IF(open_, "LocalLoadReportingComponent::Open called while open");

        if (FailoverConfig::GetConfig().MaxNumberOfLoadReportsPerMessage > 0)
        {
            // Keep the component closed if the config value is not positive value
            open_ = true;
        }
    }
}

void LocalLoadReportingComponent::Close()
{
    {
        AcquireWriteLock grab(lock_);

        RAPEventSource::Events->LoadReportingClose(id_, failoverUnitLoadMap_.size());

        if (FailoverConfig::GetConfig().MaxNumberOfLoadReportsPerMessage > 0)
        {
            ASSERT_IFNOT(open_, "LocalLoadReportingComponent::Close while not open");

            open_ = false;

            if (reportTimerSPtr_ != nullptr)
            {
                reportTimerSPtr_->Cancel();
                reportTimerSPtr_ = nullptr;
            }

            failoverUnitLoadMap_.clear();
        }
    }
}

void LocalLoadReportingComponent::SetTimer()
{
    {
        AcquireWriteLock grab(lock_);

        if (!open_)
        {
            // Component has been shutdown
            return;
        }

        if (!reportTimerSPtr_)
        {
            auto root = Root.CreateComponentRoot();

            // Create the timer
            reportTimerSPtr_ = Common::Timer::Create(
                "RA.ReportLoad",
                [this, root] (Common::TimerSPtr const &)
                { 
                    this->ReportLoadCallback();
                });

            reportTimerSPtr_->Change(FailoverConfig::GetConfig().SendLoadReportInterval);

            RAPEventSource::Events->LoadReportingSetTimer(id_, failoverUnitLoadMap_.size());
        }
    }
}

void LocalLoadReportingComponent::UpdateTimer()
{
    {
        AcquireWriteLock grab(lock_);

        if (!open_)
        {
            // Component has been shutdown
            return;
        }

        ASSERT_IFNOT(reportTimerSPtr_, "UpdateTimer: timer is not set");

        reportTimerSPtr_->Change(FailoverConfig::GetConfig().SendLoadReportInterval);

        RAPEventSource::Events->LoadReportingUpdateTimer(id_, failoverUnitLoadMap_.size());
    }
}

void LocalLoadReportingComponent::ReportLoadCallback()
{
    vector<ReportLoadMessageBody> loadReportMessages;

    {
        AcquireWriteLock grab(lock_);

        if (open_ && !failoverUnitLoadMap_.empty())
        {
            StopwatchTime now = Stopwatch::Now();
            vector<LoadBalancingComponent::LoadOrMoveCostDescription> loadReports;
            for (auto iter = failoverUnitLoadMap_.begin(); iter != failoverUnitLoadMap_.end(); ++iter)
            {
                if (loadReports.size() == static_cast<size_t>(FailoverConfig::GetConfig().MaxNumberOfLoadReportsPerMessage))
                {
                    loadReportMessages.push_back(ReportLoadMessageBody(move(loadReports), now));
                    loadReports.clear();
                }

                loadReports.push_back(move(iter->second));
            }

            // Add whatever left off
            if (loadReports.size() > 0)
            {
                loadReportMessages.push_back(ReportLoadMessageBody(move(loadReports), now));
                loadReports.clear();
            }

            failoverUnitLoadMap_.clear();

        }
    }

    RAPEventSource::Events->LoadReportingReportCallback(id_, loadReportMessages.size());
    for (auto iter = loadReportMessages.begin(); iter != loadReportMessages.end(); ++iter)
    {
        Transport::MessageUPtr msgUPtr = RAMessage::GetReportLoad().CreateMessage(*iter);
        ipcClient_.SendOneWay(move(msgUPtr));
    }

    UpdateTimer();
}

void LocalLoadReportingComponent::AddLoad(
    FailoverUnitId const& fuId, 
    wstring && serviceName, 
    bool isStateful, 
    ReplicaRole::Enum replicaRole, 
    vector<LoadBalancingComponent::LoadMetric> && loadMetrics,
    Federation::NodeId const& nodeId)
{
    {
        AcquireWriteLock grab(lock_);

        if (!open_)
        {
            return;
        }

        if (!IsValidLoad(isStateful, replicaRole, loadMetrics))
        {
            RAPEventSource::Events->LoadReportingInvalidLoad(id_, fuId.Guid, replicaRole);
            return;
        }

        for (auto const& metric : loadMetrics)
        {
            RAPEventSource::Events->LoadReportingAddLoad(id_, failoverUnitLoadMap_.size(), fuId.Guid, replicaRole, metric.Name, metric.Value);
        }
        
        auto entry = failoverUnitLoadMap_.find(fuId);
        if (entry == failoverUnitLoadMap_.end())
        {
            entry = failoverUnitLoadMap_.insert(make_pair(fuId, LoadBalancingComponent::LoadOrMoveCostDescription(fuId.Guid, move(serviceName), isStateful))).first;
        }

        entry->second.MergeLoads(ReplicaRole::ConvertToPLBReplicaRole(isStateful, replicaRole), move(loadMetrics), Stopwatch::Now(), true, nodeId);
    }

    SetTimer();
}

bool LocalLoadReportingComponent::IsValidLoad(
    bool isStateful, 
    Reliability::ReplicaRole::Enum replicaRole, 
    std::vector<LoadBalancingComponent::LoadMetric> const& loadMetrics)
{
    bool isValid = true;

    if(isStateful && replicaRole == ReplicaRole::None)
    {
        isValid = false;
    }
    else
    {
        auto invalidMetric = find_if(
        loadMetrics.begin(),
        loadMetrics.end(),
            [] (LoadBalancingComponent::LoadMetric const& metric)
            {
                return !Common::ParameterValidator::IsValid(metric.Name.c_str(), Common::ParameterValidator::MinStringSize, Common::ParameterValidator::MaxNameSize).IsSuccess();
            }
        );

        if (invalidMetric != loadMetrics.end())
        {
            isValid = false;
        }
    }

    return isValid;
}

void LocalLoadReportingComponent::RemoveFailoverUnit(FailoverUnitId const & fuId)
{
    {
        AcquireWriteLock grab(lock_);

        auto reportLoadEntry = failoverUnitLoadMap_.find(fuId);
        if (reportLoadEntry != failoverUnitLoadMap_.end())
        {
            RAPEventSource::Events->LoadReportingRemoveLoad(id_, failoverUnitLoadMap_.size(), fuId.Guid);

            failoverUnitLoadMap_.erase(fuId);
        }
    }
}
