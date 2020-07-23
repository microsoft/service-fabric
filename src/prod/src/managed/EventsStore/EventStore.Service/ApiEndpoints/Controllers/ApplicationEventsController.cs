// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Controllers
{
    using EventsStore.EventsModels.Application;
    using EventStore.Service.ApiEndpoints.Common;
    using EventStore.Service.ApiEndpoints.Operations;
    using EventStore.Service.ApiEndpoints.Server;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using System.Web.Http;

    [RoutePrefix("EventsStore/Applications")]
    public class ApplicationEventsController : BaseController
    {
        public ApplicationEventsController(ControllerSetting controllerSetting) : base(controllerSetting)
        {
        }

        [Route("Events")]
        [HttpGet]
        public async Task<IList<ApplicationEvent>> GetApplicationsEvents(
            [FromUri(Name = "starttimeutc")] string startTimeUtc,
            [FromUri(Name = "endtimeutc")] string endTimeUtc,
            [FromUri(Name = "api-version")] string apiVersion = "6.2-preview",
            [FromUri(Name = "eventstypesfilter")] string eventsTypesFilter = null,
            [FromUri(Name = "excludeanalysisevents")] bool excludeAnalysisEvents = false,
            [FromUri(Name = "skipcorrelationlookup")] bool skipCorrelationLookup = false)
        {
            return await this.RunAsync(new GetApplicationEventsOperation(apiVersion, this.CurrentRuntime, null, startTimeUtc, endTimeUtc, eventsTypesFilter));
        }

        [Route("{applicationId}/$/Events")]
        [HttpGet]
        public async Task<IList<ApplicationEvent>> GetApplicationEvents(
            string applicationId,
            [FromUri(Name = "starttimeutc")] string startTimeUtc,
            [FromUri(Name = "endtimeutc")] string endTimeUtc,
            [FromUri(Name = "api-version")] string apiVersion = "6.2-preview",
            [FromUri(Name = "eventstypesfilter")] string eventsTypesFilter = null,
            [FromUri(Name = "excludeanalysisevents")] bool excludeAnalysisEvents = false,
            [FromUri(Name = "skipcorrelationlookup")] bool skipCorrelationLookup = false)
        {
            applicationId = UriConverters.ConvertApiIdToFullUri(applicationId);

            return await this.RunAsync(new GetApplicationEventsOperation(apiVersion, this.CurrentRuntime, applicationId, startTimeUtc, endTimeUtc, eventsTypesFilter));
        }
    }
}