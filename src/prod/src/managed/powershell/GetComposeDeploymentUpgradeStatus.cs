// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricComposeDeploymentUpgrade")]
    public sealed class GetComposeDeploymentUpgradeStatus : ComposeDeploymentCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string DeploymentName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var upgradeProgress = this.GetComposeDeploymentUpgradeProgress(this.DeploymentName);
            this.WriteObject(this.FormatOutput(upgradeProgress), true);
        }
        
        protected override object FormatOutput(object output)
        {
            if (output is ComposeDeploymentUpgradeProgress)
            {
                var casted = output as ComposeDeploymentUpgradeProgress;

                var itemPSObj = new PSObject(casted);

                var monitoredPolicy = casted.UpgradePolicyDescription as MonitoredRollingApplicationUpgradePolicyDescription;
                if (monitoredPolicy != null)
                {
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeKindPropertyName, monitoredPolicy.Kind));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.ForceRestartPropertyName, monitoredPolicy.ForceRestart));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeModePropertyName, monitoredPolicy.UpgradeMode));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeReplicaSetCheckTimeoutPropertyName, monitoredPolicy.UpgradeReplicaSetCheckTimeout));

                    if (monitoredPolicy.MonitoringPolicy != null)
                    {
                        itemPSObj.Properties.Add(new PSNoteProperty(Constants.FailureActionPropertyName, monitoredPolicy.MonitoringPolicy.FailureAction));
                        itemPSObj.Properties.Add(new PSNoteProperty(Constants.HealthCheckWaitDurationPropertyName, monitoredPolicy.MonitoringPolicy.HealthCheckWaitDuration));
                        itemPSObj.Properties.Add(new PSNoteProperty(Constants.HealthCheckStableDurationPropertyName, monitoredPolicy.MonitoringPolicy.HealthCheckStableDuration));
                        itemPSObj.Properties.Add(new PSNoteProperty(Constants.HealthCheckRetryTimeoutPropertyName, monitoredPolicy.MonitoringPolicy.HealthCheckRetryTimeout));
                        itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeDomainTimeoutPropertyName, monitoredPolicy.MonitoringPolicy.UpgradeDomainTimeout));
                        itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeTimeoutPropertyName, monitoredPolicy.MonitoringPolicy.UpgradeTimeout));
                    }

                    if (monitoredPolicy.HealthPolicy != null)
                    {
                        this.AddToPSObject(itemPSObj, monitoredPolicy.HealthPolicy);
                    }
                }
                else
                {
                    var policy = casted.UpgradePolicyDescription as RollingUpgradePolicyDescription;
                    if (policy != null)
                    {
                        itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeKindPropertyName, policy.Kind));
                        itemPSObj.Properties.Add(new PSNoteProperty(Constants.ForceRestartPropertyName, policy.ForceRestart));
                        itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeModePropertyName, policy.UpgradeMode));
                        itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeReplicaSetCheckTimeoutPropertyName, policy.UpgradeReplicaSetCheckTimeout));
                    }
                }

                var upgradeDomainsPropertyPSObj = new PSObject(casted.UpgradeDomains);
                upgradeDomainsPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.UpgradeDomainsPropertyName,
                        upgradeDomainsPropertyPSObj));

                Helpers.AddToPsObject(itemPSObj, casted.ApplicationUnhealthyEvaluations);

                if (casted.CurrentUpgradeDomainProgress != null)
                {
                    var progressDetailsPSObj = new PSObject(casted.CurrentUpgradeDomainProgress);
                    progressDetailsPSObj.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    itemPSObj.Properties.Add(
                        new PSNoteProperty(
                            Constants.CurrentUpgradeDomainProgressPropertyName,
                            progressDetailsPSObj));
                }

                if (casted.UpgradeDomainProgressAtFailure != null)
                {
                    var progressDetailsPSObj = new PSObject(casted.UpgradeDomainProgressAtFailure);
                    progressDetailsPSObj.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    itemPSObj.Properties.Add(
                        new PSNoteProperty(
                            Constants.UpgradeDomainProgressAtFailurePropertyName,
                            progressDetailsPSObj));
                }

                return itemPSObj;
            }
            else
            {
                return base.FormatOutput(output);
            }
        }
    }
}