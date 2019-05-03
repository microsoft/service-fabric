// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using Newtonsoft.Json;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Query;
    using System.Fabric.WRP.Model;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    internal class NodeStatusManager
    {
        private static readonly TraceType TraceType = new TraceType("NodeStatusManager");

        private static readonly string StoreKey = "NodeStatus";        

        private readonly KeyValueStoreReplica kvsStore;

        private readonly IConfigStore configStore;
        private readonly string configSectionName;
        private readonly IExceptionHandlingPolicy exceptionPolicy;

        public NodeStatusManager(
            KeyValueStoreReplica kvsStore,
            IConfigStore configStore,
            string configSectionName,
            IExceptionHandlingPolicy exceptionPolicy)
        {
            kvsStore.ThrowIfNull(nameof(kvsStore));
            configStore.ThrowIfNull(nameof(configStore));
            configSectionName.ThrowIfNullOrWhiteSpace(nameof(configSectionName));

            this.kvsStore = kvsStore;
            this.configStore = configStore;
            this.configSectionName = configSectionName;
            this.exceptionPolicy = exceptionPolicy;
        }

        public async Task<List<PaasNodeStatusInfo>> GetNodeStates(TimeSpan timeout,  CancellationToken cancellationToken)
        {
            List<PaasNodeStatusInfo> nodeStates = null;
            
            Func<List<UpgradeServiceNodeState>, bool> operation = (currentNodeStates) => this.GetNodeStatesInternal(currentNodeStates, out nodeStates);                
            await this.PerformKvsOperation(operation, timeout, cancellationToken);

            return nodeStates;
        }

        public async Task ProcessWRPResponse(List<PaasNodeStatusInfo> responseFromWrp, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Func<List<UpgradeServiceNodeState>, bool> operation = (currentNodeStates) => this.ProcessWRPResponseInternal(currentNodeStates, responseFromWrp);                
            await this.PerformKvsOperation(operation, timeout, cancellationToken);
        }

        public async Task ProcessNodeQuery(NodeList queryNodes, TimeSpan timeout, CancellationToken cancellationToken)
        {            
            Func<List<UpgradeServiceNodeState>, bool> operation = (currentNodeStates) => this.ProcessNodeQueryInternal(currentNodeStates, queryNodes);                
            await this.PerformKvsOperation(operation, timeout, cancellationToken);
        }

        private bool ProcessWRPResponseInternal(List<UpgradeServiceNodeState> currentUpgradeServiceNodeStates, List<PaasNodeStatusInfo> responseFromWrp)
        {
            if(responseFromWrp == null)
            {
                return false;
            }

            bool shouldUpdate = false;
            foreach (var wrpNodeStatus in responseFromWrp)
            {
                var matchingUpgradeServiceNodeState = currentUpgradeServiceNodeStates.FirstOrDefault(
                    nodeState => nodeState.Node.NodeName.Equals(wrpNodeStatus.NodeName, StringComparison.OrdinalIgnoreCase));
                if (matchingUpgradeServiceNodeState != null && matchingUpgradeServiceNodeState.Node.IntentionInstance == wrpNodeStatus.IntentionInstance)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "{0} is processed by SFRP",
                        matchingUpgradeServiceNodeState.Node);

                    matchingUpgradeServiceNodeState.IsProcessedByWRP = true;
                    shouldUpdate = true;
                }
            }

            return shouldUpdate;
        }

        private bool ProcessNodeQueryInternal(List<UpgradeServiceNodeState> currentUpgradeServiceNodeStates, NodeList queryNodes)
        {
            if (queryNodes == null)
            {
                return false;
            }

            bool shouldUpdate = false;
            foreach (var queryNode in queryNodes)
            {
                var latestNodeStatusInfo = new PaasNodeStatusInfo(queryNode);

                var matchingUpgradeServiceNodeState = currentUpgradeServiceNodeStates.FirstOrDefault(
                    nodeState => nodeState.Node.NodeName.Equals(latestNodeStatusInfo.NodeName));
                if (matchingUpgradeServiceNodeState == null)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "{0} is added newly in NodeStatus",
                        latestNodeStatusInfo);

                    // New node in the query.
                    // Add it to the nodeStates so that it will be reported to WRP     
                    // This is done to handle proper fuctioning of FabricUS even after data loss
                    currentUpgradeServiceNodeStates.Add(new UpgradeServiceNodeState(latestNodeStatusInfo));
                    shouldUpdate = true;
                    continue;
                }

                if (matchingUpgradeServiceNodeState.ProcessLatestNode(latestNodeStatusInfo))
                {
                    shouldUpdate = true;
                }
            }

            return shouldUpdate;
        }

        private bool GetNodeStatesInternal(List<UpgradeServiceNodeState> currentUpgradeServiceNodeStates, out List<PaasNodeStatusInfo> nodeStates)
        {
            var nodeStatusBatchSize = this.configStore.Read<int>(this.configSectionName, Constants.ConfigurationSection.NodeStatusBatchSize, 25);

            int nodeStateCount = 0;
            nodeStates = new List<PaasNodeStatusInfo>();
            foreach(var currentUpgradeServiceNodeState in currentUpgradeServiceNodeStates)
            {
                if(!currentUpgradeServiceNodeState.IsProcessedByWRP)
                {
                    nodeStates.Add(currentUpgradeServiceNodeState.Node);
                    if(++nodeStateCount == nodeStatusBatchSize)
                    {
                        // Throttle the number of nodes reported to SFRP to 50
                        // Other nodes will be reported in the next iteration
                        break;
                    }
                }
            }

            return false;
        }

        private async Task PerformKvsOperation(Func<List<UpgradeServiceNodeState>, bool> operation, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Trace.WriteNoise(TraceType, "PerformKvsOperation: Begin.");
            using (var tx = await this.kvsStore.CreateTransactionWithRetryAsync(this.exceptionPolicy, cancellationToken))
            {
                bool shouldAdd = false;
                List<UpgradeServiceNodeState> upgradeServiceNodeStates = new List<UpgradeServiceNodeState>();
                
                var item = this.kvsStore.TryGet(tx, StoreKey);
                if (item == null)
                {                    
                    shouldAdd = true;
                }
                else
                {
                    Trace.WriteNoise(TraceType, "PerformKvsOperation: Invoking JsonSerializationHelper.DeserializeObject<List<UpgradeServiceNodeState>>");
                    upgradeServiceNodeStates = JsonSerializationHelper.DeserializeObject<List<UpgradeServiceNodeState>>(item.Value);
                    Trace.WriteNoise(TraceType, "PerformKvsOperation: Completed JsonSerializationHelper.DeserializeObject<List<UpgradeServiceNodeState>>");
                }

                bool shouldUpdate = operation(upgradeServiceNodeStates);
                
                if (shouldAdd || shouldUpdate)
                {
                    var serializedState = JsonSerializationHelper.SerializeObject(upgradeServiceNodeStates);

                    if (shouldAdd)
                    {
                        Trace.WriteNoise(TraceType, "PerformKvsOperation: Invoking this.kvsStore.Add");
                        this.kvsStore.Add(tx, StoreKey, serializedState);
                        Trace.WriteNoise(TraceType, "PerformKvsOperation: Completed this.kvsStore.Add");
                    }
                    else
                    {
                        Trace.WriteNoise(TraceType, "PerformKvsOperation: Invoking this.kvsStore.Update");
                        this.kvsStore.Update(tx, StoreKey, serializedState, item.Metadata.SequenceNumber);
                        Trace.WriteNoise(TraceType, "PerformKvsOperation: Completed this.kvsStore.Update");
                    }
                    Trace.WriteNoise(TraceType, "PerformKvsOperation: Invoking tx.CommitAsync");
                    await tx.CommitAsync(timeout);
                    Trace.WriteNoise(TraceType, "PerformKvsOperation: Completed tx.CommitAsync");
                }
            }
            Trace.WriteNoise(TraceType, "PerformKvsOperation: End.");
        }

        internal async Task FilterNodeTypesAsync(IEnumerable<string> primaryNodeTypes, TimeSpan timeout, CancellationToken cancellationToken)
        {
            using (var tx = await this.kvsStore.CreateTransactionWithRetryAsync(this.exceptionPolicy, cancellationToken))
            {
                var item = this.kvsStore.TryGet(tx, StoreKey);
                if (item == null)
                {
                    return;
                }

                var nodeStates = JsonSerializationHelper.DeserializeObject<List<UpgradeServiceNodeState>>(item.Value);
                if (nodeStates == null || !nodeStates.Any())
                {
                    return;
                }

                if (cancellationToken.IsCancellationRequested)
                {
                    return;
                }

                var filtered = new List<UpgradeServiceNodeState>();
                foreach (var nodeState in nodeStates)
                {
                    if (primaryNodeTypes.Contains(nodeState.Node.NodeType))
                    {
                        filtered.Add(nodeState);
                    }
                }

                var serializedState = JsonSerializationHelper.SerializeObject(filtered);

                if (cancellationToken.IsCancellationRequested)
                {
                    return;
                }

                this.kvsStore.Update(tx, StoreKey, serializedState);

                await tx.CommitAsync(timeout);
            }
        }

        class UpgradeServiceNodeState
        {
            [JsonConstructor]
            public UpgradeServiceNodeState()
            {
            }

            public UpgradeServiceNodeState(PaasNodeStatusInfo node)
            {                
                this.IsProcessedByWRP = false;
                this.Node = node;
                this.Node.IntentionInstance = DateTime.Now.Ticks;
            }            

            public bool IsProcessedByWRP { get; set; }

            public PaasNodeStatusInfo Node { get; set; }

            public bool ProcessLatestNode(PaasNodeStatusInfo latestNode)
            {
                ReleaseAssert.AssertIfNot(
                    this.Node.NodeName.Equals(latestNode.NodeName), "Existing NodeName {0} does not match new name {1}", 
                    this.Node.NodeName, 
                    latestNode.NodeName);

                if(!this.Node.Equals(latestNode))
                {
                    var oldNode = this.Node;

                    this.Node = latestNode;
                    this.Node.IntentionInstance++;
                    this.IsProcessedByWRP = false;

                    Trace.WriteInfo(
                        TraceType,
                        "Updating NodeStatus. Old: {0}, Updated: {1}",
                        oldNode,
                        this.Node);

                    return true;
                }

                return false;
            }
        }
    }
}