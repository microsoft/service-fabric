// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Communication::TcpServiceCommunication;
using namespace std;
using namespace Transport;
using namespace Api;

static const StringLiteral TraceType("ServiceCommunicationClient.DisconnectAsyncOperation");

ServiceCommunicationClient::DisconnectAsyncOperation::DisconnectAsyncOperation(
    __in ServiceCommunicationClient& owner,
    TimeSpan const& timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
    :TimedAsyncOperation(timeout,callback,parent),
    owner_(owner)    
{
}

ErrorCode ServiceCommunicationClient::DisconnectAsyncOperation::End(AsyncOperationSPtr const& operation)
{
    auto casted = AsyncOperation::End<DisconnectAsyncOperation>(operation);

    return casted->Error;
}

void ServiceCommunicationClient::DisconnectAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    this->owner_.CloseClient();
    this->TryComplete(thisSPtr);
}




