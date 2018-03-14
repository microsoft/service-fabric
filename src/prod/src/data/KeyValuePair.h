// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    template<typename TKey, typename TValue>
    class KeyValuePair
    {
    public:
        KeyValuePair<TKey, TValue>();
        KeyValuePair<TKey, TValue>(__in KeyValuePair<TKey, TValue> const & rhs);
        KeyValuePair<TKey, TValue>(__in TKey const & key, __in TValue const & value);
        ~KeyValuePair<TKey, TValue>();

        __declspec(property(get = get_Key, put = set_Key)) TKey Key;
        TKey get_Key() const
        {
            return key_;
        }

        void set_Key(__in TKey const & key)
        {
            key_ = key;
        }

        __declspec(property(get = get_Value, put = set_Value)) TValue Value;
        TValue get_Value() const
        {
            return value_;
        }

        void set_Value(__in TValue const & value)
        {
            value_ = value;
        }

    private:
        TKey key_;
        TValue value_;
    };

    template<typename TKey, typename TValue>
    KeyValuePair<TKey, TValue>::KeyValuePair(
        __in TKey const & key,
        __in TValue const & value)
        : key_(key),
          value_(value)
    {
    }

    template<typename TKey, typename TValue>
    KeyValuePair<TKey, TValue>::KeyValuePair()
    {
    }

    template<typename TKey, typename TValue>
    KeyValuePair<TKey, TValue>::KeyValuePair(__in KeyValuePair<TKey, TValue> const & rhs)
    {
        key_ = rhs.key_;
        value_ = rhs.value_;
    }

    template<typename TKey, typename TValue>
    KeyValuePair<TKey, TValue>::~KeyValuePair()
    {
    }
}

