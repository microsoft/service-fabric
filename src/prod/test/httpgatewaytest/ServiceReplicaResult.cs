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
    class ServiceReplicaResult
    {
        [DataMember(IsRequired = true)]
        public System.Fabric.Query.ServiceKind ServiceKind
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public System.Fabric.Query.ServiceReplicaStatus ReplicaStatus
        {
            get;
            set;
        }

        [DataMember]
        public System.Fabric.ReplicaRole ReplicaRole
        {
            get;
            set;
        }

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
        public string Address
        {
            get;
            set;
        }

        [DataMember(IsRequired=true)]
        public string NodeName
        {
            get;
            set;
        }

        [DataMember(IsRequired=true)]
        public System.Fabric.Health.HealthState HealthState
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string LastInBuildDurationInSeconds
        {
            get;
            set;
        }

        public TimeSpan LastInBuildDuration
        {
            get
            {
                return TimeSpan.FromSeconds(Convert.ToDouble(LastInBuildDurationInSeconds));
            }
        }
    }
}