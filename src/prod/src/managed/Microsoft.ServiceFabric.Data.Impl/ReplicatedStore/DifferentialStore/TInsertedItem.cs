// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Inserted version of an item.
    /// </summary>
    /// <typeparam name="TValue">Type of value for the item.</typeparam>
    internal class TInsertedItem<TValue> : TVersionedItem<TValue>
    {
        /// <summary>
        /// Constructor called during creation on the primary.
        /// </summary>
        /// <param name="value"></param>
        /// <param name="valueSize"></param>
        /// <param name="canBeSweepedToDisk"></param>
        public TInsertedItem(TValue value, int valueSize, bool canBeSweepedToDisk)
            : this(TStoreConstants.UninitializedVersionSequenceNumber, value, valueSize, canBeSweepedToDisk)
        {
        }

        /// <summary>
        /// Constructor called on Apply.
        /// </summary>
        /// <param name="versionSequenceNumber">Version sequence number.</param>
        /// <param name="value">Value for the versioned item.</param>
        /// <param name="valueSize">Value size for the versioned item.</param>
        /// <param name="canBeSweepedToDisk"></param>
        public TInsertedItem(long versionSequenceNumber, TValue value, int valueSize, bool canBeSweepedToDisk)
        {
            this.VersionSequenceNumber = versionSequenceNumber;
            this.Value = value;
            this.CanBeSweepedToDisk = canBeSweepedToDisk;
            this.ValueSize = valueSize;
            this.InUse = true;
        }

        /// <summary>
        /// Constructor called when item is populated from disk and on sweep.
        /// </summary>
        /// <param name="versionSequenceNumber">Version sequence number.</param>
        /// <param name="fileId"></param>
        /// <param name="fileOffset"></param>
        /// <param name="valueSize"></param>
        /// <param name="valueChecksum"></param>
        public TInsertedItem(long versionSequenceNumber, uint fileId, long fileOffset, int valueSize, ulong valueChecksum)
        {
            this.VersionSequenceNumber = versionSequenceNumber;
            this.Value = default(TValue);
            this.FileId = fileId;
            this.Offset = fileOffset;
            this.ValueSize = valueSize;
            this.ValueChecksum = valueChecksum;

            // On recovery, CanBeSweeped to disk should be true and inuse should be false.
            this.CanBeSweepedToDisk = true;
            this.InUse = false;
        }
                
        /// <summary>
        /// Gets the record kind of this versioned item.
        /// </summary>
        public override RecordKind Kind
        {
            get { return RecordKind.InsertedVersion; }
        }
    }
}