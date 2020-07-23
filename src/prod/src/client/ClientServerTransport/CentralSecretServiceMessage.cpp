// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;
using namespace SystemServices;
using namespace Client;
using namespace ClientServerTransport;

GlobalWString CentralSecretServiceMessage::GetSecretsAction = make_global<wstring>(L"GetSecretsAction");
GlobalWString CentralSecretServiceMessage::SetSecretsAction = make_global<wstring>(L"SetSecretsAction");
GlobalWString CentralSecretServiceMessage::RemoveSecretsAction = make_global<wstring>(L"RemoveSecretsAction");
GlobalWString CentralSecretServiceMessage::GetSecretVersionsAction = make_global<wstring>(L"GetSecretVersionsAction");

void WrapForCentralSecretService(ClientServerRequestMessage & requestMessage)
{
    ServiceRoutingAgentMessage::WrapForRoutingAgent(requestMessage, ServiceTypeIdentifier::CentralSecretServiceTypeId);
}

CentralSecretServiceMessage::CentralSecretServiceMessage(
    wstring const & action,
    unique_ptr<ClientServerMessageBody> && body)
    : ClientServerRequestMessage(action, Actor::CSS, move(body))
{
    WrapForCentralSecretService(*this);
}

CentralSecretServiceMessage::CentralSecretServiceMessage(
    wstring const & action,
    unique_ptr<ClientServerMessageBody> && body,
    Common::ActivityId const & activityId)
    : ClientServerRequestMessage(action, Actor::CSS, move(body), activityId)
{
    WrapForCentralSecretService(*this);
}

CentralSecretServiceMessage::CentralSecretServiceMessage(
    wstring const & action,
    Common::ActivityId const & activityId)
    : ClientServerRequestMessage(action, Actor::CSS, activityId)
{
    WrapForCentralSecretService(*this);
}

CentralSecretServiceMessage::CentralSecretServiceMessage()
    : ClientServerRequestMessage()
{
    WrapForCentralSecretService(*this);
}

CentralSecretServiceMessageUPtr CentralSecretServiceMessage::CreateRequestMessage(
    wstring const & action,
    unique_ptr<ServiceModel::ClientServerMessageBody> && body)
{
    return make_unique<CentralSecretServiceMessage>(
            action, 
            move(body));
}

CentralSecretServiceMessageUPtr CentralSecretServiceMessage::CreateRequestMessage(
    wstring const & action,
    unique_ptr<ServiceModel::ClientServerMessageBody> && body,
    Common::ActivityId const & activityId)
{
    return make_unique<CentralSecretServiceMessage>(
        action,
        move(body),
        activityId);
}
