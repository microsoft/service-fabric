// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Numerics;

    /// <summary>
    /// Describes information about how a node should be started using StartNodeTransitionAsync().  
    /// </summary>
    public sealed class NodeStartDescription : NodeTransitionDescription
    {
        /// <summary>
        /// Create an instance of NodeStartDescription, which describes information about how a node should be started using StartNodeTransitionAsync().  
        /// </summary>
        /// <param name="operationId">A Guid to identify this operation.  This should be unique, and should not be used with other operations.</param>        
        /// <param name="nodeName">The name of the node to start or stop.  The name can be determined through GetNodeListAsync().</param>
        /// <param name="nodeInstanceId">The node instance id of the target node.  This can be determined through GetNodeListAsync().</param>
        public NodeStartDescription(Guid operationId, string nodeName, BigInteger nodeInstanceId)
            : base(NodeTransitionType.Start, operationId, nodeName, nodeInstanceId)
        {
        }

        internal static new unsafe NodeStartDescription CreateFromNative(IntPtr nativeRaw)
        {
            ReleaseAssert.AssertIfNot(nativeRaw != IntPtr.Zero, StringResources.Error_NullNativePointer);

            NativeTypes.FABRIC_NODE_START_DESCRIPTION native = *(NativeTypes.FABRIC_NODE_START_DESCRIPTION*)nativeRaw;

            Guid operationId = native.OperationId;
            string nodeName = NativeTypes.FromNativeString(native.NodeName);
            BigInteger nodeInstanceId = new BigInteger(native.NodeInstanceId);

            NodeStartDescription description = new NodeStartDescription(operationId, nodeName, nodeInstanceId);
            return description;
        }

        internal override IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_NODE_TRANSITION_TYPE nodeTransitionType)
        {
            var nativeDescription = new NativeTypes.FABRIC_NODE_START_DESCRIPTION();
            nativeDescription.OperationId = this.OperationId;
            nativeDescription.NodeName = pin.AddBlittable(this.NodeName);
            nativeDescription.NodeInstanceId = (ulong)this.NodeInstanceId;

            nativeDescription.Reserved = IntPtr.Zero;

            nodeTransitionType = NativeTypes.FABRIC_NODE_TRANSITION_TYPE.FABRIC_NODE_TRANSITION_TYPE_START;
            return pin.AddBlittable(nativeDescription);
        }
    }
}
