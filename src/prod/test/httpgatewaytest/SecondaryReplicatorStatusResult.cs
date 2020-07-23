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
    class SecondaryReplicatorStatusResult : ReplicatorStatusBase
    {
        public SecondaryReplicatorStatusResult(
            ReplicatorQueueStatusResult replicationQueueStatus,
            string lastReplicationOperationReceivedTimeUtc,
            bool isInBuild,
            ReplicatorQueueStatusResult copyQueueStatus,
            string lastCopyOperationReceivedTimeUtc,
            string lastAcknowledgementSentTimeUtc)
            :base(ReplicaRole.ActiveSecondary)
        {
            ReplicationQueueStatus = replicationQueueStatus;
            LastReplicationOperationReceivedTimeUtc = lastReplicationOperationReceivedTimeUtc;
            IsInBuild = isInBuild;
            CopyQueueStatus = copyQueueStatus;
            LastCopyOperationReceivedTimeUtc = lastCopyOperationReceivedTimeUtc;
            LastAcknowledgementSentTimeUtc = lastAcknowledgementSentTimeUtc;

            if (isInBuild)
            {
                Kind = ReplicaRole.IdleSecondary;
            }
        }
        public ReplicatorQueueStatusResult ReplicationQueueStatus { get; private set; }

        public string LastReplicationOperationReceivedTimeUtc { get; private set; }

        public bool IsInBuild { get; private set; }

        public ReplicatorQueueStatusResult CopyQueueStatus { get; private set; }

        public string LastCopyOperationReceivedTimeUtc { get; private set; }

        public string LastAcknowledgementSentTimeUtc { get; private set; }
    }
}