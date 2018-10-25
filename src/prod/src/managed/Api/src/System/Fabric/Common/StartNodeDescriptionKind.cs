// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Interop;

    /// <summary>
    /// This is an enum used to indicate what overload of the RestartNode API is being used 
    /// </summary>
    /// <remarks>
    /// The values indicate whether RestartNode .API is being called using NodeName etc. or using ReplicaSelector etc.
    /// </remarks>
    [Serializable]
    internal enum StartNodeDescriptionKind
    {
        Invalid = NativeTypes.FABRIC_START_NODE_DESCRIPTION_KIND.FABRIC_START_NODE_DESCRIPTION_KIND_INVALID,
        /// <summary>
        /// Do not verify the completion of the action.
        /// </summary>
        UsingNodeName = NativeTypes.FABRIC_START_NODE_DESCRIPTION_KIND.FABRIC_START_NODE_DESCRIPTION_KIND_USING_NODE_NAME,
        /// <summary>
        /// Verify the completion of the action.
        /// </summary>
        UsingReplicaSelector = NativeTypes.FABRIC_START_NODE_DESCRIPTION_KIND.FABRIC_START_NODE_DESCRIPTION_KIND_USING_REPLICA_SELECTOR
    }
}