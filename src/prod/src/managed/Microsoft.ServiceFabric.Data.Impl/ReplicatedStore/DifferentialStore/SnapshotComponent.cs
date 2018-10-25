// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Data.DataStructures.Concurrent;

    /// <summary>
    /// Holds versions needed for snapshot reads. It is an aggregated store component.
    /// </summary>
    /// <typeparam name="TKey">Key type.</typeparam>
    /// <typeparam name="TValue">Value type.</typeparam>
    /// <typeparam name="TKeyComparer">Key comparer type.</typeparam>
    /// <typeparam name="TKeyEqualityComparer">Key equality comparer type.</typeparam>
    internal class SnapshotComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer>
        where TKeyComparer : IComparer<TKey>, new()
        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
    {
        private static TKeyComparer KeyComparer = new TKeyComparer();
        private readonly bool isValueAReferenceType;
        private readonly LoadValueCounter valueCounter;

        #region Instance Members

        private ConcurrentSkipList<TKey, TVersionedItem<TValue>> component = new ConcurrentSkipList<TKey, TVersionedItem<TValue>>(KeyComparer);
        private SnapshotContainer<TKey, TValue, TKeyComparer, TKeyEqualityComparer> container;

        private readonly string traceType;
        #endregion

        /// <summary>
        /// Creates a snapshot component to snapshot reads for a given visibility lsn.
        /// </summary>
        public SnapshotComponent(SnapshotContainer<TKey, TValue, TKeyComparer, TKeyEqualityComparer> container, LoadValueCounter valueCounter, bool isValueAReferenceType, string traceType)
        {
            this.container = container;
            this.isValueAReferenceType = isValueAReferenceType;
            this.valueCounter = valueCounter;
            this.traceType = traceType;
        }

        /// <summary>
        /// Adds a new entry to this store component.
        /// </summary>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        public void Add(TKey key, TVersionedItem<TValue> value)
        {
            // When items are moved to snapshot component, there can be cases where an lsn comes from the differential and subsequently a
            // smaller lsn comes from the consolidated component, when this happens the smaller value should not replace the larger lsn.
            bool isAdded = this.component.TryAdd(key, value);
            if (isAdded == false)
            {
                this.component.Update(
                    key,
                    (currentKey, currentValue) => { return (currentValue.VersionSequenceNumber <= value.VersionSequenceNumber) ? value : currentValue; });
            }
        }

        /// <summary>
        /// Reads the value for a given key.
        /// </summary>
        /// <param name="key"></param>
        /// <param name="visibilityLsn"></param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <returns></returns>
        public async Task<StoreComponentReadResult<TValue>> ReadAsync(TKey key, long visibilityLsn, ReadMode readMode)
        {
            TVersionedItem<TValue> versionedItem;
            var result = this.component.TryGetValue(key, out versionedItem);
            
            if (result)
            {
                Diagnostics.Assert(versionedItem != null, this.traceType, "Versioned item is null.");

                TValue value = versionedItem.Value;
                if (versionedItem.ShouldValueBeLoadedFromDisk(this.isValueAReferenceType, value, this.traceType))
                {
                    // Get file metadata table.
                    ConcurrentDictionary<uint, FileMetadata> fileMetadataTable = null;
                    this.container.FileMetadataTableLsnMap.TryGetValue(visibilityLsn, out fileMetadataTable);

                    Diagnostics.Assert(fileMetadataTable != null, this.traceType, "File metadata table is null.");
                    Diagnostics.Assert(versionedItem.FileId > 0, this.traceType, "FileId must be positive");

                    // Get file metadata.
                    FileMetadata fileMetadata = null;
                    var foundMetadata = fileMetadataTable.TryGetValue(versionedItem.FileId, out fileMetadata);
                    Diagnostics.Assert(foundMetadata, this.traceType, "Failed to find file id");

                    if (versionedItem.Kind != RecordKind.DeletedVersion && readMode == ReadMode.CacheResult)
                    {
                        // If the TValue is a reference type, TrySet the inUse flag to 1.
                        versionedItem.SetInUseToTrue(this.isValueAReferenceType);
                    }

                    // Load the value into memory.
                    value = await versionedItem.GetValueAsync(fileMetadata, this.container.ValueSerializer, this.isValueAReferenceType, readMode, this.valueCounter, CancellationToken.None, this.traceType).ConfigureAwait(false);
                }

                if (versionedItem.VersionSequenceNumber <= visibilityLsn)
                {
                    return new StoreComponentReadResult<TValue>(versionedItem, value);
                }
            }

            return new StoreComponentReadResult<TValue>(null, default(TValue));
        }

        /// <summary>
        /// Returns an enumerable for this collection.
        /// </summary>
        /// <returns>An enumerable that can be used to iterate through the collection.</returns>
        public IEnumerable<TKey> GetEnumerable()
        {
            return this.component;
        }
    }
}