// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace ApiMonitoring
    {
        class MonitoringData;

        typedef std::pair<MonitoringData const *, FABRIC_SEQUENCE_NUMBER> MonitoringHealthEvent;

        typedef std::vector<MonitoringHealthEvent> MonitoringHealthEventList;

        class ApiCallDescription;
        typedef std::shared_ptr<ApiCallDescription> ApiCallDescriptionSPtr;

        typedef std::function<void(MonitoringHealthEventList const &)> HealthReportCallback;
        typedef std::function<void(MonitoringData const &)> ApiEventTraceCallback;
        typedef std::function<void(MonitoringData const &, Common::TimeSpan, Common::ErrorCode const &)> ApiFinishTraceCallback;

        class MonitoringComponent;
        typedef std::unique_ptr<MonitoringComponent> MonitoringComponentUPtr;

		typedef std::pair<bool, bool> MonitoringActionsNeeded;
    }
}

