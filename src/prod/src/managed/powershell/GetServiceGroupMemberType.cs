// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricServiceGroupMemberType")]
    public sealed class GetServiceGroupMemberType : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string ApplicationTypeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1)]
        public string ApplicationTypeVersion
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 2)]
        public string ServiceGroupTypeName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var queryResult = clusterConnection.GetServiceGroupMemberTypeListAsync(
                                      this.ApplicationTypeName,
                                      this.ApplicationTypeVersion,
                                      this.ServiceGroupTypeName,
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
            if (output is ServiceGroupMemberType)
            {
                var item = output as ServiceGroupMemberType;

                var itemPSObj = new PSObject(item);

                var memberDescriptionsPSObj = new PSObject(item.ServiceGroupMemberTypeDescription);
                memberDescriptionsPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));
                itemPSObj.Properties.Add(new PSNoteProperty(
                                             Constants.MemberTypesPropertyName,
                                             memberDescriptionsPSObj));

                return itemPSObj;
            }

            return base.FormatOutput(output);
        }
    }
}