// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;
using namespace ServiceModel;
using namespace Query;

StringLiteral const TraceType("QueryMessageHandler");

class QueryMessageHandler::QueryMessageHandlerAsyncOperation
    : public TimedAsyncOperation
{
public:
    QueryMessageHandlerAsyncOperation(
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , ReplyMessage()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, _Out_ MessageUPtr & reply)
    {
        auto casted = AsyncOperation::End<QueryMessageHandlerAsyncOperation>(operation);
        auto error = casted->Error;

        if (casted->ReplyMessage)
        {
            reply = move(casted->ReplyMessage);
        }
        return error;
    }

protected:
    MessageUPtr ReplyMessage;
};

class QueryMessageHandler::ProcessParallelQueryAsyncOperation:
    public QueryMessageHandlerAsyncOperation
{
public:
    ProcessParallelQueryAsyncOperation(
        Query::QueryNames::Enum queryName,
        QueryArgumentMap const & queryArgs,
        QuerySpecificationSPtr const & querySpecSPtr,
        QueryMessageHandler & owner,
        Transport::MessageHeaders & headers,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : QueryMessageHandlerAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , queryName_(queryName)
        , queryArgs_(queryArgs)
        , querySpecSPtr_(querySpecSPtr)
        , headers_(headers)
        , parallelQuerySpecification_()
        , resultMap_()
        , pendingInnerQueryCount_(0)
        , errorCount_(0)
        , activityHeader_()
        , lock_()
    {
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ASSERT_IFNOT(
            headers_.TryReadFirst(activityHeader_),
            "ActivityId should be available in the message");

        if (querySpecSPtr_ == nullptr)
        {
            querySpecSPtr_ = QuerySpecificationStore::Get().GetSpecification(queryName_, queryArgs_);
        }
        parallelQuerySpecification_ = static_pointer_cast<ParallelQuerySpecification>(querySpecSPtr_);

        if (!parallelQuerySpecification_)
        {
            WriteWarning(
                TraceType,
                "{0}: QuerySpecification could not be created for parallel query {1}. Invalid configuration for query or upgrade is in progress",
                activityHeader_.ActivityId,
                queryName_);
            TryComplete(thisSPtr, ErrorCodeValue::InvalidConfiguration);
            return;
        }

        LONG operationSize = static_cast<LONG>(parallelQuerySpecification_->ParallelQuerySpecifications.size());
        if (operationSize == 0L)
        {
            WriteWarning(
                TraceType,
                "{0}, ParallelQuerySpecifications {1} is empty. Completing it.",
                activityHeader_.ActivityId,
                queryName_);

            this->TryComplete(thisSPtr);
            return;
        }

        pendingInnerQueryCount_.store(operationSize);

        QueryEventSource::EventSource->ProcessingParallelQuery(
                activityHeader_.ActivityId,
                owner_.addressSegment_,
                parallelQuerySpecification_->QueryName);

        ExecuteParallelOperations(thisSPtr);
    }

    void ExecuteParallelOperations(AsyncOperationSPtr const & thisSPtr)
    {
        for (auto index = 0; index < parallelQuerySpecification_->ParallelQuerySpecifications.size(); ++index)
        {
            QueryArgumentMap currentQueryArg = parallelQuerySpecification_->GetQueryArgumentMap(
                index,
                queryArgs_);

            QuerySpecificationSPtr currentQuerySpecification = parallelQuerySpecification_->ParallelQuerySpecifications[index];
            wstring queryAddress;
            auto queryGenerationError = currentQuerySpecification->GenerateAddress(
                activityHeader_.ActivityId,
                currentQueryArg,
                queryAddress);
            // We assume that the parallel query specification should include all the arguments required for inner queries.
            // Hence if the address generation fails, there is something wrong with the configuration of query
            if (!queryGenerationError.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    "{0}: Address could not be generated for query {1} as part of parallel query {2} execution. Error = {3}. Invalid configuration.",
                    activityHeader_.ActivityId,
                    currentQuerySpecification->QueryName,
                    queryName_,
                    queryGenerationError);
                TryComplete(thisSPtr, ErrorCodeValue::InvalidConfiguration);
                return;
            }

            auto queryMessage = QueryMessage::CreateMessage(currentQuerySpecification->QueryName, currentQuerySpecification->QueryType, currentQueryArg, queryAddress);

            queryMessage->Headers.Add(activityHeader_);

            // We have the message for the sub-query that needs to be executed for parallel query. Execute the query.
            auto operation = owner_.BeginProcessQueryMessage(
                *queryMessage,
                parallelQuerySpecification_->GetParallelQueryExecutionTimeout(currentQuerySpecification, RemainingTime),
                [this, currentQuerySpecification](AsyncOperationSPtr const & operation){ this->FinishProcessInnerQueryOperation(operation, currentQuerySpecification, false); },
                thisSPtr);
            this->FinishProcessInnerQueryOperation(operation, currentQuerySpecification, true);
        }
    }

    void FinishProcessInnerQueryOperation(AsyncOperationSPtr const & operation, QuerySpecificationSPtr const & querySpecification, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr result;
        auto error = owner_.EndProcessQueryMessage(operation, result);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceType,
                "{0}: ParallelQuery {1}: subquery {2} failed: {3}.",
                activityHeader_.ActivityId,
                parallelQuerySpecification_->QueryName,
                querySpecification->QueryName,
                error);
            CompleteIfLastInnerOperation(operation->Parent, querySpecification, QueryResult(error));
            return;
        }

        ASSERT_IFNOT(result, "{0}: ParallelQuery {1}: subquery {2} return Success and null reply.", activityHeader_.ActivityId, parallelQuerySpecification_->QueryName, querySpecification->QueryName);

        ServiceModel::QueryResult queryResult;
        if (!result->GetBody<ServiceModel::QueryResult>(queryResult))
        {
            WriteInfo(
                TraceType,
                "{0}: ParallelQuery {1}: error getting Body for subquery {2}.",
                activityHeader_.ActivityId,
                parallelQuerySpecification_->QueryName,
                querySpecification->QueryName);
            CompleteIfLastInnerOperation(operation->Parent, querySpecification, QueryResult(ErrorCodeValue::SerializationError));
            return;
        }

        CompleteIfLastInnerOperation(operation->Parent, querySpecification, move(queryResult));
    }

    // As the operations are happening in parallel, multiple of these can result in error
    // If we wish to complete this operation on first error, we need to make sure that we don't call
    // TryComplete multiple times.
    void CompleteIfLastInnerOperation(
        AsyncOperationSPtr const & thisSPtr,
        QuerySpecificationSPtr const & querySpecification,
        QueryResult && queryResult)
    {
        if (!queryResult.QueryProcessingError.IsSuccess())
        {
            if (++errorCount_ == parallelQuerySpecification_->ParallelQuerySpecifications.size())
            {
                WriteWarning(
                    TraceType,
                    "{0}: ParallelQuery: {1}. All the subqueries failed. Last error {2}.",
                    activityHeader_.ActivityId,
                    parallelQuerySpecification_->QueryName,
                    queryResult.QueryProcessingError);

                ReplyMessage = QueryMessage::GetQueryFailure(queryResult);
                TryComplete(thisSPtr, queryResult.QueryProcessingError);
                return;
            }
        }

        { // lock
            AcquireExclusiveLock lock(lock_);
            resultMap_.insert(make_pair(querySpecification, move(queryResult)));
        } // endlock

        if (--pendingInnerQueryCount_ == 0)
        {
            auto error = parallelQuerySpecification_->OnParallelQueryExecutionComplete(activityHeader_.ActivityId, resultMap_, ReplyMessage);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    "{0}: ParallelQuery: {1}. Error {2} during the merge operation.",
                    activityHeader_.ActivityId,
                    parallelQuerySpecification_->QueryName,
                    error);
                ReplyMessage = QueryMessage::GetQueryFailure(QueryResult(error));
            }

            TryComplete(thisSPtr, error);
        }
    }

