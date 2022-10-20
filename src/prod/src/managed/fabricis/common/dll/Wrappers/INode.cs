// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Query;

    /// <summary>
    /// Wrapper for a <see cref="Node"/> for unit-testability.
    /// </summary>
    internal interface INode
    {
        string NodeName { get; }

        /// <summary>
        /// The timestamp at which the node was last up. 
        /// <c>null</c> means that the node is down (or was down when this object was created).
        /// </summary>
        DateTimeOffset? NodeUpTimestamp { get; }        
    }
}