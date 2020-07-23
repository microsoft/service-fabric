// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Runtime.Serialization;
using System.Fabric.Query;

namespace System.Fabric.Test.HttpGatewayTest
{
    [Serializable]
    [DataContract]
    class RemoteReplicatorStatusResult
    {
        [DataMember(IsRequired = true)]
        public long ReplicaId { get; set; }

        [DataMember(IsRequired = true)]
        public string LastAcknowledgementProcessedTimeUtc { get; set; }

        [DataMember(IsRequired = true)]
        public long LastReceivedReplicationSequenceNumber { get; set; }

        [DataMember(IsRequired = true)]
        public long LastAppliedReplicationSequenceNumber { get; set; }

        [DataMember(IsRequired = true)]
        public bool IsInBuild { get; set; }

        [DataMember(IsRequired = true)]
        public long LastReceivedCopySequenceNumber { get; set; }

        [DataMember(IsRequired = true)]
        public long LastAppliedCopySequenceNumber { get; set; }

        [DataMember(IsRequired = true)]
        public RemoteReplicatorAcknowledgementStatus RemoteReplicatorAcknowledgementStatus { get; set; }
    }
}