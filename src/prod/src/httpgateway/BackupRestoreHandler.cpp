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
using namespace Management::ClusterManager;
using namespace Management::RepairManager;
using namespace Management::UpgradeOrchestrationService;

StringLiteral const TraceType("BackupRestoreHandler");

BackupRestoreHandler::BackupRestoreHandlerAsyncOperation::BackupRestoreHandlerAsyncOperation(
    RequestHandlerBase & owner,
    IRequestMessageContextUPtr messageContext,
    Common::AsyncCallback const &callback,
    Common::AsyncOperationSPtr const &parent)
    : RequestHandlerBase::HandlerAsyncOperation(owner, std::move(messageContext), callback, parent)
{
}

void BackupRestoreHandler::BackupRestoreHandlerAsyncOperation::GetRequestBody(__in Common::AsyncOperationSPtr const& thisSPtr)
{
    // Skip reading the body for system reverse proxy cases.
    Uri.Handler(thisSPtr);
}

BackupRestoreHandler::BackupRestoreHandler(HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

Common::AsyncOperationSPtr BackupRestoreHandler::BeginProcessRequest(
    __in IRequestMessageContextUPtr request,
    __in Common::AsyncCallback const& callback,
    __in Common::AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<BackupRestoreHandlerAsyncOperation>(*this, move(request), callback, parent);
}

ErrorCode BackupRestoreHandler::Initialize()
{
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::BackupRestorePath,
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ProcessReverseProxyRequest),
        false));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::BackupRestorePath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(ProcessReverseProxyRequest),
        false));

    return server_.InnerServer->RegisterHandler(Constants::BackupRestoreHandlerPath, shared_from_this());
}

void BackupRestoreHandler::ProcessReverseProxyRequest(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    std::unordered_map<std::wstring, std::wstring> headersToAdd;
    
    std::pair<std::wstring, std::wstring>  roleHeader(Constants::ClientRoleHeaderName, Transport::RoleMask::EnumToString(handlerOperation->FabricClient.ClientRole));
    headersToAdd.insert(roleHeader);
    handlerOperation->SetAdditionalHeadersToSend(move(headersToAdd));

    // Set the service name as Backup restore service
    handlerOperation->SetServiceName(Constants::BackupRestoreServiceName);

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

void BackupRestoreHandler::OnProcessReverseProxyRequestComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = ErrorCodeValue::Success;
    error = server_.AppGatewayHandler->EndProcessReverseProxyRequest(operation);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "PerformBackupRestoreOperation EndProcessRequest failed with {0}", error);
    }

    // Complete the async. AppGatewayHandler ProcessReverseProxyRequest would have responded to the HTTP request.
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    handlerOperation->TryComplete(operation->Parent, error);
}
