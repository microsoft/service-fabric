// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Management.Automation;

    [Cmdlet(VerbsData.Update, "ServiceFabricApplicationUpgrade", SupportsShouldProcess = true, ConfirmImpact = ConfirmImpact.High)]
    public sealed class UpdateApplicationUpgrade : ApplicationCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public Uri ApplicationName
        {
            get;
            set;
        }

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
        public string DefaultServiceTypeHealthPolicy
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateRange(0, 100)]
        public byte? MaxPercentUnhealthyDeployedApplications
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public Hashtable ServiceTypeHealthPolicyMap
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

        protected override void ProcessRecord()
        {
            try
            {
                var updateDescription = new ApplicationUpgradeUpdateDescription()
                {
                    ApplicationName = this.ApplicationName,
                };

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
                                StringResources.PowerShell_ApplicationHealthPolicyUpdateWarning),
                            //// warning and caption show up when prompting for confirmation
                            StringResources.PowerShell_ApplicationHealthPolicyUpdateWarning,
                            StringResources.PowerShell_HealthPolicyUpdateCaption))
                    {
                        return;
                    }

                    var healthPolicy = new ApplicationHealthPolicy();

                    if (this.ConsiderWarningAsError.HasValue)
                    {
                        healthPolicy.ConsiderWarningAsError = this.ConsiderWarningAsError.Value;
                    }

                    if (this.DefaultServiceTypeHealthPolicy != null)
                    {
                        healthPolicy.DefaultServiceTypeHealthPolicy = this.ParseServiceTypeHealthPolicy(this.DefaultServiceTypeHealthPolicy);
                    }

                    if (this.MaxPercentUnhealthyDeployedApplications.HasValue)
                    {
                        healthPolicy.MaxPercentUnhealthyDeployedApplications = this.MaxPercentUnhealthyDeployedApplications.Value;
                    }

                    if (this.ServiceTypeHealthPolicyMap != null)
                    {
                        foreach (DictionaryEntry entry in this.ServiceTypeHealthPolicyMap)
                        {
                            healthPolicy.ServiceTypeHealthPolicyMap.Add(entry.Key as string, this.ParseServiceTypeHealthPolicy(entry.Value as string));
                        }
                    }

                    updateDescription.HealthPolicy = healthPolicy;
                }

                this.UpdateApplicationUpgrade(updateDescription);
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.UpdateApplicationUpgradeErrorId,
                    null);
            }
        }

        protected override object FormatOutput(object output)
        {
            var casted = output as ApplicationUpgradeUpdateDescription;

            if (casted != null)
            {
                var itemPSObj = new PSObject(casted);

                if (casted.HealthPolicy != null)
                {
                    this.AddToPSObject(itemPSObj, casted.HealthPolicy);
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
                this.DefaultServiceTypeHealthPolicy != null ||
                this.MaxPercentUnhealthyDeployedApplications.HasValue ||
                this.ServiceTypeHealthPolicyMap != null;
        }

        private bool IsHealthPolicyComplete()
        {
            return
                this.ConsiderWarningAsError.HasValue &&
                this.DefaultServiceTypeHealthPolicy != null &&
                this.MaxPercentUnhealthyDeployedApplications.HasValue &&
                this.ServiceTypeHealthPolicyMap != null;
        }
    }
}