// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Operations
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.Common;
    using EventsStore.EventsModels;
    using EventStore.Service.DataReader;

    internal class GetCorrelatedEventsOperation : BaseOperation<IList<FabricEvent>>
    {
        // Not the ideal solution, but good enough for the 1 box scenario - in this case, we only go back max of 3 days to find the event.
        private static TimeSpan MaxDurationForCorrelationLookup = TimeSpan.FromDays(3);

        private Guid eventInstanceId;

        internal GetCorrelatedEventsOperation(string apiVersion, EventStoreRuntime currentRuntime, Guid eventInstanceId)
            : base(apiVersion, currentRuntime)
        {
            this.eventInstanceId = eventInstanceId;
        }

        public override EntityType GetSupportedEntityType()
        {
            return EntityType.Correlation;
        }

        protected override Task<IList<FabricEvent>> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            var duration = new Duration(DateTime.UtcNow - MaxDurationForCorrelationLookup, DateTime.UtcNow);

            return this.CurrentRuntime.EventStoreReader.GetCorrelatedEvents(
                this.eventInstanceId,
                duration,
                cancellationToken);
        }
    }
}