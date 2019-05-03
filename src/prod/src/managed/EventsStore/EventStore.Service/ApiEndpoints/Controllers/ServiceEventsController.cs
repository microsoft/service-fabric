// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Controllers
{
    using EventsStore.EventsModels.Service;
    using EventStore.Service.ApiEndpoints.Common;
    using EventStore.Service.ApiEndpoints.Operations;
    using EventStore.Service.ApiEndpoints.Server;
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using System.Web.Http;

    [RoutePrefix("EventsStore/Services")]
    public class ServiceEventsController : BaseController
    {
        public ServiceEventsController(ControllerSetting controllerSetting) : base(controllerSetting)
        {
        }

        [Route("Events")]
        [HttpGet]
        public async Task<IList<ServiceEvent>> GetServicesEvents(
            [FromUri(Name = "starttimeutc")] string startTimeUtc,
            [FromUri(Name = "endtimeutc")] string endTimeUtc,
            [FromUri(Name = "api-version")] string apiVersion = "6.2-preview",
            [FromUri(Name = "eventstypesfilter")] string eventsTypesFilter = null,
            [FromUri(Name = "excludeanalysisevents")] bool excludeAnalysisEvents = false,
            [FromUri(Name = "skipcorrelationlookup")] bool skipCorrelationLookup = false)
        {
            return await this.RunAsync(new GetServiceEventsOperation(apiVersion, this.CurrentRuntime, null, startTimeUtc, endTimeUtc, eventsTypesFilter));
        }

        [Route("{serviceId}/$/Events")]
        [HttpGet]
        public async Task<IList<ServiceEvent>> GetNodeEvents(
            string serviceId,
            [FromUri(Name = "starttimeutc")] string startTimeUtc,
            [FromUri(Name = "endtimeutc")] string endTimeUtc,
            [FromUri(Name = "api-version")] string apiVersion = "6.2-preview",
            [FromUri(Name = "eventstypesfilter")] string eventsTypesFilter = null,
            [FromUri(Name = "excludeanalysisevents")] bool excludeAnalysisEvents = false,
            [FromUri(Name = "skipcorrelationlookup")] bool skipCorrelationLookup = false)
        {
            serviceId = ConvertServiceIdToFullUri(serviceId);

            return await this.RunAsync(new GetServiceEventsOperation(apiVersion, this.CurrentRuntime, serviceId, startTimeUtc, endTimeUtc, eventsTypesFilter));
        }

        private static string ConvertServiceIdToFullUri(string serviceId)
        {
            if (serviceId.StartsWith("system~", StringComparison.InvariantCultureIgnoreCase) ||
                serviceId.StartsWith("system/", StringComparison.InvariantCultureIgnoreCase))
            {
                return serviceId.Substring(7);
            }

            return UriConverters.ConvertApiIdToFullUri(serviceId);
        }
    }
}