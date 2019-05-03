// ----------------------------------------------------------------------
//  <copyright file="PrimaryMoveAnalysisAgent.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Cluster.ClusterQuery;
    using ClusterAnalysis.AnalysisCore.Callback;
    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;
    using ClusterAnalysis.AnalysisCore.Insights.Agents;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Config;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Store;
    using ClusterAnalysis.TraceAccessLayer;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RAP;
    using Assert = Common.Util.Assert;
    using ReconfigurationType = Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA.ReconfigurationType;

    /// <summary>
    /// Agent for analyzing details of a Reconfiguration that has happened.
    /// </summary>
    internal class PrimaryMoveAnalysisAgent : Agent
    {
        private readonly IPrimaryReplicaContextPersistentStore primaryReplicaContextStore;
        private readonly IPrimaryMoveAnalysisQueryStoreReader primaryMoveAnalysisQueryStoreReader;
        private readonly IPrimaryMoveAnalysisEventStoreReader primaryMoveAnalysisEventStoreReader;

        /// <inheritdoc />
        internal PrimaryMoveAnalysisAgent(
            Config config,
            ITaskRunner taskRunner,
            ILogger logger,
            IStoreProvider storeProvider,
            ITraceStoreReader eventStoreReader,
            ITraceStoreReader queryStoreReader,
            IClusterQuery clusterQuery,
            CancellationToken token) : base(taskRunner, logger, storeProvider, eventStoreReader, queryStoreReader, clusterQuery, token)
        {
            // This is to keep track of the previous location of the primary
            this.primaryReplicaContextStore = new PrimaryReplicaContextPersistentStoreWrapper(this.Logger, storeProvider, this.CancelToken);
            this.primaryMoveAnalysisQueryStoreReader = new PrimaryMoveAnalysisQueryStoreReaderWrapper(this.Logger, this.QueryStoreReader, this.CancelToken);
            this.primaryMoveAnalysisEventStoreReader = new PrimaryMoveAnalysisEventStoreReaderWrapper(this.Logger, this.EventStoreReader, this.CancelToken);
        }

        /// <inheritdoc />
        public override AgentIdentifier AgentIdentifier
        {
            get { return AgentIdentifier.PrimaryMoveAnalysisAgent; }
        }

        /// <summary>
        /// PrimaryMoveAnalysisAgent is interested in Reconfiguration scenario, from there it will back track to attribute a reason
        /// </summary>
        public override IEnumerable<NotifyFilter> NotifyFilters
        {
            get { return new List<NotifyFilter> { new NotifyFilter(Scenario.Reconfiguration, TimeSpan.Zero) }; }
        }

        /// <inheritdoc />
        public override Task StopAsync(StopOptions options)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Not all reconfigurations are interesting, here we filter further.
        /// </summary>
        /// <param name="targetEntity"></param>
        /// <param name="data"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        public override async Task<bool> IsEligibleForAnalysisAsync(BaseEntity targetEntity, ScenarioData data)
        {
            Assert.IsNotNull(targetEntity, "targetEntity");
            Assert.IsNotNull(data, "data");

            var reconfig = data.TraceRecord as ReconfigurationCompletedTraceRecord;
            Assert.IsNotNull(
               reconfig,
               string.Format(CultureInfo.InvariantCulture, "Actual Type: {0}, Expected: {1}", data.TraceRecord.GetType(), typeof(ReconfigurationCompletedTraceRecord)));

            var partitionEntity = targetEntity as PartitionEntity;

            if (partitionEntity == null)
            {
                this.Logger.LogWarning("targetEntity is not a ParitionEntity, thus ineligible for primary move analysis.", reconfig.PartitionId);
                return false;
            }

            // We only do analysis for stateful service partitions
            if (partitionEntity.ServiceKind != System.Fabric.Query.ServiceKind.Stateful)
            {
                this.Logger.LogWarning("Partition Id {0} does not belong to a stateful partition, so PrimaryMoveAnalysis does not apply.", reconfig.PartitionId);
                return false;
            }

            bool primaryPreviousLocationKnown = await this.primaryReplicaContextStore.IsPartitionIdKnownAsync(reconfig.PartitionId).ConfigureAwait(false);

            // Registers the primary location for the first time depending on RA.ReconfigurationCompleted with ReconfigType == Other
            // Afterwards, updates of the primary location is done in the DoAnalysisAsync which happens only for Failover and SwapPrimary type reconfigurations
            if (!primaryPreviousLocationKnown)
            {
                PrimaryReplicaContext primaryReplicaContext = new PrimaryReplicaContext(reconfig.PartitionId, reconfig.NodeName, reconfig.NodeInstanceId, reconfig.TimeStamp.Ticks);
                await this.primaryReplicaContextStore.SavePrimaryReplicaContextAsync(primaryReplicaContext).ConfigureAwait(false);
            }

            return this.ShouldPerformAnalysis(reconfig, primaryPreviousLocationKnown);
        }

        /// <inheritdoc />
        public override async Task<Continuation> DoAnalysisAsync(AnalysisContainer analysis)
        {
            if (analysis.GetProgressedTill() == ProgressTracker.NotStarted)
            {
                PrimaryMoveAnalysisEvent primaryMoveAnalysisEvent = analysis.AnalysisEvent as PrimaryMoveAnalysisEvent;

                primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.Unknown;

                var reconfigRecord = primaryMoveAnalysisEvent.TriggerReconfigurationCompletedTraceRecord;

                primaryMoveAnalysisEvent.PreviousPrimaryContext = await this.primaryReplicaContextStore.GetPrimaryReplicaContextAsync(reconfigRecord.PartitionId).ConfigureAwait(false);

                if (primaryMoveAnalysisEvent.PreviousPrimaryContext == null)
                {
                    this.Logger.LogWarning("PreviousPrimaryContext is null, cannot perform PrimaryMoveAnalysis.");
                    analysis.SetProgressedTill(ProgressTracker.Finished);
                    return Continuation.Done;
                }

                primaryMoveAnalysisEvent.CurrentPrimaryContext = new PrimaryReplicaContext(reconfigRecord.PartitionId, reconfigRecord.NodeName, reconfigRecord.NodeInstanceId, reconfigRecord.TimeStamp.Ticks);

                if (primaryMoveAnalysisEvent.CurrentPrimaryContext == null)
                {
                    this.Logger.LogWarning("CurrentPrimaryContext is null, cannot perform PrimaryMoveAnalysis.");
                    analysis.SetProgressedTill(ProgressTracker.Finished);
                    return Continuation.Done;
                }

                // CurrentPrimaryContext becomes the PreviousPrimaryContext for the next analysis
                await this.primaryReplicaContextStore.SavePrimaryReplicaContextAsync(primaryMoveAnalysisEvent.CurrentPrimaryContext).ConfigureAwait(false);
                analysis.SetProgressedTill(ProgressTracker.Checkpoint1);
                return Continuation.ResumeImmediately;
            }
            else if (analysis.GetProgressedTill() == ProgressTracker.Checkpoint1)
            {
                PrimaryMoveAnalysisEvent primaryMoveAnalysisEvent = analysis.AnalysisEvent as PrimaryMoveAnalysisEvent;

                if (primaryMoveAnalysisEvent.TriggerReconfigurationCompletedTraceRecord.ReconfigType == ReconfigurationType.Failover)
                {
                    primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.Failover;
                    analysis.SetProgressedTill(ProgressTracker.Checkpoint2);
                    return Continuation.ResumeImmediately;
                }
                else if (primaryMoveAnalysisEvent.TriggerReconfigurationCompletedTraceRecord.ReconfigType == ReconfigurationType.SwapPrimary)
                {
                    primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.SwapPrimary;
                    analysis.SetProgressedTill(ProgressTracker.Checkpoint3);
                    return Continuation.ResumeImmediately;
                }
            }
            else if (analysis.GetProgressedTill() == ProgressTracker.Checkpoint2)
            {
                PrimaryMoveAnalysisEvent primaryMoveAnalysisEvent = analysis.AnalysisEvent as PrimaryMoveAnalysisEvent;
                bool dueToNodeDown = await this.AnalyzeNodeDownAsync(primaryMoveAnalysisEvent).ConfigureAwait(false);
                if (!dueToNodeDown)
                {
                    analysis.SetProgressedTill(ProgressTracker.Checkpoint4);
                    return Continuation.ResumeImmediately;
                }
                else
                {
                    analysis.SetProgressedTill(ProgressTracker.Finished);
                    primaryMoveAnalysisEvent.AnalysisEndTimeStamp = DateTime.UtcNow;
                    return Continuation.Done;
                }
            }
            else if (analysis.GetProgressedTill() == ProgressTracker.Checkpoint3)
            {
                PrimaryMoveAnalysisEvent primaryMoveAnalysisEvent = analysis.AnalysisEvent as PrimaryMoveAnalysisEvent;
                await this.AnalyzeCRMOperationAsync(primaryMoveAnalysisEvent).ConfigureAwait(false);

                analysis.SetProgressedTill(ProgressTracker.Finished);
                primaryMoveAnalysisEvent.AnalysisEndTimeStamp = DateTime.UtcNow;
                return Continuation.Done;
            }
            else if (analysis.GetProgressedTill() == ProgressTracker.Checkpoint4)
            {
                PrimaryMoveAnalysisEvent primaryMoveAnalysisEvent = analysis.AnalysisEvent as PrimaryMoveAnalysisEvent;

                var replicaStateChangeTraceRecordList = await this.primaryMoveAnalysisQueryStoreReader.GetReplicaStateChangeTraceRecordsAsync(primaryMoveAnalysisEvent).ConfigureAwait(false);

                if (replicaStateChangeTraceRecordList == null || !replicaStateChangeTraceRecordList.Any())
                {
                    this.Logger.LogWarning("No replica closing traces found with duration {0}, cannot perform further analysis.", primaryMoveAnalysisEvent.GetDuration());
                    analysis.SetProgressedTill(ProgressTracker.Finished);
                    return Continuation.Done;
                }

                primaryMoveAnalysisEvent.ReasonActivityId = replicaStateChangeTraceRecordList.First().ReasonActivityId;
                primaryMoveAnalysisEvent.ReasonActivityType = replicaStateChangeTraceRecordList.First().ReasonActivityType;

                primaryMoveAnalysisEvent.AddCorrelatedTraceRecordRange(replicaStateChangeTraceRecordList);

                if (replicaStateChangeTraceRecordList.First().ReasonActivityType == ActivityType.ServicePackageEvent)
                {
                    primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.ApplicationHostDown;

                    analysis.SetProgressedTill(ProgressTracker.Checkpoint5);
                    return Continuation.ResumeImmediately;
                }
                else if (replicaStateChangeTraceRecordList.First().ReasonActivityType == ActivityType.ClientReportFaultEvent || replicaStateChangeTraceRecordList.First().ReasonActivityType == ActivityType.ServiceReportFaultEvent)
                {
                    // TODO: Break report fault analysis into two separate analyses because ReplicaStateChange already shows which one of the two happened
                    primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.ClientApiReportFault;

                    analysis.SetProgressedTill(ProgressTracker.Checkpoint6);
                    return Continuation.ResumeImmediately;
                }
            }
            else if (analysis.GetProgressedTill() == ProgressTracker.Checkpoint5)
            {
                PrimaryMoveAnalysisEvent primaryMoveAnalysisEvent = analysis.AnalysisEvent as PrimaryMoveAnalysisEvent;
                await this.AnalyzeAppHostDownAsync(primaryMoveAnalysisEvent).ConfigureAwait(false);

                analysis.SetProgressedTill(ProgressTracker.Finished);
                primaryMoveAnalysisEvent.AnalysisEndTimeStamp = DateTime.UtcNow;
                return Continuation.Done;
            }
            else if (analysis.GetProgressedTill() == ProgressTracker.Checkpoint6)
            {
                PrimaryMoveAnalysisEvent primaryMoveAnalysisEvent = analysis.AnalysisEvent as PrimaryMoveAnalysisEvent;
                await this.AnalyzeReportFaultAsync(primaryMoveAnalysisEvent).ConfigureAwait(false);

                analysis.SetProgressedTill(ProgressTracker.Finished);
                primaryMoveAnalysisEvent.AnalysisEndTimeStamp = DateTime.UtcNow;
                return Continuation.Done;
            }

            throw new Exception(string.Format(CultureInfo.InvariantCulture, "Progress Stage {0} not Valid", analysis.GetProgressedTill()));
        }

        /// <inheritdoc />
        public override AnalysisContainer CreateNewAnalysisContainer(TraceRecord traceRecord)
        {
            return new AnalysisContainer(new PrimaryMoveAnalysisEvent((ReconfigurationCompletedTraceRecord)traceRecord), AgentIdentifier.PrimaryMoveAnalysisAgent);
        }

        private async Task<bool> AnalyzeNodeDownAsync(PrimaryMoveAnalysisEvent primaryMoveAnalysisEvent)
        {
            IEnumerable<NodeDownTraceRecord> nodeDownTraceRecords = await this.primaryMoveAnalysisEventStoreReader.GetNodeDownTraceRecordsAsync(primaryMoveAnalysisEvent).ConfigureAwait(false);

            if (!nodeDownTraceRecords.Any())
            {
                IEnumerable<NodeUpTraceRecord> nodeUpTraceRecords = await this.primaryMoveAnalysisEventStoreReader.GetNodeUpTraceRecordsAsync(primaryMoveAnalysisEvent).ConfigureAwait(false);

                if (!nodeUpTraceRecords.Any())
                {
                    this.Logger.LogWarning("No node up or node down traces found with duration {0}, will try other failover possibilities.", primaryMoveAnalysisEvent.GetDuration());
                    return false;
                }
                else
                {
                    primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.NodeDown;
                    primaryMoveAnalysisEvent.AddCorrelatedTraceRecordRange(nodeUpTraceRecords);

                    return true;
                }
            }
            else
            {
                primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.NodeDown;
                primaryMoveAnalysisEvent.AddCorrelatedTraceRecordRange(nodeDownTraceRecords);

                return true;
            }
        }

        private async Task AnalyzeAppHostDownAsync(PrimaryMoveAnalysisEvent primaryMoveAnalysisEvent)
        {
            IEnumerable<ApplicationHostTerminatedTraceRecord> appHostDownTraceRecords = await this.primaryMoveAnalysisQueryStoreReader.GetApplicationHostTerminatedTraceRecordsAsync(primaryMoveAnalysisEvent).ConfigureAwait(false);

            if (!appHostDownTraceRecords.Any())
            {
                this.Logger.LogWarning("No application host down traces found with the reason activity id {0} and duration {1}, cannot perform further analysis.", primaryMoveAnalysisEvent.ReasonActivityId, primaryMoveAnalysisEvent.GetDuration());
                return;
            }

            primaryMoveAnalysisEvent.AddCorrelatedTraceRecordRange(appHostDownTraceRecords);
        }

        private async Task AnalyzeReportFaultAsync(PrimaryMoveAnalysisEvent primaryMoveAnalysisEvent)
        {
            // This is RAP api report fault
            IEnumerable<ApiReportFaultTraceRecord> apiReportFaultTraceRecords = await this.primaryMoveAnalysisQueryStoreReader.GetApiReportFaultTraceRecordsAsync(primaryMoveAnalysisEvent).ConfigureAwait(false);

            if (apiReportFaultTraceRecords.Any())
            {
                primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.ServiceApiReportFault;
                primaryMoveAnalysisEvent.AddCorrelatedTraceRecordRange(apiReportFaultTraceRecords);
            }
            else
            {
                //// this is client api report fault

                var clientApiBeginReportFaultTraceRecords = await this.primaryMoveAnalysisQueryStoreReader.GetBeginReportFaultTraceRecordsAsync(primaryMoveAnalysisEvent).ConfigureAwait(false);

                if (clientApiBeginReportFaultTraceRecords.Any())
                {
                    primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.ClientApiReportFault;
                    primaryMoveAnalysisEvent.AddCorrelatedTraceRecordRange(clientApiBeginReportFaultTraceRecords);
                }
                else
                {
                    var clientApiReportFaultTraceRecords = await this.primaryMoveAnalysisQueryStoreReader.GetReportFaultTraceRecordsAsync(primaryMoveAnalysisEvent).ConfigureAwait(false);

                    if (clientApiReportFaultTraceRecords.Any())
                    {
                        primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.ClientApiReportFault;
                        primaryMoveAnalysisEvent.AddCorrelatedTraceRecordRange(clientApiReportFaultTraceRecords);
                    }
                    else
                    {
                        this.Logger.LogWarning("No report fault traces found with activity id {0} and duration {1}, cannot perform further analysis.", primaryMoveAnalysisEvent.ReasonActivityId, primaryMoveAnalysisEvent.GetDuration());
                        return;
                    }
                }
            }
        }

        private async Task AnalyzeCRMOperationAsync(PrimaryMoveAnalysisEvent primaryMoveAnalysisEvent)
        {
            var effectiveOperationRecords = await this.primaryMoveAnalysisQueryStoreReader.GetEffectiveOperationTraceRecordsAsync(primaryMoveAnalysisEvent).ConfigureAwait(false);

            if (!effectiveOperationRecords.Any())
            {
                this.Logger.LogWarning("No crm operation traces found with duration {0}.", primaryMoveAnalysisEvent.GetDuration());
                return;
            }

            foreach (var crme in effectiveOperationRecords)
            {
                bool interestingEvent = true;

                if (crme.SchedulerPhase == PLBSchedulerActionType.ClientApiMovePrimary)
                {
                    primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.ClientApiMovePrimary;
                }
                else if (crme.SchedulerPhase == PLBSchedulerActionType.ClientApiPromoteToPrimary)
                {
                    primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.ClientPromoteToPrimaryApiCall;
                }
                else if (crme.SchedulerPhase == PLBSchedulerActionType.Upgrade)
                {
                    // upgrade or deactivate node
                    // DeactivateNodeCompleted happens later than RA.ReconfigurationCompleted, so this query will not find the DeactivateNodeCompleted trace record.
                    // We may want to look if a DeactivateNode was issued and if it was on going.
                    // Ideally FM should send some activity ID to RA and that should be emitted in RA.ReconfigurationCompleted eliminating the need for guesswork.
                    primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.Upgrade;
                }
                else if (crme.SchedulerPhase == PLBSchedulerActionType.NewReplicaPlacementWithMove)
                {
                    primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.MakeRoomForNewReplicas;
                }
                else if (crme.SchedulerPhase == PLBSchedulerActionType.QuickLoadBalancing || crme.SchedulerPhase == PLBSchedulerActionType.LoadBalancing)
                {
                    primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.LoadBalancing;
                }
                else if (crme.SchedulerPhase == PLBSchedulerActionType.ConstraintCheck)
                {
                    primaryMoveAnalysisEvent.Reason = PrimaryMoveReason.ConstraintViolation;
                }
                else
                {
                    interestingEvent = false;
                }

                if (interestingEvent)
                {
                    primaryMoveAnalysisEvent.AddCorrelatedTraceRecord(crme);
                    return;
                }
                else
                {
                    this.Logger.LogWarning("No relevant CRM operation traces found, cannot perform further analysis.");
                    return;
                }
            }
        }

        private bool ShouldPerformAnalysis(ReconfigurationCompletedTraceRecord reconfigRecord, bool primaryReplicaPreviousLocationKnown)
        {
            return primaryReplicaPreviousLocationKnown &&
                    (reconfigRecord.ReconfigType == ReconfigurationType.Failover || reconfigRecord.ReconfigType == ReconfigurationType.SwapPrimary) &&
                    reconfigRecord.Result == ReconfigurationResult.Completed;
        }
    }
}