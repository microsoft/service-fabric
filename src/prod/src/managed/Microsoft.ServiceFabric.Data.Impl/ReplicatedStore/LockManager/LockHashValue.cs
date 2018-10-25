// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Threading;

    /// <summary>
    /// Represents a hash entry for a lock resource name.
    /// </summary>
    internal struct LockHashValue
    {
        #region Instance Members

        /// <summary>
        /// Povides concurrency control on this lock hash value.
        /// </summary>
        /// <remarks>
        /// In general rw locks are costlier than exclusive locks but it can be up to ~7x here because of spinning. 
        /// This gets worse when contention is high with retries. In the ReaderWriterLockSlim implementation, threads that are 
        /// releasing locks and ones acquiring locks have same priority causing the lock to be held longer. 
        ///
        /// In summary, using Monitor provides better perf than a ReaderWriteLockSlim
        /// </remarks>
        internal object LockHashValueLock;

        /// <summary>
        /// List of lock resource control blocks for this lock resource name.
        /// </summary>
        internal LockResourceControlBlock LockResourceControlBlock;

        #endregion

        /// <summary>
        /// Create an initialized instance of a LockHashValue (C# does not support default constructors for struct).
        /// </summary>
        public static LockHashValue Create()
        {
            return new LockHashValue()
            {
                LockHashValueLock = new object(),
                LockResourceControlBlock = new LockResourceControlBlock(),
            };
        }

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting managed/unmanaged resources.
        /// </summary>
        public void Close()
        {
            this.LockHashValueLock = null;

            if (null != this.LockResourceControlBlock)
            {
                this.LockResourceControlBlock.Dispose();
                this.LockResourceControlBlock = null;
            }
        }
    }
}