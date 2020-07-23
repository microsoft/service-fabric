// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricServiceGroupMember", DefaultParameterSetName = "Non-Adhoc")]
    public sealed class GetServiceGroup : CommonCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Adhoc")]
        public SwitchParameter Adhoc
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "Non-Adhoc")]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 1)]
        public Uri ServiceName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                var queryResult = clusterConnection.GetServiceGroupMemberListAsync(
                                      this.Adhoc ? null : this.ApplicationName,
                                      this.ServiceName,
                                      this.GetTimeout(),
                                      this.GetCancellationToken()).Result;

                IList<ServiceGroupMember> serviceGroupMembers = queryResult;
                foreach (ServiceGroupMember serviceGroup in serviceGroupMembers)
                {
                    this.WriteObject(this.FormatOutput(serviceGroup));
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetServiceErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            var result = new PSObject(output);

            if (output is ServiceGroupMember)
            {
                var serviceGroup = output as ServiceGroupMember;

                result.Properties.Add(new PSNoteProperty(
                                          Constants.ServiceNamePropertyName,
                                          serviceGroup.ServiceName));

                var serviceGroupMemberMembersPSObject = new PSObject(serviceGroup.ServiceGroupMemberMembers);
                serviceGroupMemberMembersPSObject.Members.Add(new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));
                result.Properties.Add(new PSNoteProperty(Constants.ServiceGroupMembersPropertyName, serviceGroupMemberMembersPSObject));

                return result;
            }

            return base.FormatOutput(output);
        }
    }
}