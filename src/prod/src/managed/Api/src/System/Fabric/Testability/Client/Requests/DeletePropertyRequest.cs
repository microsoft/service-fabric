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

    public class DeletePropertyRequest : FabricRequest
    {
        public DeletePropertyRequest(IFabricClient fabricClient, Uri name, string propertyName, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(name, "name");
            ThrowIf.Null(propertyName, "propertyName");

            this.Name = name;
            this.PropertyName = propertyName;
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

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Delete property at name {0} with property name {1} with timeout {2}", this.Name, this.PropertyName, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.DeletePropertyAsync(this.Name, this.PropertyName, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PROPERTY_DOES_NOT_EXIST);

            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_WRITE_CONFLICT);
        }
    }
}


#pragma warning restore 1591