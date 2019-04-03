// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Util;
    using System.Fabric.Common.Tracing;

    internal class QueryStatTracker
    {
        private string entityType;
        private bool isIdentifierPresent;
        private string eventTypeFilters;
        private QueryParametersWrapper queryParamWrapper;
        private DateTime startTime;

        private QueryStatTracker(string typeEntity, string identifier, QueryParametersWrapper queryParams)
        {
            Assert.IsNotNull(typeEntity, "typeEntity != null");
            Assert.IsNotNull(queryParams, "queryParams != null");

            this.entityType = typeEntity;
            this.queryParamWrapper = queryParams;

            this.isIdentifierPresent = false;
            if (!string.IsNullOrEmpty(identifier))
            {
                this.isIdentifierPresent = true;
            }

            if (queryParams.EventsTypesFilter == null || !queryParams.EventsTypesFilter.Any())
            {
                this.eventTypeFilters = "Null";
            }
            else
            {
                this.eventTypeFilters = string.Join(",", queryParams.EventsTypesFilter);
            }

            this.startTime = DateTime.UtcNow;
        }

        // Mark Query Start
        public static QueryStatTracker CreateAndMarkQueryStart(string typeEntity, string identifier, QueryParametersWrapper queryParams)
        {
            return new QueryStatTracker(typeEntity, identifier, queryParams);
        }

        // Mark Query end.
        public void MarkQueryEnd(int itemCount)
        {
            // If no date param specified, we use Current time for both of them.
            FabricEvents.Events.EventStoreQueryStat(
                this.entityType,
                this.queryParamWrapper.APIVersion,
                this.queryParamWrapper.StartTimeUtc == DateTime.MinValue ? DateTime.UtcNow : this.queryParamWrapper.StartTimeUtc, // this is to avoid failure in ToFileTimeUtc call.
                this.queryParamWrapper.EndTimeUtc == DateTime.MaxValue ? DateTime.UtcNow : this.queryParamWrapper.EndTimeUtc,
                this.isIdentifierPresent,
                this.eventTypeFilters,
                this.queryParamWrapper.SkipCorrelationLookup,
                (DateTime.UtcNow - this.startTime).TotalMilliseconds,
                itemCount);
        }
    }
}
