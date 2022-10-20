// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Result;
    using System.Linq;
    using System.Management.Automation;

    using Microsoft.ServiceFabric.DeploymentManager;
    using Microsoft.ServiceFabric.DeploymentManager.Model;

    [Cmdlet(VerbsCommon.Get, Constants.GetRuntimeUpgradeVersion)]

    public sealed class GetRuntimeUpgradeVersion : ClusterCmdletBase
    {
        [Parameter(Mandatory = true)]
        public string BaseVersion
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {            
            try
            {
                var runtimeVersions = this.GetRuntimeUpgradeVersion(
                        this.BaseVersion);
                this.WriteObject(this.FormatOutput(runtimeVersions));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetRuntimeVersionsErrorId,
                        this);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            var item = output as List<RuntimePackageDetails>;

            if (item == null)
            {
                return base.FormatOutput(output);
            }

            var itemPsObj = new PSObject(item);           
            
            var statePropertyPsObj = new PSObject(item);
            statePropertyPsObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            itemPsObj.Properties.Add(
                new PSNoteProperty(
                    Constants.RuntimePackagesPropertyName,
                    statePropertyPsObj));

            return itemPsObj;
        }
    }
}