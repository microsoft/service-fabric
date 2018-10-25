// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Data;

    internal sealed class AsyncEnumerator<TKey, TValue> : IAsyncEnumerator<KeyValuePair<TKey, TValue>>
    {
        private readonly IEnumerable<TKey> keyEnumerables;
        private readonly IReadableStoreComponent<TKey, TVersionedItem<TValue>> differentState;
        private readonly IReadableStoreComponent<TKey, TVersionedItem<TValue>> consolidatedState;
        private readonly MetadataTable currentMetadataTable;
        private readonly IStateSerializer<TValue> valueSerializer;
        private readonly bool isValueAReferenceType;
        private readonly LoadValueCounter valueCounter;

        private readonly ReadMode readMode;

        private IEnumerator<TKey> keyEnumerator;
        private KeyValuePair<TKey, TValue> currentItem;
        private readonly string traceType;

        public AsyncEnumerator(
            bool isValueAReferenceType,
            ReadMode readMode,
            IEnumerable<TKey> keyEnumerables, 
            IReadableStoreComponent<TKey, TVersionedItem<TValue>> differentState, 
            IReadableStoreComponent<TKey, TVersionedItem<TValue>> consolidatedState,
            MetadataTable currentMetadataTable,
            LoadValueCounter valueCounter,
            IStateSerializer<TValue> valueSerializer,
            string traceType)
        {
            this.traceType = traceType;
            this.isValueAReferenceType = isValueAReferenceType;
            this.readMode = readMode;
            this.keyEnumerables = keyEnumerables;
            this.keyEnumerator = keyEnumerables.GetEnumerator();
            this.differentState = differentState;
            this.consolidatedState = consolidatedState;
            this.currentMetadataTable = currentMetadataTable;
            this.valueCounter = valueCounter;
            this.valueSerializer = valueSerializer;

            this.currentItem = new KeyValuePair<TKey, TValue>();
        }

        public KeyValuePair<TKey, TValue> Current
        {
            get { return this.currentItem; }
        }

        public async Task<bool> MoveNextAsync(CancellationToken cancellationToken)
        {
            while (this.keyEnumerator.MoveNext())
            {
                TKey currentKey = this.keyEnumerator.Current;

                var versionedItem = this.differentState.Read(currentKey);
                if (versionedItem == null)
                {
                    versionedItem = this.consolidatedState.Read(currentKey);
                }

                if (versionedItem.Kind == RecordKind.DeletedVersion)
                {
                    continue;
                }

                TValue value = versionedItem.Value;

                if (versionedItem.ShouldValueBeLoadedFromDisk(this.isValueAReferenceType, value, this.traceType))
                {
                    // Current metadata table is sufficient since this is only called during restore, recovery and copy
                    var pagedValue = await versionedItem.GetValueAsync(this.currentMetadataTable, this.valueSerializer, this.isValueAReferenceType, this.readMode, this.valueCounter, CancellationToken.None, this.traceType).ConfigureAwait(false);
                    this.currentItem = new KeyValuePair<TKey, TValue>(currentKey, pagedValue);
                    return true;
                }
                else
                {
                    this.currentItem = new KeyValuePair<TKey, TValue>(currentKey, value);
                    return true;
                }
            }

            return false;
        }

        public void Reset()
        {
            Diagnostics.Assert(this.keyEnumerator != null, this.traceType, "enumerator can never be null");
            this.keyEnumerator.Dispose();
            this.keyEnumerator = this.keyEnumerables.GetEnumerator();
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool isDisposing)
        {
            if (isDisposing == false)
            {
                return;
            }

            Diagnostics.Assert(this.keyEnumerator != null, this.traceType, "enumerator can never be null");
            this.keyEnumerator.Dispose();
        }
    }
}