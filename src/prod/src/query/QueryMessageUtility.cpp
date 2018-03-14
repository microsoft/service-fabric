// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;
using namespace Query;

static const StringLiteral TraceType("QueryMessageUtility");

QueryMessageUtility::QueryMessageParseResult::Enum QueryMessageUtility::ParseQueryMessage(
    Transport::Message & requestMessage,
    std::wstring const & ownerAddressSegment, // address segment of the owner
    std::wstring & addressSegment, // next address segment
    std::wstring & addressSegmentMetadata, // metadata out
    Common::ActivityId & activityId,
    QueryRequestMessageBodyInternal & messageBody)
{
    QueryAddressHeader addressHeader;
    if (!requestMessage.Headers.TryReadFirst(addressHeader))
    {
        return QueryMessageParseResult::Drop;
    }

    QueryAddress address(addressHeader.Address);
    addressSegment = L"";
    addressSegmentMetadata = L"";

    auto error = address.GetNextSegmentTo(ownerAddressSegment, addressSegment, addressSegmentMetadata);
    if (!error.IsSuccess())
    {
        return QueryMessageParseResult::Drop;
    }

    if (addressSegment == L".")
    {
        if (!requestMessage.GetBody(messageBody))
        {
            return QueryMessageParseResult::Drop;
        }

        FabricActivityHeader activityHeader;
        if (!requestMessage.Headers.TryReadFirst(activityHeader))
        {
            WriteWarning(
                TraceType,
                "ActivityId not available in query message - addressSegment {0}",
                addressSegment);
            return QueryMessageParseResult::Drop;
        }

        activityId = activityHeader.ActivityId;
        QueryEventSource::EventSource->QueryProcessHandler(
            activityHeader.ActivityId,
            ownerAddressSegment,
            messageBody.QueryName);

        if (messageBody.QueryType == QueryType::Parallel)
        {
            return QueryMessageParseResult::Parallel;
        }
        else if (messageBody.QueryType == QueryType::Sequential)
        {
            return QueryMessageParseResult::Sequential;
        }
        else
        {
            return QueryMessageParseResult::Single;
        }
    }
    else
    {
        QueryEventSource::EventSource->QueryForward(
            ownerAddressSegment,
            addressSegment,
            addressSegmentMetadata);

        return QueryMessageParseResult::Forward;
    }
}
