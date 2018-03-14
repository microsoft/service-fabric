// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        template <typename TKey, typename TValue>
        class IPartition
        {
        public:
            virtual const TKey& GetFirstKey() const = 0;
            virtual int Count() = 0;
            virtual void Add(__in const TKey& key, __in const TValue& value) = 0;
            virtual const TKey& GetKey(__in int index) const = 0;
            virtual const TValue& GetValue(__in int index) const = 0;
            virtual void UpdateValue(__in int index, __in const TValue& value) = 0;
            virtual int BinarySearch(__in const TKey& key, __in const IComparer<TKey>& comparer) = 0;
        };
    }
}
