// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class NodeEntityHealthInformation
        : public EntityHealthInformation
    {
        DENY_COPY(NodeEntityHealthInformation)

    public:
        NodeEntityHealthInformation();

        NodeEntityHealthInformation(
            Common::LargeInteger const& nodeId,
            std::wstring const& nodeName,
            FABRIC_NODE_INSTANCE_ID nodeInstanceId);

        NodeEntityHealthInformation(NodeEntityHealthInformation && other) = default;
        NodeEntityHealthInformation & operator = (NodeEntityHealthInformation && other) = default;

        __declspec(property(get=get_NodeId)) Common::LargeInteger const& NodeId;
        Common::LargeInteger const& get_NodeId() const { return nodeId_; }
        
        __declspec(property(get=get_NodeName)) std::wstring const& NodeName;
        std::wstring const& get_NodeName() const { return nodeName_; }
                
        __declspec(property(get=get_NodeInstanceId)) FABRIC_NODE_INSTANCE_ID  NodeInstanceId;
        FABRIC_NODE_INSTANCE_ID get_NodeInstanceId() const { return nodeInstanceId_; }
        
        std::wstring const & get_EntityId() const;
        DEFINE_NODE_INSTANCE_FIELD(nodeInstanceId_)

        Common::ErrorCode GenerateNodeId();

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
            __out FABRIC_HEALTH_REPORT & healthReport) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_REPORT const & healthReport,
            __inout HealthInformation & commonHealthInformation,
            __out AttributeList & attributes);

        FABRIC_FIELDS_04(kind_, nodeId_, nodeName_, nodeInstanceId_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeId_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeName_)
        END_DYNAMIC_SIZE_ESTIMATION()
        
    private:
        Common::LargeInteger nodeId_;
        std::wstring nodeName_;
        FABRIC_NODE_INSTANCE_ID nodeInstanceId_;
    };

    DEFINE_HEALTH_ENTITY_ACTIVATOR(NodeEntityHealthInformation, FABRIC_HEALTH_REPORT_KIND_NODE)
}
