// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Threading.Tasks;

    /// <summary>
    /// Manages a collection of locks.
    /// </summary>
    internal interface ILockContainer
    {
        /// <summary>
        /// Lock request made a spart of this transaction.
        /// </summary>
        /// <param name="lockManager">Lock manager owning the resource.</param>
        /// <param name="lockResourceNameHash">Logical resource name hash to lock.</param>
        /// <param name="mode">Lock mode for this request.</param>
        /// <param name="lockTimeout">Lock timeout for this request.</param>
        /// <returns></returns>
        Task<ILock> LockAsync(LockManager lockManager, ulong lockResourceNameHash, LockMode mode, int lockTimeout);

        /// <summary>
        /// Clear all transaction locks.
        /// </summary>
        void ClearLocks();
    }
}