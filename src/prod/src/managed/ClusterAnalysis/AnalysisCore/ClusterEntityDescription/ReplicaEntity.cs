// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.ClusterEntityDescription
{
    using System;
    using System.Fabric;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Runtime.Serialization;

    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// Wrapper over Cluster Replica for easy unit testability. All props are internal so that
    /// they can be easily set by Unit test.
    /// </summary>
    [DataContract]
    public class ReplicaEntity : BaseEntity
    {
        /// <summary>
        /// </summary>
        /// <param name="replicaObj"></param>
        /// <param name="partitionGuid"></param>
        public ReplicaEntity(Replica replicaObj, Guid partitionGuid)
        {
            Assert.IsNotNull(replicaObj, "Replica object can't be null");
            this.Id = replicaObj.Id;
            this.HealthState = replicaObj.HealthState;
            this.ReplicaStatus = replicaObj.ReplicaStatus;
            this.LastInBuildDuration = replicaObj.LastInBuildDuration;
            this.NodeName = replicaObj.NodeName;
            this.ServiceKind = replicaObj.ServiceKind;
            this.ReplicaRole = ReplicaRole.Unknown;
            this.PartitionId = partitionGuid;

            // Check if stateful service and if yes, assign appropriate replica role.
            var statefulReplica = replicaObj as StatefulServiceReplica;
            if (statefulReplica != null)
            {
                this.ReplicaRole = statefulReplica.ReplicaRole;
            }
        }

        internal ReplicaEntity()
        {
        }

        [DataMember(IsRequired = true)]
        public long Id { get; internal set; }

        [DataMember(IsRequired = true)]
        public Guid PartitionId { get; internal set; }

        [DataMember(IsRequired = true)]
        public HealthState HealthState { get; internal set; }

        [DataMember(IsRequired = true)]
        public TimeSpan LastInBuildDuration { get; internal set; }

        [DataMember(IsRequired = true)]
        public string NodeName { get; internal set; }

        [DataMember(IsRequired = true)]
        public ServiceReplicaStatus ReplicaStatus { get; internal set; }

        [DataMember(IsRequired = true)]
        public ServiceKind ServiceKind { get; internal set; }

        [DataMember(IsRequired = true)]
        public ReplicaRole ReplicaRole { get; internal set; }

        public override string ToString()
        {
            return string.Format(
                "ReplicaId: '{0}', HealthState: '{1}', ReplicaStatus: '{2}', NodeName: '{3}', PartitionId: '{4}'",
                this.Id,
                this.HealthState,
                this.ReplicaStatus,
                this.NodeName,
                this.PartitionId);
        }

        /// <inheritdoc />
        public override bool Equals(BaseEntity other)
        {
            Assert.IsNotNull(other);
            var otherObj = other as ReplicaEntity;
            if (otherObj == null)
            {
                return false;
            }

            return this.Id == otherObj.Id && this.PartitionId == otherObj.PartitionId;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = (hash * 23) + this.Id.GetHashCode();
                hash = (hash * 23) + this.PartitionId.GetHashCode();
                return hash;
            }
        }

        /// <inheritdoc />
        public override int GetUniqueIdentity()
        {
            return this.GetHashCode();
        }
    }
}