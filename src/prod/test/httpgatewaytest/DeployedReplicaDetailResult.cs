// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.Serialization;

namespace System.Fabric.Test.HttpGatewayTest
{
    [Serializable]
    [DataContract]
    class DeployedReplicaDetail
    {
        public DeployedReplicaDetail()
        {
            this.ReportedLoad = new List<LoadMetricReportResult>();
        }

        [DataMember(IsRequired = true)]
        public System.Fabric.Query.ServiceKind ServiceKind { get; set; }

        [DataMember]
        public string InstanceId
        {
            get;
            set;
        }

        [DataMember]
        public string ReplicaId
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public Guid PartitionId { get; set; }

        [DataMember(IsRequired = true)]
        public string ServiceName { get; set; }

        [DataMember(IsRequired = true)]
        public Query.ServiceOperationName CurrentServiceOperation { get; set; }

        [DataMember(IsRequired = true)]
        public string CurrentServiceOperationStartTimeUtc { get; set; }

        [DataMember(IsRequired = true)]
        public List<LoadMetricReportResult> ReportedLoad { get; set; }

        [DataMember]
        public Query.ReplicatorOperationName CurrentReplicatorOperation { get; set; }

        [DataMember]
        public PartitionAccessStatus ReadStatus { get; set; }

        [DataMember]
        public PartitionAccessStatus WriteStatus { get; set; }

        [DataMember]
        public ReplicatorStatusResult ReplicatorStatus { get; set; }
    }
}