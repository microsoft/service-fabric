// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // LocalHealthReportingComponent; used by RAP to manage health reporting
        class LocalHealthReportingComponent: 
            public Common::RootedObject
        {
            DENY_COPY(LocalHealthReportingComponent);
        public:
            LocalHealthReportingComponent(
                Common::ComponentRoot const & root,
                Client::IpcHealthReportingClientSPtr && healthClient);

            void Open(
                ReconfigurationAgentProxyId const & id, 
                Federation::NodeInstance const & nodeInstance,
                wstring const & nodeName);
            
            void Close();
            
            void StartMonitoring(Common::ApiMonitoring::ApiCallDescriptionSPtr const & apiCallDescription);

            void StopMonitoring(
                Common::ApiMonitoring::ApiCallDescriptionSPtr const & apiCallDescription, 
                Common::ErrorCode const & error);

            void ReportHealth(
                std::vector<ServiceModel::HealthReport> && healthReports,
                ServiceModel::HealthReportSendOptionsUPtr && sendOptions);

            void ReportHealth(
                ServiceModel::HealthReport && healthReport,
                ServiceModel::HealthReportSendOptionsUPtr && sendOptions);
            
        private:
            void OnHealthEvent(
                Common::SystemHealthReportCode::Enum code, 
                Common::ApiMonitoring::MonitoringHealthEventList const & events, 
                ApiMonitoring::MonitoringComponentMetadata const & metaData,
                Common::TimeSpan const & ttl,
                std::wstring const & status);

            Client::IpcHealthReportingClientSPtr healthClient_;
            Common::ApiMonitoring::MonitoringComponentUPtr monitoringComponent_;
        };
    }
}
