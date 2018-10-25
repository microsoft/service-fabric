// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;

    internal abstract class ReplicaStateTransitionAction : StateTransitionAction
    {
        protected ReplicaStateTransitionAction(Uri serviceUri, Guid partitionId, long replicaId, StateTransitionActionType type, Guid groupId)
            : base(type, groupId, Guid.NewGuid())
        {
            this.ServiceUri = serviceUri;
            this.PartitionId = partitionId;
            this.ReplicaId = replicaId;
        }
        
        public Uri ServiceUri { get; private set; }

        public Guid PartitionId { get; private set; }

        public long ReplicaId { get; private set; }

        public override string ToString()
        {
            return string.Format("{0}, ServiceUri: {1}, PartitionId: {2}, ReplicaId: {3}",
                base.ToString(),
                this.ServiceUri,
                this.PartitionId,
                this.ReplicaId);
        }
    }
}