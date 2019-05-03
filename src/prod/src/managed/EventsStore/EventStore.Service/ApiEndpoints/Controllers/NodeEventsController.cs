// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Controllers
{
    using EventsStore.EventsModels.Node;
    using EventStore.Service.ApiEndpoints.Operations;
    using EventStore.Service.ApiEndpoints.Server;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using System.Web.Http;

    [RoutePrefix("EventsStore/Nodes")]
    public class NodeEventsController : BaseController
    {
        public NodeEventsController(ControllerSetting controllerSetting) : base(controllerSetting)
        {
        }

        [Route("Events")]
        [HttpGet]
        public async Task<IList<NodeEvent>> GetNodesEvents(
            [FromUri(Name = "starttimeutc")] string startTimeUtc,
            [FromUri(Name = "endtimeutc")] string endTimeUtc,
            [FromUri(Name = "api-version")] string apiVersion = "6.2-preview",
            [FromUri(Name = "eventstypesfilter")] string eventsTypesFilter = null,
            [FromUri(Name = "excludeanalysisevents")] bool excludeAnalysisEvents = false,
            [FromUri(Name = "skipcorrelationlookup")] bool skipCorrelationLookup = false)
        {
            return await this.RunAsync(new GetNodeEventsOperation(apiVersion, this.CurrentRuntime, null, startTimeUtc, endTimeUtc, eventsTypesFilter));
        }

        [Route("{nodeName}/$/Events")]
        [HttpGet]
        public async Task<IList<NodeEvent>> GetNodeEvents(
            string nodeName,
            [FromUri(Name = "starttimeutc")] string startTimeUtc,
            [FromUri(Name = "endtimeutc")] string endTimeUtc,
            [FromUri(Name = "api-version")] string apiVersion = "6.2-preview",
            [FromUri(Name = "eventstypesfilter")] string eventsTypesFilter = null,
            [FromUri(Name = "excludeanalysisevents")] bool excludeAnalysisEvents = false,
            [FromUri(Name = "skipcorrelationlookup")] bool skipCorrelationLookup = false)
        {
            return await this.RunAsync(new GetNodeEventsOperation(apiVersion, this.CurrentRuntime, nodeName, startTimeUtc, endTimeUtc, eventsTypesFilter));
        }
    }
}