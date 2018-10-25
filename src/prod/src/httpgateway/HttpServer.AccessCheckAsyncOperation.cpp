// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;
using namespace Transport;

void HttpGatewayImpl::AccessCheckAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    if (owner_.securitySettings_.SecurityProvider() == SecurityProvider::None)
    {
        TryComplete(thisSPtr, ErrorCode::Success());
        return;
    }

    TESTASSERT_IF(owner_.httpAuthHandlers_.empty());
    
    StartOrContinueCheckAccess(thisSPtr);
}

void HttpGatewayImpl::AccessCheckAsyncOperation::StartOrContinueCheckAccess(AsyncOperationSPtr const& thisSPtr)
{
    if (handlerItr_ != owner_.httpAuthHandlers_.cend())
    {
        auto &handlerSPtr = *handlerItr_;

        auto inner = handlerSPtr->BeginCheckAccess(
            request_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const& operation)
            {
                this->OnCheckAccessComplete(operation, false);
            },
            thisSPtr);

        OnCheckAccessComplete(inner, true);
        return;
    }

    TryComplete(thisSPtr, authOperationError_);
}

void HttpGatewayImpl::AccessCheckAsyncOperation::OnCheckAccessComplete(
    AsyncOperationSPtr const& operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
    auto &handlerSPtr = *handlerItr_;

    authOperationError_ = handlerSPtr->EndCheckAccess(
        operation,
        httpStatusOnError_,
        authHeaderName_,
        authHeaderValue_,
        role_);

    if (authOperationError_.IsSuccess())
    {
        TryComplete(operation->Parent, authOperationError_);
        return;
    }

    ++handlerItr_;
    StartOrContinueCheckAccess(operation->Parent);
}

ErrorCode HttpGatewayImpl::AccessCheckAsyncOperation::End(
    Common::AsyncOperationSPtr const& operation,
    __out USHORT & httpStatusOnError,
    __out std::wstring & headerName,
    __out std::wstring & headerValue,
    __out RoleMask::Enum & role)
{
    auto thisPtr = AsyncOperation::End<AccessCheckAsyncOperation>(operation);
    if (thisPtr->Error.IsError(ErrorCodeValue::InvalidCredentials))
    {
        httpStatusOnError = thisPtr->httpStatusOnError_;
        headerName = thisPtr->authHeaderName_;
        headerValue = thisPtr->authHeaderValue_;
    }
    else if (thisPtr->Error.IsSuccess())
    {
        role = thisPtr->role_;
    }

    return thisPtr->Error;
}