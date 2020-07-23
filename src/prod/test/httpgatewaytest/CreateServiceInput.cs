// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.HttpGatewayTest
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Linq;
    using System.Text;
    using System.Runtime.Serialization;

    [Serializable]
    [DataContract]
    class CreateServiceInput
    {
        public CreateServiceInput()
        {
            ApplicationName = "";
            ServiceKind = Query.ServiceKind.Invalid;
            ServiceName = "";
            ServiceTypeName = "";
            PartitionDescription = new PartitionDescription();
            TargetReplicaSetSize = 0;
            MinReplicaSetSize = 0;
            HasPersistedState = false;
            PlacementConstraints = "";
            ReplicaRestartWaitDurationSeconds = 0;
            ServiceLoadMetrics = new List<ServiceLoadMetricDescription>();
            CorrelationScheme = new List<ServiceCorrelationDescription>();
            InitializationData = new Byte[]{};
            ServicePackageActivationMode = ServicePackageActivationMode.SharedProcess;
            ScalingPolicies = new List<ScalingPolicyDescriptionTest>();
        }

        [DataMember]
        public System.Fabric.Query.ServiceKind ServiceKind
        {
            get;
            set;
        }

        [DataMember]
        public string ApplicationName
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
        public Byte[] InitializationData
        {
            get;
            set;
        }

        [DataMember]
        public PartitionDescription PartitionDescription
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

        [DataMember]
        public bool HasPersistedState
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
        public string PlacementConstraints
        {
            get;
            set;
        }

        [DataMember]
        public List<ServiceCorrelationDescription> CorrelationScheme
        {
            get;
            set;
        }

        [DataMember]
        public Int32 ReplicaRestartWaitDurationSeconds
        {
            get;
            set;
        }

        [DataMember]
        public List<ServiceLoadMetricDescription> ServiceLoadMetrics
        {
            get;
            set;
        }

        [DataMember]
        public ServicePackageActivationMode ServicePackageActivationMode
        {
            get;
            set;
        }

        [DataMember]
        public List<ScalingPolicyDescriptionTest> ScalingPolicies
        {
            get;
            set;
        }
    }
}