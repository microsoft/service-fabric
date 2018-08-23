// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// Type of QuorumLoss that will be invoked.
    /// </summary>
    [Serializable]
    public enum QuorumLossMode
    {
        /// <summary>
        /// Reserved.  Do not pass into API.
        /// </summary>
        Invalid = NativeTypes.FABRIC_QUORUM_LOSS_MODE.FABRIC_QUORUM_LOSS_MODE_INVALID,
        
        ///<summary>Partial Quorum loss mode : Minimum number of replicas for a partition will be down that will cause a quorum loss.</summary>
        QuorumReplicas = NativeTypes.FABRIC_QUORUM_LOSS_MODE.FABRIC_QUORUM_LOSS_MODE_QUORUM_REPLICAS,
        
        ///<summary>Full Quorum loss mode : All replicas for a partition will be down that will cause a quorum loss. </summary>
        AllReplicas = NativeTypes.FABRIC_QUORUM_LOSS_MODE.FABRIC_QUORUM_LOSS_MODE_ALL_REPLICAS
    }
}