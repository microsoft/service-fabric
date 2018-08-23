// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// This is for internal use only.
    /// Context for the lock.
    /// </summary>
    public abstract class LockContext
    {
        /// <summary>
        /// lock manager.
        /// </summary>
        private readonly ILockManager lockManager;

        /// <summary>
        /// Indicates if the context is being tracked or not.
        /// </summary>
        private bool isTracking;

        /// <summary>
        ///  Initializes a new instance of the <see cref="LockContext"/> class. 
        /// </summary>
        protected LockContext()
        {
        }

        /// <summary>
        ///  Initializes a new instance of the <see cref="LockContext"/> class. 
        /// </summary>
        /// <param name="lockManager">lock manager</param>
        protected LockContext(ILockManager lockManager)
        {
            this.lockManager = lockManager;
            this.isTracking = false;
        }

        /// <summary>
        /// Gets or sets a value indicating tracking context.
        /// </summary>
        public bool IsTrackingContext
        {
            get { return this.isTracking; }

            set { this.isTracking = value; }
        }

        /// <summary>
        /// Gets lock manager.
        /// </summary>
        internal ILockManager LockManager
        {
            get { return this.lockManager; }
        }

        /// <summary>
        /// Releases locks.
        /// </summary>
        public virtual void Unlock(LockContextMode mode)
        {
            this.lockManager.Unlock(this);
        }

        /// <summary>
        /// Gets a value that indicates whether the current lock manager instance is the same as the other lock manager.
        /// </summary>
        internal bool IsEqual(ILockManager inputLockManager, object stateOrKey)
        {
            if (this.lockManager == inputLockManager)
            {
                return this.IsEqual(stateOrKey);
            }

            return false;
        }

        /// <summary>
        /// Gets a value that indicates if the states are equal.
        /// </summary>
        /// <param name="stateOrKey"></param>
        /// <returns></returns>
        protected abstract bool IsEqual(object stateOrKey);
    }
}