// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace HttpGateway;
using namespace HttpServer;

RequestHandlerBase::RequestHandlerBase(HttpGatewayImpl & server)
    : enable_shared_from_this()
    , server_(server)
{
}

RequestHandlerBase::~RequestHandlerBase()
{
}

AsyncOperationSPtr RequestHandlerBase::BeginProcessRequest(
    __in IRequestMessageContextUPtr request,
    __in AsyncCallback const& callback,
    __in AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<HandlerAsyncOperation>(*this, move(request), callback, parent);
}

ErrorCode RequestHandlerBase::EndProcessRequest(__in AsyncOperationSPtr const& operation)
{
    return HandlerAsyncOperation::End(operation);
}

wstring RequestHandlerBase::GetLocation(wstring const& collection, wstring &id)
{
    wstring location = collection;
    StringUtility::TrimTrailing<wstring>(location, L"/");
    return location + L"/" + id;
}

