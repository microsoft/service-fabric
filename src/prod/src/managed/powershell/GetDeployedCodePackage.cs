// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricDeployedCodePackage")]
    public sealed class GetDeployedCodePackage : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1)]
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
        public string CodePackageName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                var queryResult = clusterConnection.GetDeployedCodePackageListAsync(
                                      this.NodeName,
                                      this.ApplicationName,
                                      this.ServiceManifestName,
                                      this.CodePackageName,
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
                        Constants.GetDeployedApplicationErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is DeployedCodePackage)
            {
                var item = output as DeployedCodePackage;
                var itemPSObj = new PSObject(item);

                var entryPointPSObject = new PSObject(item.EntryPoint);
                entryPointPSObject.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.EntryPointPropertyName,
                        entryPointPSObject));

                if (item.SetupEntryPoint != null)
                {
                    var setupEntryPointPSObject = new PSObject(item.SetupEntryPoint);
                    setupEntryPointPSObject.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    itemPSObj.Properties.Add(
                        new PSNoteProperty(
                            Constants.SetupEntryPointPropertyName,
                            setupEntryPointPSObject));
                }
            }

            return base.FormatOutput(output);
        }
    }
}