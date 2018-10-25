// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Linq;

    internal class NodeFaultInformationHelper
    {
        private const string TraceType = "NodeFautInformationHelper";
        private Random random;

        private double minLiveNodesRatio;
        private double minLiveSeedNodesRatio;

        public NodeFaultInformationHelper(
            IReadOnlyCollection<NodeEntity> clusterNodes, 
            double minLiveNodesRatio,
            double minLiveSeedNodesRatio,
            bool unsafeModeEnabled,
            Random random)
        {
            this.random = random;
            this.ClusterNodes = clusterNodes;
            this.minLiveNodesRatio = minLiveNodesRatio;
            this.minLiveSeedNodesRatio = minLiveSeedNodesRatio;

            this.LiveSeedNodes = new SortedList<string, bool>();
            this.DownSeedNodes = new SortedList<string, bool>();
            this.LiveNonSeedNodes = new SortedList<string, bool>();
            this.DownNonSeedNodes = new SortedList<string, bool>();

            foreach (NodeEntity clusterNode in this.ClusterNodes)
            {
                var nodesSortedList = this.GetSortedListNodesInfo(clusterNode.CurrentNodeInfo.IsSeedNode, clusterNode.CurrentNodeInfo.IsNodeUp);
                nodesSortedList[clusterNode.CurrentNodeInfo.NodeName] = unsafeModeEnabled ? clusterNode.IsAvailableToFault : !clusterNode.IsUnsafeToFault;
            }

            this.LiveExcludedNodesCount = this.ClusterNodes.Count(node => node.CurrentNodeInfo.IsNodeUp && node.IsExcludedForFaults());
            this.LiveExcludedSeedNodesCount = this.ClusterNodes.Count(node => node.CurrentNodeInfo.IsSeedNode && node.CurrentNodeInfo.IsNodeUp && node.IsExcludedForFaults());
        }

        public IEnumerable<NodeEntity> ClusterNodes { get; private set; }

        public SortedList<string, bool> LiveSeedNodes { get; private set; }
        public SortedList<string, bool> DownSeedNodes { get; private set; }
        public SortedList<string, bool> LiveNonSeedNodes { get; private set; }
        public SortedList<string, bool> DownNonSeedNodes { get; private set; }

        public int ClusterNodesCount
        {
            get
            {
                return this.LiveSeedNodesCount + this.DownSeedNodesCount + this.LiveNonSeedNodesCount + this.DownNonSeedNodesCount;
            }
        }

        public int SeedNodesCount
        {
            get
            {
                return this.LiveSeedNodesCount + this.DownSeedNodesCount;
            }
        }
        
        public int NonSeedNodesCount
        {
            get
            {
                return this.LiveNonSeedNodesCount + this.DownNonSeedNodesCount;
            }
        }

        public int MinLiveNodesCount
        {
            get { return (int)Math.Ceiling(this.ClusterNodesCount * this.minLiveNodesRatio); }
        }

        public int MinLiveSeedNodesCount
        {
            get { return (int)Math.Ceiling(this.SeedNodesCount * this.minLiveSeedNodesRatio); }
        }

        /// <summary>
        /// The number of live nodes with which we start a Chaos iteration -- based on the StateSnapshot.
        /// </summary>
        public int LiveNodesCount
        {
            get { return this.LiveSeedNodesCount + this.LiveNonSeedNodesCount; }
        }

        /// <summary>
        /// The number of nodes that would have been alive at this moment, had we executed the pending faults right now.
        /// For example, if we are in the middle of a Chaos iteration and 
        /// we have already chosen (but have not executed yet) one node to restart, the FaultReadyLiveNodesCount will
        /// give us one less live nodes than the number of live nodes with which we started the current iteration.
        /// </summary>
        public int FaultReadyLiveNodesCount
        {
            get
            {
                return this.LiveSeedNodes.Count(seedNode => seedNode.Value) + this.LiveNonSeedNodes.Count(nonSeedNode => nonSeedNode.Value);
            }
        }

        public int DownNodesCount 
        {
            get { return this.DownSeedNodesCount + this.DownNonSeedNodesCount; }
        }

        public int LiveSeedNodesCount
        {
            get { return this.LiveSeedNodes.Count(); }
        }

        public int FaultReadyLiveSeedNodesCount
        {
            get
            {
                return this.LiveSeedNodes.Count(n => n.Value);
            }
        }

        public int DownSeedNodesCount
        {
            get { return this.DownSeedNodes.Count(); }
        }
        
        public int LiveNonSeedNodesCount
        {
            get { return this.LiveNonSeedNodes.Count(); }
        }

        public int DownNonSeedNodesCount
        {
            get { return this.DownNonSeedNodes.Count(); }
        }

        /// <summary>
        /// These are the nodes that are alive and are not included in the passed in NodeTypeInclusionList of ChaosTargetFilter
        /// </summary>
        public int LiveExcludedNodesCount { get; }

        /// <summary>
        /// These are the seed nodes that are alive and are not included in the passed in NodeTypeInclusionList of ChaosTargetFilter
        /// </summary>
        public int LiveExcludedSeedNodesCount { get; }

        public bool IsRingUnHealthy
        {
            get
            {
                return this.LiveSeedNodesCount < this.MinLiveSeedNodesCount
                        || (this.LiveNonSeedNodesCount + this.LiveSeedNodesCount) < this.MinLiveNodesCount;
            }
        }

        public NodeEntity GetDownNodeToFault(Guid activityId = default(Guid))
        {
            if (this.LiveNodesCount < this.ClusterNodesCount)
            {
                bool selectSeedNode = false;
                if (this.LiveSeedNodesCount < this.SeedNodesCount)
                {
                    selectSeedNode = this.random.NextDouble() < (this.SeedNodesCount / (double)this.ClusterNodesCount);
                }

                var chosenList = this.GetSortedListNodesInfo(selectSeedNode, false/*DownNodes*/).Where(kvp => kvp.Value == true /*IsAvailableToFault*/);
                if (chosenList.Count() > 0)
                {
                    var chosenNode = GetRandomFrom(chosenList);
                    ReleaseAssert.AssertIfNot(chosenNode.Value, "Should not have chosen in transition node");
                    var clusterNode = this.ClusterNodes.Where(n => n.CurrentNodeInfo.NodeName == chosenNode.Key).FirstOrDefault();
                    ReleaseAssert.AssertIf(clusterNode == null, "Cluster node with name {0} should exist in local structure", chosenNode.Key);
                    return clusterNode;
                }
            }

            return null;
        }

        public NodeEntity GetUpNodeToFault(Guid activityId = default(Guid))
        {
            // Say, we have three node types: FrontEnd (3 nodes), Application (3 nodes), and BackEnd (5 nodes).
            // If we include only FrontEnd node type in the NodeTypeInclusionFilter
            // then 3 (this.FaultReadyLiveNodesCount) < 8 (this.MinLiveNodesCount) and no NodeRestart faults happen.
            // The leftside of the following inequality (>) should include all currently alive nodes, hence the addition of liveExcludedNodesCount.
            if (this.FaultReadyLiveNodesCount + this.LiveExcludedNodesCount > this.MinLiveNodesCount)
            {
                bool selectSeedNode = false;

                if (this.FaultReadyLiveSeedNodesCount + this.LiveExcludedSeedNodesCount > this.MinLiveSeedNodesCount)
                {
                    selectSeedNode = this.random.NextDouble() < (this.SeedNodesCount / (double)this.ClusterNodesCount);
                }

                var chosenListOfFaultReadyNodes = this.GetSortedListNodesInfo(selectSeedNode, true/*UpNodes*/).Where(kvp => kvp.Value == true /*IsAvailableToFault*/).ToList();

                if (chosenListOfFaultReadyNodes.Any())
                {
                    var chosenNode = this.GetRandomFrom(chosenListOfFaultReadyNodes);
                    ReleaseAssert.AssertIfNot(chosenNode.Value, "Should not have chosen in transition node");
                    var clusterNode = this.ClusterNodes.FirstOrDefault(n => n.CurrentNodeInfo.NodeName == chosenNode.Key);
                    ReleaseAssert.AssertIf(clusterNode == null, "Cluster node with name {0} should exist in local structure", chosenNode.Key);
                    return clusterNode;
                }
            }

            return null;
        }
      
        private SortedList<string, bool> GetSortedListNodesInfo(bool isSeedNode, bool isUp)
        {
            if (isSeedNode && isUp)
            {
                return this.LiveSeedNodes;
            }
            else if (isSeedNode && !isUp)
            {
                return this.DownSeedNodes;
            }
            else if (!isSeedNode && isUp)
            {
                return this.LiveNonSeedNodes;
            }
            else
            {
                return this.DownNonSeedNodes;
            }
        }

        // returns a random element from collection
        private KeyValuePair<T, X> GetRandomFrom<T, X>(IEnumerable<KeyValuePair<T, X>> enumerable)
        {
            int randIndex = this.random.Next(enumerable.Count());
            return enumerable.ElementAt(randIndex);
        }
    }
}