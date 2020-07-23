// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.DataReader
{
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Util;
    using ClusterAnalysis.TraceAccessLayer;
    using ClusterAnalysis.TraceAccessLayer.StoreConnection;
    using ConfigReader;
    using EventsStore.EventsModels;
    using EventsStore.EventsModels.Application;
    using EventsStore.EventsModels.Cluster;
    using EventsStore.EventsModels.Node;
    using EventsStore.EventsModels.Partition;
    using EventsStore.EventsModels.Replica;
    using EventsStore.EventsModels.Service;
    using EventStore.Service.Cache;
    using Exceptions;
    using LogProvider;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    internal class EventStoreReader
    {
        private static TimeSpan MaxDaysToMaintainCache = TimeSpan.FromDays(4);

        // One item, at max (including overhead) takes around 2000B. That implies that to store 50K items, we would need a 100 MB cache.
        private static int MaxItemsToCache = 50000;

        private static TimeSpan MaxDurationForCorrelatingEvent = TimeSpan.FromHours(1);

        private DataReaderMode dataReaderMode;

        private bool initialized;

        private IAzureTableStorageAccess azureAccessObject;

        private ITraceStoreReader traceStoreReader;

        private SemaphoreSlim singleAccess;

        // Used for console driven testing.
        internal EventStoreReader(string accountName, string accountKey, string tablePrefix, string deploymentId)
        {
            TraceStoreConnectionInformation connectionInfo = null;

            EventStoreLogger.Logger.LogMessage("Cloud Environment. Configuring Test Reader");

            var azureTableStorageAccess = new AzureTableCachedStorageAccess(
                new CachePolicy(MaxDaysToMaintainCache, MaxItemsToCache, TimeSpan.FromHours(8)),
                accountName,
                accountKey,
                true,
                tablePrefix,
                deploymentId);

            // Kick-off the Cache Update Task. This task is launched in a separate Task
            // which is monitored by the TaskRunner and therefore doesn't need to be monitored here.
            azureTableStorageAccess.KickOffUpdaterAsync(CancellationToken.None);

            connectionInfo = new AzureTableStoreStorageAccessInformation(azureTableStorageAccess, tablePrefix, deploymentId);

            var connection = new TraceStoreConnection(connectionInfo, EventStoreLogProvider.LogProvider);
            traceStoreReader = connection.EventStoreReader;
            this.initialized = true;
            this.singleAccess = new SemaphoreSlim(1);
        }

        public EventStoreReader(DataReaderMode readingMode)
        {
            this.dataReaderMode = readingMode;
            this.initialized = false;
            this.singleAccess = new SemaphoreSlim(1);
        }

        private async Task InitIfRequiredAsync(CancellationToken token)
        {
            if (this.initialized)
            {
                return;
            }

            await this.singleAccess.WaitAsync(token).ConfigureAwait(false);

            if (this.initialized)
            {
                return;
            }

            EventStoreLogger.Logger.LogMessage("Doing Reader Initialization");

            try
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
                    EventStoreLogger.Logger.LogMessage("Cloud Environment. Configuring Cloud Reader. Mode : {0}", this.dataReaderMode);
                    var operationalConsumer = ClusterSettingsReader.OperationalConsumer;
                    if (operationalConsumer == null)
                    {
                        throw new ConnectionParsingException(ErrorCodes.AzureConnectionInformationMissing, "Config is Missing Operational Store Connection information");
                    }

                    if (this.dataReaderMode == DataReaderMode.CacheMode)
                    {
                        EventStoreLogger.Logger.LogMessage("Caching is Enabled.");

                        this.azureAccessObject = new AzureTableCachedStorageAccess(
                            new CachePolicy(MaxDaysToMaintainCache, MaxItemsToCache, TimeSpan.FromHours(8)),
                            operationalConsumer.Connection.AccountName,
                            HandyUtil.ConvertToUnsecureString(operationalConsumer.Connection.AccountKey),
                            true,
                            operationalConsumer.TablesPrefix,
                            operationalConsumer.DeploymentId);

                        // Kick-off the Cache Update Task. This task is launched in a separate Task
                        // which is monitored by the TaskRunner and therefore doesn't need to be monitored here.
                        var task = ((AzureTableCachedStorageAccess)azureAccessObject).KickOffUpdaterAsync(token);
                    }
                    else
                    {
                        EventStoreLogger.Logger.LogMessage("Caching is Disabled");

                        this.azureAccessObject = new AccountAndKeyTableStorageAccess(
                            operationalConsumer.Connection.AccountName,
                            HandyUtil.ConvertToUnsecureString(operationalConsumer.Connection.AccountKey),
                            true);
                    }

                    connectionInfo = new AzureTableStoreStorageAccessInformation(azureAccessObject, operationalConsumer.TablesPrefix, operationalConsumer.DeploymentId);
                }

                var connection = new TraceStoreConnection(connectionInfo, EventStoreLogProvider.LogProvider);
                traceStoreReader = connection.EventStoreReader;
                this.initialized = true;
            }
            finally
            {
                this.singleAccess.Release();
            }
        }

        /// <summary>
        /// Switch Modes.
        /// </summary>
        /// <remarks>
        /// Reader works in 2 modes. Cached, or direct. This routine allows us to switch between these 2 modes.
        /// Switch is expected to happen when a replica become primary (switch to cached) and when it loses primary status.
        /// </remarks>
        /// <param name="newMode"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        public async Task SwitchModeAsync(DataReaderMode newMode, CancellationToken token)
        {
            if (newMode == this.dataReaderMode)
            {
                EventStoreLogger.Logger.LogMessage("Target Mode: {0} == Current Mode. No-Op", this.dataReaderMode);
                return;
            }

            EventStoreLogger.Logger.LogMessage("Current Mode: {0}, New Mode: {1}", this.dataReaderMode, newMode);

            // We do the switch under lock.
            await this.singleAccess.WaitAsync(token).ConfigureAwait(false);

            try
            {
                if (this.azureAccessObject != null)
                {
                    EventStoreLogger.Logger.LogMessage("Releasing Azure Access Object");
                    await this.azureAccessObject.ReleaseAccessAsync(token).ConfigureAwait(false);
                    this.azureAccessObject = null;
                }

                this.dataReaderMode = newMode;
                this.initialized = false;
            }
            finally
            {
                this.singleAccess.Release();
            }

            await this.InitIfRequiredAsync(token).ConfigureAwait(false);
        }

        public async Task<IList<NodeEvent>> GetNodeEventsAsync(string nodeName, Duration duration, IList<Type> types, CancellationToken token)
        {
            await this.InitIfRequiredAsync(token).ConfigureAwait(false);

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
            await this.InitIfRequiredAsync(token).ConfigureAwait(false);

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
            await this.InitIfRequiredAsync(token).ConfigureAwait(false);

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
            await this.InitIfRequiredAsync(token).ConfigureAwait(false);

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
            await this.InitIfRequiredAsync(token).ConfigureAwait(false);

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
            await this.InitIfRequiredAsync(token).ConfigureAwait(false);

            var filter = FilterFactory.CreateClusterFilter(types);
            var allTraceRecords = await this.traceStoreReader.ReadTraceRecordsAsync(duration, filter, token).ConfigureAwait(false);
            return allTraceRecords.Select(oneRecord => ClusterEventAdapter.Convert(oneRecord)).ToList();
        }

        public async Task<IList<FabricEvent>> GetCorrelatedEvents(Guid eventInstanceId, Duration duration, CancellationToken token)
        {
            await this.InitIfRequiredAsync(token).ConfigureAwait(false);

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