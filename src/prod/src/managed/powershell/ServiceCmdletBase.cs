// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Strings;
    using System.Management.Automation;

    public abstract class ServiceCmdletBase : CommonCmdletBase
    {
        internal static StatefulServiceLoadMetricDescription ParseStatefulMetric(string metricStr)
        {
            StatefulServiceLoadMetricDescription desc;
            string[] metricFields = metricStr.Split(',', ' ');

            if (metricFields.Length != 4)
            {
                throw new ArgumentException(StringResources.Error_InvalidMetricSpecification);
            }

            int primaryDefaultLoad, secondaryDefaultLoad;

            if (!int.TryParse(metricFields[2], out primaryDefaultLoad))
            {
                throw new ArgumentException(StringResources.Error_InvalidMetricSpecification);
            }

            if (!int.TryParse(metricFields[3], out secondaryDefaultLoad))
            {
                throw new ArgumentException(StringResources.Error_InvalidMetricSpecification);
            }

            desc = new StatefulServiceLoadMetricDescription()
            {
                Name = metricFields[0],
                Weight = ParseWeight(metricFields[1]),
                PrimaryDefaultLoad = primaryDefaultLoad,
                SecondaryDefaultLoad = secondaryDefaultLoad
            };

            return desc;
        }

        internal static StatelessServiceLoadMetricDescription ParseStatelessMetric(string metricStr)
        {
            StatelessServiceLoadMetricDescription desc;
            string[] metricFields = metricStr.Split(',', ' ');

            if (metricFields.Length != 3 && metricFields.Length != 4)
            {
                throw new ArgumentException(StringResources.Error_InvalidMetricSpecification);
            }

            int defaultLoad;

            if (!int.TryParse(metricFields[2], out defaultLoad))
            {
                throw new ArgumentException(StringResources.Error_InvalidMetricSpecification);
            }

            desc = new StatelessServiceLoadMetricDescription()
            {
                Name = metricFields[0],
                Weight = ParseWeight(metricFields[1]),
                DefaultLoad = defaultLoad
            };

            return desc;
        }

        internal static ServiceLoadMetricWeight ParseWeight(string weightStr)
        {
            if (string.Compare(weightStr, "High", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return ServiceLoadMetricWeight.High;
            }
            else if (string.Compare(weightStr, "Medium", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return ServiceLoadMetricWeight.Medium;
            }
            else if (string.Compare(weightStr, "Low", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return ServiceLoadMetricWeight.Low;
            }
            else if (string.Compare(weightStr, "Zero", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return ServiceLoadMetricWeight.Zero;
            }
            else
            {
                throw new ArgumentException(StringResources.Error_InvalidMetricSpecification);
            }
        }

        internal static ServiceCorrelationDescription ParseCorrelation(string correlationStr)
        {
            string[] correlationFields = correlationStr.Split(',', ' ');

            if (correlationFields.Length != 2)
            {
                throw new ArgumentException(StringResources.Error_InvalidCorrelationSpecification);
            }

            return new ServiceCorrelationDescription()
            {
                ServiceName = new Uri(correlationFields[0]),
                Scheme = ParseCorrelationScheme(correlationFields[1])
            };
        }

        internal static ServiceCorrelationScheme ParseCorrelationScheme(string correlationSchemeStr)
        {
            if (string.Compare(correlationSchemeStr, "Affinity", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return ServiceCorrelationScheme.Affinity;
            }
            else if (string.Compare(correlationSchemeStr, "AlignedAffinity", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return ServiceCorrelationScheme.AlignedAffinity;
            }
            else if (string.Compare(correlationSchemeStr, "NonAlignedAffinity", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return ServiceCorrelationScheme.NonAlignedAffinity;
            }
            else
            {
                throw new ArgumentException(StringResources.Error_InvalidCorrelationSpecification);
            }
        }

        internal static ServicePlacementPolicyDescription ParsePlacementPolicy(string policyStr)
        {
            string[] policyFields = policyStr.Split(',', ' ');

            if (policyFields.Length == 0 || policyFields.Length > 2)
            {
                throw new ArgumentException(StringResources.Error_InvalidPlacementPolicySpecification);
            }

            if (string.Compare(policyFields[0], "InvalidDomain", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return new ServicePlacementInvalidDomainPolicyDescription()
                {
                    DomainName = policyFields[1]
                };
            }
            else if (string.Compare(policyFields[0], "RequiredDomain", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return new ServicePlacementRequiredDomainPolicyDescription()
                {
                    DomainName = policyFields[1]
                };
            }
            else if (string.Compare(policyFields[0], "PreferredPrimaryDomain", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return new ServicePlacementPreferPrimaryDomainPolicyDescription()
                {
                    DomainName = policyFields[1]
                };
            }
            else if (string.Compare(policyFields[0], "RequiredDomainDistribution", StringComparison.OrdinalIgnoreCase) == 0)
            {
                if (policyFields.Length != 1)
                {
                    throw new ArgumentException(StringResources.Error_InvalidPlacementPolicySpecification);
                }

                return new ServicePlacementRequireDomainDistributionPolicyDescription()
                {
                };
            }
            else if (string.Compare(policyFields[0], "NonPartiallyPlace", StringComparison.OrdinalIgnoreCase) == 0)
            {
                if (policyFields.Length != 1)
                {
                    throw new ArgumentException(StringResources.Error_InvalidPlacementPolicySpecification);
                }

                return new ServicePlacementNonPartiallyPlaceServicePolicyDescription()
                {
                };
            }
            else
            {
                throw new ArgumentException(StringResources.Error_InvalidPlacementPolicySpecification);
            }
        }

        internal static MoveCost ParseMoveCost(string moveCostStr)
        {
            if (string.Compare(moveCostStr, "High", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return MoveCost.High;
            }
            else if (string.Compare(moveCostStr, "Medium", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return MoveCost.Medium;
            }
            else if (string.Compare(moveCostStr, "Low", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return MoveCost.Low;
            }
            else if (string.Compare(moveCostStr, "Zero", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return MoveCost.Zero;
            }
            else
            {
                throw new ArgumentException(StringResources.Error_InvalidMoveCostSpecification);
            }
        }
        
        protected void CreateService(ServiceDescription serviceDescription)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.CreateServiceAsync(
                    serviceDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(this.FormatOutput(serviceDescription), true);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.CreateServiceErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void CreateServiceFromTemplate(
            Uri applicationName,
            Uri serviceName,
            string serviceDnsName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.CreateServiceFromTemplateAsync(
                    applicationName,
                    serviceName,
                    serviceDnsName,
                    serviceTypeName,
                    servicePackageActivationMode,
                    null,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(StringResources.Info_CreateServiceFromTemplateSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.CreateServiceErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void UpdateService(
            Uri serviceName,
            ServiceUpdateDescription updateDescription)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.UpdateServiceAsync(
                    serviceName,
                    updateDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(StringResources.Info_UpdateServiceSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.UpdateServiceErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void RemoveService(DeleteServiceDescription deleteDescription)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.DeleteServiceAsync(
                    deleteDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(StringResources.Info_RemoveServiceSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RemoveServiceErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected StatefulServiceUpdateDescription GetStatefulServiceUpdateDescription(
            int? targetReplicaSetSize,
            int? minReplicaSetSize,
            TimeSpan? replicaRestartWaitDuration,
            TimeSpan? quorumLossWaitDuration,
            TimeSpan? standByReplicaKeepDuration,
            string[] metrics,
            string[] correlations,
            string[] placementPolicies,
            string placementConstraints,
            string defaultMoveCost,
            string[] partitionNamesToAdd,
            string[] partitionNamesToRemove,
            List<ScalingPolicyDescription> scalingPolicies)
        {
            var statefulServiceUpdateDescription = new StatefulServiceUpdateDescription();

            if (targetReplicaSetSize.HasValue)
            {
                statefulServiceUpdateDescription.TargetReplicaSetSize = targetReplicaSetSize.Value;
            }

            if (minReplicaSetSize.HasValue)
            {
                statefulServiceUpdateDescription.MinReplicaSetSize = minReplicaSetSize.Value;
            }

            if (replicaRestartWaitDuration.HasValue)
            {
                statefulServiceUpdateDescription.ReplicaRestartWaitDuration = replicaRestartWaitDuration.Value;
            }

            if (quorumLossWaitDuration.HasValue)
            {
                statefulServiceUpdateDescription.QuorumLossWaitDuration = quorumLossWaitDuration.Value;
            }

            if (standByReplicaKeepDuration.HasValue)
            {
                statefulServiceUpdateDescription.StandByReplicaKeepDuration = standByReplicaKeepDuration.Value;
            }

            if (placementConstraints != null)
            {
                statefulServiceUpdateDescription.PlacementConstraints = placementConstraints;
            }

            if (metrics != null)
            {
                statefulServiceUpdateDescription.Metrics = new KeyedItemCollection<string, ServiceLoadMetricDescription>(d => d.Name);

                foreach (var str in metrics)
                {
                    statefulServiceUpdateDescription.Metrics.Add(ServiceCmdletBase.ParseStatefulMetric(str));
                }
            }

            if (defaultMoveCost != null)
            {
                statefulServiceUpdateDescription.DefaultMoveCost = ServiceCmdletBase.ParseMoveCost(defaultMoveCost);
            }

            if (correlations != null)
            {
                statefulServiceUpdateDescription.Correlations = new List<ServiceCorrelationDescription>();

                foreach (var str in correlations)
                {
                    statefulServiceUpdateDescription.Correlations.Add(ServiceCmdletBase.ParseCorrelation(str));
                }
            }

            if (placementPolicies != null)
            {
                statefulServiceUpdateDescription.PlacementPolicies = new List<ServicePlacementPolicyDescription>();

                foreach (var str in placementPolicies)
                {
                    statefulServiceUpdateDescription.PlacementPolicies.Add(ServiceCmdletBase.ParsePlacementPolicy(str));
                }
            }

            if ((partitionNamesToAdd != null && partitionNamesToAdd.Length > 0) || (partitionNamesToRemove != null && partitionNamesToRemove.Length > 0))
            {
                statefulServiceUpdateDescription.RepartitionDescription = new NamedRepartitionDescription(
                    partitionNamesToAdd, 
                    partitionNamesToRemove);
            }

            if (scalingPolicies != null)
            {
                statefulServiceUpdateDescription.ScalingPolicies = new List<ScalingPolicyDescription>();
                foreach (var scale in scalingPolicies)
                {
                    statefulServiceUpdateDescription.ScalingPolicies.Add(scale);
                }
            }

            return statefulServiceUpdateDescription;
        }

        protected StatelessServiceUpdateDescription GetStatelessServiceUpdateDescription(
            int? instanceCount,
            string[] metrics,
            string[] correlations,
            string[] placementPolicies,
            string placementConstraints,
            string defaultMoveCost,
            string[] partitionNamesToAdd,
            string[] partitionNamesToRemove,
            List<ScalingPolicyDescription> scalingPolicies)
        {
            var statelessServiceUpdateDescription = new StatelessServiceUpdateDescription();

            if (instanceCount.HasValue)
            {
                statelessServiceUpdateDescription.InstanceCount = instanceCount.Value;
            }

            if (placementConstraints != null)
            {
                statelessServiceUpdateDescription.PlacementConstraints = placementConstraints;
            }

            if (metrics != null)
            {
                statelessServiceUpdateDescription.Metrics = new KeyedItemCollection<string, ServiceLoadMetricDescription>(d => d.Name);

                foreach (var str in metrics)
                {
                    statelessServiceUpdateDescription.Metrics.Add(ServiceCmdletBase.ParseStatelessMetric(str));
                }
            }

            if (defaultMoveCost != null)
            {
                statelessServiceUpdateDescription.DefaultMoveCost = ServiceCmdletBase.ParseMoveCost(defaultMoveCost);
            }

            if (correlations != null)
            {
                statelessServiceUpdateDescription.Correlations = new List<ServiceCorrelationDescription>();

                foreach (var str in correlations)
                {
                    statelessServiceUpdateDescription.Correlations.Add(ServiceCmdletBase.ParseCorrelation(str));
                }
            }

            if (placementPolicies != null)
            {
                statelessServiceUpdateDescription.PlacementPolicies = new List<ServicePlacementPolicyDescription>();

                foreach (var str in placementPolicies)
                {
                    statelessServiceUpdateDescription.PlacementPolicies.Add(ServiceCmdletBase.ParsePlacementPolicy(str));
                }
            }

            if ((partitionNamesToAdd != null && partitionNamesToAdd.Length > 0) || (partitionNamesToRemove != null && partitionNamesToRemove.Length > 0))
            {
                statelessServiceUpdateDescription.RepartitionDescription = new NamedRepartitionDescription(
                    partitionNamesToAdd,
                    partitionNamesToRemove);
            }

            if (scalingPolicies != null)
            {
                statelessServiceUpdateDescription.ScalingPolicies = new List<ScalingPolicyDescription>();
                foreach (var scale in scalingPolicies)
                {
                    statelessServiceUpdateDescription.ScalingPolicies.Add(scale);
                }
            }

            return statelessServiceUpdateDescription;
        }

        protected ServiceGroupUpdateDescription GetServiceGroupUpdateDescription(
            ServiceUpdateDescription updateDescription)
        {
            return new ServiceGroupUpdateDescription(updateDescription);
        }

        protected ResolvedServicePartition ResolveService(
            bool partitionKindUniformInt64,
            bool partitionKindNamed,
            bool partitionKindSingleton,
            Uri serviceName,
            string partitionKey,
            ResolvedServicePartition previousResult,
            TimeSpan timeout)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                if (partitionKindUniformInt64)
                {
                    long partitionKeyLong;
                    if (!long.TryParse(partitionKey, out partitionKeyLong))
                    {
                        throw new ArgumentException(StringResources.Error_InvalidPartitionKey);
                    }

                    return clusterConnection.ResolveServicePartitionAsync(
                               serviceName,
                               partitionKeyLong,
                               previousResult,
                               timeout,
                               this.GetCancellationToken()).Result;
                }
                else if (partitionKindNamed)
                {
                    return clusterConnection.ResolveServicePartitionAsync(
                               serviceName,
                               partitionKey,
                               previousResult,
                               timeout,
                               this.GetCancellationToken()).Result;
                }
                else if (partitionKindSingleton)
                {
                    return clusterConnection.ResolveServicePartitionAsync(
                               serviceName,
                               previousResult,
                               timeout,
                               this.GetCancellationToken()).Result;
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.ResolveServiceErrorId,
                        clusterConnection);
                    return true;
                });
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.ResolveServiceErrorId,
                    clusterConnection);
            }

            return null;
        }

        protected override object FormatOutput(object output)
        {
            var result = new PSObject(output);

            if (output is ServiceDescription)
            {
                var serviceDescription = output as ServiceDescription;

                result.Properties.Add(
                    new PSAliasProperty(
                        Constants.ServiceKindPropertyName,
                        Constants.KindPropertyName));

                result.Properties.Add(
                    new PSNoteProperty(
                        Constants.PartitionSchemePropertyName,
                        serviceDescription.PartitionSchemeDescription.Scheme));

                result.Properties.Add(
                    new PSNoteProperty(
                        Constants.PlacementConstraintsPropertyName,
                        string.IsNullOrEmpty(serviceDescription.PlacementConstraints) ? Constants.NoneResultOutput : serviceDescription.PlacementConstraints));

                PSObject loadMetricsPropertyPSObj;
                PSObject correlationPropertyPSObj;
                PSObject placementPoliciesPSObj;
                PSObject scalingPoliciesPSObj;

                if (serviceDescription.Kind == ServiceDescriptionKind.Stateful)
                {
                    var description = serviceDescription as StatefulServiceDescription;
                    loadMetricsPropertyPSObj = new PSObject(description.Metrics);
                    correlationPropertyPSObj = new PSObject(description.Correlations);
                }
                else
                {
                    var description = serviceDescription as StatelessServiceDescription;
                    loadMetricsPropertyPSObj = new PSObject(description.Metrics);
                    correlationPropertyPSObj = new PSObject(description.Correlations);
                }

                scalingPoliciesPSObj = new PSObject(serviceDescription.ScalingPolicies);
                scalingPoliciesPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                result.Properties.Add(
                    new PSNoteProperty(
                        Constants.ScalingPoliciesPropertyName,
                        scalingPoliciesPSObj));

                placementPoliciesPSObj = new PSObject(serviceDescription.PlacementPolicies);
                placementPoliciesPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                result.Properties.Add(
                    new PSNoteProperty(
                        Constants.PlacementPoliciesPropertyName,
                        placementPoliciesPSObj));

                loadMetricsPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                result.Properties.Add(
                    new PSNoteProperty(
                        Constants.LoadMetricsPropertyName,
                        loadMetricsPropertyPSObj));

                result.Properties.Add(
                    new PSNoteProperty(
                        Constants.IsDefaultMoveCostSpecifiedPropertyName,
                        serviceDescription.IsDefaultMoveCostSpecified));

                result.Properties.Add(
                    new PSNoteProperty(
                        Constants.DefaultMoveCostPropertyName,
                        (!serviceDescription.IsDefaultMoveCostSpecified) ? Constants.NoneResultOutput : base.FormatOutput(serviceDescription.DefaultMoveCost)));

                correlationPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                result.Properties.Add(
                    new PSNoteProperty(
                        Constants.ServiceCorrelationPropertyName,
                        correlationPropertyPSObj));

                if (serviceDescription.PartitionSchemeDescription.Scheme == PartitionScheme.UniformInt64Range)
                {
                    var info = serviceDescription.PartitionSchemeDescription as UniformInt64RangePartitionSchemeDescription;
                    if (info != null)
                    {
                        result.Properties.Add(new PSNoteProperty(Constants.PartitionLowKeyPropertyName, info.LowKey));
                        result.Properties.Add(new PSNoteProperty(Constants.PartitionHighKeyPropertyName, info.HighKey));
                        result.Properties.Add(new PSNoteProperty(Constants.PartitionCountPropertyName, info.PartitionCount));
                    }
                }
                else if (serviceDescription.PartitionSchemeDescription.Scheme == PartitionScheme.Named)
                {
                    var info = serviceDescription.PartitionSchemeDescription as NamedPartitionSchemeDescription;
                    if (info != null)
                    {
                        var partitionNamesPropertyPSObj = new PSObject(info.PartitionNames);
                        partitionNamesPropertyPSObj.Members.Add(
                            new PSCodeMethod(
                                Constants.ToStringMethodName,
                                typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                        result.Properties.Add(new PSNoteProperty(Constants.PartitionNamesPropertyName, partitionNamesPropertyPSObj));
                    }
                }

                return result;
            }

            return base.FormatOutput(output);
        }

        public class ServiceDescriptionBuilder
        {
            public ServiceDescriptionBuilder(
                bool isStateful,
                Uri applicationName,
                PartitionSchemeDescriptionBuilder partitionSchemeDescriptionBuilder,
                Uri serviceName,
                string serviceTypeName,
                string placementConstraints,
                string[] metrics,
                string[] correlations,
                string[] placementPolicies,
                string defaultMoveCost,
                ServicePackageActivationMode servicePackageActivationMode,
                string serviceDnsName,
                List<ScalingPolicyDescription> scalingPolicies)
            {
                this.IsStateful = isStateful;
                this.ApplicationName = applicationName;
                this.PartitionSchemeDescriptionBuilder = partitionSchemeDescriptionBuilder;
                this.ServiceName = serviceName;
                this.ServiceTypeName = serviceTypeName;
                this.PlacementConstraints = placementConstraints;
                this.Metrics = metrics;
                this.Correlations = correlations;
                this.PlacementPolicies = placementPolicies;
                this.DefaultMoveCost = defaultMoveCost;
                this.ServicePackageActivationMode = servicePackageActivationMode;
                this.ServiceDnsName = serviceDnsName;
                this.ScalingPolicies = scalingPolicies;
            }

            protected ServiceDescription ServiceDescription
            {
                get;
                set;
            }

            protected bool IsStateful
            {
                get;
                set;
            }

            protected Uri ApplicationName
            {
                get;
                set;
            }

            protected PartitionSchemeDescriptionBuilder PartitionSchemeDescriptionBuilder
            {
                get;
                set;
            }

            protected Uri ServiceName
            {
                get;
                set;
            }

            protected string ServiceTypeName
            {
                get;
                set;
            }

            protected string PlacementConstraints
            {
                get;
                set;
            }

            protected string[] Metrics
            {
                get;
                set;
            }

            protected string[] Correlations
            {
                get;
                set;
            }

            protected string[] PlacementPolicies
            {
                get;
                set;
            }

            protected string DefaultMoveCost
            {
                get;
                set;
            }

            protected ServicePackageActivationMode ServicePackageActivationMode
            {
                get;
                set;
            }

            protected string ServiceDnsName
            {
                get;
                set;
            }

            protected List<ScalingPolicyDescription> ScalingPolicies
            {
                get;
                set;
            }

            public virtual ServiceDescription Build()
            {
                this.ServiceDescription.ApplicationName = this.ApplicationName;
                this.ServiceDescription.PartitionSchemeDescription = this.PartitionSchemeDescriptionBuilder.Build();
                this.ServiceDescription.ServiceName = this.ServiceName;
                this.ServiceDescription.ServiceTypeName = this.ServiceTypeName;
                this.ServiceDescription.PlacementConstraints = this.PlacementConstraints;
                this.ServiceDescription.ServiceDnsName = this.ServiceDnsName;

                if (this.Correlations != null)
                {
                    foreach (var str in this.Correlations)
                    {
                        this.ServiceDescription.Correlations.Add(ServiceCmdletBase.ParseCorrelation(str));
                    }
                }

                if (this.PlacementPolicies != null)
                {
                    foreach (var str in this.PlacementPolicies)
                    {
                        this.ServiceDescription.PlacementPolicies.Add(ServiceCmdletBase.ParsePlacementPolicy(str));
                    }
                }

                if (this.DefaultMoveCost != null)
                {
                    this.ServiceDescription.DefaultMoveCost = ServiceCmdletBase.ParseMoveCost(this.DefaultMoveCost);
                }

                if (this.Metrics != null)
                {
                    foreach (var str in this.Metrics)
                    {
                        this.ServiceDescription.Metrics.Add(ServiceCmdletBase.ParseStatefulMetric(str));
                    }
                }

                this.ServiceDescription.ServicePackageActivationMode = this.ServicePackageActivationMode;
                
                return this.ServiceDescription;
            }
        }

        public class StatelessServiceDescriptionBuilder : ServiceDescriptionBuilder
        {
            public StatelessServiceDescriptionBuilder(
                Uri applicationName,
                PartitionSchemeDescriptionBuilder partitionSchemeDescriptionBuilder,
                int instanceCount,
                Uri serviceName,
                string serviceTypeName,
                string placementConstraints,
                string[] metrics,
                string[] correlations,
                string[] placementPolicies,
                string defaultMoveCost,
                ServicePackageActivationMode servicePackageActivationMode,
                string serviceDnsName,
                List<ScalingPolicyDescription> scalingPolicies)
            : base(
                  false, 
                  applicationName, 
                  partitionSchemeDescriptionBuilder, 
                  serviceName, 
                  serviceTypeName, 
                  placementConstraints,
                  metrics, 
                  correlations, 
                  placementPolicies, 
                  defaultMoveCost,
                  servicePackageActivationMode,
                  serviceDnsName,
                  scalingPolicies)
            {
                this.InstanceCount = instanceCount;
            }

            private int InstanceCount
            {
                get;
                set;
            }

            public override ServiceDescription Build()
            {
                var statelessDescription = new StatelessServiceDescription
                {
                    InstanceCount = this.InstanceCount
                };

                statelessDescription.ApplicationName = this.ApplicationName;
                statelessDescription.PartitionSchemeDescription = this.PartitionSchemeDescriptionBuilder.Build();
                statelessDescription.ServiceName = this.ServiceName;
                statelessDescription.ServiceTypeName = this.ServiceTypeName;
                statelessDescription.PlacementConstraints = this.PlacementConstraints;
                statelessDescription.ServiceDnsName = this.ServiceDnsName;

                if (this.Correlations != null)
                {
                    foreach (var str in this.Correlations)
                    {
                        statelessDescription.Correlations.Add(ServiceCmdletBase.ParseCorrelation(str));
                    }
                }

                if (this.PlacementPolicies != null)
                {
                    foreach (var str in this.PlacementPolicies)
                    {
                        statelessDescription.PlacementPolicies.Add(ServiceCmdletBase.ParsePlacementPolicy(str));
                    }
                }

                if (this.DefaultMoveCost != null)
                {
                    statelessDescription.DefaultMoveCost = ServiceCmdletBase.ParseMoveCost(this.DefaultMoveCost);
                }

                statelessDescription.ServicePackageActivationMode = this.ServicePackageActivationMode;
                
                if (this.Metrics != null)
                {
                    foreach (var str in this.Metrics)
                    {
                        statelessDescription.Metrics.Add(ServiceCmdletBase.ParseStatelessMetric(str));
                    }
                }

                if (this.ScalingPolicies != null)
                {
                    foreach (var scale in this.ScalingPolicies)
                    {
                        statelessDescription.ScalingPolicies.Add(scale);
                    }
                }

                return statelessDescription;
            }
        }

        public class StatefulServiceDescriptionBuilder : ServiceDescriptionBuilder
        {
            public StatefulServiceDescriptionBuilder(
                Uri applicationName,
                PartitionSchemeDescriptionBuilder partitionSchemeDescriptionBuilder,
                bool hasPersistentState,
                int targetReplicaSetSize,
                int minReplicaSetSize,
                Uri serviceName,
                string serviceTypeName,
                string placementConstraints,
                string[] metrics,
                string[] correlations,
                string[] placementPolicies,
                TimeSpan? replicaRestartWaitDuration,
                TimeSpan? quorumLossWaitDuration,
                TimeSpan? standByReplicaKeepDuration,
                string defaultMoveCost,
                ServicePackageActivationMode servicePackageActivationMode,
                string serviceDnsName,
                List<ScalingPolicyDescription> scalingPolicies)
            : base(
                  true, 
                  applicationName, 
                  partitionSchemeDescriptionBuilder, 
                  serviceName, 
                  serviceTypeName, 
                  placementConstraints, 
                  metrics, 
                  correlations, 
                  placementPolicies, 
                  defaultMoveCost,
                  servicePackageActivationMode,
                  serviceDnsName,
                  scalingPolicies)
            {
                this.HasPersistentState = hasPersistentState;
                this.TargetReplicaSetSize = targetReplicaSetSize;
                this.MinReplicaSetSize = minReplicaSetSize;
                this.ReplicaRestartWaitDuration = replicaRestartWaitDuration;
                this.QuorumLossWaitDuration = quorumLossWaitDuration;
                this.StandByReplicaKeepDuration = standByReplicaKeepDuration;
            }

            private bool HasPersistentState
            {
                get;
                set;
            }

            private int TargetReplicaSetSize
            {
                get;
                set;
            }

            private int MinReplicaSetSize
            {
                get;
                set;
            }

            private TimeSpan? ReplicaRestartWaitDuration
            {
                get;
                set;
            }

            private TimeSpan? QuorumLossWaitDuration
            {
                get;
                set;
            }

            private TimeSpan? StandByReplicaKeepDuration
            {
                get;
                set;
            }

            public override ServiceDescription Build()
            {
                var statefulDescription = new StatefulServiceDescription
                {
                    HasPersistedState = this.HasPersistentState,
                    TargetReplicaSetSize = this.TargetReplicaSetSize,
                    MinReplicaSetSize = this.MinReplicaSetSize
                };

                statefulDescription.ApplicationName = this.ApplicationName;
                statefulDescription.PartitionSchemeDescription = this.PartitionSchemeDescriptionBuilder.Build();
                statefulDescription.ServiceName = this.ServiceName;
                statefulDescription.ServiceTypeName = this.ServiceTypeName;
                statefulDescription.PlacementConstraints = this.PlacementConstraints;
                statefulDescription.ServiceDnsName = this.ServiceDnsName;

                if (this.ReplicaRestartWaitDuration.HasValue)
                {
                    statefulDescription.ReplicaRestartWaitDuration = this.ReplicaRestartWaitDuration.Value;
                }

                if (this.QuorumLossWaitDuration.HasValue)
                {
                    statefulDescription.QuorumLossWaitDuration = this.QuorumLossWaitDuration.Value;
                }

                if (this.StandByReplicaKeepDuration.HasValue)
                {
                    statefulDescription.StandByReplicaKeepDuration = this.StandByReplicaKeepDuration.Value;
                }

                if (this.Correlations != null)
                {
                    foreach (var str in this.Correlations)
                    {
                        statefulDescription.Correlations.Add(ServiceCmdletBase.ParseCorrelation(str));
                    }
                }

                if (this.PlacementPolicies != null)
                {
                    foreach (var str in this.PlacementPolicies)
                    {
                        statefulDescription.PlacementPolicies.Add(ServiceCmdletBase.ParsePlacementPolicy(str));
                    }
                }

                if (this.DefaultMoveCost != null)
                {
                    statefulDescription.DefaultMoveCost = ServiceCmdletBase.ParseMoveCost(this.DefaultMoveCost);
                }

                statefulDescription.ServicePackageActivationMode = this.ServicePackageActivationMode;

                if (this.Metrics != null)
                {
                    foreach (var str in this.Metrics)
                    {
                        statefulDescription.Metrics.Add(ServiceCmdletBase.ParseStatefulMetric(str));
                    }
                }

                if (this.ScalingPolicies != null)
                {
                    foreach (var scale in this.ScalingPolicies)
                    {
                        statefulDescription.ScalingPolicies.Add(scale);
                    }
                }

                return statefulDescription;
            }
        }

        public class PartitionSchemeDescriptionBuilder
        {
            public virtual PartitionSchemeDescription Build()
            {
                return null;
            }
        }

        public class SingletonPartitionSchemeDescriptionBuilder : PartitionSchemeDescriptionBuilder
        {
            public override PartitionSchemeDescription Build()
            {
                return new SingletonPartitionSchemeDescription();
            }
        }

        public class UniformInt64RangePartitionSchemeDescriptionBuilder : PartitionSchemeDescriptionBuilder
        {
            public UniformInt64RangePartitionSchemeDescriptionBuilder(int partitionCount, long lowKey, long highKey)
            {
                this.PartitionCount = partitionCount;
                this.LowKey = lowKey;
                this.HighKey = highKey;
            }

            private int PartitionCount
            {
                get;
                set;
            }

            private long LowKey
            {
                get;
                set;
            }

            private long HighKey
            {
                get;
                set;
            }

            public override PartitionSchemeDescription Build()
            {
                return new UniformInt64RangePartitionSchemeDescription
                {
                    PartitionCount = this.PartitionCount,
                    LowKey = this.LowKey,
                    HighKey = this.HighKey
                };
            }
        }

        public class NamedPartitionSchemeDescriptionBuilder : PartitionSchemeDescriptionBuilder
        {
            public NamedPartitionSchemeDescriptionBuilder(string[] partitionNames)
            {
                this.PartitionNames = partitionNames;
            }

            private string[] PartitionNames
            {
                get;
                set;
            }

            public override PartitionSchemeDescription Build()
            {
                var namedPartitionDescription = new NamedPartitionSchemeDescription();
                foreach (var name in this.PartitionNames)
                {
                    namedPartitionDescription.PartitionNames.Add(name);
                }

                return namedPartitionDescription;
            }
        }
    }
}