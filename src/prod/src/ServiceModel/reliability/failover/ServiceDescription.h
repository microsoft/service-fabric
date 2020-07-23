// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceDescription : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(ServiceDescription)

    public:
        ServiceDescription();

        ServiceDescription(
            ServiceDescription const & other,
            std::wstring && nameOverride);

        ServiceDescription(
            ServiceDescription const & other,
            std::vector<byte> && initializationDataOverride);

        ServiceDescription(
            ServiceDescription const & other,
            std::wstring && nameOverride,
            ServiceModel::ServiceTypeIdentifier && typeOverride,
            std::vector<byte> && initializationDataOverride,
            std::vector<ServiceLoadMetricDescription> && loadMetricOverride,
            uint defaultMoveCostOverride);

        ServiceDescription(
            ServiceDescription const & other,
            ServiceModel::ServicePackageVersionInstance const & versionOverride);

        ServiceDescription(
            std::wstring const& name,
            uint64 instance,
            uint64 updateVersion,
            int partitionCount,
            int targetReplicaSetSize,
            int minReplicaSetSize,
            bool isStateful,
            bool hasPersistedState,
            Common::TimeSpan replicaRestartWaitDuration,
            Common::TimeSpan quorumLossWaitDuration,
            Common::TimeSpan standByReplicaKeepDuration,
            ServiceModel::ServiceTypeIdentifier const& type,
            std::vector<ServiceCorrelationDescription> const& serviceCorrelations,
            std::wstring const& placementConstraints,
            int scaleoutCount,
            std::vector<ServiceLoadMetricDescription> const& metrics,
            uint defaultMoveCost,
            std::vector<byte> const & initializationData,
            std::wstring const & applicationName = L"",
            std::vector<ServiceModel::ServicePlacementPolicyDescription> const& placementPolicies = std::vector<ServiceModel::ServicePlacementPolicyDescription>(),
            ServiceModel::ServicePackageActivationMode::Enum servicePackageActivationMode = ServiceModel::ServicePackageActivationMode::SharedProcess,
            std::wstring const& serviceDnsName = L"",
            std::vector<Reliability::ServiceScalingPolicyDescription> const & scalingPolicies = std::vector<Reliability::ServiceScalingPolicyDescription>()
           );
        
        ServiceDescription & operator = (ServiceDescription const& other);
        ServiceDescription & operator = (ServiceDescription && other);

        bool operator == (ServiceDescription const & other) const;
        bool operator != (ServiceDescription const & other) const;

        Common::ErrorCode Equals(ServiceDescription const& other) const;

        // properties
        __declspec (property(get = get_Name, put = put_Name)) std::wstring const& Name;
        std::wstring const& get_Name() const { return name_; }
        void put_Name(std::wstring && value) { name_ = std::move(value); }

        __declspec (property(get = get_Instance, put = put_Instance)) uint64 Instance;
        uint64 get_Instance() const { return instance_; }
        void put_Instance(uint64 value) { instance_ = value; }

        __declspec (property(get = get_UpdateVersion, put = put_UpdateVersion)) uint64 UpdateVersion;
        uint64 get_UpdateVersion() const { return updateVersion_; }
        void put_UpdateVersion(uint64 value) { updateVersion_ = value; }

        __declspec (property(get = get_Type, put = put_Type)) ServiceModel::ServiceTypeIdentifier const& Type;
        ServiceModel::ServiceTypeIdentifier const& get_Type() const { return type_; }
        void put_Type(ServiceModel::ServiceTypeIdentifier && value) { type_ = std::move(value); }

        __declspec (property(get = get_ApplicationName, put = put_ApplicationName)) std::wstring const& ApplicationName;
        std::wstring const& get_ApplicationName() const { return applicationName_; }
        void put_ApplicationName(std::wstring && value) { applicationName_ = std::move(value); }

        __declspec (property(get = get_ApplicationId, put = put_ApplicationId)) ServiceModel::ApplicationIdentifier const& ApplicationId;
        ServiceModel::ApplicationIdentifier const& get_ApplicationId() const { return type_.ApplicationId; }
        void put_ApplicationId(ServiceModel::ApplicationIdentifier const& value) { type_ = ServiceModel::ServiceTypeIdentifier(ServiceModel::ServicePackageIdentifier(value, type_.ServicePackageId.ServicePackageName), type_.ServiceTypeName); }

        __declspec (property(get = get_PackageVersion)) ServiceModel::ServicePackageVersion const & PackageVersion;
        ServiceModel::ServicePackageVersion const & get_PackageVersion() const { return packageVersionInstance_.Version; }

        __declspec (property(get = get_PackageVersionInstance, put = put_PackageVersionInstance)) ServiceModel::ServicePackageVersionInstance & PackageVersionInstance;
        ServiceModel::ServicePackageVersionInstance const & get_PackageVersionInstance() const { return packageVersionInstance_; }
        void put_PackageVersionInstance(ServiceModel::ServicePackageVersionInstance const& value) { packageVersionInstance_ = value; }

        __declspec (property(get = get_PartitionCount, put = put_PartitionCount)) int PartitionCount;
        int get_PartitionCount() const { return partitionCount_; }
        void put_PartitionCount(int value) { partitionCount_ = value; }

        __declspec (property(get = get_TargetReplicaSetSize, put = put_TargetReplicaSetSize)) int TargetReplicaSetSize;
        int get_TargetReplicaSetSize() const { return targetReplicaSetSize_; }
        void put_TargetReplicaSetSize(int value) { targetReplicaSetSize_ = value; }

        __declspec (property(get = get_MinReplicaSetSize, put = put_MinReplicaSetSize)) int MinReplicaSetSize;
        int get_MinReplicaSetSize() const { return minReplicaSetSize_; }
        void put_MinReplicaSetSize(int value) { minReplicaSetSize_ = value; }

        __declspec (property(get = get_IsStateful, put = put_IsStateful)) bool IsStateful;
        bool get_IsStateful() const { return isStateful_; }
        void put_IsStateful(bool value) { isStateful_ = value; }

        __declspec (property(get = get_IsVolatile)) bool IsVolatile;
        bool get_IsVolatile() const { return isStateful_ && !hasPersistedState_; }

        __declspec (property(get = get_HasPersistedState, put = put_HasPersistedState)) bool HasPersistedState;
        bool get_HasPersistedState() const { return hasPersistedState_; }
        void put_HasPersistedState(bool value) { hasPersistedState_ = value; }

        __declspec (property(get = get_ReplicaRestartWaitDuration, put = put_ReplicaRestartWaitDuration)) Common::TimeSpan ReplicaRestartWaitDuration;
        Common::TimeSpan get_ReplicaRestartWaitDuration() const;
        void put_ReplicaRestartWaitDuration(Common::TimeSpan value) { replicaRestartWaitDuration_ = value; }

        __declspec (property(get = get_QuorumLossWaitDuration, put = put_QuorumLossWaitDuration)) Common::TimeSpan QuorumLossWaitDuration;
        Common::TimeSpan get_QuorumLossWaitDuration() const;
        void put_QuorumLossWaitDuration(Common::TimeSpan value) { quorumLossWaitDuration_ = value; }

        __declspec (property(get = get_StandByReplicaKeepDuration, put = put_StandByReplicaKeepDuration)) Common::TimeSpan StandByReplicaKeepDuration;
        Common::TimeSpan get_StandByReplicaKeepDuration() const;
        void put_StandByReplicaKeepDuration(Common::TimeSpan value) { standByReplicaKeepDuration_ = value; }

        __declspec (property(get = get_InitializationData, put = put_InitializationData)) std::vector<byte> const & InitializationData;
        std::vector<byte> const & get_InitializationData() const { return initializationData_; }
        std::vector<byte> & get_MutableInitializationData() { return initializationData_; }
        void put_InitializationData(std::vector<byte> && value) { initializationData_ = std::move(value); }

        __declspec(property(get = get_IsServiceGroup, put = set_IsServiceGroup)) bool IsServiceGroup;
        bool get_IsServiceGroup() const { return isServiceGroup_; }
        void set_IsServiceGroup(bool value) { isServiceGroup_ = value; }

        __declspec (property(get = get_Correlations, put = put_Correlations)) std::vector<ServiceCorrelationDescription> const& Correlations;
        std::vector<ServiceCorrelationDescription> const& get_Correlations() const { return serviceCorrelations_; }
        void put_Correlations(std::vector<ServiceCorrelationDescription> && value) { serviceCorrelations_ = std::move(value); }

        __declspec (property(get = get_PlacementConstraints, put = put_PlacementConstraints)) std::wstring const& PlacementConstraints;
        std::wstring const& get_PlacementConstraints() const { return placementConstraints_; }
        void put_PlacementConstraints(std::wstring && value) { placementConstraints_ = std::move(value); }

        __declspec (property(get = get_ScaleoutCount)) int ScaleoutCount;
        int get_ScaleoutCount() const { return scaleoutCount_; }

        __declspec (property(get = get_Metrics, put = put_Metrics)) std::vector<ServiceLoadMetricDescription> const & Metrics;
        std::vector<ServiceLoadMetricDescription> const & get_Metrics() const { return metrics_; }
        void put_Metrics(std::vector<ServiceLoadMetricDescription> && value) { metrics_ = std::move(value); }

        __declspec (property(get = get_DefaultMoveCost, put = put_DefaultMoveCost)) uint DefaultMoveCost;
        uint get_DefaultMoveCost() const { return defaultMoveCost_; }
        void put_DefaultMoveCost(uint value) { defaultMoveCost_ = value; }

        __declspec(property(get = get_PlacementPolicies, put = put_PlacementPolicies)) std::vector<ServiceModel::ServicePlacementPolicyDescription>  const & PlacementPolicies;
        std::vector<ServiceModel::ServicePlacementPolicyDescription>  const & get_PlacementPolicies() const { return placementPolicies_; }
        void put_PlacementPolicies(std::vector<ServiceModel::ServicePlacementPolicyDescription> && value) { placementPolicies_ = std::move(value); }

        __declspec (property(get = get_ServiceDnsName, put = put_ServiceDnsName)) std::wstring const& ServiceDnsName;
        std::wstring const& get_ServiceDnsName() const { return serviceDnsName_; }
        void put_ServiceDnsName(std::wstring const & value);

        __declspec (property(get = get_PackageActivationMode, put = put_PackageActivationMode)) ServiceModel::ServicePackageActivationMode::Enum PackageActivationMode;
        ServiceModel::ServicePackageActivationMode::Enum get_PackageActivationMode() const { return servicePackageActivationMode_; }
        void put_PackageActivationMode(ServiceModel::ServicePackageActivationMode::Enum value) { servicePackageActivationMode_ = value; }

        __declspec (property(get = get_ScalingPolicies, put = put_ScalingPolicies)) std::vector<Reliability::ServiceScalingPolicyDescription> const & ScalingPolicies;
         std::vector<Reliability::ServiceScalingPolicyDescription> const& get_ScalingPolicies() const { return scalingPolicies_; }
        void put_ScalingPolicies(std::vector<Reliability::ServiceScalingPolicyDescription> const & value) { scalingPolicies_ = value; }

        ServiceModel::ServicePackageActivationContext CreateActivationContext(Common::Guid const & partitionId) const
        {
            switch (servicePackageActivationMode_)
            {
            case ServiceModel::ServicePackageActivationMode::SharedProcess:
                return ServiceModel::ServicePackageActivationContext();
            case ServiceModel::ServicePackageActivationMode::ExclusiveProcess:
                return ServiceModel::ServicePackageActivationContext(partitionId);
            default:
                Common::Assert::CodingError("Unknown ServicePackageActivationMode: {0}", static_cast<int>(servicePackageActivationMode_));
            }
        }

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        void AddDefaultMetrics();

        FABRIC_FIELDS_25(
            name_, instance_, type_, applicationName_, packageVersionInstance_,
            partitionCount_, targetReplicaSetSize_,
            minReplicaSetSize_, isStateful_, hasPersistedState_, initializationData_, isServiceGroup_,
            serviceCorrelations_, placementConstraints_, scaleoutCount_, metrics_, defaultMoveCost_,
            replicaRestartWaitDuration_, quorumLossWaitDuration_, updateVersion_, placementPolicies_,
            standByReplicaKeepDuration_, servicePackageActivationMode_, serviceDnsName_, scalingPolicies_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(type_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            // packageVersionInstance_ is composed of ULONGs
            DYNAMIC_SIZE_ESTIMATION_MEMBER(replicaRestartWaitDuration_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(quorumLossWaitDuration_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(standByReplicaKeepDuration_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(initializationData_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceCorrelations_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(placementConstraints_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(metrics_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(placementPolicies_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceDnsName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(scalingPolicies_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:

        std::wstring name_;
        uint64 instance_;
        uint64 updateVersion_;
        ServiceModel::ServiceTypeIdentifier type_;
        std::wstring applicationName_;
        ServiceModel::ServicePackageVersionInstance packageVersionInstance_;

        int partitionCount_;
        int targetReplicaSetSize_;
        int minReplicaSetSize_;
        bool isStateful_;
        bool hasPersistedState_;
        Common::TimeSpan replicaRestartWaitDuration_;
        Common::TimeSpan quorumLossWaitDuration_;
        Common::TimeSpan standByReplicaKeepDuration_;

        std::vector<byte> initializationData_;
        bool isServiceGroup_;

        std::vector<ServiceCorrelationDescription> serviceCorrelations_;
        std::wstring placementConstraints_;
        int scaleoutCount_;
        std::vector<ServiceLoadMetricDescription> metrics_;
        uint defaultMoveCost_;
        std::vector<ServiceModel::ServicePlacementPolicyDescription> placementPolicies_;
        ServiceModel::ServicePackageActivationMode::Enum servicePackageActivationMode_;
        std::wstring serviceDnsName_;
        std::vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies_;
    };
}