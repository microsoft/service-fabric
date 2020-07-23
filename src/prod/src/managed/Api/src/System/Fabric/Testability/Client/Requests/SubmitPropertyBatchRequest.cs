// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Collections.ObjectModel;
    using System.Fabric.Interop;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    public class SubmitPropertyBatchRequest : FabricRequest
    {
        public SubmitPropertyBatchRequest(IFabricClient fabricClient, Uri name, Collection<PropertyBatchOperation> operations, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(name, "name");
            ThrowIf.Null(operations, "operations");

            this.Name = name;
            this.Operations = operations;
            this.ConfigureErrorCodes();
        }

        public Uri Name
        {
            get;
            private set;
        }

        public Collection<PropertyBatchOperation> Operations
        {
            get;
            private set;
        }

        public override string ToString()
        {
            var builder = new StringBuilder();
            builder.AppendFormat(CultureInfo.InvariantCulture, "Put property batch at name {0} with operations ", this.Name);

            foreach (var operation in this.Operations)
            {
                builder.AppendFormat(CultureInfo.InvariantCulture, "{{Name: {0} OperationKind: {1}}})", operation.PropertyName, operation.Kind);
            }

            builder.AppendFormat(CultureInfo.InvariantCulture, " with timeout {0}", this.Timeout);

            return builder.ToString();
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.SubmitPropertyBatchAsync(this.Name, this.Operations, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_WRITE_CONFLICT);
        }
    }
}


#pragma warning restore 1591