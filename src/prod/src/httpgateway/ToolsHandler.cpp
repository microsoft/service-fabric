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
using namespace Management::FaultAnalysisService;

StringLiteral const TraceType("ToolsHandler");

ToolsHandler::ToolsHandler(
    HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

ToolsHandler::~ToolsHandler()
{
}

ErrorCode ToolsHandler::Initialize()
{
    HandlerUriTemplate startChaosUriTemplate = HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ToolsEntityKeyPath, Constants::Start),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StartChaos));
    validHandlerUris.push_back(startChaosUriTemplate);

    HandlerUriTemplate stopChaosUriTemplate = HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ToolsEntityKeyPath, Constants::Stop),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StopChaos));
    validHandlerUris.push_back(stopChaosUriTemplate);

    HandlerUriTemplate getChaosReportUriTemplate = HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ToolsEntityKeyPath, Constants::Report),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetChaosReport));
    validHandlerUris.push_back(getChaosReportUriTemplate);

    return server_.InnerServer->RegisterHandler(Constants::ToolsHandlerPath, shared_from_this());
}

void ToolsHandler::StartChaos(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    ChaosParameters data;
    auto error = handlerOperation->Deserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    auto paramsUPtr = make_unique<ChaosParameters>(move(data));
    StartChaosDescription description(move(paramsUPtr));

    auto inner = client.TestMgmtClient->BeginStartChaos(
        move(description),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnStartChaosComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnStartChaosComplete(inner, true);
}

void ToolsHandler::OnStartChaosComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously) const
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.TestMgmtClient->EndStartChaos(operation);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ToolsHandler::StopChaos(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto inner = client.TestMgmtClient->BeginStopChaos(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnStopChaosComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnStopChaosComplete(inner, true);
}

void ToolsHandler::OnStopChaosComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously) const
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.TestMgmtClient->EndStopChaos(operation);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}


void ToolsHandler::GetChaosReport(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    DateTime startTimeUtc;
    DateTime endTimeUtc;
    wstring continuationToken;

    bool continuationTokenFound = false;

    auto returnCode = argumentParser.TryGetContinuationToken(continuationToken);

    if (!returnCode.IsError(ErrorCodeValue::Success) && !returnCode.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, returnCode);
        return;
    }

    if (returnCode.IsError(ErrorCodeValue::Success))
    {
        continuationTokenFound = true;
    }

    bool startTimeFound = false;

    returnCode = argumentParser.TryGetStartTimeUtc(startTimeUtc);

    if (!returnCode.IsError(ErrorCodeValue::Success) && !returnCode.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, returnCode);
        return;
    }

    if (returnCode.IsError(ErrorCodeValue::Success))
    {
        startTimeFound = true;
    }

    bool endTimeFound = false;

    returnCode = argumentParser.TryGetEndTimeUtc(endTimeUtc);

    if (!returnCode.IsError(ErrorCodeValue::Success) && !returnCode.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, returnCode);
        return;
    }

    if (returnCode.IsError(ErrorCodeValue::Success))
    {
        endTimeFound = true;
    }

    if (continuationTokenFound && (startTimeFound || endTimeFound))
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    ChaosReportFilter filter(startTimeUtc, endTimeUtc);
    auto filterPtr = make_shared<ChaosReportFilter>(filter);

    wstring clientType = L"rest";

    GetChaosReportDescription description(filterPtr, continuationToken, clientType);

    auto inner = client.TestMgmtClient->BeginGetChaosReport(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetChaosReportComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetChaosReportComplete(inner, true);
}

void ToolsHandler::OnGetChaosReportComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously) const
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    IChaosReportResultPtr chaosReportResultPtr;
    auto error = client.TestMgmtClient->EndGetChaosReport(operation, chaosReportResultPtr);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ChaosReport report(move(*(chaosReportResultPtr->GetChaosReport())));

    ByteBufferUPtr bufferUPtr = nullptr;
    error = JsonSerialize(report, bufferUPtr);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}
