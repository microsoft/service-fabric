// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace Store;
using namespace std;
using namespace ClientServerTransport;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("ProcessReport");
StringLiteral const TraceSequenceStream("SS");

ProcessReportRequestAsyncOperation::ProcessReportRequestAsyncOperation(
     __in EntityCacheManager & entityManager,
    Store::PartitionedReplicaId const & partitionedReplicaId,
    Common::ActivityId const & activityId,
    Transport::MessageUPtr && request,
    Federation::RequestReceiverContextUPtr && requestContext,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : ProcessRequestAsyncOperation(
        entityManager,
        partitionedReplicaId,
        activityId,
        move(request),
        move(requestContext),
        TimeoutWithoutSendBuffer(timeout),
        callback,
        parent)
    , requestCount_(0)
    , sequenceStreams_()
    , reportResults_()
    , sequenceStreamResults_()
    , lock_()
    , currentNestedActivityId_(activityId)
{
}

ProcessReportRequestAsyncOperation::~ProcessReportRequestAsyncOperation()
{
}

ErrorCode ProcessReportRequestAsyncOperation::End(Common::AsyncOperationSPtr const & operation)
{
    MessageUPtr reply;
    RequestReceiverContextUPtr unusedContext;

    auto error = ProcessRequestAsyncOperation::End(operation, reply, unusedContext);

    if (error.IsSuccess())
    {
        // Mainly for internal usage - processing was successful only if all reports
        // were processed successfully.
        //
        ReportHealthReplyMessageBody body;
        if (reply->GetBody(body))
        {
            for (auto const & it : body.HealthReportResultList)
            {
                if (it.Error != ErrorCodeValue::Success)
                {
                    error = it.Error;
                    break;
                }
            }
        }
        else
        {
            error = ErrorCode::FromNtStatus(reply->GetStatus());
        }
    }

    if (error.IsError(ErrorCodeValue::StaleRequest))
    {
        error = ErrorCodeValue::HealthStaleReport;
    }

    return error;
}

TimeSpan ProcessReportRequestAsyncOperation::TimeoutWithoutSendBuffer(Common::TimeSpan const timeout)
{
    return timeout.SubtractWithMaxAndMinValueCheck(CommonConfig::GetConfig().SendReplyBufferTimeout);
}

void ProcessReportRequestAsyncOperation::HandleRequest(Common::AsyncOperationSPtr const & thisSPtr)
{
    if (this->Request.Action != HealthManagerTcpMessage::ReportHealthAction)
    {
        TRACE_LEVEL_AND_TESTASSERT(
            this->WriteInfo,
            TraceComponent,
            "{0}: ProcessReportRequestAsyncOperation received action {1}, not supported",
            this->TraceId,
            this->Request.Action);

        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidMessage));
        return;
    }

    ReportHealthMessageBody messageBody;
    if (!this->Request.GetBody(messageBody))
    {
        TRACE_LEVEL_AND_TESTASSERT(this->WriteInfo, TraceComponent, "{0}: Error getting ReportHealthMessageBody", this->TraceId);

        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidMessage));
        return;
    }

    auto requests = messageBody.MoveEntries();
    auto sequenceStreamsInfo = messageBody.MoveSequenceStreamInfo();

    vector<SequenceStreamRequestContext> ssContexts;
    map<SequenceStreamId, ErrorCodeValue::Enum> rejectedSequences;

    for (auto it = sequenceStreamsInfo.begin(); it != sequenceStreamsInfo.end(); ++it)
    {
        SequenceStreamId id(it->Kind, it->SourceId);
        ErrorCode error = this->EntityManager.AcceptSequenceStream(currentNestedActivityId_, id, it->Instance, it->UpToLsn);
        if (error.IsSuccess())
        {
            auto pendingCount = it->ReportCount;
            if (pendingCount == 0)
            {
                currentNestedActivityId_.IncrementIndex();
                ssContexts.push_back(
                    SequenceStreamRequestContext(
                        Store::ReplicaActivityId(this->ReplicaActivityId.PartitionedReplicaId, currentNestedActivityId_),
                        thisSPtr,
                        *this,
                        move(*it)));
            }
            else
            {
                sequenceStreams_.push_back(SequenceStreamWithPendingCountPair(move(*it), pendingCount));
            }
        }
        else
        {
            this->WriteInfo(
                TraceComponent,
                "{0}: Rejecting sequence stream {1}: {2}",
                this->TraceId,
                *it,
                error);
            rejectedSequences[id] = error.ReadValue();
        }
    }

    // Create request contexts and pass them to the cache manager.
    // If any of the requests are accepted, wait for them to be processed.
    // The context will keep track of this async operation and notify it with the result of the processing.
    vector<ReportRequestContext> contexts;
    uint64 reportIndex = 0;
    this->EntityManager.HealthManagerCounters->OnHealthReportsReceived(requests.size());

    for (auto & report : requests)
    {
        // Create a health report result which takes the primary key from the report.
        // For node, it uses the node id generated on the client. For server processing,
        // the report node id is re-computed to use the server configure generated data.
        HealthReportResult result(report);

        if (report.SequenceNumber <= FABRIC_INVALID_SEQUENCE_NUMBER)
        {
            // If the new report has invalid LSN, reject immediately
            HMEvents::Trace->DropReportInvalidSourceLSN(this->TraceId, report);
            result.Error = ErrorCodeValue::HealthStaleReport;
        }
        else
        {
            SequenceStreamId id(report.Kind, report.SourceId);
            auto itReject = rejectedSequences.find(id);
            if (itReject != rejectedSequences.end())
            {
                result.Error = itReject->second;
            }
            else
            {
                // Check whether there is a sequence stream associated with this context
                FABRIC_SEQUENCE_NUMBER fromLsn = FABRIC_INVALID_SEQUENCE_NUMBER;
                FABRIC_SEQUENCE_NUMBER upToLsn = FABRIC_INVALID_SEQUENCE_NUMBER;
                for (auto itSS = sequenceStreams_.begin(); itSS != sequenceStreams_.end(); ++itSS)
                {
                    if (itSS->first.IsHealthInformationRelevant(report))
                    {
                        fromLsn = itSS->first.FromLsn;
                        upToLsn = itSS->first.UpToLsn;
                        break;
                    }
                }

                // Create a context that will be given to the cache entity manager
                // for processing
                currentNestedActivityId_.IncrementIndex();
                if (HealthEntityKind::CanAccept(report.Kind))
                {
                    contexts.push_back(ReportRequestContext(
                        Store::ReplicaActivityId(this->ReplicaActivityId.PartitionedReplicaId, currentNestedActivityId_),
                        thisSPtr,
                        *this,
                        reportIndex,
                        move(report),
                        fromLsn,
                        upToLsn));
                }
                else
                {
                    WriteInfo(TraceComponent, "{0}: Invalid context kind {1}", currentNestedActivityId_, report.Kind);
                    // Complete the report with error
                    result.Error = ErrorCodeValue::InvalidArgument;
                }
            }
        }

        // Remember the entry in the vector of results.
        // The error code will be set after the request is processed.
        // The correlation is done using the reportIndex.
        // Add under the lock, as it may race with the timeout timer
        // if a very small timeout is provided.
        { // lock
            AcquireExclusiveLock lock(lock_);
            reportResults_.push_back(move(result));
        } // endlock

        ++reportIndex;
    }

    requestCount_ = static_cast<uint64>(contexts.size());
    uint64 initialCount = requestCount_;
    if (!ssContexts.empty())
    {
        // Increment number of requests that need to complete in order
        // to consider the work for this async operation done.
        requestCount_ += ssContexts.size();
        // Remember request count before changed in callback by contexts being completed
        initialCount = requestCount_;
    }

    // Start timer after everything is initialized, to prevent races when the timeout is really small
    // and fires before the global variables are initialized.
    TimedAsyncOperation::OnStart(thisSPtr);

    for (auto && ssContext : ssContexts)
    {
        this->EntityManager.AddSequenceStreamContext(move(ssContext));
    }

    this->EntityManager.AddReports(move(contexts));
    CheckIfAllRequestsProcessed(thisSPtr, initialCount);
}

