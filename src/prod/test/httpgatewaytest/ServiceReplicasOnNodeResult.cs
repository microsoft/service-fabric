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
    public class ServiceReplicasOnNodeResult
    {
        [DataMember]
        public System.Fabric.Query.ServiceKind ServiceKind
        {
            get;
            set;
        }

        [DataMember]
        public string ServiceName
        {
            get;
            set;
        }

        [DataMember]
        public string ServiceTypeName
        {
            get;
            set;
        }

        [DataMember]
        public string ServiceManifestName
        {
            get;
            set;
        }

        [DataMember]
        public string ServiceManifestVersion
        {
            get;
            set;
        }

        [DataMember]
        public string CodePackageName
        {
            get;
            set;
        }

        [DataMember]
        public Guid PartitionId
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
        public System.Fabric.Query.ServiceReplicaStatus ReplicaStatus
        {
            get;
            set;
        }

        [DataMember]
        public string Address
        {
            get;
            set;
        }

        [DataMember]
        public string ServicePackageActivationId
        {
            get;
            set;
        }
    }
}