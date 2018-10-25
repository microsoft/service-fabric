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
    /// This class will be helpful in generating Random Actions.
    /// This class will serve as entry point to generate any RandomSessionAction.
    /// This class will basically schedule and invoke different ActionsManager based on state and their configurable weights.
    /// and will translate those to ScriptTest commands.
    /// </summary>
    internal sealed class RootActionGenerator : ActionGeneratorBase
    {
        private ActionGeneratorParameters testParameters;

        // Choose between different action category.
        private WeightedDice<ActionCategory> actionCategoryChooser;
        private WeightedDice<FaultCategory> faultCategoryChooser;

        // ActionsManagers
        private NodeFaultActionGenerator nodeFaultActionsManager;
        private ServiceFaultActionGenerator serviceFaultActionsManager;
        private WorkloadActionGenerator workloadActionsManager;

        /// <summary>
        /// Generator for system faults, like: FM Rebuild fault
        /// </summary>
        private SystemFaultActionGenerator systemFaultActionGenerator;

        private bool initialSetup;

        public RootActionGenerator(ActionGeneratorParameters testParameters, Random random)
            : base(random)
        {
            this.TraceType = "Chaos.RootActionGenerator";

            this.testParameters = testParameters ?? new ActionGeneratorParameters();

            // Initialize node fault action manager
            this.nodeFaultActionsManager = new NodeFaultActionGenerator(testParameters.NodeFaultActionsParameters, random);

            // Initialize service fault action manager
            this.serviceFaultActionsManager = new ServiceFaultActionGenerator(testParameters.ServiceFaultActionsParameters, random);

            // Initialize workload actions manager
            this.workloadActionsManager = new WorkloadActionGenerator(testParameters.WorkloadParameters, random);

            this.systemFaultActionGenerator = new SystemFaultActionGenerator(testParameters.SystemServiceFaultParameters, random);

            this.initialSetup = true;

            // Initialize weights for each action manager.
            this.InitializeCategoryWeights();
        }

        // Category of actions manager to use.
        private enum ActionCategory : int
        {
            Min = 0,
            WorkLoad,
            Faults,
            Max
        }

        private enum FaultCategory : int
        {
            Min = 0,
            NodeFaults,
            ServiceFaults,
            /// <summary>
            /// For example, FM Rebuild
            /// </summary>
            SystemFaults,
            Max
        }

        internal override IList<StateTransitionAction> GetPendingActions(ClusterStateSnapshot stateInfo, Guid activityId = default(Guid))
        {
            var pendingActionsList = new List<StateTransitionAction>();

            pendingActionsList.AddRange(this.nodeFaultActionsManager.GetPendingActions(stateInfo, activityId));
            pendingActionsList.AddRange(this.serviceFaultActionsManager.GetPendingActions(stateInfo, activityId));
            pendingActionsList.AddRange(this.workloadActionsManager.GetPendingActions(stateInfo, activityId));

            return pendingActionsList;
        }

        protected override void GenerateAndEnqueueRandomActions(ClusterStateSnapshot stateSnapshot, Guid activityId = default(Guid))
        {
            List<StateTransitionAction> generatedActions = new List<StateTransitionAction>();
            ActionCategory category;

            if (this.initialSetup && this.testParameters.WorkloadParameters.WorkloadScripts.Count > 0)
            {
                category = ActionCategory.WorkLoad;
                this.initialSetup = false;
            }
            else
            {
                category = (ActionCategory)this.actionCategoryChooser.NextRoll();
            }

            switch (category)
            {
                case ActionCategory.WorkLoad:
                    generatedActions.AddRange(this.GenerateActionsUsingActionsManager(this.workloadActionsManager, stateSnapshot, activityId));
                    break;
                case ActionCategory.Faults:
                    generatedActions.AddRange(this.GenerateRandomFaults(stateSnapshot, activityId));
                    break;
                default:
                    throw new ArgumentException("Unknown category:" + category);
            }

            this.EnqueueActions(generatedActions);
        }

        // Generates random actions of given category
        private IList<StateTransitionAction> GenerateRandomFaults(ClusterStateSnapshot stateSnapshot, Guid activityId = default(Guid))
        {
            List<StateTransitionAction> generatedActions = new List<StateTransitionAction>();

            var faultCategories = this.GenerateFaultCategories();

            foreach (var faultCategory in faultCategories)
            {
                switch (faultCategory)
                {
                    case FaultCategory.NodeFaults:
                        generatedActions.AddRange(this.GenerateActionsUsingActionsManager(this.nodeFaultActionsManager, stateSnapshot, activityId));
                        break;
                    case FaultCategory.ServiceFaults:
                        generatedActions.AddRange(this.GenerateActionsUsingActionsManager(this.serviceFaultActionsManager, stateSnapshot, activityId));
                        break;
                    case FaultCategory.SystemFaults:
                        generatedActions.AddRange(this.GenerateActionsUsingActionsManager(this.systemFaultActionGenerator, stateSnapshot, activityId));
                        break;
                    default:
                        throw new ArgumentException("Unknown category:" + faultCategory);
                }
            }

            return generatedActions;
        }

        /// <summary>
        /// Generates and returns a list of fault categories without modifying the current cluster snapshot
        /// </summary>
        /// <returns>A list of fault categories</returns>
        private IEnumerable<FaultCategory> GenerateFaultCategories()
        {
            var faultCategories = new List<FaultCategory>();
            for (int i = 0; i < this.testParameters.MaxConcurrentFaults; ++i)
            {
                var faultCategory = (FaultCategory)this.faultCategoryChooser.NextRoll();

                // When a system fault is chosen, no other fault is allowed
                if (faultCategory == FaultCategory.SystemFaults)
                {
                    faultCategories.Clear();
                    faultCategories.Add(faultCategory);
                    return faultCategories;
                }

                faultCategories.Add(faultCategory);
            }

            return faultCategories;
        }

        // Generates random actions using given actionManager 
        private IList<StateTransitionAction> GenerateActionsUsingActionsManager(
            ActionGeneratorBase actionManager,
            ClusterStateSnapshot stateSnapshot,
            Guid activityId = default(Guid))
        {
            // Generate actions first
            IList<StateTransitionAction> generatedActions = actionManager.GetNextActions(stateSnapshot, activityId);
            this.Log.WriteInfo(this.TraceType, "{0}: Returning {1} actions.", activityId, generatedActions.Count);
            return generatedActions;
        }

        private void InitializeCategoryWeights()
        {
            this.faultCategoryChooser = new WeightedDice<FaultCategory>(this.Random);
            this.actionCategoryChooser = new WeightedDice<ActionCategory>(this.Random);

            // Add a dice-face for NodeFaults
            this.faultCategoryChooser.AddNewFace(FaultCategory.NodeFaults, this.testParameters.NodeFaultActionWeight);

            // Add a dice-face for ServiceFaults
            this.faultCategoryChooser.AddNewFace(FaultCategory.ServiceFaults, this.testParameters.ServiceFaultActionWeight);

            this.faultCategoryChooser.AddNewFace(FaultCategory.SystemFaults, this.testParameters.SystemFaultActionWeight);

            var faultActionCategoryWeight = this.testParameters.NodeFaultActionWeight + this.testParameters.ServiceFaultActionWeight + this.testParameters.SystemFaultActionWeight;

            this.actionCategoryChooser.AddNewFace(
                ActionCategory.Faults,
                faultActionCategoryWeight);

            // Add a new face(outcome value) for workloads action manager. 
            // Weight of action-manager that is all workloads collectively, is multiplied with weight of one workload.
            this.actionCategoryChooser.AddNewFace(
                ActionCategory.WorkLoad,
                this.testParameters.WorkloadActionWeight * this.testParameters.WorkloadParameters.WorkloadScripts.Count);
        }
    }
}