void ProcessReportRequestAsyncOperation::OnReportProcessed(
    Common::AsyncOperationSPtr const & thisSPtr,
    ReportRequestContext const & context,
    Common::ErrorCode const & error)
{
    ASSERT_IF(thisSPtr.get() != this, "{0}: OnReportProcessed thisSPtr.get() != this: incoming {1}", this->TraceId, context);

    // Both reports and ss (sequence stream) are counted towards the max number of allowed reports
    this->EntityManager.OnContextCompleted(context);

    uint64 remainingCount = numeric_limits<uint64>::max();
    if (error.IsSuccess())
    {
        HMEvents::Trace->Complete_ProcessReport(
            context.Report.EntityInformation->EntityId,
            context.ActivityId,
            context.Report,
            wformatString(context.Report.AttributeInfo.Attributes),
            error.Message);
        this->EntityManager.HealthManagerCounters->OnSuccessfulHealthReport(context.StartTime);

        // Trace to operation channel per entity type for all reports with warning/error health states
        // Also trace for transition to OK state, from warning, error, or expired state.
        if (HMEvents::ShouldTraceToOperationalChannel(context.PreviousState, context.Report.State))
        {
            HMEvents::Trace->TraceProcessReportOperational(
                context.Report);
        }
    }
    else
    {
        HMEvents::Trace->ProcessReportFailed(
            context.Report.EntityInformation->EntityId,
            context,
            wformatString(context.Report.AttributeInfo.Attributes),
            error,
            error.Message,
            this->OriginalTimeout);
    }

    size_t requestId = static_cast<size_t>(context.RequestId);
    unique_ptr<SequenceStreamRequestContext> ssContext;

    { // lock
        AcquireExclusiveLock lock(lock_);
        reportResults_[requestId].Error = error.ReadValue();

        // Check whether it has a sequence stream associated with it
        if (CanConsiderSuccess(error) &&
            (context.SequenceStreamFromLsn != FABRIC_INVALID_SEQUENCE_NUMBER))
        {
            bool found = false;
            for (auto it = sequenceStreams_.begin(); it != sequenceStreams_.end(); ++it)
            {
                if (it->first.IsHealthInformationRelevant(context.Report))
                {
                    // Decrement the count of pending reports
                    if (--it->second == 0)
                    {
                        // Schedule context to persist the new source sequence stream
                        currentNestedActivityId_.IncrementIndex();
                        ssContext = make_unique<SequenceStreamRequestContext>(
                            Store::ReplicaActivityId(this->ReplicaActivityId.PartitionedReplicaId, currentNestedActivityId_),
                            thisSPtr,
                            *this,
                            move(it->first));

                        // Increment number of requests that need to complete in order
                        // to consider the work for this async operation done.
                        // The stream context must be completed once given to the entity manager.
                        ++requestCount_;

                        // swap "it" with last element and remove it from vector
                        swap(*it, sequenceStreams_.back());
                        sequenceStreams_.pop_back();
                    }

                    found = true;
                    break;
                }
            }

            if (!found)
            {
                TRACE_LEVEL_AND_TESTASSERT(
                    this->WriteWarning,
                    TraceComponent,
                    "Completed context {0} not found in sequenceStreams_",
                    context);
            }
        }

        remainingCount = --requestCount_;
    } // endlock

    if (ssContext)
    {
        this->EntityManager.AddSequenceStreamContext(move(*ssContext));
    }

    CheckIfAllRequestsProcessed(thisSPtr, remainingCount);
}

