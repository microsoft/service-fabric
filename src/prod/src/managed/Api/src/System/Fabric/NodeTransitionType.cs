// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes the type of node transition</para>
    /// </summary>    
    public enum NodeTransitionType
    {
        /// <summary>
        /// <para>Indicates that the node transition type is invalid. This value is not used.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_NODE_TRANSITION_TYPE.FABRIC_NODE_TRANSITION_TYPE_INVALID,

        /// <summary>
        /// <para>Indicates that the node should be started.</para>
        /// </summary>        
        Start = NativeTypes.FABRIC_NODE_TRANSITION_TYPE.FABRIC_NODE_TRANSITION_TYPE_START,

        /// <summary>
        /// <para>Indicates that the node should be stopped.</para>
        /// </summary>        
        Stop = NativeTypes.FABRIC_NODE_TRANSITION_TYPE.FABRIC_NODE_TRANSITION_TYPE_STOP
    }
}