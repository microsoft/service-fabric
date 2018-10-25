// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Fabric.Chaos.Common;

    internal sealed class MoveSecondaryReplicaStateTransitionAction : MoveReplicaStateTransitionAction
    {
        public MoveSecondaryReplicaStateTransitionAction(Uri serviceUri, Guid partitionId, string nodeFrom, string nodeTo, Guid groupId)
            : base(serviceUri, partitionId, nodeTo, StateTransitionActionType.MoveSecondary, ChaosUtility.ForcedMoveOfReplicaEnabled, groupId)
        {
            this.NodeFrom = nodeFrom;
        }

        public string NodeFrom { get; set; }

        public override string ToString()
        {
            return string.Format("ActionType: {0}, ServiceUri: {1}, PartitionId: {2}, NodeFrom: {3}, NodeTo: {4}, ForceMove: {5}",
                this.ActionType,
                this.ServiceUri,
                this.PartitionId,
                this.NodeFrom,
                this.NodeTo,
                this.ForceMove);
        }
    }
}