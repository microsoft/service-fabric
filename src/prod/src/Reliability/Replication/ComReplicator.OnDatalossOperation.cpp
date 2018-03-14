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

    ComReplicator::OnDataLossOperation::OnDataLossOperation(
        Replicator & replicator) 
        : ComReplicator::BaseOperation(replicator),
          isStateChanged_(false)
    {
    }

    HRESULT ComReplicator::OnDataLossOperation::End(
        __in IFabricAsyncOperationContext * context,
        __out BOOLEAN * isStateChanged)
    {
        if (context == NULL) { return E_POINTER; }
        ComPointer<OnDataLossOperation> asyncOperation(
            context, CLSID_ComReplicator_OnDataLossOperation);
        HRESULT hr = asyncOperation->ComReplicator::BaseOperation::End();
        if (FAILED(hr)) { return hr; }

        if (isStateChanged != NULL) 
        { 
            *isStateChanged = asyncOperation->isStateChanged_; 
        }

        return asyncOperation->Result;
    }

    void ComReplicator::OnDataLossOperation::OnStart(
        Common::AsyncOperationSPtr const & proxySPtr)
    {
        auto inner = this->ReplicatorEngine.BeginOnDataLoss(
            [this](AsyncOperationSPtr const & asyncOperation) { this->OnDataLossCallback(asyncOperation); },
            proxySPtr);

        if (inner->CompletedSynchronously)
        {
            this->FinishOnDataLoss(inner);
        }
    }

    void ComReplicator::OnDataLossOperation::OnDataLossCallback(
        AsyncOperationSPtr const & asyncOperation)
    {
        if (!asyncOperation->CompletedSynchronously)
        {
            this->FinishOnDataLoss(asyncOperation);
        }
    }

    void ComReplicator::OnDataLossOperation::FinishOnDataLoss(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto error = this->ReplicatorEngine.EndOnDataLoss(asyncOperation, isStateChanged_);
        this->TryComplete(asyncOperation->Parent, error.ToHResult());
    }

    } // end namespace ReplicationComponent
} // end namespace Reliability
