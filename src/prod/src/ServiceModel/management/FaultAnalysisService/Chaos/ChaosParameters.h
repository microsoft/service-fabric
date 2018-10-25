// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosParameters
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
        public:

            ChaosParameters();

            // The copy constructor here is required for
            // JSON serialization of a map that has
            // ChaosParameters as a value of a map entry
            // Such a map exists in ChaosSchedule
            ChaosParameters(ChaosParameters const &) = default;
            ChaosParameters(ChaosParameters &&) = default;
            ChaosParameters & operator = (ChaosParameters const & other) = default;
            ChaosParameters & operator = (ChaosParameters && other) = default;

            __declspec(property(get = get_MaxClusterStabilizationTimeoutInSeconds)) ULONG const & MaxClusterStabilizationTimeoutInSeconds;
            __declspec(property(get = get_MaxConcurrentFaults)) ULONG const & MaxConcurrentFaults;
            __declspec(property(get = get_WaitTimeBetweenIterationsInSeconds)) ULONG const & WaitTimeBetweenIterationsInSeconds;
            __declspec(property(get = get_WaitTimeBetweenFaultsInSeconds)) ULONG const & WaitTimeBetweenFaultsInSeconds;
            __declspec(property(get = get_TimeToRunInSeconds)) ULONGLONG const & TimeToRunInSeconds;
            __declspec(property(get = get_EnableMoveReplicaFaults)) bool const & EnableMoveReplicaFaults;
            __declspec(property(get = get_HealthPolicy)) std::shared_ptr<ServiceModel::ClusterHealthPolicy> const & HealthPolicy;
            __declspec(property(get = get_EventContextMap)) EventContextMap const & Context;
            __declspec(property(get = get_ChaosTargetFilter)) std::shared_ptr<ChaosTargetFilter> const & TargetFilter;

            ULONG const & get_MaxClusterStabilizationTimeoutInSeconds() const { return maxClusterStabilizationTimeoutInSeconds_; }
            ULONG const & get_MaxConcurrentFaults() const { return maxConcurrentFaults_; }
            ULONG const & get_WaitTimeBetweenIterationsInSeconds() const { return waitTimeBetweenIterationsInSeconds_; }
            ULONG const & get_WaitTimeBetweenFaultsInSeconds() const { return waitTimeBetweenFaultsInSeconds_; }
            ULONGLONG const & get_TimeToRunInSeconds() const { return timeToRunInSeconds_; }
            bool const & get_EnableMoveReplicaFaults() const { return enableMoveReplicaFaults_; }
            std::shared_ptr<ServiceModel::ClusterHealthPolicy> const & get_HealthPolicy() const { return healthPolicy_; }
            EventContextMap const & get_EventContextMap() const { return eventContextMap_; }
            std::shared_ptr<ChaosTargetFilter> const & get_ChaosTargetFilter() const { return chaosTargetFilter_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_PARAMETERS const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_PARAMETERS &) const;

            Common::ErrorCode Validate() const;

            FABRIC_FIELDS_09(
              maxClusterStabilizationTimeoutInSeconds_,
              maxConcurrentFaults_,
              waitTimeBetweenIterationsInSeconds_,
              waitTimeBetweenFaultsInSeconds_,
              timeToRunInSeconds_,
              enableMoveReplicaFaults_,
              eventContextMap_,
              healthPolicy_,
              chaosTargetFilter_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::MaxClusterStabilizationTimeoutInSeconds, maxClusterStabilizationTimeoutInSeconds_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::MaxConcurrentFaults, maxConcurrentFaults_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::WaitTimeBetweenIterationsInSeconds, waitTimeBetweenIterationsInSeconds_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::WaitTimeBetweenFaultsInSeconds, waitTimeBetweenFaultsInSeconds_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::TimeToRunInSeconds, timeToRunInSeconds_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::EnableMoveReplicaFaults, enableMoveReplicaFaults_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ClusterHealthPolicy, healthPolicy_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Context, eventContextMap_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ChaosTargetFilter, chaosTargetFilter_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            ULONG maxClusterStabilizationTimeoutInSeconds_;
            ULONG maxConcurrentFaults_;
            ULONG waitTimeBetweenIterationsInSeconds_;
            ULONG waitTimeBetweenFaultsInSeconds_;
            ULONGLONG timeToRunInSeconds_;
            bool enableMoveReplicaFaults_;
            std::shared_ptr<ServiceModel::ClusterHealthPolicy> healthPolicy_;
            EventContextMap eventContextMap_;
            std::shared_ptr<ChaosTargetFilter> chaosTargetFilter_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::FaultAnalysisService::ChaosParameters);
DEFINE_USER_MAP_UTILITY(std::wstring, Management::FaultAnalysisService::ChaosParameters);
