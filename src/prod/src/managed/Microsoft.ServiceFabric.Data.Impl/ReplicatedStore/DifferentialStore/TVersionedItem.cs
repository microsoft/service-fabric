// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Fabric.Data.Common;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Version record of an item.
    /// </summary>
    /// <typeparam name="TValue">Type of value stored in the item.</typeparam>
    internal abstract class TVersionedItem<TValue>
    {
        private const long CanBeSweepedToDiskFlag = 1L << 63;  // Most significant bit
        private const long InUseFlag = 1L << 62;     // Most significant second bit
        private const long MetadataMask = TVersionedItem<TValue>.CanBeSweepedToDiskFlag | TVersionedItem<TValue>.InUseFlag;

        private long valueOffset = 0;

        /// <summary>
        /// Gets the version of the versioned item.
        /// </summary>
        public long VersionSequenceNumber { get; set; }

        /// <summary>
        /// Gets the value of the versioned item.
        /// </summary>
        public virtual TValue Value { get; set; }

        /// <summary>
        /// Gets the record kind of this versioned item.
        /// </summary>
        public abstract RecordKind Kind { get; }

        /// <summary>
        /// Gets or sets file id.
        /// </summary>
        public uint FileId { get; set; }

        /// <summary>
        /// Gets or sets whether the value can be sweeped to disk or not. 
        /// This is false for value type and reference types that has a null for the value.
        /// </summary>
        public virtual bool CanBeSweepedToDisk
        {
            get
            {
                return (this.valueOffset & TVersionedItem<TValue>.CanBeSweepedToDiskFlag) == TVersionedItem<TValue>.CanBeSweepedToDiskFlag;
            }
            protected set
            {
                long offset = 0;
                long newOffset = 0;

                do
                {
                    offset = this.valueOffset;
                    if (value)
                    {
                        newOffset = offset | TVersionedItem<TValue>.CanBeSweepedToDiskFlag;
                    }
                    else
                    {
                        newOffset = offset & (~TVersionedItem<TValue>.CanBeSweepedToDiskFlag);
                    }

                } while (Interlocked.CompareExchange(ref this.valueOffset, newOffset, offset) != offset);
            }
        }

        /// <summary>
        /// Gets or sets the flag indicating if there is a reader who needs to access this value or not.
        /// </summary>
        public virtual bool InUse
        {
            get
            {
                return (this.valueOffset & TVersionedItem<TValue>.InUseFlag) == TVersionedItem<TValue>.InUseFlag;
            }
            protected set
            {
                long offset = 0;
                long newOffset = 0;

                do
                {
                    offset = this.valueOffset;
                    if (value)
                    {
                        newOffset = offset | TVersionedItem<TValue>.InUseFlag;
                    }
                    else
                    {
                        newOffset = offset & (~TVersionedItem<TValue>.InUseFlag);
                    }

                } while (Interlocked.CompareExchange(ref this.valueOffset, newOffset, offset) != offset);
            }
        }

        /// <summary>
        /// Gets or sets offset
        /// </summary>
        public virtual long Offset
        {
            get
            {
                return this.valueOffset & ~(TVersionedItem<TValue>.MetadataMask);
            }
            set
            {
                if ((value & TVersionedItem<TValue>.CanBeSweepedToDiskFlag) == TVersionedItem<TValue>.CanBeSweepedToDiskFlag
                    || (value & TVersionedItem<TValue>.InUseFlag) == TVersionedItem<TValue>.InUseFlag) // If either InUse or InMemory are set, then flag as error 
                {
                    Diagnostics.Assert(false, "Offset only supports long values up till 2^62-1.");
                }

                long offset = 0;
                long newOffset = 0;

                do
                {
                    offset = this.valueOffset;
                    newOffset = (offset & TVersionedItem<TValue>.MetadataMask) | value; // Retain the InMemory/InUse flags from old value but replace the last 62 bits
                } while (Interlocked.CompareExchange(ref this.valueOffset, newOffset, offset) != offset);
            }
        }

        /// <summary>
        /// Gets or sets value size
        /// </summary>
        public virtual int ValueSize { get; set; }

        /// <summary>
        /// Gets or sets the value's checksum.
        /// </summary>
        public virtual ulong ValueChecksum { get; set; }

        public virtual bool ShouldValueBeLoadedFromDisk(bool isValueAReferenceType, TValue value, string traceType)
        {
            // For value type, just check that it has been loaded .
            if (!isValueAReferenceType)
            {
                // Check if it has been loaded or not. InUse is set to true only after the value has been loaded and it can never be set to false after that.
                if (this.InUse)
                {
                    Diagnostics.Assert(!this.CanBeSweepedToDisk, traceType, "Can be sweeped to disk should be false for value types");
                    return false;
                }
                else
                {
                    return true;
                }
            }
            else
            {
                // For reference types- If it can be sweeped to disk and value is null, load it from disk.
                if (this.CanBeSweepedToDisk)
                {
                    if (value == null)
                    {
                        return true;
                    }
                }
                else
                {
                    // Assert thaht the value is null
                    Diagnostics.Assert(value == null, traceType, "Value should be null");
                }
            }

            return false;
        }

        public virtual void SetInUseToTrue(bool isValueAReferenceType)
        {
            if (isValueAReferenceType)
            {
                this.InUse = true;
            }
        }

        public virtual void SetInUseToFalse(bool isValueAReferenceType)
        {
            if (isValueAReferenceType)
            {
                this.InUse = false;
            }
        }

        public virtual void SetCanBeSweepedtoDiskToTrue()
        {
            this.CanBeSweepedToDisk = true;
        }

        public virtual void SetCanBeSweepedtoDiskToFalse()
        {
            this.CanBeSweepedToDisk = false;
        }

        /// <summary>
        /// Gets the user value in the versioned item.
        /// </summary>
        /// <param name="metadataTable">The metadata table</param>
        /// <param name="valueSerializer">The value serializer.</param>
        /// <param name="isValueAReferenceType">Is reference type</param>
        /// <param name="readMode">The read mode.</param>
        /// <param name="valueCounter">Loaded value counter.</param>
        /// <param name="cancellationToken">Token used to signal cancellation.</param>
        /// <param name="traceType">trace Id</param>
        /// <param name="duringRecovery">Called during recovery.</param>
        /// <returns>The value.</returns>
        /// <remarks>
        /// Reference Type Life Cycle
        /// At Recovery:    [InUse = false, CanBeSweepedToDisk = true, Value = null] 
        /// Read:           [InUse = readMode == CacheResult] [Value = CacheResult?] [CanBeSweepedToDisk = value != null]
        /// 
        /// Value Type Life Cycle
        /// At Recovery:    [InUse = false, CanBeSweepedToDisk = true, Value = default(TValue)] 
        /// First Read:     [Value = CacheResult?] [InUse = true] [CanBeSweepedToDisk = false]
        /// </remarks>
        public virtual async Task<TValue> GetValueAsync(
            MetadataTable metadataTable, 
            IStateSerializer<TValue> valueSerializer, 
            bool isValueAReferenceType, 
            ReadMode readMode, 
            LoadValueCounter valueCounter, 
            CancellationToken cancellationToken,
            string traceType,
            bool duringRecovery = false)
        {
            TValue value = this.Value;

            if (this.ShouldValueBeLoadedFromDisk(isValueAReferenceType, value, traceType) == false || readMode == ReadMode.Off)
            {
                return value;
            }

            Diagnostics.Assert(duringRecovery == true || readMode != ReadMode.CacheResult || isValueAReferenceType == false || this.InUse == true, traceType, "If CacheResult, InUse must have been set.");
            value = await MetadataManager.ReadValueAsync<TValue>(metadataTable.Table, this, valueSerializer, cancellationToken, traceType).ConfigureAwait(false);

            // Value must be set before updating the flags.
            // Example problem: After recovery two reads on the same key, 
            // T1 sees InUse (IsLoaded) == false, reads the value, updates InUse, *Context Switch*
            // T2 sees InUse (IsLoaded) == true, reads this.Value which has not been set by T1.
            if (this.ShouldCacheValue(readMode, isValueAReferenceType))
            {
                this.Value = value;
                valueCounter.IncrementCounter();
            }

            // Flags must always be updated. For value type, inUse (IsLoadedFromDiskAfterRecovery) must be set.
            this.UpdateFlagsFollowingLoadValue(isValueAReferenceType, value, readMode, traceType);

            return value;
        }

        public virtual async Task<TValue> GetValueAsync(FileMetadata fileMetadata, IStateSerializer<TValue> valueSerializer, bool isValueAReferenceType, ReadMode readMode, LoadValueCounter valueCounter, CancellationToken cancellationToken, string traceType)
        {
            TValue value = this.Value;

            if (this.ShouldValueBeLoadedFromDisk(isValueAReferenceType, value, traceType) == false || readMode == ReadMode.Off)
            {
                return value;
            }

            Diagnostics.Assert(readMode != ReadMode.CacheResult || isValueAReferenceType == false || this.InUse == true, traceType, "If CacheResult, InUse must have been set.");
            value = await MetadataManager.ReadValueAsync<TValue>(fileMetadata, this, valueSerializer, cancellationToken, traceType).ConfigureAwait(false);

            // Value must be set before updating the flags.
            // Example problem: After recovery two reads on the same key, 
            // T1 sees InUse (IsLoaded) == false, reads the value, updates InUse, *Context Switch*
            // T2 sees InUse (IsLoaded) == true, reads this.Value which has not been set by T1.
            if (this.ShouldCacheValue(readMode, isValueAReferenceType))
            {
                this.Value = value;
                valueCounter.IncrementCounter();
            }

            // Flags must always be updated. For value type, inUse (IsLoadedFromDiskAfterRecovery) must be set.
            this.UpdateFlagsFollowingLoadValue(isValueAReferenceType, value, readMode, traceType);

            return value;
        }

        private void UpdateFlagsFollowingLoadValue(bool isValueAReferenceType, TValue value, ReadMode readMode, string traceType)
        {
            // Called only after value has been loaded.
            if (isValueAReferenceType)
            {
                if (value == null)
                {
                    Diagnostics.Assert(this.Value == null, traceType, "Value must be null");
                    this.CanBeSweepedToDisk = false;
                }

                return;
            }

            // Set Inuse=True and CanBeSweepedToDisk=False at the same time
            // These need to be set at the same time, in case isValueAReferenceType is false
            // and another read is checking ShouldValueBeLoadedFromDisk
            if (this.ShouldCacheValue(readMode, isValueAReferenceType))
            {
                long offset = 0;
                long newOffset = 0;

                do
                {
                    offset = this.valueOffset;
                    newOffset = (offset | TVersionedItem<TValue>.InUseFlag) & (~TVersionedItem<TValue>.CanBeSweepedToDiskFlag);

                } while (Interlocked.CompareExchange(ref this.valueOffset, newOffset, offset) != offset);
            }
        }

        /// <summary>
        /// Check whether the value brought in should be inserted into the cache.
        /// </summary>
        /// <param name="readMode">The read mode.</param>
        /// <param name="isValueAReferenceType">Is value a reference type.</param>
        /// <returns>Boolean indicating whether the value should be cached.</returns>
        private bool ShouldCacheValue(ReadMode readMode, bool isValueAReferenceType)
        {
            if (readMode == ReadMode.CacheResult)
            {
                return true;
            }

            // TODO: Potential optimization: 
            // If value type and the value was loaded from disk (first time after recovery), load the value to the cache.
            // This is because default(TValue) size is same as the TValue itself if isValueAReferenceType == false.
            // However, if the TValue contains many references that are also serialized, this could increase memory overhead.
            return false;
        }
    }
}