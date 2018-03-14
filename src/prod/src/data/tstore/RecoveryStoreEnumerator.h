// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define RECOVERYSTOREENUM_TAG 'mnER'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class RecoveryStoreEnumerator : public IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>
        {
            K_FORCE_SHARED(RecoveryStoreEnumerator)

        public:
            static NTSTATUS Create(
                __in KSharedPtr<KSharedArray<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> & recoveryStoreComponent,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                SPtr output = _new(RECOVERYSTOREENUM_TAG, allocator) RecoveryStoreEnumerator(recoveryStoreComponent);

                if (!output)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    return status;
                }

                status = output->Status();
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                result = Ktl::Move(output);
                return STATUS_SUCCESS;
            }

        public:
            KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> Current()
            {
                ASSERT_IFNOT(currentIndex_ >= 0, "current index must be non-negative");
                return (*componentSPtr_)[currentIndex_];
            }

            bool MoveNext()
            {
                currentIndex_++;
                ASSERT_IFNOT(currentIndex_ >= 0, "current index must be non-negative");
                auto index = static_cast<ULONG>(currentIndex_);
                return index < componentSPtr_->Count();
            }

        private:
            LONG32 currentIndex_;
            KSharedPtr<KSharedArray<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> componentSPtr_;
            NOFAIL RecoveryStoreEnumerator(__in KSharedPtr<KSharedArray<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> & recoveryStoreComponent);
        };

        template <typename TKey, typename TValue>
        RecoveryStoreEnumerator<TKey, TValue>::RecoveryStoreEnumerator(__in KSharedPtr<KSharedArray<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> & recoveryStoreComponent)
            : componentSPtr_(recoveryStoreComponent)
        {
            currentIndex_ = -1;
        }

        template <typename TKey, typename TValue>
        RecoveryStoreEnumerator<TKey, TValue>::~RecoveryStoreEnumerator()
        {
        }
    }
}
