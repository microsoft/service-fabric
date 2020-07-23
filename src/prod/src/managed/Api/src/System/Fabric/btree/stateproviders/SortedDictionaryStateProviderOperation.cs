// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.StateProviders
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using System.Fabric.Data.LockManager;
    using System.Fabric.Data.TransactionManager;

    /// <summary>
    /// Base operation for sorted dictionary.
    /// </summary>
    class SortedDictionaryStateProviderOperation : StateProviderOperation
    {
        /// <summary>
        /// Read/Write transaction this operation is part of.
        /// </summary>
        public IReadWriteTransaction Tx;
    }

    /// <summary>
    /// Add operation.
    /// </summary>
    class SortedDictionaryStateProviderAddOperation<K, V> : SortedDictionaryStateProviderOperation
    {
        /// <summary>
        /// Key in the operation.
        /// </summary>
        public K Key;

        /// <summary>
        /// Value in the operation.
        /// </summary>
        public V Value;
    }

    /// <summary>
    /// Remove operation.
    /// </summary>
    class SortedDictionaryStateProviderRemoveOperation<K, V> : SortedDictionaryStateProviderOperation
    {
        /// <summary>
        /// Key in the operation.
        /// </summary>
        public K Key;

        /// <summary>
        /// Value in the operation.
        /// </summary>
        public V Value;
    }

    /// <summary>
    /// Clear operation.
    /// </summary>
    class SortedDictionaryStateProviderClearOperation : SortedDictionaryStateProviderOperation
    {
    }

    /// <summary>
    /// Update operation.
    /// </summary>
    class SortedDictionaryStateProviderUpdateOperation<K, V> : SortedDictionaryStateProviderOperation
    {
        /// <summary>
        /// Key in the operation.
        /// </summary>
        public K Key;

        /// <summary>
        /// Old value in the operation.
        /// </summary>
        public V OldValue;

        /// <summary>
        /// New value in the operation.
        /// </summary>
        public V NewValue;
    }
}