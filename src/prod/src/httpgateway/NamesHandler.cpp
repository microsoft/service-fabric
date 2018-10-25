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
using namespace Client;

StringLiteral const TraceType("NamesHandler");

NamesHandler::NamesHandler(__in HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

NamesHandler::~NamesHandler()
{
}

ErrorCode NamesHandler::Initialize()
{
    // POST to /Names/{name}/$/Create
    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::NamesEntitySetPath, Constants::Create),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(CreateName)));

    // DELETE to /Names/{name}
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::NamesEntityKeyPath,
        Constants::HttpDeleteVerb,
        MAKE_HANDLER_CALLBACK(DeleteName)));

    // GET to /Names/{name}
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::NamesEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(NameExists)));

    // GET to /Names/{name}/$/GetSubNames
    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::NamesEntityKeyPath, Constants::GetSubNames),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EnumerateSubNames)));

    // PUT to /Names/{name}/$/GetProperty
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::PropertyEntityKeyPathViaName,
        Constants::HttpPutVerb,
        MAKE_HANDLER_CALLBACK(PutProperty)));

    // GetProperty and GetProperties are distinguished here
    // because swagger doesn't support unique uris based on
    // query parameter alone

    // DELETE to /Names/{name}/$/GetProperty?PropertyName=
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::PropertyEntityKeyPathViaName,
        Constants::HttpDeleteVerb,
        MAKE_HANDLER_CALLBACK(DeleteProperty)));

    // GET to /Names/{name}/$/GetProperty?PropertyName=
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::PropertyEntityKeyPathViaName,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetProperty)));

    // GET to /Names/{name}/$/GetProperties
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::PropertiesEntityKeyPathViaName,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EnumerateProperties)));

    // POST to /Names/{name}/$/GetProperties/$/SubmitBatch
    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::PropertiesEntityKeyPathViaName, Constants::SubmitBatch),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(SubmitPropertyBatch)));

    return server_.InnerServer->RegisterHandler(Constants::NamesHandlerPath, shared_from_this());
}

