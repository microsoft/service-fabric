// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define KEY_VERSIONEDITEM_COMPARER_TAG 'ciVK'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class KeyVersionedItemComparer
            : public IComparer<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>
            , public KObject<KeyVersionedItemComparer<TKey, TValue>>
            , public KShared<KeyVersionedItemComparer<TKey, TValue>>
        {
            K_FORCE_SHARED(KeyVersionedItemComparer)
            K_SHARED_INTERFACE_IMP(IComparer)
        public:
            static NTSTATUS Create(
                __in IComparer<TKey> & keyComparer,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                result = _new(KEY_VERSIONEDITEM_COMPARER_TAG, allocator) KeyVersionedItemComparer<TKey, TValue>(keyComparer);
                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }

            int Compare(__in const KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> & x, __in const KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> & y) const override
            {
                int keyComparison = keyComparerSPtr_->Compare(x.Key, y.Key);
                
                if (keyComparison == 0)
                {
                    // Bigger LSNs should be sorted earlier (consistent with KeyCheckpointFileAsyncEnumerator compare)
                    LONG64 valueComparison = x.Value->GetVersionSequenceNumber() - y.Value->GetVersionSequenceNumber();
                    if (valueComparison > 0)
                    {
                        return -1;
                    }

                    if (valueComparison < 0)
                    {
                        return 1;
                    }

                    return 0;
                }
                else if (keyComparison < 0)
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }

        private:
            KeyVersionedItemComparer(__in IComparer<TKey> & keyComparer);

            KSharedPtr<IComparer<TKey>> keyComparerSPtr_;
        };

        template<typename TKey, typename TValue>
        KeyVersionedItemComparer<TKey, TValue>::KeyVersionedItemComparer(__in IComparer<TKey> & keyComparer)
        {
            keyComparerSPtr_ = &keyComparer;
        }

        template<typename TKey, typename TValue>
        KeyVersionedItemComparer<TKey, TValue>::~KeyVersionedItemComparer()
        {
        }
    }
}