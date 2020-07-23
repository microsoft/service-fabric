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

    [Cmdlet(VerbsLifecycle.Start, "ServiceFabricClusterUpgrade", DefaultParameterSetName = "Both UnmonitoredAuto", SupportsShouldProcess = true, ConfirmImpact = ConfirmImpact.High)]
    public sealed class StartClusterUpgrade : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Code UnmonitoredAuto")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Code UnmonitoredManual")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Code Monitored")]
        public SwitchParameter Code
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Config UnmonitoredAuto")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Config UnmonitoredManual")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Config Monitored")]
        public SwitchParameter Config
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Code UnmonitoredAuto")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Code UnmonitoredManual")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Both UnmonitoredAuto")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Both UnmonitoredManual")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Both Monitored")]
        public string CodePackageVersion
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Config UnmonitoredAuto")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Config UnmonitoredManual")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Both UnmonitoredAuto")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Both UnmonitoredManual")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Both Monitored")]
        public string ClusterManifestVersion
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, Position = 2)]
        public SwitchParameter ForceRestart
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, Position = 3)]
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

        [Parameter(Mandatory = true, ParameterSetName = "Code UnmonitoredAuto")]
        [Parameter(Mandatory = true, ParameterSetName = "Config UnmonitoredAuto")]
        [Parameter(Mandatory = true, ParameterSetName = "Both UnmonitoredAuto")]
        public SwitchParameter UnmonitoredAuto
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Code UnmonitoredManual")]
        [Parameter(Mandatory = true, ParameterSetName = "Config UnmonitoredManual")]
        [Parameter(Mandatory = true, ParameterSetName = "Both UnmonitoredManual")]
        public SwitchParameter UnmonitoredManual
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = true, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = true, ParameterSetName = "Both Monitored")]
        public SwitchParameter Monitored
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = true, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = true, ParameterSetName = "Both Monitored")]
        public UpgradeFailureAction FailureAction
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Both Monitored")]
        public uint? HealthCheckRetryTimeoutSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Both Monitored")]
        public uint? HealthCheckWaitDurationSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Both Monitored")]
        public uint? HealthCheckStableDurationSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Both Monitored")]
        public uint? UpgradeDomainTimeoutSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Both Monitored")]
        public uint? UpgradeTimeoutSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Both Monitored")]
        public bool? ConsiderWarningAsError
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Both Monitored")]
        [ValidateRange(0, 100)]
        public byte? MaxPercentUnhealthyApplications
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Both Monitored")]
        [ValidateRange(0, 100)]
        public byte? MaxPercentUnhealthyNodes
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Both Monitored")]
        public ApplicationTypeHealthPolicyMap ApplicationTypeHealthPolicyMap
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Both Monitored")]
        public SwitchParameter EnableDeltaHealthEvaluation
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Both Monitored")]
        [ValidateRange(0, 100)]
        public byte? MaxPercentDeltaUnhealthyNodes
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Both Monitored")]
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

        [Parameter(Mandatory = false, ParameterSetName = "Code Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Config Monitored")]
        [Parameter(Mandatory = false, ParameterSetName = "Both Monitored")]
        public ApplicationHealthPolicyMap ApplicationHealthPolicyMap
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

                RollingUpgradePolicyDescription upgradePolicyDescription;
                if (this.Monitored)
                {
                    var monitoringPolicy = new RollingUpgradeMonitoringPolicy();

                    if (this.FailureAction != UpgradeFailureAction.Invalid)
                    {
                        monitoringPolicy.FailureAction = this.FailureAction;
                    }

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

                    var monitoredPolicyDescription = new MonitoredRollingFabricUpgradePolicyDescription
                    {
                        UpgradeMode = RollingUpgradeMode.Monitored,
                        ForceRestart = this.ForceRestart,
                        MonitoringPolicy = monitoringPolicy
                    };

                    upgradePolicyDescription = monitoredPolicyDescription;

                    if (this.IsUpdatingHealthPolicy())
                    {
                        if (!this.Force && !this.IsHealthPolicyComplete() && !this.ShouldProcess(
                                //// description shows up for "-WhatIf"
                                string.Format(
                                    CultureInfo.InvariantCulture,
                                    "{0} {1}",
                                    StringResources.PowerShell_HealthPolicyUpgradeCaption,
                                    StringResources.PowerShell_ClusterHealthPolicyUpdateWarning),
                                //// warning and caption show up when prompting for confirmation
                                StringResources.PowerShell_ClusterHealthPolicyUpdateWarning,
                                StringResources.PowerShell_HealthPolicyUpgradeCaption))
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

                        monitoredPolicyDescription.HealthPolicy = healthPolicy;
                    }

                    monitoredPolicyDescription.EnableDeltaHealthEvaluation = this.EnableDeltaHealthEvaluation;

                    if (this.IsUpdatingUpgradeHealthPolicy())
                    {
                        if (!this.Force && !this.IsUpgradeHealthPolicyComplete() && !this.ShouldProcess(
                                //// description shows up for "-WhatIf"
                                string.Format(
                                    CultureInfo.InvariantCulture,
                                    "{0} {1}",
                                    StringResources.PowerShell_HealthPolicyUpgradeCaption,
                                    StringResources.PowerShell_ClusterUpgradeHealthPolicyUpdateWarning),
                                //// warning and caption show up when prompting for confirmation
                                StringResources.PowerShell_ClusterUpgradeHealthPolicyUpdateWarning,
                                StringResources.PowerShell_HealthPolicyUpgradeCaption))
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

                        monitoredPolicyDescription.UpgradeHealthPolicy = upgradeHealthPolicy;
                    }

                    if (this.ApplicationHealthPolicyMap != null)
                    {
                        foreach (var entry in this.ApplicationHealthPolicyMap)
                        {
                            monitoredPolicyDescription.ApplicationHealthPolicyMap.Add(entry.Key, entry.Value);
                        }
                    }
                }
                else if (this.UnmonitoredManual)
                {
                    upgradePolicyDescription = new RollingUpgradePolicyDescription
                    {
                        UpgradeMode = RollingUpgradeMode.UnmonitoredManual,
                        ForceRestart = this.ForceRestart,
                    };
                }
                else
                {
                    upgradePolicyDescription = new RollingUpgradePolicyDescription
                    {
                        UpgradeMode = RollingUpgradeMode.UnmonitoredAuto,
                        ForceRestart = this.ForceRestart,
                    };
                }

                if (this.UpgradeReplicaSetCheckTimeoutSec.HasValue)
                {
                    upgradePolicyDescription.UpgradeReplicaSetCheckTimeout = TimeSpan.FromSeconds(this.UpgradeReplicaSetCheckTimeoutSec.Value);
                }

                var upgradeDescription = new FabricUpgradeDescription
                {
                    UpgradePolicyDescription = upgradePolicyDescription,
                    TargetCodeVersion = this.CodePackageVersion,
                    TargetConfigVersion = this.ClusterManifestVersion
                };

                this.UpgradeCluster(upgradeDescription);
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.UpgradeClusterErrorId,
                    null);
            }
        }

        protected override object FormatOutput(object output)
        {
            var casted = output as FabricUpgradeDescription;

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