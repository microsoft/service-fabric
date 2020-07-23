// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Operations
{
    using EventsStore.EventsModels.Application;
    using EventStore.Service.DataReader;
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    internal class GetApplicationEventsOperation : BaseOperation<IList<ApplicationEvent>>
    {
        private string appId;

        internal GetApplicationEventsOperation(string apiVersion, EventStoreRuntime currentRuntime, string applicationId, string startTimeUtc, string endTimeUtc, string eventsTypesFilter)
            : base(apiVersion, currentRuntime, eventsTypesFilter)
        {
            this.ValidateAndExtractStartAndEndTime(startTimeUtc, endTimeUtc);
            this.appId = applicationId;
        }

        public override EntityType GetSupportedEntityType()
        {
            return EntityType.Application;
        }

        protected override Task<IList<ApplicationEvent>> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.CurrentRuntime.EventStoreReader.GetApplicationEventsAsync(
                this.appId,
                new ClusterAnalysis.Common.Duration(this.QueryStartTime.Value, this.QueryEndTime.Value),
                this.QueryTypeFilter,
                cancellationToken);
        }
    }
}