// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;
using namespace Transport;
using namespace TestCommon;

StringLiteral const TraceSource("TestStoreServiceProxy");

TestStoreServiceProxy::TestStoreServiceProxy(
    wstring const& clientId,
    wstring const& serviceLocation,
    wstring const& partitionId,
    wstring const& serviceName,
    Federation::NodeId const& nodeId)
    : clientId_(clientId),
    serviceLocation_(serviceLocation),
    partitionId_(partitionId),
    serviceName_(serviceName),
    nodeId_(nodeId)
{
    TestSession::WriteNoise(TraceSource, partitionId_, "TestStoreServiceProxy created");
}

TestStoreServiceProxy::~TestStoreServiceProxy()
{
    TestSession::WriteNoise(TraceSource, partitionId_, "TestStoreServiceProxy destructed");
}

ErrorCode TestStoreServiceProxy::Put(
    /* [in] */ __int64 key,
    /* [in] */ std::wstring const& value,
    /* [in] */ FABRIC_SEQUENCE_NUMBER currentVersion,
    /* [in] */ std::wstring serviceName,
    /* [retval][out] */ FABRIC_SEQUENCE_NUMBER & newVersion,
    /* [retval][out] */ ULONGLONG & dataLossVersion)
{
    MessageId messageId;
    TestSession::WriteNoise(TraceSource, partitionId_, "Put called key {0}, value {1} and messageId {2}", key, value, messageId);

    wstring command = wformatString(
        "{0} {1} {2} {3} {4} {5} {6}",
        FabricTestCommands::PutDataCommand,
        key,
        value,
        currentVersion,
        serviceName,
        serviceLocation_,
        messageId);

    auto waiter = make_shared<AsyncOperationWaiter>();
    auto operation = FABRICSESSION.BeginSendClientCommandRequest(
        command,
        messageId,
        clientId_,
        TimeSpan::FromSeconds(60),
        [this, waiter] (AsyncOperationSPtr const &) 
    { 
        waiter->Set();
    },
        AsyncOperationSPtr());

    waiter->WaitOne();

    wstring commandReply;
    auto error = FABRICSESSION.EndSendClientCommandRequest(operation, commandReply);
    if(!commandReply.empty())
    {
        vector<wstring> returnValues;
        StringUtility::Split<wstring>(commandReply, returnValues, L" ");
        TestSession::FailTestIfNot(returnValues.size() == 2, "Incorrect return value at TestStoreServiceProxy::Put: {0}", commandReply);
        TestSession::FailTestIfNot(StringUtility::TryFromWString(returnValues[0], newVersion), "Could not parse as FABRIC_SEQUENCE_NUMBER {0}", returnValues[0]);
        TestSession::FailTestIfNot(StringUtility::TryFromWString(returnValues[1], dataLossVersion), "Could not parse as ULONGLONG {0}", returnValues[1]);
    }

    return error;
}

ErrorCode TestStoreServiceProxy::Get(
    /* [in] */ __int64 key,
    /* [in] */ std::wstring serviceName,
    /* [retval][out] */ std::wstring & value,
    /* [retval][out] */ FABRIC_SEQUENCE_NUMBER & version,
    /* [retval][out] */ ULONGLONG & dataLossVersion)
{
    MessageId messageId;
    TestSession::WriteNoise(TraceSource, partitionId_, "Get called key {0}, value {1} and messageId {2}", key, value, messageId);
    wstring command = wformatString(
        "{0} {1} {2} {3} {4}",
        FabricTestCommands::GetDataCommand,
        key,
        serviceName,
        serviceLocation_,
        messageId);

    auto waiter = make_shared<AsyncOperationWaiter>();
    auto operation = FABRICSESSION.BeginSendClientCommandRequest(
        command,
        messageId,
        clientId_,
        TimeSpan::FromSeconds(60),
        [this, waiter] (AsyncOperationSPtr const &) 
    { 
        waiter->Set();
    },
        AsyncOperationSPtr());

    waiter->WaitOne();

    wstring commandReply;
    auto error = FABRICSESSION.EndSendClientCommandRequest(operation, commandReply);
    if(!commandReply.empty())
    {
       vector<wstring> returnValues;
       StringUtility::Split<wstring>(commandReply, returnValues, L" ");
       TestSession::FailTestIfNot(returnValues.size() == 3, "Incorrect return value at TestStoreServiceProxy::Get: {0}", commandReply);
       value = returnValues[0] == L"Empty" ? L"" : returnValues[0];
       TestSession::FailTestIfNot(StringUtility::TryFromWString(returnValues[1], version), "Could not parse as FABRIC_SEQUENCE_NUMBER {0}", returnValues[1]);
       TestSession::FailTestIfNot(StringUtility::TryFromWString(returnValues[2], dataLossVersion), "Could not parse as ULONGLONG {0}", returnValues[2]);
    }

    return error;
}
