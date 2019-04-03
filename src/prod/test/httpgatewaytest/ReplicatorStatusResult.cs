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
    class ReplicatorStatusBase
    {
        public ReplicatorStatusBase(ReplicaRole role)
        {
            Kind = role;
        }

        public ReplicaRole Kind;
    }

    [Serializable]
    [DataContract]
    class ReplicatorStatusResult
    {
        ReplicatorStatusResult()
        {
            RemoteReplicators = new List<RemoteReplicatorStatusResult>();
        }

        [DataMember(IsRequired = true)]
        public ReplicaRole Kind { get; set; }

        [DataMember]
        public IList<RemoteReplicatorStatusResult> RemoteReplicators { get; set; }

        [DataMember]
        public ReplicatorQueueStatusResult ReplicationQueueStatus { get; set; }

        [DataMember]
        public string LastReplicationOperationReceivedTimeUtc { get; set; }

        [DataMember]
        public bool IsInBuild { get; set; }

        [DataMember]
        public ReplicatorQueueStatusResult CopyQueueStatus { get; set; }

        [DataMember]
        public string LastCopyOperationReceivedTimeUtc { get; set; }

        [DataMember]
        public string LastAcknowledgementSentTimeUtc { get; set; }
        public ReplicatorStatusBase Status { get; private set; }

        [OnDeserialized()]
        private void Initialize(StreamingContext context)
        {
            switch (Kind)
            {
                case ReplicaRole.Primary:
                    {
                        Status = new PrimaryReplicatorStatusResult(
                            ReplicationQueueStatus,
                            RemoteReplicators);

                        return;
                    }
                case ReplicaRole.IdleSecondary:
                case ReplicaRole.ActiveSecondary:
                    {
                        Status = new SecondaryReplicatorStatusResult(
                            ReplicationQueueStatus,
                            LastReplicationOperationReceivedTimeUtc,
                            IsInBuild,
                            CopyQueueStatus,
                            LastCopyOperationReceivedTimeUtc,
                            LastAcknowledgementSentTimeUtc);

                        return;
                    }

                default:
                    return;
            }
        }
    }
}