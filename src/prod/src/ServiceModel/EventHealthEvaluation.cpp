// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("EventEvaluation");

INITIALIZE_SIZE_ESTIMATION(EventHealthEvaluation)

EventHealthEvaluation::EventHealthEvaluation()
    : HealthEvaluationBase(FABRIC_HEALTH_EVALUATION_KIND_EVENT)
    , unhealthyEvent_()
    , considerWarningAsError_(false)
{
}

EventHealthEvaluation::EventHealthEvaluation(
    FABRIC_HEALTH_STATE aggregatedHealthState,
    HealthEvent const & unhealthyEvent,
    bool considerWarningAsError)
    : HealthEvaluationBase(FABRIC_HEALTH_EVALUATION_KIND_EVENT, aggregatedHealthState)
    , unhealthyEvent_(unhealthyEvent)
    , considerWarningAsError_(considerWarningAsError)
{
}

EventHealthEvaluation::~EventHealthEvaluation()
{
}

ErrorCode EventHealthEvaluation::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    auto publicEventHealthEvaluation = heap.AddItem<FABRIC_EVENT_HEALTH_EVALUATION>();

    publicEventHealthEvaluation->Description = heap.AddString(description_);

    auto publicUnhealthyEvent = heap.AddItem<FABRIC_HEALTH_EVENT>();
    auto error = unhealthyEvent_.ToPublicApi(heap, *publicUnhealthyEvent);
    if (!error.IsSuccess()) { return error; }
    publicEventHealthEvaluation->UnhealthyEvent = publicUnhealthyEvent.GetRawPointer();

    publicEventHealthEvaluation->ConsiderWarningAsError = considerWarningAsError_ ? TRUE : FALSE;

    publicEventHealthEvaluation->AggregatedHealthState = aggregatedHealthState_;

    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_EVENT;
    publicHealthEvaluation.Value = publicEventHealthEvaluation.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode EventHealthEvaluation::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto publicEventHealthEvaluation = reinterpret_cast<FABRIC_EVENT_HEALTH_EVALUATION *>(publicHealthEvaluation.Value);

    auto hr = StringUtility::LpcwstrToWstring(publicEventHealthEvaluation->Description, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, description_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing description in FromPublicAPI: {0}", error);
        return error;
    }

    considerWarningAsError_ = (publicEventHealthEvaluation->ConsiderWarningAsError == TRUE);

    aggregatedHealthState_ = publicEventHealthEvaluation->AggregatedHealthState;

    // Event
    auto publicUnhealthyEvent = publicEventHealthEvaluation->UnhealthyEvent;
    error = unhealthyEvent_.FromPublicApi(*publicUnhealthyEvent);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

wstring EventHealthEvaluation::GetUnhealthyEvaluationDescription(wstring const & indent) const
{
    return wformatString("{0}{1} '{2}'", indent, description_, unhealthyEvent_.Description);
}

void EventHealthEvaluation::SetDescription()
{
    if (unhealthyEvent_.IsExpired)
    {
        description_ = wformatString(
            HMResource::GetResources().HealthEvaluationExpiredEvent,
            unhealthyEvent_.State,
            unhealthyEvent_.SourceId,
            unhealthyEvent_.Property,
            unhealthyEvent_.LastModifiedUtcTimestamp,
            unhealthyEvent_.TimeToLive);
    }
    else if (unhealthyEvent_.State == FABRIC_HEALTH_STATE_ERROR || !considerWarningAsError_)
    {
        description_ = wformatString(
            HMResource::GetResources().HealthEvaluationUnhealthyEvent,
            unhealthyEvent_.SourceId,
            unhealthyEvent_.State,
            unhealthyEvent_.Property);
    }
    else
    {
        // ConsiderWarningAsError is true and the event was not reported with error state.
        description_ = wformatString(
            HMResource::GetResources().HealthEvaluationUnhealthyEventPerPolicy,
            unhealthyEvent_.SourceId,
            unhealthyEvent_.State,
            unhealthyEvent_.Property);
    }
}

void EventHealthEvaluation::WriteTo(__in TextWriter& writer, FormatOptions const &) const
{
    if (unhealthyEvent_.State == FABRIC_HEALTH_STATE_ERROR)
    {
        writer.WriteLine("ErrorEvent: {0}", unhealthyEvent_);
    }
    else if (unhealthyEvent_.IsExpired)
    {
        writer.WriteLine("ExpiredEvent: {0}", unhealthyEvent_);
    }
    else
    {
        writer.WriteLine("UnhealthyEvent, ConsiderWarningAsError {0}: {1}", considerWarningAsError_, unhealthyEvent_);
    }
}

void EventHealthEvaluation::WriteToEtw(uint16 contextSequenceId) const
{
    if (unhealthyEvent_.State == FABRIC_HEALTH_STATE_ERROR)
    {
        ServiceModelEventSource::Trace->ErrorEventHealthEvaluationTrace(contextSequenceId, unhealthyEvent_);
    }
    else if (unhealthyEvent_.IsExpired)
    {
        ServiceModelEventSource::Trace->ExpiredEventHealthEvaluationTrace(contextSequenceId, unhealthyEvent_);
    }
    else
    {
        ServiceModelEventSource::Trace->UnhealthyEventHealthEvaluationTrace(contextSequenceId, considerWarningAsError_, unhealthyEvent_);
    }
}
