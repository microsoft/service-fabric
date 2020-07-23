// ----------------------------------------------------------------------
//  <copyright file="SimpleCallbackStore.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterInsight.InsightCore.DataSetLayer.CallbackStore
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterInsight.Common;
    using ClusterInsight.Common.Runtime;
    using ClusterInsight.Common.Store;
    using ClusterInsight.Common.Util;
    using ClusterInsight.EventStore;
    using ClusterInsight.InsightCore.DataSetLayer.DataModel;
    using ClusterInsight.InsightCore.DataSetLayer.FabricTraces;
    using ClusterInsight.InsightCore.Insights.Agents;

    /// <inheritdoc />
    internal class SimpleCallbackStore : BaseCallbackStore
    {
        private static readonly TimeSpan DelayBetweenNewSignalAvailabilityCheck = TimeSpan.FromSeconds(30);

        private static IReadOnlyList<Scenario> supportedCallbacks = new List<Scenario>
        {
            Scenario.ProcessCrash,
            Scenario.QuorumLoss,
            Scenario.Reconfiguration,
            Scenario.NewReplicaHealthEvent,
            Scenario.NewPartitionHealthEvent,
            Scenario.ExpiredPartitionHealthEvent,
            Scenario.ExpiredReplicaHealthEvent,
            Scenario.RunAsyncUnhandledException
        };

        private IEventStore eventStore;

        /// <summary>
        /// Create an instance of <see cref="SimpleCallbackStore"/>
        /// </summary>
        /// <remarks>
        /// Keeping private to control who can create an instance.
        /// </remarks>
        internal SimpleCallbackStore(IInsightRuntime runtime, FabricTraceStore traceStore, CancellationToken token) : base(
            runtime,
            token)
        {
            // TODO: Think the Policy passing through
            this.eventStore = new EventStoreImpl(runtime, traceStore, AgeBasedRetentionPolicy.OneWeek, token);
            this.SetStoreIsBehindByDuration(TimeSpan.FromMinutes(5));
        }

        #region Public_Abstractions

        /// <inheritdoc />
        public override IEnumerable<Scenario> GetCallbackSupportedScenario()
        {
            return supportedCallbacks;
        }

        #endregion Public_Abstractions

        #region Protected_Abstractions

        protected override Task ResetInternalAsync(CancellationToken token)
        {
            // TODO Think through this Reset and passing of stop options.
            return this.eventStore.ResetAsync(StopOptions.Default, token);
        }

        /// <inheritdoc />
        protected override async Task<IEnumerable<FabricEvent>> GetSignalsForScenarioAsync(Scenario scenario, Duration duration, CancellationToken token)
        {
            switch (scenario)
            {
                case Scenario.QuorumLoss:
                    return await this.eventStore.GetFabricEventsOfTypeAsync<QuorumLossEvent>(duration, null, token).ConfigureAwait(false);

                case Scenario.ProcessCrash:
                    return await this.eventStore.GetFabricEventsOfTypeAsync<AbnormalProcessTerminationEvent>(duration, null, token).ConfigureAwait(false);

                case Scenario.RunAsyncUnhandledException:
                    return await this.eventStore.GetFabricEventsOfTypeAsync<RunAsyncUnhandledExceptionEvent>(duration, null, token).ConfigureAwait(false);

                case Scenario.NewPartitionHealthEvent:
                    return await this.eventStore.GetFabricEventsOfTypeAsync<GenericPartitionHealthEvent>(duration, null, token).ConfigureAwait(false);

                case Scenario.ExpiredPartitionHealthEvent:
                    return this.FilterByType<GenericPartitionHealthEvent>(
                        await this.eventStore.GetFabricEventsOfTypeAsync<FabricHealthEventExpiredEvent>(duration, null, token).ConfigureAwait(false));

                case Scenario.NewReplicaHealthEvent:
                    return await this.eventStore.GetFabricEventsOfTypeAsync<GenericReplicaHealthEvent>(duration, null, token).ConfigureAwait(false);

                case Scenario.ExpiredReplicaHealthEvent:
                    return this.FilterByType<GenericReplicaHealthEvent>(
                        await this.eventStore.GetFabricEventsOfTypeAsync<FabricHealthEventExpiredEvent>(duration, null, token).ConfigureAwait(false));

                case Scenario.Reconfiguration:
                    return await this.eventStore.GetFabricEventsOfTypeAsync<ReconfigurationCompleteEvent>(duration, null, token).ConfigureAwait(false);

                default:
                    throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Scenario '{0}' not supported", scenario));
            }
        }

        /// <inheritdoc />
        protected override TimeSpan GetDelayBetweenCheckForNewSignalAvailability()
        {
            return DelayBetweenNewSignalAvailabilityCheck;
        }

        private IEnumerable<T> FilterByType<T>(IEnumerable<FabricHealthEventExpiredEvent> original) where T : FabricEvent
        {
            return original.Where(
                item =>
                {
                    Assert.IsNotNull(item.ExpiredHealthEvent, "Expired Health Event is Null");
                    var other = item.ExpiredHealthEvent as T;
                    return other != null;
                }).Select(item => item.ExpiredHealthEvent as T);
        }

        #endregion Protected_Abstractions
    }
}