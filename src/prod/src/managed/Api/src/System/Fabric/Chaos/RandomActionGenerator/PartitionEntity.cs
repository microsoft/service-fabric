// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Query;
    using System.Linq;
    using System.Text;

    internal class PartitionEntity
    {
        public PartitionEntity(Partition partition, ServiceEntity serviceEntity)
        {
            this.Partition = partition;
            this.ParentServiceEntity = serviceEntity;

            if (partition is StatefulServicePartition)
            {
                var sfPartition = partition as StatefulServicePartition;
                this.IsStateful = true;
                this.TargetReplicaSetSize = sfPartition.TargetReplicaSetSize;
                this.MinReplicaSetSize = sfPartition.MinReplicaSetSize;
            }
            else
            {
                var slPartition = partition as StatelessServicePartition;
                this.IsStateful = false;
                this.TargetReplicaSetSize = slPartition.InstanceCount;
                this.MinReplicaSetSize = 1;
            }

            this.ReplicaList = new List<ReplicaEntity>();
        }

        public ServiceEntity ParentServiceEntity { get; private set; }

        public Partition Partition { get; private set; }

        public Guid Guid
        {
            get
            {
                return this.Partition.PartitionInformation.Id;
            }
        }

        public bool IsStateful { get; private set; }

        public long TargetReplicaSetSize { get; private set; }

        public long MinReplicaSetSize { get; private set; }

        public long MinLiveReplicaThreshold
        {
            get
            {
                return this.IsStateful
                    ? FabricCluster.GetWriteQuorumSize(this.TargetReplicaSetSize)
                    : 0;
            }
        }

        public IList<ReplicaEntity> ReplicaList { get; private set; }

        public long GetPartitionFaultTolerance()
        {
            return this.GetFaultReadyReplicaList().Count - this.MinLiveReplicaThreshold;
        }

        /// <summary>
        ///  Taking safe mode into account, returns the list of replicas which are ready to be faulted.
        /// </summary>
        /// <returns>Readonly list of faultready replicas</returns>
        public IReadOnlyCollection<ReplicaEntity> GetFaultReadyReplicaList()
        {
            return this.ParentServiceEntity.ParentApplicationEntity.ClusterSnapshot.UnsafeModeEnabled ?
                this.GetAvailableReplicaList()
                : this.GetSafeReplicaList();
        }

        private IReadOnlyCollection<ReplicaEntity> GetAvailableReplicaList()
        {
            var liveReplicas = this.ReplicaList.Where(replica => replica.IsAvailableToFault).ToList();

            return liveReplicas.AsReadOnly();
        }

        private IReadOnlyCollection<ReplicaEntity> GetSafeReplicaList()
        {
            var liveReplicas = this.ReplicaList.Where(replica => !replica.IsUnsafeToFault).ToList();

            return liveReplicas.AsReadOnly();
        }

        // Create ReplicaEntity from Query.Replica and add it to this node.
        public ReplicaEntity AddReplica(Replica replica)
        {
            ReplicaEntity replicaEntity = new ReplicaEntity(replica, this);
            this.ReplicaList.Add(replicaEntity);
            return replicaEntity;
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendLine(
                string.Format(
                    "     Start for PID: {0}, FaultTolerance: {1}, T/M/mL: {2}/{3}/{4}, stateful: {5}",
                    this.Guid,
                    this.GetPartitionFaultTolerance(),
                    this.TargetReplicaSetSize,
                    this.MinReplicaSetSize,
                    this.MinLiveReplicaThreshold,
                    this.IsStateful));

            sb.AppendLine(this.GetReplicasAsString(this.ReplicaList));
            sb.AppendLine(string.Format("     End for PID: {0}", this.Guid));

            return sb.ToString();
        }

        internal ReplicaEntity FindReplicaEntityGivenNodeName(string nodeName)
        {
            Requires.Argument("nodeName", nodeName).NotNullOrEmpty();

            return this.ReplicaList.FirstOrDefault(r => ClusterStateSnapshot.MatchNodesByNameOrId(r.Replica.NodeName, nodeName));
        }

        private string GetReplicasAsString(IList<ReplicaEntity> replicaEntities)
        {
            StringBuilder sb = new StringBuilder();
            foreach (var replica in replicaEntities)
            {
                sb.AppendLine(replica.ToString());
            }

            return sb.ToString();
        }
    }
}