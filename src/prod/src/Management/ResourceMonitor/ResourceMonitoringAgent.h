// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ResourceUsage.h"
#include "ResourceDescription.h"
#include "RM.EventSource.h"
#include "Reliability/Reliability.h"

namespace ResourceMonitor
{
    // This is the agent that does the actual work (measures resource consumption).
    class ResourceMonitoringAgent :
        public Common::ComponentRoot,
        public Common::AsyncFabricComponent,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ResourceMonitor>
    {
        DENY_COPY(ResourceMonitoringAgent)
    public:

        ResourceMonitoringAgent(Hosting2::ComFabricRuntime * pRuntime, Transport::IpcClient & ipcClient);
        virtual ~ResourceMonitoringAgent();

    public:
        void MeasureResourceUsage(Common::TimerSPtr const & timer);
        void SendResourceUsage(Common::TimerSPtr const & timer);

        void ProcessIpcMessage(Transport::Message & message, Transport::IpcReceiverContextUPtr & context);
        void UpdateFromHost(Management::ResourceMonitor::HostUpdateRM & update);
        void MonitorCallback(Common::TimerSPtr const & timer);
        void SendCallback(Common::TimerSPtr const & timer);
        void TraceCallback(Common::TimerSPtr const & timer);

        void OnGetResponse(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously, Common::TimerSPtr const & timer);
        std::vector<std::wstring> GetHostsToMeasure();
        void UpdateWithMeasurements(std::map<std::wstring, Management::ResourceMonitor::ResourceMeasurement> const & measurements);


        void CreateTimers();
        void RegisterMessageHandler();

    protected:
        //AsyncFabricComponent methods
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &);

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &);

        virtual void OnAbort();

    private:

        std::map<std::wstring, ResourceDescription> hostsToMonitor_;
        std::map<Common::Guid, ResourceDescription> partitionsToMonitor_;
        // Used to send resource measure requests to fabric host
        std::unique_ptr<Transport::IpcClient> fabricHostClient_;
        // ID of application host where this service is running.
        std::wstring hostId_;

        Federation::NodeId nodeId_;

        // Timer to contron how often usage is measured.
        Common::TimerSPtr monitorTimerSPtr_;
        // Timer to control how often reports are sent to RA.
        Common::TimerSPtr sendingTimerSPtr_;
        // Timer to control how often we do tracing for shared hosts
        Common::TimerSPtr tracingTimerSPtr_;

        Transport::IpcClient & ipcClient_;

        // Open and Close async operations.
        class OpenAsyncOperation;
        class CloseAsyncOperation;

        Hosting2::ComFabricRuntime * pRuntime_;

        Common::RwLock lock_;

        RMSEventSource resourceMonitorEventSource_;

        friend class ResourceMonitorServiceInstance;
    };
}
