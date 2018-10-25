// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;

void HttpAuthHandler::AccessCheckAsyncOperation::OnStart(__in AsyncOperationSPtr const& thisSPtr)
{

    // Start the timed async for non SSL based security providers. 
    // For SSL/cert based access check, read certificate operation will manage its time.
    if (owner_.handlerType_ != Transport::SecurityProvider::Enum::Ssl)
    {
        TimedAsyncOperation::OnStart(thisSPtr);
    }

    AcquireReadLock(owner_.lock_);
    owner_.OnCheckAccess(thisSPtr);
}

ErrorCode HttpAuthHandler::AccessCheckAsyncOperation::End(
    Common::AsyncOperationSPtr const& operation,
    __out USHORT & httpStatusOnError,
    __out std::wstring & headerName,
    __out std::wstring & headerValue,
    __in Transport::RoleMask::Enum &roleOnSuccess)
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
        roleOnSuccess = thisPtr->owner_.role_;
    }

    return thisPtr->Error;
}