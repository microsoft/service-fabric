// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace std;
using namespace Store;
using namespace SystemServices;
using namespace Transport;

namespace Naming
{
    StringLiteral const TraceComponent("ProcessRequest");
    StringLiteral const FailedTraceComponent("ProcessRequestFailed");

    StoreService::ProcessRequestAsyncOperation::ProcessRequestAsyncOperation(
        MessageUPtr && request,
        __in NamingStore & namingStore,
        StoreServiceProperties & properties,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : AsyncOperation(callback, root)
        , ActivityTracedComponent(namingStore.TraceId, FabricActivityHeader::FromMessage(*request))
        , request_(std::move(request))
        , namingStore_(namingStore)
        , properties_(properties)
        , timeoutHelper_(timeout)
        , name_()
        , requestInstance_(-1)
        , isPrimaryRecovery_(false)
        , stopwatch_()
        , nameString_()
        , isForce_(false)
        , lastNameLocationVersion_()
        , retryTimer_()
        , timerLock_()
        , replyResult_()
    {
        stopwatch_.Start();
    }

    ErrorCode StoreService::ProcessRequestAsyncOperation::End(AsyncOperationSPtr const & operation, __out MessageUPtr & result)
    {
        auto casted = AsyncOperation::End<ProcessRequestAsyncOperation>(operation);
        result = std::move(casted->replyResult_);

        ErrorCode error = casted->Error;
        if (error.IsSuccess())
        {
            if (result)
            {
                WriteNoise(
                    TraceComponent,
                    "{0} processing succeeded",
                    casted->TraceId);
            }
            else
            {
                TRACE_LEVEL_AND_TESTASSERT(
                    WriteInfo,
                    TraceComponent,
                    "{0} NULL reply message on success",
                    casted->TraceId);
            }
        }
        else
        {
            WriteInfo(
                FailedTraceComponent,
                "{0} processing failed: error = {1}",
                casted->TraceId,
                error);
        }

        return error;
    }

    ErrorCode StoreService::ProcessRequestAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        MessageUPtr unusedReply;
        return ProcessRequestAsyncOperation::End(operation, unusedReply);
    }

    void StoreService::ProcessRequestAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->GetRatePerfCounter().Increment();
        
