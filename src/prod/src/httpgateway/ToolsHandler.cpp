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
    // Current
    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ChaosEntityKeyPath, Constants::Start),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StartChaos),
        Constants::V1ApiVersion));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ChaosEntityKeyPath, Constants::Stop),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StopChaos),
        Constants::V1ApiVersion));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ChaosEventSegmentsSetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetChaosEvents),
        Constants::V62ApiVersion));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ChaosEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetChaos),
        Constants::V62ApiVersion));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ChaosScheduleKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetChaosSchedule),
        Constants::V62ApiVersion));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ChaosScheduleKeyPath,
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PostChaosSchedule),
        Constants::V62ApiVersion));

    // Old
    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ChaosEntityKeyPath, Constants::Report),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetChaosReport),
        Constants::V1ApiVersion,
        Constants::V61ApiVersion));

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

void ToolsHandler::GetChaos(__in Common::AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto inner = client.TestMgmtClient->BeginGetChaos(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetChaosComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetChaosComplete(inner, true);
}

void ToolsHandler::OnGetChaosComplete(
    __in Common::AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously) const
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ISystemServiceApiResultPtr chaosResultPtr;
    auto error = client.TestMgmtClient->EndGetChaos(operation, chaosResultPtr);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    string chaosStateString = StringUtility::Utf16ToUtf8(chaosResultPtr->GetResult());
    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>(chaosStateString.begin(), chaosStateString.end());

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ToolsHandler::GetChaosEvents(__in Common::AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    DateTime startTimeUtc;
    DateTime endTimeUtc;
    ServiceModel::QueryPagingDescription queryPagingDescription;

    auto returnCode = argumentParser.TryGetStartTimeUtc(startTimeUtc);

    if (!returnCode.IsSuccess() && !returnCode.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, returnCode);
        return;
    }

    bool startTimeFound = returnCode.IsSuccess();

    returnCode = argumentParser.TryGetEndTimeUtc(endTimeUtc);

    if (!returnCode.IsSuccess() && !returnCode.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, returnCode);
        return;
    }

    bool endTimeFound = returnCode.IsSuccess();

    returnCode = argumentParser.TryGetPagingDescription(queryPagingDescription);
    if (!returnCode.IsSuccess() && !returnCode.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, returnCode);
        return;
    }

    // only allow to specify a continuation token or a filter time range, not both
    if (!queryPagingDescription.ContinuationToken.empty() && (startTimeFound || endTimeFound))
    {
        handlerOperation->OnError(thisSPtr, ErrorCode(ErrorCodeValue::InvalidArgument, StringResource::Get(IDS_FAS_GetChaosEventsArgumentInvalid)));
        return;
    }

    auto filterSPtr = make_shared<ChaosEventsFilter>(startTimeUtc, endTimeUtc);
    auto queryPagingDescriptionSPtr = make_shared<ServiceModel::QueryPagingDescription>(move(queryPagingDescription));

    wstring clientType(L"rest");

    GetChaosEventsDescription description(filterSPtr, queryPagingDescriptionSPtr, clientType);

    auto inner = client.TestMgmtClient->BeginGetChaosEvents(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetChaosEventsComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetChaosEventsComplete(inner, true);
}

void ToolsHandler::OnGetChaosEventsComplete(
    __in Common::AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously) const
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    IChaosEventsSegmentResultPtr chaosEventsSegmentResultPtr;
    auto error = client.TestMgmtClient->EndGetChaosEvents(operation, chaosEventsSegmentResultPtr);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ChaosEventsSegment eventsSegment(move(*(chaosEventsSegmentResultPtr->GetChaosEvents())));

    ByteBufferUPtr bufferUPtr = nullptr;
    error = JsonSerialize(eventsSegment, bufferUPtr);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
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

    if (!returnCode.IsSuccess() && !returnCode.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, returnCode);
        return;
    }

    if (returnCode.IsSuccess())
    {
        continuationTokenFound = true;
    }

    bool startTimeFound = false;

    returnCode = argumentParser.TryGetStartTimeUtc(startTimeUtc);

    if (!returnCode.IsSuccess() && !returnCode.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, returnCode);
        return;
    }

    if (returnCode.IsSuccess())
    {
        startTimeFound = true;
    }

    bool endTimeFound = false;

    returnCode = argumentParser.TryGetEndTimeUtc(endTimeUtc);

    if (!returnCode.IsSuccess() && !returnCode.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, returnCode);
        return;
    }

    if (returnCode.IsSuccess())
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

void ToolsHandler::GetChaosSchedule(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto inner = client.TestMgmtClient->BeginGetChaosSchedule(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetChaosScheduleComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetChaosScheduleComplete(inner, true);
}

void ToolsHandler::OnGetChaosScheduleComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously) const
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ISystemServiceApiResultPtr chaosScheduleResultPtr;
    auto error = client.TestMgmtClient->EndGetChaosSchedule(operation, chaosScheduleResultPtr);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    string scheduleString = StringUtility::Utf16ToUtf8(chaosScheduleResultPtr->GetResult());
    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>(scheduleString.begin(), scheduleString.end());

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ToolsHandler::PostChaosSchedule(__in Common::AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    wstring body(handlerOperation->Body->data(), handlerOperation->Body->data() + handlerOperation->Body->size());

    auto inner = client.TestMgmtClient->BeginSetChaosSchedule(
        body,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnPostChaosScheduleComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnPostChaosScheduleComplete(inner, true);
}

void ToolsHandler::OnPostChaosScheduleComplete(
    __in Common::AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously) const
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.TestMgmtClient->EndSetChaosSchedule(operation);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}