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

    ComReplicator::ReplicateOperation::ReplicateOperation(
        Replicator & replicator, 
        ComPointer<IFabricOperationData> && comOperationPointer) 
        : ComReplicator::BaseOperation(replicator),
          comOperationPointer_(std::move(comOperationPointer)),
          sequenceNumber_(Constants::InvalidLSN)
    {
    }

    HRESULT ComReplicator::ReplicateOperation::End(
        __in IFabricAsyncOperationContext * context,
        __out FABRIC_SEQUENCE_NUMBER * sequenceNumber)
    {
        if(context == NULL) { return E_POINTER; }

        ComPointer<ReplicateOperation> asyncOperation(
            context, CLSID_ComReplicator_ReplicateOperation);

        HRESULT hr = asyncOperation->ComReplicator::BaseOperation::End();

        if (FAILED(hr)) { return hr; }

        if (sequenceNumber != nullptr)
        {
            *sequenceNumber = asyncOperation->SequenceNumber;
        }

        return asyncOperation->Result;
    }

    void ComReplicator::ReplicateOperation::OnStart(
        Common::AsyncOperationSPtr const & proxySPtr)
    {
        auto inner = this->ReplicatorEngine.BeginReplicate(
            std::move(this->comOperationPointer_),
            sequenceNumber_,
            [this](AsyncOperationSPtr const & asyncOperation) { this->ReplicateCallback(asyncOperation); },
            proxySPtr);
                        
        if (inner->CompletedSynchronously)
        {
            this->FinishReplicate(inner);
        }
    }

    void ComReplicator::ReplicateOperation::ReplicateCallback(
        AsyncOperationSPtr const & asyncOperation)
    {
        if (!asyncOperation->CompletedSynchronously)
        {
            this->FinishReplicate(asyncOperation);
        }
    }

    void ComReplicator::ReplicateOperation::FinishReplicate(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto error = this->ReplicatorEngine.EndReplicate(asyncOperation);
        this->TryComplete(asyncOperation->Parent, error.ToHResult());
    }

    } // end namespace ReplicationComponent
} // end namespace Reliability
