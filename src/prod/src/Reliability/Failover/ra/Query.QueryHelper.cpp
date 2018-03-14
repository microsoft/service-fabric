// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace ::Query;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ReconfigurationAgentComponent::Query;
using namespace ServiceModel;
using namespace Infrastructure;

const wstring InvalidQueryMessageParseResultTypeMessage = L"QueryMessageParseResult {0} type is invalid, only QueryMessageParseResult::Single is supported.";
const wstring InvalidQueryNameMessage = L"Provided query name {0} is not valid.";

QueryHelper::QueryHelper(
    ReconfigurationAgent & ra)
    : ra_(ra)
    , queryUtility_()
    , deployedServiceReplicaUtility_()
    , replicaListQueryHandler_(make_shared<ReplicaListQueryHandler>(
        ReplicaListQueryHandler(
            ra,
            queryUtility_,
            deployedServiceReplicaUtility_)))
    , replicaDetailQueryHandler_(make_shared<ReplicaDetailQueryHandler>(
        ReplicaDetailQueryHandler(
            ra,
            queryUtility_,
            deployedServiceReplicaUtility_)))
{
}

void QueryHelper::ProcessMessage(
    Transport::Message & requestMessage,
    Federation::RequestReceiverContextUPtr && context) const
{
    QueryRequestMessageBodyInternal messageBody;
    wstring activityId;
    auto parseResult = ParseQueryMessage(requestMessage, messageBody, activityId);

    if (parseResult != QueryMessageUtility::QueryMessageParseResult::Single)
    {
        // Drop all parallel/forward query messages
        // There are no such operations on the RA
        queryUtility_.SendErrorResponseForQuery(
            ra_,
            activityId,
            ErrorCodeValue::InvalidOperation,
            *context);
        return;
    }

    IQueryHandlerSPtr queryHandler;
    auto error = TryGetQueryHandler(messageBody, queryHandler);
    if (!error.IsSuccess())
    {
        queryUtility_.SendErrorResponseForQuery(ra_, activityId, error, *context);
        return;
    }

    queryHandler->ProcessQuery(move(context), messageBody.QueryArgs, activityId);
}

QueryMessageUtility::QueryMessageParseResult::Enum QueryHelper::ParseQueryMessage(
    Transport::Message & requestMessage,
    __out QueryRequestMessageBodyInternal & messageBodyOut,
    __out wstring & activityIdOut)
{
    // Parse the message
    wstring addressSegment, addressSegmentMetadata; //unused
    Common::ActivityId activityIdObj;
    auto parseResult = QueryMessageUtility::ParseQueryMessage(
        requestMessage,
        QueryAddresses::RAAddressSegment,
        addressSegment,
        addressSegmentMetadata,
        activityIdObj,
        messageBodyOut);

    activityIdOut = activityIdObj.ToString();

    return parseResult;
}

// Selects appropriate queryHandler and entityEntries.
ErrorCode QueryHelper::TryGetQueryHandler(
    QueryRequestMessageBodyInternal & messageBody,
    __out IQueryHandlerSPtr & handler) const
{
    switch (messageBody.QueryName)
    {
    case QueryNames::GetServiceReplicaListDeployedOnNode:
    {
        handler = replicaListQueryHandler_;
        break;
    }
    case QueryNames::GetDeployedServiceReplicaDetail:
    {
        handler = replicaDetailQueryHandler_;
        break;
    }
    default:
        return ErrorCode(ErrorCodeValue::InvalidOperation, wformatString(InvalidQueryNameMessage, messageBody.QueryName));
    }

    return ErrorCodeValue::Success;
}
