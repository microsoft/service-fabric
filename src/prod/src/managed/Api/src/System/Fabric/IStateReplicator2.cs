// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>Exposes replication-related functions of the <see cref="System.Fabric.FabricReplicator" /> class that are used by <see cref="System.Fabric.IStateProvider" /> to replicate state to ensure high availability.</para>
    /// </summary>
    public interface IStateReplicator2 : IStateReplicator
    {
        /// <summary>
        /// <para>Retrieves the replicator settings during runtime.</para>
        /// </summary>
        /// <returns>
        /// <para>The current <see cref="System.Fabric.ReplicatorSettings" /> from the Service Fabric runtime.</para>
        /// </returns>
        ReplicatorSettings GetReplicatorSettings();
    }
}