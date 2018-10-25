// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServiceReplicaQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
    DEFAULT_COPY_CONSTRUCTOR(ServiceReplicaQueryResult)

    public:
        ServiceReplicaQueryResult();

        ServiceReplicaQueryResult(ServiceReplicaQueryResult && other);

        static ServiceReplicaQueryResult CreateStatelessServiceInstanceQueryResult(
            int64 instanceId, 
            FABRIC_QUERY_SERVICE_REPLICA_STATUS status,
            std::wstring const & address,
            std::wstring const & nodeName,
            int64 lastInBuildDurationInSeconds);

        static ServiceReplicaQueryResult CreateStatefulServiceReplicaQueryResult(
            int64 replicaId, 
            FABRIC_REPLICA_ROLE replicaRole, 
            FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus,
            std::wstring const & address,
            std::wstring const & nodeName,
            int64 lastInBuildDurationInSeconds);

        ServiceReplicaQueryResult const & operator = (ServiceReplicaQueryResult && other);

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM & publicServiceReplicaQueryResult) const ;

        Common::ErrorCode FromPublicApi(__in FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM const& replicaQueryResult);

        __declspec(property(get=get_Kind)) FABRIC_SERVICE_KIND Kind;
        FABRIC_SERVICE_KIND get_Kind() const { return serviceResultKind_; }

        __declspec(property(get=get_ReplicaId)) int64 ReplicaId;
        int64 get_ReplicaId() const { return replicaId_; }

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get=get_ReplicaRole)) FABRIC_REPLICA_ROLE ReplicaRole;
        FABRIC_REPLICA_ROLE get_ReplicaRole() const { return replicaRole_; }

        __declspec(property(get=get_InstanceId)) int64 InstanceId;
        int64 get_InstanceId() const { return instanceId_; }

        __declspec(property(get=get_ReplicaOrInstanceId)) int64 ReplicaOrInstanceId;
        int64 get_ReplicaOrInstanceId() const { return (serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL) ? replicaId_ : instanceId_; }

        __declspec(property(get=get_HealthState,put=put_HealthState)) FABRIC_HEALTH_STATE HealthState;
        void put_HealthState(FABRIC_HEALTH_STATE healthState) { healthState_ = healthState; }
        FABRIC_HEALTH_STATE get_HealthState() const { return healthState_; }

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::ServiceKind, serviceResultKind_)
            SERIALIZABLE_PROPERTY_IF(Constants::ReplicaId, replicaId_, serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_ENUM_IF(Constants::ReplicaRole, replicaRole_, serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(Constants::InstanceId, instanceId_, serviceResultKind_ == FABRIC_SERVICE_KIND_STATELESS)
            SERIALIZABLE_PROPERTY_ENUM(Constants::ReplicaStatus, replicaStatus_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
            SERIALIZABLE_PROPERTY(Constants::Address, replicaAddress_)
            SERIALIZABLE_PROPERTY(Constants::NodeName, nodeName_)
            SERIALIZABLE_PROPERTY(Constants::LastInBuildDurationInSeconds, lastInBuildDurationInSeconds_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_ENUM_ESTIMATION_MEMBER(serviceResultKind_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(replicaStatus_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(replicaAddress_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeName_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(replicaRole_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(healthState_)
        END_DYNAMIC_SIZE_ESTIMATION()

        FABRIC_FIELDS_09(
            serviceResultKind_,
            replicaId_,
            replicaRole_,
            instanceId_,
            replicaStatus_,
            replicaAddress_,
            nodeName_,
            healthState_,
            lastInBuildDurationInSeconds_)

        //
        // IPageContinuationToken methods
        //
        std::wstring CreateContinuationToken() const override;

    private:
        ServiceReplicaQueryResult(
            int64 replicaId, 
            FABRIC_REPLICA_ROLE replicaRole, 
            FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus,
            std::wstring const & address,
            std::wstring const & nodeName,
            int64 lastInBuildDurationInSeconds);

        ServiceReplicaQueryResult(
            int64 instanceId, 
            FABRIC_QUERY_SERVICE_REPLICA_STATUS status,
            std::wstring const & address,
            std::wstring const & nodeName,
            int64 lastInBuildDurationInSeconds);

        FABRIC_SERVICE_KIND serviceResultKind_;
        FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus_;
        std::wstring replicaAddress_;
        std::wstring nodeName_;
        int64 replicaId_;
        FABRIC_REPLICA_ROLE replicaRole_;
        int64 instanceId_;
        FABRIC_HEALTH_STATE healthState_;
        int64 lastInBuildDurationInSeconds_;
    };

    // Used to serialize results in REST
    QUERY_JSON_LIST(ReplicaList, ServiceReplicaQueryResult)
};
