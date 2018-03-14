// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Client;
using namespace ClientServerTransport;
using namespace ServiceModel;

AsyncOperationSPtr PassThroughSendTarget::BeginReceiveNotification(
    MessageUPtr &&message,
    TimeSpan timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return owningTransport_.BeginReceiveNotification(
        move(message),
        timeout,
        callback,
        parent);
}

ErrorCode PassThroughSendTarget::EndReceiveNotification(
    AsyncOperationSPtr const &operation,
    ClientServerReplyMessageUPtr &reply)
{
    return owningTransport_.EndReceiveNotification(
        operation,
        reply);
}
