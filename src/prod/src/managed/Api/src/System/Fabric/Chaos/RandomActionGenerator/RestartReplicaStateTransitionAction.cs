// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;

    internal sealed class RestartReplicaStateTransitionAction : ReplicaStateTransitionAction
    {
        public RestartReplicaStateTransitionAction(Uri serviceUri, Guid partitionId, long replicaId, Guid groupId)
            : base(serviceUri, partitionId, replicaId, StateTransitionActionType.RestartReplica, groupId)
        {
        }
    }
}