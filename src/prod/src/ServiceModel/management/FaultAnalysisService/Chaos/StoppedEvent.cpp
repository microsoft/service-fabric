// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("StoppedEvent");

StoppedEvent::StoppedEvent()
: ChaosEventBase(FABRIC_CHAOS_EVENT_KIND_STOPPED)
, reason_()
{
}

StoppedEvent::StoppedEvent(
    DateTime timeStampUtc,
    std::wstring const & reason)
    : ChaosEventBase(FABRIC_CHAOS_EVENT_KIND_STOPPED, timeStampUtc)
    , reason_(reason)
{
}

StoppedEvent::StoppedEvent(StoppedEvent && other)
: ChaosEventBase(move(other))
, reason_(move(other.reason_))
{
}

StoppedEvent & StoppedEvent::operator = (StoppedEvent && other)
{
    if (this != &other)
    {
        reason_ = move(other.reason_);
    }

    ChaosEventBase::operator=(move(other));

    return *this;
}

ErrorCode StoppedEvent::FromPublicApi(
    FABRIC_CHAOS_EVENT const & publicEvent)
{
    ErrorCode error(ErrorCodeValue::Success);

    FABRIC_STOPPED_EVENT * publicStoppedEvent = reinterpret_cast<FABRIC_STOPPED_EVENT *>(publicEvent.Value);

    timeStampUtc_ = DateTime(publicStoppedEvent->TimeStampUtc);

    HRESULT hr = StringUtility::LpcwstrToWstring(
        publicStoppedEvent->Reason, 
        false, 
        ParameterValidator::MinStringSize, 
        ParameterValidator::MaxStringSize, 
        reason_);

    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteError(TraceComponent, "StoppedEvent::FromPublicApi/Failed to parse Reason: {0}", hr);
        return error;
    }

    return error;
}

ErrorCode StoppedEvent::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_EVENT & result) const
{
    auto publicStoppedEvent = heap.AddItem<FABRIC_STOPPED_EVENT>();
    publicStoppedEvent->TimeStampUtc = timeStampUtc_.AsFileTime;
    publicStoppedEvent->Reason = heap.AddString(reason_);

    result.Kind = FABRIC_CHAOS_EVENT_KIND_STOPPED;
    result.Value = publicStoppedEvent.GetRawPointer();

    return ErrorCode::Success();
}
