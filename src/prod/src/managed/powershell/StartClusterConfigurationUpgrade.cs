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
    using System.Fabric.Result;
    using System.IO;
    using System.Management.Automation;
    using System.Numerics;
    using DeploymentManager.Common;
    using Microsoft.ServiceFabric.DeploymentManager.Model;

    [Cmdlet(VerbsLifecycle.Start, Constants.StartClusterConfigurationUpgrade)]

    public sealed class StartClusterConfigurationUpgrade : CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public string ClusterConfigPath
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
        public ApplicationHealthPolicyMap ApplicationHealthPolicies
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                string clusterConfigAbsolutePath = GetAbsolutePath(this.ClusterConfigPath);
                string jsonConfigString = File.ReadAllText(clusterConfigAbsolutePath);
                var description = new ConfigurationUpgradeDescription
                {
                    ClusterConfiguration = jsonConfigString
                };

                if (this.HealthCheckRetryTimeoutSec.HasValue)
                {
                    description.HealthCheckRetryTimeout = TimeSpan.FromSeconds(this.HealthCheckRetryTimeoutSec.Value);
                }

                if (this.HealthCheckWaitDurationSec.HasValue)
                {
                    description.HealthCheckWaitDuration = TimeSpan.FromSeconds(this.HealthCheckWaitDurationSec.Value);
                }

                if (this.HealthCheckStableDurationSec.HasValue)
                {
                    description.HealthCheckStableDuration = TimeSpan.FromSeconds(this.HealthCheckStableDurationSec.Value);
                }

                if (this.UpgradeDomainTimeoutSec.HasValue)
                {
                    description.UpgradeDomainTimeout = TimeSpan.FromSeconds(this.UpgradeDomainTimeoutSec.Value);
                }

                if (this.UpgradeTimeoutSec.HasValue)
                {
                    description.UpgradeTimeout = TimeSpan.FromSeconds(this.UpgradeTimeoutSec.Value);
                }

                description.MaxPercentUnhealthyApplications =
                    this.MaxPercentUnhealthyApplications ?? description.MaxPercentUnhealthyApplications;

                description.MaxPercentUnhealthyNodes =
                    this.MaxPercentUnhealthyNodes ?? description.MaxPercentUnhealthyNodes;

                description.MaxPercentDeltaUnhealthyNodes =
                    this.MaxPercentDeltaUnhealthyNodes ?? description.MaxPercentDeltaUnhealthyNodes;

                description.MaxPercentUpgradeDomainDeltaUnhealthyNodes =
                    this.MaxPercentUpgradeDomainDeltaUnhealthyNodes ?? description.MaxPercentUpgradeDomainDeltaUnhealthyNodes;

                if (this.ApplicationHealthPolicies != null)
                {
                    foreach (var entry in this.ApplicationHealthPolicies)
                    {
                        description.ApplicationHealthPolicies.Add(entry.Key, entry.Value);
                    }
                }

                clusterConnection.StartClusterConfigurationUpgradeAsync(
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
                        Constants.StartClusterConfigurationUpgradeErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }
    }
}