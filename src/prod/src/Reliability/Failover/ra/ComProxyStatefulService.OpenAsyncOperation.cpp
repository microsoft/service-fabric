// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

HRESULT ComProxyStatefulService::OpenAsyncOperation::BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
{
    return owner_.service_->BeginOpen(openMode_, partition_.GetRawPointer(), callback, context);
}

HRESULT ComProxyStatefulService::OpenAsyncOperation::EndComAsyncOperation(IFabricAsyncOperationContext * context)
{
    HRESULT hr;

    hr = owner_.service_->EndOpen(context, replicator_.InitializationAddress());

    if(SUCCEEDED(hr))
    {
        ASSERT_IFNOT(replicator_, "Service open returned a null replicator.");
    }

    return hr;
}

ErrorCode ComProxyStatefulService::OpenAsyncOperation::End(AsyncOperationSPtr const & asyncOperation, __out ComProxyReplicatorUPtr & replication)
{
    auto thisPtr = AsyncOperation::End<ComProxyStatefulService::OpenAsyncOperation>(asyncOperation);

    if(thisPtr->Error.IsSuccess())
    {
        replication = make_unique<ComProxyReplicator>(thisPtr->replicator_);
    }

    return thisPtr->Error;
}
