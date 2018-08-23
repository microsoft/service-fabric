// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Notifications
{
    /// <summary>
    /// Describes the action that caused the StateManagerChanged event.
    /// </summary>
    public enum NotifyStateManagerChangedAction : int
    {
        /// <summary> 
        /// A state provider has been added.
        /// </summary>
        Add = 0,

        /// <summary> 
        /// A state provider has been removed.
        /// </summary>
        Remove = 1,

        /// <summary> 
        /// State manager has been rebuilt.
        /// </summary>
        Rebuild = 2,
    }
}