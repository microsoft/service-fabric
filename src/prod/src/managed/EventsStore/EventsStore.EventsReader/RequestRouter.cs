// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Linq;
    using System.Threading;
    using ClusterAnalysis.Common;
    using DataReader;
    using EventsReader.Exceptions;
    using EventsReader.LogProvider;

    /// <summary>
    /// Router for EventsAPI.
    /// </summary>
    internal class RequestRouter
    {
        // This is the max number of records we return in single request.
        private const int MaxRecordsInResult = 120;

        private const int MaxDurationForCorrelationLookupInDays = 5;

        private const string EventSuffix = "Event";

        public static void RouteUri(ParsedUri uri, IResultWriter writer)
        {
            if (uri.BaseCollection == null ||
                !uri.BaseCollection.Equals(ReaderConstants.EventsStoreBaseCollection, StringComparison.OrdinalIgnoreCase) ||
                uri.PathCollections.Count < 1 ||
                uri.RequestCollectionOrMethod == null ||
                !uri.RequestCollectionOrMethod.Equals(ReaderConstants.EventsCollection, StringComparison.OrdinalIgnoreCase))
            {
                OnUnsupportedUri(writer);
                return;
            }

            var queryParamsWrapper = new QueryParametersWrapper(uri.QueryParameters);
            if (!CheckApiVersion(queryParamsWrapper.APIVersion))
            {
                OnUnsupportedUri(writer);
                return;
            }

            if (uri.PathCollections.Count == 1)
            {
                var collection = uri.PathCollections[0];
                switch (collection.Name.ToLower())
                {
                    case ReaderConstants.Cluster:
                        if (collection.HasKeySet())
                        {
                            OnUnsupportedUri(writer);
                            return;
                        }
                        HandleClusterEvents(writer, queryParamsWrapper);
                        break;

                    case ReaderConstants.Containers:
                        if (collection.HasKeySet())
                        {
                            OnUnsupportedUri(writer);
                            return;
                        }
                        HandleContainersEvents(writer, queryParamsWrapper);
                        break;

                    case ReaderConstants.Nodes:
                        HandleNodesEvents(writer, collection.GetValueOfCollectionKey(), queryParamsWrapper);
                        break;

                    case ReaderConstants.Applications:
                        HandleApplicationsEvents(writer, collection.GetValueOfCollectionKey(), queryParamsWrapper);
                        break;

                    case ReaderConstants.Services:
                        HandleServicesEvents(writer, collection.GetValueOfCollectionKey(), queryParamsWrapper);
                        break;

                    case ReaderConstants.Partitions:
                        HandlePartitionsEvents(writer, collection.GetValueOfCollectionKey(), queryParamsWrapper);
                        break;

                    case ReaderConstants.CorrelatedEvents:
                        HandleCorrelatedEvents(writer, collection.GetValueOfCollectionKey(), queryParamsWrapper);
                        break;

                    default:
                        OnUnsupportedUri(writer);
                        break;
                }
            }
            else if (uri.PathCollections.Count == 2)
            {
                var collection = uri.PathCollections[0];
                var subCollection = uri.PathCollections[1];

                if (!collection.HasKeySet())
                {
                    OnUnsupportedUri(writer);
                    return;
                }

                var composedName = string.Concat(collection.Name.ToLower(), ":", subCollection.Name.ToLower());
                switch (composedName)
                {
                    case ReaderConstants.PartitionsReplicas:
                        HandleReplicasEvents(writer, collection.GetValueOfCollectionKey(), subCollection.GetValueOfCollectionKey(), queryParamsWrapper);
                        break;

                    default:
                        OnUnsupportedUri(writer);
                        break;
                }
            }
            else
            {
                OnUnsupportedUri(writer);
            }
        }

        private static void HandleClusterEvents(IResultWriter writer, QueryParametersWrapper queryParameters)
        {
            if (!ValidateStartAndEndTime(queryParameters.StartTimeUtc, queryParameters.EndTimeUtc))
            {
                OnUnsupportedUri(writer);
                return;
            }

            var events = new EventStoreReader()
                .GetClusterEventsAsync(
                    new Duration(
                        queryParameters.StartTimeUtc,
                        queryParameters.EndTimeUtc),
                    ConvertUserEventTypesToUnderlyingTypes(
                        EntityType.Cluster,
                        queryParameters.EventsTypesFilter),
                    CancellationToken.None)
                .GetAwaiter()
                .GetResult()
                .ToArray();
            writer.Write(new JsonSerializerHelper().SerializeObject(events.Take(MaxRecordsInResult).OrderBy(item => item.TimeStamp)));
        }

        private static void HandleContainersEvents(IResultWriter writer, QueryParametersWrapper queryParameters)
        {
            if (!ValidateStartAndEndTime(queryParameters.StartTimeUtc, queryParameters.EndTimeUtc))
            {
                OnUnsupportedUri(writer);
                return;
            }

            throw new EventStoreException(ErrorCodes.EntityTypeNotSupported, "Container Type not supported currently");
        }

        private static void HandleNodesEvents(IResultWriter writer, string nodeName, QueryParametersWrapper queryParameters)
        {
            if (!ValidateStartAndEndTime(queryParameters.StartTimeUtc, queryParameters.EndTimeUtc))
            {
                OnUnsupportedUri(writer);
                return;
            }

            var queryTracker = QueryStatTracker.CreateAndMarkQueryStart(ReaderConstants.Nodes, nodeName, queryParameters);
            var events = new EventStoreReader()
                .GetNodeEventsAsync(
                    nodeName,
                    new Duration(
                        queryParameters.StartTimeUtc, 
                        queryParameters.EndTimeUtc), 
                    ConvertUserEventTypesToUnderlyingTypes(
                        EntityType.Node,
                        queryParameters.EventsTypesFilter),
                    CancellationToken.None)
                .GetAwaiter()
                .GetResult()
                .ToArray();
            queryTracker.MarkQueryEnd(events.Count());

            writer.Write(new JsonSerializerHelper().SerializeObject(events.Take(MaxRecordsInResult).OrderBy(item => item.TimeStamp)));
        }

        // format: applicationId="myapp~app1" for fabric:/myapp/app1
        private static void HandleApplicationsEvents(IResultWriter writer, string applicationId, QueryParametersWrapper queryParameters)
        {
            if (!ValidateStartAndEndTime(queryParameters.StartTimeUtc, queryParameters.EndTimeUtc))
            {
                OnUnsupportedUri(writer);
                return;
            }

            if (!string.IsNullOrWhiteSpace(applicationId))
            {
                applicationId = ConvertApplicationIdToFullUri(applicationId);
            }

            var queryTracker = QueryStatTracker.CreateAndMarkQueryStart(ReaderConstants.Applications, applicationId, queryParameters);
            var events = new EventStoreReader()
                .GetApplicationEventsAsync(
                    applicationId,
                    new Duration(
                        queryParameters.StartTimeUtc,
                        queryParameters.EndTimeUtc),
                    ConvertUserEventTypesToUnderlyingTypes(
                        EntityType.Application,
                        queryParameters.EventsTypesFilter),
                    CancellationToken.None)
                .GetAwaiter()
                .GetResult()
                .ToArray();
            queryTracker.MarkQueryEnd(events.Count());

            writer.Write(new JsonSerializerHelper().SerializeObject(events.Take(MaxRecordsInResult).OrderBy(item => item.TimeStamp)));
        }

        // format: serviceId="myapp~app1~svc1" for fabric:/myapp/app1/svc1
        private static void HandleServicesEvents(IResultWriter writer, string serviceId, QueryParametersWrapper queryParameters)
        {
            if (!ValidateStartAndEndTime(queryParameters.StartTimeUtc, queryParameters.EndTimeUtc))
            {
                OnUnsupportedUri(writer);
                return;
            }

            if (!string.IsNullOrWhiteSpace(serviceId))
            {
                serviceId = ConvertServiceIdToFullUri(serviceId);
            }

            var queryTracker = QueryStatTracker.CreateAndMarkQueryStart(ReaderConstants.Services, serviceId, queryParameters);
            var events = new EventStoreReader()
                .GetServiceEventsAsync(
                    serviceId,
                    new Duration(
                        queryParameters.StartTimeUtc, 
                        queryParameters.EndTimeUtc), 
                    ConvertUserEventTypesToUnderlyingTypes(
                        EntityType.Service,
                        queryParameters.EventsTypesFilter),
                    CancellationToken.None)
                .GetAwaiter()
                .GetResult()
                .ToArray();
            queryTracker.MarkQueryEnd(events.Count());

            writer.Write(new JsonSerializerHelper().SerializeObject(events.Take(MaxRecordsInResult).OrderBy(item => item.TimeStamp)));
        }

        private static void HandlePartitionsEvents(IResultWriter writer, string partitionIdString, QueryParametersWrapper queryParameters)
        {
            if (!ValidateStartAndEndTime(queryParameters.StartTimeUtc, queryParameters.EndTimeUtc))
            {
                OnUnsupportedUri(writer);
                return;
            }

            var partitionGuid = Guid.Empty;
            if (!string.IsNullOrWhiteSpace(partitionIdString))
            {
                if (!Guid.TryParse(partitionIdString, out partitionGuid))
                {
                    EventStoreLogger.Logger.LogWarning("HandlePartitionsEvents:: Partition ID : {0} Is not a valid Guid", partitionIdString);
                    OnUnsupportedUri(writer);
                    return;
                }
            }

            var queryTracker = QueryStatTracker.CreateAndMarkQueryStart(ReaderConstants.Partitions, partitionIdString, queryParameters);
            var events = new EventStoreReader()
                .GetPartitionEventsAsync(
                    partitionGuid,
                    new Duration(
                        queryParameters.StartTimeUtc, 
                        queryParameters.EndTimeUtc), 
                    ConvertUserEventTypesToUnderlyingTypes(
                        EntityType.Partition,
                        queryParameters.EventsTypesFilter), 
                    CancellationToken.None)
                .GetAwaiter()
                .GetResult()
                .ToArray();
            queryTracker.MarkQueryEnd(events.Count());

            writer.Write(new JsonSerializerHelper().SerializeObject(events.Take(MaxRecordsInResult).OrderBy(item => item.TimeStamp)));
        }

        // partitionId will always have a value here.
        private static void HandleReplicasEvents(IResultWriter writer, string partitionIdString, string replicaIdString, QueryParametersWrapper queryParameters)
        {
            if (!ValidateStartAndEndTime(queryParameters.StartTimeUtc, queryParameters.EndTimeUtc))
            {
                OnUnsupportedUri(writer);
                return;
            }

            var partitionGuid = Guid.Empty;
            if (!string.IsNullOrWhiteSpace(partitionIdString))
            {
                if (!Guid.TryParse(partitionIdString, out partitionGuid))
                {
                    EventStoreLogger.Logger.LogWarning("HandleReplicaEvents:: Partition ID : {0} Is not a valid Guid", partitionIdString);
                    OnUnsupportedUri(writer);
                    return;
                }
            }

            // For replica events, Partition ID needs to be valid. So, verify that we have a valid ID Now.
            if(partitionGuid == Guid.Empty)
            {
                EventStoreLogger.Logger.LogWarning("HandleReplicaEvents:: Partition Guid Not Valid", string.IsNullOrEmpty(partitionIdString) ? "Empty" : partitionIdString);
                OnUnsupportedUri(writer);
                return;
            }

            long? replicaId = null;
            if (!string.IsNullOrWhiteSpace(replicaIdString))
            {
                long id = 0;
                if (!long.TryParse(replicaIdString, out id))
                {
                    EventStoreLogger.Logger.LogWarning("HandleReplicaEvents:: ReplicaId : {0} Is Invalid", replicaIdString);
                    OnUnsupportedUri(writer);
                    return;
                }

                replicaId = id;
            }

            var queryTracker = QueryStatTracker.CreateAndMarkQueryStart(ReaderConstants.Replicas, string.Format("{0}:{1}", partitionGuid, replicaId), queryParameters);
            var events = new EventStoreReader()
                .GetReplicaEventsAsync(
                    partitionGuid,
                    replicaId,
                    new Duration(
                        queryParameters.StartTimeUtc,
                        queryParameters.EndTimeUtc), 
                    ConvertUserEventTypesToUnderlyingTypes(
                        EntityType.Replica, 
                        queryParameters.EventsTypesFilter), 
                    CancellationToken.None)
                .GetAwaiter()
                .GetResult()
                .ToArray();
            queryTracker.MarkQueryEnd(events.Count());

            writer.Write(new JsonSerializerHelper().SerializeObject(events.Take(MaxRecordsInResult).OrderBy(item => item.TimeStamp)));
        }

        private static void HandleCorrelatedEvents(IResultWriter writer, string eventInstanceId, QueryParametersWrapper queryParameters)
        {
            if(string.IsNullOrWhiteSpace(eventInstanceId))
            {
                EventStoreLogger.Logger.LogWarning("HandleCorrelatedEvents:: Event Instance id in param is empty/null");
                OnUnsupportedUri(writer);
                return;
            }

            var eventInstanceGuid = Guid.Empty;
            if (!Guid.TryParse(eventInstanceId, out eventInstanceGuid))
            {
                EventStoreLogger.Logger.LogWarning("HandleCorrelatedEvents:: EventInstance ID : {0} Is not a valid Guid", eventInstanceId);
                OnUnsupportedUri(writer);
                return;
            }

            if (eventInstanceGuid == Guid.Empty)
            {
                EventStoreLogger.Logger.LogWarning("HandleCorrelatedEvents:: Empty Guid for Event Instance Id");
                OnUnsupportedUri(writer);
                return;
            }

            var duration = new Duration(DateTime.UtcNow - TimeSpan.FromDays(MaxDurationForCorrelationLookupInDays), DateTime.UtcNow);
            var queryTracker = QueryStatTracker.CreateAndMarkQueryStart(ReaderConstants.CorrelatedEvents, eventInstanceId, queryParameters);
            var events = new EventStoreReader()
                .GetCorrelatedEvents(
                    eventInstanceGuid, 
                    duration,
                    CancellationToken.None)
                .GetAwaiter()
                .GetResult()
                .ToArray();
            queryTracker.MarkQueryEnd(events.Count());

            writer.Write(new JsonSerializerHelper().SerializeObject(events.Take(MaxRecordsInResult).OrderBy(item => item.TimeStamp)));
        }

        private static bool CheckApiVersion(string requestVersion)
        {
            // Temporary, to change in future to check >= minimum supported.
            return requestVersion == ReaderConstants.ApiVersion6dot2Preview ||
                requestVersion == ReaderConstants.ApiVersion6dot3Preview;
        }

        private static bool ValidateStartAndEndTime(DateTime startTimeUtc, DateTime endTimeUtc)
        {
            return startTimeUtc != DateTime.MinValue &&
                endTimeUtc != DateTime.MaxValue;
        }

        private static void OnUnsupportedUri(IResultWriter writer)
        {
            FabricEvents.Events.EventStoreUnsupportedUri("Unsupported URI");
            throw new EventStoreException(ErrorCodes.InvalidArgs, "Unsupported URI");
        }

        private static string ConvertApplicationIdToFullUri(string applicationId)
        {
            return ConvertApiIdToFullUri(applicationId);
        }

        private static string ConvertServiceIdToFullUri(string serviceId)
        {
            if (serviceId.StartsWith("system~", StringComparison.InvariantCultureIgnoreCase) ||
                serviceId.StartsWith("system/", StringComparison.InvariantCultureIgnoreCase))
            {
                return serviceId.Substring(7);
            }

            return ConvertApiIdToFullUri(serviceId);
        }

        private static string ConvertApiIdToFullUri(string entityId)
        {
            if (entityId.Contains("~"))
            {
                var segments = entityId.Split(
                    new[] { '~' },
                    StringSplitOptions.RemoveEmptyEntries);

                return string.Format("fabric:/{0}", string.Join("/", segments));
            }

            return string.Format("fabric:/{0}", entityId);
        }

        private static IList<Type> ConvertUserEventTypesToUnderlyingTypes(EntityType entityType, IList<string> userSuppliedFilters)
        {
            // If user doesn't specify any filter, it implies the user wants all Types for this entity.
            if (userSuppliedFilters == null || !userSuppliedFilters.Any())
            {
                return Mapping.EntityToEventsMap[entityType].Select(item => item.UnderlyingType).ToList();
            }

            var supportedTypes = Mapping.EntityToEventsMap[entityType];
            var requestedTypes = new List<Type>();
            foreach (var oneUserSuppliedFilter in userSuppliedFilters)
            {
                var matchingTypes = supportedTypes.Where(item =>
                    item.ModelType.Name.Equals(oneUserSuppliedFilter, StringComparison.InvariantCultureIgnoreCase) ||
                    item.ModelType.Name.Equals(oneUserSuppliedFilter + EventSuffix, StringComparison.InvariantCultureIgnoreCase));
                if (!matchingTypes.Any())
                {
                    throw new EventStoreException(ErrorCodes.InvalidArgs, string.Format("Type {0} Not Supported for Entity {1}", oneUserSuppliedFilter, entityType));
                }

                requestedTypes.AddRange(matchingTypes.Select(item => item.UnderlyingType));
            }

            return requestedTypes;
        }
    }
}