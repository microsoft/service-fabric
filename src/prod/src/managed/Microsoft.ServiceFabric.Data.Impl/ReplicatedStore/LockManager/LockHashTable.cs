// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Threading;

    /// <summary>
    /// Lock hash table, one per lock manager.
    /// </summary>
    internal struct LockHashtable
    {
        #region Instance Members

        /// <summary>
        /// Provides concurrency control on this lock hash table.
        /// </summary>
        internal ReaderWriterLockSlim LockHashTableLock;

        /// <summary>
        /// Dictionary mapping a lock resource name hash to lock control blocks for that lock resource.
        /// </summary>
        internal Dictionary<ulong, LockHashValue> LockEntries;

        #endregion

        /// <summary>
        /// Create an initialized instance of a LockHashTable (C# does not support default constructors for struct).
        /// </summary>
        public static LockHashtable Create()
        {
            return new LockHashtable()
            {
                LockHashTableLock = new ReaderWriterLockSlim(LockRecursionPolicy.NoRecursion),
                LockEntries = new Dictionary<ulong, LockHashValue>(),
            };
        }

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting managed/unmanaged resources.
        /// </summary>
        public void Close()
        {
            if (null != this.LockHashTableLock)
            {
                this.LockHashTableLock.Dispose();
                this.LockHashTableLock = null;
            }

            if (null != this.LockEntries)
            {
                foreach (var lockEntry in this.LockEntries)
                {
                    lockEntry.Value.Close();
                }

                this.LockEntries.Clear();
                this.LockEntries = null;
            }
        }
    }
}