// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Fabric.Common;
    using System.Fabric.Data.Common;
    using System.Threading;

    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Replicator;
    using Microsoft.ServiceFabric.Data.DataStructures;

    /// <summary>
    /// Disk Metadata.
    /// </summary>
    internal class FileMetadata
    {
        private const uint LowestFileId = 1;
        private const long LowestTimeStamp = 1;
        internal const long InvalidTimeStamp = -1;

        // Start with reference count 1 to account for adding ref on creation time.
        private int referenceCount = 1;
        private long numberOfValidEntries;
        private string tracer;
        private bool disposed;
        private object disposeLock;

#if DEBUG
        internal long numInvalidKeysForAssert = 0;
#endif

        internal BloomFilter<int> invalidEntryBloomFilter;

        public FileMetadata(
            string tracer, uint fileId, string fileName, long totalNumberOfEntries, long numberOfValidEntries, long timeStamp,
            long numberOfDeletedEntries, bool canBeDeleted, long oldestDeletedEntryTimeStamp, long latestDeletedEntryTimeStamp,
            long percentageOfInvalidEntriesPerFile = 33)
        {
            Diagnostics.Assert(fileId >= LowestFileId, tracer, "FileId {0} must be larger than 0", fileId);
            Diagnostics.Assert(timeStamp >= LowestTimeStamp, tracer, "TimeStamp {0} must be larger than 0", timeStamp);
            Diagnostics.Assert(
                percentageOfInvalidEntriesPerFile > 0,
                tracer,
                "percentageOfInvalidEntriesPerFile {0} must be greater than 0",
                percentageOfInvalidEntriesPerFile);

            this.FileId = fileId;
            this.FileName = fileName;
            this.TotalNumberOfEntries = totalNumberOfEntries;
            this.numberOfValidEntries = numberOfValidEntries;
            this.CanBeDeleted = canBeDeleted;
            this.TimeStamp = timeStamp;
            this.NumberOfDeletedEntries = numberOfDeletedEntries;
            this.tracer = tracer;
            this.disposeLock = new object();
            this.OldestDeletedEntryTimeStamp = oldestDeletedEntryTimeStamp;
            this.LatestDeletedEntryTimeStamp = latestDeletedEntryTimeStamp;

            // Number of invalid entries before this file is eligible for merge
            var maxInvalidEntries = totalNumberOfEntries * percentageOfInvalidEntriesPerFile / 100;
            maxInvalidEntries = maxInvalidEntries > 0 ? maxInvalidEntries : totalNumberOfEntries;
            if (maxInvalidEntries < Int32.MaxValue)
            {
                // Cap the size of bloom filter to be not more than 18KB.
                // 18KB corresponds to about 40K total entries in file
                // and hence 40K/3 invalid entires in bloom filter with 10% false positive rate probability
                // is about 18KB
                if (BloomFilter<int>.OptimalBloomFilterSize((int)maxInvalidEntries, 0.1f) > 18*1024*8) // 18KB
                {
                    this.invalidEntryBloomFilter = new BloomFilter<int>(18 * 1024 * 8, (int)maxInvalidEntries);
                }
                else
                {
                    this.invalidEntryBloomFilter = new BloomFilter<int>((int)maxInvalidEntries, 0.1f);
                }
            }
        }

        public void AddInvalidKey<T>(T key)
        {
            if (invalidEntryBloomFilter != null)
            {
                if ((NumberOfInvalidEntries * 1.0f / TotalNumberOfEntries) >= 33)
                {
                    // File is eligible for merge ignore adding invalid entries to bloom filter
                    invalidEntryBloomFilter = null;
                }
                else
                {
                    int hash = key.GetHashCode();
                    invalidEntryBloomFilter.Add(hash);
                }
            }

            DecrementValidEntries();
        }

        /// <summary>
        /// Checks if key is present in set of invalid entries in this file.
        /// Can possibly return false positive. 
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="key"></param>
        /// <returns></returns>
        public bool ContainsInvalidKey<T>(T key)
        {
            if (invalidEntryBloomFilter != null)
            {
                int hash = key.GetHashCode();
                return invalidEntryBloomFilter.Contains(hash);
            }
            else
            {
                // Defaulting to true as false positives are OK 
                return true;
            }
        }

        public CheckpointFile CheckpointFile { get; set; }

        public long TotalNumberOfEntries { get; set; }

        public long NumberOfValidEntries
        {
            get { return this.numberOfValidEntries; }
            set { this.numberOfValidEntries = value; }
        }

        public long NumberOfInvalidEntries
        {
            get { return TotalNumberOfEntries - NumberOfValidEntries; }
        }

        public long NumberOfDeletedEntries { get; set; }

        public uint FileId { get; private set; }

        public string FileName { get; set; }

        public bool CanBeDeleted { get; set; }

        // Expose a setter for testability.
        public long TimeStamp { get; set; }

        public long OldestDeletedEntryTimeStamp { get; set; }
        public long LatestDeletedEntryTimeStamp { get; set; }

        // Exposed for testability.
        public int ReferenceCount
        {
            get
            {
                return this.referenceCount;
            }
        }

        /// <summary>
        /// Gets the total size of the checkpoint on disk (key + value file)
        /// </summary>
        /// <returns>Size of the checkpoint in bytes.</returns>
        public long GetFileSize()
        {
            return this.CheckpointFile.GetFileSize();
        }

        /// <summary>
        /// Serializes the FileMetadata into an in-memory binary writer.
        /// </summary>
        /// <param name="writer"></param>
        /// <remarks>
        /// This serialization is 8 byte aligned.
        /// 
        /// Name                    || Size
        /// 
        /// TotalNumberOfEntries       8
        /// NumberOfValidEntries       8
        /// NumberOfDeletedEntries     8
        /// TimeStamp                  8
        /// 
        /// FileId                     4
        /// CanBeDeleted               1
        /// RESERVED                   3
        /// 
        /// FileName                   N
        /// PADDING                    (N % 8 ==0) ? 0 : 8 - (N % 8)
        /// 
        /// RESERVED: Fixed padding that is usable to add fields in future.
        /// PADDING:  Due to dynamic size, cannot be used for adding fields.
        /// 
        /// </remarks>
        public void Write(InMemoryBinaryWriter writer)
        {
            Utility.Assert(writer.IsAligned(), "must be aligned");

            // Serialize the metadata.
            writer.Write(this.TotalNumberOfEntries);
            writer.Write(this.NumberOfValidEntries);
            writer.Write(this.NumberOfDeletedEntries);
            writer.Write(this.TimeStamp);

            // With RESERVE
            writer.Write(this.FileId);
            writer.WritePaddingUntilAligned();

            // With PADDING
            writer.Write(this.FileName);
            writer.WritePaddingUntilAligned();

#if !DotNetCoreClr
            FabricEvents.Events.CheckpointFileMetadata(this.tracer, DifferentialStoreConstants.FileMetadata_Write, FileName, FileId,
                TotalNumberOfEntries, NumberOfDeletedEntries, NumberOfInvalidEntries);
#endif

            Utility.Assert(writer.IsAligned(), "must be aligned");
        }

        /// <summary>
        /// Deserialzies the FileMetadata from stream.
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="tracer"></param>
        /// <returns></returns>
        /// <remarks>
        /// Name                    || Size
        /// 
        /// TotalNumberOfEntries       8
        /// NumberOfValidEntries       8
        /// NumberOfDeletedEntries     8
        /// TimeStamp                  8
        /// 
        /// FileId                     4
        /// CanBeDeleted               1
        /// RESERVED                   3
        /// 
        /// FileName                   N
        /// PADDING                    (N % 8 ==0) ? 0 : 8 - (N % 8)
        /// 
        /// RESERVED: Fixed padding that is usable to add fields in future.
        /// PADDING:  Due to dynamic size, cannot be used for adding fields.
        /// 
        /// </remarks>
        public static FileMetadata Read(InMemoryBinaryReader reader, string tracer)
        {
            Utility.Assert(reader.IsAligned(), "must be aligned");

            var totalNumberOfEntries = reader.ReadInt64();
            var numberOfValidEntries = reader.ReadInt64();
            var numberOfDeletedEntries = reader.ReadInt64();
            var timeStamp = reader.ReadInt64();

            var fileId = reader.ReadUInt32();
            reader.ReadPaddingUntilAligned(true);

            var fileName = reader.ReadString();
            reader.ReadPaddingUntilAligned(false);

            Utility.Assert(reader.IsAligned(), "must be aligned");

            return new FileMetadata(tracer, fileId, 
                fileName, totalNumberOfEntries, 
                numberOfValidEntries, timeStamp, 
                numberOfDeletedEntries, false, 
                FileMetadata.InvalidTimeStamp, 
                FileMetadata.InvalidTimeStamp);
        }

        // Caller ensures that this never races with ReleaseRef and Dispose.
        // AddRef is called when items are moved into snapshot componenet, it comes during consolidation (which cannot happen on close) or during apply but
        // items are always in memory during apply.  So it's okay to do AddRef instead of TryAddRef.
        public void AddRef()
        {
            Interlocked.Increment(ref this.referenceCount);
            Diagnostics.Assert(this.referenceCount >= 2, this.tracer, "Unbalanced AddRef-ReleaseRef detected!");
        }

        public void ReleaseRef()
        {
            var refCount = Interlocked.Decrement(ref this.referenceCount);
            Diagnostics.Assert(refCount >= 0, this.tracer, "Unbalanced AddRef-ReleaseRef detected!");

            if (refCount == 0)
            {
                this.Dispose();
            }
        }

        public void DecrementValidEntries()
        {
            Interlocked.Decrement(ref this.numberOfValidEntries);

            // TODO Enable the assert below once item 5861402 is fixed.
            //Diagnostics.InternalAssert(this.numberOfValidEntries >= 0, "Incorrect number of invalid entries");
        }

        public void ResetInvalidCount()
        {
            this.numberOfValidEntries = this.TotalNumberOfEntries;
        }

        public void IncrementInvalidCountForAssert(RecordKind recordKind)
        {
#if DEBUG
            // DeletedVersion is not invalidated
            if (recordKind != RecordKind.DeletedVersion)
            {
                this.numInvalidKeysForAssert++;
            }
#endif
        }

        public void Validate()
        {
            if (this.NumberOfDeletedEntries == 0)
            {
                Diagnostics.Assert(OldestDeletedEntryTimeStamp == FileMetadata.InvalidTimeStamp, this.tracer, "oldestDeletedEntryTimeStamp should not be set when no deleted entries");
                Diagnostics.Assert(LatestDeletedEntryTimeStamp == FileMetadata.InvalidTimeStamp, this.tracer, "latestDeletedEntryTimeStamp should not be set when no deleted entries");
            }
            else
            {
                Diagnostics.Assert(OldestDeletedEntryTimeStamp > FileMetadata.InvalidTimeStamp, this.tracer, "oldestDeletedEntryTimeStamp should be set when deleted entries are present");
                Diagnostics.Assert(LatestDeletedEntryTimeStamp > FileMetadata.InvalidTimeStamp, this.tracer, "latestDeletedEntryTimeStamp should be set when deleted entries are present");
            }
        }

        internal void UpdateDeleteEntryTimeStamps(long timeStamp)
        {
            if (timeStamp < OldestDeletedEntryTimeStamp || OldestDeletedEntryTimeStamp == FileMetadata.InvalidTimeStamp)
            {
                OldestDeletedEntryTimeStamp = timeStamp;
            }
            if (timeStamp > LatestDeletedEntryTimeStamp || LatestDeletedEntryTimeStamp == FileMetadata.InvalidTimeStamp)
            {
                LatestDeletedEntryTimeStamp = timeStamp;
            }
        }

        public void Dispose()
        {
            lock (this.disposeLock)
            {
                if (this.disposed)
                {
                    return;
                }

                if (this.CheckpointFile != null)
                {
                    var keyFileName = this.CheckpointFile.KeyCheckpointFileName;
                    var valueFileName = this.CheckpointFile.ValueCheckpointFileName;

                    this.CheckpointFile.Dispose();

                    if (this.CanBeDeleted)
                    {
                        FabricEvents.Events.FileMetadataDeleteKeyValue(this.tracer, keyFileName, valueFileName);
                        FabricFile.Delete(keyFileName);
                        FabricFile.Delete(valueFileName);
                    }

                    this.disposed = true;
                }
            }
        }
    }
}