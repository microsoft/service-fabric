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
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Holds snapshot components that are needed for snapshot reads.
    /// </summary>
    /// <typeparam name="TKey">Key type.</typeparam>
    /// <typeparam name="TValue">Value type.</typeparam>
    /// <typeparam name="TKeyComparer">Key comparer type.</typeparam>
    /// <typeparam name="TKeyEqualityComparer">Key equality comparer type.</typeparam>
    internal class SnapshotContainer<TKey, TValue, TKeyComparer, TKeyEqualityComparer> : IDisposable
        where TKeyComparer : IComparer<TKey>, new()
        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
    {
        #region Instance Members

        private readonly string traceType;

        /// <summary>
        /// Would contain version that cannot be removed due to ongoing snapshot reads.
        /// </summary>
        /// <devote>Key is lsn and it uses default comparer.</devote>
        private ConcurrentDictionary<long, SnapshotComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer>> container =
            new ConcurrentDictionary<long, SnapshotComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer>>();

        /// <summary>
        /// Contains the snapshot of the metadata table used to service snapshot reads.
        /// </summary>
        private ConcurrentDictionary<long, ConcurrentDictionary<uint, FileMetadata>> fileMetadataTableLsnMap =
            new ConcurrentDictionary<long, ConcurrentDictionary<uint, FileMetadata>>();

        // Lock to avoid race between SnapshotContainer::Remove and FabricDirectory::Delete in TStore::RemoveStateAsync
        internal ReaderWriterLockSlim checkpointFileDeleteLock;

        #endregion

        public SnapshotContainer(IStateSerializer<TValue> valueSerializer, string traceType)
        {
            this.ValueSerializer = valueSerializer;
            this.traceType = traceType;
            this.checkpointFileDeleteLock = new ReaderWriterLockSlim();
        }

        public ConcurrentDictionary<long, ConcurrentDictionary<uint, FileMetadata>> FileMetadataTableLsnMap
        {
            get { return this.fileMetadataTableLsnMap; }
        }

        public IStateSerializer<TValue> ValueSerializer { get; set; }

        /// <summary>
        /// Checks to see if a certain key is present in the version chain.
        /// </summary>
        /// <param name="key">Lookup key.</param>
        /// <returns>True if key already exists in the write set.</returns>
        public bool ContainsKey(long key)
        {
            return this.container.ContainsKey(key);
        }

        /// <summary>
        /// Reads the value for a given key.
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <returns>True and value if the key exists.</returns>
        public SnapshotComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer> Read(long key)
        {
            SnapshotComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer> component;
            if (this.container.TryGetValue(key, out component))
                return component;

            return null;
        }

        /// <summary>
        /// Gets or Adds a snapshot component for a visibility lsn.
        /// </summary>
        public SnapshotComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer> GetOrAdd(
            long key, SnapshotComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer> value)
        {
            var result = this.container.GetOrAdd(key, value);
            return result;
        }

        /// <summary>
        /// Tries to set the metadata table for a visibility lsn.
        /// </summary>
        /// <param name="lsn"></param>
        /// <param name="fileMetadata"></param>
        public bool TryAddFileMetadata(long lsn, FileMetadata fileMetadata)
        {
            var table = this.FileMetadataTableLsnMap.GetOrAdd(lsn, l => new ConcurrentDictionary<uint, FileMetadata>());

            if (table.TryAdd(fileMetadata.FileId, fileMetadata))
            {
                fileMetadata.AddRef();
                return true;
            }

            return false;
        }

        /// <summary>
        /// Removes the given key.
        /// </summary>
        /// <param name="key"></param>
        public void Remove(long key)
        {
            SnapshotComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer> value = null;
            var result = this.container.TryRemove(key, out value);
            Diagnostics.Assert(result, this.traceType, "SnapshotComponent should be removed.");

            // Remove the file metadata table from the lsn map.
            ConcurrentDictionary<uint, FileMetadata> fileMetadataTable = null;
            if (this.FileMetadataTableLsnMap.TryRemove(key, out fileMetadataTable))
            {
                // Acquire reader lock before trying to remove checkpoint file
                checkpointFileDeleteLock.EnterReadLock();

                try
                {
                    // Decrement ref on each file metadata.
                    foreach (var item in fileMetadataTable)
                    {
                        var metadata = item.Value;
                        metadata.ReleaseRef();
                    }
                }
                finally
                {
                    checkpointFileDeleteLock.ExitReadLock();
                }                
            }
        }

        /// <summary>
        /// Test only function for getting the number of unique inflight visibility sequence numbers.
        /// </summary>
        /// <returns>The count.</returns>
        public int GetCount()
        {
            return this.container.Count;
        }

        public void Dispose()
        {
            if (this.FileMetadataTableLsnMap != null)
            {
                foreach (var lsnMap in this.FileMetadataTableLsnMap)
                {
                    foreach (var item in lsnMap.Value)
                    {
                        item.Value.Dispose();
                    }
                }

                this.FileMetadataTableLsnMap.Clear();
            }
        }
    }
}