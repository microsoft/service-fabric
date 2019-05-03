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
    using System.Fabric.Strings;
    using System.Management.Automation;

    public abstract class ServiceGroupCmdletBase : CommonCmdletBase
    {
        protected void CreateServiceGroup(ServiceGroupDescription serviceGroupDescription)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.CreateServiceGroupAsync(
                    serviceGroupDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(this.FormatOutput(serviceGroupDescription), true);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.CreateServiceGroupErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void CreateServiceGroupFromTemplate(
            Uri applicationName,
            Uri serviceName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.CreateServiceGroupFromTemplateAsync(
                    applicationName,
                    serviceName,
                    serviceTypeName,
                    servicePackageActivationMode,
                    null,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(StringResources.Info_CreateServiceGroupFromTemplateSucceeded);
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

        protected void UpdateServiceGroup(
            Uri serviceName,
            ServiceGroupUpdateDescription updateDescription)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.UpdateServiceGroupAsync(
                    serviceName,
                    updateDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(StringResources.Info_UpdateServiceGroupSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.UpdateServiceGroupErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void RemoveServiceGroup(Uri serviceName)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.DeleteServiceGroupAsync(
                    serviceName,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(StringResources.Info_RemoveServiceGroupSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RemoveServiceGroupErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            var result = new PSObject(output);

            if (output is ServiceGroupDescription)
            {
                var serviceGroupDescription = output as ServiceGroupDescription;

                // Adding a helper toString method for printing IList<MemberDescription>
                var memberDescriptionsPSObj = new PSObject(serviceGroupDescription.MemberDescriptions);
                memberDescriptionsPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));
                result.Properties.Add(new PSNoteProperty(Constants.MemberDescriptionsPropertyName, memberDescriptionsPSObj));

                result.Properties.Add(new PSNoteProperty(
                                          Constants.ApplicationNamePropertyName,
                                          serviceGroupDescription.ServiceDescription.ApplicationName));
                result.Properties.Add(new PSNoteProperty(
                                          Constants.ServiceNamePropertyName,
                                          serviceGroupDescription.ServiceDescription.ServiceName));
                result.Properties.Add(new PSNoteProperty(
                                          Constants.ServiceTypeNamePropertyName,
                                          serviceGroupDescription.ServiceDescription.ServiceTypeName));
                result.Properties.Add(new PSNoteProperty(
                                          Constants.ServiceKindPropertyName,
                                          serviceGroupDescription.ServiceDescription.Kind));
                if (serviceGroupDescription.ServiceDescription is StatefulServiceDescription)
                {
                    StatefulServiceDescription statefulServiceDescription = serviceGroupDescription.ServiceDescription as StatefulServiceDescription;
                    result.Properties.Add(new PSNoteProperty(
                                              Constants.HasPersistedStatePropertyName,
                                              statefulServiceDescription.HasPersistedState));
                    result.Properties.Add(new PSNoteProperty(
                                              Constants.MinReplicaSetSizePropertyName,
                                              statefulServiceDescription.MinReplicaSetSize));
                    result.Properties.Add(new PSNoteProperty(
                                              Constants.TargetReplicaSetSizePropertyName,
                                              statefulServiceDescription.TargetReplicaSetSize));

                    if (statefulServiceDescription.HasPersistedState)
                    {
                        result.Properties.Add(new PSNoteProperty(
                                                  Constants.ReplicaRestartWaitDurationPropertyName,
                                                  statefulServiceDescription.ReplicaRestartWaitDuration));
                        result.Properties.Add(new PSNoteProperty(
                                                  Constants.QuorumLossWaitDurationPropertyName,
                                                  statefulServiceDescription.QuorumLossWaitDuration));
                        result.Properties.Add(new PSNoteProperty(
                                                  Constants.StandByReplicaKeepDurationPropertyName,
                                                  statefulServiceDescription.StandByReplicaKeepDuration));
                    }
                }
                else if (serviceGroupDescription.ServiceDescription is StatelessServiceDescription)
                {
                    StatelessServiceDescription statelessServiceDescription = serviceGroupDescription.ServiceDescription as StatelessServiceDescription;
                    result.Properties.Add(new PSNoteProperty(
                                              Constants.InstanceCountPropertyName,
                                              statelessServiceDescription.InstanceCount));
                }

                result.Properties.Add(
                    new PSNoteProperty(
                        Constants.PartitionSchemePropertyName,
                        serviceGroupDescription.ServiceDescription.PartitionSchemeDescription.Scheme));
                result.Properties.Add(
                    new PSNoteProperty(
                        Constants.PlacementConstraintsPropertyName,
                        string.IsNullOrEmpty(serviceGroupDescription.ServiceDescription.PlacementConstraints) ? Constants.NoneResultOutput : serviceGroupDescription.ServiceDescription.PlacementConstraints));

                if (serviceGroupDescription.ServiceDescription.PartitionSchemeDescription.Scheme == PartitionScheme.UniformInt64Range)
                {
                    var info = serviceGroupDescription.ServiceDescription.PartitionSchemeDescription as UniformInt64RangePartitionSchemeDescription;
                    if (info != null)
                    {
                        result.Properties.Add(new PSNoteProperty(Constants.PartitionLowKeyPropertyName, info.LowKey));
                        result.Properties.Add(new PSNoteProperty(Constants.PartitionHighKeyPropertyName, info.HighKey));
                        result.Properties.Add(new PSNoteProperty(Constants.PartitionCountPropertyName, info.PartitionCount));
                    }
                }
                else if (serviceGroupDescription.ServiceDescription.PartitionSchemeDescription.Scheme == PartitionScheme.Named)
                {
                    var info = serviceGroupDescription.ServiceDescription.PartitionSchemeDescription as NamedPartitionSchemeDescription;
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

        public class ServiceGroupDescriptionBuilder : ServiceCmdletBase.ServiceDescriptionBuilder
        {
            public ServiceGroupDescriptionBuilder(
                bool isStateful,
                Uri applicationName,
                ServiceCmdletBase.PartitionSchemeDescriptionBuilder partitionSchemeDescriptionBuilder,
                Uri serviceName,
                string serviceTypeName,
                string placementConstraints,
                string[] metrics,
                string[] correlations,
                string[] placementPolicies,
                Hashtable[] memberDescriptions,
                ServicePackageActivationMode servicePackageActivationMode) :
            base(
                isStateful,
                applicationName,
                partitionSchemeDescriptionBuilder,
                serviceName,
                serviceTypeName,
                placementConstraints, 
                metrics, 
                correlations, 
                placementPolicies, 
                null,
                servicePackageActivationMode,
                string.Empty,
                null)
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
                this.MemberDescriptions = memberDescriptions;
            }

            protected ServiceGroupDescription ServiceGroupDescription
            {
                get;
                set;
            }

            protected Hashtable[] MemberDescriptions
            {
                get;
                set;
            }

            /*
             * This method is only used to create ServiceGroupDescription, and is not related to the
             * ServiceDescriptionBuilder:Build().
             */
            public new virtual ServiceGroupDescription Build()
            {
                this.ServiceGroupDescription.ServiceDescription.ApplicationName = this.ApplicationName;
                this.ServiceGroupDescription.ServiceDescription.PartitionSchemeDescription = this.PartitionSchemeDescriptionBuilder.Build();
                this.ServiceGroupDescription.ServiceDescription.ServiceName = this.ServiceName;
                this.ServiceGroupDescription.ServiceDescription.ServiceTypeName = this.ServiceTypeName;
                this.ServiceGroupDescription.ServiceDescription.PlacementConstraints = this.PlacementConstraints;

                if (this.Correlations != null)
                {
                    foreach (var str in this.Correlations)
                    {
                        this.ServiceGroupDescription.ServiceDescription.Correlations.Add(ServiceCmdletBase.ParseCorrelation(str));
                    }
                }

                if (this.PlacementPolicies != null)
                {
                    foreach (var str in this.PlacementPolicies)
                    {
                        this.ServiceGroupDescription.ServiceDescription.PlacementPolicies.Add(ServiceCmdletBase.ParsePlacementPolicy(str));
                    }
                }

                if (this.MemberDescriptions != null)
                {
                    foreach (var memberDescription in this.MemberDescriptions)
                    {
                        this.ServiceGroupDescription.MemberDescriptions.Add(this.ParseMemberDescription(memberDescription));
                    }
                }

                this.ServiceGroupDescription.ServiceDescription.ServicePackageActivationMode = this.ServicePackageActivationMode;

                return this.ServiceGroupDescription;
            }

            protected ServiceGroupMemberDescription ParseMemberDescription(Hashtable memberDescriptionHashtable)
            {
                ServiceGroupMemberDescription serviceGroupMemberDescription;

                if (!memberDescriptionHashtable.ContainsKey(Constants.ServiceNamePropertyName) ||
                    !memberDescriptionHashtable.ContainsKey(Constants.ServiceTypeNamePropertyName))
                {
                    throw new ArgumentException(StringResources.Error_InvalidMemberDescriptionSpecification);
                }

                serviceGroupMemberDescription = new ServiceGroupMemberDescription()
                {
                    ServiceName = new Uri((string)memberDescriptionHashtable[Constants.ServiceNamePropertyName]),
                    ServiceTypeName = (string)memberDescriptionHashtable[Constants.ServiceTypeNamePropertyName]
                };

                if (memberDescriptionHashtable.ContainsKey(Constants.LoadMetricsPropertyName))
                {
                    if (this.IsStateful)
                    {
                        serviceGroupMemberDescription.Metrics.Add(ServiceCmdletBase.ParseStatefulMetric((string)memberDescriptionHashtable[Constants.LoadMetricsPropertyName]));
                    }
                    else
                    {
                        serviceGroupMemberDescription.Metrics.Add(ServiceCmdletBase.ParseStatelessMetric((string)memberDescriptionHashtable[Constants.LoadMetricsPropertyName]));
                    }
                }

                return serviceGroupMemberDescription;
            }
        }

        public class StatelessServiceGroupDescriptionBuilder : ServiceGroupDescriptionBuilder
        {
            public StatelessServiceGroupDescriptionBuilder(
                Uri applicationName,
                ServiceCmdletBase.PartitionSchemeDescriptionBuilder partitionSchemeDescriptionBuilder,
                int instanceCount,
                Uri serviceName,
                string serviceTypeName,
                string placementConstraints,
                string[] metrics,
                string[] correlations,
                string[] placementPolicies,
                Hashtable[] memberDescriptions,
                ServicePackageActivationMode servicePackageActivationMode)
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
                memberDescriptions,
                servicePackageActivationMode)
            {
                this.InstanceCount = instanceCount;
            }

            private int InstanceCount
            {
                get;
                set;
            }

            public override ServiceGroupDescription Build()
            {
                this.ServiceGroupDescription = new ServiceGroupDescription();
                this.ServiceGroupDescription.ServiceDescription = new StatelessServiceDescription
                {
                    InstanceCount = this.InstanceCount
                };

                if (this.Metrics != null)
                {
                    foreach (var str in this.Metrics)
                    {
                        this.ServiceGroupDescription.ServiceDescription.Metrics.Add(ServiceCmdletBase.ParseStatelessMetric(str));
                    }
                }

                return base.Build();
            }
        }

        public class StatefulServiceGroupDescriptionBuilder : ServiceGroupDescriptionBuilder
        {
            public StatefulServiceGroupDescriptionBuilder(
                Uri applicationName,
                ServiceCmdletBase.PartitionSchemeDescriptionBuilder partitionSchemeDescriptionBuilder,
                bool hasPersistentState,
                int targetReplicaSetSize,
                int minReplicaSetSize,
                Uri serviceName,
                string serviceTypeName,
                string placementConstraints,
                string[] metrics,
                string[] correlations,
                string[] placementPolicies,
                Hashtable[] memberDescriptions,
                TimeSpan? replicaRestartWaitDuration,
                TimeSpan? quorumLossWaitDuration,
                ServicePackageActivationMode servicePackageActivationMode)
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
                memberDescriptions,
                servicePackageActivationMode)
            {
                this.HasPersistentState = hasPersistentState;
                this.TargetReplicaSetSize = targetReplicaSetSize;
                this.MinReplicaSetSize = minReplicaSetSize;
                this.ReplicaRestartWaitDuration = replicaRestartWaitDuration;
                this.QuorumLossWaitDuration = quorumLossWaitDuration;
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

            public override ServiceGroupDescription Build()
            {
                this.ServiceGroupDescription = new ServiceGroupDescription();
                var statefulServiceDescription = new StatefulServiceDescription
                {
                    HasPersistedState = this.HasPersistentState,
                    TargetReplicaSetSize = this.TargetReplicaSetSize,
                    MinReplicaSetSize = this.MinReplicaSetSize
                };

                if (this.ReplicaRestartWaitDuration.HasValue)
                {
                    statefulServiceDescription.ReplicaRestartWaitDuration = this.ReplicaRestartWaitDuration.Value;
                }

                if (this.QuorumLossWaitDuration.HasValue)
                {
                    statefulServiceDescription.QuorumLossWaitDuration = this.QuorumLossWaitDuration.Value;
                }

                this.ServiceGroupDescription.ServiceDescription = statefulServiceDescription;

                if (this.Metrics != null)
                {
                    foreach (var str in this.Metrics)
                    {
                        this.ServiceGroupDescription.ServiceDescription.Metrics.Add(ServiceCmdletBase.ParseStatefulMetric(str));
                    }
                }

                return base.Build();
            }
        }
    }
}
