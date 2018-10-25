// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class NodeAggregatedHealthState
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
    public:
        NodeAggregatedHealthState();

        NodeAggregatedHealthState(
            std::wstring const & nodeName,
            Federation::NodeId const & nodeId,
            FABRIC_HEALTH_STATE aggregatedHealthState);

        NodeAggregatedHealthState(NodeAggregatedHealthState const & other) = default;
        NodeAggregatedHealthState & operator = (NodeAggregatedHealthState const & other) = default;

        NodeAggregatedHealthState(NodeAggregatedHealthState && other) = default;
        NodeAggregatedHealthState & operator = (NodeAggregatedHealthState && other) = default;

        ~NodeAggregatedHealthState();

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get=get_NodeId)) Federation::NodeId const & NodeId;
        Federation::NodeId const & get_NodeId() const { return nodeId_; }
    
        __declspec(property(get=get_AggregatedHealthState)) FABRIC_HEALTH_STATE AggregatedHealthState;
        FABRIC_HEALTH_STATE get_AggregatedHealthState() const { return aggregatedHealthState_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_NODE_HEALTH_STATE & publicNodeAggregatedHealthState) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_NODE_HEALTH_STATE const & publicNodeAggregatedHealthState);
       
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        //
        // IPageContinuationToken methods
        //
        std::wstring CreateContinuationToken() const override;

        FABRIC_FIELDS_03(nodeName_, nodeId_, aggregatedHealthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, nodeName_)
            SERIALIZABLE_PROPERTY(Constants::Id, nodeId_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::AggregatedHealthState, aggregatedHealthState_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeName_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(aggregatedHealthState_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring nodeName_;
        Federation::NodeId nodeId_;
        FABRIC_HEALTH_STATE aggregatedHealthState_;
    };

    using NodeAggregatedHealthStateList = std::vector<NodeAggregatedHealthState>;
}
