// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class SystemApplicationHealthEvaluation
        : public HealthEvaluationWithChildrenBase
    {
        DENY_COPY(SystemApplicationHealthEvaluation)

    public:
        SystemApplicationHealthEvaluation();

        SystemApplicationHealthEvaluation(
            FABRIC_HEALTH_STATE aggregatedHealthState,
            HealthEvaluationList && unhealthyEvaluations);

        SystemApplicationHealthEvaluation(SystemApplicationHealthEvaluation && other) = default;
        SystemApplicationHealthEvaluation & operator = (SystemApplicationHealthEvaluation && other) = default;

        virtual void SetDescription() override;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;
        
        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);

        FABRIC_FIELDS_04(kind_, description_, unhealthyEvaluations_, aggregatedHealthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvaluations, unhealthyEvaluations_)
        END_JSON_SERIALIZABLE_PROPERTIES()     

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(unhealthyEvaluations_)
        END_DYNAMIC_SIZE_ESTIMATION()

    };

    DEFINE_HEALTH_EVALUATION_ACTIVATOR( SystemApplicationHealthEvaluation, FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION )
}
