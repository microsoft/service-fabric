// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Notifications
{
    using System;

    /// <summary>
    /// Provides data for the StateManagerChanged event.
    /// </summary>
    public abstract class NotifyStateManagerChangedEventArgs : EventArgs
    {
        /// <summary>
        /// The action.
        /// </summary>
        private readonly NotifyStateManagerChangedAction action;

        /// <summary>
        /// Initializes a new instance of the <cref name="NotifyStateManagerChangedEventArgs"/>
        /// </summary>
        /// <param name="action">Type of the event.</param>
        public NotifyStateManagerChangedEventArgs(NotifyStateManagerChangedAction action)
        {
            this.action = action;
        }

        /// <summary>
        /// Gets the action that caused the event.
        /// </summary>
        /// <value>
        /// The type of notification.
        /// </value>
        public NotifyStateManagerChangedAction Action
        {
            get
            {
                return this.action;
            }
        }
    }
}