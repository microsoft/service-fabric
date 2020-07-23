// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Operations
{
    using EventsStore.EventsModels.Partition;
    using EventStore.Service.DataReader;
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    internal class GetPartitionEventsOperation : BaseOperation<IList<PartitionEvent>>
    {
        private Guid partitionGuid;

        internal GetPartitionEventsOperation(string apiVersion, EventStoreRuntime currentRuntime, Guid partitionId, string startTimeUtc, string endTimeUtc, string eventsTypesFilter)
            : base(apiVersion, currentRuntime, eventsTypesFilter)
        {
            this.ValidateAndExtractStartAndEndTime(startTimeUtc, endTimeUtc);
            this.partitionGuid = partitionId;
        }

        public override EntityType GetSupportedEntityType()
        {
            return EntityType.Partition;
        }

        protected override Task<IList<PartitionEvent>> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.CurrentRuntime.EventStoreReader.GetPartitionEventsAsync(
                this.partitionGuid,
                new ClusterAnalysis.Common.Duration(this.QueryStartTime.Value, this.QueryEndTime.Value),
                this.QueryTypeFilter,
                cancellationToken);
        }
    }
}