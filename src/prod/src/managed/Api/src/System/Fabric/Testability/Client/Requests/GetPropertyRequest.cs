// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Testability.Common;

    public class GetPropertyRequest : FabricRequest
    {
        public GetPropertyRequest(IFabricClient fabricClient, Uri name, string propertyName, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(name, "name");
            ThrowIf.NullOrEmpty(propertyName, "propertyName");

            this.Name = name;
            this.PropertyName = propertyName;
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
            return string.Format(CultureInfo.InvariantCulture, "Get property at name {0} with property name {1} with timeout {2}", this.Name, this.PropertyName, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetPropertyAsync(this.Name, this.PropertyName, this.Timeout, cancellationToken);
        }

        public static object GetNamedPropertyValue(NamedProperty property)
        {
            ThrowIf.Null(property, "property");
            object propertyValue = null;

            // Special case GetValue calls (See Bug: 860194 for details)...
            FabricErrorCode errorCode = (FabricErrorCode)FabricClientState.HandleException(() => propertyValue = property.GetValue<object>());
            if (errorCode != 0 && errorCode != FabricErrorCode.PropertyValueEmpty)
            {
                throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "GetProperty failed with an unexpected error code {0}", errorCode));
            }

            return propertyValue;
        }
    }
}


#pragma warning restore 1591