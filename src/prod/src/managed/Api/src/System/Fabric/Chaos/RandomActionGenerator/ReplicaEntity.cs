// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Fabric.Chaos.Common;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Linq;

    internal class ReplicaEntity
    {
        private const string TraceType = "Chaos.ReplicaEntity";

        public ReplicaEntity(Replica replica, PartitionEntity partitionEntity)
        {
            this.Replica = replica;
            this.ParentPartitionEntity = partitionEntity;
            this.ReplicaFlags = ClusterEntityFlags.Excluded;
        }

        public Replica Replica { get; private set; }

        public PartitionEntity ParentPartitionEntity { get; private set; }

        public ClusterEntityFlags ReplicaFlags { get; private set; }

        public void MoveReplica(string nodeName, Guid activityId = default(Guid))
        {
            // Confirm replica is available to fault
            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.MoveReplica_Unavailable_TelemetryId,
                !this.IsAvailableToFault,
                string.Format(
                    StringResources.ChaosEngineError_ReplicaEntity_ReplicaInTransition,
                    this),
                this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot);

            // If not in unsafe mode then confirm that the fault tolerance of this partition is at least 1
            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.MoveReplica_PartitionNotFaultTolerant_TelemetryId,
                this.ParentPartitionEntity.GetPartitionFaultTolerance() <= 0 && !this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot.UnsafeModeEnabled,
                string.Format(
                    StringResources.ChaosEngineError_ReplicaEntity_PartitionNotFaultTolerantInSafeMode,
                    this.ParentPartitionEntity.Partition.PartitionId(),
                    this),
                this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot);

            TestabilityTrace.TraceSource.WriteInfo(
                TraceType,
                "{0}: Marking current replica {1} as Faulted with MarkReplicaAsInTransitionInternal",
                    activityId,
                    this);

            // Mark current replica as faulted
            this.MarkReplicaAsInTransitionInternal(activityId);


            //
            // Mark the node moving to and any replica/code package on new node as unavailable
            //
            var clusterNode = this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot.Nodes.FirstOrDefault(n => n.CurrentNodeInfo.NodeName == nodeName);

            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.MoveReplica_InvalidNode_TelemetryId,
                clusterNode == null,
                string.Format(
                    StringResources.ChaosEngineError_ReplicaEntity_NodeInfoNotFound,
                    nodeName),
                this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot);

            TestabilityTrace.TraceSource.WriteInfo(
                            TraceType,
                            "{0}: Marking node {1} to which the replica {2} is moving as Unavailable",
                                activityId,
                                clusterNode.CurrentNodeInfo.NodeName,
                                this);

            // The new node where the current replica is moving to is marked as used and cannot be used for any other node faults
            // after this fault
            clusterNode.MarkNodeAsUnavailableForFaults();

            // check if new node already has a replica (In case of MovePrimary) and if so fault it. 
            var newNodeReplica = this.ParentPartitionEntity.FindReplicaEntityGivenNodeName(nodeName);
            if (newNodeReplica != null)
            {
                TestabilityTrace.TraceSource.WriteInfo(
                            TraceType,
                            "{0}: Marking newNodeReplica {1} as Faulted with MarkReplicaAsInTransitionInternal",
                                activityId,
                                newNodeReplica);

                // We call the internal version of the method since it is possible the replica has already been faulted by the call to 
                // MarkReplicaAsInTransitionInternal for this current replica due to GetPartitionFaultTolerance() <= 0
                newNodeReplica.MarkReplicaAsInTransitionInternal(activityId);

                // Since we are moving to this new replica we want to mark its code package as not available for faults
                var newNodeReplicaCodePackage = this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.GetCodePackagEntityForReplica(newNodeReplica);
                if (newNodeReplicaCodePackage != null)
                {
                    TestabilityTrace.TraceSource.WriteInfo(
                            TraceType,
                            "{0}: Marking newNodeReplicaCodepackage {1} as Unavailable",
                                activityId,
                                newNodeReplicaCodePackage);

                    newNodeReplicaCodePackage.MarkCodePackageAsUnavailableForFaults();
                }
            }
        }

        public void MarkReplicaAsUnsafeToFault()
        {
            this.ReplicaFlags |= ClusterEntityFlags.Unsafe;
        }

        public void FaultReplica(bool faultedDueToCodePackageFault, Guid activityId = default(Guid))
        {
            // Confirm replica is available to fault
            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.FaultReplica_ReplicaInTransition_TelemetryId,
                !faultedDueToCodePackageFault && !this.IsAvailableToFault,
                string.Format(
                    StringResources.ChaosEngineError_ReplicaEntity_ReplicaInTransition,
                    this),
                this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot);

            // If not in unsafe mode then confirm that the fault tolerance of this partition is at least 1
            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.FaultReplica_PartitionNotFaultTolerant_TelemetryId,
                this.ParentPartitionEntity.GetPartitionFaultTolerance() <= 0 && !this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot.UnsafeModeEnabled,
                string.Format(
                    StringResources.ChaosEngineError_ReplicaEntity_PartitionNotFaultTolerantInSafeMode,
                    this.ParentPartitionEntity.Partition.PartitionId(),
                    this),
                this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot);

            // Mark current replicas as faulted
            this.MarkReplicaAsInTransitionInternal(activityId);
        }

        public void MarkReplicaAsExcludedFromFault()
        {
            this.ReplicaFlags |= ClusterEntityFlags.Excluded;
        }

        public bool IsExcludedForFaults()
        {
            return this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot.ChaosTargetFilterPresent && this.ReplicaFlags.HasFlag(ClusterEntityFlags.Excluded);
        }

        public void ClearReplicaOfExclusionFlags()
        {
            this.ReplicaFlags &= ~ClusterEntityFlags.Excluded;
        }

        public bool IsAvailableToFault
        {
            get
            {
                bool isInTransition = this.ReplicaFlags.HasFlag(ClusterEntityFlags.Faulted) ||
                    this.ReplicaFlags.HasFlag(ClusterEntityFlags.Unavailable) ||
                    this.IsExcludedForFaults();

                bool isUpgrading = this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.Application.ApplicationStatus == ApplicationStatus.Upgrading;

                return (this.Replica.ReplicaStatus == ServiceReplicaStatus.Ready
                    && !isInTransition
                    && !isUpgrading);
            }
        }

        public bool IsUnsafeToFault
        {
            get
            {
                bool isUnsafe = this.ReplicaFlags.HasFlag(ClusterEntityFlags.Unsafe);

                return isUnsafe || !this.IsAvailableToFault;
            }
        }

        public override string ToString()
        {
            return string.Format("      ReplicaID: {0}, PartitionId: {1} IsAvailableToFault: {2}, NodeName: {3}, Status: {4}, Health: {5}, IsUnsafeToFault: {6}, ReplicaFlags: {7}",
                this.Replica.Id,
                this.ParentPartitionEntity.Partition.PartitionId(),
                this.IsAvailableToFault,
                this.Replica.NodeName,
                this.Replica.ReplicaStatus,
                this.Replica.HealthState,
                this.IsUnsafeToFault,
                this.ReplicaFlags);
        }

        private void MarkReplicaAsInTransitionInternal(Guid activityId = default(Guid))
        {
            TestabilityTrace.TraceSource.WriteInfo(
                            TraceType,
                            "{0}: Marking replica {1} as faulted",
                                activityId,
                                this);

            this.ReplicaFlags |= ClusterEntityFlags.Faulted;
            var isPartitionFaultTolerant = this.ParentPartitionEntity.GetPartitionFaultTolerance() > 0;

            if (!isPartitionFaultTolerant)
            {
                // If partition is no longer fault tolerant after this fault then mark all corresponding replicas
                // in the partition as unsafe to fault. The corresponding nodes and code packages are also marked as unsafe to fault
                foreach (ReplicaEntity replica in this.ParentPartitionEntity.ReplicaList)
                {
                    // Unsafe to fault this partition anymore. Mark any node/codepackage/replica for this partition as unsafe to fault
                    var clusterNode =
                        this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot.Nodes.FindMatchingNodeEntity(replica.Replica.NodeName);

                    ChaosUtility.ThrowOrAssertIfTrue(
                        ChaosConstants.MarkReplciaAsInTransition_UnavailableTargetNode_TelemetryId,
                        clusterNode == null,
                        string.Format(
                            StringResources.ChaosEngineError_ReplicaEntity_UnavaliableNode,
                            replica.Replica.NodeName,
                            replica),
                        this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot);

                    TestabilityTrace.TraceSource.WriteInfo(
                        TraceType, 
                        "{0}: Marking node '{1}' as Unsafe for replica '{2}' due to partition being not-fault-tolerant", 
                            activityId,
                            clusterNode.CurrentNodeInfo.NodeName, 
                            replica);

                    clusterNode.MarkNodeAsUnsafeToFault();

                    var replicaCodePackage = this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.GetCodePackagEntityForReplica(replica);
                    if (replicaCodePackage != null)
                    {
                        TestabilityTrace.TraceSource.WriteInfo(
                            TraceType,
                            "{0}: Marking replicaCodepackage '{1}' as Unsafe for due to partition '{2}' being not-fault-tolerant due to faulting replica '{3}'",
                                activityId,
                                replicaCodePackage,
                                replica.ParentPartitionEntity.Partition.PartitionId(),
                                replica.Replica.Id);

                        replicaCodePackage.MarkCodePackageAsUnsafeToFault();
                    }

                    TestabilityTrace.TraceSource.WriteInfo(
                        TraceType,
                        "{0}: Marking replica '{1}' as Unsafe due to partition being not-fault-tolerant",
                            activityId,
                            replica);

                    replica.MarkReplicaAsUnsafeToFault();
                }
            }

            // Mark the node on which this replica is as used for faults so that it cannot be used for any node faults
            var currentReplicaNodeEntity =
                this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot.Nodes.FindMatchingNodeEntity(this.Replica.NodeName);

            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.MarkReplicaAsInTransition_UnavailableSourceNode_TelemetryId,
                currentReplicaNodeEntity == null,
                string.Format(
                    StringResources.ChaosEngineError_ReplicaEntity_UnavaliableNode,
                    this.Replica.NodeName,
                    this),
                this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot);

            // Assert (or throw) if higher level entities are already faulted
            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.MarkReplicaAsInTransition_NodeFaulted_TelemetryId,
                currentReplicaNodeEntity.NodeFlags.HasFlag(ClusterEntityFlags.Faulted),
                string.Format(
                    StringResources.ChaosEngineError_ReplicaEntity_FaultedNode,
                    currentReplicaNodeEntity.CurrentNodeInfo.NodeName,
                    this),
                this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot);

            TestabilityTrace.TraceSource.WriteInfo(
                TraceType,
                "{0}: Marking currentReplicaNodeEntity {1} as Unavailable for replica {2} due to partition being not-fault-tolerant",
                    activityId,
                    currentReplicaNodeEntity.CurrentNodeInfo.NodeName,
                    this);

            currentReplicaNodeEntity.MarkNodeAsUnavailableForFaults();

            var currentReplicaCodePackage = this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.GetCodePackagEntityForReplica(this);
            if (currentReplicaCodePackage != null)
            {
                TestabilityTrace.TraceSource.WriteInfo(
                            TraceType,
                            "{0}: Marking currentReplicaCodePackage {1} as unavailable due to faulting replica {2}",
                                activityId,
                                currentReplicaCodePackage,
                                this);

                currentReplicaCodePackage.MarkCodePackageAsUnavailableForFaults();

                ChaosUtility.ThrowOrAssertIfTrue(
                    ChaosConstants.MarkReplicaAsInTransition_CodepackageFaulted_TelemetryId,
                    currentReplicaCodePackage.CodePackageFlags.HasFlag(ClusterEntityFlags.Faulted),
                    string.Format(
                        StringResources.ChaosEngineError_ReplicaEntity_FaultCodePackage,
                        currentReplicaCodePackage,
                        currentReplicaCodePackage.NodeName,
                        this),
                    this.ParentPartitionEntity.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot);
            }
        }
    }
}