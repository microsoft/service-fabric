// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;
    using System.Threading.Tasks;

    [Cmdlet("Invoke", "ServiceFabricInfrastructureQuery")]
    public sealed class InvokeInfrastructureQuery : InvokeInfrastructureCmdletBase
    {
        internal override string ErrorId
        {
            get
            {
                return Constants.InvokeInfrastructureQueryErrorId;
            }
        }

        internal override Task<string> InvokeInfrastructureAsync(IClusterConnection clusterConnection)
        {
            return clusterConnection.InvokeInfrastructureQueryAsync(
                       this.ServiceName,
                       this.Command,
                       this.GetTimeout(),
                       this.GetCancellationToken());
        }       
    }
}