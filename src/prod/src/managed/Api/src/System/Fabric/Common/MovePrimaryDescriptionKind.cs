// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Interop;

    /// <summary>
    /// This is an enum used to indicate what overload of the MovePrimary API is being used 
    /// </summary>
    /// <remarks>
    /// The values indicate whether MovePrimary .API is being called using NodeName etc. or using ReplicaSelector etc.
    /// </remarks>
    [Serializable]
    internal enum MovePrimaryDescriptionKind
    {
        Invalid = NativeTypes.FABRIC_MOVE_PRIMARY_DESCRIPTION_KIND.FABRIC_MOVE_PRIMARY_DESCRIPTION_KIND_INVALID,
        /// <summary>
        /// Do not verify the completion of the action.
        /// </summary>
        UsingNodeName = NativeTypes.FABRIC_MOVE_PRIMARY_DESCRIPTION_KIND.FABRIC_MOVE_PRIMARY_DESCRIPTION_KIND_USING_NODE_NAME,
        /// <summary>
        /// Verify the completion of the action.
        /// </summary>
        UsingReplicaSelector = NativeTypes.FABRIC_MOVE_PRIMARY_DESCRIPTION_KIND.FABRIC_MOVE_PRIMARY_DESCRIPTION_KIND_USING_REPLICA_SELECTOR
    }
}