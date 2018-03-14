// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Client;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;
using namespace Transport;


IpcHealthReportingClient::IpcHealthReportingClientTransport::IpcHealthReportingClientTransport(
    Common::ComponentRoot const & root,
    IpcHealthReportingClient & owner
    )
    : HealthReportingTransport(root)
    , owner_(owner)
{
}

AsyncOperationSPtr IpcHealthReportingClient::IpcHealthReportingClientTransport::BeginReport(
    Transport::MessageUPtr && message,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    message->Headers.Replace(ActorHeader(owner_.actor_));

    return owner_.ipcClient_.BeginRequest(
        move(message),
        timeout,
        callback,
        parent);
}

ErrorCode IpcHealthReportingClient::IpcHealthReportingClientTransport::EndReport(
    AsyncOperationSPtr const & operation,
    __out Transport::MessageUPtr & reply)
{
    return owner_.ipcClient_.EndRequest(operation, reply);
}

IpcHealthReportingClient::IpcHealthReportingClient(
    Common::ComponentRoot const & root,
    Transport::IpcClient & ipcClient,
    bool enableMaxNumberOfReportThrottle,
    std::wstring const & traceContext,
    Transport::Actor::Enum actor,
    bool isEnabled)
    : Common::FabricComponent()
    , Common::RootedObject(root)
    , ipcClient_(ipcClient)
    , healthClient_(nullptr)
    , enableMaxNumberOfReportThrottle(enableMaxNumberOfReportThrottle)
    , traceContext_(traceContext)
    , actor_(actor)
    , isEnabled_(isEnabled)
{
}

IpcHealthReportingClient::~IpcHealthReportingClient()
{
}

Common::ErrorCode IpcHealthReportingClient::OnOpen()
{
    healthClientTransport_ = make_unique<IpcHealthReportingClientTransport>(get_Root(), *this);
    healthClient_ = make_shared<HealthReportingComponent>(*healthClientTransport_,traceContext_,enableMaxNumberOfReportThrottle);
    return healthClient_->Open();
}

Common::ErrorCode IpcHealthReportingClient::OnClose()
{
    if (healthClient_)
    {
        return healthClient_->Close();
    }

    return ErrorCodeValue::Success;
}

void IpcHealthReportingClient::OnAbort()
{
    auto result = OnClose();
}


Common::ErrorCode IpcHealthReportingClient::AddReport(
    HealthReport && healthReport,
    HealthReportSendOptionsUPtr && sendOptions)
{
    ErrorCode error; 
    if (isEnabled_)
    {
        if (IsOpened())
        {
            return healthClient_->AddHealthReport(move(healthReport), move(sendOptions));
        }

        return ErrorCodeValue::InvalidState;
    }
    return error;
}

Common::ErrorCode IpcHealthReportingClient::AddReports(
    std::vector<ServiceModel::HealthReport> && healthReports,
    HealthReportSendOptionsUPtr && sendOptions)
{
    ErrorCode error;
    if (isEnabled_)
    {
        if (IsOpened())
        {
            return healthClient_->AddHealthReports(move(healthReports), move(sendOptions));
        }
        return ErrorCodeValue::InvalidState;
    }
    return error;
}




