// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;
using namespace Transport;

StringLiteral const TraceType("QueryGateway");

class QueryGateway::ProcessIncomingQueryOperation
    : public TimedAsyncOperation
{
public:
    ProcessIncomingQueryOperation(
        Message & gatewayRequestMessage,
        Transport::FabricActivityHeader const & activityHeader,
        QueryGateway & owner,
        TimeSpan timeout,
        AsyncCallback callback,
        AsyncOperationSPtr parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , activityHeader_(activityHeader)
        , owner_(owner)
        , replyMessage_()
        , timeout_(timeout)
        , requestMessageBody_()
        , requestProcessingError_()
    {
        if (!gatewayRequestMessage.GetBody(requestMessageBody_))
        {
            requestProcessingError_ = ErrorCodeValue::InvalidMessage;
        }
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!requestProcessingError_.IsSuccess())
        {
            TryComplete(thisSPtr, move(requestProcessingError_));
            return;
        }

        auto foundLookupItem = owner_.nameEnumlookupMap_.find(requestMessageBody_.QueryName);
        if (foundLookupItem == owner_.nameEnumlookupMap_.end())
        {
            QueryEventSource::EventSource->QueryNotFound(
                activityHeader_.ActivityId,
                requestMessageBody_.QueryName);
            TryComplete(
                thisSPtr,
                ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_QUERY_RC(Query_Not_Supported), requestMessageBody_.QueryName)));
            return;
        }

        // Get the query specification:
        QuerySpecificationSPtr querySpecification = QuerySpecificationStore::Get().GetSpecification(
            foundLookupItem->second,
            requestMessageBody_.QueryArgs);

        if (!querySpecification)
        {
            QueryEventSource::EventSource->QueryNotFound(
                activityHeader_.ActivityId,
                requestMessageBody_.QueryName);
            TryComplete(
                thisSPtr,
                ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_QUERY_RC(Query_Not_Supported), requestMessageBody_.QueryName)));
            return;
        }

        auto error = querySpecification->HasRequiredArguments(requestMessageBody_.QueryArgs);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, move(error));
            return;
        }

        wstring address;
        error = querySpecification->GenerateAddress(activityHeader_.ActivityId, requestMessageBody_.QueryArgs, address);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, move(error));
            return;
        }

        QueryEventSource::EventSource->QueryProcessGateway(
            activityHeader_.ActivityId,
            requestMessageBody_.QueryName,
            requestMessageBody_.QueryArgs);

        MessageUPtr message = QueryMessage::CreateMessage(querySpecification->QueryName, querySpecification->QueryType, requestMessageBody_.QueryArgs, address);
        message->Headers.Add(activityHeader_);
        message->Headers.Add(TimeoutHeader(RemainingTime));

        auto operation = owner_.messageHandler_.BeginProcessQueryMessage(
            *message,
            RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->FinishProcessQueryMessage(operation, false); },
            thisSPtr);
        FinishProcessQueryMessage(operation, true);
    }

    void FinishProcessQueryMessage(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.messageHandler_.EndProcessQueryMessage(operation, replyMessage_);

        // In case of multi-step query, we would send this result to the query specification for post-processing
        // For now, this is the result.
        TryComplete(operation->Parent, error);
    }

    static ErrorCode End(AsyncOperationSPtr operation, _Out_ MessageUPtr & replyMessage)
    {
        auto casted = AsyncOperation::End<ProcessIncomingQueryOperation>(operation);
        auto error = casted->Error;

        if (error.IsSuccess())
        {
            replyMessage = std::move(casted->replyMessage_);
        }

        return error;
    }

private:
    QueryRequestMessageBody requestMessageBody_;
    Transport::FabricActivityHeader activityHeader_;
    QueryGateway & owner_;
    MessageUPtr replyMessage_;
    TimeSpan timeout_;
    ErrorCode requestProcessingError_;
};

bool QueryGateway::TryGetQueryName(Message & gatewayRequestMessage, __out wstring &queryName)
{
    QueryRequestMessageBody requestMessageBody;
    if (!gatewayRequestMessage.GetBody(requestMessageBody))
    {
        return false;
    }

    queryName = requestMessageBody.QueryName;
    return true;
}

QueryGateway::QueryGateway(Query::QueryMessageHandler & messageHandler)
    : messageHandler_(messageHandler)
    , nameEnumlookupMap_()
{
    // Build name look-up map
    for (auto i = QueryNames::FIRST_QUERY_MINUS_ONE + 1; i < QueryNames::LAST_QUERY_PLUS_ONE; i++)
    {
        QueryNames::Enum enumValue = static_cast<QueryNames::Enum>(i);
        nameEnumlookupMap_.insert(make_pair(ToString(enumValue), enumValue));
    }
}

AsyncOperationSPtr QueryGateway::BeginProcessIncomingQuery(
    Message & gatewayRequestMessage,
    Transport::FabricActivityHeader const & activityHeader,
    TimeSpan timeout,
    AsyncCallback callback,
    AsyncOperationSPtr parent)
{
    return AsyncOperation::CreateAndStart<ProcessIncomingQueryOperation>(
        gatewayRequestMessage,
        activityHeader,
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode QueryGateway::EndProcessIncomingQuery(
            AsyncOperationSPtr const & asyncOperation,
            _Out_ MessageUPtr & replyMessage)
{
    return ProcessIncomingQueryOperation::End(asyncOperation, replyMessage);
}

QueryResult QueryGateway::GetQueryList()
{
    vector<QueryMetadataQueryResult> queryMetadataResults;

    for (auto lookupEntry = nameEnumlookupMap_.begin(); lookupEntry != nameEnumlookupMap_.end(); ++lookupEntry)
    {
        StringCollection requiredArguments;
        StringCollection optionalArguments;

        auto specification = QuerySpecificationStore::Get().GetSpecification(lookupEntry->second, QueryArgumentMap());

        // For QuerySpecifications that are not persisted in the store, do not return it for verification.
        // Some query specifications are created dynamically with runtime generated input so they are not persisted in the store.
        if (!specification)
        {
            continue;
        }

        for(auto itArgument = specification->SupportedArguments.begin(); itArgument != specification->SupportedArguments.end(); ++itArgument)
        {
            if (itArgument->IsRequired)
            {
                requiredArguments.push_back(itArgument->Name);
            }
            else
            {
                optionalArguments.push_back(itArgument->Name);
            }
        }

        queryMetadataResults.push_back(QueryMetadataQueryResult(
            lookupEntry->first,
            requiredArguments,
            optionalArguments));
    }

    return QueryResult(move(queryMetadataResults));
}
