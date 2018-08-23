//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Hosting2
{
    struct ContainerInfo
    {
        ContainerInfo()
            : AppHostId()
            , NodeId()
            , ReportDockerHealthStatus(false)
        {
        }

        ContainerInfo(
            wstring const & appHostId,
            wstring const & nodeId,
            bool reportDockerHealthStatus)
            : AppHostId(appHostId)
            , NodeId(nodeId)
            , ReportDockerHealthStatus(reportDockerHealthStatus)
        {
        }

        wstring AppHostId;
        wstring NodeId;
        bool ReportDockerHealthStatus;
    };

    typedef std::map<
        std::wstring /*ContainerName*/,
        ContainerEventDescription,
        Common::IsLessCaseInsensitiveComparer<std::wstring>> HealthEventMap;

    typedef std::map<
        std::wstring, /* Node id*/
        std::vector<ContainerHealthStatusInfo>,
        Common::IsLessCaseInsensitiveComparer<wstring>> HealthStatusInfoMap;

    typedef std::map<
        std::wstring, /* ContainerName*/
        ContainerInfo,
        Common::IsLessCaseInsensitiveComparer<std::wstring>> ContainerInfoMap;

    // Forward declaration.
    class ContainerActivator;

    class ContainerEventTracker : 
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
    public:
        ContainerEventTracker(ContainerActivator & owner);

        void ProcessContainerEvents(ContainerEventNotification && notification);

        void TrackContainer(
            std::wstring const & containerName,
            std::wstring const & appHostId,
            std::wstring const & nodeId,
            bool reportDockerHealthStatus);

        void UntrackContainer(std::wstring const & containerName);

        int64 GetEventSinceTime();

    private:
        void NotifyTerminationAndMergeHealthEvents(
            ContainerEventNotification && notification,
            __out HealthEventMap & healthEventMap);

        void GroupByNode(
            HealthEventMap && healthEventMap,
            __out HealthStatusInfoMap & healthStatusInfoMap);

        void NotifyHealthEvents(HealthStatusInfoMap && healthStatusInfoMap);

        bool ShouldReportDockerHealthStatus(std::wstring const & containerName);

        void OnContainerTerminated(std::wstring const & containerName, DWORD exitCode);

        bool TryGetNodeIdAndAppHostId(
            __in std::wstring const & containerName,
            __out std::wstring & nodeId,
            __out std::wstring & appHostId);

        void InitializeEventSinceTime();

    private:
        Common::RwLock mapLock_;
        ContainerActivator & owner_;
        ContainerInfoMap map_;

        Common::ExclusiveLock eventLock_;
        const Common::DateTime eventOffTime_;
        int64 eventSinceTime_;
    };
}
