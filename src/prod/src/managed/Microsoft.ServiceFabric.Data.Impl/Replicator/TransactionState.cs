// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// Transaction State
    /// </summary>
    public enum TransactionState : int
    {
        /// <summary>
        /// Active
        /// </summary>
        Active = 0,
            
        /// <summary>
        /// A read is ongoing
        /// </summary>
        Reading = 1,
        
        /// <summary>
        /// Transaction is being committed
        /// </summary>
        Committing = 2,

        /// <summary>
        /// Transaction is being aborted
        /// </summary>
        Aborting = 3,

        /// <summary>
        /// Transaction is committed
        /// </summary>
        Committed = 4,

        /// <summary>
        /// Transaction is aborted
        /// </summary>
        Aborted = 5,
        
        /// <summary>
        /// Transaction failed to commit/abort due to an exception
        /// </summary>
        Faulted = 6
    }
}