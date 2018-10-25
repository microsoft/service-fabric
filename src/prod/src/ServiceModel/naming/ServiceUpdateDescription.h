// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceUpdateDescription : public Common::IFabricJsonSerializable, public Serialization::FabricSerializable
    {
        DEFAULT_MOVE_AND_COPY( ServiceUpdateDescription )
        
    public:
        ServiceUpdateDescription();

        // Used only from PLB/FM for auto-scaling
        ServiceUpdateDescription(
            bool isStateful,
            std::vector<std::wstring> && namesToAdd,
            std::vector<std::wstring> && namesToRemove);

        __declspec(property(get = get_ServiceKind)) FABRIC_SERVICE_KIND ServiceKind;
        FABRIC_SERVICE_KIND get_ServiceKind() const { return serviceKind_; }

        __declspec(property(get=get_Flags)) LONG UpdateFlags;
        LONG get_Flags() const { return updateFlags_; }

        __declspec(property(get=get_UpdateTargetReplicaSetSize, put=put_UpdateTargetReplicaSetSize)) bool UpdateTargetReplicaSetSize;
        bool get_UpdateTargetReplicaSetSize() const { return (updateFlags_ & Flags::TargetReplicaSetSize) != 0; }
        void put_UpdateTargetReplicaSetSize(bool const value) { updateFlags_ = value ? (updateFlags_ | Flags::TargetReplicaSetSize) : (updateFlags_ & ~Flags::TargetReplicaSetSize); }

        __declspec(property(get=get_TargetReplicaSetSize, put=put_TargetReplicaSetSize)) int TargetReplicaSetSize;
        int get_TargetReplicaSetSize() const { return targetReplicaSetSize_; }
        void put_TargetReplicaSetSize(int const value) { targetReplicaSetSize_ = value; }

        __declspec(property(get=get_UpdateMinReplicaSetSize, put=put_UpdateMinReplicaSetSize)) bool UpdateMinReplicaSetSize;
        bool get_UpdateMinReplicaSetSize() const { return (updateFlags_ & Flags::MinReplicaSetSize) != 0; }
        void put_UpdateMinReplicaSetSize(bool const value) { updateFlags_ = value ? (updateFlags_ | Flags::MinReplicaSetSize) : (updateFlags_ & ~Flags::MinReplicaSetSize); }

        __declspec(property(get=get_MinReplicaSetSize, put=put_MinReplicaSetSize)) int MinReplicaSetSize;
        int get_MinReplicaSetSize() const { return minReplicaSetSize_; }
        void put_MinReplicaSetSize(int const value) { minReplicaSetSize_ = value; }

        __declspec(property(get=get_UpdateReplicaRestartWaitDuration, put=put_UpdateReplicaRestartWaitDuration)) bool UpdateReplicaRestartWaitDuration;
        bool get_UpdateReplicaRestartWaitDuration() const { return (updateFlags_ & Flags::ReplicaRestartWaitDuration) != 0; }
        void put_UpdateReplicaRestartWaitDuration(bool const value) { updateFlags_ = value ? (updateFlags_ | Flags::ReplicaRestartWaitDuration) : (updateFlags_ & ~Flags::ReplicaRestartWaitDuration); }

        __declspec(property(get=get_ReplicaRestartWaitDuration, put=put_ReplicaRestartWaitDuration)) Common::TimeSpan ReplicaRestartWaitDuration;
        Common::TimeSpan get_ReplicaRestartWaitDuration() const { return replicaRestartWaitDuration_; }
        void put_ReplicaRestartWaitDuration(Common::TimeSpan const & value) { replicaRestartWaitDuration_ = value; }

        __declspec(property(get=get_UpdateQuorumLossWaitDuration, put=put_UpdateQuorumLossWaitDuration)) bool UpdateQuorumLossWaitDuration;
        bool get_UpdateQuorumLossWaitDuration() const { return (updateFlags_ & Flags::QuorumLossWaitDuration) != 0; }
        void put_UpdateQuorumLossWaitDuration(bool const value) { updateFlags_ = value ? (updateFlags_ | Flags::QuorumLossWaitDuration) : (updateFlags_ & ~Flags::QuorumLossWaitDuration); }

        __declspec(property(get=get_QuorumLossWaitDuration, put=put_QuorumLossWaitDuration)) Common::TimeSpan QuorumLossWaitDuration;
        Common::TimeSpan get_QuorumLossWaitDuration() const { return quorumLossWaitDuration_; }
        void put_QuorumLossWaitDuration(Common::TimeSpan const & value) { quorumLossWaitDuration_ = value; }

        __declspec(property(get=get_UpdateStandByReplicaKeepDuration, put=put_UpdateStandByReplicaKeepDuration)) bool UpdateStandByReplicaKeepDuration;
        bool get_UpdateStandByReplicaKeepDuration() const { return (updateFlags_ & Flags::StandByReplicaKeepDuration) != 0; }
        void put_UpdateStandByReplicaKeepDuration(bool const value) { updateFlags_ = value ? (updateFlags_ | Flags::StandByReplicaKeepDuration) : (updateFlags_ & ~Flags::StandByReplicaKeepDuration); }

        __declspec(property(get=get_StandByReplicaKeepDuration, put=put_StandByReplicaKeepDuration)) Common::TimeSpan StandByReplicaKeepDuration;
        Common::TimeSpan get_StandByReplicaKeepDuration() const { return standByReplicaKeepDuration_; }
        void put_StandByReplicaKeepDuration(Common::TimeSpan const & value) { standByReplicaKeepDuration_ = value; }

        __declspec(property(get=get_UpdateCorrelations,put=put_UpdateCorrelations)) bool UpdateCorrelations;
        bool get_UpdateCorrelations() const { return (updateFlags_ & Flags::Correlations) != 0; }
        void put_UpdateCorrelations(bool const value) { updateFlags_ = value ? (updateFlags_ | Flags::Correlations) : (updateFlags_ & ~Flags::Correlations); }

        __declspec (property(get=get_Correlations, put=put_Correlations)) std::vector<Reliability::ServiceCorrelationDescription> const& Correlations;
        std::vector<Reliability::ServiceCorrelationDescription> const& get_Correlations() const { return serviceCorrelations_; }
        void put_Correlations(std::vector<Reliability::ServiceCorrelationDescription> const & value) { serviceCorrelations_ = value; }

        __declspec(property(get=get_UpdatePlacementConstraints, put=put_UpdatePlacementConstraints)) bool UpdatePlacementConstraints;
        bool get_UpdatePlacementConstraints() const { return (updateFlags_ & Flags::PlacementConstraints) != 0; }
        void put_UpdatePlacementConstraints(bool const value) { updateFlags_ = value ? (updateFlags_ | Flags::PlacementConstraints) : (updateFlags_ & ~Flags::PlacementConstraints); }

        __declspec (property(get=get_PlacementConstraints, put=put_PlacementConstraints)) std::wstring const& PlacementConstraints;
        std::wstring const& get_PlacementConstraints() const { return placementConstraints_; }
        void put_PlacementConstraints(std::wstring const & value) { placementConstraints_ = value; }

        __declspec(property(get=get_UpdateMetrics, put=put_UpdateMetrics)) bool UpdateMetrics;
        bool get_UpdateMetrics() const { return (updateFlags_ & Flags::Metrics) != 0; }
        void put_UpdateMetrics(bool const value) { updateFlags_ = value ? (updateFlags_ | Flags::Metrics) : (updateFlags_ & ~Flags::Metrics); }

        __declspec (property(get=get_Metrics, put=put_Metrics)) std::vector<Reliability::ServiceLoadMetricDescription> const & Metrics;
        std::vector<Reliability::ServiceLoadMetricDescription> const & get_Metrics() const  { return metrics_; }
        void put_Metrics(std::vector<Reliability::ServiceLoadMetricDescription> const & value) { metrics_ = value; }

        __declspec(property(get=get_UpdatePlacementPolicies, put=put_UpdatePlacementPolicies)) bool UpdatePlacementPolicies;
        bool get_UpdatePlacementPolicies() const { return (updateFlags_ & Flags::PlacementPolicyList) != 0; }
        void put_UpdatePlacementPolicies(bool const value) { updateFlags_ = value ? (updateFlags_ | Flags::PlacementPolicyList) : (updateFlags_ & ~Flags::PlacementPolicyList); }

        __declspec(property(get=get_PlacementPolicies, put=put_PlacementPolicies)) std::vector<ServiceModel::ServicePlacementPolicyDescription>  const & PlacementPolicies;
        std::vector<ServiceModel::ServicePlacementPolicyDescription>  const & get_PlacementPolicies() const { return placementPolicies_; }
        void put_PlacementPolicies(std::vector<ServiceModel::ServicePlacementPolicyDescription> const & value) { placementPolicies_ = value; }

        __declspec(property(get=get_UpdateDefaultMoveCost, put=put_UpdateDefaultMoveCost)) bool UpdateDefaultMoveCost;
        bool get_UpdateDefaultMoveCost() const { return (updateFlags_ & Flags::DefaultMoveCost) != 0; }
        void put_UpdateDefaultMoveCost(bool const value) { updateFlags_ = value ? (updateFlags_ | Flags::DefaultMoveCost) : (updateFlags_ & ~Flags::DefaultMoveCost); }

        __declspec(property(get=get_DefaultMoveCost, put=put_DefaultMoveCost)) FABRIC_MOVE_COST DefaultMoveCost;
        FABRIC_MOVE_COST get_DefaultMoveCost() const { return defaultMoveCost_; }
        void put_DefaultMoveCost(FABRIC_MOVE_COST value) { defaultMoveCost_ = value; }

        __declspec(property(get=get_RepartitionDescription, put=put_RepartitionDescription)) std::shared_ptr<RepartitionDescription> const & Repartition;
        std::shared_ptr<RepartitionDescription> const & get_RepartitionDescription() const { return repartitionDescription_; }
        void put_RepartitionDescription(std::shared_ptr<RepartitionDescription> const & value) { repartitionDescription_ = value; }

        __declspec(property(get = get_UpdateScalingPolicy, put = put_UpdateScalingPolicy)) bool UpdateScalingPolicy;
        bool get_UpdateScalingPolicy() const { return (updateFlags_ & Flags::ScalingPolicy) != 0; }
        void put_UpdateScalingPolicy(bool const value) { updateFlags_ = value ? (updateFlags_ | Flags::ScalingPolicy) : (updateFlags_ & ~Flags::ScalingPolicy); }

        __declspec(property(get = get_ScalingPolicyWrapper, put = put_ScalingPolicyWrapper)) std::vector<Reliability::ServiceScalingPolicyDescription> const & ScalingPolicies;
        std::vector<Reliability::ServiceScalingPolicyDescription> const & get_ScalingPolicyWrapper() const { return scalingPolicies_; }
        void put_ScalingPolicyWrapper(std::vector<Reliability::ServiceScalingPolicyDescription> const & value) { scalingPolicies_ = value; }

        __declspec(property(get=get_InitializationData, put=put_InitializationData)) std::shared_ptr<std::vector<byte>> const & InitializationData;
        std::shared_ptr<std::vector<byte>> const & get_InitializationData() const { return initializationData_; }
        void put_InitializationData(std::shared_ptr<std::vector<byte>> && value) { initializationData_ = std::move(value); }

        Common::ErrorCode FromPublicApi(FABRIC_SERVICE_UPDATE_DESCRIPTION const & updateDescription);
        Common::ErrorCode FromRepartitionDescription(FABRIC_SERVICE_PARTITION_KIND const publicKind, void * publicDescription);

        Common::ErrorCode TryUpdateServiceDescription(
            __inout Reliability::ServiceDescription &,
            __out bool & isUpdated) const;

        Common::ErrorCode TryUpdateServiceDescription(
            __inout PartitionedServiceDescriptor & psd, 
            __out bool & isUpdated,
            __out std::vector<Reliability::ConsistencyUnitDescription> & addedCuids,
            __out std::vector<Reliability::ConsistencyUnitDescription> & removedCuids) const;

        static Common::ErrorCode TryDiffForUpgrade(
            __in Naming::PartitionedServiceDescriptor & targetPsd,
            __in Naming::PartitionedServiceDescriptor & activePsd,
            __out std::shared_ptr<ServiceUpdateDescription> & updateDescriptionResult,
            __out std::shared_ptr<ServiceUpdateDescription> & rollbackUpdateDescriptionResult);

        Common::ErrorCode IsValid() const;

        void WriteToEtw(uint16 contextSequenceId) const;
        void WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const&) const;

        FABRIC_FIELDS_15(
            updateFlags_, 
            targetReplicaSetSize_, 
            replicaRestartWaitDuration_, 
            quorumLossWaitDuration_, 
            standByReplicaKeepDuration_, 
            minReplicaSetSize_, 
            serviceCorrelations_,
            placementConstraints_, 
            metrics_, 
            placementPolicies_, 
            serviceKind_, 
            defaultMoveCost_,
            repartitionDescription_,
            scalingPolicies_,
            initializationData_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::ServiceKind, serviceKind_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Flags, updateFlags_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::LoadMetrics, metrics_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PlacementConstraints, placementConstraints_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::CorrelationScheme, serviceCorrelations_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServicePlacementPolicies, placementPolicies_)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::InstanceCount, targetReplicaSetSize_, serviceKind_ == FABRIC_SERVICE_KIND_STATELESS)

            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::TargetReplicaSetSize, targetReplicaSetSize_, serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::MinReplicaSetSize, minReplicaSetSize_, serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::ReplicaRestartWaitDurationInMilliseconds, replicaRestartWaitDuration_, serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::QuorumLossWaitDurationInMilliseconds, quorumLossWaitDuration_, serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::StandByReplicaKeepDurationInMilliseconds, standByReplicaKeepDuration_, serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL)

            SERIALIZABLE_PROPERTY(ServiceModel::Constants::RepartitionDescription, repartitionDescription_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ScalingPolicies, scalingPolicies_)
            //
            // Updating initialization data is currently not exposed via the public API
            //
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        struct Flags
        {
            static const LONG None = 0;
            static const LONG TargetReplicaSetSize = 0x1;
            static const LONG ReplicaRestartWaitDuration = 0x2;
            static const LONG QuorumLossWaitDuration = 0x4;
            static const LONG StandByReplicaKeepDuration = 0x8;
            static const LONG MinReplicaSetSize = 0x10;
            static const LONG PlacementConstraints = 0x20;
            static const LONG PlacementPolicyList = 0x40;
            static const LONG Correlations = 0x80;
            static const LONG Metrics = 0x100;
            static const LONG DefaultMoveCost = 0x200;
            static const LONG ScalingPolicy = 0x400;

            static const LONG ValidServiceUpdateFlags = TargetReplicaSetSize | 
                                                        MinReplicaSetSize |
                                                        ReplicaRestartWaitDuration |
                                                        QuorumLossWaitDuration |
                                                        StandByReplicaKeepDuration |
                                                        PlacementConstraints |
                                                        PlacementPolicyList |
                                                        Correlations |
                                                        Metrics |
                                                        DefaultMoveCost |
                                                        ScalingPolicy;
        };

        Common::ErrorCode TryUpdateServiceDescription(
            __inout Reliability::ServiceDescription &,
            bool allowRepartition,
            __out bool & isUpdated) const;

        Common::ErrorCode TraceAndGetInvalidArgumentError(std::wstring && msg) const;

        FABRIC_SERVICE_KIND serviceKind_;

        LONG updateFlags_;
        int targetReplicaSetSize_;
        int minReplicaSetSize_;
        Common::TimeSpan replicaRestartWaitDuration_;
        Common::TimeSpan quorumLossWaitDuration_;
        Common::TimeSpan standByReplicaKeepDuration_;

        std::vector<Reliability::ServiceCorrelationDescription> serviceCorrelations_;
        std::wstring placementConstraints_;
        std::vector<Reliability::ServiceLoadMetricDescription> metrics_;
        std::vector<ServiceModel::ServicePlacementPolicyDescription> placementPolicies_;
        FABRIC_MOVE_COST defaultMoveCost_;

        std::shared_ptr<RepartitionDescription> repartitionDescription_;
        std::vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies_;
        std::shared_ptr<std::vector<byte>> initializationData_;
    };
}
