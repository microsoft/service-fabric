// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;

void HttpGatewayImpl::CloseAsyncOperation::OnStart(__in AsyncOperationSPtr const& thisSPtr)
{
    auto closeOperation = owner_.httpServer_->BeginClose(
        timeoutHelper_.GetRemainingTime(),
        [this](AsyncOperationSPtr const &operation)
        {
            this->OnCloseComplete(operation, false);
        },
        thisSPtr);

    this->OnCloseComplete(closeOperation, true);
}

void HttpGatewayImpl::CloseAsyncOperation::OnCloseComplete(
    AsyncOperationSPtr const &operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = owner_.httpServer_->EndClose(operation);
    TryComplete(operation->Parent, error);
}

ErrorCode HttpGatewayImpl::CloseAsyncOperation::End(AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
    return thisPtr->Error;
}
