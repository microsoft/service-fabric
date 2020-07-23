// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Testability.Client.Structures;
    using System.Globalization;
    using System.Threading;
    using System.Fabric.Testability.Common;
    using System.Threading.Tasks;

    public class EnumerateSubNamesRequest : FabricRequest
    {
        public EnumerateSubNamesRequest(IFabricClient fabricClient, Uri name, WinFabricNameEnumerationResult previousResult, bool recursive, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(name, "name");

            this.Name = name;
            this.PreviousResult = previousResult;
            this.Recursive = recursive;
        }

        public Uri Name
        {
            get;
            private set;
        }

        public WinFabricNameEnumerationResult PreviousResult
        {
            get;
            private set;
        }

        public bool Recursive
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Enumerate subnames at name {0} recursive? {1} with timeout {2}", this.Name, this.Recursive, this.Timeout);
        }

        public async override Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.EnumerateSubNamesAsync(this.Name, this.PreviousResult, this.Recursive, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591