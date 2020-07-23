// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Returned as part of lock acquisition requests.
    /// </summary>
    internal interface ILock
    {
        /// <summary>
        /// The lock owner for this lock.
        /// </summary>
        long Owner { get; }

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
        long GrantedTime { get; }

        /// <summary>
        /// Lock reference count.
        /// </summary>
        int Count { get; }

        /// <summary>
        /// Is this an upgrade lock request.
        /// </summary>
        bool IsUpgraded { get; }

        /// <summary>
        /// Lock manager that created this lock.
        /// </summary>
        LockManager LockManager { get; }

        /// <summary>
        /// The oldest grantee of the resource at the time this lock was expired
        /// If not expired, then this is -1
        /// </summary>
        long OldestGrantee { get; }
    }
}