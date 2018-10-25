// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Common;
    using System.Numerics;
    using System.Diagnostics.CodeAnalysis;
 
    /// <summary>
    /// <para>Specifies contextual information about a Service Fabric node such as node name, ID, node type etc.</para>
    /// </summary>
    public class NodeContext
    {
        private readonly NativeRuntime.IFabricNodeContextResult2 nativeNodeContext;

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.NodeContext"/> class.
        /// </summary>
        /// <param name="nodeName">The node name.</param>
        /// <param name="nodeId">The node id.</param>
        /// <param name="nodeInstanceId">The node instance id.</param>
        /// <param name="nodeType">The node type.</param>
        /// <param name="ipAddressOrFQDN">The IP address or FQDN of the node.</param>
        public NodeContext(
            string nodeName,
            NodeId nodeId,
            BigInteger nodeInstanceId,
            string nodeType,
            // ReSharper disable once InconsistentNaming
            string ipAddressOrFQDN)
        {
            nodeName.ThrowIfNull("nodeName");
            nodeId.ThrowIfNull("nodeId");
            nodeType.ThrowIfNull("nodeType");
            ipAddressOrFQDN.ThrowIfNull("ipAddressOrFQDN");

            this.NodeName = nodeName;
            this.NodeId = nodeId;
            this.NodeInstanceId = nodeInstanceId;
            this.NodeType = nodeType;
            this.IPAddressOrFQDN = ipAddressOrFQDN;
        }


        internal NodeContext(NativeRuntime.IFabricNodeContextResult2 nativeNodeContext)
        {
            Requires.Argument("nativeNodeContext", nativeNodeContext).NotNull();

            this.nativeNodeContext = nativeNodeContext;       
        }

        /// <summary>
        /// <para>Gets the node ID.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.NodeId" />.</para>
        /// </value>
        public NodeId NodeId { get; private set; }
        
        /// <summary>
        /// <para>Gets the node instance ID, which uniquely identifies the node.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Numerics.BigInteger" />.</para>
        /// </value>
        public BigInteger NodeInstanceId { get; private set; }

        /// <summary>
        /// <para>Gets the node name.</para>
        /// </summary>
        /// <value>
        /// <para>Returns the node name.</para>
        /// </value>
        public string NodeName { get; private set; }

        /// <summary>
        /// <para>Gets the node type.</para>
        /// </summary>
        /// <value>
        /// <para>Returns the node type string.</para>
        /// </value>
        public string NodeType { get; private set; }

        // ReSharper disable once InconsistentNaming
        /// <summary>
        /// <para>Gets the IP address or the fully qualified domain name of the node.</para>
        /// </summary>
        /// <value>
        /// <para>Returns the IP address or the fully qualified domain name of the node.</para>
        /// </value>
        public string IPAddressOrFQDN { get; private set; }

        /// <summary>
        /// Retrieves the directory path for the directory at node level.
        /// </summary>
        public string GetDirectory(string logicalDirectoryName)
        {
            Requires.Argument("logicalDirectoryName", logicalDirectoryName).NotNull();
            return Utility.WrapNativeSyncInvoke(() => this.GetDirectoryHelper(logicalDirectoryName), "NodeContext.GetNodeDirectory");
        }

        private string GetDirectoryHelper(string logicalDirectoryName)
        {
            using (var pin = new PinBlittable(logicalDirectoryName))
            {
                return StringResult.FromNative(nativeNodeContext.GetDirectory(pin.AddrOfPinnedObject()));
            }
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static NodeContext CreateFromNative(NativeRuntime.IFabricNodeContextResult2 nodeContextResult)
        {
            NodeContext nd =  FromNative(nodeContextResult);
            GC.KeepAlive(nodeContextResult);
            return nd;
        }

        internal static unsafe NodeContext FromNative(NativeRuntime.IFabricNodeContextResult2 nodeContextResult)
        {
            var nodeContext = nodeContextResult.get_NodeContext();

            NativeTypes.FABRIC_NODE_CONTEXT* nativeNodeContext = (NativeTypes.FABRIC_NODE_CONTEXT*)nodeContext;
            NativeTypes.FABRIC_NODE_ID nativenodeId = (NativeTypes.FABRIC_NODE_ID)nativeNodeContext->NodeId;
            NodeId nodeId = new NodeId(nativenodeId.High, nativenodeId.Low);
            string nodeName = NativeTypes.FromNativeString(nativeNodeContext->NodeName);
            BigInteger nodeInstanceId = new BigInteger(nativeNodeContext->NodeInstanceId);
            string nodeType = NativeTypes.FromNativeString(nativeNodeContext->NodeType);
            string ipAddressOrFQDN = NativeTypes.FromNativeString(nativeNodeContext->IPAddressOrFQDN);

            return new NodeContext(nodeContextResult)
            {
                NodeId = nodeId,
                NodeName = nodeName,
                NodeInstanceId = nodeInstanceId,
                NodeType = nodeType,
                IPAddressOrFQDN = ipAddressOrFQDN
            };
        }
    }
}