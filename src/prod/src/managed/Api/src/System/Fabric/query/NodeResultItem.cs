// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Numerics;

    /// <summary>
    /// <para>Represents a Service Fabric cluster node.</para>
    /// </summary>
    public sealed class Node
    {
        internal Node(
            string nodeName,
            string ipAddressOrFqdn,
            string nodeType,
            string codeVersion,
            string configVersion,
            NodeStatus nodeStatus,
            TimeSpan nodeUpTime,
            TimeSpan nodeDownTime,
            DateTime nodeUpAt,
            DateTime nodeDownAt,
            HealthState healthState,
            bool isSeedNode,
            string upgradeDomain,
            Uri faultDomain,
            NodeId nodeId,
            BigInteger nodeInstanceId,
            NodeDeactivationResult nodeDeactivationInfo)
            :
         this(
             nodeName,
             ipAddressOrFqdn,
             nodeType,
             codeVersion,
             configVersion,
             nodeStatus,
             nodeUpTime,
             nodeDownTime,
             nodeUpAt,
             nodeDownAt,
             healthState,
             isSeedNode,
             upgradeDomain,
             faultDomain,
             nodeId,
             nodeInstanceId,
             nodeDeactivationInfo,
             false)
        { }

        internal Node(
            string nodeName,
            string ipAddressOrFqdn,
            string nodeType,
            string codeVersion,
            string configVersion,
            NodeStatus nodeStatus,
            TimeSpan nodeUpTime,
            TimeSpan nodeDownTime,
            DateTime nodeUpAt,
            DateTime nodeDownAt,
            HealthState healthState,
            bool isSeedNode,
            string upgradeDomain,
            Uri faultDomain,
            NodeId nodeId,
            BigInteger nodeInstanceId,
            NodeDeactivationResult nodeDeactivationInfo,
            bool isStopped)
        {
            this.NodeName = nodeName;
            this.IpAddressOrFQDN = ipAddressOrFqdn;
            this.NodeType = nodeType;
            this.CodeVersion = codeVersion;
            this.ConfigVersion = configVersion;
            this.NodeStatus = nodeStatus;

#pragma warning disable CS0618
            this.NodeUpTime = nodeUpTime;
            this.NodeDownTime = nodeDownTime;
#pragma warning restore CS0618

            this.NodeUpAt = nodeUpAt;
            this.NodeDownAt = nodeDownAt;
            this.HealthState = healthState;
            this.IsSeedNode = isSeedNode;
            this.UpgradeDomain = upgradeDomain;
            this.FaultDomain = faultDomain;
            this.NodeId = nodeId;
            this.NodeInstanceId = nodeInstanceId;
            this.NodeDeactivationInfo = nodeDeactivationInfo;
            this.IsStopped = isStopped;
        }

        private Node() { }

        /// <summary>
        /// <para>Gets the name of the node.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the node.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public string NodeName { get; private set; }

        /// <summary>
        /// <para>Gets the IP address or the fully qualified domain name (FQDN) of the node.</para>
        /// </summary>
        /// <value>
        /// <para>The IP address or the fully qualified domain name (FQDN) of the node.</para>
        /// </value>
        public string IpAddressOrFQDN { get; private set; }

        /// <summary>
        /// <para>Gets the node type.</para>
        /// </summary>
        /// <value>
        /// <para>The node type.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Type)]
        public string NodeType { get; private set; }

        /// <summary>
        /// <para>Gets the Service Fabric runtime version running on the node.</para>
        /// </summary>
        /// <value>
        /// <para>The Service Fabric runtime version running on the node.</para>
        /// </value>
        public string CodeVersion { get; private set; }

        /// <summary>
        /// <para>Gets the cluster configuration version on the node.</para>
        /// </summary>
        /// <value>
        /// <para>The cluster configuration version on the node.</para>
        /// </value>
        public string ConfigVersion { get; private set; }

        /// <summary>
        /// <para>Gets the node status.</para>
        /// </summary>
        /// <value>
        /// <para>The node status.</para>
        /// </value>
        public NodeStatus NodeStatus { get; private set; }

        /// <summary>
        /// <para>Gets the node up time.</para>
        /// </summary>
        /// <value>
        /// <para>The node up time.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        [Obsolete("This property is deprecated, use NodeUpAt instead.", false)]
        public TimeSpan NodeUpTime { get; private set; }

        /// <summary>
        /// Used by the serializer
        /// </summary>
        private long NodeUpTimeInSeconds
        {
            get { return (long)(DateTime.Now - NodeUpAt).TotalSeconds; }
            set { this.NodeUpAt = DateTime.Now - TimeSpan.FromTicks(value * TimeSpan.TicksPerSecond); }
        }

        /// <summary>
        /// <para>Gets the node down time.</para>
        /// </summary>
        /// <value>
        /// <para>The node down time.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        [Obsolete("This property is deprecated, use NodeDownAt instead.", false)]
        public TimeSpan NodeDownTime { get; private set; }

        /// <summary>
        /// Used by the serializer
        /// </summary>
        private long NodeDownTimeInSeconds
        {
            get { return (long)(DateTime.Now - NodeDownAt).TotalSeconds; }
            set { this.NodeDownAt = DateTime.Now - TimeSpan.FromTicks(value * TimeSpan.TicksPerSecond); }
        }

        /// <summary>
        /// <para>Gets the date time when node status changed to up.</para>
        /// </summary>
        /// <value>
        /// <para>The date time when node status changed to up.</para>
        /// </value>
        public DateTime NodeUpAt { get; private set; }

        /// <summary>
        /// <para>Gets the date time when node status changed to down.</para>
        /// </summary>
        /// <value>
        /// <para>The date time when node status changed to down.</para>
        /// </value>
        public DateTime NodeDownAt { get; private set; }

        /// <summary>
        /// <para>Gets the health state of the node.</para>
        /// </summary>
        /// <value>
        /// <para>The health state of the node.</para>
        /// </value>
        public HealthState HealthState { get; private set; }

        /// <summary>
        /// <para>Gets a value indicating whether this is a seed node. Seed nodes are special type of node configured automatically and used internally by the system. </para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if this instance is a seed node; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsSeedNode { get; private set; }

        /// <summary>
        /// <para>Gets the upgrade domain value for this node. </para>
        /// <remarks>Upgrade domains define sets of nodes which are shut down for upgrades at approximately the same time.</remarks>
        /// </summary>
        /// <value>
        /// <para>The upgrade domain for this node.</para>
        /// </value>
        public string UpgradeDomain { get; private set; }

        /// <summary>
        /// <para>Gets the fault domain for this node.</para>
        /// <remarks>
        /// <para>Fault domains define sets of nodes which are likely to experience failure at the same time due to shared physical dependencies such as power and networking resources. 
        /// Fault domains typically represent hierarchy and hence are represented using <see cref="System.Uri"/>.</para>
        /// </remarks>
        /// </summary>
        /// <value>
        /// <para>The fault domain for this node.</para>
        /// </value>
        public Uri FaultDomain { get; private set; }

        /// <summary>
        /// <para>Gets the internal ID used by Service Fabric to uniquely identify a node.</para>
        /// </summary>
        /// <value>
        /// <para>The internal ID used by Service Fabric to uniquely identify a node.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Id)]
        public NodeId NodeId { get; private set; }

        /// <summary>
        /// <para>Gets the internal ID used by Service Fabric to uniquely identify a node instance. The NodeId is deterministically mapped
        /// from NodeName and does not change across node restarts. However, the NodeInstanceId will change with every restart of the node.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Numerics.BigInteger" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.InstanceId)]
        public BigInteger NodeInstanceId { get; private set; }

        /// <summary>
        /// <para>
        /// Gets the deactivation information for the node.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The node deactivation information.</para>
        /// </value>
        public NodeDeactivationResult NodeDeactivationInfo { get; private set; }

        /// <summary>
        /// <para>
        /// True if a node is stopped.  A node is in a stopped state if it was the target of a successful call to StartNodeTransitionAsync with a NodeTransitionType of Stop.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>Whether or not a node is stopped</para>
        /// </value>
        public bool IsStopped { get; private set; }

        internal static unsafe Node CreateFromNative(
           NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM nativeResultItem)
        {
            string faultDomain = NativeTypes.FromNativeString(nativeResultItem.FaultDomain);
            Uri faultDomainUri = null;

            if (!string.IsNullOrEmpty(faultDomain))
            {
                faultDomainUri = new Uri(faultDomain);
            }

            NodeId nodeId = null;
            UInt64 instanceId = 0;

            NodeDeactivationResult nodeDeactivationInfo = null;
            bool isStopped = false;
            TimeSpan nodeDownTime = TimeSpan.Zero;
            DateTime nodeUpAt = DateTime.MinValue;
            DateTime nodeDownAt = DateTime.MinValue;

            if (nativeResultItem.Reserved != IntPtr.Zero)
            {
                NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM_EX1* nativeNodeResultItemEx1 = (NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM_EX1*)nativeResultItem.Reserved;
                nodeId = NativeTypes.FromNativeNodeId(nativeNodeResultItemEx1->NodeId);

                if (nativeNodeResultItemEx1->Reserved != IntPtr.Zero)
                {
                    NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM_EX2* nativeNodeResultItemEx2 = (NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM_EX2*)nativeNodeResultItemEx1->Reserved;
                    instanceId = nativeNodeResultItemEx2->NodeInstanceId;

                    if (nativeNodeResultItemEx2->Reserved != IntPtr.Zero)
                    {
                        NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM_EX3* nativeNodeResultItemEx3 = (NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM_EX3*)nativeNodeResultItemEx2->Reserved;
                        nodeDeactivationInfo = NodeDeactivationResult.CreateFromNative(*(NativeTypes.FABRIC_NODE_DEACTIVATION_QUERY_RESULT_ITEM*)nativeNodeResultItemEx3->NodeDeactivationInfo);

                        if (nativeNodeResultItemEx3->Reserved != IntPtr.Zero)
                        {
                            NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM_EX4* nativeNodeResultItemEx4 = (NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM_EX4*)nativeNodeResultItemEx3->Reserved;
                            isStopped = NativeTypes.FromBOOLEAN(nativeNodeResultItemEx4->IsStopped);

                            if (nativeNodeResultItemEx4 ->Reserved != IntPtr.Zero)
                            {
                                NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM_EX5* nativeNodeResultItemEx5 = (NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM_EX5*)nativeNodeResultItemEx4->Reserved;
                                nodeDownTime = TimeSpan.FromSeconds(nativeNodeResultItemEx5->NodeDownTimeInSeconds);

                                if (nativeNodeResultItemEx5 ->Reserved != IntPtr.Zero)
                                {
                                    NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM_EX6* nativeNodeResultItemEx6 = (NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM_EX6*)nativeNodeResultItemEx5->Reserved;
                                    nodeUpAt = NativeTypes.FromNativeFILETIME(nativeNodeResultItemEx6->NodeUpAt);
                                    nodeDownAt = NativeTypes.FromNativeFILETIME(nativeNodeResultItemEx6->NodeDownAt);
                                }
                            }
                        }
                    }
                }
            }

            var item = new Node(
                NativeTypes.FromNativeString(nativeResultItem.NodeName),
                NativeTypes.FromNativeString(nativeResultItem.IpAddressOrFQDN),
                NativeTypes.FromNativeString(nativeResultItem.NodeType),
                NativeTypes.FromNativeString(nativeResultItem.CodeVersion),
                NativeTypes.FromNativeString(nativeResultItem.ConfigVersion),
                (NodeStatus)nativeResultItem.NodeStatus,
                TimeSpan.FromSeconds(nativeResultItem.NodeUpTimeInSeconds),
                nodeDownTime,
                nodeUpAt,
                nodeDownAt,
                (HealthState)nativeResultItem.HealthState,
                NativeTypes.FromBOOLEAN(nativeResultItem.IsSeedNode),
                NativeTypes.FromNativeString(nativeResultItem.UpgradeDomain),
                faultDomainUri,
                nodeId,
                new BigInteger(instanceId),
                nodeDeactivationInfo,
                isStopped);

            return item;
        }
    }
}