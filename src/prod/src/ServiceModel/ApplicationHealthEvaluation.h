// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ApplicationHealthEvaluation
        : public HealthEvaluationWithChildrenBase
    {
        DENY_COPY(ApplicationHealthEvaluation)

    public:
        ApplicationHealthEvaluation();

        ApplicationHealthEvaluation(
            std::wstring const & applicationName,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            HealthEvaluationList && evaluations);

        ApplicationHealthEvaluation(ApplicationHealthEvaluation && other) = default;
        ApplicationHealthEvaluation & operator = (ApplicationHealthEvaluation && other) = default;

        virtual ~ApplicationHealthEvaluation();

        __declspec(property(get=get_ApplicationName)) std::wstring const & ApplicationName;
        std::wstring const & get_ApplicationName() const { return applicationName_; }
    
        virtual void SetDescription() override;
    
        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;
        
        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);

        FABRIC_FIELDS_05(kind_, description_, unhealthyEvaluations_, aggregatedHealthState_, applicationName_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::ApplicationName, applicationName_); 
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvaluations, unhealthyEvaluations_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(unhealthyEvaluations_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationName_;
    };

    DEFINE_HEALTH_EVALUATION_ACTIVATOR( ApplicationHealthEvaluation, FABRIC_HEALTH_EVALUATION_KIND_APPLICATION )
}
