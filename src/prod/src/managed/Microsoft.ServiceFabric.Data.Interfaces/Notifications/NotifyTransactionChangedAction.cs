// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Notifications
{
    /// <summary>
    /// Describes the action that caused the TransactionChanged event.
    /// </summary>
    public enum NotifyTransactionChangedAction : int
    {
        /// <summary> 
        /// Transaction has committed.
        /// </summary>
        Commit = 0,
    }
}