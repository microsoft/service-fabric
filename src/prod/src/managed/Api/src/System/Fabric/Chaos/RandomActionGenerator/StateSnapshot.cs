// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Linq;
    using System.Text;

    internal class ClusterStateSnapshot
    {
        private const string TraceType = "Chaos.ClusterStateSnapshot";

        private const string Preamble = "Start of ClusterStateSnapshot...";
        private const string Postamble = "End of ClusterStateSnapshot...";

        // There appears to be a limit of 64 KB on the event size
        private const int TraceEventSizeLimit = 32000;

        public ClusterStateSnapshot(bool unsafeModeEnabled, bool shouldFaultSystem = false)
        {
            this.ClusterSnapshotId = Guid.NewGuid();
            this.UnsafeModeEnabled = unsafeModeEnabled;
            this.WorkloadList = new WorkloadList(new List<string>());
            this.Nodes = new NodeEntityList(this);
            this.Applications = new ApplicationEntityList(this);

            this.ShouldFaultSystem = shouldFaultSystem;

            NodeIdToNodeNameMap = new Dictionary<NodeId, string>();
            NodeNameToNodeIdMap = new Dictionary<string, NodeId>();
        }

        public Guid ClusterSnapshotId { get; private set; }

        public bool UnsafeModeEnabled { get; private set; }

        public WorkloadList WorkloadList { get; set; }

        public NodeEntityList Nodes { get; private set; }

        public ApplicationEntityList Applications { get; private set; }

        internal bool ShouldFaultSystem { get; private set; }

        internal bool SnapshotTraceEnabled { get; set; }

        internal bool ChaosTargetFilterPresent { get; set; }

        internal static Dictionary<string, NodeId> NodeNameToNodeIdMap { get; set; }
        internal static Dictionary<NodeId, string> NodeIdToNodeNameMap { get; set; }

        public void MarkAllUnsafeEntities()
        {
            TestabilityTrace.TraceSource.WriteNoise(TraceType, "Inside of MarkAllUnsafeEntities ...");

            var codepackages = this.GetAllCodePackages().ToArray();

            foreach (var cp in codepackages)
            {
                CodePackageEntity codePackage = cp;
                var node = this.Nodes.FirstOrDefault(n => n.CurrentNodeInfo.NodeName == codePackage.NodeName);

                ChaosUtility.ThrowOrAssertIfTrue(
                    ChaosConstants.MarkReplicaAsInTransition_NodeFaulted_TelemetryId,
                    node == null,
                    string.Format(
                        "Node entity {0} not found for code package {1}:{2}:{3}.",
                        codePackage.NodeName,
                        codePackage.ParentApplicationEntity.Application.ApplicationName,
                    codePackage.CodePackageResult.ServiceManifestName,
                    codePackage.CodePackageResult.CodePackageName));

                if (codePackage.Health.AggregatedHealthState != HealthState.Ok)
                {
                    codePackage.MarkCodePackageAsUnsafeToFault();
                    node.MarkNodeAsUnsafeToFault();
                }
                else
                {
                    // TODO: RDBug 7635808 : Test and see if DeployedPartition
                    // loop in the Chaos engine mark unsafe can be eliminated
                    //
                    var deployedPartitions = codePackage.DeployedPartitions;

                    foreach (var deployedPartition in deployedPartitions)
                    {
                        if (deployedPartition.GetPartitionFaultTolerance() <= 0
                            || deployedPartition.Partition.HealthState != HealthState.Ok)
                        {
                            codePackage.MarkCodePackageAsUnsafeToFault();
                            node.MarkNodeAsUnsafeToFault();
                        }
                    }
                }
            }

            // One way would have been to go through every replica and along with its health also check its ancestors' health
            // but we know that a partition's health state always reflect the worst healthstate among its replicas, so going
            // through the partitions is enough
            //
            var allPartitions = this.GetAllPartitions(null, null, !this.ShouldFaultSystem).ToArray();

            foreach (var p in allPartitions)
            {
                PartitionEntity partition = p;

                if (partition.GetPartitionFaultTolerance() <= 0
                    || partition.Partition.HealthState != HealthState.Ok
                    || partition.ParentServiceEntity.Service.HealthState != HealthState.Ok
                    || partition.ParentServiceEntity.ParentApplicationEntity.Application.HealthState != HealthState.Ok)
                {
                    this.MarkPartitionAsUnsafe(partition);
                }
            }

            foreach (var unhealthyNode in this.Nodes.Where(n => n.CurrentNodeInfo.HealthState != HealthState.Ok))
            {
                unhealthyNode.MarkNodeAsUnsafeToFault();
            }
        }

        private void MarkPartitionAsUnsafe(PartitionEntity partitionEntity)
        {
            foreach (var r in partitionEntity.ReplicaList)
            {
                ReplicaEntity replica = r;

                var currentReplicaCodePackage = partitionEntity.ParentServiceEntity.ParentApplicationEntity.GetCodePackagEntityForReplica(replica);

                if (currentReplicaCodePackage != null)
                {
                    currentReplicaCodePackage.MarkCodePackageAsUnsafeToFault();
                }

                var clusterNode = partitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot.Nodes.FindMatchingNodeEntity(replica.Replica.NodeName);

                if (clusterNode != null)
                {
                    clusterNode.MarkNodeAsUnsafeToFault();
                }

                replica.MarkReplicaAsUnsafeToFault();
            }
        }

        public IEnumerable<CodePackageEntity> GetAllCodePackages()
        {
            return this.Applications.Where(
                app =>
                    {
                        bool toAdd = true;
                        if (IsSystemApplication(app) && !this.ShouldFaultSystem)
                        {
                            toAdd = false;
                        }

                        return toAdd;
                    })
                .SelectMany(applicationEntity => applicationEntity.CodePackages);
        }

        public IEnumerable<NodeEntity> GetFaultableNodes()
        {
            return this.Nodes.Where(n => (this.UnsafeModeEnabled ? n.IsAvailableToFault : !n.IsUnsafeToFault) && n.CurrentNodeInfo.IsNodeUp);
        }

        public IEnumerable<CodePackageEntity> GetFaultableCodePackages()
        {
            var faultTolerantCodePackages = new List<CodePackageEntity>();

            foreach (var codePackage in this.GetAllCodePackages().Where(cp => !IsDummyCodePackage(cp)))
            {
                var node = this.Nodes.FirstOrDefault(n => n.CurrentNodeInfo.NodeName == codePackage.NodeName);
                ReleaseAssert.AssertIf(
                    node == null,
                    "Node entity {0} not found for code package {1}:{2}:{3}",
                    codePackage.NodeName,
                    codePackage.ParentApplicationEntity.Application.ApplicationName,
                    codePackage.CodePackageResult.ServiceManifestName,
                    codePackage.CodePackageResult.CodePackageName);

                if (node != null)
                {
                    bool isCodePackageFaultable = 
                        this.UnsafeModeEnabled 
                        ? codePackage.IsAvailableToFault 
                        : !codePackage.IsUnsafeToFault;

                    if (isCodePackageFaultable)
                    {
                        faultTolerantCodePackages.Add(codePackage);
                    }
                }
            }

            return faultTolerantCodePackages;
        }

        public IEnumerable<ReplicaEntity> GetFaultableReplicas(bool? isStateful = null, bool? hasPersistedState = null)
        {
            var faultTolerantReplicas = new List<ReplicaEntity>();

            var partitions = new List<PartitionEntity>();

            if (this.UnsafeModeEnabled && this.ShouldFaultSystem)
            {
                partitions.AddRange(this.GetAllPartitions(isStateful, hasPersistedState, false /*excludeSystemServices*/));
            }
            else if (this.UnsafeModeEnabled && !this.ShouldFaultSystem)
            {
                partitions.AddRange(this.GetAllPartitions(isStateful, hasPersistedState, true /*excludeSystemServices*/));
            }
            else
            {
                partitions.AddRange(this.GetFaultablePartitions(isStateful, hasPersistedState));
            }

            foreach (var partitionEntity in partitions)
            {
                faultTolerantReplicas.AddRange(partitionEntity.GetFaultReadyReplicaList());
            }

            return faultTolerantReplicas;
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            
            sb.AppendLine("Nodes:");

            foreach(var node in this.Nodes)
            {
                sb.AppendLine(string.Format("Node: {0}", node));
                sb.AppendLine(string.Format(" Deployed codepackages:"));
                var codepackages = this.GetAllCodePackages().Where(cp => cp.NodeName == node.CurrentNodeInfo.NodeName);

                foreach(var cp in codepackages)
                {
                    sb.AppendLine(string.Format("  cp: {0}", cp));

                    foreach(var partition in cp.DeployedPartitions)
                    {
                        sb.AppendLine(string.Format("   deployed partition: {0}", partition));

                        foreach(var replica in partition.ReplicaList)
                        {
                            sb.AppendLine(string.Format("    replica: {0}", replica));
                        }
                    }
                }
            }

            sb.AppendLine("Applications:");
            sb.AppendLine(this.Applications.ToString());

            return sb.ToString();
        }

        public void WriteToTrace(
            string traceType = TraceType, 
            int chunkSize = 5000, 
            string customPreamble = "", 
            string customPostamble = "", 
            Guid activityId = default(Guid))
        {
            if (!this.SnapshotTraceEnabled)
            {
                return;
            }

            if (chunkSize > TraceEventSizeLimit)
            {
                TestabilityTrace.TraceSource.WriteWarning(traceType, "chunkSize is too big.");
                return;
            }

            if (!string.IsNullOrEmpty(customPreamble))
            {
                TestabilityTrace.TraceSource.WriteInfo(traceType, "{0}: {1}", activityId, customPreamble);
            }

            TestabilityTrace.TraceSource.WriteInfo(traceType, "{0}: {1}", activityId, Preamble);

            string text = this.ToString();

            int nchunks = (int)Math.Ceiling(text.Length / (double)chunkSize);

            int i;
            for (i = 0; i < nchunks - 1; ++i)
            {
                TestabilityTrace.TraceSource.WriteInfo(traceType, "{0}: {1}", activityId, text.Substring(i * chunkSize, chunkSize));
            }

            TestabilityTrace.TraceSource.WriteInfo(traceType, "{0}: {1}", activityId, text.Substring(i * chunkSize));

            TestabilityTrace.TraceSource.WriteInfo(traceType, "{0}: {1}", activityId, Postamble);

            if (!string.IsNullOrEmpty(customPostamble))
            {
                TestabilityTrace.TraceSource.WriteInfo(traceType, "{0}: {1}", activityId, customPostamble);
            }
        }

        private IEnumerable<PartitionEntity> GetFaultablePartitions(bool? isStateful = null, bool? hasPersistedState = null)
        {
            List<PartitionEntity> faultTolerantPartitions = new List<PartitionEntity>();

            foreach (PartitionEntity partitionEntity in this.GetAllPartitions(isStateful, hasPersistedState, !this.ShouldFaultSystem))
            {
                if (partitionEntity.GetPartitionFaultTolerance() > 0)
                {
                    faultTolerantPartitions.Add(partitionEntity);
                }
            }

            return faultTolerantPartitions;
        }

        private IEnumerable<PartitionEntity> GetAllPartitions(bool? isStateful = null, bool? hasPersistedState = null, bool excludeSystemServices = true)
        {
            return this.GetAllServices().Where(service =>
            {
                bool toAdd = true;
                if (isStateful.HasValue)
                {
                    toAdd = service.IsStateful == isStateful;
                }

                if (hasPersistedState.HasValue)
                {
                    toAdd = service.HasPersistedState == hasPersistedState;
                }

                if (excludeSystemServices 
                    && service.ParentApplicationEntity.Application.ApplicationName.OriginalString.Equals(
                            Constants.SystemApplicationName, 
                            StringComparison.OrdinalIgnoreCase))
                {
                    toAdd = false;
                }

                return toAdd;
            })
            .SelectMany(serviceEntity => serviceEntity.PartitionList);
        }

        private IEnumerable<ServiceEntity> GetAllServices()
        {
            return this.Applications.SelectMany(app => app.ServiceList);
        }

        internal void PopulateNodeMaps(IEnumerable<NodeInfo> nodes)
        {
            foreach (var node in nodes)
            {
                NodeNameToNodeIdMap[node.NodeName] = node.NodeId;
                NodeIdToNodeNameMap[node.NodeId] = node.NodeName;
            }
        }

        internal void ApplyChaosTargetFilter(ChaosTargetFilter chaosTargetFilter)
        {
            if (chaosTargetFilter != null)
            {
                this.ChaosTargetFilterPresent = true;
                this.ApplyChaosTargetInclusionFilters(chaosTargetFilter);
            }
        }

        private void ApplyChaosTargetInclusionFilters(ChaosTargetFilter chaosTargetFilter)
        {
            this.ApplyNodeTypeInclusionFilter(chaosTargetFilter.NodeTypeInclusionList);
            this.ApplyApplicationInclusionFilter(chaosTargetFilter.ApplicationInclusionList);
        }

        private void ApplyNodeTypeInclusionFilter(IList<string> nodetypeInclusionList)
        {
            if (nodetypeInclusionList != null && nodetypeInclusionList.Any())
            {
                var includedNodes = this.Nodes.Where(
                    n => nodetypeInclusionList.Contains(n.CurrentNodeInfo.NodeType)).ToList();

                foreach (var node in includedNodes)
                {
                    node.ClearNodeOfExclusionFlags();
                }
            }
        }

        private void ApplyApplicationInclusionFilter(IList<string> applicationInclusionList)
        {
            if (applicationInclusionList != null && applicationInclusionList.Any())
            {
                 var includedApplications =
                        this.Applications.Where(
                            a => applicationInclusionList.Contains(a.Application.ApplicationName.OriginalString)).ToList();

                // Include all code packages belonging to the included applications for faulting
                var allCodepackages = this.GetAllCodePackages().ToList();

                var codepackageFromIncludedApplications =
                    allCodepackages.Where(cp => includedApplications.Contains(cp.ParentApplicationEntity)).ToList();
                codepackageFromIncludedApplications.ForEach(cp => cp.ClearCodePackageOfExclusionFlags());
            }
        }

        internal static bool IsSystemApplication(ApplicationEntity app)
        {
            return app.Application.ApplicationName.OriginalString.Equals(
                                                                        Constants.SystemApplicationName,
                                                                        StringComparison.OrdinalIgnoreCase);
        }

        internal static bool IsSystemServiceReplica(DeployedServiceReplica replica)
        {
            // Except for fabric:/System/InfrastructureService, for all other System service replicas, replica.ServiceName matches one of the
            // System service names in Constants.SystemServiceList. For fabric:/System/InfrastructureService the name may have a suffix
            // like fabric:/System/InfrastructureService/ABC, so  we are doing StartsWith here.
            return replica.ServiceName.OriginalString.StartsWith(Constants.SystemApplicationName)
                && Constants.SystemServiceList.Any(s => replica.ServiceName.OriginalString.StartsWith(s.OriginalString, StringComparison.Ordinal));
        }

        internal static bool IsDummyCodePackage(CodePackageEntity cp)
        {
            // This is only true for FM, CM, and NS
            return string.IsNullOrEmpty(cp.CodePackageResult.CodePackageName)
                   && string.IsNullOrEmpty(cp.CodePackageResult.ServiceManifestName);
        }

        /// <summary>
        /// Matches nodeName1 with nodeName2; if only one of these two node names have "nodeid:&lt;id-value&gt;" format,
        /// this method converts the other node name into that format as well and then matches (RDBug 6864934, 6720354)
        /// </summary>
        /// <param name="nodeName1">A node name</param>
        /// <param name="nodeName2">A node name</param>
        /// <returns>true if the node names match, else false</returns>
        /// <remarks>Look into RDBug 6864934 for more details</remarks>
        internal static bool MatchNodesByNameOrId(string nodeName1, string nodeName2)
        {
            if (nodeName1.Equals(nodeName2, StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }

            if (nodeName2.StartsWith(ChaosConstants.NodeNameUsingIdPrefix, StringComparison.OrdinalIgnoreCase))
            {
                if(!NodeNameToNodeIdMap.ContainsKey(nodeName1))
                {
                    return false;
                }

                // nodeName2 is in "nodeid:<id-value>" format, so convert nodeName1 into that format before matching
                string name1UsingId = ChaosConstants.NodeNameUsingIdPrefix + NodeNameToNodeIdMap[nodeName1];

                return name1UsingId.Equals(nodeName2, StringComparison.OrdinalIgnoreCase);
            }
            else if (nodeName1.StartsWith(ChaosConstants.NodeNameUsingIdPrefix, StringComparison.OrdinalIgnoreCase))
            {
                if (!NodeNameToNodeIdMap.ContainsKey(nodeName2))
                {
                    return false;
                }

                // nodeName1 is in "nodeid:<id-value>" format, so convert nodeName2 into that format before matching
                string name2UsingId = ChaosConstants.NodeNameUsingIdPrefix + NodeNameToNodeIdMap[nodeName2];

                return name2UsingId.Equals(nodeName1, StringComparison.OrdinalIgnoreCase);
            }

            return false;
        }
    }
}