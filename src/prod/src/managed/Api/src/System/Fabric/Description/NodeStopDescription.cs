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
    /// Describes information about how a node should be stopped using StartNodeTransitionAsync().  
    /// </summary>
    public sealed class NodeStopDescription : NodeTransitionDescription
    {
        private readonly int stopDurationInSeconds;

        /// <summary>
        /// Constructs a NodeTransitionDescription, which describes information about a node to stop.
        /// </summary>        
        /// <param name="operationId">A Guid to identify this operation.  This should be unique, and should not be used with other operations.</param>        
        /// <param name="nodeName">The name of the node to start or stop.  The name can be determined through GetNodeListAsync().</param>
        /// <param name="nodeInstanceId">The node instance id of the target node.  This can be determined through GetNodeListAsync().</param>
        /// <param name="stopDurationInSeconds">The duration to keep the node stopped.  After this time expires, the node will automatically come back up.  The units is in seconds.  The minimum value is 600, the maximum is 14400.</param>
        public NodeStopDescription(Guid operationId, string nodeName, BigInteger nodeInstanceId, int stopDurationInSeconds)
            : base(NodeTransitionType.Stop, operationId, nodeName, nodeInstanceId)
        {
            this.stopDurationInSeconds = stopDurationInSeconds;
        }

        /// <summary>
        /// The duration to keep the node stopped.  After this time expires, the node will automatically come back up.  The units is in seconds.  The minimum value is 600, the maximum is 14400.
        /// </summary>
        public int StopDurationInSeconds
        {
            get
            {
                return this.stopDurationInSeconds;
            }           
        }

        /// <summary>
        /// Prints a string representation of the object.
        /// </summary>        
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "{0} Duration:{1}", base.ToString(), this.StopDurationInSeconds);
        }

        internal static new unsafe NodeStopDescription CreateFromNative(IntPtr nativeRaw)
        {
            ReleaseAssert.AssertIfNot(nativeRaw != IntPtr.Zero, StringResources.Error_NullNativePointer);

            NativeTypes.FABRIC_NODE_STOP_DESCRIPTION native = *(NativeTypes.FABRIC_NODE_STOP_DESCRIPTION*)nativeRaw;

            Guid operationId = native.OperationId;
            string nodeName = NativeTypes.FromNativeString(native.NodeName);
            BigInteger nodeInstanceId = new BigInteger(native.NodeInstanceId);

            uint durationInSeconds = native.StopDurationInSeconds;
            TimeSpan duration = TimeSpan.FromSeconds(durationInSeconds);

            NodeStopDescription description = new NodeStopDescription(operationId, nodeName, nodeInstanceId, (int)duration.TotalSeconds);

            return description;
        }

        internal override IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_NODE_TRANSITION_TYPE nodeTransitionType)
        {
            var nativeDescription = new NativeTypes.FABRIC_NODE_STOP_DESCRIPTION();
            nativeDescription.OperationId = this.OperationId;

            nativeDescription.NodeName = pin.AddBlittable(this.NodeName);
            nativeDescription.NodeInstanceId = (ulong)this.NodeInstanceId;
            nativeDescription.StopDurationInSeconds = (uint)this.StopDurationInSeconds;

            nativeDescription.Reserved = IntPtr.Zero;

            nodeTransitionType = NativeTypes.FABRIC_NODE_TRANSITION_TYPE.FABRIC_NODE_TRANSITION_TYPE_STOP;
            return pin.AddBlittable(nativeDescription);
        }
    }
}
