// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;

    internal abstract class ActionGeneratorBase
    {
        internal FabricEvents.ExtensionsEvents Log = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.RandomActionGenerator);
        protected internal RandomExtensions RandomExt;
        protected Random Random;

        private List<StateTransitionAction> actionsQueue = new List<StateTransitionAction>();

        protected internal ActionGeneratorBase(Random random)
        {
            this.Random = random;
            this.RandomExt = new RandomExtensions(random);
            this.TraceType = "Chaos.ActionGeneratorBase";
        }

        protected virtual string TraceType { get; set; }

        public IList<StateTransitionAction> GetNextActions(ClusterStateSnapshot stateInfo, Guid activityId = default(Guid))
        {
            // Generate and enqueue actions.
            this.GenerateAndEnqueueRandomActions(stateInfo, activityId);
            List<StateTransitionAction> actions = this.DequeueActions();

            return actions;
        }

        internal virtual IList<StateTransitionAction> GetPendingActions(ClusterStateSnapshot stateInfo, Guid activityId = default(Guid))
        {
            return new List<StateTransitionAction>();
        }

        protected abstract void GenerateAndEnqueueRandomActions(ClusterStateSnapshot stateInfo, Guid activityId = default(Guid));

        // Enqueue one action if it is not null.
        protected void EnqueueAction(StateTransitionAction action)
        {
            if (action != null)
            {
                this.actionsQueue.Add(action);
            }
        }

        // Enqueue all actions in the list. Returns false if input list is null.
        protected void EnqueueActions(IList<StateTransitionAction> actions)
        {
            if (actions != null)
            {
                foreach (StateTransitionAction action in actions)
                {
                    this.EnqueueAction(action);
                }
            }
        }

        //---- NodeFaultActions storage methods. -----//
        private List<StateTransitionAction> DequeueActions()
        {
            List<StateTransitionAction> actions = new List<StateTransitionAction>();
            actions.AddRange(this.actionsQueue);
            this.actionsQueue.Clear();
            return actions;
        }
    }
}