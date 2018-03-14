// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

HRESULT ComProxyStatelessService::CloseAsyncOperation::BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
{
    return owner_.service_->BeginClose(callback, context);
}

HRESULT ComProxyStatelessService::CloseAsyncOperation::EndComAsyncOperation(IFabricAsyncOperationContext * context)
{
    return owner_.service_->EndClose(context);
}

ErrorCode ComProxyStatelessService::CloseAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<ComProxyStatelessService::CloseAsyncOperation>(asyncOperation);

    return thisPtr->Error;
}
