// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    /// <summary>
    /// Specifies how reliable collections will lock resources, which determines
    /// how the resources can be accessed by concurrent transactions.
    /// </summary>
    public enum LockMode : int
    {
        /// <summary>
        /// Use the default lock mode based on the operation and isolation level of the transaction.
        /// </summary>
        Default = 0,

        /// <summary>
        /// Used on resources that are intended to be updated. Prevents a common form of deadlock
        /// that occurs when multiple transactions are reading, locking, and potentially
        /// updating resources later.
        /// </summary>
        Update = 1,
    }
}