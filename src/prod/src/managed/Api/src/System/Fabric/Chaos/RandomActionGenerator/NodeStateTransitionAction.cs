// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Numerics;    

    internal abstract class NodeStateTransitionAction : StateTransitionAction
    {
        protected NodeStateTransitionAction(string nodeName, long nodeInstanceId, StateTransitionActionType type, Guid groupId)
            : base(type, groupId, Guid.NewGuid())
        {
            this.NodeName = nodeName;
            this.NodeInstanceId = nodeInstanceId;
        }

        public string NodeName { get; private set; }

        public long NodeInstanceId { get; private set; }

        public override string ToString()
        {
            return string.Format("{0}, NodeName: {1}, NodeInstanceId: {2}", 
                base.ToString(),
                this.NodeName, 
                this.NodeInstanceId);
        }
    }
}