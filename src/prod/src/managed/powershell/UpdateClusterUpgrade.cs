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
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Management.Automation;

    [Cmdlet(VerbsData.Update, "ServiceFabricClusterUpgrade", SupportsShouldProcess = true, ConfirmImpact = ConfirmImpact.High)]
    public sealed class UpdateClusterUpgrade : ClusterCmdletBase
    {
        [Parameter(Mandatory = false)]
        public bool? ForceRestart
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public uint? UpgradeReplicaSetCheckTimeoutSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public RollingUpgradeMode? UpgradeMode
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public UpgradeFailureAction? FailureAction
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public uint? HealthCheckRetryTimeoutSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public uint? HealthCheckWaitDurationSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public uint? HealthCheckStableDurationSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public uint? UpgradeDomainTimeoutSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public uint? UpgradeTimeoutSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public bool? ConsiderWarningAsError
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateRange(0, 100)]
        public byte? MaxPercentUnhealthyApplications
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateRange(0, 100)]
        public byte? MaxPercentUnhealthyNodes
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public ApplicationTypeHealthPolicyMap ApplicationTypeHealthPolicyMap
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public bool? EnableDeltaHealthEvaluation
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateRange(0, 100)]
        public byte? MaxPercentDeltaUnhealthyNodes
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateRange(0, 100)]
        public byte? MaxPercentUpgradeDomainDeltaUnhealthyNodes
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter Force
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public ApplicationHealthPolicyMap ApplicationHealthPolicyMap
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            try
            {
                var updateDescription = new FabricUpgradeUpdateDescription();

                if (this.ForceRestart.HasValue)
                {
                    updateDescription.ForceRestart = this.ForceRestart.Value;
                }

                if (this.UpgradeReplicaSetCheckTimeoutSec.HasValue)
                {
                    updateDescription.UpgradeReplicaSetCheckTimeout = TimeSpan.FromSeconds(this.UpgradeReplicaSetCheckTimeoutSec.Value);
                }

                if (this.UpgradeMode.HasValue)
                {
                    updateDescription.UpgradeMode = this.UpgradeMode.Value;
                }

                if (this.FailureAction.HasValue)
                {
                    updateDescription.FailureAction = this.FailureAction.Value;
                }

                if (this.HealthCheckWaitDurationSec.HasValue)
                {
                    updateDescription.HealthCheckWaitDuration = TimeSpan.FromSeconds(this.HealthCheckWaitDurationSec.Value);
                }

                if (this.HealthCheckStableDurationSec.HasValue)
                {
                    updateDescription.HealthCheckStableDuration = TimeSpan.FromSeconds(this.HealthCheckStableDurationSec.Value);
                }

                if (this.HealthCheckRetryTimeoutSec.HasValue)
                {
                    updateDescription.HealthCheckRetryTimeout = TimeSpan.FromSeconds(this.HealthCheckRetryTimeoutSec.Value);
                }

                if (this.UpgradeTimeoutSec.HasValue)
                {
                    updateDescription.UpgradeTimeout = TimeSpan.FromSeconds(this.UpgradeTimeoutSec.Value);
                }

                if (this.UpgradeDomainTimeoutSec.HasValue)
                {
                    updateDescription.UpgradeDomainTimeout = TimeSpan.FromSeconds(this.UpgradeDomainTimeoutSec.Value);
                }

                if (this.IsUpdatingHealthPolicy())
                {
                    if (!this.Force && !this.IsHealthPolicyComplete() && !this.ShouldProcess(
                            //// description shows up for "-WhatIf"
                            string.Format(
                                CultureInfo.InvariantCulture,
                                "{0} {1}",
                                StringResources.PowerShell_HealthPolicyUpdateCaption,
                                StringResources.PowerShell_ClusterHealthPolicyUpdateWarning),
                            //// warning and caption show up when prompting for confirmation
                            StringResources.PowerShell_ClusterHealthPolicyUpdateWarning,
                            StringResources.PowerShell_HealthPolicyUpdateCaption))
                    {
                        return;
                    }

                    var healthPolicy = new ClusterHealthPolicy();

                    if (this.ConsiderWarningAsError.HasValue)
                    {
                        healthPolicy.ConsiderWarningAsError = this.ConsiderWarningAsError.Value;
                    }

                    if (this.MaxPercentUnhealthyApplications.HasValue)
                    {
                        healthPolicy.MaxPercentUnhealthyApplications = this.MaxPercentUnhealthyApplications.Value;
                    }

                    if (this.MaxPercentUnhealthyNodes.HasValue)
                    {
                        healthPolicy.MaxPercentUnhealthyNodes = this.MaxPercentUnhealthyNodes.Value;
                    }

                    if (this.ApplicationTypeHealthPolicyMap != null)
                    {
                        foreach (var entry in this.ApplicationTypeHealthPolicyMap)
                        {
                            healthPolicy.ApplicationTypeHealthPolicyMap.Add(entry.Key, entry.Value);
                        }
                    }

                    updateDescription.HealthPolicy = healthPolicy;
                }

                if (this.EnableDeltaHealthEvaluation.HasValue)
                {
                    updateDescription.EnableDeltaHealthEvaluation = this.EnableDeltaHealthEvaluation.Value;
                }

                if (this.IsUpdatingUpgradeHealthPolicy())
                {
                    if (!this.Force && !this.IsUpgradeHealthPolicyComplete() && !this.ShouldProcess(
                            //// description shows up for "-WhatIf"
                            string.Format(
                                CultureInfo.InvariantCulture,
                                "{0} {1}",
                                StringResources.PowerShell_HealthPolicyUpdateCaption,
                                StringResources.PowerShell_ClusterUpgradeHealthPolicyUpdateWarning),
                            //// warning and caption show up when prompting for confirmation
                            StringResources.PowerShell_ClusterUpgradeHealthPolicyUpdateWarning,
                            StringResources.PowerShell_HealthPolicyUpdateCaption))
                    {
                        return;
                    }

                    var upgradeHealthPolicy = new ClusterUpgradeHealthPolicy();

                    if (this.MaxPercentDeltaUnhealthyNodes.HasValue)
                    {
                        upgradeHealthPolicy.MaxPercentDeltaUnhealthyNodes = this.MaxPercentDeltaUnhealthyNodes.Value;
                    }

                    if (this.MaxPercentUpgradeDomainDeltaUnhealthyNodes.HasValue)
                    {
                        upgradeHealthPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes = this.MaxPercentUpgradeDomainDeltaUnhealthyNodes.Value;
                    }

                    updateDescription.UpgradeHealthPolicy = upgradeHealthPolicy;
                }

                updateDescription.ApplicationHealthPolicyMap = this.ApplicationHealthPolicyMap;

                this.UpdateClusterUpgrade(updateDescription);
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.UpdateClusterUpgradeErrorId,
                    null);
            }
        }

        protected override object FormatOutput(object output)
        {
            var casted = output as FabricUpgradeUpdateDescription;

            if (casted != null)
            {
                var itemPSObj = new PSObject(casted);

                if (casted.HealthPolicy != null)
                {
                    this.AddToPSObject(itemPSObj, casted.HealthPolicy);
                }

                if (casted.UpgradeHealthPolicy != null)
                {
                    this.AddToPSObject(itemPSObj, casted.UpgradeHealthPolicy);
                }

                if (casted.ApplicationHealthPolicyMap != null)
                {
                    this.AddToPSObject(itemPSObj, casted.ApplicationHealthPolicyMap);
                }

                return itemPSObj;
            }
            else
            {
                return base.FormatOutput(output);
            }
        }

        private bool IsUpdatingHealthPolicy()
        {
            return
                this.ConsiderWarningAsError.HasValue ||
                this.MaxPercentUnhealthyApplications.HasValue ||
                this.MaxPercentUnhealthyNodes.HasValue ||
                (this.ApplicationTypeHealthPolicyMap != null);
        }

        private bool IsHealthPolicyComplete()
        {
            return
                this.ConsiderWarningAsError.HasValue &&
                this.MaxPercentUnhealthyApplications.HasValue &&
                this.MaxPercentUnhealthyNodes.HasValue &&
                (this.ApplicationTypeHealthPolicyMap != null);
        }

        private bool IsUpdatingUpgradeHealthPolicy()
        {
            return
                this.MaxPercentDeltaUnhealthyNodes.HasValue ||
                this.MaxPercentUpgradeDomainDeltaUnhealthyNodes.HasValue;
        }

        private bool IsUpgradeHealthPolicyComplete()
        {
            return
                this.MaxPercentDeltaUnhealthyNodes.HasValue &&
                this.MaxPercentUpgradeDomainDeltaUnhealthyNodes.HasValue;
        }
    }
}