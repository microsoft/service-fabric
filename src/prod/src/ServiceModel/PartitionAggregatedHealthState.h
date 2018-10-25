// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class PartitionAggregatedHealthState
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
    public:
        PartitionAggregatedHealthState();

        PartitionAggregatedHealthState(
            Common::Guid const & partitionId,
            FABRIC_HEALTH_STATE aggregatedHealthState);

        PartitionAggregatedHealthState(PartitionAggregatedHealthState const & other) = default;
        PartitionAggregatedHealthState & operator = (PartitionAggregatedHealthState const & other) = default;

        PartitionAggregatedHealthState(PartitionAggregatedHealthState && other) = default;
        PartitionAggregatedHealthState & operator = (PartitionAggregatedHealthState && other) = default;

        ~PartitionAggregatedHealthState();

        __declspec(property(get=get_PartitionId)) Common::Guid const& PartitionId;
        Common::Guid const& get_PartitionId() const { return partitionId_; }
    
        __declspec(property(get=get_AggregatedHealthState)) FABRIC_HEALTH_STATE AggregatedHealthState;
        FABRIC_HEALTH_STATE get_AggregatedHealthState() const { return aggregatedHealthState_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_PARTITION_HEALTH_STATE & publicPartitionAggregatedHealthState) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_PARTITION_HEALTH_STATE const & publicPartitionAggregatedHealthState);
        
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        //
        // IPageContinuationToken methods
        //
        std::wstring CreateContinuationToken() const override;

        FABRIC_FIELDS_02(partitionId_, aggregatedHealthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::PartitionId, partitionId_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::AggregatedHealthState, aggregatedHealthState_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionId_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(aggregatedHealthState_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        Common::Guid partitionId_;
        FABRIC_HEALTH_STATE aggregatedHealthState_;
    };    
}
