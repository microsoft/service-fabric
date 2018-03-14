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

    ComReplicator::OpenOperation::OpenOperation(
        __in Replicator & replicator) 
        :   ComReplicator::BaseOperation(replicator),
            replicationEndpoint_()
    {
    }

    HRESULT ComReplicator::OpenOperation::End(
        __in IFabricAsyncOperationContext * context, 
        __out IFabricStringResult **replicationEndpoint)
    {
        if (replicationEndpoint == NULL) { return E_POINTER; }
        *replicationEndpoint = NULL;

        ComPointer<ComReplicator::OpenOperation> asyncOperation(
            context, CLSID_ComReplicator_OpenOperation);

        HRESULT hr = asyncOperation->ComReplicator::BaseOperation::End();
        if (FAILED(hr)) { return hr; }
    
        ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult, IFabricStringResult>(asyncOperation->replicationEndpoint_);
        *replicationEndpoint = stringResult.DetachNoRelease();
        return S_OK;
    }

    void ComReplicator::OpenOperation::OnStart(
        Common::AsyncOperationSPtr const & proxySPtr)
    {
        auto inner = this->ReplicatorEngine.BeginOpen(
            [this](AsyncOperationSPtr const & asyncOperation) { this->OpenCallback(asyncOperation); },
            proxySPtr);

        if (inner->CompletedSynchronously)
        {
            this->FinishOpen(inner);
        }
    }

    void ComReplicator::OpenOperation::OpenCallback(
        AsyncOperationSPtr const & asyncOperation)
    {
        if (!asyncOperation->CompletedSynchronously)
        {
            this->FinishOpen(asyncOperation);
        }
    }

    void ComReplicator::OpenOperation::FinishOpen(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto error = this->ReplicatorEngine.EndOpen(asyncOperation, replicationEndpoint_);
        this->TryComplete(asyncOperation->Parent, error.ToHResult());
    }

    } // end namespace ReplicationComponent
} // end namespace Reliability
