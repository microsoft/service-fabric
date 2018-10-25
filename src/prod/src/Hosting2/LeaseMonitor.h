// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class IpcClient;
};

namespace Hosting2
{
    class LeaseMonitor
        : Common::RootedObject
        , Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(LeaseMonitor);
    public:
        LeaseMonitor(
            Common::ComponentRoot & root,
            std::wstring const & traceId,
            Transport::IpcClient & ipcClient,
            HANDLE leaseHandle,
            int64 initialLeaseExpirationInTicks,
            Common::TimeSpan const leaseDuration,
            bool useDirectLease);

        bool IsLeaseExpired();

        void Close();

        static void DisableIpcLeasePolling();

    private:
        Common::TimeSpan GetPollInterval() const;
        Common::TimeSpan GetPollDelay(Common::TimeSpan pollInterval) const;
        void SetPollTimer(Common::TimeSpan leaseTtl, bool isForceSet);
        void PollLeaseExpiration();
        void OnLeaseRequestComplete(Common::AsyncOperationSPtr const & operation, Common::StopwatchTime requestTime);
        bool IsLeaseExpired_DirectRead();

        const std::wstring traceId_;
        const HANDLE leaseHandle_;
        Transport::IpcClient & ipcClient_;
        volatile int64 expirationInTicks_;
        Common::TimerSPtr pollTimer_;
        bool useDirectLease_;
        const double minLeaseRenewInterval = 1000;
    };
}
