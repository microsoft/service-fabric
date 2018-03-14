// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServiceHealthEvaluation
        : public HealthEvaluationWithChildrenBase
    {
        DENY_COPY(ServiceHealthEvaluation)

    public:
        ServiceHealthEvaluation();

        ServiceHealthEvaluation(
            std::wstring const & serviceName,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            HealthEvaluationList && evaluations);

        ServiceHealthEvaluation(ServiceHealthEvaluation && other) = default;
        ServiceHealthEvaluation & operator = (ServiceHealthEvaluation && other) = default;

        virtual ~ServiceHealthEvaluation();

        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() const { return serviceName_; }
    
        virtual void SetDescription() override;
    
        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;
        
        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);

        FABRIC_FIELDS_05(kind_, description_, unhealthyEvaluations_, aggregatedHealthState_, serviceName_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::ServiceName, serviceName_); 
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvaluations, unhealthyEvaluations_)
        END_JSON_SERIALIZABLE_PROPERTIES()     

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(unhealthyEvaluations_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring serviceName_;
    };

    DEFINE_HEALTH_EVALUATION_ACTIVATOR( ServiceHealthEvaluation, FABRIC_HEALTH_EVALUATION_KIND_SERVICE )
}
