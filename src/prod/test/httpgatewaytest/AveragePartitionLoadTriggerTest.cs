// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.Serialization;
using System.Fabric;

namespace System.Fabric.Test.HttpGatewayTest
{
    [Serializable]
    [DataContract]
    class AveragePartitionLoadTriggerTest
    {
        public AveragePartitionLoadTriggerTest()
        {
            Kind = Fabric.Description.ScalingTriggerKind.AveragePartitionLoadTrigger;
        }

        [DataMember]
        public System.Fabric.Description.ScalingTriggerKind Kind
        {
            get;
            set;
        }

        [DataMember]
        public int ScaleIntervalInSeconds
        {
            get;
            set;
        }

        [DataMember]
        public string MetricName
        {
            get;
            set;
        }

        [DataMember]
        public double LowerLoadThreshold
        {
            get;
            set;
        }

        [DataMember]
        public double UpperLoadThreshold
        {
            get;
            set;
        }
    }
}