// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;

using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Federation;

#pragma warning(push)
#pragma warning(disable: 4355) // this used in base member initialization list
BackgroundWorkManagerWithRetry::BackgroundWorkManagerWithRetry(
    std::wstring const & id,
    std::wstring const & timerActivityIdPrefix,
    WorkFunctionPointer const & workFunction,
    TimeSpanConfigEntry const & minIntervalBetweenWork,
    TimeSpanConfigEntry const & retryInterval,
    ReconfigurationAgent & ra)
    : workFunction_(workFunction),
      bgm_(
            id,
            [this] (wstring const & activityId, ReconfigurationAgent & ra, BackgroundWorkManager &) 
            {
                this->OnBGMCallback_Internal(activityId, ra);
            },
            minIntervalBetweenWork,
            ra),
      retryTimer_(
            id,
            ra, 
            retryInterval,
            timerActivityIdPrefix,
            [this] (wstring const & activityId, ReconfigurationAgent &) 
            {
                this->OnRetryTimerCallback_Internal(activityId);
            })
{
}
#pragma warning(pop)

void BackgroundWorkManagerWithRetry::Request(std::wstring const & activityId)
{
    bgm_.Request(activityId);
}

void BackgroundWorkManagerWithRetry::SetTimer()
{
    retryTimer_.Set();
}

void BackgroundWorkManagerWithRetry::OnWorkComplete(bool isRetryRequired)
{
    bgm_.OnWorkComplete();

    if (isRetryRequired)
    {
        retryTimer_.Set();
    }
    else
    {
        retryTimer_.TryCancel(sequenceNumberWhenCallbackStarted_);
    }
}

void BackgroundWorkManagerWithRetry::OnWorkComplete(BackgroundWorkRetryType::Enum retryType)
{
    bool cancelTimer = false;
    bool setTimer = false;
    bool request = false;

    switch (retryType)
    {
    case BackgroundWorkRetryType::None:
        cancelTimer = true;
        break;
        
    case BackgroundWorkRetryType::Deferred:
        setTimer = true;
        break;

    case BackgroundWorkRetryType::Immediate:
        request = true;
        cancelTimer = true;
        break;

    default:
        Assert::CodingError("unknown enum value BGWRT {0}", static_cast<int>(retryType));
    }

    if (cancelTimer)
    {
        retryTimer_.TryCancel(sequenceNumberWhenCallbackStarted_);
    }

    if (setTimer)
    {
        retryTimer_.Set();
    }

    if (request)
    {
        bgm_.Request(lastActivityId_);
    }

    bgm_.OnWorkComplete();
}

void BackgroundWorkManagerWithRetry::Close()
{
    bgm_.Close();
    retryTimer_.Close();
}

void BackgroundWorkManagerWithRetry::OnBGMCallback_Internal(std::wstring const & activityId, ReconfigurationAgent & component)
{
    lastActivityId_ = activityId;
    sequenceNumberWhenCallbackStarted_ = retryTimer_.SequenceNumber;
    workFunction_(activityId, component, *this);
}

void BackgroundWorkManagerWithRetry::OnRetryTimerCallback_Internal(wstring const & activityId)
{
    bgm_.Request(activityId);
}
