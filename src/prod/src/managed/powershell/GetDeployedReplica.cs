// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricDeployedReplica", DefaultParameterSetName = "Non-Adhoc")]
    public sealed class GetDeployedReplica : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Adhoc")]
        public SwitchParameter Adhoc
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1, ParameterSetName = "Non-Adhoc")]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 2)]
        public string ServiceManifestName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 3)]
        public Guid? PartitionId
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var queryResult = clusterConnection.GetDeployedReplicaListAsync(
                                      this.NodeName,
                                      this.Adhoc ? null : this.ApplicationName,
                                      this.ServiceManifestName,
                                      this.PartitionId,
                                      this.GetTimeout(),
                                      this.GetCancellationToken()).Result;

                foreach (var item in queryResult)
                {
                    this.WriteObject(this.FormatOutput(item));
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetDeployedReplicaErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is DeployedStatefulServiceReplica)
            {
                var item = output as DeployedStatefulServiceReplica;
                var result = new PSObject(item);

                result.Properties.Add(
                    new PSAliasProperty(Constants.ReplicaOrInstanceIdPropertyName, Constants.ReplicaIdPropertyName));

                FormatReconfigurationInformation(item.ReconfigurationInformation, result);

                return result;
            }

            if (output is DeployedStatelessServiceInstance)
            {
                var item = output as DeployedStatelessServiceInstance;
                var result = new PSObject(item);

                result.Properties.Add(
                    new PSAliasProperty(Constants.ReplicaOrInstanceIdPropertyName, Constants.InstanceIdPropertyName));

                return result;
            }

            return base.FormatOutput(output);
        }

        private static void FormatReconfigurationInformation(System.Fabric.ReconfigurationInformation reconfigurationInformation, PSObject result)
        {
            var reconfigurationInformationObj = new PSObject(reconfigurationInformation);
            reconfigurationInformationObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));
            result.Properties.Add(
                new PSNoteProperty(
                    Constants.ReconfigurationInformation,
                    reconfigurationInformationObj));
        }
    }
}