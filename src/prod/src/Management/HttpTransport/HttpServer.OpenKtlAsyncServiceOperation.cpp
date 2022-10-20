// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpServer;
using namespace HttpCommon;
using namespace Transport;

NTSTATUS HttpServerImpl::OpenKtlAsyncServiceOperation::StartKtlServiceOperation(
    __in KAsyncContextBase* const parentAsync,
    __in KAsyncServiceBase::OpenCompletionCallback callbackPtr)
{
    KUriView uriView(listenUri_.c_str());
    KHttpUtils::HttpAuthType authType;
    if (SecurityProvider::IsWindowsProvider(credentialType_))
    {
        authType = KHttpUtils::AuthKerberos;
    }
    else if (credentialType_ == SecurityProvider::Enum::Ssl)
    {
        authType = KHttpUtils::AuthCertificate;
    }
    else
    {
        authType = KHttpUtils::AuthNone;
    }

    return service_->StartOpen(
        uriView,
        reqHandler_,
        reqHeaderHandler_,
        activeListeners_,
        HttpConstants::DefaultHeaderBufferSize,
        HttpConstants::MaxEntityBodySize,
        parentAsync,
        callbackPtr,
        nullptr,
        authType,
        0);
}

ErrorCode HttpServerImpl::OpenKtlAsyncServiceOperation::End(__in AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<OpenKtlAsyncServiceOperation>(operation);
    return thisPtr->Error;
}
