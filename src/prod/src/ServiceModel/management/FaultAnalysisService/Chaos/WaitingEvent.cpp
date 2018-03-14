// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("WaitingEvent");

WaitingEvent::WaitingEvent()
: ChaosEventBase(FABRIC_CHAOS_EVENT_KIND_WAITING)
, reason_()
{
}

WaitingEvent::WaitingEvent(
    DateTime timeStampUtc,
    wstring & reason)
    : ChaosEventBase(FABRIC_CHAOS_EVENT_KIND_WAITING, timeStampUtc)
    , reason_(reason)
{
}

WaitingEvent::WaitingEvent(WaitingEvent && other)
: ChaosEventBase(move(other))
, reason_(move(other.reason_))
{
}

WaitingEvent & WaitingEvent::operator = (WaitingEvent && other)
{
    if (this != &other)
    {
        reason_ = move(other.reason_);
    }

    ChaosEventBase::operator=(move(other));

    return *this;
}

ErrorCode WaitingEvent::FromPublicApi(
    FABRIC_CHAOS_EVENT const & publicEvent)
{
    auto publicWaitingEvent = reinterpret_cast<FABRIC_WAITING_EVENT *>(publicEvent.Value);

    timeStampUtc_ = DateTime(publicWaitingEvent->TimeStampUtc);

    HRESULT hr = StringUtility::LpcwstrToWstring(
        publicWaitingEvent->Reason,
        false,
        ParameterValidator::MinStringSize,
        ParameterValidator::MaxStringSize,
        reason_);
    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "WaitingEvent::FromPublicApi/Failed to parse Reason: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    return ErrorCodeValue::Success;
}

ErrorCode WaitingEvent::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_EVENT & result) const
{
    auto publicWaitingEvent = heap.AddItem<FABRIC_WAITING_EVENT>();
    publicWaitingEvent->TimeStampUtc = timeStampUtc_.AsFileTime;
    publicWaitingEvent->Reason = heap.AddString(reason_);

    result.Kind = FABRIC_CHAOS_EVENT_KIND_WAITING;
    result.Value = publicWaitingEvent.GetRawPointer();

    return ErrorCode::Success();
}
