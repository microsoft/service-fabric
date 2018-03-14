// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "dummyservice.h"

using namespace std;
using namespace Common;
using namespace Transport;
using  namespace Communication::TcpServiceCommunication;

Common::AsyncOperationSPtr DummyService::BeginProcessRequest(
    wstring const & clientId,
    MessageUPtr && message, 
	TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
	UNREFERENCED_PARAMETER(clientId);
	UNREFERENCED_PARAMETER(message);
	UNREFERENCED_PARAMETER(timeout);

    return AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(Common::ErrorCodeValue::Success, callback, parent);
}

Common::ErrorCode DummyService::EndProcessRequest(
    AsyncOperationSPtr const &  operation,
    MessageUPtr & reply)
{
    reply = make_unique<Message>();
    Common::AsyncOperation::End<CompletedAsyncOperation>(operation);
    return ErrorCodeValue::Success;
}

Common::ErrorCode DummyService::HandleOneWay(
    std::wstring const & clientId,
    Transport::MessageUPtr && message)
{
	UNREFERENCED_PARAMETER(clientId);
	UNREFERENCED_PARAMETER(message);

    notificationHandled_.Set();
    return ErrorCodeValue::Success;
}
