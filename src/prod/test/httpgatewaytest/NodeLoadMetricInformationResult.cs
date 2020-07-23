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
    class NodeLoadMetricInformationResult
    {
        [DataMember(IsRequired = true)]
        public string Name { get; set; }

        [DataMember(IsRequired = true)]
        public int NodeCapacity { get; set; }
        
        [DataMember(IsRequired = true)]
        public int NodeLoad { get; set; }

        [DataMember(IsRequired = true)]
        public int NodeRemainingCapacity { get; set; }

        [DataMember(IsRequired = true)]
        public bool IsCapacityViolation { get; set; }

        [DataMember(IsRequired = true)]
        public int NodeBufferedCapacity { get; set; }

        [DataMember(IsRequired = true)]
        public int NodeRemainingBufferedCapacity { get; set; }

        [DataMember(IsRequired = true)]
        public double CurrentNodeLoad { get; set; }

        [DataMember(IsRequired = true)]
        public double NodeCapacityRemaining { get; set; }

        [DataMember(IsRequired = true)]
        public double BufferedNodeCapacityRemaining { get; set; }
    }
}