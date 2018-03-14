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

StringLiteral const TraceComponent("TestErrorEvent");

TestErrorEvent::TestErrorEvent()
: ChaosEventBase(FABRIC_CHAOS_EVENT_KIND_TEST_ERROR)
, reason_()
{
}

TestErrorEvent::TestErrorEvent(
    DateTime timeStampUtc,
    wstring & reason)
    : ChaosEventBase(FABRIC_CHAOS_EVENT_KIND_TEST_ERROR, timeStampUtc)
    , reason_(reason)
{
}

TestErrorEvent::TestErrorEvent(TestErrorEvent && other)
: ChaosEventBase(move(other))
, reason_(move(reason_))
{
}

TestErrorEvent & TestErrorEvent::operator = (TestErrorEvent && other)
{
    if (this != &other)
    {
        reason_ = move(other.reason_);
    }

    ChaosEventBase::operator=(move(other));

    return *this;
}

Common::ErrorCode TestErrorEvent::FromPublicApi(
    FABRIC_CHAOS_EVENT const & publicEvent)
{
    ErrorCode error(ErrorCodeValue::Success);

    FABRIC_TEST_ERROR_EVENT * publicTestErrorEvent = reinterpret_cast<FABRIC_TEST_ERROR_EVENT *>(publicEvent.Value);

    timeStampUtc_ = DateTime(publicTestErrorEvent->TimeStampUtc);

    HRESULT hr = StringUtility::LpcwstrToWstring(
        publicTestErrorEvent->Reason,
        false,
        ParameterValidator::MinStringSize,
        ParameterValidator::MaxStringSize,
        reason_);

    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "TestErrorEvent::FromPublicApi/Failed to parse Reason: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    return error;
}

ErrorCode TestErrorEvent::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_EVENT & result) const
{
    auto publicTestErrorEvent = heap.AddItem<FABRIC_TEST_ERROR_EVENT>();
    publicTestErrorEvent->TimeStampUtc = timeStampUtc_.AsFileTime;
    publicTestErrorEvent->Reason = heap.AddString(reason_);

    result.Kind = FABRIC_CHAOS_EVENT_KIND_TEST_ERROR;
    result.Value = publicTestErrorEvent.GetRawPointer();

    return ErrorCode::Success();
}
