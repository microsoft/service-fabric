// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;

    /// <summary>
    /// Encapsulates a lock resource control block.
    /// </summary>
    internal class LockResourceControlBlock : Disposable
    {
        #region Instance Members

        /// <summary>
        /// Max lock mode granted for a list of lock control blocks.
        /// </summary>
        internal LockMode LockModeGranted = LockMode.Free;

        /// <summary>
        /// The list of lock control blocks associated with lock requests. Contains granted  requests.
        /// </summary>
        internal List<LockControlBlock> GrantedList = new List<LockControlBlock>();

        /// <summary>
        /// The list of lock control blocks associated with lock requests. Contains pending requests.
        /// </summary>
        internal List<LockControlBlock> WaitingQueue = new List<LockControlBlock>();

        #endregion

        /// <summary>
        /// Finds a lock owner for this lock mode. The lock owner could be in the granted list of wainting list.
        /// </summary>
        /// <param name="owner">Lock owner to retrieve.</param>
        /// <param name="isWaitingList">True, if the search is to be performed in the waiting list. Otherwise, granted list.</param>
        /// <returns></returns>
        internal LockControlBlock LocateLockClient(long owner, bool isWaitingList)
        {
            LockControlBlock lockControlBlock = null;
            var workingList = isWaitingList ? this.WaitingQueue : this.GrantedList;
            for (var index = 0; index < workingList.Count; index++)
            {
                if (owner == workingList[index].LockOwner)
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
        internal bool IsSingleLockOwnerGranted(long owner)
        {
            if (long.MinValue == owner)
            {
                if (0 != this.GrantedList.Count)
                {
                    owner = this.GrantedList[0].LockOwner;
                }
                else
                {
                    return false;
                }
            }

            for (var index = 0; index < this.GrantedList.Count; index++)
            {
                if (owner != this.GrantedList[index].Owner)
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
            if (!this.Disposed)
            {
                if (disposing)
                {
                    if (null != this.GrantedList)
                    {
                        for (var index = 0; index < this.GrantedList.Count; index++)
                        {
                            this.GrantedList[index].Dispose();
                        }

                        this.GrantedList.Clear();
                        this.GrantedList = null;
                    }

                    if (null != this.WaitingQueue)
                    {
                        for (var index = 0; index < this.WaitingQueue.Count; index++)
                        {
                            this.WaitingQueue[index].Dispose();
                        }

                        this.WaitingQueue.Clear();
                        this.WaitingQueue = null;
                    }
                }

                this.Disposed = true;
            }
        }

        #endregion
    }
}