// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class DeployedApplicationsHealthEvaluation
        : public HealthEvaluationWithChildrenBase
    {
        DENY_COPY(DeployedApplicationsHealthEvaluation)

    public:
        DeployedApplicationsHealthEvaluation();

        DeployedApplicationsHealthEvaluation(
            FABRIC_HEALTH_STATE aggregatedHealthState,
            HealthEvaluationList && unhealthyEvaluations,
            ULONG totalCount,
            BYTE maxPercentUnhealthyDeployedApplicationsPerService);

        DeployedApplicationsHealthEvaluation(DeployedApplicationsHealthEvaluation && other) = default;
        DeployedApplicationsHealthEvaluation & operator = (DeployedApplicationsHealthEvaluation && other) = default;

        virtual ~DeployedApplicationsHealthEvaluation();

        __declspec(property(get=get_MaxPercentUnhealthyDeployedApplications)) BYTE MaxPercentUnhealthyDeployedApplications;
        BYTE get_MaxPercentUnhealthyDeployedApplications() const { return maxPercentUnhealthyDeployedApplications_; }

        __declspec(property(get=get_TotalCount)) ULONG TotalCount;
        ULONG get_TotalCount() const { return totalCount_; }

        virtual void SetDescription() override;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;
        
        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);

        FABRIC_FIELDS_06(kind_, description_, unhealthyEvaluations_, maxPercentUnhealthyDeployedApplications_, totalCount_, aggregatedHealthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvaluations, unhealthyEvaluations_)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentUnhealthyDeployedApplications, maxPercentUnhealthyDeployedApplications_)
            SERIALIZABLE_PROPERTY(Constants::TotalCount, totalCount_); 
        END_JSON_SERIALIZABLE_PROPERTIES()     

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(unhealthyEvaluations_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        BYTE maxPercentUnhealthyDeployedApplications_;
        ULONG totalCount_;
    };

    DEFINE_HEALTH_EVALUATION_ACTIVATOR( DeployedApplicationsHealthEvaluation, FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS )
}
