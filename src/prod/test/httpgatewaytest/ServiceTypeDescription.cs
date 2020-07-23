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
    public class ServiceTypeDescription
    {
        public ServiceTypeDescription()
        {
            LoadMetrics = new List<ServiceLoadMetricDescription>();
            Extensions = new Dictionary<string, string>();
        }

        [DataMember]
        public System.Fabric.Query.ServiceKind ServiceKind
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
        public string PlacementConstraints
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
        public string ReplicaRestartWaitDurationInMilliSeconds
        {
            get;
            set;
        }

        [DataMember]
        public bool UseImplicitHost
        {
            get;
            set;
        }

        [DataMember]
        public List<ServiceLoadMetricDescription> LoadMetrics
        {
            get;
            set;
        }

        [DataMember]
        public Dictionary<string, string> Extensions
        {
            get;
            set;
        }
    }
}