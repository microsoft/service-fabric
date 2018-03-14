// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{     
    using namespace Common;
    using namespace Federation;
    using namespace std;
    using namespace Reliability;
    using namespace Transport;

    StringLiteral const TraceComponent("ProcessRequest");

    EntreeService::RequestAsyncOperationBase::RequestAsyncOperationBase(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        Common::TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ActivityTracedComponent(properties.InstanceString, FabricActivityHeader::FromMessage(*receivedMessage))
        , TimedAsyncOperation(timeout, callback, parent)
        , properties_(properties)
        , receivedMessage_(std::move(receivedMessage))
        , namingUri_()
        , reply_(nullptr)
        , retryTimerLock_()
        , retryTimer_()
        , retryCount_(0)
    {
    }

    EntreeService::RequestAsyncOperationBase::RequestAsyncOperationBase(
        __in GatewayProperties & properties,
        NamingUri const & name,
        FabricActivityHeader const & activityHeader,
        Common::TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ActivityTracedComponent(properties.InstanceString, activityHeader)
        , TimedAsyncOperation(timeout, callback, parent)
        , properties_(properties)
        , receivedMessage_(nullptr)
        , namingUri_(name)
        , reply_(nullptr)
        , retryTimerLock_()
        , retryTimer_()
        , retryCount_(0)
    {
    }

    EntreeService::RequestAsyncOperationBase::~RequestAsyncOperationBase()
    {
        WriteNoise(TraceComponent, "{0}: RequestAsyncOperationBase destructing", this->TraceId);
    }

    void EntreeService::RequestAsyncOperationBase::SetReplyAndComplete(
        AsyncOperationSPtr const & thisSPtr,
        Transport::MessageUPtr && reply, 
        Common::ErrorCode const & error)
    {
        if (this->TryStartComplete())
        {
            reply_ = move(reply);
            this->FinishComplete(thisSPtr, error);
        }
    }

    bool EntreeService::RequestAsyncOperationBase::CanConsiderSuccess()
    {
        return false;
    }

    bool EntreeService::RequestAsyncOperationBase::TryGetNameFromRequestBody()
    {
        if (receivedMessage_)
        {
            return receivedMessage_->GetBody(namingUri_);
        }
        else
        {
            // Assume name was set explicitly via ctor
            return true;
        }
    }

    ErrorCode EntreeService::RequestAsyncOperationBase::End(
        AsyncOperationSPtr const & operation,
        __out MessageUPtr & reply) 
    {        
        auto casted = AsyncOperation::End<RequestAsyncOperationBase>(operation);

        auto error = casted->Error;

        if (error.IsSuccess())
        {
            reply = std::move(casted->reply_);
        }
        else if (casted->CanConsiderSuccess())
        {
            WriteNoise(
                TraceComponent,
                "{0}: handling '{1}' as Success on retry#{2}",
                casted->TraceId,
                error,
                casted->retryCount_);

            reply = std::move(casted->reply_);
            error = ErrorCodeValue::Success;
        }

        if (error.IsSuccess())
        {
            if (reply)
            {
                WriteNoise(
                    TraceComponent,
                    "{0}: RequestAsyncOperationBase: processing succeeded",
                    casted->TraceId);
            }
            else
            {
                TRACE_INFO_AND_TESTASSERT(
                    TraceComponent,
                    "{0}: RequestAsyncOperationBase: NULL reply message on success",
                    casted->TraceId);
            }
        }

        return error;
    }    

    void EntreeService::RequestAsyncOperationBase::OnRetry(Common::AsyncOperationSPtr const &)
    {
        // intentional no-op
    }

    void EntreeService::RequestAsyncOperationBase::OnStart(Common::AsyncOperationSPtr const & thisSPtr)
    {
            this->OnStartRequest(thisSPtr);
    }

    void EntreeService::RequestAsyncOperationBase::OnCancel()
    {
        this->CancelRetryTimer();
    }

    void EntreeService::RequestAsyncOperationBase::OnCompleted()
    {
        TimedAsyncOperation::OnCompleted();

        this->CancelRetryTimer();

        switch (this->Error.ReadValue())
        {
        case ErrorCodeValue::NameNotFound:
        case ErrorCodeValue::UserServiceNotFound:
        case ErrorCodeValue::PartitionNotFound:
        //
        // Faster cleanup of PSD cache if a delayed PSD read 
        // request re-introduces a PSD into the cache after it's
        // deleted by broadcast
        //
        case ErrorCodeValue::ServiceOffline:
            this->Properties.PsdCache.TryRemove(this->Name.Name);
            break;
        }
    }

    void EntreeService::RequestAsyncOperationBase::CancelRetryTimer()
    {
        TimerSPtr snap;
        {
            AcquireExclusiveLock lock(retryTimerLock_);
            snap = move(retryTimer_);
        }

        if (snap)
        {
            snap->Cancel();
        }
    }

    void EntreeService::RequestAsyncOperationBase::HandleRetryStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->TryHandleRetryStart(thisSPtr);
    }

    bool EntreeService::RequestAsyncOperationBase::TryHandleRetryStart(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan remainingTime = RemainingTime;
        if (RemainingTime <= TimeSpan::Zero)
        {
            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Timeout));

            return false;
        }

        TimeSpan timeout = Properties.OperationRetryInterval;
        if (remainingTime < timeout)
        {
            timeout = remainingTime;
        }

        {
            AcquireExclusiveLock lock(retryTimerLock_);

            if (!this->InternalIsCompleted)
            {
                ++retryCount_;
                retryTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const & timer)
                    {
                        timer->Cancel();
                        OnRetry(thisSPtr);
                    },
                    true); // allow concurrency

                retryTimer_->Change(timeout);

                return true;
            }
        }

        return false;
    }

    void EntreeService::RequestAsyncOperationBase::RouteToNode(
        Transport::MessageUPtr && request,
        Federation::NodeId const & nodeId,
        uint64 instanceId,
        bool useExactRouting,
        Common::AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = Properties.InnerRingCommunication.BeginRouteRequest(
            move(request),
            nodeId,
            instanceId,
            useExactRouting,
            RemainingTime,
            [this] (AsyncOperationSPtr const & operation) { this->OnRouteToNodeRequestComplete(operation, false /*expectedCompletedSynchronously*/); },
            thisSPtr);

        this->OnRouteToNodeRequestComplete(operation, true /*expectedCompletedSynchronously*/);
    }

    void EntreeService::RequestAsyncOperationBase::OnRouteToNodeRequestComplete(Common::AsyncOperationSPtr const & asyncOperation, bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        ErrorCode error = Properties.InnerRingCommunication.EndRouteRequest(asyncOperation, reply);

        if (error.IsSuccess())
        {
            error = RequestInstanceTrackerHelper::UpdateInstanceOnStaleRequestFailureReply(reply, this->Properties.RequestInstance);
        }

        if (error.IsSuccess())
        {
            OnRouteToNodeSuccessful(asyncOperation->Parent, reply);            
        }
        else if (this->IsRetryable(error))
        {
            OnRouteToNodeFailedRetryableError(asyncOperation->Parent, reply, error);
        }
        else 
        {
            OnRouteToNodeFailedNonRetryableError(asyncOperation->Parent, error);
        }

    }

    void EntreeService::RequestAsyncOperationBase::OnRouteToNodeSuccessful(
        Common::AsyncOperationSPtr const & thisSPtr,
        Transport::MessageUPtr & reply)
    {
        this->ProcessReply(reply);

        this->SetReplyAndComplete(thisSPtr, std::move(reply), ErrorCode::Success());
    }

    void EntreeService::RequestAsyncOperationBase::OnRouteToNodeFailedRetryableError(
        Common::AsyncOperationSPtr const & thisSPtr,
        Transport::MessageUPtr & ,
        Common::ErrorCode const & error)
    {
        WriteNoise(
            TraceComponent, 
            "{0}: Route to node - retrying. error = {1}", 
            this->TraceId,
            error);

        this->HandleRetryStart(thisSPtr);
    }

    void EntreeService::RequestAsyncOperationBase::OnRouteToNodeFailedNonRetryableError(
        Common::AsyncOperationSPtr const & thisSPtr,
        Common::ErrorCode const & error)
    {
        TryComplete(thisSPtr, error);
    }

    bool EntreeService::RequestAsyncOperationBase::IsRetryable(ErrorCode const & error)
    {
        return NamingErrorCategories::IsRetryableAtGateway(error);
    }
}
