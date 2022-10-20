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
    class ServicePartitionResult
    {
        [DataMember(IsRequired = true)]
        public System.Fabric.Query.ServiceKind ServiceKind
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public PartitionInformation PartitionInformation
        {
            get;
            set;
        }

        [DataMember]
        public int InstanceCount
        {
            get;
            set;
        }

        [DataMember]
        public int TargetReplicaSetSize
        {
            get;
            set;
        }

        [DataMember]
        public int MinReplicaSetSize
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public System.Fabric.Health.HealthState HealthState
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public System.Fabric.Query.ServicePartitionStatus PartitionStatus
        {
            get;
            set;
        }

        [DataMember]
        public string LastQuorumLossDurationInSeconds
        {
            get;
            set;
        }

        public TimeSpan LastQuorumLossDuration
        {
            get
            {
                return TimeSpan.FromSeconds(Convert.ToDouble(LastQuorumLossDurationInSeconds));
            }
        }
    }
}