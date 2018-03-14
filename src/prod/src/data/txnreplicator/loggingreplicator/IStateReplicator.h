// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        // Interface to talk to the V1 Replicator
        interface IStateReplicator
        {
            K_SHARED_INTERFACE(IStateReplicator)

        public:

            // Replicates a log record operation data represented by the replicationData parameter.
            // Return value of null indicates not enough resources to allocate
            virtual  TxnReplicator::CompletionTask::SPtr ReplicateAsync(
                __in Utilities::OperationData const & replicationData,
                __out LONG64 & logicalSequenceNumber) = 0;

            virtual NTSTATUS GetCopyStream(__out IOperationStream::SPtr & result) = 0;
            
            virtual NTSTATUS GetReplicationStream(__out IOperationStream::SPtr & result) = 0;

            virtual int64 GetMaxReplicationMessageSize() = 0;
        };
    }
}
