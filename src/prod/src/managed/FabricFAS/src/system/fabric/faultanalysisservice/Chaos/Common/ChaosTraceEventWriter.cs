// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Chaos
{
    using System.Collections.Generic;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Chaos.RandomActionGenerator;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Globalization;

    internal static class ChaosTraceEventWriter
    {
        private const string ListEntryDelimeter = ",";

        public static void WriteChaosStartedTraceEvent(ChaosParameters chaosParameters)
        {
            Requires.Argument<ChaosParameters>("chaosParameters", chaosParameters).NotNull();

            Guid eventInstanceId = Guid.NewGuid();
            long maxConcurrentFaults = chaosParameters.MaxConcurrentFaults;
            double timeToRunInSeconds = chaosParameters.TimeToRun.TotalSeconds;
            double maxClusterStabilizationTimeoutInSeconds = chaosParameters.MaxClusterStabilizationTimeout.TotalSeconds;
            double waitTimeBetweenIterationsInSeconds = chaosParameters.WaitTimeBetweenIterations.TotalSeconds;
            double waitTimeBetweenFaultsInSeconds = chaosParameters.WaitTimeBetweenFaults.TotalSeconds;
            bool enableMoveReplicaFaults = chaosParameters.EnableMoveReplicaFaults;

            ChaosTargetFilter targetFilter = chaosParameters.ChaosTargetFilter;
            IList<string> nodeTypeList = targetFilter != null ? targetFilter.NodeTypeInclusionList : null;
            IList<string> applicationList = targetFilter != null ? targetFilter.ApplicationInclusionList : null;
            string nodeTypeListAsString = string.Empty;
            string applicationListAsString = string.Empty;

            if (targetFilter != null && nodeTypeList != null)
            {
                nodeTypeListAsString = string.Join(ListEntryDelimeter, nodeTypeList);
            }

            if (targetFilter != null && applicationList != null)
            {
                applicationListAsString = string.Join(ListEntryDelimeter, applicationList);
            }

            var clusterHealthPolicy = chaosParameters.ClusterHealthPolicy;

            string clusterHealthPolicyAsString = string.Empty;

            if (clusterHealthPolicy != null)
            {
                clusterHealthPolicyAsString = clusterHealthPolicy.ToString();
            }

            var context = chaosParameters.Context;

            string contextAsString = string.Empty;

            if (context != null)
            {
                contextAsString = string.Join(ListEntryDelimeter, context);
            }

            FabricEvents.Events.ChaosStartedEvent(
                eventInstanceId,
                maxConcurrentFaults,
                timeToRunInSeconds,
                maxClusterStabilizationTimeoutInSeconds,
                waitTimeBetweenIterationsInSeconds,
                waitTimeBetweenFaultsInSeconds,
                enableMoveReplicaFaults,
                nodeTypeListAsString,
                applicationListAsString,
                clusterHealthPolicyAsString,
                contextAsString);
        }

        public static void WriteChaosStoppedTraceEvent(ChaosStopReason reason)
        {
            Guid eventInstanceId = Guid.NewGuid();

            FabricEvents.Events.ChaosStoppedEvent(
                eventInstanceId,
                reason.ToString());
        }

        public static void WriteChaosFaultScheduledTraceEvent(IList<StateTransitionAction> faultList)
        {
            foreach (var fault in faultList)
            {
                Guid eventInstanceId = Guid.NewGuid();

                RestartNodeStateTransitionAction nodeRestartFault = fault as RestartNodeStateTransitionAction;

                if (nodeRestartFault != null)
                {
                    WriteRestartNodeFaultScheduledTraceEvent(eventInstanceId, nodeRestartFault);
                }
                else
                {
                    RestartReplicaStateTransitionAction replicaRestartFault = fault as RestartReplicaStateTransitionAction;

                    if (replicaRestartFault != null)
                    {
                        WriteRestartReplicaFaultScheduledTraceEvent(eventInstanceId, replicaRestartFault);
                    }
                    else
                    {
                        RemoveReplicaStateTransitionAction replicaRemoveFault = fault as RemoveReplicaStateTransitionAction;

                        if (replicaRemoveFault != null)
                        {
                            WriteRemoveReplicaFaultScheduledTraceEvent(eventInstanceId, replicaRemoveFault);
                        }
                        else
                        {
                            RestartCodePackageStateTransitionAction codePackageRestartFault = fault as RestartCodePackageStateTransitionAction;

                            if (codePackageRestartFault != null)
                            {
                                WriteRestartCodepackageFaultScheduledTraceEvent(eventInstanceId, codePackageRestartFault);
                            }
                            else
                            {
                                MovePrimaryReplicaStateTransitionAction movePrimaryFault = fault as MovePrimaryReplicaStateTransitionAction;

                                if (movePrimaryFault != null)
                                {
                                    WriteMovePrimaryReplicaFaultScheduledTraceEvent(eventInstanceId, movePrimaryFault);
                                }
                                else
                                {
                                    MoveSecondaryReplicaStateTransitionAction moveSecondaryFault = fault as MoveSecondaryReplicaStateTransitionAction;

                                    if (moveSecondaryFault != null)
                                    {
                                        WriteMoveSecondaryReplicaFaultScheduledTraceEvent(eventInstanceId, moveSecondaryFault);
                                    }
                                    else
                                    {
                                        throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Unrecognized Chaos fault type {0}, cannot write trace event.", fault.GetType().Name));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        private static void WriteRestartNodeFaultScheduledTraceEvent(Guid eventInstanceId, RestartNodeStateTransitionAction nodeRestartFault)
        {
            FabricEvents.Events.ChaosRestartNodeFaultScheduledEvent(
                eventInstanceId,
                nodeRestartFault.NodeName,
                nodeRestartFault.NodeInstanceId,
                nodeRestartFault.StateTransitionActionGroupId,
                nodeRestartFault.StateTransitionActionId);
        }

        private static void WriteRestartReplicaFaultScheduledTraceEvent(Guid eventInstanceId, RestartReplicaStateTransitionAction replicaRestartFault)
        {
            FabricEvents.Events.ChaosRestartReplicaFaultScheduledEvent(
                eventInstanceId,
                replicaRestartFault.PartitionId,
                replicaRestartFault.ReplicaId,
                replicaRestartFault.StateTransitionActionGroupId,
                replicaRestartFault.StateTransitionActionId,
                replicaRestartFault.ServiceUri.OriginalString);
        }

        private static void WriteRemoveReplicaFaultScheduledTraceEvent(Guid eventInstanceId, RemoveReplicaStateTransitionAction replicaRemoveFault)
        {
            FabricEvents.Events.ChaosRemoveReplicaFaultScheduledEvent(
                eventInstanceId,
                replicaRemoveFault.PartitionId,
                replicaRemoveFault.ReplicaId,
                replicaRemoveFault.StateTransitionActionGroupId,
                replicaRemoveFault.StateTransitionActionId,
                replicaRemoveFault.ServiceUri.OriginalString);
        }

        private static void WriteRestartCodepackageFaultScheduledTraceEvent(Guid eventInstanceId, RestartCodePackageStateTransitionAction codePackageRestartFault)
        {
            FabricEvents.Events.ChaosRestartCodePackageFaultScheduledEvent(
                eventInstanceId,
                codePackageRestartFault.ApplicationName.OriginalString,
                codePackageRestartFault.StateTransitionActionGroupId,
                codePackageRestartFault.StateTransitionActionId,
                codePackageRestartFault.NodeName,
                codePackageRestartFault.ServiceManifestName,
                codePackageRestartFault.CodePackageName,
                codePackageRestartFault.ServicePackageActivationId == null ? string.Empty : codePackageRestartFault.ServicePackageActivationId);
        }

        private static void WriteMovePrimaryReplicaFaultScheduledTraceEvent(Guid eventInstanceId, MovePrimaryReplicaStateTransitionAction movePrimaryFault)
        {
            FabricEvents.Events.ChaosMovePrimaryFaultScheduledEvent(
                eventInstanceId,
                movePrimaryFault.PartitionId,
                movePrimaryFault.StateTransitionActionGroupId,
                movePrimaryFault.StateTransitionActionId,
                movePrimaryFault.ServiceUri.OriginalString,
                movePrimaryFault.NodeTo,
                movePrimaryFault.ForceMove);
        }

        private static void WriteMoveSecondaryReplicaFaultScheduledTraceEvent(Guid eventInstanceId, MoveSecondaryReplicaStateTransitionAction moveSecondaryFault)
        {
            FabricEvents.Events.ChaosMoveSecondaryFaultScheduledEvent(
                eventInstanceId,
                moveSecondaryFault.PartitionId,
                moveSecondaryFault.StateTransitionActionGroupId,
                moveSecondaryFault.StateTransitionActionId,
                moveSecondaryFault.ServiceUri.OriginalString,
                moveSecondaryFault.NodeFrom,
                moveSecondaryFault.NodeTo,
                moveSecondaryFault.ForceMove);
        }
    }
}