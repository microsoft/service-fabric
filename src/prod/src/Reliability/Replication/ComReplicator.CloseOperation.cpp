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

    ComReplicator::CloseOperation::CloseOperation(Replicator & replicator) 
        : ComReplicator::BaseOperation(replicator)
    {
    }

    HRESULT ComReplicator::CloseOperation::End(
        __in IFabricAsyncOperationContext * context)
    {
        if(context == NULL) { return E_POINTER; }
        ComPointer<ComReplicator::CloseOperation> asyncOperation(
            context, CLSID_ComReplicator_CloseOperation);
        HRESULT hr = asyncOperation->ComReplicator::BaseOperation::End();
        if (FAILED(hr)) { return hr; }

        return asyncOperation->Result;
    }

    void ComReplicator::CloseOperation::OnStart(
        Common::AsyncOperationSPtr const & proxySPtr)
    {
        auto inner = this->ReplicatorEngine.BeginClose(
            [this](AsyncOperationSPtr const & asyncOperation) { this->CloseCallback(asyncOperation); },
            proxySPtr);

        if (inner->CompletedSynchronously)
        {
            this->FinishClose(inner);
        }
    }

    void ComReplicator::CloseOperation::CloseCallback(
        AsyncOperationSPtr const & asyncOperation)
    {
        if (!asyncOperation->CompletedSynchronously)
        {
            this->FinishClose(asyncOperation);
        }
    }

    void ComReplicator::CloseOperation::FinishClose(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto error = this->ReplicatorEngine.EndClose(asyncOperation);
        this->TryComplete(asyncOperation->Parent, error.ToHResult());
    }

    } // end namespace ReplicationComponent
} // end namespace Reliability
