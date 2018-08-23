// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class PartitionedServiceDescWrapper : public Common::IFabricJsonSerializable
    {
        DENY_COPY(PartitionedServiceDescWrapper)
    public:
        PartitionedServiceDescWrapper();
        PartitionedServiceDescWrapper(
            FABRIC_SERVICE_KIND serviceKind,
            std::wstring const& applicationName,
            std::wstring const& serviceName,
            std::wstring const& serviceTypeName,
            Common::ByteBuffer const& initializationData,
            PartitionSchemeDescription::Enum partScheme,
            uint partitionCount_,       
            __int64 lowKeyInt64_,
            __int64 highKeyInt64_,
            std::vector<std::wstring> const& partitionNames,
            LONG instanceCount,
            LONG TargetReplicaSize,
            LONG MinReplicaSize,
            bool HasPersistedState,
            std::wstring const& placementConstraints,
            std::vector<Reliability::ServiceCorrelationDescription> const& correlations,
            std::vector<Reliability::ServiceLoadMetricDescription> const& metrics,
            std::vector<ServicePlacementPolicyDescription> const& placementPolicies,
            FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS flags,
            DWORD ReplicaRestartWaitDurationSeconds,
            DWORD QuorumLossWaitDurationSeconds,
            DWORD StandByReplicaKeepDurationSeconds,
            FABRIC_MOVE_COST defaultMoveCost,
            ServicePackageActivationMode::Enum const servicePackageActivationMode,
            std::wstring const& serviceDnsName,
            std::vector<Reliability::ServiceScalingPolicyDescription> const & scalingPolicies);
       
        ~PartitionedServiceDescWrapper() {};

        PartitionedServiceDescWrapper & operator = (PartitionedServiceDescWrapper && other) = default;
        PartitionedServiceDescWrapper(PartitionedServiceDescWrapper && other) = default;

        Common::ErrorCode FromPublicApi(
            __in FABRIC_SERVICE_DESCRIPTION const & serviceDescription);

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_SERVICE_DESCRIPTION & serviceDescription) const;

        __declspec(property(get=get_ServiceKind)) FABRIC_SERVICE_KIND const &ServiceKind;
        __declspec(property(get=get_AppName, put=set_AppName)) std::wstring &ApplicationName;
        __declspec(property(get=get_ServiceName)) std::wstring const &ServiceName;
        __declspec(property(get=get_ServiceTypeName)) std::wstring const &ServiceTypeName;
        __declspec(property(get=get_InitData)) Common::ByteBuffer const &InitializationData;
        __declspec(property(get=get_PartitionScheme)) PartitionSchemeDescription::Enum const &PartitionScheme;
        __declspec(property(get=get_PartitionCount)) uint const &PartitionCount;
        __declspec(property(get=get_LowKey)) __int64 const &LowKeyInt64;
        __declspec(property(get=get_HighKey)) __int64 const& HighKeyInt64;
        __declspec(property(get=get_PartitionNames)) std::vector<std::wstring> const& PartitionNames;
        __declspec(property(get=get_InstanceCount)) LONG const& InstanceCount;
        __declspec(property(get=get_TargetReplicaSize)) LONG const& TargetReplicaSetSize;
        __declspec(property(get=get_MinReplicaSize)) LONG const& MinReplicaSetSize;
        __declspec(property(get=get_HasPersistedState)) bool const& HasPersistedState;
        __declspec(property(get=get_PlacementConstraints)) std::wstring const &PlacementConstraints;
        __declspec(property(get=get_Correlations)) std::vector<Reliability::ServiceCorrelationDescription> const &Correlations;
        __declspec(property(get=get_Metrics)) std::vector<Reliability::ServiceLoadMetricDescription> const &Metrics;
        __declspec(property(get=get_PlacementPolicies)) std::vector<ServicePlacementPolicyDescription> const &PlacementPolicies;
        __declspec(property(get=get_Flags)) FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS Flags;
        __declspec(property(get=get_ReplicaRestartWaitDurationSeconds)) DWORD ReplicaRestartWaitDurationSeconds;
        __declspec(property(get=get_QuorumLossWaitDurationSeconds)) DWORD QuorumLossWaitDurationSeconds;
        __declspec(property(get=get_StandByReplicaKeepDurationSeconds)) DWORD StandByReplicaKeepDurationSeconds;
        __declspec(property(get=get_IsServiceGroup, put=set_IsServiceGroup)) bool IsServiceGroup;
        __declspec(property(get=get_DefaultMoveCost)) FABRIC_MOVE_COST const DefaultMoveCost;
        __declspec(property(get=get_IsDefaultMoveCostSpecified)) bool const IsDefaultMoveCostSpecified;
        __declspec(property(get = get_PackageActivationMode)) ServicePackageActivationMode::Enum const PackageActivationMode;
        __declspec(property(get = get_ServiceDnsName)) std::wstring const &ServiceDnsName;
        __declspec(property(get = get_ScalingPolicies)) std::vector<Reliability::ServiceScalingPolicyDescription> const& ScalingPolicies;

        FABRIC_SERVICE_KIND const& get_ServiceKind() const { return serviceKind_; }
        std::wstring const& get_AppName() const { return applicationName_; }
        std::wstring const& get_ServiceName() const { return serviceName_; }
        std::wstring const& get_ServiceTypeName() const { return serviceTypeName_; }
        Common::ByteBuffer const& get_InitData() const { return initializationData_; }
        PartitionSchemeDescription::Enum const& get_PartitionScheme() const { return partitionDescription_.Scheme; }
        uint const& get_PartitionCount() const { return partitionDescription_.PartitionCount; }
        __int64 const& get_LowKey() const { return partitionDescription_.LowKey; }
        __int64 const& get_HighKey() const { return partitionDescription_.HighKey; }
        std::vector<std::wstring> const& get_PartitionNames() const { return partitionDescription_.PartitionNames; }
        LONG const& get_InstanceCount() const { return instanceCount_; }
        LONG const& get_TargetReplicaSize() const { return targetReplicaSetSize_; }
        LONG const& get_MinReplicaSize() const { return minReplicaSetSize_; }
        bool const& get_HasPersistedState() const { return hasPersistedState_; }
        std::wstring const & get_PlacementConstraints() const { return placementConstraints_; }
        FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS get_Flags() const { return flags_; }
        DWORD get_ReplicaRestartWaitDurationSeconds() const { return replicaRestartWaitDurationSeconds_; }
        DWORD get_QuorumLossWaitDurationSeconds() const { return quorumLossWaitDurationSeconds_; }
        DWORD get_StandByReplicaKeepDurationSeconds() const { return standByReplicaKeepDurationSeconds_; }
        bool get_IsServiceGroup() const { return isServiceGroup_; }
        void set_IsServiceGroup (bool value) { isServiceGroup_ = value; }
        void set_AppName(std::wstring const& appName) { applicationName_ = appName; }
        FABRIC_MOVE_COST get_DefaultMoveCost() const { return defaultMoveCost_; }
        bool get_IsDefaultMoveCostSpecified() const { return isDefaultMoveCostSpecified_; }
        ServicePackageActivationMode::Enum get_PackageActivationMode() const { return servicePackageActivationMode_; }
        std::wstring const& get_ServiceDnsName() const { return serviceDnsName_; }
        std::vector<Reliability::ServiceScalingPolicyDescription> const& get_ScalingPolicies() const { return scalingPolicies_; }

        std::vector<Reliability::ServiceCorrelationDescription> const & get_Correlations() const
        {
            return correlations_;
        }

        std::vector<Reliability::ServiceLoadMetricDescription> const & get_Metrics() const
        {
            return metrics_;
        }

        std::vector<ServicePlacementPolicyDescription> const & get_PlacementPolicies() const
        {
            return placementPolicies_;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::ServiceKind, serviceKind_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationName, applicationName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceName, serviceName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceTypeName, serviceTypeName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::InitializationData, initializationData_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PartitionDescription, partitionDescription_)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::TargetReplicaSetSize, targetReplicaSetSize_, serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::MinReplicaSetSize, minReplicaSetSize_, serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::HasPersistedState, hasPersistedState_, serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::InstanceCount, instanceCount_, serviceKind_ == FABRIC_SERVICE_KIND_STATELESS)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PlacementConstraints, placementConstraints_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::CorrelationScheme, correlations_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceLoadMetrics, metrics_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServicePlacementPolicies, placementPolicies_)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::Flags, (LONG&)flags_, serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::ReplicaRestartWaitDurationSeconds, replicaRestartWaitDurationSeconds_, (serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL && hasPersistedState_ && ((flags_ & FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION) != 0)))
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::QuorumLossWaitDurationSeconds, quorumLossWaitDurationSeconds_, (serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL && hasPersistedState_ && ((flags_ & FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION) != 0)))
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::StandByReplicaKeepDurationSeconds, standByReplicaKeepDurationSeconds_, (serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL && hasPersistedState_ && ((flags_ & FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION) != 0)))
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::DefaultMoveCost, defaultMoveCost_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::IsDefaultMoveCostSpecified, isDefaultMoveCostSpecified_)
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::ServicePackageActivationMode, servicePackageActivationMode_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceDnsName, serviceDnsName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ScalingPolicies, scalingPolicies_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    protected: // TODO: Are all these below need to be protected??

        Common::ErrorCode TraceAndGetErrorDetails(Common::ErrorCodeValue::Enum errorCode, std::wstring && msg);
        Common::ErrorCode TraceNullArgumentAndGetErrorDetails(std::wstring && msg);

        FABRIC_SERVICE_KIND serviceKind_;
        std::wstring applicationName_;
        std::wstring serviceName_;
        std::wstring serviceTypeName_;
        Common::ByteBuffer initializationData_;
        PartitionDescription partitionDescription_;     
        // stateless
        LONG instanceCount_;
        // stateful
        LONG targetReplicaSetSize_;
        LONG minReplicaSetSize_;
        bool hasPersistedState_;

        FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS flags_;
        DWORD replicaRestartWaitDurationSeconds_;
        DWORD quorumLossWaitDurationSeconds_;
        DWORD standByReplicaKeepDurationSeconds_;

        std::wstring placementConstraints_;
        std::vector<Reliability::ServiceCorrelationDescription> correlations_;
        std::vector<Reliability::ServiceLoadMetricDescription> metrics_;
        std::vector<ServicePlacementPolicyDescription> placementPolicies_;
        FABRIC_MOVE_COST defaultMoveCost_;
        bool isDefaultMoveCostSpecified_;
        bool isServiceGroup_;
        std::wstring serviceDnsName_;
        std::vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies_;

    private:
        ServicePackageActivationMode::Enum servicePackageActivationMode_;

        Common::ErrorCode InitializePartitionDescription(
            ::FABRIC_PARTITION_SCHEME partitionScheme,
            void * partitionDescription, 
            ServiceModel::ServiceTypeIdentifier const & typeIdentifier);
    };
}