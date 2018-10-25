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

    internal sealed class NodeFaultActionGenerator : ActionGeneratorBase
    {
        private const int maxTrialPerActionGeneration = 10;
        private readonly NodeFaultActionGeneratorParameters testParameters;
        private readonly WeightedDice<NodeFaultCategory> nodeFaultCategoryChooser;

        public NodeFaultActionGenerator(NodeFaultActionGeneratorParameters testParameters, Random random)
            : base(random)
        {
            this.testParameters = testParameters;
            this.nodeFaultCategoryChooser = new WeightedDice<NodeFaultCategory>(this.Random);
            this.nodeFaultCategoryChooser.AddNewFace(NodeFaultCategory.RestartFault, testParameters.RestartNodeFaultWeight);
            this.nodeFaultCategoryChooser.AddNewFace(NodeFaultCategory.StartStopFault, testParameters.StartStopNodeFaultWeight);

            this.TraceType = "Chaos.NodeFaultActionGenerator";
        }

        protected override void GenerateAndEnqueueRandomActions(ClusterStateSnapshot stateInfo, Guid activityId = default(Guid))
        {
            // Use current state information.
            this.GenerateAndEnqueueRandomActions(stateInfo.Nodes, stateInfo.UnsafeModeEnabled, stateInfo.ClusterSnapshotId, activityId);
        }

        private enum NodeFaultCategory : int
        {
            Min = 0,
            RestartFault,
            StartStopFault,
            Max
        }

        private void GenerateAndEnqueueRandomActions(IReadOnlyCollection<NodeEntity> nodeList, bool unsafeModeEnabled, Guid clusterSnapshotId, Guid activityId = default(Guid))
        {
            var nodeStateHelper = new NodeFaultInformationHelper(
                nodeList,
                this.testParameters.MinLiveNodesRatio, 
                this.testParameters.MinLiveSeedNodesRatio, 
                unsafeModeEnabled,
                this.Random);
            var action = this.CreateNodeFaultAction(nodeStateHelper, clusterSnapshotId, activityId);
            this.EnqueueAction(action);
        }
        
        // Creates and returns random nodes whose state can be toggled
        private StateTransitionAction CreateNodeFaultAction(NodeFaultInformationHelper nodeStateHelper, Guid clusterSnapshotId, Guid activityId = default(Guid))
        {
            var action = this.CreateOneNodeFaultAction(nodeStateHelper, clusterSnapshotId, activityId);
            return action;
        }

        // Returns a nodeInfo of a node whose status can be toggled. Returns null if can't find a valid node.
        private NodeStateTransitionAction CreateOneNodeFaultAction(NodeFaultInformationHelper nodeStateHelper, Guid clusterSnapshotId, Guid activityId = default(Guid))
        {
            NodeStateTransitionAction action = null;
            NodeEntity selectedNode = null;
            for (int trial = 0; trial < maxTrialPerActionGeneration && action == null; trial++)
            {
                // Choose category 
                NodeFaultCategory faultCategory = (NodeFaultCategory)this.nodeFaultCategoryChooser.NextRoll();

                switch (faultCategory)
                {
                    case NodeFaultCategory.RestartFault:
                        {
                            selectedNode = nodeStateHelper.GetUpNodeToFault(activityId);
                            if (selectedNode != null)
                            {
                                var nodeInfo = nodeStateHelper.ClusterNodes.FirstOrDefault(n => n.CurrentNodeInfo.NodeName == selectedNode.CurrentNodeInfo.NodeName);
                                ReleaseAssert.AssertIf(nodeInfo == null, "NodeInfo should not be null");
                                TestabilityTrace.TraceSource.WriteInfo(this.TraceType, "{0}: '{1}' is chosen for RestartNode.", activityId, selectedNode);
                                action = new RestartNodeStateTransitionAction(nodeInfo.CurrentNodeInfo.NodeName, (long)nodeInfo.CurrentNodeInfo.NodeInstanceId, clusterSnapshotId);
                            }
                        }

                        break;
                    case NodeFaultCategory.StartStopFault:
                        {
                            selectedNode = this.Random.NextDouble() > 0.5
                                ? nodeStateHelper.GetUpNodeToFault(activityId)
                                : nodeStateHelper.GetDownNodeToFault(activityId);

                            if (selectedNode != null)
                            {
                                var node = selectedNode;
                                var nodeInfo = nodeStateHelper.ClusterNodes.FirstOrDefault(n => n.CurrentNodeInfo.NodeName == node.CurrentNodeInfo.NodeName);
                                ReleaseAssert.AssertIf(nodeInfo == null, "NodeInfo should not be null");

                                if (nodeInfo.CurrentNodeInfo.IsNodeUp)
                                {
                                    TestabilityTrace.TraceSource.WriteInfo(this.TraceType, "{0}: '{1}' is chosen for StopNode.", activityId, selectedNode);
                                    action = new StopNodeStateTransitionAction(nodeInfo.CurrentNodeInfo.NodeName, (long)nodeInfo.CurrentNodeInfo.NodeInstanceId, clusterSnapshotId);

                                    nodeInfo.CurrentNodeInfo.IsNodeUp = false;
                                }
                                else
                                {
                                    TestabilityTrace.TraceSource.WriteInfo(this.TraceType, "{0}: '{1}' is chosen for StartNode.", activityId, selectedNode);
                                    action = new StartNodeStateTransitionAction(nodeInfo.CurrentNodeInfo.NodeName, (long)nodeInfo.CurrentNodeInfo.NodeInstanceId, clusterSnapshotId);
                                }
                            }
                        }
                        break;
                    default:
                        throw new ArgumentException("Unknown category:" + faultCategory);
                }

                // Validate this generated action.
                if (action != null)
                {
                    ReleaseAssert.AssertIf(selectedNode == null, "selectedNode cannot be null");
                    selectedNode.FaultNode(activityId);
                }
            }

            return action;
        }
    }
}