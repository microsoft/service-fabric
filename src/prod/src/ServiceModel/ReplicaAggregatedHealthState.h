// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ReplicaAggregatedHealthState
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {        
    public:
        ReplicaAggregatedHealthState();

        ReplicaAggregatedHealthState(
            FABRIC_SERVICE_KIND kind,
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            FABRIC_HEALTH_STATE aggregatedHealthState);

        ReplicaAggregatedHealthState(ReplicaAggregatedHealthState const & other) = default;
        ReplicaAggregatedHealthState & operator = (ReplicaAggregatedHealthState const & other) = default;

        ReplicaAggregatedHealthState(ReplicaAggregatedHealthState && other) = default;
        ReplicaAggregatedHealthState & operator = (ReplicaAggregatedHealthState && other) = default;

        ~ReplicaAggregatedHealthState();

        __declspec(property(get=get_Kind)) FABRIC_SERVICE_KIND  Kind;
        FABRIC_SERVICE_KIND get_Kind() const { return kind_; }

        __declspec(property(get=get_PartitionId)) Common::Guid const& PartitionId;
        Common::Guid const& get_PartitionId() const { return partitionId_; }

        __declspec(property(get=get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
        FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }
            
        __declspec(property(get=get_AggregatedHealthState)) FABRIC_HEALTH_STATE AggregatedHealthState;
        FABRIC_HEALTH_STATE get_AggregatedHealthState() const { return aggregatedHealthState_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_REPLICA_HEALTH_STATE & publicReplicaHealth) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_REPLICA_HEALTH_STATE const & publicReplicaHealth);
        
        static Common::ErrorCode FromPublicApiList(
            FABRIC_REPLICA_HEALTH_STATE_LIST const & publicReplicaHealthList,
            __out std::vector<ReplicaAggregatedHealthState> & replicasAggregatedHealthStates);
        
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        //
        // IPageContinuationToken methods
        //
        std::wstring CreateContinuationToken() const override;

        FABRIC_FIELDS_04(kind_, partitionId_, replicaId_, aggregatedHealthState_);
        
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::ServiceKind, kind_)
            SERIALIZABLE_PROPERTY(Constants::PartitionId, partitionId_)
            SERIALIZABLE_PROPERTY(Constants::ReplicaId, replicaId_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::AggregatedHealthState, aggregatedHealthState_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_ENUM_ESTIMATION_MEMBER(kind_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionId_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(aggregatedHealthState_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        FABRIC_SERVICE_KIND kind_;
        Common::Guid partitionId_;
        FABRIC_REPLICA_ID replicaId_;
        FABRIC_HEALTH_STATE aggregatedHealthState_;
    };    
}
