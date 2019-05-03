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
    class LoadMetricInformationResult
    {
        [DataMember(IsRequired = true)]
        public string Name { get; set; }

        [DataMember(IsRequired = true)]
        public bool IsBalancedBefore { get; internal set; }

        [DataMember(IsRequired = true)]
        public bool IsBalancedAfter { get; internal set; }

        [DataMember(IsRequired = true)]
        public double DeviationBefore { get; internal set; }

        [DataMember(IsRequired = true)]
        public double DeviationAfter { get; internal set; }

        [DataMember(IsRequired = true)]
        public double BalancingThreshold { get; internal set; }

        [DataMember(IsRequired = true)]
        public string Action { get; internal set; }

        [DataMember(IsRequired = true)]
        public long ActivityThreshold { get; internal set; }

        [DataMember(IsRequired = true)]
        public long ClusterCapacity { get; internal set; }

        [DataMember(IsRequired = true)]
        public long ClusterLoad { get; internal set; }

        [DataMember(IsRequired = true)]
        public long RemainingUnbufferedCapacity { get; internal set; }

        [DataMember(IsRequired = true)]
        public double NodeBufferPercentage { get; internal set; }

        [DataMember(IsRequired = true)]
        public long BufferedCapacity { get; internal set; }

        [DataMember(IsRequired = true)]
        public long RemainingBufferedCapacity { get; internal set; }

        [DataMember(IsRequired = true)]
        public bool IsClusterCapacityViolation { get; internal set; }

        [DataMember(IsRequired = true)]
        public long MinNodeLoadValue { get; internal set; }

        [DataMember(IsRequired = true)]
        public NodeId MinNodeLoadId { get; internal set; }

        [DataMember(IsRequired = true)]
        public long MaxNodeLoadValue { get; internal set; }

        [DataMember(IsRequired = true)]
        public NodeId MaxNodeLoadId { get; internal set; }

        [DataMember(IsRequired = true)]
        public double CurrentClusterLoad { get; internal set; }

        [DataMember(IsRequired = true)]
        public double BufferedClusterCapacityRemaining { get; internal set; }

        [DataMember(IsRequired = true)]
        public double ClusterCapacityRemaining { get; internal set; }

        [DataMember(IsRequired = true)]
        public double MaximumNodeLoad { get; internal set; }

        [DataMember(IsRequired = true)]
        public double MinimumNodeLoad { get; internal set; }
    }
}