void ProcessReportRequestAsyncOperation::OnSequenceStreamProcessed(
    Common::AsyncOperationSPtr const & thisSPtr,
    SequenceStreamRequestContext const & context,
    Common::ErrorCode const & error)
{
    ASSERT_IF(thisSPtr.get() != this, "{0}: OnReportProcessed thisSPtr.get() != this: incoming {1}", this->TraceId, context);

    // Both reports and ss are counted towards the max number of allowed reports
    this->EntityManager.OnContextCompleted(context);

    uint64 remainingCount = numeric_limits<uint64>::max();

    { // lock
        // Add the ACK to reply message, so reporting component knows that the sequence stream has been updated
        AcquireWriteLock lock(lock_);
        if (CanConsiderSuccess(error))
        {
            sequenceStreamResults_.push_back(SequenceStreamResult(
                context.Kind,
                context.SequenceStream.SourceId,
                context.SequenceStream.Instance,
                context.SequenceStream.UpToLsn));
        }

        remainingCount = --requestCount_;
    } // endlock

    if (error.IsSuccess())
    {
        HMEvents::Trace->ReportReplyAddSS(context, remainingCount);
    }
    else
    {
        HMEvents::Trace->ReportReplySSFailed(context, error.ReadValue(), remainingCount);
    }

    CheckIfAllRequestsProcessed(thisSPtr, remainingCount);
}

