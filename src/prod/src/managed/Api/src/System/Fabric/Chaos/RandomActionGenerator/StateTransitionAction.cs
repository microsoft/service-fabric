// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Globalization;

    /// <summary>
    /// This class will represent an action on WindowsFabric cluster which gets generated during RandomSession.
    /// In general an action could represent a change in set of cluster nodes, faults,
    /// applications, services or verification step or just a query.
    /// Action will be executed with help of one or more ScriptTest commands.
    /// </summary>
    /// TODO FW: make it abstract after script test side code is refactor and totally dependent on testability.
    internal abstract class StateTransitionAction 
    {
        public StateTransitionAction(StateTransitionActionType actionType, Guid groupId, Guid actionId)
        {
            this.ActionType = actionType;
            this.StateTransitionActionGroupId = groupId;
            this.StateTransitionActionId = actionId;
        }

        /// <summary>
        /// A group of actions is produced in one iteration of Chaos, this ID uniquely identifies that group of actions
        /// </summary>
        public Guid StateTransitionActionGroupId { get; set; }

        /// <summary>
        /// Each chosen action by Chaos is a fault, this ID uniquely identifies the fault
        /// </summary>
        public Guid StateTransitionActionId { get; set; }

        public StateTransitionActionType ActionType { get; private set; }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "ActionType: {0}",
                this.ActionType);
        }
    }
}