// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        // Represents a stream of replication or copy operations that are sent from the Primary to the Secondary replica.
        // The streams returned from IStateReplicator.GetCopyStream and IStateReplicator.GetReplicationStream 
        // are objects that implement IOperationStream
        interface IOperationStream
        {
            K_SHARED_INTERFACE(IOperationStream)

        public:
            // Gets the next object that implements IOperation from the underlying IOperationStream
            virtual ktl::Awaitable<NTSTATUS> GetOperationAsync(__out IOperation::SPtr & result) = 0;
        };
    }
}
