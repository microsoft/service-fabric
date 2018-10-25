// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // Wrapper for the health evaluation hierarchical classes.
    class HealthEvaluation 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
    {
    public:
        HealthEvaluation();
        explicit HealthEvaluation(HealthEvaluationBaseSPtr && evaluation);
        explicit HealthEvaluation(HealthEvaluationBaseSPtr const & evaluation);
        
        HealthEvaluation(HealthEvaluation const & other) = default;
        HealthEvaluation & operator = (HealthEvaluation const & other) = default;

        HealthEvaluation(HealthEvaluation && other) = default;
        HealthEvaluation & operator = (HealthEvaluation && other) = default;

        __declspec(property(get=get_EvaluationReason)) HealthEvaluationBaseSPtr const & Evaluation;
        HealthEvaluationBaseSPtr const & get_EvaluationReason() const { return evaluation_; }

        // Recursively generates description for all entries in the vector.
        static void GenerateDescription(__in std::vector<HealthEvaluation> & evaluations);
        static std::wstring GetUnhealthyEvaluationDescription(std::vector<HealthEvaluation> const & evaluations);

        // Updates health evaluations with description.
        // If max allowed size is hit, the evaluations are trimmed if possible.
        // Otherwise, EntryTooLarge error is returned.
        // If success, currentSize is updated with evaluations size.
        static Common::ErrorCode GenerateDescriptionAndTrimIfNeeded(
            __in std::vector<HealthEvaluation> & evaluations,
            size_t maxAllowedSize,
            __inout size_t & currentSize);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);
    
        std::wstring ToString() const;

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_01(evaluation_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY( Constants::HealthEvaluation, evaluation_ )
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(evaluation_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        HealthEvaluationBaseSPtr evaluation_;
    };

    using HealthEvaluationList = std::vector<HealthEvaluation>;
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::HealthEvaluation);
