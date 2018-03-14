// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("ValidationFailedEvent");

ValidationFailedEvent::ValidationFailedEvent()
: ChaosEventBase(FABRIC_CHAOS_EVENT_KIND_VALIDATION_FAILED)
, reason_()
{
}

ValidationFailedEvent::ValidationFailedEvent(
    DateTime timeStampUtc,
    std::wstring const & reason)
    : ChaosEventBase(FABRIC_CHAOS_EVENT_KIND_VALIDATION_FAILED, timeStampUtc)
    , reason_(reason)
{
}

ValidationFailedEvent::ValidationFailedEvent(ValidationFailedEvent && other)
: ChaosEventBase(move(other))
, reason_(move(other.reason_))
{
}

ValidationFailedEvent & ValidationFailedEvent::operator = (ValidationFailedEvent && other)
{
    if (this != &other)
    {
        reason_ = move(other.reason_);
    }

    ChaosEventBase::operator=(move(other));

    return *this;
}

ErrorCode ValidationFailedEvent::FromPublicApi(
    FABRIC_CHAOS_EVENT const & publicEvent)
{
    FABRIC_VALIDATION_FAILED_EVENT * publicValidationFailedEvent = reinterpret_cast<FABRIC_VALIDATION_FAILED_EVENT *>(publicEvent.Value);

    timeStampUtc_ = DateTime(publicValidationFailedEvent->TimeStampUtc);

    HRESULT hr = StringUtility::LpcwstrToWstring(
        publicValidationFailedEvent->Reason,
        false,
        ParameterValidator::MinStringSize,
        ParameterValidator::MaxStringSize,
        reason_);

    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "ValidationFailedEvent::FromPublicApi/Failed to parse Reason: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    return ErrorCodeValue::Success;
}

ErrorCode ValidationFailedEvent::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_EVENT & result) const
{
    auto publicValidationFailedEvent = heap.AddItem<FABRIC_VALIDATION_FAILED_EVENT>();
    publicValidationFailedEvent->TimeStampUtc = timeStampUtc_.AsFileTime;
    publicValidationFailedEvent->Reason = heap.AddString(reason_);

    result.Kind = FABRIC_CHAOS_EVENT_KIND_VALIDATION_FAILED;
    result.Value = publicValidationFailedEvent.GetRawPointer();

    return ErrorCodeValue::Success;
}
