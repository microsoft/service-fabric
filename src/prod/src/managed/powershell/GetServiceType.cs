// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricServiceType")]
    public sealed class GetServiceType : CommonCmdletBase
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
        public string ServiceTypeName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var queryResult = clusterConnection.GetServiceTypeListAsync(
                                      this.ApplicationTypeName,
                                      this.ApplicationTypeVersion,
                                      this.ServiceTypeName,
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
            if (output is ServiceType)
            {
                var item = output as ServiceType;

                var loadMetricsPropertyPSObj = new PSObject(item.ServiceTypeDescription.LoadMetrics);
                loadMetricsPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var placementPoliciesPSObj = new PSObject(item.ServiceTypeDescription.Policies);
                placementPoliciesPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var extensionsPropertyPSObj = new PSObject(item.ServiceTypeDescription.Extensions);
                extensionsPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var itemPSObj = new PSObject(item);

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.ServiceTypeNamePropertyName,
                        item.ServiceTypeDescription.ServiceTypeName));

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.IsServiceGroupPropertyName,
                        item.IsServiceGroup));

                itemPSObj.Properties.Add(new PSNoteProperty(
                                             Constants.ServiceTypeKindPropertyName,
                                             item.ServiceTypeDescription.ServiceTypeKind));

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.LoadMetricsPropertyName,
                        loadMetricsPropertyPSObj));

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.PlacementPoliciesPropertyName,
                        placementPoliciesPSObj));

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.ExtensionsPropertyName,
                        extensionsPropertyPSObj));

                itemPSObj.Properties.Add(new PSNoteProperty(
                                             Constants.PlacementConstraintsPropertyName,
                                             string.IsNullOrEmpty(item.ServiceTypeDescription.PlacementConstraints) ? Constants.NoneResultOutput : item.ServiceTypeDescription.PlacementConstraints));

                if (item.ServiceTypeDescription.ServiceTypeKind == ServiceDescriptionKind.Stateful)
                {
                    itemPSObj.Properties.Add(new PSNoteProperty(
                                                 Constants.HasPersistedStatePropertyName,
                                                 ((StatefulServiceTypeDescription)item.ServiceTypeDescription).HasPersistedState));
                }
                else if (item.ServiceTypeDescription.ServiceTypeKind == ServiceDescriptionKind.Stateless)
                {
                    itemPSObj.Properties.Add(new PSNoteProperty(
                                                 Constants.UseImplicitHostPropertyName,
                                                 ((StatelessServiceTypeDescription)item.ServiceTypeDescription).UseImplicitHost));
                }

                return itemPSObj;
            }

            return base.FormatOutput(output);
        }
    }
}