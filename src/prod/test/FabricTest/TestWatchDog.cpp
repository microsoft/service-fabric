// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace FabricTest;
using namespace TestCommon;
using namespace ServiceModel;
using namespace Api;

int64 TestWatchDog::Sequence = 0;

TestWatchDog::TestWatchDog(FabricTestDispatcher & dispatcher, wstring const & sourceId, TimeSpan reportInterval)
    :   dispatcher_(dispatcher),
        healthTable_(dispatcher.FabricClientHealth.HealthTable),
        sourceId_(sourceId),
        reportInterval_(reportInterval),
        active_(false)
{
    ComPointer<IFabricHealthClient4> result;
    HRESULT hr = TestFabricClient::FabricCreateComFabricClient(dispatcher.Federation)->QueryInterface(
        IID_IFabricHealthClient4,
        (void **)result.InitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricHealthClient");

    healthClient_ = result;
}

int64 TestWatchDog::GetSequence()
{
    return InterlockedIncrement64(&Sequence);
}

void TestWatchDog::Report(FABRIC_HEALTH_REPORT const & report)
{
    healthClient_->ReportHealth2(&report, NULL);
}

void TestWatchDog::Start(TestWatchDogSPtr const & thisSPtr)
{
    active_ = true;
    timer_ = Timer::Create(
        TimerTagDefault,
        [thisSPtr] (TimerSPtr const&) mutable { thisSPtr->Run(); }, true);
    timer_->Change(reportInterval_);
}

void TestWatchDog::Stop()
{
    active_ = false;
    timer_->Cancel();
}

void TestWatchDog::Run()
{
    if (active_)
    {
        healthTable_->Report(*this);
        timer_->Change(reportInterval_);
    }
}
