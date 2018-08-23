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
    /// Deleted version of an item.
    /// </summary>
    /// <typeparam name="TValue">Type of value for the item.</typeparam>
    internal class TDeletedItem<TValue> : TVersionedItem<TValue>
    {
        /// <summary>
        /// Default Constructor used by the primary
        /// </summary>
        public TDeletedItem()
        {
            this.VersionSequenceNumber = TStoreConstants.UninitializedVersionSequenceNumber;
        }

        /// <summary>
        /// Proper constructor.
        /// </summary>
        /// <param name="versionSequenceNumber">Version sequence number.</param>
        public TDeletedItem(long versionSequenceNumber)
        {
            this.VersionSequenceNumber = versionSequenceNumber;
        }

        /// <summary>
        /// Used for recovery only.
        /// </summary>
        /// <param name="versionSequenceNumber">Version sequence number.</param>
        /// <param name="fileId">File Id.</param>
        public TDeletedItem(long versionSequenceNumber, uint fileId)
        {
            this.VersionSequenceNumber = versionSequenceNumber;
            this.FileId = fileId;
        }

        /// <summary>
        /// Gets the value of the versioned item.
        /// </summary>
        public override TValue Value
        {
            get { return default(TValue); }
            set { throw new InvalidOperationException(SR.Error_TDeletedItem_NoValue); }
        }

        /// <summary>
        /// Gets the record kind of this versioned item.
        /// </summary>
        public override RecordKind Kind
        {
            get { return RecordKind.DeletedVersion; }
        }

        /// <summary>
        /// Gets or sets offset
        /// </summary>
        public override long Offset
        {
            get { throw new InvalidOperationException(SR.Error_TDeletedItem_NoValue); }
            set { throw new InvalidOperationException(SR.Error_TDeletedItem_NoValue); }
        }

        /// <summary>
        /// Gets or sets value size
        /// </summary>
        public override int ValueSize
        {
            get { throw new InvalidOperationException(SR.Error_TDeletedItem_NoValue); }
            set { throw new InvalidOperationException(SR.Error_TDeletedItem_NoValue); }
        }

        /// <summary>
        /// Gets or sets the value's checksum.
        /// </summary>
        public override ulong ValueChecksum
        {
            get { throw new InvalidOperationException(SR.Error_TDeletedItem_NoValue); }
            set { throw new InvalidOperationException(SR.Error_TDeletedItem_NoValue); }
        }

        /// <summary>
        /// Gets or sets whether the value is cached in memory.
        /// </summary>
        public override bool CanBeSweepedToDisk
        {
            get { return false; }
        }

        /// <summary>
        /// Get or sets the flag indicating if there is a reader reading this item, to be used for second-chance or clock algorithm.
        /// </summary>
        public override bool InUse
        {
            get { throw new InvalidOperationException("TDeletedItem has no value."); }
        }

        public override bool ShouldValueBeLoadedFromDisk(bool isValueAReferenceType, TValue value, string traceType)
        {
            return false;
        }

        public override void SetInUseToTrue(bool isValueAReferenceType)
        {
            throw new InvalidOperationException("TDeletedItem has no value.");
        }

        public override void SetInUseToFalse(bool isValueAReferenceType)
        {
            throw new InvalidOperationException("TDeletedItem has no value.");
        }

        public override Task<TValue> GetValueAsync(MetadataTable metadataTable, IStateSerializer<TValue> valueSerializer, bool isValueARefernceType, ReadMode readMode, LoadValueCounter valueCounter, CancellationToken cancellationToken, string traceType, bool duringRecovery = false)
        {
            throw new InvalidOperationException(SR.Error_TDeletedItem_NoValue);
        }

        public override Task<TValue> GetValueAsync(FileMetadata fileMetadata, IStateSerializer<TValue> valueSerializer, bool isValueARefernceType, ReadMode readMode, LoadValueCounter valueCounter, CancellationToken cancellationToken, string traceType)
        {
            throw new InvalidOperationException(SR.Error_TDeletedItem_NoValue);
        }
    }
}