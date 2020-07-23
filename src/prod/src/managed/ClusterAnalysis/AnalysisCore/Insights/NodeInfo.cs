// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;

    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;
    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// Wraps some key information about a cluster Node
    /// </summary>
    /// TODO: Switch with NodeEntity
    [DataContract]
    public class NodeInfo
    {
        public NodeInfo(NodeEntity node)
        {
            Assert.IsNotNull(node, "Node can't be null");
            this.NodeId = node.NodeId;
            this.NodeName = node.NodeName;
            this.NodeUpTime = node.NodeUpTime;
            this.IsSeedNode = node.IsSeedNode;
            this.CodeVersion = node.CodeVersion;
        }

        /// <summary>
        ///     ID of the Node
        /// </summary>
        [DataMember(IsRequired = true)]
        public string NodeId { get; private set; }

        /// <summary>
        ///     Name of the node where running
        /// </summary>
        [DataMember]
        public string NodeName { get; private set; }

        /// <summary>
        ///     Time Node has been Up
        /// </summary>
        [DataMember]
        public TimeSpan NodeUpTime { get; private set; }

        /// <summary>
        /// Service Fabric code version running on node
        /// </summary>
        [DataMember]
        public string CodeVersion { get; private set; }

        /// <summary>
        ///     Is node seed Node
        /// </summary>
        [DataMember]
        public bool IsSeedNode { get; private set; }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "NodeId: '{0}', Name: '{1}', UpTime: '{2}', IsSeed: '{3}'", this.NodeId, this.NodeName, this.NodeUpTime, this.IsSeedNode);
        }
    }
}