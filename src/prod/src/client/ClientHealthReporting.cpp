// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Client
{
    using namespace Common;
    using namespace std;
    using namespace Transport;
    
    ClientHealthReporting::ClientHealthReporting(
        ComponentRoot const & root,
        __in FabricClientImpl &client,
        wstring const & traceContext)
        : RootedObject(root)
        , healthReportTransportUPtr_()
        , healthClient_()
        , client_(client)
        , traceContext_(traceContext)
    {
    }

    ErrorCode ClientHealthReporting::OnOpen()
    {
        healthReportTransportUPtr_ = make_unique<FabricClientHealthReportTransport>(get_Root(), client_);

        healthClient_ = make_shared<HealthReportingComponent>(
            *healthReportTransportUPtr_,
            FabricClientInternalSettingsHolder(client_),
            traceContext_);

        return healthClient_->Open();
    }

    ErrorCode ClientHealthReporting::OnClose()
    {
        return healthClient_->Close();
    }

    void ClientHealthReporting::OnAbort()
    {
        healthClient_->Abort();
    }

    ClientHealthReporting::FabricClientHealthReportTransport::FabricClientHealthReportTransport(
        ComponentRoot const & root,
        __in FabricClientImpl &client)
        : HealthReportingTransport(root)
        , client_(client)
    {
    }

    AsyncOperationSPtr ClientHealthReporting::FabricClientHealthReportTransport::BeginReport(
        Transport::MessageUPtr && message,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return client_.BeginInternalForwardToService(
            move(ClientServerRequestMessageTransportMessageWrapper::Create(move(message))), 
            timeout,
            callback,
            parent);
    }

    ErrorCode ClientHealthReporting::FabricClientHealthReportTransport::EndReport(
        AsyncOperationSPtr const & operation,
        __out Transport::MessageUPtr & reply)
    {
        ClientServerReplyMessageUPtr replyMsg;
        ErrorCode error = client_.EndInternalForwardToService(operation, replyMsg);
        if (error.IsSuccess())
        {
            reply = move(replyMsg->GetTcpMessage());
        }
        return error;
    }
}
