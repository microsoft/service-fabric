// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    internal class RestartNodeStateTransitionAction : NodeStateTransitionAction
    {
        public RestartNodeStateTransitionAction(string nodeName, long nodeInstanceId, Guid groupId)
            : base(nodeName, nodeInstanceId, StateTransitionActionType.NodeRestart, groupId)
        {
        }

        public override string ToString()
        {
            return string.Format("ActionType: {0}, NodeName: {1}, NodeInstanceId: {2}",
                this.ActionType,
                this.NodeName,
                this.NodeInstanceId);
        }
    }
}