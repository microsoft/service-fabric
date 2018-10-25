// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;

    class KeyedItemCollection<TKey, TItem> : KeyedCollection<TKey, TItem>
    {
        private Func<TItem, TKey> getKey;

        protected internal KeyedItemCollection(Func<TItem, TKey> getKeyCallback)
            : base()
        {
            this.getKey = getKeyCallback;
        }

        protected internal KeyedItemCollection(Func<TItem, TKey> getKeyCallback, IEqualityComparer<TKey> comparer)
            : base(comparer)
        {

            this.getKey = getKeyCallback;
        }

        protected internal KeyedItemCollection(Func<TItem, TKey> getKeyCallback, IEqualityComparer<TKey> comparer, int dictionaryCreationThreshold)
            : base(comparer, dictionaryCreationThreshold)
        {
            this.getKey = getKeyCallback;
        }

        protected override TKey GetKeyForItem(TItem item)
        {
            return this.getKey(item);
        }
    }
}