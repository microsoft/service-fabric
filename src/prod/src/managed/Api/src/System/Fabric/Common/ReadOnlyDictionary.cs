// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Fabric.Strings;

    internal class ReadOnlyDictionary<TKey, TValue> : IDictionary<TKey, TValue>
    {
        private readonly Dictionary<TKey, TValue> dictionary;

        public ReadOnlyDictionary(Dictionary<TKey, TValue> dictionary)
        {
            this.dictionary = dictionary;
        }

        public ICollection<TKey> Keys
        {
            get
            {
                return this.dictionary.Keys;
            }
        }

        public ICollection<TValue> Values
        {
            get
            {
                return this.dictionary.Values;
            }
        }

        public int Count
        {
            get
            {
                return this.dictionary.Count;
            }
        }

        public bool IsReadOnly
        {
            get
            {
                return true;
            }
        }

        public TValue this[TKey key]
        {
            get
            {
                return this.dictionary[key];
            }

            set
            {
                throw new InvalidOperationException(StringResources.Error_ReadOnlyDictionary);
            }
        }

        public void Add(TKey key, TValue value)
        {
            throw new InvalidOperationException(StringResources.Error_ReadOnlyDictionary);
        }

        public bool ContainsKey(TKey key)
        {
            return this.dictionary.ContainsKey(key);
        }

        public bool Remove(TKey key)
        {
            throw new InvalidOperationException(StringResources.Error_ReadOnlyDictionary);
        }

        public bool TryGetValue(TKey key, out TValue value)
        {
            return this.dictionary.TryGetValue(key, out value);
        }

        public void Add(KeyValuePair<TKey, TValue> item)
        {
            throw new InvalidOperationException(StringResources.Error_ReadOnlyDictionary);
        }

        public void Clear()
        {
            throw new InvalidOperationException(StringResources.Error_ReadOnlyDictionary);
        }

        public bool Contains(KeyValuePair<TKey, TValue> item)
        {
            return ((ICollection<KeyValuePair<TKey, TValue>>)this.dictionary).Contains(item);
        }

        public void CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex)
        {
            ((ICollection)this.dictionary).CopyTo(array, arrayIndex);
        }

        public bool Remove(KeyValuePair<TKey, TValue> item)
        {
            throw new InvalidOperationException(StringResources.Error_ReadOnlyDictionary);
        }

        public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
        {
            return this.dictionary.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.dictionary.GetEnumerator();
        }
    }
}