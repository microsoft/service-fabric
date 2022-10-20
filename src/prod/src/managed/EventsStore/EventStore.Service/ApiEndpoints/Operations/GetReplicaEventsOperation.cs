// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Operations
{
    using EventsStore.EventsModels.Replica;
    using EventStore.Service.DataReader;
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    internal class GetReplicaEventsOperation : BaseOperation<IList<ReplicaEvent>>
    {
        private Guid partitionGuid;

        private long? replicaId;

        internal GetReplicaEventsOperation(string apiVersion, EventStoreRuntime currentRuntime, Guid partitionId, string replicaId, string startTimeUtc, string endTimeUtc, string eventsTypesFilter)
            : base(apiVersion, currentRuntime, eventsTypesFilter)
        {
            this.ValidateAndExtractStartAndEndTime(startTimeUtc, endTimeUtc);

            if (partitionId == Guid.Empty)
            {
                throw new ArgumentException("partitionId can be empty Guid");
            }

            this.partitionGuid = partitionId;
            this.replicaId = null;

            long parsedId;
            if (long.TryParse(replicaId, out parsedId))
            {
                this.replicaId = parsedId;
            }
        }

        public override EntityType GetSupportedEntityType()
        {
            return EntityType.Replica;
        }

        protected override Task<IList<ReplicaEvent>> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.CurrentRuntime.EventStoreReader.GetReplicaEventsAsync(
                this.partitionGuid,
                this.replicaId,
                new ClusterAnalysis.Common.Duration(this.QueryStartTime.Value, this.QueryEndTime.Value),
                this.QueryTypeFilter,
                cancellationToken);
        }
    }
}