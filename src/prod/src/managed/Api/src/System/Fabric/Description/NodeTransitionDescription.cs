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
    using System.Globalization;
    using System.Numerics;

    /// <summary>
    /// Describes information about how a node should be transitioned, and is used with StartNodeTransitionAsync().  
    /// This object should not be used directly, the derived classes should be used.
    /// </summary>
    public abstract class NodeTransitionDescription
    {
        private readonly NodeTransitionType nodeTransitionType;
        private readonly Guid operationId;
        private readonly string nodeName;
        private readonly BigInteger nodeInstanceId;

        /// <summary>
        /// Constructs a NodeTransitionDescription
        /// </summary>
        /// <param name="nodeTransitionType">Indicates the type of transition to perform.  NodeTransitionType.Start will start a stopped node.  NodeTransitionType.Stop will stop a node that is up.</param>
        /// <param name="operationId">A Guid to identify this operation.  This should be unique, and should not be used with other operations.</param>        
        /// <param name="nodeName">The name of the node to start or stop.  The name can be determined through GetNodeListAsync().</param>
        /// <param name="nodeInstanceId">The node instance id of the target node.  This can be determined through GetNodeListAsync().</param>
        protected NodeTransitionDescription(NodeTransitionType nodeTransitionType, Guid operationId, string nodeName, BigInteger nodeInstanceId)
        {
            this.nodeTransitionType = nodeTransitionType;
            this.operationId = operationId;
            this.nodeName = nodeName;
            this.nodeInstanceId = nodeInstanceId;
        }

        /// <summary>
        /// NodeTransitionType determines what type of transition to perform.  Start will start a stopped node.  Stop will stop an up node.
        /// </summary>
        public NodeTransitionType NodeTransitionType
        {
            get
            {
                return nodeTransitionType;
            }
        }


        /// <summary>
        /// A Guid to identify this operation.  This should be unique, and should not be used with other operations.
        /// </summary>
        public Guid OperationId
        {
            get
            {
                return this.operationId;
            }
        }

        /// <summary>
        /// The name of the node to start or stop.  The name can be determined through GetNodeListAsync().
        /// </summary>
        public string NodeName
        {
            get
            {
                return this.nodeName;
            }
        }

        /// <summary>
        /// The node instance id of the target node.  This can be determined through GetNodeListAsync().
        /// </summary>
        public BigInteger NodeInstanceId
        {
            get 
            {
                return this.nodeInstanceId;
            }
        }

        /// <summary>
        /// Prints a string representation of this object.
        /// </summary>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "NodeTransitionType:{0} OperationId: {1}, NodeName: {2}, NodeInstanceId: {3}",
                this.NodeTransitionType,
                this.OperationId,
                this.NodeName,
                this.NodeInstanceId);
        }

        internal static unsafe NodeTransitionDescription CreateFromNative(IntPtr native)
        {
            ReleaseAssert.AssertIfNot(native != IntPtr.Zero, StringResources.Error_NullNativePointer);

            NativeTypes.FABRIC_NODE_TRANSITION_DESCRIPTION* casted = (NativeTypes.FABRIC_NODE_TRANSITION_DESCRIPTION*)native;
            switch (casted->NodeTransitionType)
            {
                case NativeTypes.FABRIC_NODE_TRANSITION_TYPE.FABRIC_NODE_TRANSITION_TYPE_START:
                    return NodeStartDescription.CreateFromNative(casted->Value);
                case NativeTypes.FABRIC_NODE_TRANSITION_TYPE.FABRIC_NODE_TRANSITION_TYPE_STOP:
                    return NodeStopDescription.CreateFromNative(casted->Value);
                default:
                    AppTrace.TraceSource.WriteError("NodeTransitionDescription.CreateFromNative", "Unknown node transition description type {0}", casted->NodeTransitionType);

                    ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_UnknownNodeTransitionType, casted->NodeTransitionType));
                    break;
            }

            return null;
        }

        internal unsafe IntPtr ToNative(PinCollection pin)
        {
            var nativeDescription = new NativeTypes.FABRIC_NODE_TRANSITION_DESCRIPTION[1];

            NativeTypes.FABRIC_NODE_TRANSITION_TYPE nodeTransitionType;
            nativeDescription[0].Value = this.ToNative(pin, out nodeTransitionType);
            nativeDescription[0].NodeTransitionType = nodeTransitionType;

            return pin.AddBlittable(nativeDescription);
        }

        internal abstract IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_NODE_TRANSITION_TYPE nodeTransitionType);
    }
}
