// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;

    internal abstract class MoveReplicaStateTransitionAction : StateTransitionAction
    {
        protected MoveReplicaStateTransitionAction(Uri serviceUri, Guid partitionId, string nodeTo, StateTransitionActionType type, bool forceMove, Guid groupId)
            : base(type, groupId, Guid.NewGuid())
        {
            this.ServiceUri = serviceUri;
            this.PartitionId = partitionId;
            this.NodeTo = nodeTo;
            this.ForceMove = forceMove;
        }

        public Uri ServiceUri { get; private set; }

        public Guid PartitionId { get; private set; }

        public string NodeTo { get; private set; }

        public bool ForceMove { get; private set; }

        public override string ToString()
        {
            return string.Format("{0}, ServiceUri: {1}, PartitionId: {2}, NodeTo: {3}, ForceMove: {4}",
                base.ToString(),
                this.ServiceUri,
                this.PartitionId,
                this.NodeTo,
                this.ForceMove);
        }
    }
}