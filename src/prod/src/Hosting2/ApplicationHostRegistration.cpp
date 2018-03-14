// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceType_ApplicationHostRegistration("ApplicationHostRegistration");

ApplicationHostRegistration::ApplicationHostRegistration(
    wstring const & hostId, 
    ApplicationHostType::Enum hostType)
    : lock_(),
    hostId_(hostId),
    hostType_(hostType),
    status_(ApplicationHostRegistrationStatus::NotRegistered),
    hostProcessMonitor_(),
    finishRegistrationTimer_()
{
}

ApplicationHostRegistration::~ApplicationHostRegistration()
{
    if (hostProcessMonitor_)
    {
        hostProcessMonitor_->Cancel();
    }
    if (finishRegistrationTimer_)
    {
        finishRegistrationTimer_->Cancel();
    }
}

Common::ErrorCode ApplicationHostRegistration::OnStartRegistration(Common::TimerSPtr && timer)
{
    {
        AcquireWriteLock lock(lock_);
        if (status_ != ApplicationHostRegistrationStatus::NotRegistered)
        {
            WriteWarning(
                TraceType_ApplicationHostRegistration,
                "The registration status for host Id={0}, Type={1} should be NotRegistered, it is {2}",
                hostId_,
                hostType_,
                status_);

            return ErrorCode(ErrorCodeValue::InvalidState);
        }

        finishRegistrationTimer_ = move(timer);
        status_ = ApplicationHostRegistrationStatus::InProgress;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode ApplicationHostRegistration::OnMonitoringInitialized(Common::ProcessWaitSPtr && processMonitor)
{
    {
        AcquireWriteLock lock(lock_);
        if (status_ != ApplicationHostRegistrationStatus::InProgress)
        {
            WriteWarning(
                TraceType_ApplicationHostRegistration,
                "The registration status for host Id={0}, Type={1} should be InProgress, it is {2}",
                hostId_,
                hostType_,
                status_);

            return ErrorCode(ErrorCodeValue::InvalidState);
        }

        hostProcessMonitor_ = move(processMonitor);
        status_ = ApplicationHostRegistrationStatus::InProgress_Monitored;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode ApplicationHostRegistration::OnFinishRegistration()
{
    {
        AcquireWriteLock lock(lock_);
        if (hostType_ == ApplicationHostType::Enum::NonActivated)
        {
            if(status_ != ApplicationHostRegistrationStatus::InProgress_Monitored)

            {
                WriteWarning(
                    TraceType_ApplicationHostRegistration,
                    "The registration status for host Id={0}, Type={1} should be InProgress_Monitored, it is {2}",
                    hostId_,
                    hostType_,
                    status_);

                return ErrorCode(ErrorCodeValue::InvalidState);
            }
        }
        else if(status_ != ApplicationHostRegistrationStatus::InProgress)
        {
                            WriteWarning(
                    TraceType_ApplicationHostRegistration,
                    "The registration status for host Id={0}, Type={1} should be InProgress_Monitored, it is {2}",
                    hostId_,
                    hostType_,
                    status_);

                return ErrorCode(ErrorCodeValue::InvalidState);
        }
        status_ = ApplicationHostRegistrationStatus::Completed;
    }

    if (finishRegistrationTimer_)
    {
        finishRegistrationTimer_->Cancel();
    }
    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode ApplicationHostRegistration::OnRegistrationTimedout()
{
    {
        AcquireWriteLock lock(lock_);
        if ((status_ != ApplicationHostRegistrationStatus::InProgress_Monitored) &&
            (status_ != ApplicationHostRegistrationStatus::InProgress))
        {
            WriteWarning(
                TraceType_ApplicationHostRegistration,
                "The registration status for host Id={0}, Type={1} should be InProgress or InProgress_Monitored, it is {2}",
                hostId_,
                hostType_,
                status_);

            return ErrorCode(ErrorCodeValue::InvalidState);
        }

        status_ = ApplicationHostRegistrationStatus::Timedout;
    }

    return ErrorCode(ErrorCodeValue::Success);
}


ApplicationHostRegistrationStatus::Enum ApplicationHostRegistration::GetStatus() const
{
    {
        AcquireReadLock lock(lock_);
        return status_;
    }
}

void ApplicationHostRegistration::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("ApplicationHostRegistration { ");
    w.Write("HostId = {0}", HostId);
    w.Write("HostType = {0}", HostType);
    w.Write("Status = {0}", GetStatus());
    w.Write("}");
}
