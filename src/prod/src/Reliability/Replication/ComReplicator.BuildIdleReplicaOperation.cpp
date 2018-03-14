// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability
{
    namespace ReplicationComponent 
    {

    using namespace Common;

    ComReplicator::BuildIdleReplicaOperation::BuildIdleReplicaOperation(
        Replicator & replicator, 
        FABRIC_REPLICA_INFORMATION const * idleReplica)
        : ComReplicator::BaseOperation(replicator),
          replica_(idleReplica)
    {
    }

    HRESULT ComReplicator::BuildIdleReplicaOperation::End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }
        ComPointer<BuildIdleReplicaOperation> asyncOperation(
            context, CLSID_ComReplicator_BuildIdleReplicaOperation);
        HRESULT hr = asyncOperation->ComReplicator::BaseOperation::End();
        if (FAILED(hr)) { return hr; }

        return asyncOperation->Result;
    }

    void ComReplicator::BuildIdleReplicaOperation::OnStart(
        Common::AsyncOperationSPtr const & proxySPtr)
    {
        auto inner = this->ReplicatorEngine.BeginBuildReplica(
            replica_,
            [this](AsyncOperationSPtr const & asyncOperation) { this->BuildIdleReplicaCallback(asyncOperation); },
            proxySPtr);

        if (inner->CompletedSynchronously)
        {
            this->FinishBuildIdleReplica(inner);
        }
    }

    void ComReplicator::BuildIdleReplicaOperation::BuildIdleReplicaCallback(
        AsyncOperationSPtr const & asyncOperation)
    {
        if (!asyncOperation->CompletedSynchronously)
        {
            this->FinishBuildIdleReplica(asyncOperation);
        }
    }

    void ComReplicator::BuildIdleReplicaOperation::FinishBuildIdleReplica(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto error = this->ReplicatorEngine.EndBuildReplica(asyncOperation);
        this->TryComplete(asyncOperation->Parent, error.ToHResult());
    }

    } // end namespace ReplicationComponent
} // end namespace Reliability
