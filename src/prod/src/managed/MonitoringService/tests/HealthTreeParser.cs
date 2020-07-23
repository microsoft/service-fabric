// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Tests
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using Health;

    /// <summary>
    /// Base class for parsing TreeNode based health tree.
    /// </summary>
    internal abstract class HealthTreeParser
    {
        internal HealthTreeParser()
        {
            this.NodeList = new List<string>();
            this.ApplicationList = new List<Uri>();
            this.ServiceList = new List<Uri>();
            this.PartitionList = new List<Guid>();
            this.ReplicaList = new List<Tuple<Guid, long>>();
        }

        protected IList<string> NodeList { get; private set; }

        protected IList<Uri> ApplicationList { get; private set; }

        protected IList<Uri> ServiceList { get; private set; }

        protected IList<Guid> PartitionList { get; private set; }

        protected IList<Tuple<Guid, long>> ReplicaList { get; private set; }

        protected string ClusterName { get; private set; }

        protected void Parse(string clusterName, TreeNode clusterNode)
        {
            this.ParseClusterNode(clusterName, clusterNode);
        }

        protected abstract void ParseCluserNode(string clusterName, TreeNode clusterNode);

        protected abstract void ParseNodeHealthNode(string clusterName, TreeNode node);

        protected abstract void ParseAppNode(string clusterName, TreeNode appNode);

        protected abstract void ParseServiceNode(string clusterName, Uri appUri, TreeNode serviceNode);

        protected abstract void ParsePartitionNode(string clusterName, Uri appUri, Uri serviceUri, TreeNode partitionNode);

        protected abstract void ParseReplicaNode(string clusterName, Uri appUri, Uri serviceUri, Guid partitionId, TreeNode replicaNode);

        private void ParseClusterNode(string clusterName, TreeNode clusterNode)
        {
            this.ParseCluserNode(clusterName, clusterNode);
            this.ClusterName = clusterName;

            var appTreeNodes = clusterNode.AsEnumerable().Where(node => node.Health is ApplicationHealth);
            foreach (var appNode in appTreeNodes)
            {
                this.ParseAppNode(clusterName, appNode);

                var appHealth = appNode.Health as ApplicationHealth;
                this.ApplicationList.Add(appHealth.ApplicationUri);

                var serviceNodes = appNode.AsEnumerable().Where(node => node.Health is ServiceHealth);
                foreach (var serviceNode in serviceNodes)
                {
                    this.ParseServiceNode(clusterName, appHealth.ApplicationUri, serviceNode);

                    var serviceHealth = serviceNode.Health as ServiceHealth;
                    this.ServiceList.Add(serviceHealth.ServiceName);

                    var partitionNodes = serviceNode.AsEnumerable().Where(node => node.Health is PartitionHealth);
                    foreach (var partitionNode in partitionNodes)
                    {
                        this.ParsePartitionNode(clusterName, appHealth.ApplicationUri, serviceHealth.ServiceName, partitionNode);

                        var partitionHealth = partitionNode.Health as PartitionHealth;
                        this.PartitionList.Add(partitionHealth.PartitionId);

                        var replicaNodes = partitionNode.AsEnumerable().Where(node => node.Health is ReplicaHealth);
                        foreach (var replicaNode in replicaNodes)
                        {
                            this.ParseReplicaNode(clusterName, appHealth.ApplicationUri, serviceHealth.ServiceName, partitionHealth.PartitionId, replicaNode);

                            var replicaHealth = replicaNode.Health as ReplicaHealth;
                            this.ReplicaList.Add(new Tuple<Guid, long>(partitionHealth.PartitionId, replicaHealth.ReplicaId));
                        }
                    }
                }
            }

            var nodeTreeNodes = clusterNode.AsEnumerable().Where(node => node.Health is NodeHealth);
            foreach (var node in nodeTreeNodes)
            {
                this.ParseNodeHealthNode(clusterName, node);

                var nodeHealth = node.Health as NodeHealth;
                this.NodeList.Add(nodeHealth.NodeName);
            }
        }
    }
}