// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServicesHealthEvaluation
        : public HealthEvaluationWithChildrenBase
    {
        DENY_COPY(ServicesHealthEvaluation)

    public:
        ServicesHealthEvaluation();

        ServicesHealthEvaluation(
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::wstring const & serviceTypeName,
            HealthEvaluationList && unhealthyEvaluations,
            ULONG totalCount,
            BYTE maxPercentUnhealthyServices);

        ServicesHealthEvaluation(ServicesHealthEvaluation && other) = default;
        ServicesHealthEvaluation & operator = (ServicesHealthEvaluation && other) = default;

        virtual ~ServicesHealthEvaluation();

        __declspec(property(get=get_MaxPercentUnhealthyServices)) BYTE MaxPercentUnhealthyServices;
        BYTE get_MaxPercentUnhealthyServices() const { return maxPercentUnhealthyServices_; }

         __declspec(property(get=get_ServiceTypeName)) std::wstring const & ServiceTypeName;
        std::wstring const & get_ServiceTypeName() const { return serviceTypeName_; }
        
        __declspec(property(get=get_TotalCount)) ULONG TotalCount;
        ULONG get_TotalCount() const { return totalCount_; }

        virtual void SetDescription() override;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;
        
        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);

        FABRIC_FIELDS_07(kind_, description_, serviceTypeName_, unhealthyEvaluations_, maxPercentUnhealthyServices_, totalCount_, aggregatedHealthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::ServiceTypeName, serviceTypeName_)
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvaluations, unhealthyEvaluations_)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentUnhealthyServices, maxPercentUnhealthyServices_)
            SERIALIZABLE_PROPERTY(Constants::TotalCount, totalCount_); 
        END_JSON_SERIALIZABLE_PROPERTIES()     

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceTypeName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(unhealthyEvaluations_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring serviceTypeName_;
        BYTE maxPercentUnhealthyServices_;
        ULONG totalCount_;
    };

    DEFINE_HEALTH_EVALUATION_ACTIVATOR( ServicesHealthEvaluation, FABRIC_HEALTH_EVALUATION_KIND_SERVICES )
}
