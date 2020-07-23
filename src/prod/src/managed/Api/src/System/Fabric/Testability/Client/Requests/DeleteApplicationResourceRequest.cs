// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Description;

    public class DeleteApplicationResourceRequest : FabricRequest
    {
        public DeleteApplicationResourceRequest(IFabricClient fabricClient, string deploymentName, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.DeploymentName = deploymentName;
        }

        public string DeploymentName
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture,
                "DeleteApplicationResourceRequest with deploymentName {0} timeout {1}",
                this.DeploymentName,
                this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.DeleteApplicationResourceAsync(this.DeploymentName, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SINGLE_INSTANCE_APPLICATION_NOT_FOUND);
        }
    }
}

#pragma warning restore 1591
