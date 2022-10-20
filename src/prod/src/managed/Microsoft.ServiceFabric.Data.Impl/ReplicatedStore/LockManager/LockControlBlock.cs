// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Encapsulates a lock control block.
    /// </summary>
    internal class LockControlBlock : Disposable, ILock
    {
        #region Instance Members

        /// <summary>
        /// The client that requested this lock.
        /// </summary>
        internal long LockOwner;

        /// <summary>
        /// Lock resource name hash.
        /// </summary>
        internal ulong LockResourceNameHash;

        /// <summary>
        /// Lock mode.
        /// </summary>
        internal LockMode LockMode;

        /// <summary>
        /// Lock timeout for the request.
        /// </summary>
        internal int LockTimeout;

        /// <summary>
        /// Lock status (success or pending).
        /// </summary>
        internal LockStatus LockStatus;

        /// <summary>
        /// Number of times this lock has been granted.
        /// </summary>
        internal int LockCount;

        /// <summary>
        /// Time at which the lock was granted.
        /// </summary>
        internal long GrantTime;

        /// <summary>
        /// Waiter associated with this lock control block until the lock is granted.
        /// </summary>
        internal TaskCompletionSource<ILock> Waiter;

        /// <summary>
        /// True if the lock control block is created for an upgrade lock request.
        /// </summary>
        internal bool IsUpgradedLock;

        /// <summary>
        /// Timer that checks for expired lock case.
        /// </summary>
        private Timer timerExpiredLockRequest;

        /// <summary>
        /// Lock manager using this lock control block.
        /// </summary>
        private LockManager lockManager;

        /// <summary>
        /// The oldest grantee of the resource at the time this lock was expired
        /// If not expired, then this is -1
        /// </summary>
        internal long oldestGrantee;

        #endregion

        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="lockManager">Lock manager that owns this lock control block.</param>
        /// <param name="owner">Owner of this lock control block.</param>
        /// <param name="resourceNameHash">The hash of the resource name that is being locked.</param>
        /// <param name="mode">Lock mode requested.</param>
        /// <param name="timeout">Specifies for how long a pending request is kept alive.</param>
        /// <param name="status">Current lock status.</param>
        /// <param name="isUpgraded"></param>
        /// <param name="oldestGrantee">The oldest grantee of the lock when this is expired</param>
        [SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Justification = "By design.")]
        internal LockControlBlock(
            LockManager lockManager,
            long owner,
            ulong resourceNameHash,
            LockMode mode,
            int timeout,
            LockStatus status,
            bool isUpgraded,
            long oldestGrantee = -1)
        {
            this.lockManager = lockManager;
            this.LockOwner = owner;
            this.LockResourceNameHash = resourceNameHash;
            this.LockMode = mode;
            this.LockTimeout = timeout;
            this.LockStatus = status;
            this.LockCount = 1;
            this.GrantTime = long.MinValue;
            this.IsUpgradedLock = isUpgraded;
            this.oldestGrantee = oldestGrantee;
        }

        #region ILockResult Members

        /// <summary>
        /// 
        /// </summary>
        public long Owner
        {
            get { return this.LockOwner; }
        }

        /// <summary>
        /// 
        /// </summary>
        public LockMode Mode
        {
            get { return this.LockMode; }
        }

        /// <summary>
        /// 
        /// </summary>
        public int Timeout
        {
            get { return this.LockTimeout; }
        }

        /// <summary>
        /// 
        /// </summary>
        public LockStatus Status
        {
            get { return this.LockStatus; }
        }

        /// <summary>
        /// 
        /// </summary>
        public long GrantedTime
        {
            get { return this.GrantTime; }
        }

        /// <summary>
        /// 
        /// </summary>
        public int Count
        {
            get { return this.LockCount; }
        }

        /// <summary>
        /// 
        /// </summary>
        public bool IsUpgraded
        {
            get { return this.IsUpgradedLock; }
        }

        /// <summary>
        /// 
        /// </summary>
        public LockManager LockManager
        {
            get { return this.lockManager; }
        }

        public long OldestGrantee
        {
            get { return this.oldestGrantee; }
        }

        #endregion

        /// <summary>
        /// Override from object.
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Owner={0}, Resource={1}, Mode={2}, Status={3}, Count={4}, Upgrade={5})",
                this.LockOwner,
                this.LockResourceNameHash,
                this.LockMode,
                this.LockStatus,
                this.LockCount,
                this.IsUpgradedLock);
        }

        /// <summary>
        /// Expires this lock control block.
        /// </summary>
        internal void StartExpire()
        {
            if (System.Threading.Timeout.Infinite != this.LockTimeout)
            {
                this.timerExpiredLockRequest = new Timer(this.Expire, null, this.LockTimeout, System.Threading.Timeout.Infinite);
            }
        }

        /// <summary>
        /// Terminates the expiration timer.
        /// </summary>
        internal bool StopExpire()
        {
            var success = true;
            if (System.Threading.Timeout.Infinite != this.LockTimeout && null != this.timerExpiredLockRequest)
            {
                success = this.timerExpiredLockRequest.Change(System.Threading.Timeout.Infinite, System.Threading.Timeout.Infinite);
            }

            return success;
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
                    if (null != this.timerExpiredLockRequest)
                    {
                        this.timerExpiredLockRequest.Dispose();
                        this.timerExpiredLockRequest = null;
                    }

                    if (null != this.lockManager)
                    {
                        this.lockManager = null;
                    }

                    this.Disposed = true;
                }
            }
        }

        #endregion

        /// <summary>
        /// If the lock control block has expired, call the lock manager to complete the pending lock request.
        /// </summary>
        private void Expire(object state)
        {
            var lockManager = this.lockManager;
            if (null != lockManager && lockManager.ExpireLock(this))
            {
                this.Dispose();
            }
        }
    }
}