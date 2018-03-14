// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace LoggingReplicatorTests;

#define APIFAULTUTILITY_TAG 'FipA'

ULONG const waitQuantumMs = 1;

ApiFaultUtility::ApiFaultUtility()
    : KObject()
    , lock_()
    , faultData_(GetThisAllocator())
{
}

ApiFaultUtility::~ApiFaultUtility()
{
}

ApiFaultUtility::SPtr ApiFaultUtility::Create(__in KAllocator & allocator)
{
    ApiFaultUtility * pointer = _new(APIFAULTUTILITY_TAG, allocator)ApiFaultUtility();

    ASSERT_IFNOT(
        pointer != nullptr,
        "ApiFaultUtility: allocation failure");

    return ApiFaultUtility::SPtr(pointer);
}

Awaitable<NTSTATUS> ApiFaultUtility::WaitUntilSignaledAsync(__in ApiName apiName)
{
    Common::TimeSpan durationToWait = Common::TimeSpan::Zero;

    Common::Stopwatch waitedDuration;
    waitedDuration.Start();

    do
    {
        FaultInfo faultInfo = GetFaultInfo(apiName);

        if (durationToWait != faultInfo.delayDuration_)
        {
            // reset duration to wait if value has changed
            // This is to ensure we stop waiting immediately if a block was induced on an API
            durationToWait = faultInfo.delayDuration_;
        }

        if (durationToWait > waitedDuration.Elapsed)
        {
            NTSTATUS status = co_await KTimer::StartTimerAsync(GetThisAllocator(), APIFAULTUTILITY_TAG, waitQuantumMs, nullptr);

            ASSERT_IFNOT(
                status == STATUS_SUCCESS,
                "Invalid status starting timer in api fault utility: {0}", status);

            // decrement the overall duration to wait
            durationToWait = durationToWait - Common::TimeSpan::FromMilliseconds(waitQuantumMs);
        }
        else
        {
            if (!NT_SUCCESS(faultInfo.errorCode_))
            {
                co_return faultInfo.errorCode_;
            }
            break;
        }
    } while (true);

    co_return STATUS_SUCCESS;
}

NTSTATUS ApiFaultUtility::WaitUntilSignaled(__in ApiName apiName)
{
    Common::TimeSpan durationToWait = Common::TimeSpan::Zero;

    Common::Stopwatch waitedDuration;
    waitedDuration.Start();

    do
    {
        FaultInfo faultInfo = GetFaultInfo(apiName);

        if (durationToWait != faultInfo.delayDuration_)
        {
            // reset duration to wait if value has changed
            // This is to ensure we stop waiting immediately if a block was induced on an API
            durationToWait = faultInfo.delayDuration_;
        }

        if (durationToWait > waitedDuration.Elapsed)
        {
            Sleep(waitQuantumMs);
            // decrement the overall duration to wait
            durationToWait = durationToWait - Common::TimeSpan::FromMilliseconds(waitQuantumMs);
        }
        else
        {
            if (!NT_SUCCESS(faultInfo.errorCode_))
            {
                return faultInfo.errorCode_;
            }
            break;
        }
    } while (true);

    return STATUS_SUCCESS;
}

void ApiFaultUtility::BlockApi(__in ApiName apiName)
{
    FaultInfo faultInfo(apiName, Common::TimeSpan::MaxValue, STATUS_SUCCESS);
    AddFaultInfo(faultInfo);
}

void ApiFaultUtility::DelayApi(
    __in ApiName apiName,
    __in Common::TimeSpan const & duration)
{
    FaultInfo faultInfo(apiName, duration, STATUS_SUCCESS);
    AddFaultInfo(faultInfo);
}

void ApiFaultUtility::FailApi(
    __in ApiName apiName,
    __in NTSTATUS errorCode)
{
    FaultInfo faultInfo(apiName, Common::TimeSpan::Zero, errorCode);
    AddFaultInfo(faultInfo);
}

void ApiFaultUtility::ClearFault(__in ApiName apiName)
{
    K_LOCK_BLOCK(lock_)
    {
        for (ULONG i = 0; i < faultData_.Count(); i++)
        {
            if (faultData_[i].apiName_ == apiName)
            {
                faultData_.Remove(i);
                return;
            }
        }
    }

    ASSERT_IFNOT(false, "ApiFaultUtility ClearFault called while there is no fault present.");
}

void ApiFaultUtility::AddFaultInfo(__in FaultInfo const & faultInfo)
{
    K_LOCK_BLOCK(lock_)
    {
        for (ULONG i = 0; i < faultData_.Count(); i++)
        {
            if (faultData_[i].apiName_ == faultInfo.apiName_)
            {
                // Fault already present. Replace it
                faultData_.Remove(i); 
                faultData_.Append(faultInfo);
                return;
            }
        }

        faultData_.Append(faultInfo);
    }
}

ApiFaultUtility::FaultInfo ApiFaultUtility::GetFaultInfo(__in ApiName apiName)
{
    K_LOCK_BLOCK(lock_)
    {
        for (ULONG i = 0; i < faultData_.Count(); i++)
        {
            if (faultData_[i].apiName_ == apiName)
            {
                return faultData_[i];
            }
        }
    }
    
    // If faultdata is not found, there is no fault to induce
    FaultInfo faultInfo(apiName, Common::TimeSpan::Zero, STATUS_SUCCESS);
    return faultInfo;
}
