// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    template <typename TKey, typename TValue>
    class IDictionary : public KObject<IDictionary<TKey, TValue>>, public KShared<IDictionary<TKey, TValue>>
    {
        K_FORCE_SHARED_WITH_INHERITANCE(IDictionary)

    public:
        __declspec(property(get = get_Count)) ULONG Count;
        virtual ULONG get_Count() const = 0;

        virtual void Add(__in const TKey&, __in const TValue&) = 0;
        virtual void Clear() = 0;
        virtual bool ContainsKey(__in const TKey&) const = 0;
        virtual KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> GetEnumerator() const = 0;
        virtual bool Remove(__in const TKey&) = 0;
        virtual bool TryGetValue(__in const TKey&, __out TValue& value) const = 0;
    };

    template <typename TKey, typename TValue>
    IDictionary<TKey, TValue>::IDictionary()
    {
    }

    template <typename TKey, typename TValue>
    IDictionary<TKey, TValue>::~IDictionary()
    {
    }
}
