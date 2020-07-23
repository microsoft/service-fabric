// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.LockManager
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Data.Common;


    /// <summary>
    /// Represents a hash entry for a lock resource name.
    /// </summary>
    class LockHashValue : Disposable
    {
        [SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors")]
        /// <summary>
        /// Constructor.
        /// </summary>
        public LockHashValue()
        {
            AppTrace.TraceSource.WriteNoise("LockHashValue.LockHashValue", "{0}", this.GetHashCode());
        }

        #region Instance Members

        /// <summary>
        /// Povides concurrency control on this lock hash value.
        /// </summary>
        internal ReaderWriterLockSlim lockHashValueLock = new ReaderWriterLockSlim(LockRecursionPolicy.NoRecursion);

        /// <summary>
        /// List of lock resource control blocks for this lock resource name.
        /// </summary>
        internal LockResourceControlBlock lockResourceControlBlock = new LockResourceControlBlock();

        #endregion

        #region IDisposable Overrides

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting managed/unmanaged resources.
        /// </summary>
        /// <param name="disposing">True, if managed resources only are freed.</param>
        protected override void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    AppTrace.TraceSource.WriteNoise("LockHashValue.Dispose", "{0}", this.GetHashCode());

                    if (null != this.lockHashValueLock)
                    {
                        this.lockHashValueLock.Dispose();
                        this.lockHashValueLock = null;
                    }
                    if (null != this.lockResourceControlBlock)
                    {
                        this.lockResourceControlBlock.Dispose();
                        this.lockResourceControlBlock = null;
                    }
                }
                this.disposed = true;
            }
        }

        #endregion
    }

    /// <summary>
    /// Lock hash table, one per lock manager.
    /// </summary>
    class LockHashtable : Disposable
    {
        [SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors")]
        /// <summary>
        /// Constructor.
        /// </summary>
        public LockHashtable()
        {
            AppTrace.TraceSource.WriteNoise("LockHashtable.LockHashtable", "{0}", this.GetHashCode());
        }

        #region Instance Members

        /// <summary>
        /// Provides concurrency control on this lock hash table.
        /// </summary>
        internal ReaderWriterLockSlim lockHashTableLock = new ReaderWriterLockSlim(LockRecursionPolicy.NoRecursion);

        /// <summary>
        /// Dictionary mapping a lock resource name to lock control blocks for that lock resource.
        /// </summary>
        internal Dictionary<string, LockHashValue> lockEntries = new Dictionary<string, LockHashValue>();

        #endregion

        #region IDisposable Overrides

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting managed/unmanaged resources.
        /// </summary>
        /// <param name="disposing">True, if managed resources only are freed.</param>
        protected override void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    AppTrace.TraceSource.WriteNoise("LockHashtable.Dispose", "{0}", this.GetHashCode());

                    if (null != this.lockHashTableLock)
                    {
                        this.lockHashTableLock.Dispose();
                        this.lockHashTableLock = null;
                    }
                    if (null != this.lockEntries)
                    {
                        Dictionary<string, LockHashValue>.Enumerator enumerator = this.lockEntries.GetEnumerator();
                        while (enumerator.MoveNext())
                        {
                            enumerator.Current.Value.Dispose();
                        }
                        this.lockEntries.Clear();
                        this.lockEntries = null;
                    }
                }
                this.disposed = true;
            }
        }

        #endregion
    }
}