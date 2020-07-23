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

    [Cmdlet(VerbsLifecycle.Start, "ServiceFabricApplicationUpgrade", DefaultParameterSetName = "UnmonitoredAuto", SupportsShouldProcess = true, ConfirmImpact = ConfirmImpact.High)]
    public sealed class StartApplicationUpgrade : ApplicationCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1)]
        public string ApplicationTypeVersion
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, Position = 2)]
        public Hashtable ApplicationParameter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, Position = 3)]
        public SwitchParameter ForceRestart
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, Position = 4)]
        public uint? UpgradeReplicaSetCheckTimeoutSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public uint? ReplicaQuorumTimeoutSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter RestartProcess
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "UnmonitoredAuto")]
        public SwitchParameter UnmonitoredAuto
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "UnmonitoredManual")]
        public SwitchParameter UnmonitoredManual
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Monitored")]
        public SwitchParameter Monitored
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Monitored")]
        public UpgradeFailureAction FailureAction
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Monitored")]
        public uint? HealthCheckRetryTimeoutSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Monitored")]
        public uint? HealthCheckWaitDurationSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Monitored")]
        public uint? HealthCheckStableDurationSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Monitored")]
        public uint? UpgradeDomainTimeoutSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Monitored")]
        public uint? UpgradeTimeoutSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Monitored")]
        public bool? ConsiderWarningAsError
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Monitored")]
        public string DefaultServiceTypeHealthPolicy
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Monitored")]
        [ValidateRange(0, 100)]
        public byte? MaxPercentUnhealthyDeployedApplications
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Monitored")]
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
                if (this.ReplicaQuorumTimeoutSec.HasValue)
                {
                    this.WriteWarning(StringResources.PowerShell_ReplicaQuorumTimeoutSec_Deprecated);

                    if (!this.UpgradeReplicaSetCheckTimeoutSec.HasValue)
                    {
                        this.UpgradeReplicaSetCheckTimeoutSec = this.ReplicaQuorumTimeoutSec.Value;
                    }
                }

                if (this.RestartProcess)
                {
                    this.WriteWarning(StringResources.PowerShell_RestartProcess_Deprecated);

                    if (!this.ForceRestart)
                    {
                        this.ForceRestart = this.RestartProcess;
                    }
                }

                RollingUpgradePolicyDescription policyDescription;
                if (this.Monitored)
                {
                    var monitoringPolicy = new RollingUpgradeMonitoringPolicy();

                    if (this.FailureAction == UpgradeFailureAction.Invalid)
                    {
                        this.FailureAction = Constants.DefaultUpgradeFailureAction;
                    }

                    monitoringPolicy.FailureAction = this.FailureAction;

                    if (this.HealthCheckRetryTimeoutSec.HasValue)
                    {
                        monitoringPolicy.HealthCheckRetryTimeout = TimeSpan.FromSeconds(this.HealthCheckRetryTimeoutSec.Value);
                    }

                    if (this.HealthCheckWaitDurationSec.HasValue)
                    {
                        monitoringPolicy.HealthCheckWaitDuration = TimeSpan.FromSeconds(this.HealthCheckWaitDurationSec.Value);
                    }

                    if (this.HealthCheckStableDurationSec.HasValue)
                    {
                        monitoringPolicy.HealthCheckStableDuration = TimeSpan.FromSeconds(this.HealthCheckStableDurationSec.Value);
                    }

                    if (this.UpgradeDomainTimeoutSec.HasValue)
                    {
                        monitoringPolicy.UpgradeDomainTimeout = TimeSpan.FromSeconds(this.UpgradeDomainTimeoutSec.Value);
                    }

                    if (this.UpgradeTimeoutSec.HasValue)
                    {
                        monitoringPolicy.UpgradeTimeout = TimeSpan.FromSeconds(this.UpgradeTimeoutSec.Value);
                    }

                    var monitoredPolicyDescription = new MonitoredRollingApplicationUpgradePolicyDescription
                    {
                        UpgradeMode = RollingUpgradeMode.Monitored,
                        ForceRestart = this.ForceRestart,
                        MonitoringPolicy = monitoringPolicy
                    };

                    policyDescription = monitoredPolicyDescription;

                    if (this.IsUpdatingHealthPolicy())
                    {
                        if (!this.Force && !this.IsHealthPolicyComplete() && !this.ShouldProcess(
                                //// description shows up for "-WhatIf"
                                string.Format(
                                    CultureInfo.InvariantCulture,
                                    "{0} {1}",
                                    StringResources.PowerShell_HealthPolicyUpgradeCaption,
                                    StringResources.PowerShell_ApplicationHealthPolicyUpdateWarning),
                                //// warning and caption show up when prompting for confirmation
                                StringResources.PowerShell_ApplicationHealthPolicyUpdateWarning,
                                StringResources.PowerShell_HealthPolicyUpgradeCaption))
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

                        monitoredPolicyDescription.HealthPolicy = healthPolicy;
                    }
                }
                else if (this.UnmonitoredManual)
                {
                    policyDescription = new RollingUpgradePolicyDescription
                    {
                        UpgradeMode = RollingUpgradeMode.UnmonitoredManual,
                        ForceRestart = this.ForceRestart,
                    };
                }
                else
                {
                    policyDescription = new RollingUpgradePolicyDescription
                    {
                        UpgradeMode = RollingUpgradeMode.UnmonitoredAuto,
                        ForceRestart = this.ForceRestart,
                    };
                }

                if (this.UpgradeReplicaSetCheckTimeoutSec.HasValue)
                {
                    policyDescription.UpgradeReplicaSetCheckTimeout = TimeSpan.FromSeconds(this.UpgradeReplicaSetCheckTimeoutSec.Value);
                }

                var upgradeDescription = new ApplicationUpgradeDescription
                {
                    ApplicationName = this.ApplicationName,
                    TargetApplicationTypeVersion = this.ApplicationTypeVersion,
                    UpgradePolicyDescription = policyDescription
                };
                upgradeDescription.ApplicationParameters.Add(this.GetNameValueCollection(this.ApplicationParameter));
                this.UpgradeApplication(upgradeDescription);
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.UpgradeApplicationErrorId,
                    null);
            }
        }

        protected override object FormatOutput(object output)
        {
            var casted = output as ApplicationUpgradeDescription;

            if (casted != null)
            {
                return this.ToPSObject(casted);
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