private:
    QueryMessageHandler & owner_;
    QueryNames::Enum queryName_;
    QueryArgumentMap queryArgs_;
    QuerySpecificationSPtr querySpecSPtr_;
    ParallelQuerySpecificationSPtr parallelQuerySpecification_;
    std::map<Query::QuerySpecificationSPtr, ServiceModel::QueryResult> resultMap_;
    Common::atomic_long errorCount_;
    Transport::MessageHeaders & headers_;
    Common::atomic_long pendingInnerQueryCount_;
    Transport::FabricActivityHeader activityHeader_;
    Common::RwLock lock_;
};

class QueryMessageHandler::ProcessSequentialQueryAsyncOperation:
    public QueryMessageHandlerAsyncOperation
{
public:
    ProcessSequentialQueryAsyncOperation(
        Query::QueryNames::Enum queryName,
        QueryArgumentMap const & queryArgs,
        QueryMessageHandler & owner,
        Transport::MessageHeaders & headers,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : QueryMessageHandlerAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , queryName_(queryName)
        , queryArgs_(queryArgs)
        , headers_(headers)
        , innerQueriesProgressCount_(0)
        , activityHeader_()
        , sequentialQuerySpecification_()
        , queryResults_()
    {
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ASSERT_IFNOT(
            headers_.TryReadFirst(activityHeader_),
            "ActivityId should be available in the message");

        auto querySpecification = QuerySpecificationStore::Get().GetSpecification(queryName_, queryArgs_);
        sequentialQuerySpecification_ = static_pointer_cast<SequentialQuerySpecification>(querySpecification);

        if (!sequentialQuerySpecification_)
        {
            WriteWarning(
                TraceType,
                "{0}: QuerySpecification could not be created for sequential query {1}. Invalid configuration for query or upgrade is in progress",
                activityHeader_.ActivityId,
                queryName_);

            TryComplete(thisSPtr, ErrorCodeValue::InvalidConfiguration);
            return;
        }

        ExecuteNextQuery(thisSPtr, sequentialQuerySpecification_);
    }

    void ExecuteNextQuery(AsyncOperationSPtr const & thisSPtr, QuerySpecificationSPtr const & previousQuerySpec)
    {
        QuerySpecificationSPtr currentQuerySpecSptr;
        ErrorCode error;
        auto queryArgs = queryArgs_;

        innerQueriesProgressCount_ += 1;
        error = sequentialQuerySpecification_->GetNext(
            previousQuerySpec,
            queryResults_,
            queryArgs,
            currentQuerySpecSptr,
            ReplyMessage);

        if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        if (!error.IsSuccess())
        {
            ReplyMessage = QueryMessage::GetQueryFailure(QueryResult(error));
            TryComplete(thisSPtr, error);
            return;
        }

        ASSERT_IF(
            !currentQuerySpecSptr,
            "Sequential Query {0} returned empty inner query. Inner Query Progress Count = {1}", sequentialQuerySpecification_->QueryName, innerQueriesProgressCount_);

        QueryEventSource::EventSource->ProcessingSequentialQuery(
            activityHeader_.ActivityId,
            owner_.addressSegment_,
            sequentialQuerySpecification_->QueryName,
            static_cast<LONGLONG>(innerQueriesProgressCount_),
            currentQuerySpecSptr->QueryName);

        wstring queryAddress;
        error = currentQuerySpecSptr->GenerateAddress(activityHeader_.ActivityId, queryArgs, queryAddress);

        // We assume that the sequential query specification should include all the arguments required for inner queries.
        // Hence if the address generation fails, there is something wrong with the configuration of query
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "{0}: Address could not be generated for query {1} as part of sequential query {2} execution. Error = {3}. Invalid configuration.",
                activityHeader_.ActivityId,
                currentQuerySpecSptr->QueryName,
                queryName_,
                error);

            ReplyMessage = QueryMessage::GetQueryFailure(QueryResult(ErrorCodeValue::InvalidConfiguration));
            TryComplete(thisSPtr, ErrorCodeValue::InvalidConfiguration);
            return;
        }

        auto queryMessage = QueryMessage::CreateMessage(currentQuerySpecSptr->QueryName, currentQuerySpecSptr->QueryType, queryArgs, queryAddress);

        queryMessage->Headers.Add(activityHeader_);

        auto operation = owner_.BeginProcessQueryMessage(
            *queryMessage,
            currentQuerySpecSptr,
            RemainingTime,
            [this, currentQuerySpecSptr](AsyncOperationSPtr const & operation){ this->FinishProcessInnerQueryOperation(operation, currentQuerySpecSptr, false); },
            thisSPtr);

        this->FinishProcessInnerQueryOperation(operation, currentQuerySpecSptr, true);
    }

    void FinishProcessInnerQueryOperation(AsyncOperationSPtr const & operation, QuerySpecificationSPtr const & processedQuerySpecification, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr result;
        ServiceModel::QueryResult queryResult;
        auto error = owner_.EndProcessQueryMessage(operation, result);

        if (sequentialQuerySpecification_->ShouldContinueSequentialQuery(processedQuerySpecification, error))
        {
            if (result->GetBody<ServiceModel::QueryResult>(queryResult))
            {
                queryResults_[processedQuerySpecification->QueryName] = move(queryResult);
                ExecuteNextQuery(operation->Parent, processedQuerySpecification);
            }
            else
            {
                WriteWarning(
                    TraceType,
                    "{0}: Sequential Query: {1} failed to retrieve query result from inner query {2}",
                    activityHeader_.ActivityId,
                    sequentialQuerySpecification_->QueryName,
                    processedQuerySpecification->QueryName);

                ReplyMessage = QueryMessage::GetQueryFailure(QueryResult(ErrorCodeValue::SerializationError));
                TryComplete(operation->Parent, ErrorCodeValue::SerializationError);
            }
        }
        else
        {
            WriteWarning(
                TraceType,
                "{0}: Sequential Query: {1} failed to process inner query {2}. Error = {3}.",
                activityHeader_.ActivityId,
                sequentialQuerySpecification_->QueryName,
                processedQuerySpecification->QueryName,
                error);

            ReplyMessage = QueryMessage::GetQueryFailure(QueryResult(error));

            TryComplete(operation->Parent, error);
        }
    }

