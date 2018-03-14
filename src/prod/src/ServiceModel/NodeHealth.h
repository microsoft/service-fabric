// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class NodeHealth
        : public EntityHealthBase
    {
        DENY_COPY(NodeHealth)
    public:
        NodeHealth();

        NodeHealth(
            std::wstring const & nodeName,
            std::vector<HealthEvent> && events,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::vector<HealthEvaluation> && unhealthyEvaluations);

        NodeHealth(NodeHealth && other) = default;
        NodeHealth & operator = (NodeHealth && other) = default;

        virtual ~NodeHealth();

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }
    
        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_NODE_HEALTH & publicNodeHealth) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_NODE_HEALTH const & publicNodeHealth);
        
        FABRIC_FIELDS_04(nodeName_, events_, aggregatedHealthState_, unhealthyEvaluations_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::Name, nodeName_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring nodeName_;
    };    
}
