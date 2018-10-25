// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Result
{
    using System.Fabric.Common;
    using System.Numerics;
    using System.Fabric.Interop;

    /// <summary>
    /// Returns Node result object. 
    /// </summary>
    /// <remarks>
    /// This class returns nodeName and nodeInstanceId. 
    /// This class is part of RestartNode, StartNode, StopNode actions result structure.
    /// </remarks>
    [Serializable]
    public class NodeResult
    {
        // Needed for json deserialization of NodeTransitionProgress, which contains this
        internal NodeResult()
        { }

        /// <summary>
        /// Node result constructor.
        /// </summary>
        /// <param name="nodeName">node name</param>
        /// <param name="nodeInstance">node instance id</param>
        internal NodeResult(string nodeName, BigInteger nodeInstance)
        {
            ReleaseAssert.AssertIf(string.IsNullOrEmpty(nodeName), "Node name cannot be null or empty");
            this.NodeName = nodeName;
            this.NodeInstance = nodeInstance;
        }

        /// <summary>
        /// Gets node name.
        /// </summary>
        /// <value>The node name.</value>
        public string NodeName { get; private set; }

        /// <summary>
        /// Gets node instance id.
        /// </summary>
        /// <value>The node instance id.</value>
        public BigInteger NodeInstance { get; private set; }

        /// <summary>
        /// Returns a string like: "NodeName: string, NodeInstance: BigInteger"
        /// </summary>
        /// <returns>A string</returns>
        public override string ToString()
        {
            return string.Format("NodeName: {0}, NodeInstance: {1}", this.NodeName, this.NodeInstance);
        }

        internal unsafe static NodeResult CreateFromNative(IntPtr nativeNodeResult)
        {
            NativeTypes.FABRIC_NODE_RESULT nodeResult = *(NativeTypes.FABRIC_NODE_RESULT*)nativeNodeResult;

            string nodeName = NativeTypes.FromNativeString(nodeResult.NodeName);

            BigInteger nodeInstance = new BigInteger(nodeResult.NodeInstance);

            return new NodeResult(nodeName, nodeInstance);
        }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeNodeResult = new NativeTypes.FABRIC_NODE_RESULT();

            nativeNodeResult.NodeName = pin.AddObject(this.NodeName);

            nativeNodeResult.NodeInstance = (ulong)this.NodeInstance;

            return pin.AddBlittable(nativeNodeResult);
        }
    }
}