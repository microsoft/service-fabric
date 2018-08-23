// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Notifications
{
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Provides data for the StateManagerChanged event caused by a rebuild.
    /// Commonly called during recovery, restore and end of copy.
    /// </summary>
    public class NotifyStateManagerRebuildEventArgs : NotifyStateManagerChangedEventArgs
    {
        /// <summary>
        /// The state providers.
        /// </summary>
        private readonly IAsyncEnumerable<IReliableState> reliableStates;

        /// <summary>
        /// Initializes a new instance of the <cref name="NotifyStateManagerChangedEventArgs"/>
        /// </summary>
        /// <param name="reliableStates">
        /// <cref name="IAsyncEnumerable"/> of all the ReliableState after the rebuild.
        /// </param>
        public NotifyStateManagerRebuildEventArgs(IAsyncEnumerable<IReliableState> reliableStates) : base(NotifyStateManagerChangedAction.Rebuild)
        {
            this.reliableStates = reliableStates;
        }

        /// <summary>
        /// Enumerable of all the new state providers now in the State Manager.
        /// </summary>
        /// <value>
        /// Asynchronous enumerable that contains the new set of <cref name="IReliableState"/>s.
        /// </value>
        public IAsyncEnumerable<IReliableState> ReliableStates
        {
            get
            {
                return this.reliableStates;
            }
        }
    }
}