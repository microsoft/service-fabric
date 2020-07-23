// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpServer;

NTSTATUS HttpServerImpl::CloseKtlAsyncServiceOperation::StartKtlServiceOperation(
    __in KAsyncContextBase* const parentAsync,
    __in KAsyncServiceBase::CloseCompletionCallback callbackPtr)
{
    return service_->StartClose(parentAsync, callbackPtr);
}

ErrorCode HttpServerImpl::CloseKtlAsyncServiceOperation::End(__in AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<CloseKtlAsyncServiceOperation>(operation);
    return thisPtr->Error;
}
