// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationUnhealthyEvaluation
        : public ApplicationAggregatedHealthState
    {
    public:
        ApplicationUnhealthyEvaluation() = default;
        ApplicationUnhealthyEvaluation(
            std::wstring const & applicationName,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::vector<HealthEvaluation> && unhealthyEvaluations);

        ApplicationUnhealthyEvaluation(ApplicationUnhealthyEvaluation && other) = default;
        ApplicationUnhealthyEvaluation & operator=(ApplicationUnhealthyEvaluation && other) = default;

        __declspec(property(get=get_UnhealthyEvaluations)) std::vector<HealthEvaluation> const & UnhealthyEvaluations;
        std::vector<HealthEvaluation> const & get_UnhealthyEvaluations() const { return unhealthyEvaluations_; }

        FABRIC_FIELDS_01(unhealthyEvaluations_);

    private:
        std::vector<HealthEvaluation> unhealthyEvaluations_;
    };
}
