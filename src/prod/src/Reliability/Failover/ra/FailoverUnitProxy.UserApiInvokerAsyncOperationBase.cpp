// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

ProxyErrorCode FailoverUnitProxy::UserApiInvokerAsyncOperationBase::EndHelper() 
{
    auto error = Error;
    return invoker_.GetError(move(error));
}

void FailoverUnitProxy::UserApiInvokerAsyncOperationBase::TraceBeforeStart(Common::AcquireExclusiveLock & proxyLock) const
{
    invoker_.TraceBeforeStart(proxyLock);
}

bool FailoverUnitProxy::UserApiInvokerAsyncOperationBase::TryStartUserApi(
    Common::AcquireExclusiveLock & proxyLock,
    Common::AsyncOperationSPtr const & thisSPtr)
{
    return TryStartUserApi(proxyLock, L"", thisSPtr);
}

bool FailoverUnitProxy::UserApiInvokerAsyncOperationBase::TryStartUserApi(
    Common::AcquireExclusiveLock & proxyLock,
    std::wstring const & metadata,
    Common::AsyncOperationSPtr const & thisSPtr)
{
    if (!invoker_.TryStartUserApi(proxyLock, metadata))
    {
        bool didComplete = thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState);
        ASSERT_IFNOT(didComplete, "Failed to complete OpenInstanceAsyncOperation");
        return false;
    }
    else
    {
        return true;
    }
}

void FailoverUnitProxy::UserApiInvokerAsyncOperationBase::TryContinueUserApi(
    Common::AsyncOperationSPtr const & userOperation)
{
    bool cancel = false;

    {
        AcquireExclusiveLock grab(owner_.lock_);
        cancel = !userOperation->IsCompleted && !invoker_.GetOperationManager(grab).TryContinueOperation(invoker_.Api, userOperation);
    }

    if (cancel)
    {
        userOperation->Cancel();
    }
}

void FailoverUnitProxy::UserApiInvokerAsyncOperationBase::FinishUserApi(
    Common::AcquireWriteLock & proxyLock,
    Common::ErrorCode const & errorCode)
{
    invoker_.FinishUserApi(proxyLock, errorCode);
}
