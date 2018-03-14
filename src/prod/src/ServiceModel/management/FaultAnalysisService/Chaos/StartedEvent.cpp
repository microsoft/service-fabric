// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("StartedEvent");

StartedEvent::StartedEvent()
: ChaosEventBase(FABRIC_CHAOS_EVENT_KIND_STARTED)
, chaosParametersUPtr_()
{
}

StartedEvent::StartedEvent(StartedEvent && other)
: ChaosEventBase(move(other))
, chaosParametersUPtr_(move(other.chaosParametersUPtr_))
{
}

StartedEvent & StartedEvent::operator = (StartedEvent && other)
{
    if (this != &other)
    {
        chaosParametersUPtr_ = move(other.chaosParametersUPtr_);
    }

    ChaosEventBase::operator=(move(other));

    return *this;
}

ErrorCode StartedEvent::FromPublicApi(
    FABRIC_CHAOS_EVENT const & publicEvent)
{
    FABRIC_STARTED_EVENT * publicStartedEvent = reinterpret_cast<FABRIC_STARTED_EVENT *>(publicEvent.Value);

    if (publicStartedEvent == NULL)
    {
        return ErrorCodeValue::ArgumentNull;
    }

    timeStampUtc_ = DateTime(publicStartedEvent->TimeStampUtc);

    ChaosParameters chaosParameters;
    auto error = chaosParameters.FromPublicApi(*publicStartedEvent->ChaosParameters);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "StartedEvent::FromPublicApi/Failed at chaosParameters.FromPublicApi");
        return error;
    }

    chaosParametersUPtr_ = make_unique<ChaosParameters>(move(chaosParameters));

    return ErrorCodeValue::Success;
}

ErrorCode StartedEvent::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_EVENT & result) const
{
    auto startedEventResult = heap.AddItem<FABRIC_STARTED_EVENT>();
    startedEventResult->TimeStampUtc = timeStampUtc_.AsFileTime;

    auto publicChaosParametersPtr = heap.AddItem<FABRIC_CHAOS_PARAMETERS>();
    chaosParametersUPtr_->ToPublicApi(heap, *publicChaosParametersPtr);
    startedEventResult->ChaosParameters = publicChaosParametersPtr.GetRawPointer();

    result.Kind = FABRIC_CHAOS_EVENT_KIND_STARTED;
    result.Value = startedEventResult.GetRawPointer();

    return ErrorCode::Success();
}
