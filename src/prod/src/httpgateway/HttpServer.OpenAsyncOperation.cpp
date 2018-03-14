// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if defined(PLATFORM_UNIX)
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>
#include <random>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
# include <sys/time.h>
#endif

#endif

using namespace Common;
using namespace HttpGateway;
using namespace HttpServer;

void HttpGatewayImpl::OpenAsyncOperation::OnStart(__in AsyncOperationSPtr const& thisSPtr)
{
    auto error = owner_.InitializeServer();
    if (!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
    }

#if !defined(PLATFORM_UNIX)
    auto timeout = timeoutHelper_.GetRemainingTime();
    auto operation = owner_.serviceResolver_->BeginOpen(
        timeout,
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnServiceResolverOpenComplete(operation, false);
    },
        thisSPtr);

    OnServiceResolverOpenComplete(operation, true);
#else
    auto openOperation = owner_.httpServer_->BeginOpen(
        timeoutHelper_.GetRemainingTime(),
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnOpenComplete(operation, false);
    },
        thisSPtr);

    this->OnOpenComplete(openOperation, true);
#endif
}

#if !defined(PLATFORM_UNIX)
void HttpGatewayImpl::OpenAsyncOperation::OnServiceResolverOpenComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = owner_.serviceResolver_->EndOpen(operation);
    if (!error.IsSuccess())
    {
        TryComplete(operation->Parent, error);
        return;
    }

    auto openOperation = owner_.httpServer_->BeginOpen(
        timeoutHelper_.GetRemainingTime(),
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnOpenComplete(operation, false);
    },
        operation->Parent);

    this->OnOpenComplete(openOperation, true);
}
#endif

void HttpGatewayImpl::OpenAsyncOperation::OnOpenComplete(
    AsyncOperationSPtr const &operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = owner_.httpServer_->EndOpen(operation);

    TryComplete(operation->Parent, error);
}

ErrorCode HttpGatewayImpl::OpenAsyncOperation::End(AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
    return thisPtr->Error;
}
