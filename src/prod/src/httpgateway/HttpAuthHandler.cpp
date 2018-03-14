// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace Api;
using namespace Client;
using namespace HttpGateway;
using namespace Transport;
using namespace HttpServer;

StringLiteral const TraceType("HttpAuthHandler");

AsyncOperationSPtr HttpAuthHandler::BeginCheckAccess(
    IRequestMessageContext & request,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<AccessCheckAsyncOperation>(
        *this,
        request,
        timeout,
        callback,
        parent);
}

ErrorCode HttpAuthHandler::EndCheckAccess(
    AsyncOperationSPtr const& operation,
    __out USHORT &httpStatusOnError,
    __inout wstring & authHeaderName,
    __inout wstring & authHeaderValue,
    __out Transport::RoleMask::Enum& roleOnSuccess)
{
    return AccessCheckAsyncOperation::End(operation, httpStatusOnError, authHeaderName, authHeaderValue, roleOnSuccess);
}
