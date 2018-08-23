// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    internal class StartNodeStateTransitionAction : NodeStateTransitionAction
    {
        public StartNodeStateTransitionAction(string nodeName, long nodeInstanceId, Guid groupId)
            : base(nodeName, nodeInstanceId, StateTransitionActionType.NodeUp, groupId)
        {
        }
    }
}