void NamesHandler::CreateName(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    NameDescription nameDescData;
    auto error = handlerOperation->Deserialize(nameDescData, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri nameUri;
    error = NamingUri::TryParse(nameDescData.Name, Constants::HttpGatewayTraceId, nameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.PropertyMgmtClient->BeginCreateName(
        nameUri,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnCreateNameComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnCreateNameComplete(inner, true);
}

void NamesHandler::OnCreateNameComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.PropertyMgmtClient->EndCreateName(operation);
    if (!error.IsSuccess()) 
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusCreated, *Constants::StatusDescriptionCreated);
}

void NamesHandler::DeleteName(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri nameUri;
    auto error = argumentParser.TryGetName(nameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclientoperation.
    //
    auto inner = client.PropertyMgmtClient->BeginDeleteName(
        nameUri,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnDeleteNameComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnDeleteNameComplete(inner, true);
}

void NamesHandler::OnDeleteNameComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.PropertyMgmtClient->EndDeleteName(operation);
    if (!error.IsSuccess()) 
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void NamesHandler::NameExists(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri nameUri;
    auto error = argumentParser.TryGetName(nameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclientoperation.
    //
    auto inner = client.PropertyMgmtClient->BeginNameExists(
        nameUri,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnNameExistsComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnNameExistsComplete(inner, true);
}

void NamesHandler::OnNameExistsComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    bool nameExists;
    auto error = client.PropertyMgmtClient->EndNameExists(operation, nameExists);
    if (!error.IsSuccess()) 
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    if (nameExists)
    {
        handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
    }
    else
    {
        handlerOperation->OnError(operation->Parent, Constants::StatusNotFound, *Constants::StatusDescriptionNotFound);
    }
}

void NamesHandler::EnumerateSubNames(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri nameUri;
    auto error = argumentParser.TryGetName(nameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    std::wstring continuationToken;
    EnumerateSubNamesToken token;
    error = argumentParser.TryGetContinuationToken(continuationToken);
    if (error.IsError(ErrorCodeValue::NameNotFound))
    {
        error = ErrorCode::Success();
    }
    else if (error.IsSuccess())
    {
        error = EnumerateSubNamesToken::Create(continuationToken, token);
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool recursive = false;
    error = argumentParser.TryGetRecursive(recursive);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclientoperation.
    //
    auto inner = client.PropertyMgmtClient->BeginEnumerateSubNames(
        nameUri,
        token,
        recursive,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnEnumerateSubNamesComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnEnumerateSubNamesComplete(inner, true);
}

void NamesHandler::OnEnumerateSubNamesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    EnumerateSubNamesResult subNamesResult;
    auto error = client.PropertyMgmtClient->EndEnumerateSubNames(operation, subNamesResult);
    if (!error.IsSuccess()) 
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    std::wstring token;
    if (!subNamesResult.IsFinished())
    {
        error = subNamesResult.ContinuationToken.ToEscapedString(token);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, "EnumerateSubNamesToken.ToEscapedString failed with {0}", error);
            handlerOperation->OnError(operation->Parent, error);
            return;
        }
    }

    PagedSubNameInfoList subNameList(std::move(subNamesResult), std::move(token));

    ByteBufferUPtr responseBody;
    error = handlerOperation->Serialize(subNameList, responseBody);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(responseBody));
}

void NamesHandler::PutProperty(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri nameUri;
    auto error = argumentParser.TryGetName(nameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    PropertyDescription propDesc;
    error = handlerOperation->Deserialize(propDesc, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    error = propDesc.Verify();
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ByteBuffer value;
    error = propDesc.Value->GetValueBytes(value);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    if (StringUtility::AreEqualCaseInsensitive(propDesc.CustomTypeId, L""))
    {
        //
        // Make the actual fabricclientoperation.
        //
        auto inner = client.PropertyMgmtClient->BeginPutProperty(
            nameUri,
            propDesc.PropertyName,
            propDesc.Value->Kind,
            std::move(value),
            handlerOperation->Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnPutPropertyComplete(operation, false, false);
            },
            handlerOperation->shared_from_this());

        OnPutPropertyComplete(inner, true, false);
    }
    else
    {
        //
        // Make the actual fabricclientoperation.
        //
        auto inner = client.PropertyMgmtClient->BeginPutCustomProperty(
            nameUri,
            propDesc.PropertyName,
            propDesc.Value->Kind,
            std::move(value),
            propDesc.CustomTypeId,
            handlerOperation->Timeout,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnPutPropertyComplete(operation, false, true);
            },
            handlerOperation->shared_from_this());

        OnPutPropertyComplete(inner, true, true);
    }
}

void NamesHandler::OnPutPropertyComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously,
    bool isCustomProperty)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ErrorCode error;
    if (isCustomProperty)
    {
        error = client.PropertyMgmtClient->EndPutCustomProperty(operation);
    }
    else
    {
        error = client.PropertyMgmtClient->EndPutProperty(operation);
    }

    if (!error.IsSuccess()) 
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void NamesHandler::DeleteProperty(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri nameUri;
    auto error = argumentParser.TryGetName(nameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    std::wstring propName;
    error = argumentParser.TryGetPropertyName(propName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclientoperation.
    //
    auto inner = client.PropertyMgmtClient->BeginDeleteProperty(
        nameUri,
        propName,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnDeletePropertyComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnDeletePropertyComplete(inner, true);
}

void NamesHandler::OnDeletePropertyComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.PropertyMgmtClient->EndDeleteProperty(operation);
    if (!error.IsSuccess()) 
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void NamesHandler::GetProperty(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri nameUri;
    auto error = argumentParser.TryGetName(nameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    std::wstring propName;
    error = argumentParser.TryGetPropertyName(propName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclientoperation.
    //
    auto inner = client.PropertyMgmtClient->BeginGetProperty(
        nameUri,
        propName,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
        this->OnGetPropertyComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetPropertyComplete(inner, true);
}

void NamesHandler::OnGetPropertyComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    NamePropertyResult nameProperty;
    auto error = client.PropertyMgmtClient->EndGetProperty(operation, nameProperty);
    if (!error.IsSuccess()) 
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }
    
    PropertyInfo propInfo = PropertyInfo::Create(std::move(nameProperty), true);

    ByteBufferUPtr responseBody;
    error = handlerOperation->Serialize(propInfo, responseBody);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(responseBody));
}

void NamesHandler::EnumerateProperties(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri nameUri;
    auto error = argumentParser.TryGetName(nameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool includeValues = false;
    error = argumentParser.TryGetIncludeValues(includeValues);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    std::wstring continuationToken;
    EnumeratePropertiesToken token;
    error = argumentParser.TryGetContinuationToken(continuationToken);
    if (error.IsError(ErrorCodeValue::NameNotFound))
    {
        error = ErrorCode::Success();
    }
    else if (error.IsSuccess())
    {
        error = EnumeratePropertiesToken::Create(continuationToken, token);
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclientoperation.
    //
    auto inner = client.PropertyMgmtClient->BeginEnumerateProperties(
        nameUri,
        includeValues,
        token,
        handlerOperation->Timeout,
        [this, includeValues](AsyncOperationSPtr const & operation)
        {
            this->OnEnumeratePropertiesComplete(operation, false, includeValues);
        },
        handlerOperation->shared_from_this());

    OnEnumeratePropertiesComplete(inner, true, includeValues);
}

void NamesHandler::OnEnumeratePropertiesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously,
    bool includeValues)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    EnumeratePropertiesResult propResult;
    auto error = client.PropertyMgmtClient->EndEnumerateProperties(operation, propResult);
    if (!error.IsSuccess()) 
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    std::wstring token;
    if (!propResult.IsFinished())
    {
        error = propResult.ContinuationToken.ToEscapedString(token);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(operation->Parent, error);
            return;
        }
    }

    PagedPropertyInfoList propertyList(std::move(propResult), std::move(token), includeValues);

    ByteBufferUPtr responseBody;
    error = handlerOperation->Serialize(propertyList, responseBody);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(responseBody));
}

void NamesHandler::SubmitPropertyBatch(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri nameUri;
    auto error = argumentParser.TryGetName(nameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    PropertyBatchDescription propBatchDescription;
    error = handlerOperation->Deserialize(propBatchDescription, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    error = propBatchDescription.Verify();
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamePropertyOperationBatch batch(nameUri);
    error = batch.FromPublicApi(propBatchDescription.TakeOperations());
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclientoperation.
    //
    auto inner = client.PropertyMgmtClient->BeginSubmitPropertyBatch(
        std::move(batch),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnSubmitPropertyBatchComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnSubmitPropertyBatchComplete(inner, true);
}

void NamesHandler::OnSubmitPropertyBatchComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    NamePropertyOperationBatchResult batch;
    auto error = client.PropertyMgmtClient->EndSubmitPropertyBatch(operation, batch);
    if (!error.IsSuccess()) 
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    PropertyBatchInfo batchInfo(std::move(batch));

    ByteBufferUPtr responseBody;
    error = handlerOperation->Serialize(batchInfo, responseBody);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    if (batchInfo.Kind == PropertyBatchInfoType::Successful)
    {
        handlerOperation->OnSuccess(operation->Parent, move(responseBody));
        return;
    }
    else if (batchInfo.Kind == PropertyBatchInfoType::Failed)
    {
        handlerOperation->OnSuccess(operation->Parent, move(responseBody), Constants::StatusConflict, *Constants::StatusDescriptionConflict);
        return;
    }
    
    handlerOperation->OnError(operation->Parent, ErrorCodeValue::OperationFailed);
}
