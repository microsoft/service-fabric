// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ClientServerTransport;

namespace Naming
{
    EntreeService::PingAsyncOperation::PingAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : RequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::PingAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        //
        // EntreeService doesnt listen on public endpoints. In regular operation, it listens on 
        // an IPC endpoint and in Zombiemode, it listens on the federation endpoint.
        //
        GatewayDescription gateway(
            L"",
            this->Properties.Instance,
            this->Properties.NodeName);

        MessageUPtr reply = NamingTcpMessage::GetGatewayPingReply(Common::make_unique<PingReplyMessageBody>(move(gateway)))->GetTcpMessage();
        reply->Headers.Add(*ClientProtocolVersionHeader::CurrentVersionHeader);

        this->SetReplyAndComplete(
            thisSPtr, 
            move(reply),
            ErrorCodeValue::Success);
    }

    void EntreeService::PingAsyncOperation::OnRetry(AsyncOperationSPtr const &)
    {
    }
}
