// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    internal struct KeyData<TKey, TValue>
    {
        private readonly TKey key;
        private readonly TVersionedItem<TValue> versionedItem;
        private readonly long timeStamp;

        public KeyData(TKey key, TVersionedItem<TValue> versionedItem, long timeStamp)
        {
            this.key = key;
            this.versionedItem = versionedItem;
            this.timeStamp = timeStamp;
        }

        public TKey Key
        {
            get { return this.key; }
        }

        public TVersionedItem<TValue> Value
        {
            get { return this.versionedItem; }
        }

        public long TimeStamp
        {
            get { return this.timeStamp; }
        }
    }
}