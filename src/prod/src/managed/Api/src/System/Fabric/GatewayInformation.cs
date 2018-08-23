// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;
    using System.Numerics;

    /// <summary>
    /// <para>Contains information identifying a Service Fabric node in the cluster.</para>
    /// </summary>
    public sealed class GatewayInformation
    {
        internal GatewayInformation()
        {
        }

        /// <summary>
        /// <para>Gets the address that Service Fabric clients use when connecting to this node (as specified in the Cluster Manifest).</para>
        /// </summary>
        /// <value>
        /// <para>The address that Service Fabric clients use when connecting to this node (as specified in the Cluster Manifest).</para>
        /// </value>
        public string NodeAddress { get; private set; }

        /// <summary>
        /// <para>The unique identifier used internally by Service Fabric to identify a node. </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.NodeId" />.</para>
        /// </value>
        public NodeId NodeId { get; private set; }
        
        /// <summary>
        /// <para>The instance of a Service Fabric node changes when the node is restarted.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Numerics.BigInteger" />.</para>
        /// </value>
        public BigInteger NodeInstanceId { get; private set; }

        /// <summary>
        /// <para>The friendly name of the Service Fabric node (defined in the Cluster Manifest) used to generate the NodeId.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string NodeName { get; private set; }

        internal static unsafe GatewayInformation FromNative(NativeClient.IFabricGatewayInformationResult nativeResult)
        {
            var nativeInfo = (NativeTypes.FABRIC_GATEWAY_INFORMATION*)nativeResult.get_GatewayInformation();
            var managedInfo = new GatewayInformation();

            managedInfo.NodeAddress = NativeTypes.FromNativeString(nativeInfo->NodeAddress);

            var nativeNodeId = (NativeTypes.FABRIC_NODE_ID)nativeInfo->NodeId;
            managedInfo.NodeId = new NodeId(nativeNodeId.High, nativeNodeId.Low);
            managedInfo.NodeInstanceId = new BigInteger(nativeInfo->NodeInstanceId);

            managedInfo.NodeName = NativeTypes.FromNativeString(nativeInfo->NodeName);

            GC.KeepAlive(nativeResult);

            return managedInfo;
        }
    }
}