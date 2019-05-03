// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Controllers
{
    using EventsStore.EventsModels.Partition;
    using EventsStore.EventsModels.Replica;
    using EventStore.Service.ApiEndpoints.Operations;
    using EventStore.Service.ApiEndpoints.Server;
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using System.Web.Http;

    [RoutePrefix("EventsStore/Partitions")]
    public class PartitionEventsController : BaseController
    {
        public PartitionEventsController(ControllerSetting controllerSetting) : base(controllerSetting)
        {
        }

        [Route("Events")]
        [HttpGet]
        public async Task<IList<PartitionEvent>> GetPartitionsEvents(
            [FromUri(Name = "starttimeutc")] string startTimeUtc,
            [FromUri(Name = "endtimeutc")] string endTimeUtc,
            [FromUri(Name = "api-version")] string apiVersion = "6.2-preview",
            [FromUri(Name = "eventstypesfilter")] string eventsTypesFilter = null,
            [FromUri(Name = "excludeanalysisevents")] bool excludeAnalysisEvents = false,
            [FromUri(Name = "skipcorrelationlookup")] bool skipCorrelationLookup = false)
        {
            return await this.RunAsync(new GetPartitionEventsOperation(apiVersion, this.CurrentRuntime, Guid.Empty, startTimeUtc, endTimeUtc, eventsTypesFilter));
        }

        [Route("{partitionId}/$/Events")]
        [HttpGet]
        public async Task<IList<PartitionEvent>> GetPartitionEvents(
            Guid partitionId,
            [FromUri(Name = "starttimeutc")] string startTimeUtc,
            [FromUri(Name = "endtimeutc")] string endTimeUtc,
            [FromUri(Name = "api-version")] string apiVersion = "6.2-preview",
            [FromUri(Name = "eventstypesfilter")] string eventsTypesFilter = null,
            [FromUri(Name = "excludeanalysisevents")] bool excludeAnalysisEvents = false,
            [FromUri(Name = "skipcorrelationlookup")] bool skipCorrelationLookup = false)
        {
            return await this.RunAsync(new GetPartitionEventsOperation(apiVersion, this.CurrentRuntime, partitionId, startTimeUtc, endTimeUtc, eventsTypesFilter));
        }

        [Route("{partitionId}/$/Replicas/Events")]
        [HttpGet]
        public async Task<IList<ReplicaEvent>> GetPartitionReplicasEvents(
            Guid partitionId,
            [FromUri(Name = "starttimeutc")] string startTimeUtc,
            [FromUri(Name = "endtimeutc")] string endTimeUtc,
            [FromUri(Name = "api-version")] string apiVersion = "6.2-preview",
            [FromUri(Name = "eventstypesfilter")] string eventsTypesFilter = null,
            [FromUri(Name = "excludeanalysisevents")] bool excludeAnalysisEvents = false,
            [FromUri(Name = "skipcorrelationlookup")] bool skipCorrelationLookup = false)
        {
            return await this.RunAsync(new GetReplicaEventsOperation(apiVersion, this.CurrentRuntime, partitionId, null, startTimeUtc, endTimeUtc, eventsTypesFilter));
        }

        [Route("{partitionId}/$/Replicas/{replicaId}/$/Events")]
        [HttpGet]
        public async Task<IList<ReplicaEvent>> GetPartitionReplicaEvents(
            Guid partitionId,
            string replicaId,
            [FromUri(Name = "starttimeutc")] string startTimeUtc,
            [FromUri(Name = "endtimeutc")] string endTimeUtc,
            [FromUri(Name = "api-version")] string apiVersion = "6.2-preview",
            [FromUri(Name = "eventstypesfilter")] string eventsTypesFilter = null,
            [FromUri(Name = "excludeanalysisevents")] bool excludeAnalysisEvents = false,
            [FromUri(Name = "skipcorrelationlookup")] bool skipCorrelationLookup = false)
        {
            return await this.RunAsync(new GetReplicaEventsOperation(apiVersion, this.CurrentRuntime, partitionId, replicaId, startTimeUtc, endTimeUtc, eventsTypesFilter));
        }
    }
}