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

    ComReplicator::UpdateEpochOperation::UpdateEpochOperation(
        Replicator & replicator, 
        FABRIC_EPOCH const & epoch) 
        :   ComReplicator::BaseOperation(replicator), 
            epoch_(epoch)
    {
    }

    void ComReplicator::UpdateEpochOperation::OnStart(
        Common::AsyncOperationSPtr const & proxySPtr)
    {
        auto inner = this->ReplicatorEngine.BeginUpdateEpoch(
            epoch_,
            [this](AsyncOperationSPtr const & asyncOperation) { this->UpdateEpochCallback(asyncOperation); },
            proxySPtr);

        if (inner->CompletedSynchronously)
        {
            this->FinishUpdateEpoch(inner);
        }
    }

    void ComReplicator::UpdateEpochOperation::UpdateEpochCallback(
        AsyncOperationSPtr const & asyncOperation)
    {
        if (!asyncOperation->CompletedSynchronously)
        {
            this->FinishUpdateEpoch(asyncOperation);
        }
    }

    void ComReplicator::UpdateEpochOperation::FinishUpdateEpoch(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto error = this->ReplicatorEngine.EndUpdateEpoch(asyncOperation);
        this->TryComplete(asyncOperation->Parent, error.ToHResult());
    }

    HRESULT ComReplicator::UpdateEpochOperation::End(
        __in IFabricAsyncOperationContext * context)
    {
        if(context == NULL) { return E_POINTER; }
        ComPointer<ComReplicator::UpdateEpochOperation> asyncOperation(
            context, CLSID_ComReplicator_UpdateEpochOperation);
        HRESULT hr = asyncOperation->ComReplicator::BaseOperation::End();
        if (FAILED(hr)) { return hr; }
        
        return asyncOperation->Result;
    }

    } // end namespace ReplicationComponent
} // end namespace Reliability
