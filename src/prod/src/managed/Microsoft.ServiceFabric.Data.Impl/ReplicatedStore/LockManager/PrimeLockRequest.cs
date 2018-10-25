// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Contains lock manager and lock mode associated with a prime lock.
    /// </summary>
    internal struct PrimeLockRequest
    {
        private LockManager lockManager;
        private LockMode lockMode;

        /// <summary>
        ///  Initializes a new instance of the <see cref="PrimeLockRequest"/> class. 
        /// </summary>
        /// <param name="lockManager"></param>
        /// <param name="lockMode"></param>
        public PrimeLockRequest(LockManager lockManager, LockMode lockMode)
        {
            this.lockManager = lockManager;
            this.lockMode = lockMode;
        }

        /// <summary>
        /// Lock manager associated with the lock request.
        /// </summary>
        public LockManager LockManager
        {
            get { return this.lockManager; }
        }

        /// <summary>
        /// Lock mode for the lock request.
        /// </summary>
        public LockMode LockMode
        {
            get { return this.lockMode; }
        }
    }
}