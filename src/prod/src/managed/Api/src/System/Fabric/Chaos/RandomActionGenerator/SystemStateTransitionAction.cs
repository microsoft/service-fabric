// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    /// <summary>
    /// This class is used to create instances of various system state transition actions
    /// </summary>
    internal class SystemStateTransitionAction : StateTransitionAction
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Chaos.RandomActionGenerator.SystemStateTransitionAction"/> class.
        /// </summary>
        /// <param name="actionType">StateTransitionActionType</param>
        private SystemStateTransitionAction(StateTransitionActionType actionType)
            : base(actionType, Guid.NewGuid(), Guid.NewGuid())
        {
        }

        /// <summary>
        /// Creates and returns a state transition action, upon execution of which, the Failover
        /// Manager service is forced to rebuild the GFUM
        /// </summary>
        /// <returns>A system service state transition action</returns>
        public static SystemStateTransitionAction CreateFmRebuildStateTransitionAction()
        {
            return new SystemStateTransitionAction(StateTransitionActionType.FmRebuild);
        }

        public override string ToString()
        {
            return base.ToString();
        }
    }
}