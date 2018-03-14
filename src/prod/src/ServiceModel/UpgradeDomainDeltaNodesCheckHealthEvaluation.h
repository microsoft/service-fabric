// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class UpgradeDomainDeltaNodesCheckHealthEvaluation
        : public HealthEvaluationWithChildrenBase
    {
        DENY_COPY(UpgradeDomainDeltaNodesCheckHealthEvaluation)

    public:
        UpgradeDomainDeltaNodesCheckHealthEvaluation();

        UpgradeDomainDeltaNodesCheckHealthEvaluation(
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::wstring const & upgradeDomainName,
            ULONG baselineErrorCount,
            ULONG baselineTotalCount,
            ULONG totalCount,
            BYTE maxPercentUpgradeDomainDeltaUnhealthyNodes,
            HealthEvaluationList && unhealthyEvaluations);

        UpgradeDomainDeltaNodesCheckHealthEvaluation(UpgradeDomainDeltaNodesCheckHealthEvaluation && other) = default;
        UpgradeDomainDeltaNodesCheckHealthEvaluation & operator = (UpgradeDomainDeltaNodesCheckHealthEvaluation && other) = default;

        virtual ~UpgradeDomainDeltaNodesCheckHealthEvaluation();

        __declspec(property(get = get_UpgradeDomainName)) std::wstring const & UpgradeDomainName;
        std::wstring const & get_UpgradeDomainName() const { return upgradeDomainName_; }

        __declspec(property(get = get_BaselineErrorCount)) ULONG BaselineErrorCount;
        ULONG get_BaselineErrorCount() const { return baselineErrorCount_; }

        __declspec(property(get = get_BaselineTotalCount)) ULONG BaselineTotalCount;
        ULONG get_BaselineTotalCount() const { return baselineTotalCount_; }

        __declspec(property(get = get_TotalCount)) ULONG TotalCount;
        ULONG get_TotalCount() const { return totalCount_; }

        __declspec(property(get = get_MaxPercentUpgradeDomainDeltaUnhealthyNodes)) BYTE MaxPercentUpgradeDomainDeltaUnhealthyNodes;
        BYTE get_MaxPercentUpgradeDomainDeltaUnhealthyNodes() const { return maxPercentUpgradeDomainDeltaUnhealthyNodes_; }

        virtual void SetDescription() override;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __inout FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);

        FABRIC_FIELDS_09(description_, aggregatedHealthState_, upgradeDomainName_, baselineErrorCount_, baselineTotalCount_, totalCount_, maxPercentUpgradeDomainDeltaUnhealthyNodes_, unhealthyEvaluations_, kind_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::UpgradeDomainName, upgradeDomainName_)
            SERIALIZABLE_PROPERTY(Constants::BaselineErrorCount, baselineErrorCount_)
            SERIALIZABLE_PROPERTY(Constants::BaselineTotalCount, baselineTotalCount_)
            SERIALIZABLE_PROPERTY(Constants::TotalCount, totalCount_)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentUpgradeDomainDeltaUnhealthyNodes, maxPercentUpgradeDomainDeltaUnhealthyNodes_)
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvaluations, unhealthyEvaluations_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(upgradeDomainName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(unhealthyEvaluations_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring upgradeDomainName_;
        ULONG baselineErrorCount_;
        ULONG baselineTotalCount_;
        ULONG totalCount_;
        BYTE maxPercentUpgradeDomainDeltaUnhealthyNodes_;
    };

    DEFINE_HEALTH_EVALUATION_ACTIVATOR( UpgradeDomainDeltaNodesCheckHealthEvaluation, FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK )
}

