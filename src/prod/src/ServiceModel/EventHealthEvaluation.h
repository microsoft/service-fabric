// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class EventHealthEvaluation
        : public HealthEvaluationBase
    {
        DENY_COPY(EventHealthEvaluation)

    public:
        EventHealthEvaluation();

        EventHealthEvaluation(
            FABRIC_HEALTH_STATE aggregatedHealthState,
            HealthEvent const & unhealthyEvent,
            bool considerWarningAsError);

        EventHealthEvaluation(EventHealthEvaluation && other) = default;
        EventHealthEvaluation & operator = (EventHealthEvaluation && other) = default;

        virtual ~EventHealthEvaluation();

        __declspec(property(get=get_UnhealthyEvent)) HealthEvent const & UnhealthyEvent;
        HealthEvent const & get_UnhealthyEvent() const { return unhealthyEvent_; }

        __declspec(property(get=get_ConsiderWarningAsError)) bool ConsiderWarningAsError;
        bool get_ConsiderWarningAsError() const { return considerWarningAsError_; }

        virtual void SetDescription() override;

        virtual std::wstring GetUnhealthyEvaluationDescription(std::wstring const & indent = L"") const override;
        
        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_05(kind_, description_, unhealthyEvent_, considerWarningAsError_, aggregatedHealthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvent, unhealthyEvent_)
            SERIALIZABLE_PROPERTY(Constants::ConsiderWarningAsError, considerWarningAsError_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(unhealthyEvent_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        HealthEvent unhealthyEvent_;
        bool considerWarningAsError_;
    };

    DEFINE_HEALTH_EVALUATION_ACTIVATOR( EventHealthEvaluation, FABRIC_HEALTH_EVALUATION_KIND_EVENT )
}
