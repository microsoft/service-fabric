// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Notifications
{
    using System;

    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Event arguments for transactions.
    /// </summary>
    public class NotifyTransactionChangedEventArgs : EventArgs
    {
        private readonly NotifyTransactionChangedAction action;
        private readonly ITransaction transaction;

        /// <summary>
        /// Initializes a new instance of the <cref name="NotifyStateManagerSingleEntityChangedEventArgs"/>
        /// </summary>
        /// <param name="transaction">Transaction that the change is related to.</param>
        /// <param name="action">The type of notification.</param>
        public NotifyTransactionChangedEventArgs(ITransaction transaction, NotifyTransactionChangedAction action)
        {
            this.action = action;
            this.transaction = transaction;
        }

        /// <summary>
        /// Type of action for which the event was created.
        /// </summary>
        /// <value>
        /// The type of notification.
        /// </value>
        public NotifyTransactionChangedAction Action
        {
            get
            {
                return this.action;
            }
        }

        /// <summary>
        /// Gets the transaction.
        /// </summary>
        /// <value>
        /// The transaction associated with the operation.
        /// </value>
        public ITransaction Transaction
        {
            get
            {
                return this.transaction;
            }
        }
    }
}