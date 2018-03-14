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

    ComReplicator::CatchupReplicaSetOperation::CatchupReplicaSetOperation(
        Replicator & replicator, FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode) 
        : ComReplicator::BaseOperation(replicator),
          catchUpMode_(catchUpMode)
    {
    }

    HRESULT ComReplicator::CatchupReplicaSetOperation::End(
        __in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }
        ComPointer<CatchupReplicaSetOperation> asyncOperation(
            context, CLSID_ComReplicator_CatchupReplicaSetOperation);
        HRESULT hr = asyncOperation->ComReplicator::BaseOperation::End();
        if (FAILED(hr)) { return hr; }

        return asyncOperation->Result;
    }

    void ComReplicator::CatchupReplicaSetOperation::OnStart(
        Common::AsyncOperationSPtr const & proxySPtr)
    {
        auto inner = this->ReplicatorEngine.BeginWaitForCatchUpQuorum(
            catchUpMode_,
            [this](AsyncOperationSPtr const & asyncOperation) { this->CatchupReplicaSetCallback(asyncOperation); },
            proxySPtr);

        if (inner->CompletedSynchronously)
        {
            this->FinishCatchupReplicaSet(inner);
        }
    }

    void ComReplicator::CatchupReplicaSetOperation::CatchupReplicaSetCallback(
        AsyncOperationSPtr const & asyncOperation)
    {
        if (!asyncOperation->CompletedSynchronously)
        {
            this->FinishCatchupReplicaSet(asyncOperation);
        }
    }

    void ComReplicator::CatchupReplicaSetOperation::FinishCatchupReplicaSet(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto error = this->ReplicatorEngine.EndWaitForCatchUpQuorum(asyncOperation);
        this->TryComplete(asyncOperation->Parent, error.ToHResult());
    }

    } // end namespace ReplicationComponent
} // end namespace Reliability
