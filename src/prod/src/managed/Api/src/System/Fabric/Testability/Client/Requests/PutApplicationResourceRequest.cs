// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using Newtonsoft.Json;

    public class PutApplicationResourceRequest : FabricRequest
    {
        public PutApplicationResourceRequest(IFabricClient fabricClient, string applicationName, string descriptionJson, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(applicationName, "applicationName");
            ThrowIf.Null(descriptionJson, "descriptionJson");

            // Validate JSON format
            JsonConvert.DeserializeObject(descriptionJson);

            this.ApplicationName = applicationName;
            this.Description = descriptionJson;
            this.ConfigureErrorCodes();
        }

        public string ApplicationName
        {
            get;
            private set;
        }

        public string Description
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Put application resource {0} with timeout {1}",
                this.ApplicationName,
                this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.PutApplicationResourceAsync(this.ApplicationName, this.Description, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SINGLE_INSTANCE_APPLICATION_ALREADY_EXISTS);
        }
    }
}

#pragma warning restore 1591