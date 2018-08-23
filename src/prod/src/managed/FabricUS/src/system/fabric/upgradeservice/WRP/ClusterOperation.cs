// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.WRP.Model
{
    /// <summary>
    /// <para>Cluster operations</para>
    /// 
    /// <para>
    /// Note: Please preserve the order and to avoid serialization issues in the communication with SFRP
    /// </para>
    /// </summary>
    public enum ClusterOperation
    {
        ProcessClusterUpgrade,
        EnableNodes,
        DisableNodes,
        UpdateSystemServicesReplicaSetSize,
        ProcessNodesStatus
    }
}