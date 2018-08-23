// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(HealthEvaluation)

HealthEvaluation::HealthEvaluation()
    : evaluation_()
{
}

HealthEvaluation::HealthEvaluation(
    HealthEvaluationBaseSPtr && evaluation)
    : evaluation_(move(evaluation))
{
}

HealthEvaluation::HealthEvaluation(
    HealthEvaluationBaseSPtr const & evaluation)
    : evaluation_(evaluation)
{
}

void HealthEvaluation::GenerateDescription(__in std::vector<HealthEvaluation> & evaluations)
{
    for (auto & eval : evaluations)
    {
        eval.Evaluation->GenerateDescription();
    }
}

wstring HealthEvaluation::GetUnhealthyEvaluationDescription(vector<HealthEvaluation> const & evaluations)
{
    wstring ret;
    StringWriter writer(ret);
    for (auto const & eval : evaluations)
    {
        writer.Write(eval.Evaluation->GetUnhealthyEvaluationDescription());
        writer.WriteLine();
    }

    return ret;
}

Common::ErrorCode HealthEvaluation::GenerateDescriptionAndTrimIfNeeded(
    __in std::vector<HealthEvaluation> & evaluations,
    size_t maxAllowedSize,
    __inout size_t & currentSize)
{
    // Reset the evaluations and add only the ones that fit, trimmed if needed
    vector<HealthEvaluation> temp = move(evaluations);

    ErrorCode error;
    for (auto && eval : temp)
    {
        if (!eval.Evaluation)
        {
            Assert::CodingError("GenerateDescriptionAndTrimIfNeeded: evaluation_ not set");
        }

        error = eval.Evaluation->GenerateDescriptionAndTrimIfNeeded(maxAllowedSize, currentSize);
        if (error.IsSuccess())
        {
            evaluations.push_back(move(eval));
        }
        else
        {
            // Error adding current evaluation, stop adding any more.
            // If at least one evaluation is added, consider success.
            if (evaluations.empty())
            {
                return error;
            }

            break;
        }
    }   

    return ErrorCode::Success();
}

ErrorCode HealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    if (evaluation_)
    {
        return evaluation_->ToPublicApi(heap, publicHealthEvaluation);
    }

    return ErrorCode::Success();
}

Common::ErrorCode HealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    evaluation_ = HealthEvaluationBase::CreateSPtr(publicHealthEvaluation.Kind);
    auto error = evaluation_->FromPublicApi(publicHealthEvaluation);
    if (!error.IsSuccess())
    {
        return error;
    }

    return ErrorCode::Success();
}


wstring HealthEvaluation::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<HealthEvaluation&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

void HealthEvaluation::WriteTo(__in TextWriter& writer, FormatOptions const &) const
{
    writer.Write("{0}", *evaluation_);
}

void HealthEvaluation::WriteToEtw(uint16 contextSequenceId) const
{
    ServiceModel::ServiceModelEventSource::Trace->HealthEvaluationDescriptionTrace(
        contextSequenceId,
        *evaluation_);
}
