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
    /// Encapsulates a lock resource control block.
    /// </summary>
    class LockResourceControlBlock : Disposable
    {
        [SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors")]
        /// <summary>
        /// Constructor.
        /// </summary>
        public LockResourceControlBlock()
        {
            AppTrace.TraceSource.WriteNoise("LockResourceControlBlock.LockResourceControlBlock", "{0}", this.GetHashCode());
        }

        #region Instance Members

        /// <summary>
        /// Max lock mode granted for a list of lock control blocks.
        /// </summary>
        internal LockMode lockModeGranted = LockMode.Free;

        /// <summary>
        /// The list of lock control blocks associated with lock requests. Contains granted  requests.
        /// </summary>
        internal List<LockControlBlock> grantedList = new List<LockControlBlock>();

        /// <summary>
        /// The list of lock control blocks associated with lock requests. Contains pending requests.
        /// </summary>
        internal List<LockControlBlock> waitingQueue = new List<LockControlBlock>();

        #endregion

        /// <summary>
        /// Finds a lock owner for this lock mode. The lock owner could be in the granted list of wainting list.
        /// </summary>
        /// <param name="owner">Lock owner to retrieve.</param>
        /// <param name="isWaitingList">True, if the search is to be performed in the waiting list. Otherwise, granted list.</param>
        /// <returns></returns>
        internal LockControlBlock LocateLockClient(Uri owner, bool isWaitingList)
        {
            LockControlBlock lockControlBlock = null;
            List<LockControlBlock> workingList = isWaitingList ? this.waitingQueue : this.grantedList;
            for (int index = 0; index < workingList.Count; index++)
            {
                if (0 == Uri.Compare(owner, workingList[index].lockOwner, UriComponents.AbsoluteUri, UriFormat.UriEscaped, StringComparison.Ordinal))
                {
                    lockControlBlock = workingList[index];
                    break;
                }
            }
            return lockControlBlock;
        }
        /// <summary>
        /// Returs true, if there is a single lock owner for this resource control block.
        /// </summary>
        /// <param name="owner">Lock owner to be retrieved.</param>
        /// <returns></returns>
        internal bool IsSingleLockOwnerGranted(Uri owner)
        {
            if (null == owner)
            {
                if (0 != this.grantedList.Count)
                {
                    owner = this.grantedList[0].lockOwner;
                }
                else
                {
                    return false;
                }
            }
            for (int index = 0; index < this.grantedList.Count; index++)
            {
                if (0 != Uri.Compare(owner, this.grantedList[index].lockOwner, UriComponents.AbsoluteUri, UriFormat.UriEscaped, StringComparison.Ordinal))
                {
                    return false;
                }
            }
            return true;
        }

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
                    AppTrace.TraceSource.WriteNoise("LockResourceControlBlock.Dispose", "{0}", this.GetHashCode());

                    if (null != this.grantedList)
                    {
                        for (int index = 0; index < this.grantedList.Count; index++)
                        {
                            this.grantedList[index].Dispose();
                        }
                        this.grantedList.Clear();
                        this.grantedList = null;
                    }
                    if (null != this.waitingQueue)
                    {
                        for (int index = 0; index < this.waitingQueue.Count; index++)
                        {
                            this.waitingQueue[index].Dispose();
                        }
                        this.waitingQueue.Clear();
                        this.waitingQueue = null;
                    }
                }
                this.disposed = true;
            }
        }

        #endregion
    }
}