// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.ClusterEntityDescription
{
    using System;
    using System.Fabric.Query;
    using System.Globalization;
    using System.Runtime.Serialization;
    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// Wrapper over Cluster Node for easy unit testability. All props are internal so that
    /// they can be easily set by Unit test.
    /// </summary>
    [DataContract]
    public class NodeEntity : BaseEntity
    {
        /// <summary>
        /// </summary>
        /// <param name="node"></param>
        public NodeEntity(Node node)
        {
            Assert.IsNotNull(node, "Node can't be null");
            this.NodeId = node.NodeId.ToString();
            this.NodeName = node.NodeName;
            this.NodeUpTime = DateTime.UtcNow - node.NodeUpAt;
            this.NodeStatus = node.NodeStatus;
            this.IsSeedNode = node.IsSeedNode;
            this.CodeVersion = node.CodeVersion;
            this.ConfigVersion = node.ConfigVersion;
        }

        internal NodeEntity()
        {
        }

        /// <summary>
        ///     ID of the Node
        /// </summary>
        [DataMember(IsRequired = true)]
        public string NodeId { get; internal set; }

        /// <summary>
        ///     Name of the node where running
        /// </summary>
        [DataMember]
        public string NodeName { get; internal set; }

        /// <summary>
        ///     Time Node has been Up
        /// </summary>
        [DataMember]
        public TimeSpan NodeUpTime { get; internal set; }

        /// <summary>
        /// Current state of Node
        /// </summary>
        [DataMember]
        public NodeStatus NodeStatus { get; internal set; }

        /// <summary>
        /// Service Fabric code version running on node
        /// </summary>
        [DataMember] 
        public string CodeVersion { get; internal set; }

        /// <summary>
        /// Service Fabric config version running on node
        /// </summary>
        [DataMember]
        public string ConfigVersion { get; internal set; }

        /// <summary>
        ///     Is node seed Node
        /// </summary>
        [DataMember]
        public bool IsSeedNode { get; internal set; }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "NodeId: '{0}', Name: '{1}', UpTime: '{2}', IsSeed: '{3}'", this.NodeId, this.NodeName, this.NodeUpTime, this.IsSeedNode);
        }

        /// <inheritdoc />
        public override bool Equals(BaseEntity other)
        {
            Assert.IsNotNull(other);
            var otherObj = other as NodeEntity;
            if (otherObj == null)
            {
                return false;
            }

            return this.Equals(otherObj);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = (hash * 23) + this.NodeName.GetHashCode();
                hash = (hash * 23) + this.NodeId.GetHashCode();
                return hash;
            }
        }

        /// <inheritdoc />
        public override int GetUniqueIdentity()
        {
            return this.GetHashCode();
        }
    }
}