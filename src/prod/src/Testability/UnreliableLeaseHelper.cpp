// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Hosting2;
using namespace Query;
using namespace std;
using namespace ServiceModel;
using namespace Testability;

StringLiteral const TraceType("UnreliableLease");
wstring const LeaseBehaviorWarning = L"UnreliableLeaseHelper Warning:";

UnreliableLeaseHelper::UnreliableLeaseHelper()
{
}

UnreliableLeaseHelper::~UnreliableLeaseHelper()
{
}

ErrorCode UnreliableLeaseHelper::AddUnreliableLeaseBehavior(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    UNREFERENCED_PARAMETER(activityId);
    UNREFERENCED_PARAMETER(queryResult);

    wstring name;
    if (!queryArgument.TryGetValue(QueryResourceProperties::UnreliableLeaseBehavior::Alias, name) ||
        name.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    wstring behavior;
    if (!queryArgument.TryGetValue(QueryResourceProperties::UnreliableLeaseBehavior::Behavior, behavior) ||
        behavior.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    std::wstringstream behaviorStringStream(behavior);
    vector<wstring> behaviorStringTokens;
    do
    {
        wstring behaviorString;
        behaviorStringStream >> behaviorString;
        if (!behaviorString.empty())
        {
            behaviorStringTokens.push_back(move(behaviorString));
        }
    } while (behaviorStringStream);

    if (behaviorStringTokens.size() != 3)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    TRANSPORT_LISTEN_ENDPOINT ListenEndPointLocal = { 0 };
    TRANSPORT_LISTEN_ENDPOINT ListenEndPointRemote = { 0 };

    bool fromAny = false;
    bool toAny = false;
    if (behaviorStringTokens[0] == L"*")
    {
        fromAny = true;
    }
    else
    {
        LeaseWrapper::LeaseAgent::InitializeListenEndpoint(nullptr, behaviorStringTokens[0], ListenEndPointLocal);
    }
    if (behaviorStringTokens[1] == L"*")
    {
        toAny = true;
    }
    else
    {
        LeaseWrapper::LeaseAgent::InitializeListenEndpoint(nullptr, behaviorStringTokens[1], ListenEndPointRemote);
    }
    
    wstring messageType = behaviorStringTokens[2];
    LEASE_BLOCKING_ACTION_TYPE messageTypeEnum;
    if (messageType == L"request"){
        messageTypeEnum = LEASE_ESTABLISH_ACTION;
    }
    else if (messageType == L"response"){
        messageTypeEnum = LEASE_ESTABLISH_RESPONSE;
    }
    else if (messageType == L"indirect"){
        messageTypeEnum = LEASE_INDIRECT;
    }
    else if (messageType == L"ping_response"){
        messageTypeEnum = LEASE_PING_RESPONSE;
    }
    else if (messageType == L"ping_request"){
        messageTypeEnum = LEASE_PING_REQUEST;
    }
    else if (messageType == L"*"){
        messageTypeEnum = LEASE_BLOCKING_ALL;
    }
    else{
        return ErrorCodeValue::InvalidArgument;
    }
    
    #ifndef PLATFORM_UNIX

    ::addLeaseBehavior(&ListenEndPointLocal, fromAny, &ListenEndPointRemote, toAny, messageTypeEnum, name);
    
    #endif

    return ErrorCode();
}

ErrorCode UnreliableLeaseHelper::RemoveUnreliableLeaseBehavior(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    UNREFERENCED_PARAMETER(activityId);
    UNREFERENCED_PARAMETER(queryResult);
    wstring name;
    if (!queryArgument.TryGetValue(QueryResourceProperties::UnreliableLeaseBehavior::Alias, name) ||
        name.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    #ifndef PLATFORM_UNIX

    ::removeLeaseBehavior(name);
    
    #endif

    return ErrorCode();
}