        RequestInstanceHeader requestInstanceHeader;
        if (NamingMessage::TryGetNamingHeader(*request_, requestInstanceHeader))
        {
            requestInstance_ = requestInstanceHeader.Instance;

            if (requestInstance_ < 0)
            {
                TRACE_LEVEL_AND_TESTASSERT(
                    WriteInfo,
                    TraceComponent,
                    "{0}: Request instance cannot be negative: {1}",
                    TraceId,
                    requestInstance_);

                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);

                return;
            }
        }

        // Derived class must set name from message body
        ErrorCode error = this->HarvestRequestMessage(std::move(request_));

        if (error.IsSuccess())
        {
            bool allowFragment = this->AllowNameFragment();
            error = NamingUri::ValidateName(this->Name, this->NameString, allowFragment);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0} validate name failed with {1}: {2}",
                    this->TraceId,
                    error,
                    error.Message);
            }
        }

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            this->AddHealthMonitoredOperation();
            this->PerformRequest(thisSPtr);
        }
    }

    void StoreService::ProcessRequestAsyncOperation::OnCompleted()
    {
        TimerSPtr snap;
        {
            AcquireExclusiveLock lock(timerLock_);
            snap = retryTimer_;
        }

        if (snap)
        {
            snap->Cancel();
        }

        this->GetDurationPerfCounterBase().Increment();
        this->GetDurationPerfCounter().IncrementBy(stopwatch_.ElapsedMicroseconds);
        this->CompleteHealthMonitoredOperation();
    }
    
    bool StoreService::ProcessRequestAsyncOperation::TryGetNameFromRequest(Transport::MessageUPtr const & request)
    {
        NamingUri name;
        if (request->GetBody(name))
        {
            this->SetName(name);
            return true;
        }
        else
        {
            return false;
        }
    }

    void StoreService::ProcessRequestAsyncOperation::SetName(NamingUri const & name)
    {
        name_ = name;
        nameString_ = name_.ToString();
    }

    bool StoreService::ProcessRequestAsyncOperation::TryAcceptRequestAtAuthorityOwner(AsyncOperationSPtr const & thisSPtr)
    {
        MessageUPtr failureReply;

        bool accepted = this->Properties.AuthorityOwnerRequestTracker.TryAcceptRequest(
            this->ActivityHeader.ActivityId,
            this->Name,
            this->RequestInstance,
            failureReply);

        if (!accepted)
        {
            this->Reply = move(failureReply);

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }

        return accepted;
    }

    bool StoreService::ProcessRequestAsyncOperation::TryAcceptRequestAtNameOwner(AsyncOperationSPtr const & thisSPtr)
    {
        MessageUPtr failureReply;

        bool accepted = this->Properties.NameOwnerRequestTracker.TryAcceptRequest(
            this->ActivityHeader.ActivityId,
            this->Name,
            this->RequestInstance,
            failureReply);

        if (!accepted)
        {
            this->Reply = move(failureReply);

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }

        return accepted;
    }

    bool StoreService::ProcessRequestAsyncOperation::IsLocalRetryable(ErrorCode const & error)
    {
        switch (error.ReadValue())
        {
            // Asynchronous replication may still commit after timing out
            // or being canceled
            //
            case ErrorCodeValue::Timeout:
            case ErrorCodeValue::OperationCanceled:
            //
            // Optimize write conflicts and perform local retry. For all other errors, only
            // retry while recovering primary replica.
            //
            case ErrorCodeValue::StoreOperationCanceled:
            case ErrorCodeValue::StoreWriteConflict:
            case ErrorCodeValue::StoreSequenceNumberCheckFailed:
                return true;

            default:
                return false;
        }
    }
    
    void StoreService::ProcessRequestAsyncOperation::CompleteOrScheduleRetry(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode && error,
        RetryCallback const & callback)
    {
        if (error.IsSuccess() || !namingStore_.IsRepairable(error))
        {
            this->TryComplete(thisSPtr, move(error));
        }
        else if (properties_.Router.ShouldAbortProcessing())
        {
            WriteInfo(
                TraceComponent,
                "{0} abort request processing",
                this->TraceId);

            this->TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
        }
        else if (ShouldTerminateProcessing())
        {
            WriteInfo(
                TraceComponent,
                "{0} terminate retry because of timeout",
                this->TraceId);

            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed, error.TakeMessage()));
        }
        else
        {
            // Repair indefinitely in background to achieve consistency
            //
            auto delay = NamingConfig::GetConfig().RepairInterval;
            auto minTimeout = NamingConfig::GetConfig().RepairOperationTimeout;

            if (timeoutHelper_.GetRemainingTime() < minTimeout)
            {
                timeoutHelper_.SetRemainingTime(minTimeout.AddWithMaxAndMinValueCheck(delay));
            }

            this->ScheduleRetry(thisSPtr, delay, callback);
        }
    }

    void StoreService::ProcessRequestAsyncOperation::CompleteOrRecoverPrimary(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode && error,
        RetryCallback const & callback)
    {
        if (this->IsPrimaryRecovery)
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error), callback);
        }
        else
        {
            this->TryComplete(thisSPtr, move(error));
        }
    }

    void StoreService::ProcessRequestAsyncOperation::TimeoutOrScheduleRetry(
        AsyncOperationSPtr const & thisSPtr,
        RetryCallback const & callback)
    {
        if (this->HasRunOutOfTime())
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::Timeout);
        }
        else
        {
            auto delay = NamingConfig::GetConfig().RepairInterval;

            this->ScheduleRetry(thisSPtr, delay, callback);
        }
    }

    void StoreService::ProcessRequestAsyncOperation::ScheduleRetry(
        AsyncOperationSPtr const & thisSPtr,
        TimeSpan const delay,
        RetryCallback const & callback)
    {
        if (this->IsPrimaryRecovery)
        {
            WriteInfo(
                TraceComponent,
                "{0} scheduling primary recovery retry: delay = {1} timeout = {2}",
                this->TraceId,
                delay,
                timeoutHelper_.GetRemainingTime());
        }
        else
        {
            WriteNoise(
                TraceComponent,
                "{0} scheduling retry: delay = {1} timeout = {2}",
                this->TraceId,
                delay,
                timeoutHelper_.GetRemainingTime());
        }

        {
            AcquireExclusiveLock lock(timerLock_);

            if (!this->InternalIsCompleted)
            {
                retryTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr, callback](TimerSPtr const & timer) 
                    { 
                        timer->Cancel();
                        callback(thisSPtr);
                    },
                    true); // allow concurrency
                retryTimer_->Change(delay);
            }
        }
    }

    //
    // *** Helper functions for interacting with the Federation layer
    //

    AsyncOperationSPtr StoreService::ProcessRequestAsyncOperation::BeginRequestReplyToPeer(
        MessageUPtr && message,
        NodeInstance const & target,
        AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & root)
    {
        return properties_.Router.BeginRequestReplyToPeer(
            std::move(message),
            target,
            this->ActivityHeader,
            GetRemainingTime(),
            callback,
            root);
    }

    ErrorCode StoreService::ProcessRequestAsyncOperation::EndRequestReplyToPeer(
        AsyncOperationSPtr const & requestOperation,
        __out MessageUPtr & reply)
    {
        ErrorCode error = properties_.Router.EndRequestReplyToPeer(requestOperation, reply);
        if (!error.IsSuccess())
        {
            this->Reply = NamingMessage::GetPeerNamingServiceCommunicationFailureReply(CacheModeHeader(lastNameLocationVersion_));
        }

        return error;
    }

    //
    // *** Helper functions for interacting with reliability layer
    //

    AsyncOperationSPtr StoreService::ProcessRequestAsyncOperation::BeginResolveNameLocation(
        NamingUri const & namingUri,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & operationRoot)
    {
        CacheModeHeader header;
        bool hadCacheHeader = NamingMessage::TryGetNamingHeader(*request_, header);
        return AsyncOperation::CreateAndStart<ResolveNameLocationAsyncOperation>(
            properties_.Resolver,
            namingUri,
            hadCacheHeader ? header.CacheMode : CacheMode::UseCached,
            hadCacheHeader ? header.PreviousVersion : lastNameLocationVersion_,
            properties_.NamingCuidCollection,
            this->BaseTraceId,
            this->ActivityHeader,
            GetRemainingTime(),
            callback,
            operationRoot);
    }

    ErrorCode StoreService::ProcessRequestAsyncOperation::EndResolveNameLocation(
        AsyncOperationSPtr const & asyncOperation,
        __out SystemServiceLocation & locationResult)
    {
        return ResolveNameLocationAsyncOperation::End(asyncOperation, locationResult, lastNameLocationVersion_);
    }  

    //
    // *** General helper functions
    //

    TimeSpan const StoreService::ProcessRequestAsyncOperation::GetRemainingTime()
    {
        return timeoutHelper_.GetRemainingTime();
    }

    bool StoreService::ProcessRequestAsyncOperation::HasRunOutOfTime()
    {
        return timeoutHelper_.IsExpired;
    }

    void StoreService::ProcessRequestAsyncOperation::ProcessRecoveryHeader(Transport::MessageUPtr && request)
    {
        PrimaryRecoveryHeader recoveryHeader;
        if (request->Headers.TryReadFirst(recoveryHeader))
        {
            this->SetIsPrimaryRecovery();
        }
    }

    Common::TimeSpan StoreService::ProcessRequestAsyncOperation::GetProcessOperationHealthDuration() const
    { 
        return NamingConfig::GetConfig().NamingServiceProcessOperationHealthDuration; 
    }

    void StoreService::ProcessRequestAsyncOperation::AddHealthMonitoredOperation()
    {
        this->Properties.HealthMonitor.AddMonitoredOperation(
            this->ActivityHeader.ActivityId,
            this->GetHealthOperationName(),
            this->NameString,
            this->GetProcessOperationHealthDuration());
    } 

    void StoreService::ProcessRequestAsyncOperation::CompleteHealthMonitoredOperation() 
    { 
        this->Properties.HealthMonitor.CompleteMonitoredOperation(
            this->ActivityHeader.ActivityId,
            this->GetHealthOperationName(),
            this->NameString,
            this->Error,
            this->IsLocalRetryable(this->Error));
    }
}
