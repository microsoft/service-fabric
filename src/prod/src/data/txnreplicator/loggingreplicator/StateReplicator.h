// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class StateReplicator final
            : public IStateReplicator
            , public KObject<StateReplicator>
            , public KShared<StateReplicator>
        {
            K_FORCE_SHARED(StateReplicator)
            K_SHARED_INTERFACE_IMP(IStateReplicator)

        public:

            static StateReplicator::SPtr Create(
                __in IStateReplicator & fabricReplicator,
                __in KAllocator & allocator);

            ktl::Awaitable<LONG64> ReplicateAsync(
                __in Utilities::OperationData const & replicationData,
                __out LONG64 & logicalSequenceNumber) override;

            Data::LoggingReplicator::IOperationStream::SPtr GetCopyStream() override;

            Data::LoggingReplicator::IOperationStream::SPtr GetReplicationStream() override;

        private:

            StateReplicator(__in IStateReplicator & fabricReplicator);

            IStateReplicator::SPtr fabricReplicator_;
        };
    }
}
