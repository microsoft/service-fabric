// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;

using namespace Naming;

EntreeService::ProcessServiceNotificationRequestAsyncOperation::ProcessServiceNotificationRequestAsyncOperation(
    __in GatewayProperties & properties,
    MessageUPtr && receivedMessage,
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
  : RequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
{
}

void EntreeService::ProcessServiceNotificationRequestAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
{
    TimedAsyncOperation::OnStart(thisSPtr);

    this->StartRequest(thisSPtr);
}

void EntreeService::ProcessServiceNotificationRequestAsyncOperation::StartRequest(AsyncOperationSPtr const& thisSPtr)
{
    MessageUPtr reply;

    auto error = this->Properties.ServiceNotificationManager.ProcessRequest(
        this->ReceivedMessage,
        reply);

    if (error.IsSuccess())
    {
        this->SetReplyAndComplete(thisSPtr, move(reply), ErrorCodeValue::Success);
    }

    this->TryComplete(thisSPtr, error);
}
