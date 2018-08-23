// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Collections
    {
        template<typename TValue>
        interface IReliableConcurrentQueue
        {
            K_SHARED_INTERFACE(IReliableConcurrentQueue)

        public:
            virtual ktl::Awaitable<void> EnqueueAsync(
                __in TxnReplicator::TransactionBase& replicatorTransaction,
                __in TValue value,
                __in Common::TimeSpan timeout,
                __in ktl::CancellationToken const & cancellationToken) = 0;

            virtual ktl::Awaitable<bool> TryDequeueAsync(
                __in TxnReplicator::TransactionBase& replicatorTransaction,
                __out TValue& value,
                __in Common::TimeSpan timeout,
                __in ktl::CancellationToken const & cancellationToken) = 0;

            // Call it GetQueueCount to avoid conflicting with IStore (trigger ambiguity failure everywhere)
            virtual LONG64 GetQueueCount() const = 0;
        };
    }
}
