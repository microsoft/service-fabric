// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.Threading;

    /// <summary>
    /// Metadata table that tracks metadata about key files and values files.
    /// </summary>
    internal class MetadataTable : IDisposable
    {
        /// <summary>
        /// Test flag only. Due to infinite backwards compat requirement from Backup/Restore, we can never turn this on except for tests.
        /// </summary>
        public static bool Test_IsCheckpointLSNExpected = false;

        private readonly bool isDoNotCopy;

        private long checkpointLSN;

        private int referenceCount = 1;
        private bool disposed = false;
        private readonly string traceType;

        public Dictionary<uint, FileMetadata> Table { get; set; }

        public long MetadataFileSize { get; set; }

        public MetadataTable(string traceType) : this(DifferentialStoreConstants.InvalidLSN, false)
        {
            this.traceType = traceType;
        }

        public MetadataTable(long checkpointLSN) : this(checkpointLSN, false)
        {
        }

        public MetadataTable(long checkpointLSN, bool isDoNotCopy)
        {
            this.Table = new Dictionary<uint, FileMetadata>();
            this.isDoNotCopy = isDoNotCopy;

            this.checkpointLSN = checkpointLSN;
        }

        public bool IsDoNotCopy
        {
            get
            {
                return this.isDoNotCopy;
            }
        }

        public long CheckpointLSN
        {
            get
            {
                return this.checkpointLSN;
            }
            set
            {
                // MCoskun: Once we know a checkpoint 
                if (Test_IsCheckpointLSNExpected)
                {
                    Diagnostics.Assert(value >= DifferentialStoreConstants.ZeroLSN, this.traceType, "CheckpointLSN must be positive.");
                }

                this.checkpointLSN = value;
            }
        }

        public void AddRef()
        {
            Interlocked.Increment(ref this.referenceCount);
        }

        public void ReleaseRef()
        {
            var refCount = Interlocked.Decrement(ref this.referenceCount);
            Diagnostics.Assert(refCount >= 0, this.traceType, "Unbalanced AddRef-ReleaseRef detected!");

            if (refCount == 0)
            {
                this.Dispose();
            }
        }

        public bool TryAddRef()
        {
            var refCount = 0;

            // Spin instead of locking since contention is low.
            do
            {
                refCount = Volatile.Read(ref this.referenceCount);
                if (refCount == 0)
                    return false;
            } while (Interlocked.CompareExchange(ref this.referenceCount, refCount + 1, refCount) != refCount);

            return true;
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                foreach (var metadata in this.Table)
                {
                    metadata.Value.ReleaseRef();
                }

                this.disposed = true;
            }
        }
    }
}