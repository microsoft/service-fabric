// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ApiMonitoringWrapper : public Common::ComponentRoot
        {
            DENY_COPY(ApiMonitoringWrapper)

        public:
            static ApiMonitoringWrapperSPtr Create(
                IReplicatorHealthClientSPtr & healthClient,
                REInternalSettingsSPtr const & config,
                Common::Guid const & partitionId,
                ReplicationEndpointId const & endpointId);

            static Common::ApiMonitoring::ApiCallDescriptionSPtr GetApiCallDescriptionFromName(
                ReplicationEndpointId const & currentReplicatorId,
                Common::ApiMonitoring::ApiNameDescription const & nameDesc, 
                Common::TimeSpan const & slowApiTime);
            
            void StartMonitoring(Common::ApiMonitoring::ApiCallDescriptionSPtr const & desc);
            void StopMonitoring(Common::ApiMonitoring::ApiCallDescriptionSPtr const & desc, Common::ErrorCode const & errorCode);

            ~ApiMonitoringWrapper();
            void Close();

        private:
            static void ReportHealth(
                Common::Guid const & partitionId,
                ReplicationEndpointId const & endpointId,
                FABRIC_HEALTH_STATE toReport,
                Common::ApiMonitoring::MonitoringHealthEvent const & monitoringEvent,
                IReplicatorHealthClientSPtr healthClient);

            ApiMonitoringWrapper(
                IReplicatorHealthClientSPtr & healthClient,
                Common::Guid const & partitionId,
                ReplicationEndpointId const & endpointId);

            void Open(Common::TimeSpan const & scanInterval);

            void SlowHealthCallback(Common::ApiMonitoring::MonitoringHealthEventList const & slowList);
            void ClearSlowHealth(Common::ApiMonitoring::MonitoringHealthEventList const & slowList);
            
            Common::RwLock lock_;
            IReplicatorHealthClientSPtr healthClient_;
            Common::ApiMonitoring::MonitoringComponentUPtr monitor_;
            bool isActive_;
            ReplicationEndpointId const endpointUniqueId_; 
            Common::Guid const partitionId_;
        };
    }
}
