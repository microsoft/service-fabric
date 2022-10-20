// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.New, "ServiceFabricService", DefaultParameterSetName = "Stateless Singleton Non-Adhoc")]
    public sealed class NewService : ServiceCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateful Singleton Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateful UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateful Named Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateful Singleton Adhoc")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateful UniformInt64 Adhoc")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateful Named Adhoc")]
        public SwitchParameter Stateful
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateless Singleton Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateless UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateless Named Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateless Singleton Adhoc")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateless UniformInt64 Adhoc")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateless Named Adhoc")]
        public SwitchParameter Stateless
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Stateless Singleton Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Stateful Singleton Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Stateless Singleton Adhoc")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Stateful Singleton Adhoc")]
        public SwitchParameter PartitionSchemeSingleton
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Stateless UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Stateful UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Stateless UniformInt64 Adhoc")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Stateful UniformInt64 Adhoc")]
        public SwitchParameter PartitionSchemeUniformInt64
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Stateless Named Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Stateful Named Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Stateless Named Adhoc")]
        [Parameter(Mandatory = true, Position = 1, ParameterSetName = "Stateful Named Adhoc")]
        public SwitchParameter PartitionSchemeNamed
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 2, ParameterSetName = "Stateless Singleton Adhoc")]
        [Parameter(Mandatory = true, Position = 2, ParameterSetName = "Stateless UniformInt64 Adhoc")]
        [Parameter(Mandatory = true, Position = 2, ParameterSetName = "Stateless Named Adhoc")]
        [Parameter(Mandatory = true, Position = 2, ParameterSetName = "Stateful Singleton Adhoc")]
        [Parameter(Mandatory = true, Position = 2, ParameterSetName = "Stateful UniformInt64 Adhoc")]
        [Parameter(Mandatory = true, Position = 2, ParameterSetName = "Stateful Named Adhoc")]
        public SwitchParameter Adhoc
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 2, ParameterSetName = "Stateless Singleton Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 2, ParameterSetName = "Stateless UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 2, ParameterSetName = "Stateless Named Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 2, ParameterSetName = "Stateful Singleton Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 2, ParameterSetName = "Stateful UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, Position = 2, ParameterSetName = "Stateful Named Non-Adhoc")]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 3)]
        public Uri ServiceName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 4)]
        public string ServiceTypeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Stateless UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateless UniformInt64 Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful UniformInt64 Adhoc")]
        public int PartitionCount
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Stateless UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateless UniformInt64 Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful UniformInt64 Adhoc")]
        public long LowKey
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Stateless UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateless UniformInt64 Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful UniformInt64 Adhoc")]
        public long HighKey
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Stateless Named Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful Named Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateless Named Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful Named Adhoc")]
        public string[] PartitionNames
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Stateful Singleton Non-Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful Named Non-Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful Singleton Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful UniformInt64 Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful Named Adhoc")]
        public SwitchParameter HasPersistedState
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Stateful Singleton Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful Named Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful Singleton Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful UniformInt64 Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful Named Adhoc")]
        public int TargetReplicaSetSize
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Stateful Singleton Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful Named Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful Singleton Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful UniformInt64 Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateful Named Adhoc")]
        public int MinReplicaSetSize
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Stateful Singleton Non-Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful Named Non-Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful Singleton Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful UniformInt64 Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful Named Adhoc")]
        public TimeSpan? ReplicaRestartWaitDuration
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Stateful Singleton Non-Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful Named Non-Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful Singleton Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful UniformInt64 Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful Named Adhoc")]
        public TimeSpan? QuorumLossWaitDuration
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Stateful Singleton Non-Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful Named Non-Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful Singleton Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful UniformInt64 Adhoc")]
        [Parameter(Mandatory = false, ParameterSetName = "Stateful Named Adhoc")]
        public TimeSpan? StandByReplicaKeepDuration
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Stateless Singleton Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateless UniformInt64 Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateless Named Non-Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateless Singleton Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateless UniformInt64 Adhoc")]
        [Parameter(Mandatory = true, ParameterSetName = "Stateless Named Adhoc")]
        public int InstanceCount
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string PlacementConstraint
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] Metric
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] Correlation
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] PlacementPolicy
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateSet("Zero", "Low", "Medium", "High")]
        public string DefaultMoveCost
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public ServicePackageActivationMode? ServicePackageActivationMode
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string ServiceDnsName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public List<ScalingPolicyDescription> ScalingPolicies
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            PartitionSchemeDescriptionBuilder partitionSchemeDescriptionBuilder;

            if (this.PartitionSchemeUniformInt64)
            {
                partitionSchemeDescriptionBuilder =
                    new UniformInt64RangePartitionSchemeDescriptionBuilder(
                    this.PartitionCount, this.LowKey, this.HighKey);
            }
            else if (this.PartitionSchemeNamed)
            {
                partitionSchemeDescriptionBuilder = new NamedPartitionSchemeDescriptionBuilder(this.PartitionNames);
            }
            else
            {
                partitionSchemeDescriptionBuilder = new SingletonPartitionSchemeDescriptionBuilder();
            }

            var activationMode = System.Fabric.Description.ServicePackageActivationMode.SharedProcess;
            if (this.ServicePackageActivationMode.HasValue)
            {
                activationMode = this.ServicePackageActivationMode.Value;
            }

            if (this.Stateful)
            {
                StatefulServiceDescriptionBuilder serviceDescriptionBuilder = new StatefulServiceDescriptionBuilder(
                    this.Adhoc ? null : this.ApplicationName,
                    partitionSchemeDescriptionBuilder,
                    this.HasPersistedState,
                    this.TargetReplicaSetSize,
                    this.MinReplicaSetSize,
                    this.ServiceName,
                    this.ServiceTypeName,
                    this.PlacementConstraint,
                    this.Metric,
                    this.Correlation,
                    this.PlacementPolicy,
                    this.ReplicaRestartWaitDuration,
                    this.QuorumLossWaitDuration,
                    this.StandByReplicaKeepDuration,
                    this.DefaultMoveCost,
                    activationMode,
                    this.ServiceDnsName,
                    this.ScalingPolicies);

                ServiceDescription sd = null;
                try
                {
                    sd = serviceDescriptionBuilder.Build();
                }
                catch (Exception exception)
                {
                    this.ThrowTerminatingError(
                        exception,
                        Constants.CreateServiceErrorId,
                        null);
                }

                this.CreateService(sd);
            }
            else
            {
                StatelessServiceDescriptionBuilder statelessServiceDescriptionBuilder = new StatelessServiceDescriptionBuilder(
                    this.Adhoc ? null : this.ApplicationName,
                    partitionSchemeDescriptionBuilder,
                    this.InstanceCount,
                    this.ServiceName,
                    this.ServiceTypeName,
                    this.PlacementConstraint,
                    this.Metric,
                    this.Correlation,
                    this.PlacementPolicy,
                    this.DefaultMoveCost,
                    activationMode,
                    this.ServiceDnsName,
                    this.ScalingPolicies);

                ServiceDescription sd = null;
                try
                {
                    sd = statelessServiceDescriptionBuilder.Build();
                }
                catch (Exception exception)
                {
                    this.ThrowTerminatingError(
                        exception,
                        Constants.CreateServiceErrorId,
                        null);
                }

                this.CreateService(sd);
            }
        }
    }
}