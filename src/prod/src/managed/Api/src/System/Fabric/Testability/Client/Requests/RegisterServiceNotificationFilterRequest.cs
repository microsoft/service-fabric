// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class RegisterServiceNotificationFilterRequest : FabricRequest
    {
        public RegisterServiceNotificationFilterRequest(IFabricClient fabricClient, ServiceNotificationFilterDescription filterDescription, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(filterDescription, "filterDescription");

            this.FilterDescription = filterDescription;
        }

        public ServiceNotificationFilterDescription FilterDescription
        {
            get;
            private set;
        }

        public override string ToString()
        {
            var conditionStrings = new List<string>();
            if (this.FilterDescription.MatchNamePrefix)
            {
                conditionStrings.Add("PrefixMatchEnabled");
            }
            if (this.FilterDescription.MatchPrimaryChangeOnly)
            {
                conditionStrings.Add("PrimaryChangeOnly");
            }
            var conditions = String.Join(",", conditionStrings);
            var conditionsToPrint = String.IsNullOrEmpty(conditions) ? String.Empty : String.Concat("(", conditions, ")");

            return string.Format(
                CultureInfo.InvariantCulture, 
                "Register service notification filter for {0}{1} with timeout {2}", 
                this.FilterDescription.Name, 
                conditionsToPrint, 
                this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.RegisterServiceNotificationFilterAsync(this.FilterDescription, this.Timeout, cancellationToken);
        }
    }
}