// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Federation
{
    using namespace std;
    using namespace Common;
    using namespace Transport;

    static const StringLiteral RoutingTimerTag("Routing");

    RouteAsyncOperation::RouteAsyncOperation(
        SiteNodeSPtr siteNode,
        MessageUPtr && message,
        NodeId nodeId,
        TimeSpan retryTimeout,
        RoutingHeader const & header,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent)
    , siteNode_(siteNode)
    , message_(std::move(message))
    , header_(header)
    , nodeId_(nodeId)
    , retryTimeout_(retryTimeout)
    , timeoutHelper_(timeout)
    , timer_()
    , retryCount_(0)
    {
    }

    RouteAsyncOperation::~RouteAsyncOperation()
    {
    }

    Common::ErrorCode RouteAsyncOperation::End(AsyncOperationSPtr const & asyncOperation, MessageUPtr & reply)
    {
        auto error = AsyncOperation::End<RouteAsyncOperation>(asyncOperation)->Error;
        reply = std::move(AsyncOperation::Get<RouteAsyncOperation>(asyncOperation)->reply_);

        return error;
    }

    void RouteAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->MakeRequest(thisSPtr, true);
    }

    void RouteAsyncOperation::MakeRequest(AsyncOperationSPtr const & thisSPtr, bool newRequest)
    {
        bool ownsToken;
        PartnerNodeSPtr to = siteNode_->Table.GetRoutingHop(this->nodeId_, header_.ToRing, retryCount_ >= 3, ownsToken);
        if (!to)
        {
            this->OnNoRoutingPartner(thisSPtr);
            return;
        }

        TimeSpan remainingTime = this->timeoutHelper_.GetRemainingTime();
        if (remainingTime <= TimeSpan::Zero)
        {
            this->CompleteWithTimeout(thisSPtr);
            return;
        }

        TimeSpan timeout;
        LONG retryCount;
        MessageUPtr request;
        {
            AcquireExclusiveLock lock(this->lock_);
            newRequest = (newRequest || retryCount_ == 0);
            bool doSend = (ownsToken || (to.get() != siteNode_.get()));
            bool doRetry = (remainingTime > this->retryTimeout_);

            if (newRequest && doSend && !doRetry)
            {
                timeout = remainingTime;
            }
            else
            {
                if (!timer_)
                {
                    this->timer_ = Common::Timer::Create(
                        RoutingTimerTag,
                        [thisSPtr](TimerSPtr const &) -> void { RouteAsyncOperation::OnRetryCallback(thisSPtr); },
                        true);
                }

                timer_->Change(min(this->retryTimeout_, remainingTime));
                timeout = TimeSpan::MaxValue;
            }

            if (doSend)
            {
                request = message_->Clone();

                this->header_.Expiration = remainingTime;
                request->Headers.Add(this->header_);

                if (retryCount_ != 0)
                {
                    request->Headers.Add(RetryHeader(retryCount_));
                }

                retryCount_++;
            }

            retryCount = retryCount_;
        }

        if (request)
        {
            if (newRequest)
            {
                RoutingManager::WriteNoise(
                    "Request",
                    "Making request with message {0} to {1} with {2} remaining",
                    message_->MessageId,
                    to->Instance,
                    remainingTime);

                siteNode_->pointToPointManager_->BeginPToPSendRequest(
                    move(request),
                    to,
                    false,
                    PToPActor::Routing,
                    timeout,
                    [to] (AsyncOperationSPtr const & operation)
                    { 
                        RouteAsyncOperation * routeAsyncOperation = AsyncOperation::Get<RouteAsyncOperation>(operation->Parent);
                        routeAsyncOperation->OnRequestCallback(operation, to);
                    },
                    thisSPtr);
            }
            else
            {
                RoutingManager::WriteInfo(
                    "Retry",
                    "Retry message {0}({1}) to {2} with {3} remaining",
                    message_->MessageId,
                    retryCount,
                    to->Instance,
                    remainingTime);

                siteNode_->pointToPointManager_->RetryPToPSendRequest(
                    move(request),
                    to, 
                    false,
                    PToPActor::Routing,
                    retryCount);
            }
        }
        else
        {
            RoutingManager::WriteInfo(
                "Hold",
                "Skip message {0}({1}) to local node",
                message_->MessageId,
                retryCount_);
        }
    }

    void RouteAsyncOperation::StopTimer()
    {
        if (this->timer_)
        {
            this->timer_->Cancel();
            this->timer_.reset();
        }
    }
    
    void RouteAsyncOperation::CompleteWithTimeout(AsyncOperationSPtr const & operation)
    {
        // TODO: Consider not completing here, but propagating the timeout error code to the completion of the request so OnRequestCallback will call complete.
        // Right now it will trace that it failed to complete with "OperationCanceled"
        if (this->TryComplete(operation, ErrorCode(ErrorCodeValue::Timeout)))
        {
            RoutingManager::WriteInfo(
                "Complete",
                "Message {0}({1}) header {2} completed with timeout",
                this->message_->MessageId,
                this->header_,
                this->retryCount_);
        }

        this->siteNode_->pointToPointManager_->CancelPToPRequest(this->message_->MessageId);

        AcquireExclusiveLock lock(this->lock_);
        this->StopTimer();
    }

    void RouteAsyncOperation::OnNoRoutingPartner(AsyncOperationSPtr const & operation)
    {
        // Unable to find someone to route to
        RoutingManager::WriteInfo(
            "Fault",
            "Unable to find a routing partner for message {0}({1})",
            message_->MessageId,
            retryCount_);

        this->TryComplete(operation, ErrorCode(ErrorCodeValue::OperationFailed));

        this->siteNode_->pointToPointManager_->CancelPToPRequest(this->message_->MessageId);

        AcquireExclusiveLock lock(this->lock_);
        this->StopTimer();
    }

    void RouteAsyncOperation::OnRetryCallback(AsyncOperationSPtr const & operation)
    {
        RouteAsyncOperation * routeAsyncOperation = AsyncOperation::Get<RouteAsyncOperation>(operation);
        routeAsyncOperation->MakeRequest(operation, false);
    }

    void RouteAsyncOperation::OnRequestCallback(AsyncOperationSPtr const & operation, PartnerNodeSPtr const & hopTo)
    {
        ErrorCode error = siteNode_->pointToPointManager_->EndPToPSendRequest(operation, reply_);
        {
            AcquireExclusiveLock lock(lock_);
            StopTimer();
        }

        if (error.IsSuccess())
        {
            hopTo->OnReceive(false);
        }

        if (!error.IsSuccess() && RoutingManager::IsRetryable(error, message_->Idempotent) && CanRetry())
        {
            RoutingManager::WriteInfo(
                "Retry",
                "Request failed for {0}({1}) with {2}.  Retrying...",
                message_->MessageId,
                retryCount_,
                error);

            MakeRequest(operation->Parent, true);
        }
        else
        {
            TryComplete(operation->Parent, error);
            if (!error.IsSuccess())
            {
                RoutingManager::WriteInfo(
                    "Complete",
                    "Routing of message {0}({1}) header {2} failed with {3}",
                    message_->MessageId, retryCount_, header_, error);
            }

            if (error.IsError(ErrorCodeValue::RoutingNodeDoesNotMatchFault) &&
                header_.UseExactRouting &&
                header_.To.InstanceId > 0)
            {
                siteNode_->Table.SetShutdown(header_.To, header_.ToRing);
            }
        }
    }
}
