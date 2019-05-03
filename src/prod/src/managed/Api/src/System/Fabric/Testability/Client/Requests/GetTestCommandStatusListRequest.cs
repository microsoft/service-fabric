// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class GetTestCommandStatusListRequest : FabricRequest
    {
        public GetTestCommandStatusListRequest(
            IFabricClient fabricClient,
            TestCommandStateFilter stateFilter,
            TestCommandTypeFilter typeFilter,
            TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.TestCommandStateFilter = stateFilter;
            this.TestCommandTypeFilter = typeFilter;

            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RECONFIGURATION_PENDING);
        }

        public TestCommandStateFilter TestCommandStateFilter
        {
            get;
            private set;
        }

        public TestCommandTypeFilter TestCommandTypeFilter
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetTestCommandStatusListRequest with timeout={0}", this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetTestCommandStatusListAsync(this.TestCommandStateFilter, this.TestCommandTypeFilter, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591