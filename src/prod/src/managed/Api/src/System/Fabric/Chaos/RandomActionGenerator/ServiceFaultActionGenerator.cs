// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Query;
    using System.Linq;

    internal class ServiceFaultActionGenerator : ActionGeneratorBase
    {
        private readonly int maxTrialPerActionGeneration = 10;
        private ClusterStateSnapshot stateSnapshot;
        private ServiceFaultActionGeneratorParameters testParameters;
        private WeightedDice<ServiceFaultCategory> serviceFaultCategoryChooser;

        public ServiceFaultActionGenerator(ServiceFaultActionGeneratorParameters testParameters, Random random)
            : base(random)
        {
            this.testParameters = testParameters;

            this.serviceFaultCategoryChooser = new WeightedDice<ServiceFaultCategory>(this.Random);
            this.serviceFaultCategoryChooser.AddNewFace(ServiceFaultCategory.RestartReplica, testParameters.RestartReplicaFaultWeight);
            this.serviceFaultCategoryChooser.AddNewFace(ServiceFaultCategory.RemoveReplica, testParameters.RemoveReplicaFaultWeight);
            this.serviceFaultCategoryChooser.AddNewFace(ServiceFaultCategory.RestartCodePackage, testParameters.RestartCodePackageFaultWeight);
            this.serviceFaultCategoryChooser.AddNewFace(ServiceFaultCategory.MovePrimary, testParameters.MovePrimaryFaultWeight);
            this.serviceFaultCategoryChooser.AddNewFace(ServiceFaultCategory.MoveSecondary, testParameters.MoveSecondaryFaultWeight);

            this.TraceType = "Chaos.ServiceFaultActionGenerator";
        }

        private enum ServiceFaultCategory : int
        {
            Min = 0,
            RestartReplica,
            RemoveReplica,
            RestartCodePackage,
            MovePrimary,
            MoveSecondary,
            Max
        }

        protected override void GenerateAndEnqueueRandomActions(ClusterStateSnapshot stateSnapshot, Guid activityId = default(Guid))
        {
            // Enqueue Actions
            this.stateSnapshot = stateSnapshot;
            this.CreateAndEnqueueServiceFaultActions(activityId);
        }

        // Will generate some random service fault actions and enqueue those actions
        private void CreateAndEnqueueServiceFaultActions(Guid activityId = default(Guid))
        {
            List<StateTransitionAction> actions = this.GenerateServiceFaultActions(activityId);
            this.EnqueueActions(actions);
        }

        // Creates service fault actions
        private List<StateTransitionAction> GenerateServiceFaultActions(Guid activityId = default(Guid))
        {
            List<StateTransitionAction> serviceFaultActions = new List<StateTransitionAction>();

            StateTransitionAction replicaAction = null;
            for (int trial = 0; trial < this.maxTrialPerActionGeneration && replicaAction == null; trial++)
            {
                // Choose category 
                ServiceFaultCategory faultCategory = (ServiceFaultCategory)this.serviceFaultCategoryChooser.NextRoll();

                switch (faultCategory)
                {
                    case ServiceFaultCategory.RemoveReplica:
                        {
                            replicaAction = this.GenerateOneRemoveReplicaAction(activityId);
                        }

                        break;
                    case ServiceFaultCategory.RestartReplica:
                        {
                            replicaAction = this.GenerateOneRestartReplicaAction(activityId);
                        }

                        break;
                    case ServiceFaultCategory.RestartCodePackage:
                        {
                            replicaAction = this.GenerateOneRestartCodePackageAction(activityId);
                        }

                        break;
                    case ServiceFaultCategory.MovePrimary:
                        {
                            replicaAction = this.GenerateOneMovePrimaryAction(activityId);
                        }

                        break;
                    case ServiceFaultCategory.MoveSecondary:
                        {
                            replicaAction = this.GenerateOneMoveSecondaryAction(activityId);
                        }

                        break;
                    default:
                        throw new ArgumentException("Unknown category:" + faultCategory);
                }
            }

            if (replicaAction != null)
            {
                serviceFaultActions.Add(replicaAction);
            }

            return serviceFaultActions;
        }

        private StateTransitionAction GenerateOneRestartCodePackageAction(Guid activityId = default(Guid))
        {
            var availableCodePackages = this.stateSnapshot.GetFaultableCodePackages();
            if (availableCodePackages.Count() <= 0)
            {
                return null;
            }

            var chosenCodePackage = availableCodePackages.ElementAt(this.Random.Next(availableCodePackages.Count()));

            TestabilityTrace.TraceSource.WriteInfo(
                this.TraceType, 
                "{0}: CP:'{1}' has been chosen for RestartCodePackage.", 
                    activityId, 
                    chosenCodePackage);

            chosenCodePackage.FaultCodePackage(activityId);

            var replicaAction = new RestartCodePackageStateTransitionAction(
                                        chosenCodePackage.NodeName,
                                        chosenCodePackage.ParentApplicationEntity.Application.ApplicationName,
                                        chosenCodePackage.CodePackageResult.ServiceManifestName,
                                        chosenCodePackage.CodePackageResult.ServicePackageActivationId,
                                        chosenCodePackage.CodePackageResult.CodePackageName,
                                        this.stateSnapshot.ClusterSnapshotId);

            return replicaAction;
        }

        private StateTransitionAction GenerateOneMovePrimaryAction(Guid activityId = default(Guid))
        {
            var availablePrimaryReplicas = this.stateSnapshot.GetFaultableReplicas(true/*IsStateful*/).Where(r => this.SelectReadyPrimary(r));
            if (availablePrimaryReplicas.Count() <= 0)
            {
                return null;
            }

            var primaryChosen = availablePrimaryReplicas.ElementAt(this.Random.Next(availablePrimaryReplicas.Count()));

            var availableNodesToMove = this.stateSnapshot.GetFaultableNodes().Where(n => n.CurrentNodeInfo.NodeName != primaryChosen.Replica.NodeName);
            if (availableNodesToMove.Count() <= 0)
            {
                return null;
            }

            var newPrimaryNode = availableNodesToMove.ElementAt(this.Random.Next(availableNodesToMove.Count()));

            TestabilityTrace.TraceSource.WriteInfo(
                this.TraceType, 
                "{0}: Replica:'{1}' has been chosen for MovePrimary to '{2}'.", 
                    activityId, 
                    primaryChosen,
                    newPrimaryNode.CurrentNodeInfo.NodeName);

            primaryChosen.MoveReplica(newPrimaryNode.CurrentNodeInfo.NodeName, activityId);

            var replicaAction = new MovePrimaryReplicaStateTransitionAction(
                                        primaryChosen.ParentPartitionEntity.ParentServiceEntity.ServiceName(),
                                        primaryChosen.ParentPartitionEntity.Guid,
                                        newPrimaryNode.CurrentNodeInfo.NodeName,
                                        this.stateSnapshot.ClusterSnapshotId);

            return replicaAction;
        }

        private StateTransitionAction GenerateOneMoveSecondaryAction(Guid activityId = default(Guid))
        {
            var availableSecondaryReplicas = this.stateSnapshot.GetFaultableReplicas(true/*IsStateful*/).Where(r => this.SelectReadySecondary(r));
            if (availableSecondaryReplicas.Count() <= 0)
            {
                return null;
            }

            var secondaryChosen = availableSecondaryReplicas.ElementAt(this.Random.Next(availableSecondaryReplicas.Count()));
            var partitionEntities = secondaryChosen.ParentPartitionEntity.ReplicaList.Select<ReplicaEntity, string>(r => r.Replica.NodeName);
            var availableNodesToMove = this.stateSnapshot.GetFaultableNodes().Where(n => !partitionEntities.Contains(n.CurrentNodeInfo.NodeName));
            if (availableNodesToMove.Count() <= 0)
            {
                return null;
            }

            var newSecondaryNode = availableNodesToMove.ElementAt(this.Random.Next(availableNodesToMove.Count()));

            TestabilityTrace.TraceSource.WriteInfo(
                this.TraceType, 
                "{0}: Replica '{1}' has been chosen for MoveSecondary to '{2}'.", 
                    activityId, 
                    secondaryChosen, 
                    newSecondaryNode.CurrentNodeInfo.NodeName);

            secondaryChosen.MoveReplica(newSecondaryNode.CurrentNodeInfo.NodeName, activityId);

            var replicaAction = new MoveSecondaryReplicaStateTransitionAction(
                                        secondaryChosen.ParentPartitionEntity.ParentServiceEntity.ServiceName(),
                                        secondaryChosen.ParentPartitionEntity.Guid,
                                        secondaryChosen.Replica.NodeName,
                                        newSecondaryNode.CurrentNodeInfo.NodeName,
                                        this.stateSnapshot.ClusterSnapshotId);

            return replicaAction;
        }

        private bool SelectReadyPrimary(ReplicaEntity replica)
        {
            var statefulReplica = replica.Replica as StatefulServiceReplica;
            ReleaseAssert.AssertIf(statefulReplica == null, "Replica type should be stateful");
            return statefulReplica.ReplicaRole == ReplicaRole.Primary && replica.IsAvailableToFault;
        }

        private bool SelectReadySecondary(ReplicaEntity replica)
        {
            var statefulReplica = replica.Replica as StatefulServiceReplica;
            ReleaseAssert.AssertIf(statefulReplica == null, "Replica type should be stateful");
            return statefulReplica.ReplicaRole == ReplicaRole.ActiveSecondary && replica.IsAvailableToFault;
        }

        private StateTransitionAction GenerateOneRemoveReplicaAction(Guid activityId = default(Guid))
        {
            var availableReplicas = this.stateSnapshot.GetFaultableReplicas();
            if (availableReplicas.Count() <= 0)
            {
                return null;
            }

            // Randomly choose a replica from all replicas available for fault action.
            // Select a replica at random index. 
            var replicaChosen = availableReplicas.ElementAt(this.Random.Next(availableReplicas.Count()));

            TestabilityTrace.TraceSource.WriteInfo(
                this.TraceType, "{0}: Replica:'{1}' has been chosen for RemoveReplica.", 
                    activityId, 
                    replicaChosen);

            replicaChosen.FaultReplica(false/*faultedDueToCodePackageFault*/, activityId);

            var partitionEntity = replicaChosen.ParentPartitionEntity;

            var replicaAction = new RemoveReplicaStateTransitionAction(
                                    replicaChosen.ParentPartitionEntity.ParentServiceEntity.ServiceName(),
                                    partitionEntity.Guid,
                                    replicaChosen.Replica.Id,
                                    this.stateSnapshot.ClusterSnapshotId);

            return replicaAction;
        }

        private StateTransitionAction GenerateOneRestartReplicaAction(Guid activityId = default(Guid))
        {
            var availableReplicas = this.stateSnapshot.GetFaultableReplicas(true/*IsStateful*/, true/*HasPersistedState*/);
            if (availableReplicas.Count() <= 0)
            {
                return null;
            }

            // Randomly choose a replica from all replicas available for fault action.
            // Select a replica at random index. 
            var replicaChosen = availableReplicas.ElementAt(this.Random.Next(availableReplicas.Count()));

            TestabilityTrace.TraceSource.WriteInfo(
                this.TraceType, 
                "{0}: Replica '{1}' has been chosen for RestartReplica.", 
                    activityId, 
                    replicaChosen);

            replicaChosen.FaultReplica(false/*faultedDueToCodePackageFault*/, activityId);

            var partitionEntity = replicaChosen.ParentPartitionEntity;

            var replicaAction = new RestartReplicaStateTransitionAction(
                                        replicaChosen.ParentPartitionEntity.ParentServiceEntity.ServiceName(),
                                        partitionEntity.Guid,
                                        replicaChosen.Replica.Id,
                                        this.stateSnapshot.ClusterSnapshotId);

            return replicaAction;
        }
    }
}