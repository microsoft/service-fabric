// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class PartitionSearchKey : public IPartition<TKey, TValue>
        {
        public:
            const TKey& GetFirstKey() const
            {
                return creationValue_;
            }

            int Count()
            {
                throw ktl::Exception(K_STATUS_OBJECT_NOT_ATTACHED); //TODO: Pick the right status for NotImplementedException
            }

            void Add(__in const TKey&, __in const TValue&)
            {
                throw ktl::Exception(K_STATUS_OBJECT_NOT_ATTACHED); //TODO: Pick the right status for NotImplementedException
            }

            const TKey& GetKey(__in int) const
            {
                throw ktl::Exception(K_STATUS_OBJECT_NOT_ATTACHED); //TODO: Pick the right status for NotImplementedException
            }

            const TValue& GetValue(__in int) const
            {
                throw ktl::Exception(K_STATUS_OBJECT_NOT_ATTACHED); //TODO: Pick the right status for NotImplementedException
            }

            void UpdateValue(__in int, __in const TValue&)
            {
                throw ktl::Exception(K_STATUS_OBJECT_NOT_ATTACHED); //TODO: Pick the right status for NotImplementedException
            }

            int BinarySearch(__in const TKey&, __in const IComparer<TKey>&)
            {
                throw ktl::Exception(K_STATUS_OBJECT_NOT_ATTACHED); //TODO: Pick the right status for NotImplementedException
            }

            PartitionSearchKey<TKey, TValue>(__in TKey key);
            ~PartitionSearchKey<TKey, TValue>();

        private:
            TKey creationValue_;
        };

        template<typename TKey, typename TValue>
        PartitionSearchKey<TKey, TValue>::PartitionSearchKey(__in TKey key):
            creationValue_(key)
        {
        }

        template<typename TKey, typename TValue>
        PartitionSearchKey<TKey, TValue>::~PartitionSearchKey()
        {
        }
    }
}