void ProcessReportRequestAsyncOperation::CheckIfAllRequestsProcessed(
    Common::AsyncOperationSPtr const & thisSPtr,
    uint64 pendingRequests)
{
    if (pendingRequests == 0)
    {
        if (this->TryStartComplete())
        {
            // All done: put the results for all operations in the reply
            // and complete with success regardless of the individual results
            // It's safe to move the member items, as all inner operations have completed
            // and nobody should access them any longer at this point.
            // Since TryStartComplete allowed this thread access, it won't allow the timeout callback to be executed in parallel.
            this->SetReply(
                HealthManagerMessage::GetClientOperationSuccess(
                    ReportHealthReplyMessageBody(move(reportResults_), move(sequenceStreamResults_)),
                    this->ActivityId));
            this->FinishComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }
    }
}

void ProcessReportRequestAsyncOperation::OnTimeout(
    Common::AsyncOperationSPtr const & thisSPtr)
{
    TimedAsyncOperation::OnTimeout(thisSPtr);

    { // lock
        AcquireExclusiveLock lock(lock_);

        // For all unprocessed reports, set exception to timeout
        for (auto & reportResult : reportResults_)
        {
            if (reportResult.Error == ErrorCodeValue::NotReady)
            {
                reportResult.Error = ErrorCodeValue::Timeout;
            }
        }

        // Create reply with current state of the requests.
        // This ensures that any partial results are returned to the reporting component.
        // Note that the results can't be moved, because they may be accessed later on, when the context is completed
        // It's OK to call SetReply directly here, because the TimedAyncOperation only calls'
        // OnTimeout after calling TryStartComplete, so no other thread can complete at the same time.
        this->SetReply(
            HealthManagerMessage::GetClientOperationSuccess(
                ReportHealthReplyMessageBody(reportResults_, sequenceStreamResults_),
                this->ActivityId));
        HMEvents::Trace->ProcessReportTimedOut(this->TraceId, requestCount_);
    } // endlock
}

bool ProcessReportRequestAsyncOperation::CanConsiderSuccess(Common::ErrorCode const & error)
{
    switch (error.ReadValue())
    {
    case ErrorCodeValue::Success:
    case ErrorCodeValue::StaleRequest:
    case ErrorCodeValue::HealthEntityDeleted:
    case ErrorCodeValue::HealthStaleReport:
        return true;

    default:
        return false;
    }
}
