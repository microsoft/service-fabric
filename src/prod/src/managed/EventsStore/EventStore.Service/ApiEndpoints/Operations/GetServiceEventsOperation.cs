// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Operations
{
    using EventsStore.EventsModels.Service;
    using EventStore.Service.DataReader;
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    internal class GetServiceEventsOperation : BaseOperation<IList<ServiceEvent>>
    {
        private string serviceName;

        internal GetServiceEventsOperation(string apiVersion, EventStoreRuntime currentRuntime, string serviceId, string startTimeUtc, string endTimeUtc, string eventsTypesFilter)
            : base(apiVersion, currentRuntime, eventsTypesFilter)
        {
            this.ValidateAndExtractStartAndEndTime(startTimeUtc, endTimeUtc);
            this.serviceName = serviceId;
        }

        public override EntityType GetSupportedEntityType()
        {
            return EntityType.Service;
        }

        protected override Task<IList<ServiceEvent>> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.CurrentRuntime.EventStoreReader.GetServiceEventsAsync(
                this.serviceName,
                new ClusterAnalysis.Common.Duration(this.QueryStartTime.Value, this.QueryEndTime.Value),
                this.QueryTypeFilter,
                cancellationToken);
        }
    }
}