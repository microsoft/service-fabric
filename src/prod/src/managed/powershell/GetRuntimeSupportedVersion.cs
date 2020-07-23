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

    [Cmdlet(VerbsCommon.Get, Constants.GetRuntimeSupportedVersion)]

    public sealed class GetRuntimeSupportedVersion : ClusterCmdletBase
    {
        [Parameter(Mandatory = false, Position = 0)]
        public SwitchParameter Latest
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        { 
            DownloadableRuntimeVersionReturnSet returnSet;
            if (this.Latest)
            {
               returnSet = DownloadableRuntimeVersionReturnSet.Latest;
            }
            else
            {
                returnSet = DownloadableRuntimeVersionReturnSet.Supported;
            }

            try
            {
                var runtimeVersions = this.GetRuntimeSupportedVersion(
                        returnSet);
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
            RuntimePackageDetails goalPackageDetails = item.SingleOrDefault(goalPackage => goalPackage.IsGoalPackage);           
            if (goalPackageDetails != null)
            {
                // State
                var statePropertyPsObj = new PSObject(goalPackageDetails.Version);
                statePropertyPsObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                itemPsObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.GoalRuntimeVersionPropertyName,
                        statePropertyPsObj));

                var statePropertyPsObj2 = new PSObject(goalPackageDetails.TargetPackageLocation);
                statePropertyPsObj2.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                itemPsObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.GoalRuntimeLocationPropertyName,
                        statePropertyPsObj2));
            }
            
            var statePropertyPsObj3 = new PSObject(item);
            statePropertyPsObj3.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            itemPsObj.Properties.Add(
                new PSNoteProperty(
                    Constants.RuntimePackagesPropertyName,
                    statePropertyPsObj3));

            return itemPsObj;
        }
    }
}