private:
    QueryMessageHandler & owner_;
    QueryNames::Enum queryName_;
    QueryArgumentMap queryArgs_;
    Query::SequentialQuerySpecificationSPtr sequentialQuerySpecification_;
    LONG innerQueriesProgressCount_;
    Transport::MessageHeaders & headers_;
    Transport::FabricActivityHeader activityHeader_;
    std::unordered_map<QueryNames::Enum, QueryResult> queryResults_;
};


class QueryMessageHandler::ProcessMessageAsyncOperation:
    public QueryMessageHandlerAsyncOperation
{
public:
    ProcessMessageAsyncOperation(
        Message & message,
        QuerySpecificationSPtr const & querySpecSPtr,
        QueryMessageHandler & owner,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : QueryMessageHandlerAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , requestMessage_(message)
        , querySpecSPtr_(querySpecSPtr)
    {
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.State.Value != FabricComponentState::Opened)
        {
            // This could happen during a service failover, returning an error so that this can be retried.
            TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
            return;
        }

        owner_.requestCount_++;

        ActivityId activityId;
        QueryRequestMessageBodyInternal messageBody;
        wstring addressSegment = L"";
        wstring addressSegmentMetadata = L"";

        auto parseResult = QueryMessageUtility::ParseQueryMessage(
            requestMessage_,
            owner_.addressSegment_,
            addressSegment,
            addressSegmentMetadata,
            activityId,
            messageBody);

        // querySpecSPtr_ is passed if a certain QuerySpecification is dynamically constructed in runtime.
        // The processor should skip over reading from the query store if querySpecSPtr_ is not nullptr.
        // It is currently only supported in Parallel query.
        if (parseResult == QueryMessageUtility::QueryMessageParseResult::Parallel)
        {
            auto operation = owner_.BeginProcessParallelQuery(
                messageBody.QueryName,
                messageBody.QueryArgs,
                querySpecSPtr_,
                requestMessage_.Headers,
                RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->FinishProcessParallelQuery(operation, false); },
                thisSPtr);
            this->FinishProcessParallelQuery(operation, true);
        }
        else if (parseResult == QueryMessageUtility::QueryMessageParseResult::Single)
        {
            auto operation = owner_.beginProcessQueryFunction_(
                messageBody.QueryName,
                messageBody.QueryArgs,
                activityId,
                RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->FinishProcessQueryOperation(operation, false); },
                thisSPtr);
            this->FinishProcessQueryOperation(operation, true);
        }
        else if (parseResult == QueryMessageUtility::QueryMessageParseResult::Forward)
        {
            auto messageClone = requestMessage_.Clone();
            auto operation = owner_.beginForwardQueryMessageFunction_(
                addressSegment,
                addressSegmentMetadata,
                messageClone,
                RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->FinishForwardQueryOperation(operation, false); },
                thisSPtr);
            this->FinishForwardQueryOperation(operation, true);
        }
        else if (parseResult == QueryMessageUtility::QueryMessageParseResult::Sequential)
        {
            auto operation = owner_.BeginProcessSequentialQuery(
                messageBody.QueryName,
                messageBody.QueryArgs,
                requestMessage_.Headers,
                RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->FinishProcessSequentialQuery(operation, false); },
                thisSPtr);
            this->FinishProcessSequentialQuery(operation, true);

        }
        else if (parseResult == QueryMessageUtility::QueryMessageParseResult::Drop)
        {
            auto errorCode = ErrorCodeValue::InvalidMessage;
            QueryResult result = QueryResult(errorCode);
            ReplyMessage = QueryMessage::GetQueryFailure(result);
            TryComplete(thisSPtr, errorCode);
        }
        else
        {
            Assert::CodingError("Unknown parse result {0}", static_cast<int>(parseResult));
        }
    }

    void FinishProcessQueryOperation(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        TryComplete(operation->Parent, owner_.endProcessQueryFunction_(operation, ReplyMessage));
        ReleaseRegistrationsIfClosed();
    }

    void FinishForwardQueryOperation(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        TryComplete(operation->Parent, owner_.endForwardQueryMessageFunction_(operation, ReplyMessage));
        ReleaseRegistrationsIfClosed();
    }

    void FinishProcessParallelQuery(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        TryComplete(operation->Parent, owner_.EndProcessParallelQuery(operation, ReplyMessage));
        ReleaseRegistrationsIfClosed();
    }

    void FinishProcessSequentialQuery(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        TryComplete(operation->Parent, owner_.EndProcessSequentialQuery(operation, ReplyMessage));
        ReleaseRegistrationsIfClosed();
    }

    void ReleaseRegistrationsIfClosed()
    {
        if ((--owner_.requestCount_ == 0) && owner_.closedOrAborted_.load())
        {
            owner_.UnRegisterAll();
        }
    }

