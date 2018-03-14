// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Transport;
using namespace Federation;

static const StringLiteral BroadcastRetryTimerTag("BroadcastRetry");

BroadcastReplyContext::BroadcastReplyContext(
    BroadcastManager & manager,
    MessageUPtr && original,
    MessageId broadcastId,
    TimeSpan retryInterval)
    :   manager_(manager),
        replies_(ReaderQueue<Message>::Create()),
        original_(std::move(original)),
        broadcastId_(broadcastId),
        retryInterval_(retryInterval),
        replyCount_(0),
        readCount_(0),
        closed_(false)
{
    ASSERT_IF(!original_->MessageId.IsEmpty(),
        "Broadcast original message has message id: {0}",
        *original_);

    BroadcastManager::WriteInfo(
            "Start",
            "Started broadcast {0}",
            this->broadcastId_);
}

BroadcastReplyContext::~BroadcastReplyContext()
{
}

void BroadcastReplyContext::Start()
{
    auto thisSPtr = this->shared_from_this();

    this->retryTimer_ = Common::Timer::Create(
        BroadcastRetryTimerTag,
        [thisSPtr](TimerSPtr const & timer)
        {
            thisSPtr->OnTimerCallback(timer);
        },
        true);

    this->retryTimer_->Change(this->retryInterval_, this->retryInterval_);
}

Common::AsyncOperationSPtr BroadcastReplyContext::BeginReceive(TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
{
    return this->replies_->BeginDequeue(timeout, callback, parent);
}

Common::ErrorCode BroadcastReplyContext::EndReceive(AsyncOperationSPtr const & operation, __out MessageUPtr & message, __out NodeInstance & replySender)
{
    ++this->readCount_;

    auto result = this->replies_->EndDequeue(operation, message);
    if (result.IsSuccess() && message /*last reply message is null to indicate completion*/)
    {
        PToPHeader pointToPointHeader;
        bool foundHeader = message->Headers.TryReadFirst(pointToPointHeader);
        ASSERT_IFNOT(foundHeader, "PToPHeader missing on broadcast reply");
        replySender = pointToPointHeader.From;
    }

    return result;
}

void BroadcastReplyContext::OnReply(MessageUPtr && message, NodeIdRange const & range)
{
    auto messageId = message->MessageId;

    ++this->replyCount_;
    this->replies_->EnqueueAndDispatch(std::move(message));

    bool areAllrangesReceived;
    {
        AcquireExclusiveLock lock(this->lock_);

        if (this->rangeTable_.AreAllRangesReceived)
        {
            // This context has already been completed
            return;
        }

        NodeIdRangeTable oldTable(this->rangeTable_);
        this->rangeTable_.AddRange(range);

        BroadcastManager::WriteInfo(
            "Range",
            "Adding range {0} to {1} for broadcast id {2} with message {3}, remaining {4}",
            range,
            oldTable,
            this->broadcastId_,
            messageId,
            this->rangeTable_);

        areAllrangesReceived = this->rangeTable_.AreAllRangesReceived;
    }

    if (areAllrangesReceived)
    {
        BroadcastManager::WriteInfo(
            "Complete",
            "Broadcast context completed for broadcast id {0} with {1} replies and {2} read",
            this->broadcastId_,
            this->replyCount_.load(),
            this->readCount_.load());

        // this can only happen once, and all messages have been added to the queue already so our null message can be added
        // to singal to the user that it has completed
        this->replies_->EnqueueAndDispatch(MessageUPtr());
    }
}

void BroadcastReplyContext::Close()
{
    {
        AcquireExclusiveLock lock(this->lock_);
        if (this->closed_)
        {
            return;
        }

        this->closed_ = true;
    }

    this->replies_->Close();

    BroadcastManager::WriteInfo(
            "Close",
            "Closing context for broadcast id {0} with {1} replies and {2} read",
            this->broadcastId_,
            this->replyCount_.load(),
            this->readCount_.load());

    if (this->retryTimer_)
    {
        this->retryTimer_->Cancel();
    }

    this->manager_.OnBroadcastCompleted(this->broadcastId_);

    return;
}

void BroadcastReplyContext::Cancel()
{
    {
        AcquireExclusiveLock lock(this->lock_);
        if (this->closed_)
        {
            return;
        }

        this->closed_ = true;
    }

    this->replies_->Abort();

    BroadcastManager::WriteInfo(
            "Cancel",
            "Canceling context for broadcast id {0} with {1} replies and {2} read",
            this->broadcastId_,
            this->replyCount_.load(),
            this->readCount_.load());

    if (this->retryTimer_)
    {
        this->retryTimer_->Cancel();
    }

    // signal to the user to call Close()
    this->replies_->EnqueueAndDispatch(MessageUPtr());
}

void BroadcastReplyContext::OnTimerCallback(TimerSPtr const &)
{
    NodeIdRangeTable ranges;
    {
        AcquireReadLock lock(this->lock_);
        if (closed_)
        {
            return;
        }

        ranges = this->rangeTable_;
    }

    for (NodeIdRange const & range : ranges)
    {
        // Original message should still have broadcast headers
        this->manager_.RouteBroadcastMessageToHole(this->original_->Clone(), this->broadcastId_, range, this->retryInterval_);
    }
}
