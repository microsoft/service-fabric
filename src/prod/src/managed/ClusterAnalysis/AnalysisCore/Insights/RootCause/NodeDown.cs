// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.RootCause
{
    using System.Runtime.Serialization;
    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// </summary>
    [DataContract]
    [KnownType(typeof(NodeInfo))]
    public class NodeDown : IRootCause
    {
        /// <summary>
        /// </summary>
        /// <param name="nodeInfo"></param>
        /// <param name="impact"></param>
        public NodeDown(NodeInfo nodeInfo, string impact)
        {
            Assert.IsNotNull(nodeInfo, "NodeInfo can't be null");
            Assert.IsNotEmptyOrNull(impact, "impact");
            this.DownNode = nodeInfo;
            this.Impact = impact;
        }

        /// <summary>
        /// </summary>
        [DataMember(IsRequired = true)]
        public NodeInfo DownNode { get; private set; }

        /// <inheritdoc />
        [DataMember(IsRequired = true)]
        public string Impact { get; private set; }

        /// <inheritdoc />
        public string GetUserFriendlySummary()
        {
            return string.Format(
                "Node '{0}' which is seed Node '{1}', went Down with Impact '{2}'",
                this.DownNode.NodeName,
                this.DownNode.IsSeedNode,
                this.Impact);
        }
    }
}