// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Controllers
{
    using EventsStore.EventsModels.Cluster;
    using EventStore.Service.ApiEndpoints.Operations;
    using EventStore.Service.ApiEndpoints.Server;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using System.Web.Http;

    [RoutePrefix("EventsStore/Cluster")]
    public class ClusterEventsController : BaseController
    {
        public ClusterEventsController(ControllerSetting controllerSetting) : base(controllerSetting)
        {
        }

        [Route("Events")]
        [HttpGet]
        public async Task<IList<ClusterEvent>> GetClusterEvents(
            [FromUri(Name = "starttimeutc")] string startTimeUtc,
            [FromUri(Name = "endtimeutc")] string endTimeUtc,
            [FromUri(Name = "api-version")] string apiVersion = "6.2-preview",
            [FromUri(Name = "eventstypesfilter")] string eventsTypesFilter = null,
            [FromUri(Name = "excludeanalysisevents")] bool excludeAnalysisEvents = false,
            [FromUri(Name = "skipcorrelationlookup")] bool skipCorrelationLookup = false)
        {
            return await this.RunAsync(new GetClusterEventsOperation(apiVersion, this.CurrentRuntime, startTimeUtc, endTimeUtc, eventsTypesFilter));
        }
    }
}