//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Transport;

StringLiteral const TraceType("ContainerEventTracker");

ContainerEventTracker::ContainerEventTracker(ContainerActivator & owner)
    : owner_(owner)
    , map_()
    , mapLock_()
    , eventLock_()
    , eventOffTime_(L"1970-01-01T00:00:00.000Z")
{
    this->InitializeEventSinceTime();
}

void ContainerEventTracker::ProcessContainerEvents(ContainerEventNotification && notification)
{
    {
        AcquireExclusiveLock lock(eventLock_);

        if (notification.UntilTime < eventSinceTime_)
        {
            WriteNoise(
                TraceType,
                "Ignoring stale event notification with SinceTime={0}, UntilTime={1}. EventTrackerSinceTime={2}.",
                notification.SinceTime,
                notification.UntilTime,
                eventSinceTime_);

            return;
        }

        HealthEventMap healthEventMap;
        this->NotifyTerminationAndMergeHealthEvents(move(notification), healthEventMap);

        HealthStatusInfoMap healthStatusInfoMap;
        this->GroupByNode(move(healthEventMap), healthStatusInfoMap);

        this->NotifyHealthEvents(move(healthStatusInfoMap));

        eventSinceTime_ = notification.UntilTime;
    }
}

void ContainerEventTracker::TrackContainer(
    wstring const & containerName,
    wstring const & appHostId,
    wstring const & nodeId,
    bool reportDockerHealthStatus)
{
    AcquireWriteLock lock(mapLock_);
    this->map_[containerName] = move(ContainerInfo(appHostId, nodeId, reportDockerHealthStatus));
}

void ContainerEventTracker::UntrackContainer(wstring const & containerName)
{
    AcquireWriteLock lock(mapLock_);

    auto it = map_.find(containerName);
    if (it != map_.end())
    {
        map_.erase(it);
    }
}

int64 ContainerEventTracker::GetEventSinceTime()
{
    AcquireExclusiveLock lock(eventLock_);
    return eventSinceTime_;
}

void ContainerEventTracker::NotifyTerminationAndMergeHealthEvents(
    ContainerEventNotification && notification,
    __out HealthEventMap & healthEventMap)
{
    for (auto const & containerEvent : notification.EventList)
    {
        auto ignoreEvent = (containerEvent.TimeStampInSeconds < eventSinceTime_);

        WriteNoise(
            TraceType,
            "Received event for Container:{0}, ID:{1}, Time:{2}, EventType:{3}, IsHealthy:{4}, IgnoreEvent={5}.",
            containerEvent.ContainerName,
            containerEvent.ContainerId,
            containerEvent.TimeStampInSeconds,
            containerEvent.EventType,
            containerEvent.IsHealthy,
            ignoreEvent);

        if (ignoreEvent)
        {
            continue;
        }

        auto containerName = containerEvent.ContainerName;

        if (containerEvent.IsHealthEvent)
        {
            if (!this->ShouldReportDockerHealthStatus(containerName))
            {
                WriteNoise(
                    TraceType,
                    "Skipping docker health status reporting for Container:{0}.",
                    containerName);

                continue;
            }

            auto iter = healthEventMap.find(containerName);
            if (iter == healthEventMap.end())
            {
                healthEventMap.insert(make_pair(containerName, containerEvent));
                continue;
            }

            //
            // Only keep the latest health status
            //
            if (iter->second.TimeStampInSeconds < containerEvent.TimeStampInSeconds)
            {
                iter->second = containerEvent;
            }
        }
        else
        {
            this->OnContainerTerminated(containerName, containerEvent.ExitCode);
        }
    }
}

void ContainerEventTracker::GroupByNode(
    HealthEventMap && healthEventMap,
    __out HealthStatusInfoMap & healthStatusInfoMap)
{
    if (healthEventMap.empty())
    {
        return;
    }

    for (auto const & healthEvent : healthEventMap)
    {
        wstring nodeId;
        wstring appHostId;

        if (this->TryGetNodeIdAndAppHostId(healthEvent.first, nodeId, appHostId))
        {
            auto iter = healthStatusInfoMap.find(nodeId);
            if (iter == healthStatusInfoMap.end())
            {
                auto res = healthStatusInfoMap.insert(
                    make_pair(move(nodeId), move(vector<ContainerHealthStatusInfo>())));

                iter = res.first;
            }

            iter->second.push_back(
                move(
                    ContainerHealthStatusInfo(
                        appHostId,
                        healthEvent.first,
                        healthEvent.second.TimeStampInSeconds,
                        healthEvent.second.IsHealthy)));
        }
    }
}

void ContainerEventTracker::NotifyHealthEvents(HealthStatusInfoMap && healthStatusInfoMap)
{
    for (auto const & healthStatusInfo : healthStatusInfoMap)
    {
        owner_.healthCheckStatusCallback_(healthStatusInfo.first, healthStatusInfo.second);
    }
}

bool ContainerEventTracker::ShouldReportDockerHealthStatus(wstring const & containerName)
{
    {
        AcquireReadLock lock(mapLock_);

        auto it = map_.find(containerName);
        if (it != map_.end() && it->second.ReportDockerHealthStatus)
        {
            return true;
        }
    }

    return false;
}

void ContainerEventTracker::OnContainerTerminated(wstring const & containerName, DWORD exitCode)
{
    wstring nodeId;
    wstring appServiceId;
    {
        AcquireWriteLock lock(mapLock_);

        auto it = map_.find(containerName);
        if (it != map_.end())
        {
            appServiceId = it->second.AppHostId;
            nodeId = it->second.NodeId;
            map_.erase(it);
        }
    }

    if (!appServiceId.empty() && !nodeId.empty())
    {
        owner_.terminationCallback_(appServiceId, nodeId, exitCode);
    }
}

bool ContainerEventTracker::TryGetNodeIdAndAppHostId(
    __in wstring const & containerName,
    __out wstring & nodeId,
    __out wstring & appHostId)
{
    wstring tempNodeId;
    wstring tempAppHostId;

    {
        AcquireReadLock lock(mapLock_);

        auto it = map_.find(containerName);
        if (it != map_.end())
        {
            tempAppHostId = it->second.AppHostId;
            tempNodeId = it->second.NodeId;
        }
    }

    if (!tempNodeId.empty() && !tempAppHostId.empty())
    {
        nodeId = move(tempNodeId);
        appHostId = move(tempAppHostId);

        return true;
    }

    return false;
}

void ContainerEventTracker::InitializeEventSinceTime()
{
    auto nowTime = DateTime::Now();
    auto sinceTime = (nowTime - eventOffTime_);
    auto elapsedTime = TimeSpan::FromTicks(sinceTime.Ticks);

    eventSinceTime_ = elapsedTime.TotalSeconds();
}



