// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Common;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Linq;

    internal sealed class CodePackageEntity
    {
        private const string TraceType = "Chaos.CodePackageEntity";

        private readonly DeployedServicePackageHealth health = null;

        public CodePackageEntity(DeployedCodePackage codePackageResult, string nodeName, ApplicationEntity applicationEntity, DeployedServicePackageHealth health)
        {
            this.CodePackageResult = codePackageResult;
            this.NodeName = nodeName;
            this.ParentApplicationEntity = applicationEntity;
            this.DeployedPartitions = new List<PartitionEntity>();
            this.CodePackageFlags = ClusterEntityFlags.Excluded;
            this.health = health;
        }

        public ApplicationEntity ParentApplicationEntity { get; private set; }

        public DeployedCodePackage CodePackageResult { get; private set; }

        public DeployedServicePackageHealth Health
        {
            get
            {
                return this.health;
            }
        }

        public string NodeName { get; private set; }

        public bool IsAvailableToFault
        {
            get
            {
                var isInTransition = this.CodePackageFlags.HasFlag(ClusterEntityFlags.Faulted)
                                     || this.CodePackageFlags.HasFlag(ClusterEntityFlags.Unavailable)
                                     || this.IsExcludedForFaults();

                return !isInTransition;
            }
        }

        public bool IsUnsafeToFault
        {
            get
            {
                var isUnsafe = this.CodePackageFlags.HasFlag(ClusterEntityFlags.Unsafe);

                return isUnsafe || !this.IsAvailableToFault;
            }
        }

        public ClusterEntityFlags CodePackageFlags { get; private set; }

        public IList<PartitionEntity> DeployedPartitions { get; private set; }

        public void MarkCodePackageAsUnsafeToFault()
        {
            this.CodePackageFlags |= ClusterEntityFlags.Unsafe;
        }

        public void MarkCodePackageAsUnavailableForFaults()
        {
            this.CodePackageFlags |= ClusterEntityFlags.Unavailable;
        }

        public void MarkCodePackageAsExcludedFromFaults()
        {
            this.CodePackageFlags |= ClusterEntityFlags.Excluded;
        }

        public bool IsExcludedForFaults()
        {
            return this.ParentApplicationEntity.ClusterSnapshot.ChaosTargetFilterPresent && this.CodePackageFlags.HasFlag(ClusterEntityFlags.Excluded);
        }

        public void ClearCodePackageOfExclusionFlags()
        {
            if (!this.IsExcludedForFaults())
            {
                return;
            }

            foreach (var partitionEntity in this.DeployedPartitions)
            {
                var deployedReplica = partitionEntity.FindReplicaEntityGivenNodeName(this.NodeName);

                ChaosUtility.ThrowOrAssertIfTrue(
                    ChaosConstants.ClearCodePackageOfExclusionFlags_DeployedReplicaNotFound_TelemetryId,
                    deployedReplica == null,
                    string.Format(
                        StringResources.ChaosEngineError_ClearCodePackageOfExclusionFlags_DeployedReplicaNotFound,
                        this,
                        this.NodeName,
                        partitionEntity),
                    this.ParentApplicationEntity.ClusterSnapshot);

                deployedReplica.ClearReplicaOfExclusionFlags();
            }

            this.CodePackageFlags &= ~ClusterEntityFlags.Excluded;
        }

        public void FaultCodePackage(Guid activityId = default(Guid))
        {
            // Confirm first that neither the code package nor any of its children entities is faulted
            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.FaultCodePackage_CodepackageEntityInTransition_TelemetryId,
                !this.IsAvailableToFault,
                string.Format(
                    StringResources.ChaosEngineError_CodePackageEntityInTransition,
                    this),
                this.ParentApplicationEntity.ClusterSnapshot);

            var node = this.ParentApplicationEntity.ClusterSnapshot.Nodes.FirstOrDefault(n => n.CurrentNodeInfo.NodeName == this.NodeName);
            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.FaultCodePackage_NullNode_TelemetryId,
                node == null,
                string.Format(
                    StringResources.ChaosEngineError_CodePackageEntity_NullNode,
                    this.NodeName),
                this.ParentApplicationEntity.ClusterSnapshot);

            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.FaultCodePackage_FaultedNode_TelemetryId,
                node.NodeFlags.HasFlag(ClusterEntityFlags.Faulted),
                string.Format(
                    StringResources.ChaosEngineError_CodePackageEntity_FaultedNode,
                    this.NodeName,
                    this),
                this.ParentApplicationEntity.ClusterSnapshot);

            // Here we fault all the replicas in this code package so that they are marked correctly and the 
            // fault generator does not use them again
            foreach (var partitionEntity in this.DeployedPartitions)
            {
                // If !UnsafeModeEnabled then the fault tolerance of any partition hosted in this code package should not be at or
                // below 0 i.e. it should at least be 1
                ChaosUtility.ThrowOrAssertIfTrue(
                    ChaosConstants.FaultCodePackage_PartitionNotFaultTolerant_TelemetryId,
                    partitionEntity.GetPartitionFaultTolerance() <= 0 && !this.ParentApplicationEntity.ClusterSnapshot.UnsafeModeEnabled,
                    string.Format(
                        StringResources.ChaosEngineError_CodePackageEntity_PartitionNotFaultTolerantInSafeMode,
                        partitionEntity.Partition.PartitionId(),
                        this),
                    this.ParentApplicationEntity.ClusterSnapshot);

                // Fault the corresponding deployed replica so that it cannot be reused for faults
                var deployedReplica = partitionEntity.FindReplicaEntityGivenNodeName(this.NodeName);
                if (deployedReplica != null)
                {
                    TestabilityTrace.TraceSource.WriteInfo(
                        TraceType,
                        "{0}: Going to fault replica '{1}' due to faulting codepackage '{2}'",
                            activityId,
                            deployedReplica,
                            this);

                    deployedReplica.FaultReplica(true/*faultedDueToCodePackageFault*/, activityId);
                }
            }

            TestabilityTrace.TraceSource.WriteInfo(
                        TraceType, 
                        "{0}: Marking node '{1}' due to faulting codepackage '{2}'",
                            activityId,
                            node,
                            this);

            // Mark the node the code package is hosted on as unavailable so that it cannot be used for any 
            // other faults in this iteration
            node.MarkNodeAsUnavailableForFaults();

            TestabilityTrace.TraceSource.WriteInfo(
                        TraceType,
                        "{0}: Marking codepackage '{1}' as faulted",
                            activityId,
                            this);

            // Mark code package as faulted
            this.CodePackageFlags |= ClusterEntityFlags.Faulted;
        }

        public override string ToString()
        {
            return string.Format(
                " CPName: {0}, Status: {1}, NodeName: {2}, IsAvailableToFault: {3}, CodePackageFlags: {4}, AppName: {5}, ServiceManifestName: {6}, ServicePackageActivationId: {7}, Version: {8}, HealthState: {9}",
                this.CodePackageResult.CodePackageName,
                this.CodePackageResult.DeployedCodePackageStatus,
                this.NodeName,
                this.IsAvailableToFault,
                this.CodePackageFlags,
                this.ParentApplicationEntity.Application.ApplicationName,
                this.CodePackageResult.ServiceManifestName,
                this.CodePackageResult.ServicePackageActivationId,
                this.CodePackageResult.CodePackageVersion,
                this.Health.AggregatedHealthState);
        }
    }
}