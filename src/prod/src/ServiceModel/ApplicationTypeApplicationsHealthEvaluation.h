// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ApplicationTypeApplicationsHealthEvaluation
        : public HealthEvaluationWithChildrenBase
    {
        DENY_COPY(ApplicationTypeApplicationsHealthEvaluation)

    public:
        ApplicationTypeApplicationsHealthEvaluation();

        ApplicationTypeApplicationsHealthEvaluation(
            std::wstring const & applicationTypeName,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            HealthEvaluationList && unhealthyEvaluations,
            ULONG totalCount,
            BYTE maxPercentUnhealthyApplications);

        ApplicationTypeApplicationsHealthEvaluation(ApplicationTypeApplicationsHealthEvaluation && other) = default;
        ApplicationTypeApplicationsHealthEvaluation & operator = (ApplicationTypeApplicationsHealthEvaluation && other) = default;

        virtual ~ApplicationTypeApplicationsHealthEvaluation();
        
        __declspec(property(get=get_ApplicationTypeName)) std::wstring const & ApplicationTypeName;
        std::wstring const & get_ApplicationTypeName() const { return applicationTypeName_; }

        __declspec(property(get = get_MaxPercentUnhealthyApplications)) BYTE MaxPercentUnhealthyApplications;
        BYTE get_MaxPercentUnhealthyApplications() const { return maxPercentUnhealthyApplications_; }
        
        __declspec(property(get=get_TotalCount)) ULONG TotalCount;
        ULONG get_TotalCount() const { return totalCount_; }

        virtual void SetDescription() override;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;
        
        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);

        FABRIC_FIELDS_07(kind_, applicationTypeName_, description_, unhealthyEvaluations_, maxPercentUnhealthyApplications_, totalCount_, aggregatedHealthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvaluations, unhealthyEvaluations_)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentUnhealthyApplications, maxPercentUnhealthyApplications_)
            SERIALIZABLE_PROPERTY(Constants::TotalCount, totalCount_); 
            SERIALIZABLE_PROPERTY(Constants::ApplicationTypeName, applicationTypeName_);
        END_JSON_SERIALIZABLE_PROPERTIES()     

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(unhealthyEvaluations_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationTypeName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationTypeName_;
        BYTE maxPercentUnhealthyApplications_;
        ULONG totalCount_;
    };

    DEFINE_HEALTH_EVALUATION_ACTIVATOR( ApplicationTypeApplicationsHealthEvaluation, FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS )
}
