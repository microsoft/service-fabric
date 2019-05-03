// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Operations
{
    using EventsStore.EventsModels.Cluster;
    using EventStore.Service.DataReader;
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    internal class GetClusterEventsOperation : BaseOperation<IList<ClusterEvent>>
    {
        internal GetClusterEventsOperation(string apiVersion, EventStoreRuntime currentRuntime, string startTimeUtc, string endTimeUtc, string eventsTypesFilter)
            : base(apiVersion, currentRuntime, eventsTypesFilter)
        {
            this.ValidateAndExtractStartAndEndTime(startTimeUtc, endTimeUtc);
        }

        public override EntityType GetSupportedEntityType()
        {
            return EntityType.Cluster;
        }

        protected override Task<IList<ClusterEvent>> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.CurrentRuntime.EventStoreReader.GetClusterEventsAsync(
                new ClusterAnalysis.Common.Duration(this.QueryStartTime.Value, this.QueryEndTime.Value),
                this.QueryTypeFilter,
                cancellationToken);
        }
    }
}