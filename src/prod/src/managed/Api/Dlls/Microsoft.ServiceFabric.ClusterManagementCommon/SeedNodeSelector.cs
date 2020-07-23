// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Linq;

    public sealed class SeedNodeSelector
    {
        private const int MaxSeedNode = 9;
        private static readonly TraceType TraceType = new TraceType("SeedNodeSelector");

        private TimeSpan timeout;
        private TimeoutHelper timeoutHelper;
        private int lastNodeCount;
        private Dictionary<string, int> faultDomainMap;
        private Dictionary<string, int> upgradeDomainMap;
        private int row;
        private int col;
        private ITraceLogger traceLogger;

        public SeedNodeSelector(TimeSpan timeout, ITraceLogger logger)
        {
            this.timeout = timeout;
            this.timeoutHelper = new TimeoutHelper(timeout);
            this.lastNodeCount = 0;
            this.faultDomainMap = new Dictionary<string, int>();
            this.upgradeDomainMap = new Dictionary<string, int>();
            this.traceLogger = logger;
        }

        public static int GetSeedNodeCount(int totalExpectedNode)
        {
            if (totalExpectedNode >= MaxSeedNode)
            {
                return MaxSeedNode;
            }

            if (totalExpectedNode > 3 && totalExpectedNode % 2 == 0)
            {
                return totalExpectedNode - 1;
            }
            else
            {
                return totalExpectedNode;
            }
        }

        public IEnumerable<NodeDescription> Select(
            ReliabilityLevel reliabilityLevel,
            int totalExpectedPrimaryNodeCount,
            IEnumerable<NodeDescription> primaryNodeTypeNodes,
            int faultDomainCount,
            int upgradeDomainCount,
            bool isVmss)
        {
            this.traceLogger.WriteInfo(
                SeedNodeSelector.TraceType,
                "SeedNodeSelector.Select: Entering. reliabilityLevel: {0}, primaryNodeCount: {1}, primaryNodeTypeNodes.Count: {2}, faultDomainCount: {3}, upgradeDomainCount: {4}, primaryNodeTypeNodes: {5}",
                reliabilityLevel,
                totalExpectedPrimaryNodeCount,
                primaryNodeTypeNodes.Count(),
                faultDomainCount,
                upgradeDomainCount,
                this.DumpSeedNodes(primaryNodeTypeNodes));

            bool foundNewNode = this.UpdateNodeInfo(primaryNodeTypeNodes, faultDomainCount, upgradeDomainCount);
            this.traceLogger.WriteInfo(SeedNodeSelector.TraceType, "SeedNodeSelector.Select: foundNewNode is {0}", foundNewNode);

            var targetSeedNodeCount = reliabilityLevel.GetSeedNodeCount();
            this.traceLogger.WriteInfo(SeedNodeSelector.TraceType, "SeedNodeSelector.Select: nodes.Count: {0}, targetSeedNodeCount: {1}, totalExpectedNode: {2}", primaryNodeTypeNodes.Count(), targetSeedNodeCount, totalExpectedPrimaryNodeCount);

            if (primaryNodeTypeNodes.Count() < targetSeedNodeCount)
            {
                return null;
            }

            if (targetSeedNodeCount == totalExpectedPrimaryNodeCount && primaryNodeTypeNodes.Count() == targetSeedNodeCount)
            {
                return primaryNodeTypeNodes;
            }

            if (!foundNewNode && !TimeoutHelper.HasExpired(this.timeoutHelper))
            {
                // During optimal seed node selection, run the logic when
                // more nodes are available than the last attempt
                return null;
            }

            if (isVmss)
            {
                var selectedSeedNodes =
                    primaryNodeTypeNodes.OrderBy(item => item.NodeInstance)
                    .Take(targetSeedNodeCount)
                    .TakeWhile((nodeDescription, index) => index == nodeDescription.NodeInstance);

                if (selectedSeedNodes.Count() == targetSeedNodeCount)
                {
                    return selectedSeedNodes;
                }

                if (!TimeoutHelper.HasExpired(this.timeoutHelper))
                {
                    return null;
                }

                return this.AddSeedNodes(
                           targetSeedNodeCount - selectedSeedNodes.Count(),
                           primaryNodeTypeNodes,
                           selectedSeedNodes,
                           faultDomainCount,
                           upgradeDomainCount);
            }
            else
            {
                return this.SelectInternal(targetSeedNodeCount, primaryNodeTypeNodes, TimeoutHelper.HasExpired(this.timeoutHelper) || primaryNodeTypeNodes.Count() == totalExpectedPrimaryNodeCount);
            }
        }

        public bool TryUpdate(
            ReliabilityLevel reliabilityLevel,
            IEnumerable<NodeDescription> existingSeedNodes,
            IEnumerable<NodeDescription> enabledNodes, /*list of nodes which are enabled*/
            int faultDomainCount,
            int upgradeDomainCount,
            out List<NodeDescription> seedNodesToAdd,
            out List<NodeDescription> seedNodesToRemove)
        {
            existingSeedNodes.ThrowIfNull("existingSeedNodes");
            enabledNodes.ThrowIfNull("nodes");

            seedNodesToAdd = new List<NodeDescription>();
            seedNodesToRemove = new List<NodeDescription>();

            var targetSeedNodeCount = reliabilityLevel.GetSeedNodeCount();

            if (targetSeedNodeCount == existingSeedNodes.Count())
            {
                return true;
            }
            else if (targetSeedNodeCount < existingSeedNodes.Count())
            {
                var seedNodesToRemoveCount = existingSeedNodes.Count() - targetSeedNodeCount;
                seedNodesToRemove = this.RemoveSeedNodes(
                                        seedNodesToRemoveCount,
                                        existingSeedNodes);
            }
            else
            {
                var seedNodesToAddCount = targetSeedNodeCount - existingSeedNodes.Count();
                seedNodesToAdd = this.AddSeedNodes(
                                     seedNodesToAddCount,
                                     enabledNodes,
                                     existingSeedNodes,
                                     faultDomainCount,
                                     upgradeDomainCount);
                if (seedNodesToAdd == null)
                {
                    // not enough nodes available to reach target seed nodes
                    return false;
                }
            }

            return true;
        }

        public List<NodeDescription> AddSeedNodes(
    int seedNodesToAdd,
    IEnumerable<NodeDescription> nodes,
    IEnumerable<NodeDescription> currentSeedNodes,
    int faultDomainCount,
    int upgradeDomainCount)
        {
            this.UpdateNodeInfo(nodes, faultDomainCount, upgradeDomainCount);

            if (seedNodesToAdd <= 0 || nodes.Count() < currentSeedNodes.Count() + seedNodesToAdd)
            {
                return null;
            }

            if (nodes.Count() == currentSeedNodes.Count() + seedNodesToAdd)
            {
                return SubtractNodes(nodes, currentSeedNodes);
            }

            var data = this.GetMatrix(nodes);
            var existing = this.GetMatrix(currentSeedNodes);

            var result = RandomMatrixSearch.Run(data, existing, currentSeedNodes.Count() + seedNodesToAdd, true);
            for (int i = 0; i < this.row; i++)
            {
                for (int j = 0; j < this.col; j++)
                {
                    result[i, j] -= existing[i, j];
                }
            }

            return this.GetNodesFromMatrix(SubtractNodes(nodes, currentSeedNodes), result);
        }

        public List<NodeDescription> RemoveSeedNodes(
            int seedNodesToRemove,
            IEnumerable<NodeDescription> currentSeedNodes)
        {
            this.UpdateNodeInfo(currentSeedNodes, this.row, this.col);

            if (seedNodesToRemove <= 0 || currentSeedNodes.Count() <= seedNodesToRemove)
            {
                return null;
            }

            var data = this.GetMatrix(currentSeedNodes);
            var result = RandomMatrixSearch.Run(data, null, currentSeedNodes.Count() - seedNodesToRemove, true);
            var newSeedNodes = this.GetNodesFromMatrix(currentSeedNodes, result);

            return SubtractNodes(currentSeedNodes, newSeedNodes);
        }

        private static bool Validate(int[,] data, int target, int[,] result)
        {
            var row = result.GetUpperBound(0) + 1;
            var col = result.GetUpperBound(1) + 1;

            var rowCount = new int[row];
            var colCount = new int[col];
            var total = 0;

            for (var i = 0; i < row; i++)
            {
                for (var j = 0; j < col; j++)
                {
                    if (result[i, j] < 0 || result[i, j] > data[i, j])
                    {
                        return false;
                    }

                    rowCount[i] += result[i, j];
                    colCount[j] += result[i, j];
                    total += result[i, j];
                }
            }

            if (total != target)
            {
                return false;
            }

            Array.Sort(rowCount);
            Array.Sort(colCount);

            return rowCount[row - 1] - rowCount[0] <= 1 && colCount[col - 1] - colCount[0] <= 1;
        }

        private static List<NodeDescription> SubtractNodes(IEnumerable<NodeDescription> nodes1, IEnumerable<NodeDescription> nodes2)
        {
            List<NodeDescription> result = new List<NodeDescription>();
            foreach (var node in nodes1)
            {
                if (!nodes2.Contains(node))
                {
                    result.Add(node);
                }
            }

            return result;
        }

        private bool UpdateNodeInfo(IEnumerable<NodeDescription> nodes, int faultDomainCount, int upgradeDomainCount)
        {
            foreach (var node in nodes)
            {
                if (!this.faultDomainMap.ContainsKey(node.FaultDomain))
                {
                    this.faultDomainMap.Add(node.FaultDomain, this.faultDomainMap.Count);
                }

                if (!this.upgradeDomainMap.ContainsKey(node.UpgradeDomain))
                {
                    this.upgradeDomainMap.Add(node.UpgradeDomain, this.upgradeDomainMap.Count);
                }
            }

            this.row = Math.Max(faultDomainCount, this.faultDomainMap.Count);
            this.col = Math.Max(upgradeDomainCount, this.upgradeDomainMap.Count);

            if (nodes.Count() <= this.lastNodeCount)
            {
                return false;
            }

            this.timeoutHelper = new TimeoutHelper(this.timeout);
            this.lastNodeCount = nodes.Count();

            return true;
        }

        private int[,] GetMatrix(IEnumerable<NodeDescription> nodes)
        {
            var result = new int[this.row, this.col];
            foreach (var node in nodes)
            {
                int i = this.faultDomainMap[node.FaultDomain];
                int j = this.upgradeDomainMap[node.UpgradeDomain];
                result[i, j]++;
            }

            return result;
        }

        private IEnumerable<NodeDescription> SelectInternal(
            int targetSeedNodeCount,
            IEnumerable<NodeDescription> nodes,
            bool allowSuboptimal)
        {
            this.traceLogger.WriteInfo(SeedNodeSelector.TraceType, "SeedNodeSelector.SelectInternal: Entering. allowSuboptimal: {0}", allowSuboptimal);

            var data = this.GetMatrix(nodes);
            if (!allowSuboptimal)
            {
                var upperLimit = (targetSeedNodeCount + Math.Max(this.row, this.col) - 1) / Math.Max(this.row, this.col);
                for (int i = 0; i < this.row; i++)
                {
                    for (int j = 0; j < this.col; j++)
                    {
                        if (data[i, j] > upperLimit)
                        {
                            data[i, j] = upperLimit;
                        }
                    }
                }
            }

            if (nodes.Count() == targetSeedNodeCount)
            {
                if (!allowSuboptimal && !Validate(data, targetSeedNodeCount, data))
                {
                    this.traceLogger.WriteInfo(SeedNodeSelector.TraceType, "SeedNodeSelector.SelectInternal: return null.");
                    return null;
                }

                this.traceLogger.WriteInfo(SeedNodeSelector.TraceType, "SeedNodeSelector.SelectInternal: return original nodes.");
                return nodes;
            }

            var result = IterativeMatrixSearch.Run(data, targetSeedNodeCount);
            this.traceLogger.WriteInfo(SeedNodeSelector.TraceType, "SeedNodeSelector.SelectInternal: IterativeMatrixSearch.Run returns {0}.", result != null);

            if (result == null && allowSuboptimal)
            {
                result = RandomMatrixSearch.Run(data, null, targetSeedNodeCount, true);
                this.traceLogger.WriteInfo(SeedNodeSelector.TraceType, "SeedNodeSelector.SelectInternal: RandomMatrixSearch.Run returns {0}.", result != null);
            }

            if (result == null)
            {
                return null;
            }

            return this.GetNodesFromMatrix(nodes, result);
        }

        private List<NodeDescription> GetNodesFromMatrix(IEnumerable<NodeDescription> nodes, int[,] result)
        {
            var seedNodes = new List<NodeDescription>();
            foreach (var node in nodes)
            {
                var i = this.faultDomainMap[node.FaultDomain];
                var j = this.upgradeDomainMap[node.UpgradeDomain];
                if (result[i, j] > 0)
                {
                    result[i, j]--;
                    seedNodes.Add(node);
                }
            }

            return seedNodes;
        }

        private string DumpSeedNodes(IEnumerable<NodeDescription> nodes)
        {
            return string.Join(",", nodes.Select(p => string.Format("{0}.{1}.{2}", p.NodeName, p.FaultDomain, p.UpgradeDomain)));
        }
    }
}