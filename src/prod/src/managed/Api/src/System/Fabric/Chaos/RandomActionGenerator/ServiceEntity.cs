// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Collections.Generic;
    using System.Fabric.Query;
    using System.Text;

    internal class ServiceEntity
    {
        public ServiceEntity(Service service, ApplicationEntity applicationEntity)
        {
            this.Service = service;
            this.ParentApplicationEntity = applicationEntity;
            this.PartitionList = new List<PartitionEntity>();
        }

        // Could be null initially.
        public ApplicationEntity ParentApplicationEntity { get; internal set; }

        // Null is not expected after creation of object, though it could be.
        public Service Service { get; internal set; }

        // Will not be null, could be empty list.
        public IList<PartitionEntity> PartitionList { get; private set; }

        public bool IsStateful 
        {
            get
            {
                return this.Service.ServiceKind == Query.ServiceKind.Stateful;
            }
        }

        public bool HasPersistedState
        {
            get
            {
                return (this.Service is StatefulService) && (this.Service as StatefulService).HasPersistedState;
            }
        }

        // Create PartitionEntity from Query.Partition and add it to this node.
        public PartitionEntity AddPartition(Partition partition)
        {
            PartitionEntity partitionEntity = new PartitionEntity(partition, this);
            this.PartitionList.Add(partitionEntity);
            return partitionEntity;
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendLine(string.Format("    Start for SvcName: {0}, SvcTypeName: {1}, Status: {2}, HealthState: {3}",
                this.Service.ServiceName,
                this.Service.ServiceTypeName,
                this.Service.ServiceStatus,
                this.Service.HealthState));

            foreach (var p in this.PartitionList)
            {
                sb.AppendLine(p.ToString());
            }

            sb.AppendLine(string.Format("    End for SvcName: {0}", this.Service.ServiceName));

            return sb.ToString();
        }
    }
}