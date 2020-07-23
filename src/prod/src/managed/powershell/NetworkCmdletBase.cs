// ----------------------------------------------------------------------
//  <copyright file="NetworkCmdletBase.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Strings;

    public abstract class NetworkCmdletBase : CommonCmdletBase
    {
        protected void NewNetwork(string networkName, string networkAddressPrefix)
        {
            LocalNetworkDescription localnetworkDescription = new LocalNetworkDescription(new LocalNetworkConfigurationDescription(networkAddressPrefix));
            var cluster = this.GetClusterConnection();
            try
            {
                this.WriteObject(StringResources.Info_CreatingServiceFabricNetwork);
                cluster.FabricClient.NetworkManager.CreateNetworkAsync(
                networkName,
                localnetworkDescription,
                this.GetTimeout(),
                this.GetCancellationToken()).Wait();
                this.WriteObject(StringResources.Info_CreateServiceFabricNetworkSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetNetworkErrorId,
                        null);
                    return true;
                });
            }
            catch (Exception exception)
            {
                this.WriteObject(StringResources.Error_CreateServiceFabricNetworkFailed);
                this.WriteObject("changes made");
                this.WriteObject(exception.ToString());
                this.ThrowTerminatingError(
                    exception,
                    Constants.NewNetworkConfigurationErrorId,
                    null);
            }
        }
    }
}
