// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::AcquireExclusiveLock;
using Common::AcquireReadLock;
using Common::Assert;
using Common::DateTime;
using Common::Stopwatch;
using Common::RwLock;
using Common::StringWriter;
using Common::Timer;
using Common::TimerSPtr;
using Common::TimeSpan;

using std::wstring;

AckSender::AckSender(
    REInternalSettingsSPtr const & config,
    Common::Guid const & partitionId, 
    ReplicationEndpointId const & endpointUniqueId)
    :   config_(config),
        partitionId_(partitionId),
        endpointUniqueId_(endpointUniqueId),
        sendCallback_(),
        lastTraceTime_(),
        isActive_(false),
        forceSendAckOnMaxPendingItems_(
            (config->BatchAcknowledgementInterval > TimeSpan::Zero) && 
            (config->MaxPendingAcknowledgements > 0)),
        batchSendTimer_(),
        timerActive_(false),
        lock_(),
        lastAckSentAt_(DateTime::Zero)
{
}

AckSender::~AckSender()
{
    AcquireExclusiveLock lock(lock_);
    ASSERT_IF(
        isActive_,
        "{0}: AckSender.dtor: isActive is true, Close should have been called",
        endpointUniqueId_);
    ASSERT_IF(
        batchSendTimer_,
        "{0}: AckSender.dtor: Timer not null, Close should have been called",
        endpointUniqueId_);
}


DateTime AckSender::get_LastAckSentAt() const
{
    AcquireReadLock lock(lock_);
    return lastAckSentAt_;
}

void AckSender::Open(
    Common::ComponentRoot const & root, 
    SendCallback const & sendCallback)
{
    ASSERT_IF(config_->BatchAcknowledgementInterval < TimeSpan::Zero, "{0}: BatchAcknowledgementInterval can't be negative", endpointUniqueId_);
    ASSERT_IF(config_->BatchAcknowledgementInterval >= config_->RetryInterval, "BatchAcknowledgementInterval must be less than RetryInterval");

    bool createTimer = config_->BatchAcknowledgementInterval > TimeSpan::Zero;
    ReplicatorEventSource::Events->AckSenderPolicy(
        partitionId_, 
        endpointUniqueId_,
        createTimer);

    lastTraceTime_.Start();

    {
        AcquireExclusiveLock lock(lock_);
        ASSERT_IF(isActive_, "{0}: Open called but active is true", endpointUniqueId_);
        isActive_ = true;
        if (createTimer)
        {
            auto componentRoot = root.CreateComponentRoot();
            batchSendTimer_ = Timer::Create(
                TimerTagDefault,
                [this, componentRoot](TimerSPtr const &)
            {
                this->OnTimerCallback();
            },
                true);
        }

        sendCallback_ = sendCallback;
    }
}

void AckSender::Close()
{
    bool sendFinalAck = false;
    {
        AcquireExclusiveLock lock(lock_);
        if (isActive_)
        {
            isActive_ = false;
            if (batchSendTimer_)
            {
                batchSendTimer_->Cancel();
                batchSendTimer_ = nullptr;
                // If ACKs are pending, send them immediately
                sendFinalAck = timerActive_;
                timerActive_ = false;

                if (sendFinalAck)
                {
                    lastAckSentAt_ = DateTime::Now();
                }
            }
        }
    }

    if (sendFinalAck)
    {
        sendCallback_(true);
    }
}

void AckSender::OnTimerCallback()
{
    bool callTraceMethod = false;

    {
        AcquireExclusiveLock lock(lock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->AckSenderNotActive(
                partitionId_, 
                endpointUniqueId_);
            return;
        }

        timerActive_ = false;
        callTraceMethod = ShouldTraceCallerHoldsLock();
        lastAckSentAt_ = DateTime::Now();
    }

    sendCallback_(callTraceMethod);
}

void AckSender::ScheduleOrSendAck(bool forceSend)
{
    bool sendNow = true;
    bool callTraceMethod = false;

    {
        AcquireExclusiveLock lock(lock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->AckSenderNotActive(
                partitionId_, 
                endpointUniqueId_);
            return;
        }

        if (batchSendTimer_)
        {
            if (forceSend)
            {
                // Disable timer if started
                if (timerActive_)
                {
                    ReplicatorEventSource::Events->AckSenderForceSend(
                        partitionId_,
                        endpointUniqueId_);
                    batchSendTimer_->Change(TimeSpan::MaxValue);
                    timerActive_ = false;                    
                }

                lastAckSentAt_ = DateTime::Now();
            }
            else
            {
                sendNow = false;

                // Start the batch timer, if is not already started
                // The message will be sent when the timer fires.
                if (!timerActive_)
                {
                    // Start the timer
                    batchSendTimer_->Change(config_->BatchAcknowledgementInterval);
                    timerActive_ = true;
                }  
            }
        }
        else
        {
            lastAckSentAt_ = DateTime::Now();
        }

        callTraceMethod = ShouldTraceCallerHoldsLock();
    }
 
    if (sendNow)
    {
        // AckSender is active and the callback should be sent immediately
        sendCallback_(callTraceMethod);
    }
}

bool AckSender::ShouldTraceCallerHoldsLock()
{
    if (config_->TraceInterval == TimeSpan::Zero)
    {
        // If someone dynamically changed this to zero, return false right here
        return false;
    }

    if (lastTraceTime_.Elapsed > config_->TraceInterval)
    {
        lastTraceTime_.Restart();
        return true;
    }

    return false;
}

} // end namespace ReplicationComponent
} // end namespace Reliability
