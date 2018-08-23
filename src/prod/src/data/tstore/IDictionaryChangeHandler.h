// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        namespace DictionaryChangeEventMask
        {
            // Don't change these value - they need to match exactly with C API layer
            enum Enum : uint32_t 
            {
                None = 0x0,
                Add = 0x1,
                Update = 0x2,
                Remove = 0x4,
                Clear = 0x8,
                Rebuild = 0x10,
                All = 0xffffffff
            };
        }

        template<typename TKey, typename TValue>
        interface IDictionaryChangeHandler
        {
            K_SHARED_INTERFACE(IDictionaryChangeHandler)
        public:
            virtual ktl::Awaitable<void> OnAddedAsync(
                __in TxnReplicator::TransactionBase const& replicatorTransaction,
                __in TKey key,
                __in TValue value,
                __in LONG64 sequenceNumber, 
                __in bool isPrimary) noexcept = 0;
            
            virtual ktl::Awaitable<void> OnUpdatedAsync(
                __in TxnReplicator::TransactionBase const& replicatorTransaction,
                __in TKey key,
                __in TValue value,
                __in LONG64 sequenceNumber,
                __in bool isPrimary) noexcept = 0;

            virtual ktl::Awaitable<void> OnRemovedAsync(
                __in TxnReplicator::TransactionBase const& replicatorTransaction,
                __in TKey key,
                __in LONG64 sequenceNumber,
                __in bool isPrimary) noexcept = 0;

            virtual ktl::Awaitable<void> OnRebuiltAsync(__in Utilities::IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>> & enumerableState) noexcept = 0;
        };
    }
}
