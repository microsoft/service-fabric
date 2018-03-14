// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

ComProxyReplicator::CatchupReplicaSetAsyncOperation::CatchupReplicaSetAsyncOperation(
    ::FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
    ComProxyReplicator & owner,
    Common::AsyncCallback callback,
    Common::AsyncOperationSPtr const & parent) 
    :   Common::ComProxyAsyncOperation(callback, parent, true),
        catchUpMode_(catchUpMode),
        owner_(owner)
    {
        // Empty
    }

HRESULT ComProxyReplicator::CatchupReplicaSetAsyncOperation::BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
{
    return owner_.primary_->BeginWaitForCatchUpQuorum(
        catchUpMode_, 
        callback, 
        context);
}

HRESULT ComProxyReplicator::CatchupReplicaSetAsyncOperation::EndComAsyncOperation(IFabricAsyncOperationContext * context)
{
    return owner_.primary_->EndWaitForCatchUpQuorum(context);
}

ErrorCode ComProxyReplicator::CatchupReplicaSetAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<CatchupReplicaSetAsyncOperation>(asyncOperation);
    return thisPtr->Error;
}
