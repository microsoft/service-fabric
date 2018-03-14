// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

HRESULT ComProxyReplicator::ChangeRoleAsyncOperation::BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
{
    return owner_.replicator_->BeginChangeRole(&epoch_, newRole_, callback, context);
}

HRESULT ComProxyReplicator::ChangeRoleAsyncOperation::EndComAsyncOperation(IFabricAsyncOperationContext * context)
{
    return owner_.replicator_->EndChangeRole(context);
}

ErrorCode ComProxyReplicator::ChangeRoleAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<ComProxyReplicator::ChangeRoleAsyncOperation>(asyncOperation);
    return thisPtr->Error;
}
