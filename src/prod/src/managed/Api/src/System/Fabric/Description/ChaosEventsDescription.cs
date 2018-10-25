//----------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    internal sealed class ChaosEventsDescription
        : PagedQueryDescriptionBase
    {
        public ChaosEventsDescription()
        {
            this.Filter = new ChaosEventsSegmentFilter();
            this.ContinuationToken = string.Empty;
            this.MaxResults = 0; // interpreted as get all results in api
            this.ClientType = string.Empty;
        }

        public ChaosEventsDescription(
            ChaosEventsSegmentFilter filter,
            string continuationToken,
            long maxResults)
        {
            this.Filter = filter;
            this.ContinuationToken = continuationToken;
            this.MaxResults = maxResults;
            this.ClientType = string.Empty;
        }

        public ChaosEventsSegmentFilter Filter
        {
            get;
            private set;
        }

        // From PagedQueryDescriptionBase
        // public string ContinuationToken
        // public long? MaxResults

        internal string ClientType
        {
            get;
            set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Filter={0}, ContinuationToken={1}, MaxResults={2} ClientType: {3}",
                this.Filter,
                this.ContinuationToken,
                this.MaxResults,
                this.ClientType);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeChaosEventsDescription = new NativeTypes.FABRIC_CHAOS_EVENTS_SEGMENT_DESCRIPTION();

            nativeChaosEventsDescription.Filter = this.Filter.ToNative(pinCollection);

            nativeChaosEventsDescription.PagingDescription = this.ToNativePagingDescription(pinCollection);

            var clientType = new NativeTypes.FABRIC_CHAOS_CLIENT_TYPE
            {
                ClientType = pinCollection.AddObject(ChaosConstants.ManagedClientTypeName)
            };

            nativeChaosEventsDescription.Reserved = pinCollection.AddBlittable(clientType);

            return pinCollection.AddBlittable(nativeChaosEventsDescription);
        }

        internal static unsafe ChaosEventsDescription CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_CHAOS_EVENTS_SEGMENT_DESCRIPTION native = *(NativeTypes.FABRIC_CHAOS_EVENTS_SEGMENT_DESCRIPTION*)nativeRaw;
            var filter = ChaosEventsSegmentFilter.FromNative(native.Filter);

            NativeTypes.FABRIC_QUERY_PAGING_DESCRIPTION nativePagingDescription = *(NativeTypes.FABRIC_QUERY_PAGING_DESCRIPTION*)native.PagingDescription;
            var continuationToken = NativeTypes.FromNativeString(nativePagingDescription.ContinuationToken);
            var maxResults = (long)nativePagingDescription.MaxResults;

            var description = new ChaosEventsDescription(filter, continuationToken, maxResults);

            if (native.Reserved != IntPtr.Zero)
            {
                var clientType = *((NativeTypes.FABRIC_CHAOS_CLIENT_TYPE*)native.Reserved);

                if (clientType.ClientType != IntPtr.Zero)
                {
                    description.ClientType = NativeTypes.FromNativeString(clientType.ClientType);
                }
            }

            return description;
        }
    }
}

