// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    internal sealed class GetChaosReportDescription
    {
        public GetChaosReportDescription(
            ChaosReportFilter filter,
            string continuationToken)
        {
            this.Filter = filter;
            this.ContinuationToken = continuationToken;
        }

        public ChaosReportFilter Filter
        {
            get;
            private set;
        }

        public string ContinuationToken
        {
            get;
            internal set;
        }

        internal string ClientType { get; set; }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Filter={0}, ContinuationToken: {1}, ClientType: {2}",
                this.Filter,
                this.ContinuationToken,
                this.ClientType);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeGetChaosReportDescription = new NativeTypes.FABRIC_GET_CHAOS_REPORT_DESCRIPTION();

            nativeGetChaosReportDescription.Filter = this.Filter.ToNative(pinCollection);
            nativeGetChaosReportDescription.ContinuationToken = pinCollection.AddObject(this.ContinuationToken);
            var clientType = new NativeTypes.FABRIC_CHAOS_CLIENT_TYPE
            {
                ClientType = pinCollection.AddObject(ChaosConstants.ManagedClientTypeName)
            };

            nativeGetChaosReportDescription.Reserved = pinCollection.AddBlittable(clientType);

            return pinCollection.AddBlittable(nativeGetChaosReportDescription);
        }

        internal static unsafe GetChaosReportDescription CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_GET_CHAOS_REPORT_DESCRIPTION native = *(NativeTypes.FABRIC_GET_CHAOS_REPORT_DESCRIPTION*)nativeRaw;

            var filter = ChaosReportFilter.FromNative(native.Filter);
            string continuationToken = NativeTypes.FromNativeString(native.ContinuationToken);

            var description = new GetChaosReportDescription(filter, continuationToken);

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
