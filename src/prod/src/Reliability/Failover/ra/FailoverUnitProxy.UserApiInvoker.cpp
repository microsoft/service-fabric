// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Common::ApiMonitoring;

ProxyErrorCode FailoverUnitProxy::UserApiInvoker::GetError(ErrorCode && userApiError)
{
    if (userApiError.IsSuccess())
    {
        return ProxyErrorCode::Success();
    }
    else
    {
        return ProxyErrorCode::Create(move(userApiError), move(description_));
    }
}

void FailoverUnitProxy::UserApiInvoker::TraceBeforeStart(Common::AcquireExclusiveLock & proxyLock) const
{
    owner_.TraceBeforeOperation(proxyLock, interface_, api_);
}

void FailoverUnitProxy::UserApiInvoker::TraceBeforeStart(
    Common::AcquireExclusiveLock & proxyLock,
    Common::TraceCorrelatedEventBase const & data) const
{
    owner_.TraceBeforeOperation(proxyLock, interface_, api_, data);
}

bool FailoverUnitProxy::UserApiInvoker::TryStartUserApi(
    Common::AcquireExclusiveLock & proxyLock)
{
    return TryStartUserApi(proxyLock, L"");
}

bool FailoverUnitProxy::UserApiInvoker::TryStartUserApi(
    Common::AcquireExclusiveLock & proxyLock,
    std::wstring const & metadata)
{
    auto & opMgr = GetOperationManager(proxyLock);

    auto rv = opMgr.TryStartOperation(api_, metadata);
    if (rv != nullptr)
    {
        /*
            The description also contains metadata which may not be available at the caller
            TODO: The medata should also come from the code that invokes the API
        */
        description_ = ApiMonitoring::ApiNameDescription(rv->Data.Api);
        return true;
    }
    else
    {
        return false;
    }
}

void FailoverUnitProxy::UserApiInvoker::FinishUserApi(
    Common::AcquireWriteLock & grab,
    Common::ErrorCode const & errorCode)
{
    GetOperationManager(grab).FinishOperation(api_, errorCode);
}

FailoverUnitProxyOperationManagerBase & FailoverUnitProxy::UserApiInvoker::GetOperationManager(Common::AcquireWriteLock &)
{
    if (interface_ == ApiMonitoring::InterfaceName::IReplicator)
    {
        return *owner_.replicatorOperationManager_;
    }
    else
    {
        return *owner_.serviceOperationManager_;
    }
}
