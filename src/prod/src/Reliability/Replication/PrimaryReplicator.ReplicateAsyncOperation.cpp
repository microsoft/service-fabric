// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

    using Common::AsyncCallback;
    using Common::AsyncOperation;
    using Common::AsyncOperationSPtr;
    using Common::ComPointer;
    using Common::ErrorCode;

    using std::move;

    PrimaryReplicator::ReplicateAsyncOperation::ReplicateAsyncOperation(
        __in PrimaryReplicator & parent,
        ComPointer<IFabricOperationData> && comOperationPointer,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & state)
        :   AsyncOperation(callback, state),
            parent_(parent),
            comOperationPointer_(move(comOperationPointer)),
            sequenceNumber_(Constants::InvalidLSN)
    {
    }

    void PrimaryReplicator::ReplicateAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (parent_.CheckReportedFault())
        {
            TryComplete(thisSPtr, Common::ErrorCodeValue::ObjectClosed);
            return;
        }

        bool operationCommitted;
        ComOperationCPtr comOperation;

        ErrorCode error = parent_.replicaManagerSPtr_->AddReplicateOperation(
            thisSPtr,
            move(comOperationPointer_),
            operationCommitted,
            comOperation);

        if (error.IsSuccess())
        {
            sequenceNumber_ = comOperation->SequenceNumber;
            if (operationCommitted)
            {
                parent_.UpdateCatchupOperation();
            }
        }
        else
        {
            TryComplete(thisSPtr, error);
        }
    }

    ErrorCode PrimaryReplicator::ReplicateAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto casted = AsyncOperation::End<ReplicateAsyncOperation>(asyncOperation);
        return casted->Error;
    }
    
} // end namespace ReplicationComponent
} // end namespace Reliability
