// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

HRESULT ComProxyReplicator::CloseAsyncOperation::BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
{
    return owner_.replicator_->BeginClose(callback, context);
}

HRESULT ComProxyReplicator::CloseAsyncOperation::EndComAsyncOperation(IFabricAsyncOperationContext * context)
{
    return owner_.replicator_->EndClose(context);
}

ErrorCode ComProxyReplicator::CloseAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<ComProxyReplicator::CloseAsyncOperation>(asyncOperation);

    return thisPtr->Error;
}
