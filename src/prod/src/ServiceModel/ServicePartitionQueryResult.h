// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServicePartitionQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
        DENY_COPY(ServicePartitionQueryResult)

    public:
        ServicePartitionQueryResult();

        ServicePartitionQueryResult(ServicePartitionQueryResult && other);

        static ServicePartitionQueryResult CreateStatelessServicePartitionQueryResult(
            ServicePartitionInformation const & partitionInformation,
            int instanceCount,
            FABRIC_QUERY_SERVICE_PARTITION_STATUS partitionStatus);

        static ServicePartitionQueryResult CreateStatefulServicePartitionQueryResult(
            ServicePartitionInformation const & partitionInformation,
            uint targetReplicaSetSize,
            uint minReplicaSetSize,
            FABRIC_QUERY_SERVICE_PARTITION_STATUS partitionStatus,
            int64 lastQuorumLossDurationInSeconds,
            const Reliability::Epoch & CurrentConfigurationEpoch);

        __declspec(property(get=get_PartitionId)) Common::Guid const & PartitionId;
        Common::Guid const & get_PartitionId() const { return partitionInformation_.PartitionId; }

        __declspec(property(get=get_TargetReplicaSetSize)) uint TargetReplicaSetSize;
        uint get_TargetReplicaSetSize() const { return targetReplicaSetSize_; }

        __declspec(property(get=get_MinReplicaSetSize)) uint MinReplicaSetSize;
        uint get_MinReplicaSetSize() const { return minReplicaSetSize_; }

        __declspec(property(get=get_InQuorumLoss)) bool InQuorumLoss;
        bool get_InQuorumLoss() const { return partitionStatus_ == FABRIC_QUERY_SERVICE_PARTITION_STATUS_IN_QUORUM_LOSS; }

        __declspec(property(get=get_HealthState,put=put_HealthState)) FABRIC_HEALTH_STATE HealthState;
        FABRIC_HEALTH_STATE get_HealthState() const { return healthState_; }
        void put_HealthState(FABRIC_HEALTH_STATE healthState) { healthState_ = healthState; }

        __declspec(property(get = get_PartitionInformation)) ServicePartitionInformation const & PartitionInformation;
        ServicePartitionInformation const & get_PartitionInformation() const { return partitionInformation_; }

        void SetRenameAsPrimaryEpoch(bool renameAsPrimaryEpoch);

        ServicePartitionQueryResult const & operator = (ServicePartitionQueryResult && other);

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM & publicServicePartitionQueryResult) const ;

        Common::ErrorCode FromPublicApi(__in FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM const& partitionQueryResult);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::ServiceKind, serviceResultKind_)
            SERIALIZABLE_PROPERTY(Constants::PartitionInformation, partitionInformation_)
            SERIALIZABLE_PROPERTY_IF(Constants::InstanceCount, instanceCount_, serviceResultKind_ == FABRIC_SERVICE_KIND_STATELESS)
            SERIALIZABLE_PROPERTY_IF(Constants::TargetReplicaSetSize, targetReplicaSetSize_, serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(Constants::MinReplicaSetSize, minReplicaSetSize_, serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::PartitionStatus, partitionStatus_)
            SERIALIZABLE_PROPERTY_IF(Constants::LastQuorumLossDurationInSeconds, lastQuorumLossDurationInSeconds_, serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(renameAsPrimaryEpoch_? Constants::PrimaryEpoch: Constants::CurrentConfigurationEpoch, currentConfigurationEpoch_, serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
        END_JSON_SERIALIZABLE_PROPERTIES()

        // TODO: DYNAMIC_SIZE_ESTIMATION_MEMBER(currentConfigurationEpoch_) ?
        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_ENUM_ESTIMATION_MEMBER(serviceResultKind_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionInformation_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(healthState_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(partitionStatus_)
        END_DYNAMIC_SIZE_ESTIMATION()

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_09(serviceResultKind_, partitionInformation_, instanceCount_, targetReplicaSetSize_, minReplicaSetSize_, healthState_, partitionStatus_, lastQuorumLossDurationInSeconds_, currentConfigurationEpoch_)

        //
        // IPageContinuationToken methods
        //
        std::wstring CreateContinuationToken() const override;

    private:
        ServicePartitionQueryResult(
            ServicePartitionInformation const & partitionInformation,
            uint targetReplicaSetSize,
            uint minReplicaSetSize,
            FABRIC_QUERY_SERVICE_PARTITION_STATUS partitionStatus,
            int64 lastQuorumLossDurationInSeconds,
            const Reliability::Epoch & currentConfigurationEpoch);

        ServicePartitionQueryResult(
            ServicePartitionInformation const & partitionInformation,
            int instanceCount,
            FABRIC_QUERY_SERVICE_PARTITION_STATUS partitionStatus);

        FABRIC_SERVICE_KIND serviceResultKind_;
        ServicePartitionInformation partitionInformation_;
        uint targetReplicaSetSize_;
        uint minReplicaSetSize_;
        int instanceCount_;
        FABRIC_HEALTH_STATE healthState_;
        FABRIC_QUERY_SERVICE_PARTITION_STATUS partitionStatus_;
        int64 lastQuorumLossDurationInSeconds_;
        Reliability::Epoch currentConfigurationEpoch_;
        bool renameAsPrimaryEpoch_;
    };

    // Used to serialize results in REST
    QUERY_JSON_LIST(PartitionList, ServicePartitionQueryResult)
}
