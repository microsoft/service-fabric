// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Management.Automation;
    using Microsoft.ServiceFabric.Preview.Client.Description;

    public abstract class ComposeDeploymentCmdletBase : ApplicationCmdletBase
    {
        protected void CreateComposeDeployment(ComposeDeploymentDescription description)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.CreateComposeDeploymentAsync(
                    description,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(new PSObject(description));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.CreateComposeDeploymentInstanceErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void DeleteComposeDeployment(DeleteComposeDeploymentDescription description)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.DeleteComposeDeploymentAsync(
                    description,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RemoveComposeDeploymentInstanceErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected ComposeDeploymentUpgradeProgress GetComposeDeploymentUpgradeProgress(string deploymentName)
        {
            var clusterConnection = this.GetClusterConnection();

            ComposeDeploymentUpgradeProgress currentProgress = null;
            try
            {
                currentProgress =
                    clusterConnection.GetComposeDeploymentUpgradeProgressAsync(
                        deploymentName,
                        this.GetTimeout(),
                        this.GetCancellationToken()).Result;
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetComposeDeploymentUpgradeProgressErrorId,
                        clusterConnection);
                    return true;
                });
            }

            return currentProgress;
        }

        protected void UpgradeComposeDeployment(ComposeDeploymentUpgradeDescription upgradeDescription)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.UpgradeComposeDeploymentAsync(
                    upgradeDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(this.FormatOutput(upgradeDescription), true);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.UpgradeComposeDeploymentErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void RollbackComposeDeploymentUpgrade(ComposeDeploymentRollbackDescription description)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.RollbackComposeDeploymentUpgradeAsync(
                    description,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RollbackComposeDeploymentUpgradeErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected PSObject ToPSObject(ComposeDeploymentUpgradeDescription upgradeDescription)
        {
            var itemPSObj = new PSObject(upgradeDescription);

            var composeFilePathsPSObj = new PSObject(upgradeDescription.ComposeFilePaths);
            composeFilePathsPSObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            itemPSObj.Properties.Add(
                new PSNoteProperty(
                    Constants.ComposeFilePathsPropertyName,
                    composeFilePathsPSObj));

            if (upgradeDescription.UpgradePolicyDescription == null)
            {
                return itemPSObj;
            }

            var monitoredPolicy = upgradeDescription.UpgradePolicyDescription as MonitoredRollingApplicationUpgradePolicyDescription;
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
                var policy = upgradeDescription.UpgradePolicyDescription as RollingUpgradePolicyDescription;
                if (policy != null)
                {
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeKindPropertyName, policy.Kind));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.ForceRestartPropertyName, policy.ForceRestart));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeModePropertyName, policy.UpgradeMode));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeReplicaSetCheckTimeoutPropertyName, policy.UpgradeReplicaSetCheckTimeout));
                }
            }

            return itemPSObj;
        }
    }
}