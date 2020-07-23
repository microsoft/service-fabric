// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.DataReader
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.TraceAccessLayer;
    using ClusterAnalysis.TraceAccessLayer.StoreConnection;
    using EventsReader.ConfigReader;
    using EventsReader.Exceptions;
    using EventsReader.LogProvider;
    using EventsStore.EventsModels;
    using EventsStore.EventsModels.Application;
    using EventsStore.EventsModels.Cluster;
    using EventsStore.EventsModels.Node;
    using EventsStore.EventsModels.Partition;
    using EventsStore.EventsModels.Replica;
    using EventsStore.EventsModels.Service;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    internal class EventStoreReader
    {
        private static TimeSpan MaxDurationForCorrelatingEvent = TimeSpan.FromHours(1);

        private ITraceStoreReader traceStoreReader;

        public EventStoreReader()
        {
            TraceStoreConnectionInformation connectionInfo = null;

            if (ClusterSettingsReader.IsOneBoxEnvironment())
            {
                EventStoreLogger.Logger.LogMessage("One Box Environment. Configuring local Reader");
                connectionInfo = new LocalTraceStoreConnectionInformation(
                    FabricEnvironment.GetDataRoot(),
                    FabricEnvironment.GetLogRoot(),
                    FabricEnvironment.GetCodePath());
            }
            else
            {
                EventStoreLogger.Logger.LogMessage("Cloud Environment. Configuring Cloud Reader");
                var operationalConsumer = ClusterSettingsReader.OperationalConsumer;
                if (operationalConsumer == null)
                {
                    throw new ConnectionParsingException(ErrorCodes.AzureConnectionInformationMissing, "Config is Missing Operational Store Connection information");
                }

                connectionInfo = new AzureTraceStoreConnectionInformation(
                    operationalConsumer.Connection.AccountName,
                    operationalConsumer.Connection.AccountKey,
                    operationalConsumer.TablesPrefix,
                    null,
                    operationalConsumer.DeploymentId,
                    EventStoreLogProvider.LogProvider);
            }

            var connection = new TraceStoreConnection(connectionInfo, EventStoreLogProvider.LogProvider);
            traceStoreReader = connection.EventStoreReader;
        }

        public async Task<IList<NodeEvent>> GetNodeEventsAsync(string nodeName, Duration duration, IList<Type> types, CancellationToken token)
        {
            var filter = FilterFactory.CreateNodeFilter(this.traceStoreReader, types, nodeName);
            var allNodeEvents = (await this.traceStoreReader.ReadTraceRecordsAsync(duration, filter, token).ConfigureAwait(false)).Select(oneRecord => NodeEventAdapter.Convert(oneRecord)).ToList();

            if (this.traceStoreReader.IsPropertyLevelFilteringSupported() || string.IsNullOrWhiteSpace(nodeName))
            {
                return allNodeEvents;
            }

            return allNodeEvents.Where(oneEvent => oneEvent.NodeName.Equals(nodeName, StringComparison.InvariantCultureIgnoreCase)).ToList();
        }

        public async Task<IList<ApplicationEvent>> GetApplicationEventsAsync(string applicationName, Duration duration, IList<Type> types, CancellationToken token)
        {
            var filter = FilterFactory.CreateApplicationFilter(this.traceStoreReader, types, applicationName);
            var allApplicationEvents = (await this.traceStoreReader.ReadTraceRecordsAsync(duration, filter, token).ConfigureAwait(false)).Select(oneRecord => ApplicationEventAdapter.Convert(oneRecord)).ToList();

            if (this.traceStoreReader.IsPropertyLevelFilteringSupported() || string.IsNullOrWhiteSpace(applicationName))
            {
                return allApplicationEvents;
            }

            // Today, local reader doesn't support filtering, so we filter on the model event.
            return allApplicationEvents.Where(oneEvent => oneEvent.ApplicationId.Equals(ApplicationEvent.TransformAppName(applicationName), StringComparison.InvariantCultureIgnoreCase)).ToList();
        }

        public async Task<IList<ServiceEvent>> GetServiceEventsAsync(string serviceName, Duration duration, IList<Type> types, CancellationToken token)
        {
            var filter = FilterFactory.CreateServiceFilter(this.traceStoreReader, types, serviceName);
            var allServiceEvents = (await this.traceStoreReader.ReadTraceRecordsAsync(duration, filter, token).ConfigureAwait(false)).Select(oneRecord => ServiceEventAdapter.Convert(oneRecord)).ToList();

            if (this.traceStoreReader.IsPropertyLevelFilteringSupported() || string.IsNullOrWhiteSpace(serviceName))
            {
                return allServiceEvents;
            }

            return allServiceEvents.Where(oneEvent => oneEvent.ServiceId.Equals(ServiceEvent.TransformServiceName(serviceName), StringComparison.InvariantCultureIgnoreCase)).ToList();
        }

        public async Task<IList<PartitionEvent>> GetPartitionEventsAsync(Guid partitionId, Duration duration, IList<Type> types, CancellationToken token)
        {
            var filter = FilterFactory.CreatePartitionFilter(this.traceStoreReader, types, partitionId);
            var allPartitionEvents = (await this.traceStoreReader.ReadTraceRecordsAsync(duration, filter, token).ConfigureAwait(false)).Select(oneRecord => PartitionEventAdapter.Convert(oneRecord)).ToList();

            if (!this.traceStoreReader.IsPropertyLevelFilteringSupported() && partitionId != Guid.Empty)
            {
                allPartitionEvents = allPartitionEvents.Where(oneEvent => oneEvent.PartitionId == partitionId).ToList();
            }

            // Today, only partition Events have correlation feature available, so we call decorate only for them. Subject to change in future.
            await this.DecorateCorrelationAttributeAsync(duration, allPartitionEvents.Cast<FabricEvent>().ToList(), token);

            return allPartitionEvents;
        }

        public async Task<IList<ReplicaEvent>> GetReplicaEventsAsync(Guid partitionId, long? replicaId, Duration duration, IList<Type> types, CancellationToken token)
        {
            var filter = FilterFactory.CreateReplicaFilter(this.traceStoreReader, types, partitionId, replicaId);
            var allReplicaEvents = (await this.traceStoreReader.ReadTraceRecordsAsync(duration, filter, token).ConfigureAwait(false)).Select(oneRecord => ReplicaEventAdapter.Convert(oneRecord)).ToList();

            if (this.traceStoreReader.IsPropertyLevelFilteringSupported() || partitionId == Guid.Empty)
            {
                return allReplicaEvents;
            }

            return allReplicaEvents.Where(oneEvent => oneEvent.PartitionId == partitionId && (!replicaId.HasValue || oneEvent.ReplicaId == replicaId)).ToList();
        }

        public async Task<IList<ClusterEvent>> GetClusterEventsAsync(Duration duration, IList<Type> types, CancellationToken token)
        {
            var filter = FilterFactory.CreateClusterFilter(types);
            var allTraceRecords = await this.traceStoreReader.ReadTraceRecordsAsync(duration, filter, token).ConfigureAwait(false);
            return allTraceRecords.Select(oneRecord => ClusterEventAdapter.Convert(oneRecord)).ToList();
        }

        public async Task<IList<FabricEvent>> GetCorrelatedEvents(Guid eventInstanceId, Duration duration, CancellationToken token)
        {
            // First Fetch all Correlated Trace Records.
            var allCorrelationTraces = await this.GetCorrelationTraceRecordsAsync(duration, eventInstanceId, token).ConfigureAwait(false);

            IList<FabricEvent> correlationEvent = new List<FabricEvent>();
            foreach (var oneCorrelationTrace in allCorrelationTraces)
            {
                FabricEvent oneCorrelatedEvent = null;

                // Today, only Partition Events have correlation, so we only grab those. In future, this would be refactored.
                if (oneCorrelationTrace.RelatedFromId == eventInstanceId)
                {
                    oneCorrelatedEvent = await this.GetPartitionEventsAsync(duration, oneCorrelationTrace.RelatedToId, token).ConfigureAwait(false);
                }
                else
                {
                    oneCorrelatedEvent = await this.GetPartitionEventsAsync(duration, oneCorrelationTrace.RelatedFromId, token).ConfigureAwait(false);
                }

                if (oneCorrelatedEvent != null)
                {
                    oneCorrelatedEvent.HasCorrelatedEvents = true;
                    correlationEvent.Add(oneCorrelatedEvent);
                }
            }

            return correlationEvent;
        }

        private async Task<FabricEvent> GetPartitionEventsAsync(Duration duration, Guid instanceId, CancellationToken token)
        {
            var records = await this.traceStoreReader.ReadTraceRecordsAsync(
                duration,
                ReadFilter.CreateReadFilter(Mapping.EntityToEventsMap[EntityType.Partition].Select(item => item.UnderlyingType).ToList()),
                token).ConfigureAwait(false);

            var recordOfInterest = records.SingleOrDefault(item => item.ObjectInstanceId?.Id == instanceId);
            if (recordOfInterest != null)
            {
                return PartitionEventAdapter.Convert(recordOfInterest);
            }

            return null;
        }

        private async Task DecorateCorrelationAttributeAsync(Duration duration, IList<FabricEvent> partitionEvents, CancellationToken token)
        {
            var newTimeDuration = new Duration(
                duration.StartTime,
                DateTime.UtcNow - duration.EndTime > MaxDurationForCorrelatingEvent ? duration.EndTime + MaxDurationForCorrelatingEvent : DateTime.UtcNow);

            var filter = ReadFilter.CreateReadFilter(typeof(CorrelationTraceRecord));
            var allCorrelationTraces = (await this.traceStoreReader.ReadTraceRecordsAsync(newTimeDuration, filter, token).ConfigureAwait(false));

            foreach (var oneCorrelationTrace in allCorrelationTraces)
            {
                var castedRecord = (CorrelationTraceRecord)oneCorrelationTrace;
                var matchingEvents = partitionEvents.Where(item => item.EventInstanceId == castedRecord.RelatedFromId || item.EventInstanceId == castedRecord.RelatedToId);
                if (matchingEvents.Any())
                {
                    foreach (var oneEvent in matchingEvents)
                    {
                        oneEvent.HasCorrelatedEvents = true;
                    }
                }
            }
        }

        private async Task<IList<CorrelationTraceRecord>> GetCorrelationTraceRecordsAsync(Duration duration, Guid eventInstanceId, CancellationToken token)
        {
            var filter = FilterFactory.CreateCorrelationEventFilter(this.traceStoreReader, new List<Guid> { eventInstanceId });
            var allCorrelationTraces = (await this.traceStoreReader.ReadTraceRecordsAsync(
                duration,
                filter,
                token).ConfigureAwait(false));

            if (!this.traceStoreReader.IsPropertyLevelFilteringSupported())
            {
                allCorrelationTraces = allCorrelationTraces.Where(item =>
                {
                    var castedObj = (CorrelationTraceRecord)item;
                    return castedObj.RelatedFromId == eventInstanceId || castedObj.RelatedToId == eventInstanceId;
                });
            }

            return allCorrelationTraces.Cast<CorrelationTraceRecord>().ToList();
        }
    }
}