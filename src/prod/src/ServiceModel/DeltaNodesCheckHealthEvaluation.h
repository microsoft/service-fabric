// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeltaNodesCheckHealthEvaluation
        : public HealthEvaluationWithChildrenBase
    {
        DENY_COPY(DeltaNodesCheckHealthEvaluation)

    public:
        DeltaNodesCheckHealthEvaluation();

        DeltaNodesCheckHealthEvaluation(
            FABRIC_HEALTH_STATE aggregatedHealthState,
            ULONG baselineErrorCount,
            ULONG baselineTotalCount,
            ULONG totalCount,
            BYTE maxPercentDeltaUnhealthyNodes,
            HealthEvaluationList && unhealthyEvaluations);

        DeltaNodesCheckHealthEvaluation(DeltaNodesCheckHealthEvaluation && other) = default;
        DeltaNodesCheckHealthEvaluation & operator = (DeltaNodesCheckHealthEvaluation && other) = default;

        virtual ~DeltaNodesCheckHealthEvaluation();

        __declspec(property(get = get_BaselineErrorCount)) ULONG BaselineErrorCount;
        ULONG get_BaselineErrorCount() const { return baselineErrorCount_; }

        __declspec(property(get = get_BaselineTotalCount)) ULONG BaselineTotalCount;
        ULONG get_BaselineTotalCount() const { return baselineTotalCount_; }

        __declspec(property(get = get_TotalCount)) ULONG TotalCount;
        ULONG get_TotalCount() const { return totalCount_; }

        __declspec(property(get = get_MaxPercentDeltaUnhealthyNodes)) BYTE MaxPercentDeltaUnhealthyNodes;
        BYTE get_MaxPercentDeltaUnhealthyNodes() const { return maxPercentDeltaUnhealthyNodes_; }

        virtual void SetDescription() override;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __inout FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);

        FABRIC_FIELDS_08(kind_, description_, aggregatedHealthState_, baselineErrorCount_, baselineTotalCount_, totalCount_, maxPercentDeltaUnhealthyNodes_, unhealthyEvaluations_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::BaselineErrorCount, baselineErrorCount_)
            SERIALIZABLE_PROPERTY(Constants::BaselineTotalCount, baselineTotalCount_)
            SERIALIZABLE_PROPERTY(Constants::TotalCount, totalCount_)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentDeltaUnhealthyNodes, maxPercentDeltaUnhealthyNodes_)
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvaluations, unhealthyEvaluations_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(unhealthyEvaluations_)
        END_DYNAMIC_SIZE_ESTIMATION()
    private:
        ULONG baselineErrorCount_;
        ULONG baselineTotalCount_;
        ULONG totalCount_;
        BYTE maxPercentDeltaUnhealthyNodes_;
    };

    DEFINE_HEALTH_EVALUATION_ACTIVATOR( DeltaNodesCheckHealthEvaluation, FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK )
}

