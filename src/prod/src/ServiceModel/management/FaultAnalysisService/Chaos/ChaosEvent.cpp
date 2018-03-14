// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::FaultAnalysisService;

ChaosEvent::ChaosEvent()
: chaosEvent_()
{
}

ChaosEvent::ChaosEvent(
    ChaosEventBaseSPtr && chaosEvent)
    : chaosEvent_(move(chaosEvent))
{
}

ChaosEvent::ChaosEvent(
    ChaosEventBaseSPtr const & chaosEvent)
    : chaosEvent_(chaosEvent)
{
}

ChaosEvent::ChaosEvent(ChaosEvent && other)
: chaosEvent_(move(other.chaosEvent_))
{
}

ChaosEvent & ChaosEvent::operator = (ChaosEvent && other)
{
    if (this != &other)
    {
        chaosEvent_ = move(other.chaosEvent_);
    }

    return *this;
}

ChaosEvent::ChaosEvent(ChaosEvent const & other)
: chaosEvent_(other.chaosEvent_)
{
}

ChaosEvent & ChaosEvent::operator = (ChaosEvent const & other)
{
    if (this != &other)
    {
        chaosEvent_ = other.chaosEvent_;
    }

    return *this;
}

ErrorCode ChaosEvent::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_EVENT & publicChaosEvent) const
{
    if (chaosEvent_)
    {
        return chaosEvent_->ToPublicApi(heap, publicChaosEvent);
    }

    return ErrorCode::Success();
}

Common::ErrorCode ChaosEvent::FromPublicApi(
    FABRIC_CHAOS_EVENT const & publicChaosEvent)
{
    chaosEvent_ = ChaosEventBase::CreateSPtr(publicChaosEvent.Kind);
    auto error = chaosEvent_->FromPublicApi(publicChaosEvent);
    if (!error.IsSuccess())
    {
        return error;
    }

    return ErrorCode::Success();
}
