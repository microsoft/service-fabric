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

    ComReplicator::ChangeRoleOperation::ChangeRoleOperation(
        Replicator & replicator, 
        FABRIC_EPOCH const & epoch, 
        FABRIC_REPLICA_ROLE role) 
        :   ComReplicator::BaseOperation(replicator), 
            epoch_(epoch),
            role_(role)
    {
    }

    HRESULT ComReplicator::ChangeRoleOperation::End(
        __in IFabricAsyncOperationContext * context)
    {
        if(context == NULL) { return E_POINTER; }
        ComPointer<ComReplicator::ChangeRoleOperation> asyncOperation(
            context, CLSID_ComReplicator_ChangeRoleOperation);
        HRESULT hr = asyncOperation->ComReplicator::BaseOperation::End();
        if (FAILED(hr)) { return hr; }
        
        return asyncOperation->Result;
    }

    void ComReplicator::ChangeRoleOperation::OnStart(
        Common::AsyncOperationSPtr const & proxySPtr)
    {
        auto inner = this->ReplicatorEngine.BeginChangeRole(
            epoch_,
            role_,
            [this](AsyncOperationSPtr const & asyncOperation) { this->ChangeRoleCallback(asyncOperation); },
            proxySPtr);

        if (inner->CompletedSynchronously)
        {
            this->FinishChangeRole(inner);
        }
    }

    void ComReplicator::ChangeRoleOperation::ChangeRoleCallback(
        AsyncOperationSPtr const & asyncOperation)
    {
        if (!asyncOperation->CompletedSynchronously)
        {
            this->FinishChangeRole(asyncOperation);
        }
    }

    void ComReplicator::ChangeRoleOperation::FinishChangeRole(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto error = this->ReplicatorEngine.EndChangeRole(asyncOperation);
        this->TryComplete(asyncOperation->Parent, error.ToHResult());
    }

    } // end namespace ReplicationComponent
} // end namespace Reliability
