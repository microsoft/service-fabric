// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class ApiMonitoringWrapper final
        : public KObject<ApiMonitoringWrapper>
        , public KShared<ApiMonitoringWrapper>
        , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TR>
    {
        K_FORCE_SHARED(ApiMonitoringWrapper)

        class ApiMonitoringComponent;
        typedef std::shared_ptr<ApiMonitoringComponent> ApiMonitoringComponentSPtr;

    public:
        __declspec(property(get = get_Component)) ApiMonitoringComponentSPtr Component;
        ApiMonitoringComponentSPtr get_Component() const
        {
            return component_;
        }

        static ApiMonitoringWrapper::SPtr Create(
            __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
            __in Common::TimeSpan const & scanInterval,
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in KAllocator & allocator);

        static Common::ApiMonitoring::ApiCallDescriptionSPtr GetApiCallDescriptionFromName(
            __in Data::Utilities::PartitionedReplicaId & partitionedReplicaId,
            __in Common::ApiMonitoring::ApiNameDescription const & nameDesc,
            __in Common::TimeSpan const & slowApiTime);
    private:

        class ApiMonitoringComponent final
            : public Common::ComponentRoot
        {
        public:

            static ApiMonitoringComponentSPtr Create(
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in Common::TimeSpan const & scanInterval,
                __in::FABRIC_REPLICA_ID const & endpointUniqueId_,
                __in Common::Guid const & partitionId_);

            static Common::ApiMonitoring::ApiCallDescriptionSPtr GetApiCallDescriptionFromName(
                __in Data::Utilities::PartitionedReplicaId & partitionedReplicaId,
                __in Common::ApiMonitoring::ApiNameDescription const & nameDesc,
                __in Common::TimeSpan const & slowApiTime);

            void StartMonitoring(
                __in Common::ApiMonitoring::ApiCallDescriptionSPtr const & desc);

            void StopMonitoring(
                __in Common::ApiMonitoring::ApiCallDescriptionSPtr const & desc,
                __out Common::ErrorCode const & errorCode);

            void Open(
                __in Common::TimeSpan const & timeSpan);

            ~ApiMonitoringComponent();
            void Close();
        private:

            ApiMonitoringComponent(
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in::FABRIC_REPLICA_ID const & endpointUniqueId_,
                __in Common::Guid const & partitionId_);

            void SlowHealthCallback(Common::ApiMonitoring::MonitoringHealthEventList const & slowList);
            void ClearSlowHealth(Common::ApiMonitoring::MonitoringHealthEventList const & slowList);
            static void ReportHealth(
                const std::wstring traceId,
                FABRIC_HEALTH_STATE toReport,
                Common::ApiMonitoring::MonitoringHealthEvent const & monitoringEvent,
                Reliability::ReplicationComponent::IReplicatorHealthClientSPtr healthClient);

            Data::Utilities::PartitionedReplicaId::SPtr partitionedReplicaId_;
            KSpinLock lock_;
            Common::ApiMonitoring::MonitoringComponentUPtr monitor_;
            Reliability::ReplicationComponent::IReplicatorHealthClientSPtr healthClient_;
            bool isActive_;
            ::FABRIC_REPLICA_ID const endpointUniqueId_;
            Common::Guid const partitionId_;
        };

        ApiMonitoringWrapper(
            __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
            __in Common::TimeSpan const & scanInterval,
            __in Data::Utilities::PartitionedReplicaId const & traceId);

        ApiMonitoringComponentSPtr component_;
    };
}
