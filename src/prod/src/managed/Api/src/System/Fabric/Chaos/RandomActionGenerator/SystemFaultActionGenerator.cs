// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;

    /// <summary>
    /// This class will keep track of current state of the WindowsFabricCluster being used for testing
    /// and generates random actions to change state, induce fault in it or verify consistency of the state.
    /// </summary>
    internal class SystemFaultActionGenerator : ActionGeneratorBase
    {
        /// <summary>
        /// Parameters for generating system fault actions
        /// </summary>
        private SystemFaultActionGeneratorParameters testParameters;

        /// <summary>
        /// Current snapshot of the cluster
        /// </summary>
        private ClusterStateSnapshot stateSnapshot = null;

        /// <summary>
        /// Probabilistically chooses a system fault category
        /// </summary>
        private WeightedDice<SystemFaultCategory> systemFaultCategoryChooser;

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Chaos.RandomActionGenerator.SystemFaultActionGenerator"/> class.
        /// </summary>
        /// <param name="testParameters"> Parameters for this random session.</param>
        /// <param name="random"> Random number generator </param>
        public SystemFaultActionGenerator(SystemFaultActionGeneratorParameters testParameters, Random random)
            : base(random)
        {
            this.testParameters = testParameters;
            this.systemFaultCategoryChooser = new WeightedDice<SystemFaultCategory>(this.Random);
            this.systemFaultCategoryChooser.AddNewFace(SystemFaultCategory.FmRebuild, testParameters.FmRebuildFaultWeight);

            this.TraceType = "Chaos.SystemFaultActionGenerator";
        }

        /// <summary>
        /// System fault categories
        /// </summary>
        private enum SystemFaultCategory : int
        {
            Min = 0,
            /// <summary>
            /// Forces the Failover Manager service to rebuild the GFUM
            /// </summary>
            FmRebuild,
            Max
        }

        protected override void GenerateAndEnqueueRandomActions(ClusterStateSnapshot stateInfo, Guid activityId = default(Guid))
        {
            this.stateSnapshot = stateInfo;

            this.GenerateAndEnqueueRandomActions(activityId);
        }

        /// <summary>
        /// Generates and enqueues random actions
        /// </summary>
        private void GenerateAndEnqueueRandomActions(Guid activityId = default(Guid))
        {
            var action = this.CreateSystemFaultAction(activityId);
            this.EnqueueAction(action);
        }

        /// <summary>
        /// Creates and returns random system fault actions
        /// </summary>
        /// <returns>A state transition action</returns>
        private StateTransitionAction CreateSystemFaultAction(Guid activityId = default(Guid))
        {
            List<StateTransitionAction> clusterActions = new List<StateTransitionAction>();

            var action = this.CreateOneSystemFaultAction(activityId);
            return action;
        }

        /// <summary>
        /// Creates and returns one system fault action
        /// </summary>
        /// <returns>A transition action for a system service</returns>
        private SystemStateTransitionAction CreateOneSystemFaultAction(Guid activityId = default(Guid))
        {
            SystemStateTransitionAction action = null;

            //// Choose category 
            SystemFaultCategory faultCategory = (SystemFaultCategory)this.systemFaultCategoryChooser.NextRoll();

            switch (faultCategory)
            {
                case SystemFaultCategory.FmRebuild:
                    {
                        TestabilityTrace.TraceSource.WriteInfo(this.TraceType, "{0}: {1} has been chosen.", activityId, faultCategory);
                        action = SystemStateTransitionAction.CreateFmRebuildStateTransitionAction();
                    }
                    break;

                default:
                    throw new ArgumentException("Unknown category:" + faultCategory);
            }

            return action;
        }
    }
}