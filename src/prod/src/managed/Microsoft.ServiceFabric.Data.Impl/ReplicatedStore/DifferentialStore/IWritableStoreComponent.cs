// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Encapsulates a writable store component functionality.
    /// </summary>
    internal interface IWritableStoreComponent<TKey, TValue>
    {
        /// <summary>
        /// Adds a new entry to this store component.
        /// </summary>
        /// <param name="isIdempotent">Specifies if the add operation is idempotent.</param>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        void Add(bool isIdempotent, TKey key, TValue value);

        /// <summary>
        /// Removes all items from the store component.
        /// </summary>
        void Clear();
    }
}