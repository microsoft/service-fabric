// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Transport;
using namespace Hosting2;
using namespace Management;
using namespace ImageModel;

StringLiteral const TraceType_TraceSessionManager("TraceSessionManager");

TraceSessionManager::TraceSessionManager(
    ComponentRoot const & root) 
    : RootedObject(root),
    pollingInterval_(Common::TimeSpan::FromSeconds(30))
{
}

TraceSessionManager::~TraceSessionManager()
{
}

Common::ErrorCode TraceSessionManager::OnOpen()
{
#ifndef PLATFORM_UNIX
    timer_ = Common::Timer::Create(
        TimerTagDefault,
        [this](TimerSPtr const &)
        {
            this->OnTimer();
        }, false);

    AcquireExclusiveLock lock(processingLock_);
    if (!this->IsOpeningOrOpened())
    {
        return ErrorCodeValue::FabricComponentAborted;
    }

    auto error = Common::TraceSession::Instance()->StopTraceSessions();
    error = Common::TraceSession::Instance()->StartTraceSessions();
    timer_->Change(pollingInterval_, pollingInterval_);
    return error;
#else
    return ErrorCodeValue::Success;
#endif
}

Common::ErrorCode TraceSessionManager::OnClose()
{
#ifndef PLATFORM_UNIX
    if (timer_)
    {
        timer_->Cancel();
    }

    // Make sure work is finished
    AcquireExclusiveLock lock(processingLock_);
    return Common::TraceSession::Instance()->StopTraceSessions();
#else
    return ErrorCodeValue::Success;
#endif
}

void TraceSessionManager::OnAbort()
{
    WriteInfo(TraceType_TraceSessionManager,
        Root.TraceId,
        "Aborting TraceSessionManager");
    OnClose();
}

void TraceSessionManager::OnTimer()
{
    // Indicate that we are doing work.
#ifndef PLATFORM_UNIX
    AcquireExclusiveLock lock(processingLock_);

    if (!IsOpened())
    {
        return;
    }

    // Start will update only if necessary
    auto err = Common::TraceSession::Instance()->StartTraceSessions();
    if (!err.IsSuccess())
    {
        WriteWarning(TraceType_TraceSessionManager, Root.TraceId, "Unable to start trace sessions {0}.", err.ErrorCodeValueToString());
        return;
    }

    WriteNoise(TraceType_TraceSessionManager, Root.TraceId, "Trace sessions successfully started.");
#endif
}
