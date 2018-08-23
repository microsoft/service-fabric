// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Notifications
{
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Provides data for the DictionaryChanged event caused by a transactional single entity operation.
    /// </summary>
    public class NotifyStateManagerSingleEntityChangedEventArgs : NotifyStateManagerChangedEventArgs
    {
        private readonly ITransaction transaction;
        private readonly IReliableState reliableState;

        /// <summary>
        /// Initializes a new instance of the <cref name="NotifyStateManagerSingleEntityChangedEventArgs"/>
        /// </summary>
        /// <param name="transaction">Transaction that the change is related to.</param>
        /// <param name="reliableState"><cref name="IReliableState"/> that was changed.</param>
        /// <param name="action">The type of the change.</param>
        public NotifyStateManagerSingleEntityChangedEventArgs(
            ITransaction transaction,
            IReliableState reliableState,
            NotifyStateManagerChangedAction action) : base(action)
        {
            this.transaction = transaction;
            this.reliableState = reliableState;
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

        /// <summary>
        /// Gets the reliable state
        /// </summary>
        /// <value>
        /// The <cref name="IReliableState"/> associated with the notification.
        /// </value>
        public IReliableState ReliableState
        {
            get
            {
                return this.reliableState;
            }
        }
    }
}