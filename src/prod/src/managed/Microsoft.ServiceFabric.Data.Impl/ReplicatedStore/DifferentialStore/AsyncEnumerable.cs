// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.Threading;

    using Microsoft.ServiceFabric.Data;

    internal class AsyncEnumerable<TKey, TValue> : IAsyncEnumerable<KeyValuePair<TKey, TValue>>
    {
        private readonly IReadableStoreComponent<TKey, TVersionedItem<TValue>> differentState;
        private readonly IReadableStoreComponent<TKey, TVersionedItem<TValue>> consolidatedState;
        private readonly MetadataTable currentMetadataTable;
        private readonly IStateSerializer<TValue> valueSerializer;
        private readonly IEnumerable<TKey> keyEnumerable;
        private readonly bool isValueAReferenceType;
        private readonly LoadValueCounter loadValueCounter;

        private readonly ReadMode readMode;

        private bool isInvalidated;

        private readonly string traceType;

        public AsyncEnumerable(
            bool isValueAReferenceType,
            ReadMode readMode,
            IEnumerable<TKey> keyEnumerables,
            IReadableStoreComponent<TKey, TVersionedItem<TValue>> differentState,
            IReadableStoreComponent<TKey, TVersionedItem<TValue>> consolidatedState,
            MetadataTable currentMetadataTable,
            LoadValueCounter loadValueCounter,
            IStateSerializer<TValue> valueSerializer,
            string traceType)
        {
            this.traceType = traceType;
            this.isValueAReferenceType = isValueAReferenceType;
            this.readMode = readMode;
            this.keyEnumerable = keyEnumerables;
            this.differentState = differentState;
            this.consolidatedState = consolidatedState;
            this.currentMetadataTable = currentMetadataTable;
            this.loadValueCounter = loadValueCounter;
            this.valueSerializer = valueSerializer;

            this.isInvalidated = false;
        }

        public IAsyncEnumerator<KeyValuePair<TKey, TValue>> GetAsyncEnumerator()
        {
            var snappedInvalidationFlag = Volatile.Read(ref this.isInvalidated);

            if (snappedInvalidationFlag)
            {
                throw new InvalidOperationException(SR.Error_TStore_InvalidEnumerable);
            }

            return new AsyncEnumerator<TKey, TValue>(this.isValueAReferenceType, this.readMode, keyEnumerable, differentState, consolidatedState, currentMetadataTable, this.loadValueCounter, valueSerializer, this.traceType);
        }

        internal void InvalidateEnumerable()
        {
            var snappedInvalidationFlag = Volatile.Read(ref this.isInvalidated);
            Diagnostics.Assert(snappedInvalidationFlag == false, this.traceType, "Must only be set once.");

            Volatile.Write(ref this.isInvalidated, true);
        }
    }
}