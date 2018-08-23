// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// Lock Manager.
    /// </summary>
    public interface ILockManager
    {
        /// <summary>
        /// Releases locks.
        /// </summary>
        /// <param name="lockContext">contex associated with the lock.</param>
        void Unlock(LockContext lockContext);
    }
}