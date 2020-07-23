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
    class PrimaryReplicatorStatusResult : ReplicatorStatusBase
    {
        public PrimaryReplicatorStatusResult(ReplicatorQueueStatusResult replicationQueueStatus, IList<RemoteReplicatorStatusResult> remoteReplicators)
            : base(ReplicaRole.Primary)
        {
            ReplicationQueueStatus = replicationQueueStatus;
            RemoteReplicators = remoteReplicators;
        }

        public ReplicatorQueueStatusResult ReplicationQueueStatus { get; private set; }

        public IList<RemoteReplicatorStatusResult> RemoteReplicators { get; private set; }
    }
}