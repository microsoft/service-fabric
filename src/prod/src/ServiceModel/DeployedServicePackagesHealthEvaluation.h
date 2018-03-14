// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedServicePackagesHealthEvaluation
        : public HealthEvaluationWithChildrenBase
    {
        DENY_COPY(DeployedServicePackagesHealthEvaluation)

    public:
        DeployedServicePackagesHealthEvaluation();

        DeployedServicePackagesHealthEvaluation(
            FABRIC_HEALTH_STATE aggregatedHealthState,
            HealthEvaluationList && unhealthyEvaluations,
            ULONG totalCount);

        DeployedServicePackagesHealthEvaluation(DeployedServicePackagesHealthEvaluation && other) = default;
        DeployedServicePackagesHealthEvaluation & operator = (DeployedServicePackagesHealthEvaluation && other) = default;

        virtual ~DeployedServicePackagesHealthEvaluation();

        __declspec(property(get=get_TotalCount)) ULONG TotalCount;
        ULONG get_TotalCount() const { return totalCount_; }

        virtual void SetDescription() override;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);

        FABRIC_FIELDS_05(kind_, description_, unhealthyEvaluations_, totalCount_, aggregatedHealthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvaluations, unhealthyEvaluations_)
            SERIALIZABLE_PROPERTY(Constants::TotalCount, totalCount_);
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(unhealthyEvaluations_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        ULONG totalCount_;
    };

    DEFINE_HEALTH_EVALUATION_ACTIVATOR( DeployedServicePackagesHealthEvaluation, FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES )
}
