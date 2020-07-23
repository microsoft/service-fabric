// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace Query;
using namespace ServiceModel;
using namespace Naming;
using namespace HttpGateway;
using namespace HttpServer;

StringLiteral const TraceType("EventsStoreHandler");

EventsStoreHandler::EventsStoreHandlerAsyncOperation::EventsStoreHandlerAsyncOperation(
    RequestHandlerBase & owner,
    IRequestMessageContextUPtr messageContext,
    Common::AsyncCallback const &callback,
    Common::AsyncOperationSPtr const &parent)
    : RequestHandlerBase::HandlerAsyncOperation(owner, std::move(messageContext), callback, parent)
{
}

void EventsStoreHandler::EventsStoreHandlerAsyncOperation::GetRequestBody(__in Common::AsyncOperationSPtr const& thisSPtr)
{
    // Skip reading the body for system reverse proxy cases.
    Uri.Handler(thisSPtr);
}

EventsStoreHandler::EventsStoreHandler(HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

Common::AsyncOperationSPtr EventsStoreHandler::BeginProcessRequest(
    __in IRequestMessageContextUPtr request,
    __in Common::AsyncCallback const& callback,
    __in Common::AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<EventsStoreHandlerAsyncOperation>(*this, move(request), callback, parent);
}

ErrorCode EventsStoreHandler::Initialize()
{
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::EventsStoreHandlerPath,
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ProcessReverseProxyRequest),
        false));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::EventsStoreHandlerPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(ProcessReverseProxyRequest),
        false));

    return server_.InnerServer->RegisterHandler(Constants::EventsStoreHandlerPath, shared_from_this());
}

void EventsStoreHandler::ProcessReverseProxyRequest(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    std::unordered_map<std::wstring, std::wstring> headersToAdd;

    std::pair<std::wstring, std::wstring>  roleHeader(Constants::ClientRoleHeaderName, Transport::RoleMask::EnumToString(handlerOperation->FabricClient.ClientRole));
    headersToAdd.insert(roleHeader);
    handlerOperation->SetAdditionalHeadersToSend(move(headersToAdd));

    // Set the service name as Events store service
    handlerOperation->SetServiceName(Constants::EventsStoreServiceName);

    WriteInfo(
        TraceType,
        "ProcessRequest: New request is received ClientRequestId:{0}, Request:{1}",
        handlerOperation->MessageContextUPtr->GetClientRequestId(),
        handlerOperation->MessageContextUPtr->GetSuffix());

    AsyncOperationSPtr operation = server_.AppGatewayHandler->BeginProcessReverseProxyRequest(
        Common::Guid::NewGuid().ToString(),
        handlerOperation->MessageContextUPtr,
        nullptr,
        wstring(),
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnProcessReverseProxyRequestComplete(operation, false);
    },
        thisSPtr);

    OnProcessReverseProxyRequestComplete(operation, true);
}

void EventsStoreHandler::OnProcessReverseProxyRequestComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = ErrorCodeValue::Success;
    error = server_.AppGatewayHandler->EndProcessReverseProxyRequest(operation);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "OnProcessReverseProxyRequestComplete failed with {0}", error);
    }

    // Complete the async. AppGatewayHandler ProcessReverseProxyRequest would have responded to the HTTP request.
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    handlerOperation->TryComplete(operation->Parent, error);
}