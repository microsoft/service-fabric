// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class DnsServiceEnvironmentManager
        : Common::RootedObject
        , Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(DnsServiceEnvironmentManager);

    public:
        DnsServiceEnvironmentManager(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting);

        virtual ~DnsServiceEnvironmentManager();

        Common::AsyncOperationSPtr BeginSetupDnsEnvAndStartMonitor(
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent);
        Common::ErrorCode EndSetupDnsEnvAndStartMonitor(AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginRemoveDnsFromEnvAndStopMonitor(
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRemoveDnsFromEnvAndStopMonitor(AsyncOperationSPtr const & operation);

        Common::ErrorCode RemoveDnsFromEnvAndStopMonitorAndWait();

        void StopMonitor();

    private:
        void OnConfigureNodeForDnsServiceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
        void OnTimer();
        bool IsDnsEnvironmentHealthy();
        void StopMonitorUnsafe();

    private:
        HostingSubsystem & hosting_;
        Common::TimerSPtr timer_;
        RwLock lock_;
#if !defined(PLATFORM_UNIX)
        std::vector<std::wstring> adapters_;
        std::vector<HANDLE> adapterChangedEvents_;
#endif
    };
}
