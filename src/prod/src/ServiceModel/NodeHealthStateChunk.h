// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NodeHealthStateChunk
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DENY_COPY(NodeHealthStateChunk)

    public:
        NodeHealthStateChunk();

        NodeHealthStateChunk(
            std::wstring const & nodeName,
            FABRIC_HEALTH_STATE healthState);

        NodeHealthStateChunk(NodeHealthStateChunk && other) = default;
        NodeHealthStateChunk & operator = (NodeHealthStateChunk && other) = default;

        virtual ~NodeHealthStateChunk();

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get=get_HealthState)) FABRIC_HEALTH_STATE HealthState;
        FABRIC_HEALTH_STATE get_HealthState() const { return healthState_; }

        Common::ErrorCode FromPublicApi(FABRIC_NODE_HEALTH_STATE_CHUNK const & publicNodeHealthStateChunk);

        Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __inout FABRIC_NODE_HEALTH_STATE_CHUNK & publicNodeHealthStateChunk) const;

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(nodeName_, healthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::NodeName, nodeName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
        END_JSON_SERIALIZABLE_PROPERTIES()
        
        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeName_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(healthState_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring nodeName_;
        FABRIC_HEALTH_STATE healthState_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::NodeHealthStateChunk);

