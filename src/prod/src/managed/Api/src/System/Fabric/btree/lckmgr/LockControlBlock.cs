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
    using System.Globalization;
    using System.Fabric.Common;
    using System.Fabric.Data.Common;


    /// <summary>
    /// Returned as part of lock acquisition requests.
    /// </summary>
    public interface ILock
    {
        /// <summary>
        /// The lock owner for this lock.
        /// </summary>
        Uri Owner { get; }

        /// <summary>
        /// Lock resource name.
        /// </summary>
        string ResourceName { get; }

        /// <summary>
        /// Lock mode.
        /// </summary>
        LockMode Mode { get; }

        /// <summary>
        /// Lock timeout.
        /// </summary>
        int Timeout { get; }

        /// <summary>
        /// Lock status (success or pending).
        /// </summary>
        LockStatus Status { get; }

        /// <summary>
        /// 
        /// </summary>
        long GrantTime { get; }

        /// <summary>
        /// Lock reference count.
        /// </summary>
        int Count { get; }

        /// <summary>
        /// Is this an upgrade lock request.
        /// </summary>
        bool IsUpgraded { get; }
    }

    /// <summary>
    /// Encapsulates a lock control block.
    /// </summary>
    class LockControlBlock : Disposable, ILock
    {
        [SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors")]
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="lockManager">Lock manager that owns this lock control block.</param>
        /// <param name="owner">Owner of this lock control block.</param>
        /// <param name="resourceName">The resource name that is being locked.</param>
        /// <param name="mode">Lock mode requested.</param>
        /// <param name="timeout">Specifies for how long a pending request is kept alive.</param>
        /// <param name="status">Current lock status.</param>
        internal LockControlBlock(
            LockManager lockManager,
            Uri owner,
            string resourceName, 
            LockMode mode, 
            int timeout,
            LockStatus status,
            bool isUpgraded)
        {
            this.lockManager = lockManager;
            this.lockOwner = owner;
            this.lockResourceName = resourceName;
            this.lockMode = mode;
            this.lockTimeout = timeout;
            this.lockStatus = status;
            this.lockCount = 1;
            this.grantTime = long.MinValue;
            this.isUpgraded = isUpgraded;
            AppTrace.TraceSource.WriteNoise("LockControlBlock.LockControlBlock", "{0}", this.ToString());
        }

        #region Instance Members

        /// <summary>
        /// Lock manager using this lock control block.
        /// </summary>
        LockManager lockManager;

        /// <summary>
        /// The client that requested this lock.
        /// </summary>
        internal Uri lockOwner;

        /// <summary>
        /// Lock resource name.
        /// </summary>
        internal string lockResourceName;

        /// <summary>
        /// Lock mode.
        /// </summary>
        internal LockMode lockMode;

        /// <summary>
        /// Lock timeout for the request.
        /// </summary>
        internal int lockTimeout;

        /// <summary>
        /// Lock status (success or pending).
        /// </summary>
        internal LockStatus lockStatus;

        /// <summary>
        /// Number of times this lock has been granted.
        /// </summary>
        internal int lockCount;

        /// <summary>
        /// Time at which the lock was granted.
        /// </summary>
        internal long grantTime;

        /// <summary>
        /// Waiter associated with this lock control block until the lock is granted.
        /// </summary>
        internal TaskCompletionSource<ILock> waiter;

        /// <summary>
        /// Timer that checks for expired lock case.
        /// </summary>
        Timer timerExpiredLockRequest;

        /// <summary>
        /// True if the lock control block is created for an upgrade lock request.
        /// </summary>
        internal bool isUpgraded;

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
                AppTrace.TraceSource.WriteNoise("LockControlBlock.Dispose", "{0}", this.ToString());

                if (disposing)
                {
                    if (null != this.timerExpiredLockRequest)
                    {
                        this.timerExpiredLockRequest.Dispose();
                        this.timerExpiredLockRequest = null;
                    }
                }
                this.disposed = true;
            }
        }

        #endregion

        #region ILockResult Members

        /// <summary>
        /// 
        /// </summary>
        public Uri Owner
        {
            get { return this.lockOwner; }
        }

        /// <summary>
        /// 
        /// </summary>
        public string ResourceName
        {
            get { return this.lockResourceName; }
        }

        /// <summary>
        /// 
        /// </summary>
        public LockMode Mode
        {
            get { return this.lockMode; }
        }

        /// <summary>
        /// 
        /// </summary>
        public int Timeout
        {
            get { return this.lockTimeout; }
        }

        /// <summary>
        /// 
        /// </summary>
        public LockStatus Status
        {
            get { return this.lockStatus; }
        }

        /// <summary>
        /// 
        /// </summary>
        public long GrantTime 
        {
            get
            {
                return this.grantTime;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public int Count
        {
            get
            {
                return this.lockCount;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public bool IsUpgraded
        {
            get
            {
                return this.isUpgraded;
            }
        }

        #endregion

        /// <summary>
        /// Expires this lock control block.
        /// </summary>
        internal void StartExpire()
        {
            if (System.Threading.Timeout.Infinite != this.lockTimeout)
            {
                AppTrace.TraceSource.WriteNoise("LockControlBlock.StartExpire", "{0}", this.ToString());
                this.timerExpiredLockRequest = new Timer(this.Expire, null, this.lockTimeout, System.Threading.Timeout.Infinite);
            }
        }

        /// <summary>
        /// If the lock contorl block has expired, call the lock manager to complete the pending lock request.
        /// </summary>
        void Expire(object state)
        {
            AppTrace.TraceSource.WriteNoise("LockControlBlock.Expire", "{0}", this.ToString());
            if (this.lockManager.ExpireLock(this))
            {
                this.Dispose();
            }
        }

        /// <summary>
        /// Terminates the expiration timer.
        /// </summary>
        internal bool StopExpire()
        {
            bool success = true;
            if (System.Threading.Timeout.Infinite != this.lockTimeout && null != this.timerExpiredLockRequest)
            {
                success = this.timerExpiredLockRequest.Change(System.Threading.Timeout.Infinite, System.Threading.Timeout.Infinite);
                AppTrace.TraceSource.WriteNoise("LockControlBlock.StopExpire", "{0}", this.ToString());
            }
            return success;
        }

        /// <summary>
        /// Override from object.
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Owner={0}, Resource={1}, Mode={2}, Status={3}, Count={4}, Upgrade={5})", 
                this.lockOwner, 
                this.lockResourceName, 
                this.lockMode, 
                this.lockStatus, 
                this.lockCount,
                this.isUpgraded);
        }
    }
}