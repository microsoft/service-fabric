// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;

ComProxyReplicator::BuildIdleReplicaAsyncOperation::BuildIdleReplicaAsyncOperation(
    ReplicaDescription const & idleReplicaDescription,
    ComProxyReplicator & owner,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) 
    :   ComProxyAsyncOperation(callback, parent, true),
        owner_(owner),
        idleReplicaDescription_(idleReplicaDescription)
    {
    }

HRESULT ComProxyReplicator::BuildIdleReplicaAsyncOperation::BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
{
    ASSERT_IFNOT(idleReplicaDescription_.IsUp, "Unexpected replica status");
    
    ScopedHeap heap;

    FABRIC_REPLICA_INFORMATION replicaInfo = FailoverUnitProxy::ConfigurationUtility().CreateReplicaInformation(
        heap,
        idleReplicaDescription_,
        ReplicaRole::Idle,
        false,
        false);

    return owner_.primary_->BeginBuildReplica(&replicaInfo, callback, context);
}

HRESULT ComProxyReplicator::BuildIdleReplicaAsyncOperation::EndComAsyncOperation(IFabricAsyncOperationContext * context)
{
    return owner_.primary_->EndBuildReplica(context);
}

ErrorCode ComProxyReplicator::BuildIdleReplicaAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
{
    auto thisPtr = AsyncOperation::End<BuildIdleReplicaAsyncOperation>(asyncOperation);
    return thisPtr->Error;
}
