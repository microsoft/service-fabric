// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("ExecutingFaultsEvent");

ExecutingFaultsEvent::ExecutingFaultsEvent()
: ChaosEventBase(FABRIC_CHAOS_EVENT_KIND_EXECUTING_FAULTS)
, faults_()
{
}

ExecutingFaultsEvent::ExecutingFaultsEvent(
    DateTime timeStampUtc,
    vector<wstring> const & faults)
    : ChaosEventBase(FABRIC_CHAOS_EVENT_KIND_EXECUTING_FAULTS, timeStampUtc)
    , faults_(faults)
{
}

ExecutingFaultsEvent::ExecutingFaultsEvent(ExecutingFaultsEvent && other)
: ChaosEventBase(move(other))
, faults_(move(other.faults_))
{
}

ExecutingFaultsEvent & ExecutingFaultsEvent::operator = (ExecutingFaultsEvent && other)
{
    if (this != &other)
    {
        faults_ = move(other.faults_);
    }

    ChaosEventBase::operator=(move(other));

    return *this;
}

ErrorCode ExecutingFaultsEvent::FromPublicApi(
    FABRIC_CHAOS_EVENT const & publicEvent)
{
    FABRIC_EXECUTING_FAULTS_EVENT * publicExecutingFaultsEvent = reinterpret_cast<FABRIC_EXECUTING_FAULTS_EVENT *>(publicEvent.Value);
    
    timeStampUtc_ = DateTime(publicExecutingFaultsEvent->TimeStampUtc);

    faults_.clear();

    auto publicFaultList = (FABRIC_STRING_LIST*)publicExecutingFaultsEvent->Faults;

    auto publicFaults = (LPCWSTR*)publicFaultList->Items;

    for (size_t i = 0; i != publicFaultList->Count; ++i)
    {
        std::wstring fault;
        auto hr = StringUtility::LpcwstrToWstring(
            (*(publicFaults + i)), 
            false,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxStringSize,
            fault);

        faults_.push_back(fault);

        if (FAILED(hr))
        {
            return ErrorCode::FromHResult(hr);
        }
    }

    return ErrorCode::Success();
}

ErrorCode ExecutingFaultsEvent::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_EVENT & result) const
{
    auto publicExecutingFaultsEvent = heap.AddItem<FABRIC_EXECUTING_FAULTS_EVENT>();
    
    publicExecutingFaultsEvent->TimeStampUtc = timeStampUtc_.AsFileTime;

    // list of faults
    auto faultList = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, faults_, faultList);
    publicExecutingFaultsEvent->Faults = faultList.GetRawPointer();

    result.Kind = FABRIC_CHAOS_EVENT_KIND_EXECUTING_FAULTS;

    result.Value = publicExecutingFaultsEvent.GetRawPointer();

    return ErrorCode::Success();
}