private:
    QueryMessageHandler & owner_;
    Message & requestMessage_;
    QuerySpecificationSPtr querySpecSPtr_;
};

class QueryMessageHandler::ProcessQuerySynchronousCallerAsyncOperation
    : public QueryMessageHandlerAsyncOperation
{
public:
    ProcessQuerySynchronousCallerAsyncOperation(
        ProcessQueryFunction const & processQueryFunction,
        QueryNames::Enum queryName,
        QueryArgumentMap const & queryArgs,
        Common::ActivityId const & activityId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : QueryMessageHandlerAsyncOperation(timeout, callback, parent)
        , processQueryFunction_(processQueryFunction)
        , queryName_(queryName)
        , queryArgs_(queryArgs)
        , activityId_(activityId)
    {
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = processQueryFunction_(
                queryName_,
                queryArgs_,
                activityId_,
                ReplyMessage);

        if (!error.IsSuccess())
        {
            ReplyMessage = QueryMessage::GetQueryFailure(QueryResult(error));
        }

        TryComplete(thisSPtr, error);
    }

private:
    ProcessQueryFunction processQueryFunction_;
    QueryNames::Enum queryName_;
    QueryArgumentMap queryArgs_;
    Common::ActivityId activityId_;
};

class QueryMessageHandler::ForwardQuerySynchronousCallerAsyncOperation
    : public QueryMessageHandlerAsyncOperation
{
public:
    ForwardQuerySynchronousCallerAsyncOperation(
        ForwardQueryFunction const & forwardQueryFunction,
        wstring const & childAddressSegment,
        wstring const & childAddressSegmentMetadata,
        MessageUPtr & message,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : QueryMessageHandlerAsyncOperation(timeout, callback, parent)
        , forwardQueryFunction_(forwardQueryFunction)
        , childAddressSegment_(childAddressSegment)
        , childAddressSegmentMetadata_(childAddressSegmentMetadata)
        , message_(message)
    {
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = forwardQueryFunction_(
            childAddressSegment_,
            childAddressSegmentMetadata_,
            message_,
            ReplyMessage);

        if (!error.IsSuccess())
        {
            ReplyMessage = QueryMessage::GetQueryFailure(QueryResult(error));
        }

        TryComplete(thisSPtr, error);
    }

private:
    ForwardQueryFunction forwardQueryFunction_;
    wstring childAddressSegment_;
    wstring childAddressSegmentMetadata_;
    MessageUPtr & message_;
};

QueryMessageHandler::QueryMessageHandler(
    ComponentRoot const & root,
    wstring const & addressSegment)
    : RootedObject(root)
    , addressSegment_(addressSegment)
    , beginProcessQueryFunction_(nullptr)
    , endProcessQueryFunction_(nullptr)
    , beginForwardQueryMessageFunction_(nullptr)
    , endForwardQueryMessageFunction_(nullptr)
    , requestCount_(0)
    , closedOrAborted_(false)
{
}

AsyncOperationSPtr QueryMessageHandler::BeginProcessQueryMessage(
    Message & message,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return this->BeginProcessQueryMessage(
        message,
        nullptr,
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr QueryMessageHandler::BeginProcessQueryMessage(
    Message & message,
    QuerySpecificationSPtr const & querySpecSPtr,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ProcessMessageAsyncOperation>(
        message,
        querySpecSPtr,
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode QueryMessageHandler::EndProcessQueryMessage(
    AsyncOperationSPtr const & asyncOperation,
    _Out_ MessageUPtr & reply)
{
    return QueryMessageHandlerAsyncOperation::End(asyncOperation, reply);
}


AsyncOperationSPtr QueryMessageHandler::BeginProcessParallelQuery(
    Query::QueryNames::Enum queryName,
    ServiceModel::QueryArgumentMap const & queryArgs,
    QuerySpecificationSPtr const & querySpecSPtr,
    Transport::MessageHeaders & headers,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ProcessParallelQueryAsyncOperation>(
         queryName,
         queryArgs,
         querySpecSPtr,
         *this,
         headers,
         timeout,
         callback,
         parent);
}

ErrorCode QueryMessageHandler::EndProcessParallelQuery(
    Common::AsyncOperationSPtr const & asyncOperation,
    _Out_ Transport::MessageUPtr & reply)
{
    return QueryMessageHandlerAsyncOperation::End(asyncOperation, reply);
}

AsyncOperationSPtr QueryMessageHandler::BeginProcessSequentialQuery(
    Query::QueryNames::Enum queryName,
    ServiceModel::QueryArgumentMap const & queryArgs,
    Transport::MessageHeaders & headers,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ProcessSequentialQueryAsyncOperation>(
         queryName,
         queryArgs,
         *this,
         headers,
         timeout,
         callback,
         parent);

}

ErrorCode QueryMessageHandler::EndProcessSequentialQuery(
    Common::AsyncOperationSPtr const & asyncOperation,
    _Out_ Transport::MessageUPtr & reply)
{
    return QueryMessageHandlerAsyncOperation::End(asyncOperation, reply);
}

MessageUPtr QueryMessageHandler::ProcessQueryMessage(Message & requestMessage)
{
    auto operationWaiter = make_shared<AsyncOperationWaiter>();
    MessageUPtr replyMessage;
    auto operation = BeginProcessQueryMessage(
        requestMessage,
        TimeSpan::MaxValue,
        [this, operationWaiter, &replyMessage] (AsyncOperationSPtr const & operation)
            { operationWaiter->SetError(EndProcessQueryMessage(operation, replyMessage)); operationWaiter->Set(); },
        Root.CreateAsyncOperationRoot());
    operationWaiter->WaitOne();
    return replyMessage;
}

ErrorCode QueryMessageHandler::RegisterQueryHandler(ProcessQueryFunction const & processQuery)
{
    return RegisterQueryHandler(
        [processQuery, this](QueryNames::Enum queryName, QueryArgumentMap const & queryArgs, Common::ActivityId const & activityId, TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
            { return BeginProcessQuerySynchronousCaller(processQuery, queryName, queryArgs, activityId, timeout, callback, parent); },
        [this](AsyncOperationSPtr const & asyncOperation, _Out_ MessageUPtr & reply)
            { return EndProcessQuerySynchronousCaller(asyncOperation, reply); });
}

ErrorCode QueryMessageHandler::RegisterQueryHandler(BeginProcessQueryFunction const & beginProcessQuery, EndProcessQueryFunction const & endProcessQuery)
{
    if (beginProcessQueryFunction_ || endProcessQueryFunction_)
    {
        Assert::CodingError("Query handlers can only be registered once");
    }

    beginProcessQueryFunction_ = beginProcessQuery;
    endProcessQueryFunction_ = endProcessQuery;

    return ErrorCode::Success();
}

ErrorCode QueryMessageHandler::RegisterQueryForwardHandler(ForwardQueryFunction const & forwardQuery)
{
    return RegisterQueryForwardHandler(
        [forwardQuery, this] (wstring const & childAddressSegment, wstring const & childAddressSegmentMetadata, MessageUPtr & message, TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
            { return BeginForwardMessageSynchronousCaller(forwardQuery, childAddressSegment, childAddressSegmentMetadata, message, timeout, callback, parent); },
        [this](AsyncOperationSPtr const & asyncOperation, _Out_ MessageUPtr & reply)
            { return EndForwardMessageSynchronousCaller(asyncOperation, reply); });
}

ErrorCode QueryMessageHandler::RegisterQueryForwardHandler(BeginForwardQueryMessageFunction const & beginForwardQuery, EndForwardQueryMessageFunction const & endForwardQuery)
{
    if (beginForwardQueryMessageFunction_ || endForwardQueryMessageFunction_)
    {
        Assert::CodingError("Query forwarders can only be registered once");
    }

    beginForwardQueryMessageFunction_ = beginForwardQuery;
    endForwardQueryMessageFunction_ = endForwardQuery;

    return ErrorCode::Success();
}

AsyncOperationSPtr QueryMessageHandler::BeginProcessQuerySynchronousCaller(
    ProcessQueryFunction const & processQueryFunction,
    QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs,
    Common::ActivityId const & activityId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ProcessQuerySynchronousCallerAsyncOperation>(
        processQueryFunction,
        queryName,
        queryArgs,
        activityId,
        timeout,
        callback,
        parent);
}

ErrorCode QueryMessageHandler::EndProcessQuerySynchronousCaller(
    AsyncOperationSPtr const & asyncOperation,
    _Out_ MessageUPtr & reply)
{
    return QueryMessageHandlerAsyncOperation::End(asyncOperation, reply);
}

 AsyncOperationSPtr QueryMessageHandler::BeginForwardMessageSynchronousCaller(
    ForwardQueryFunction const & forwardQueryFunction,
    wstring const & childAddressSegment,
    wstring const & childAddressSegmentMetadata,
    MessageUPtr & message,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
 {
     return AsyncOperation::CreateAndStart<ForwardQuerySynchronousCallerAsyncOperation>(
         forwardQueryFunction,
         childAddressSegment,
         childAddressSegmentMetadata,
         message,
         timeout,
         callback,
         parent);
 }

 ErrorCode QueryMessageHandler::EndForwardMessageSynchronousCaller(
    AsyncOperationSPtr const & asyncOperation,
    _Out_ MessageUPtr & reply)
 {
     return QueryMessageHandlerAsyncOperation::End(asyncOperation, reply);
 }

 ErrorCode QueryMessageHandler::OnOpen()
 {
     // One of the two begin/end pairs should be registered
     if ((beginProcessQueryFunction_ && endProcessQueryFunction_) ||
         (beginForwardQueryMessageFunction_ && endForwardQueryMessageFunction_))
     {
         return ErrorCode::Success();
     }

     return ErrorCodeValue::InvalidConfiguration;
 }

ErrorCode QueryMessageHandler::OnClose()
{
    OnAbort();
    return ErrorCode::Success();
}

void QueryMessageHandler::OnAbort()
{
    closedOrAborted_.store(true);
    if (requestCount_.load() == 0)
    {
        UnRegisterAll();
    }
}

void QueryMessageHandler::UnRegisterAll()
{
    beginProcessQueryFunction_ = nullptr;
    endProcessQueryFunction_ = nullptr;
    beginForwardQueryMessageFunction_ = nullptr;
    endForwardQueryMessageFunction_ = nullptr;
}
