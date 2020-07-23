// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public abstract class GetEventsStoreEventsRequest : FabricRequest
    {
        public GetEventsStoreEventsRequest(
            IFabricClient fabricClient,
            DateTime startTimeUtc,
            DateTime endTimeUtc,
            IList<string> eventsTypesFilter,
            bool excludeAnalysisEvents,
            bool skipCorrelationLookup,
            TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.StartTimeUtc = startTimeUtc;
            this.EndTimeUtc = endTimeUtc;
            this.EventsTypesFilter = eventsTypesFilter;
            this.ExcludeAnalysisEvents = excludeAnalysisEvents;
            this.SkipCorrelationLookup = skipCorrelationLookup;

            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RECONFIGURATION_PENDING);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_NOT_FOUND);
        }

        public DateTime StartTimeUtc { get; private set; }

        public DateTime EndTimeUtc { get; private set; }

        public IList<string> EventsTypesFilter { get; private set; }

        public bool ExcludeAnalysisEvents { get; private set; }

        public bool SkipCorrelationLookup { get; private set; }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "{0} with Timeout={1}, StartTimeUtc={2}, EndTimeUtc={3}, EventsTypesFilter={4}, ExcludeAnalysisEvents={5}, SkipCorrelationLookup={6}",
                this.GetType().Name,
                this.Timeout,
                this.StartTimeUtc,
                this.EndTimeUtc,
                string.Join(",", this.EventsTypesFilter),
                this.ExcludeAnalysisEvents,
                this.SkipCorrelationLookup);
        }
    }
}

#pragma warning restore 1591