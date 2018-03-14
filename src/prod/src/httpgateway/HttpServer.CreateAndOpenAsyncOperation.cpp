// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;

ErrorCode HttpGatewayImpl::CreateAndOpenAsyncOperation::End(
    __in AsyncOperationSPtr const& operation,
    __out HttpGatewayImplSPtr & server)
{
    auto thisPtr = AsyncOperation::End<CreateAndOpenAsyncOperation>(operation);
    server = std::move(thisPtr->server_);
    return thisPtr->Error;
}

void HttpGatewayImpl::CreateAndOpenAsyncOperation::OnStart(__in AsyncOperationSPtr const& thisSPtr)
{
    //
    // Create the Http gateway and start it.
    //
    auto error = HttpGatewayImpl::Create(config_, server_);

    if (!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
        return;
    }

    server_->BeginOpen(
        TimeSpan::MaxValue,
        [this] (AsyncOperationSPtr const& operation)
        {
            auto errorCode = this->server_->EndOpen(operation);
            TryComplete(operation->Parent, errorCode);
        },
        thisSPtr);
}
