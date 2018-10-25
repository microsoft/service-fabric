// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("HealthEvaluationWithChildrenBase");

HealthEvaluationWithChildrenBase::HealthEvaluationWithChildrenBase()
    : HealthEvaluationBase()
    , unhealthyEvaluations_()
{
}

HealthEvaluationWithChildrenBase::HealthEvaluationWithChildrenBase(
    FABRIC_HEALTH_EVALUATION_KIND kind)
    : HealthEvaluationBase(kind)
    , unhealthyEvaluations_()
{
}

HealthEvaluationWithChildrenBase::HealthEvaluationWithChildrenBase(
    FABRIC_HEALTH_EVALUATION_KIND kind,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    HealthEvaluationList && evaluations)
    : HealthEvaluationBase(kind, aggregatedHealthState)
    , unhealthyEvaluations_(move(evaluations))
{
}

HealthEvaluationWithChildrenBase::~HealthEvaluationWithChildrenBase()
{
}

Common::ErrorCode HealthEvaluationWithChildrenBase::GenerateDescriptionAndTrimIfNeeded(
    size_t maxAllowedSize,
    __inout size_t & currentSize)
{
    // First generate description using all children, which may be needed for statistics
    this->SetDescription();

    // Estimate size without any unhealthy evaluations
    auto evaluations = move(unhealthyEvaluations_);

    auto error = this->TryAdd(maxAllowedSize, currentSize);
    if (!error.IsSuccess()) { return error; }
    
    // Try to add the unhealthy evaluation children, trimmed if needed
    error = HealthEvaluation::GenerateDescriptionAndTrimIfNeeded(evaluations, maxAllowedSize, currentSize);
    if (error.IsSuccess() || !evaluations.empty())
    {
        unhealthyEvaluations_ = move(evaluations);
    }

    // Since the description fit, return success even if no child evaluation can be added.
    return ErrorCode::Success();
}

void HealthEvaluationWithChildrenBase::GenerateDescription()
{
    this->SetDescription();
    HealthEvaluation::GenerateDescription(unhealthyEvaluations_);
}

wstring HealthEvaluationWithChildrenBase::GetUnhealthyEvaluationDescription(wstring const & indent) const
{
    wstring ret;
    StringWriter writer(ret);
    writer.Write("{0}{1}", indent, description_);
    if (!unhealthyEvaluations_.empty())
    {
        wstring newIndent = indent + L"  ";
        for (auto const & eval : unhealthyEvaluations_)
        {
            // Write each child on a new line
            writer.WriteLine();
            writer.Write(eval.Evaluation->GetUnhealthyEvaluationDescription(newIndent));
        }
    }

    return ret;
}

void HealthEvaluationWithChildrenBase::WriteTo(__in TextWriter& writer, FormatOptions const &) const
{
    if (unhealthyEvaluations_.empty())
    {
        writer.Write("\r\n{0}", description_);
    }
    else if (unhealthyEvaluations_.size() == 1)
    {
        writer.Write("\r\n{0} {1}", description_, *unhealthyEvaluations_[0].Evaluation);
    }
    else
    {
        // Print the vector
        writer.WriteLine("\r\n{0} {1}", description_, unhealthyEvaluations_);
    }
}

void HealthEvaluationWithChildrenBase::WriteToEtw(uint16 contextSequenceId) const
{
    if (unhealthyEvaluations_.empty())
    {
        ServiceModel::ServiceModelEventSource::Trace->UnhealthyHealthEvaluationNoChildDescription(
            contextSequenceId,
            description_);
    }
    else if (unhealthyEvaluations_.size() == 1)
    {
        // Optimization to avoid serializing the vector in this case, which is the most common one
        ServiceModel::ServiceModelEventSource::Trace->UnhealthyHealthEvaluationWithOneChildDescription(
            contextSequenceId,
            description_,
            *unhealthyEvaluations_[0].Evaluation);
    }
    else
    {
        ServiceModel::ServiceModelEventSource::Trace->UnhealthyHealthEvaluationDescription(
            contextSequenceId,
            description_,
            unhealthyEvaluations_);
    }
}
