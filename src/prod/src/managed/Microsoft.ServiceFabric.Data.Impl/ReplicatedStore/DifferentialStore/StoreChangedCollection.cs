// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System;
    using System.Collections.Generic;

    internal class StoreChangedCollection<TKey, TValue> : EventArgs, INotifyStoreChangeCollectionEventArgs<TKey, TValue>
    {
        /// <summary>
        /// Change list.
        /// </summary>
        private IEnumerable<INotifyStoreChangedEvent> changes;

        /// <summary>
        /// Proper constructor.
        /// </summary>
        /// <param name="changes">Change list.</param>
        public StoreChangedCollection(IEnumerable<INotifyStoreChangedEvent> changes)
        {
            this.changes = changes;
        }

        #region INotifyStoreChangedEventArgs<TKey, TValue> Members

        public StoreChangedEventType StoreChangeType
        {
            get
            {
                return StoreChangedEventType.Changed;
            }
        }

        public StoreChangeEventType ChangeType
        {
            get
            {
                return StoreChangeEventType.Store;
            }
        }
        
        #endregion

        #region INotifyStoreChangeCollectionEventArgs<TKey, TValue> Members

        /// <summary>
        /// Gets the change list.
        /// </summary>
        public IEnumerable<INotifyStoreChangedEvent> ChangeList
        {
            get 
            {
                return this.changes;
            }
        }

        #endregion
    }
}