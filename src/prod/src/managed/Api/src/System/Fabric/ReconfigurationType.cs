// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// Represents replica reconfiguration type.
    /// </summary>
    public enum ReconfigurationType
    {
        /// <summary>
        /// Invalid reconfiguration type.
        /// </summary>
        Unknown,

        /// <summary>
        /// Reconfiguration triggered to promote an active secondary replica to the primary role and demote existing primary replica to secondary role.
        /// </summary>
        SwapPrimary,

        /// <summary>
        /// Reconfiguration triggered in response to a primary going down. This could be due to many reasons such as primary replica crashing etc
        /// </summary>
        Failover,

        /// <summary>
        /// Reconfigurations where no change is happening to the primary.
        /// </summary>
        Other,

        /// <summary>
        /// No reconfiguration is taking place currently.
        /// </summary>
        None,
    }
}