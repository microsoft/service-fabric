// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Stores the result following the read of a versioned item from the disk.
    /// </summary>
    /// <typeparam name="TValue">Type of value for the item.</typeparam>
    internal struct StoreComponentReadResult<TValue>
    {
        private readonly TVersionedItem<TValue> versionedItem;
        private readonly TValue userValue;

        public StoreComponentReadResult(TVersionedItem<TValue> versionedItem, TValue value)
        {
            this.versionedItem = versionedItem;
            this.userValue = value;
        }

        public bool HasValue
        {
            get
            {
                return this.VersionedItem != null;
            }
        }

        /// <summary>
        /// Gets the versioned item.
        /// </summary>
        public TVersionedItem<TValue> VersionedItem
        {
            get
            {
                return this.versionedItem;
            }
        }

        /// <summary>
        /// Gets the user tvalue
        /// </summary>ry>
        public TValue UserValue
        {
            get
            {
                return this.userValue;
            }
        }
    }
}