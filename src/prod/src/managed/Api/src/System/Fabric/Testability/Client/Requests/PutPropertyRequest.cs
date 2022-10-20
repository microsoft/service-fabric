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
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class PutPropertyRequest<T> : FabricRequest
    {
        public PutPropertyRequest(IFabricClient fabricClient, Uri name, string propertyName, T data, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(name, "name");
            ThrowIf.Null(propertyName, "propertyName");

            this.Name = name;
            this.PropertyName = propertyName;
            this.Data = data;
            this.ConfigureErrorCodes();
        }

        public Uri Name
        {
            get;
            private set;
        }

        public string PropertyName
        {
            get;
            private set;
        }

        public T Data
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Put property at name {0} with property name {1} with data {2} with timeout {3}", this.Name, this.PropertyName, this.Data, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.PutPropertyAsync(this.Name, this.PropertyName, this.Data, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_WRITE_CONFLICT);
        }
    }
}


#pragma warning restore 1591