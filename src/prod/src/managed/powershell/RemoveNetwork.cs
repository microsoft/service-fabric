// ----------------------------------------------------------------------
//  <copyright file="RemoveNetwork.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Strings;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricNetwork")]
    public sealed class RemoveNetwork : CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public string NetworkName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            DeleteNetworkDescription deletenetworkDescription = new DeleteNetworkDescription(this.NetworkName);
            var cluster = this.GetClusterConnection();
            try
            {
                this.WriteObject(StringResources.Info_DeletingServiceFabricNetwork);
                cluster.FabricClient.NetworkManager.DeleteNetworkAsync(
                    deletenetworkDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(StringResources.Info_DeleteServiceFabricNetworkSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                this.WriteObject(StringResources.Error_DeleteServiceFabricNetworkFailed);
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
                this.WriteObject(StringResources.Error_DeleteServiceFabricNetworkFailed);
                this.WriteObject(exception.ToString());
                this.ThrowTerminatingError(
                    exception,
                    Constants.NewNetworkConfigurationErrorId,
                    null);
            }
        }
    }
}