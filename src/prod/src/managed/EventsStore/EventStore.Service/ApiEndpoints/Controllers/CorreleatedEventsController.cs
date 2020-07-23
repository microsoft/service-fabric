// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Controllers
{
    using EventsStore.EventsModels;
    using EventStore.Service.ApiEndpoints.Operations;
    using EventStore.Service.ApiEndpoints.Server;
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using System.Web.Http;

    [RoutePrefix("EventsStore/CorrelatedEvents")]
    public class CorrelatedEventsController : BaseController
    {
        public CorrelatedEventsController(ControllerSetting controllerSetting) : base(controllerSetting)
        {
        }

        [Route("{eventInstanceId}/$/Events")]
        [HttpGet]
        public async Task<IList<FabricEvent>> GetCorrelatedEvents(
            Guid eventInstanceId,
            [FromUri(Name = "api-version")] string apiVersion = "6.2-preview")
        {
            return await this.RunAsync(new GetCorrelatedEventsOperation(apiVersion, this.CurrentRuntime, eventInstanceId));
        }
    }
}