// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Strings;
    using System.Numerics;

    /// <summary>
    /// This class will provide methods to translate RandomSessionAction to equivalent TestabilityAction(s).
    /// </summary>
    internal class ActionTranslator
    {
        private const string TraceType = "Chaos.ActionTranslator";
        private static readonly FabricEvents.ExtensionsEvents Log = TestabilityTrace.TraceSource;

        private readonly Action<string> reportFunction;

        public ActionTranslator()
        {
        }

        public ActionTranslator(Action<string> reportFunction)
        {
            this.reportFunction = reportFunction;
        }

        /// Create and append commands for the given action(s).
        public IDictionary<FabricTestAction, StateTransitionAction> GetCommandsMap(IList<StateTransitionAction> ragActions)
        {
            var actionMap = new Dictionary<FabricTestAction, StateTransitionAction>();

            foreach (StateTransitionAction rsAction in ragActions)
            {
                actionMap.Add(this.GetCommandForActions(rsAction), rsAction);
            }

            return actionMap;
        }

        /// Create and append commands for the given action.
        public FabricTestAction GetCommandForActions(StateTransitionAction ragAction)
        {
            if (ragAction == null)
            {
                return null;
            }

            switch (ragAction.ActionType)
            {
                case StateTransitionActionType.NodeUp:
                    {
                        return this.GetStartNodeAction(ragAction as NodeStateTransitionAction);
                    }

                case StateTransitionActionType.NodeDown:
                    {
                        return this.GetStopNodeAction(ragAction as NodeStateTransitionAction);
                    }

                case StateTransitionActionType.NodeRestart:
                    {
                        return this.GetRestartNodeAction(ragAction as NodeStateTransitionAction);
                    }

                case StateTransitionActionType.RemoveReplica:
                    {
                        return this.GetRemoveReplicaAction(ragAction as ReplicaStateTransitionAction);
                    }

                case StateTransitionActionType.RestartReplica:
                    {
                        return this.GetRestartReplicaAction(ragAction as ReplicaStateTransitionAction);
                    }

                case StateTransitionActionType.MovePrimary:
                    {
                        return this.GetMovePrimaryReplicaAction(ragAction as MovePrimaryReplicaStateTransitionAction);
                    }
                case StateTransitionActionType.MoveSecondary:
                    {
                        return this.GetMoveSecondaryReplicaAction(ragAction as MoveSecondaryReplicaStateTransitionAction);
                    }

                case StateTransitionActionType.CodePackageRestart:
                    {
                        return this.GetRestartCodePackageAction(ragAction as RestartCodePackageStateTransitionAction);
                    }

                default:
                    {
                        throw new InvalidOperationException("Action type not yet supported.");
                    }
            }
        }

        private FabricTestAction GetStartNodeAction(NodeStateTransitionAction ragAction)
        {
            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.StartNode_InvalidActionType_TelemetryId,
                ragAction == null || ragAction.ActionType != StateTransitionActionType.NodeUp,
                StringResources.ChaosEngineError_StartNode_InvalidActionType);

            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.StartNode_InvalidNodeName_TelemetryId,
                string.IsNullOrEmpty(ragAction.NodeName),
                StringResources.ChaosEngineError_StartNode_InvalidNodeName);

            string report = StringHelper.Format("Generating Random Action: {0}\n\t\tNodeName: {1}\n\t\tNodeInstanceId: {2}", ragAction.ActionType, ragAction.NodeName, ragAction.NodeInstanceId);
            Log.WriteInfo(TraceType, report);
            if(this.reportFunction != null)
            {
                this.reportFunction(report);
            }

            string nodeName = ragAction.NodeName;
            BigInteger nodeInstanceId = ragAction.NodeInstanceId;
            StartNodeAction startNodeAction = new StartNodeAction(nodeName, nodeInstanceId);
            return startNodeAction;
        }

        private FabricTestAction GetStopNodeAction(NodeStateTransitionAction ragAction)
        {
            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.StopNode_InvalidActionType_TelemetryId,
                ragAction == null || ragAction.ActionType != StateTransitionActionType.NodeDown,
                StringResources.ChaosEngineError_StopNode_InvalidActionType);

            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.StopNode_InvalidNodeName_TelemetryId,
                string.IsNullOrEmpty(ragAction.NodeName),
                StringResources.ChaosEngineError_StopNode_InvalidNodeName);

            string report = StringHelper.Format("Generating Action: {0}\n\t\tNodeName: {1}\n\t\tNodeInstanceId: {2}", ragAction.ActionType, ragAction.NodeName, ragAction.NodeInstanceId);
            Log.WriteInfo(TraceType, report);
            if (this.reportFunction != null)
            {
                this.reportFunction(report);
            }

            string nodeName = ragAction.NodeName;
            BigInteger nodeInstanceId = ragAction.NodeInstanceId;
            StopNodeAction stopNodeAction = new StopNodeAction(nodeName, nodeInstanceId);
            return stopNodeAction;
        }

        private FabricTestAction GetRestartNodeAction(NodeStateTransitionAction ragAction)
        {
            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.RestartNode_InvalidActionType_TelemetryId,
                ragAction == null || ragAction.ActionType != StateTransitionActionType.NodeRestart,
                StringResources.ChaosEngineError_RestartNode_InvalidActionType);

            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.RestartNode_InvalidNodeName_TelemetryId,
                string.IsNullOrEmpty(ragAction.NodeName),
                StringResources.ChaosEngineError_RestartNode_InvalidNodeName);

            string report = StringHelper.Format("Generating Action: {0}\n\t\tNodeName: {1}\n\t\tNodeInstanceId: {2}", ragAction.ActionType, ragAction.NodeName, ragAction.NodeInstanceId);
            Log.WriteInfo(TraceType, report);
            if (this.reportFunction != null)
            {
                this.reportFunction(report);
            }

            string nodeName = ragAction.NodeName;
            BigInteger nodeInstanceId = ragAction.NodeInstanceId;
            RestartNodeAction restartNodeAction = new RestartNodeAction(nodeName, nodeInstanceId);
            return restartNodeAction;
        }

        private FabricTestAction GetRemoveReplicaAction(ReplicaStateTransitionAction ragAction)
        {
            Uri serviceUri = ragAction.ServiceUri;
            Guid guid = ragAction.PartitionId;
            long replicaId = ragAction.ReplicaId;

            string report = StringHelper.Format("Generating Action: {0}\n\t\tService: {1}\n\t\tPartition: {2}\n\t\tReplicaId: {3}", ragAction.ActionType, ragAction.ServiceUri, ragAction.PartitionId, ragAction.ReplicaId);

            // Select specific replica using ReplicaSelector.
            ReplicaSelector replicaSelector = ReplicaSelector.ReplicaIdOf(PartitionSelector.PartitionIdOf(serviceUri, guid), replicaId);

            RemoveReplicaAction restartReplicaAction = new RemoveReplicaAction(replicaSelector);
            return restartReplicaAction;
        }

        private FabricTestAction GetRestartReplicaAction(ReplicaStateTransitionAction ragAction)
        {
            Uri serviceUri = ragAction.ServiceUri;
            Guid guid = ragAction.PartitionId;
            long replicaId = ragAction.ReplicaId;

            string report = StringHelper.Format("Generating Action: {0}\n\t\tService: {1}\n\t\tPartition: {2}\n\t\tReplicaId: {3}", ragAction.ActionType, ragAction.ServiceUri, ragAction.PartitionId, ragAction.ReplicaId);
            Log.WriteInfo(TraceType, report);
            if (this.reportFunction != null)
            {
                this.reportFunction(report);
            }

            // Select specific replica using ReplicaSelector.
            ReplicaSelector replicaSelector = ReplicaSelector.ReplicaIdOf(PartitionSelector.PartitionIdOf(serviceUri, guid), replicaId);

            RestartReplicaAction restartReplicaAction = new RestartReplicaAction(replicaSelector);
            return restartReplicaAction;
        }

        private FabricTestAction GetRestartCodePackageAction(RestartCodePackageStateTransitionAction ragAction)
        {
            Uri applicationName = ragAction.ApplicationName;
            string nodeName = ragAction.NodeName;
            string serviceManifest = ragAction.ServiceManifestName;
            string servicePackageActivationId = ragAction.ServicePackageActivationId;
            string codePackageName = ragAction.CodePackageName;

            string report = StringHelper.Format("Generating Action: {0}\n\t\tApplication: {1}\n\t\tNode: {2}\n\t\tCodePackage: {3}:{4}", ragAction.ActionType, applicationName, nodeName, serviceManifest, codePackageName);
            Log.WriteInfo(TraceType, report);
            if (this.reportFunction != null)
            {
                this.reportFunction(report);
            }

            RestartDeployedCodePackageAction restartCodePackageAction = new RestartDeployedCodePackageAction(
                nodeName,
                applicationName,
                serviceManifest,
                servicePackageActivationId,
                codePackageName,
                0);

            return restartCodePackageAction;
        }

        private FabricTestAction GetMovePrimaryReplicaAction(MovePrimaryReplicaStateTransitionAction ragAction)
        {
            Uri serviceUri = ragAction.ServiceUri;
            Guid guid = ragAction.PartitionId;
            string newNodeName = ragAction.NodeTo;

            string report = StringHelper.Format(
                "Generating Action: {0}\n\t\tService: {1}\n\t\tPartition: {2}\n\t\tTo: {3}",
                ragAction.ActionType,
                serviceUri,
                guid,
                newNodeName);
            Log.WriteInfo(TraceType, report);
            if (this.reportFunction != null)
            {
                this.reportFunction(report);
            }

            var partitionSelector = PartitionSelector.PartitionIdOf(serviceUri, guid);
            return new MovePrimaryAction(newNodeName, partitionSelector, ragAction.ForceMove);
        }

        private FabricTestAction GetMoveSecondaryReplicaAction(MoveSecondaryReplicaStateTransitionAction ragAction)
        {
            Requires.Argument("moveSecondaryReplicaStateTransitionAction", ragAction).NotNull();

            Uri serviceUri = ragAction.ServiceUri;
            Guid guid = ragAction.PartitionId;
            string currentNodeName = ragAction.NodeFrom;
            string newNodeName = ragAction.NodeTo;

            string report = StringHelper.Format(
                "Generating Action: {0}\n\t\tService: {1}\n\t\tPartition: {2}\n\t\tFrom: {3} To: {4}",
                ragAction.ActionType,
                serviceUri,
                guid,
                currentNodeName,
                newNodeName);
            Log.WriteInfo(TraceType, report);
            if (this.reportFunction != null)
            {
                this.reportFunction(report);
            }

            var partitionSelector = PartitionSelector.PartitionIdOf(serviceUri, guid);
            return new MoveSecondaryAction(currentNodeName, newNodeName, partitionSelector, ragAction.ForceMove);
        }
    }
}