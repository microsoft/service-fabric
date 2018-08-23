// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // Base class for unhealthy evaluations that have a list of inner unhealthy evaluations
    // due to the hierarchical health model.
    class HealthEvaluationWithChildrenBase
        : public HealthEvaluationBase
    {
        DENY_COPY(HealthEvaluationWithChildrenBase)

    public:
        HealthEvaluationWithChildrenBase();

        explicit HealthEvaluationWithChildrenBase(
            FABRIC_HEALTH_EVALUATION_KIND kind);

        HealthEvaluationWithChildrenBase(
            FABRIC_HEALTH_EVALUATION_KIND kind,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            HealthEvaluationList && evaluations);

        HealthEvaluationWithChildrenBase(HealthEvaluationWithChildrenBase && other) = default;
        HealthEvaluationWithChildrenBase & operator = (HealthEvaluationWithChildrenBase && other) = default;

        virtual ~HealthEvaluationWithChildrenBase();

        __declspec(property(get=get_UnhealthyEvaluations)) HealthEvaluationList const & UnhealthyEvaluations;
        HealthEvaluationList const & get_UnhealthyEvaluations() const { return unhealthyEvaluations_; }

        virtual void GenerateDescription() override;
        virtual std::wstring GetUnhealthyEvaluationDescription(std::wstring const & indent = L"") const override;

        // Generate the description string.
        // If the object with description but without children doesn't respect maxAllowedSize, return EntryTooLarge.
        // Otherwise, try adding children until all fit or until the child that reaches max size.
        // Return success if all evaluations fit or EntryTooLarge otherwise.
        virtual Common::ErrorCode GenerateDescriptionAndTrimIfNeeded(
            size_t maxAllowedSize,
            __inout size_t & currentSize) override;

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const override;
        void WriteToEtw(uint16 contextSequenceId) const override;

    protected:
        HealthEvaluationList unhealthyEvaluations_;
    };
}
