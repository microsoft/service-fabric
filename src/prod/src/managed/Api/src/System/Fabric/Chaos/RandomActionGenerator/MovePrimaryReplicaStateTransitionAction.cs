// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Fabric.Chaos.Common;
    using System.Text;

    internal sealed class MovePrimaryReplicaStateTransitionAction : MoveReplicaStateTransitionAction
    {
        public MovePrimaryReplicaStateTransitionAction(Uri serviceUri, Guid partitionId, string nodeTo, Guid groupId)
            : base(serviceUri, partitionId, nodeTo, StateTransitionActionType.MovePrimary, ChaosUtility.ForcedMoveOfReplicaEnabled, groupId)
        {
        }
    }
}