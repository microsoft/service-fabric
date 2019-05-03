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
    class ReplicatorQueueStatusResult
    {
        [DataMember(IsRequired = true)]
        public long QueueUtilizationPercentage { get; set; }

        [DataMember(IsRequired = true)]
        public long QueueMemorySize { get; set; }

        [DataMember(IsRequired = true)]
        public long FirstSequenceNumber { get; set; }

        [DataMember(IsRequired = true)]
        public long CompletedSequenceNumber { get; set; }

        [DataMember(IsRequired = true)]
        public long CommittedSequenceNumber { get; set; }

        [DataMember(IsRequired = true)]
        public long LastSequenceNumber { get; set; }
    }
}