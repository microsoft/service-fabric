// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ResourceMonitor
{
    class TelemetryEvent;
    class TelemetryClient :
        public Common::RootedObject
    {
        DENY_COPY(TelemetryClient)
    public:

        TelemetryClient(std::wstring const & instrumentationKey, std::wstring const & telemetryUri, Common::ComponentRoot const & root);

        Common::AsyncOperationSPtr BeginReportTelemetry(
            std::vector<TelemetryEvent> && events,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndReportTelemetry(
            Common::AsyncOperationSPtr const & operation);

    private:
        class SendEventsAsyncOperation;

        std::wstring instrumentationKey_;
        std::wstring telemetryUri_;
    };
}
