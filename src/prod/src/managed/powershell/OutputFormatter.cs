// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Collections.Specialized;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Fabric.Security;
    using System.Globalization;
    using System.Management.Automation;
    using System.Text;

    [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1305:FieldNamesMustNotUseHungarianNotation", Justification = "Reviewed. Suppression is OK here for psObject.")]
    public class OutputFormatter
    {
        private static readonly DateTime DateTimeFileTimeUtcZero = DateTime.FromFileTimeUtc(0);

        private static readonly Dictionary<Type, Delegate> TypeToMethod = new Dictionary<Type, Delegate>()
        {
            { typeof(ApplicationParameterList), new Func<ApplicationParameterList, string>(ToString) },
            { typeof(NameValueCollection), new Func<NameValueCollection, string>(ToString) },
            { typeof(ReadOnlyCollection<UpgradeDomainStatus>), new Func<ReadOnlyCollection<UpgradeDomainStatus>, string>(ToString) },
            { typeof(ApplicationHealthPolicyMap), new Func<IDictionary<Uri, ApplicationHealthPolicy>, string>(ToString) },
            { typeof(CodePackageEntryPoint), new Func<CodePackageEntryPoint, string>(ToString) },
            { typeof(KeyedCollection<string, ServiceLoadMetricDescription>), new Func<KeyedCollection<string, ServiceLoadMetricDescription>, string>(ToString) },
            { typeof(KeyedItemCollection<string, ServiceLoadMetricDescription>), new Func<KeyedCollection<string, ServiceLoadMetricDescription>, string>(ToString) },
            { typeof(List<ServiceCorrelationDescription>), new Func<IList<ServiceCorrelationDescription>, string>(ToString) },
            { typeof(List<ServicePlacementPolicyDescription>), new Func<IList<ServicePlacementPolicyDescription>, string>(ToString) },
            { typeof(List<string>), new Func<IList<string>, string>(ToString) },
            { typeof(ItemList<string>), new Func<IList<string>, string>(ToString) },
            { typeof(ReadOnlyCollection<string>), new Func<IList<string>, string>(ToString) },
            { typeof(Dictionary<string, string>), new Func<IDictionary<string, string>, string>(ToString) },
            { typeof(ApplicationHealthStateList), new Func<IList<ApplicationHealthState>, string>(ToString) },
            { typeof(ServiceHealthStateList), new Func<IList<ServiceHealthState>, string>(ToString) },
            { typeof(PartitionHealthStateList), new Func<IList<PartitionHealthState>, string>(ToString) },
            { typeof(ReplicaHealthStateList), new Func<IList<ReplicaHealthState>, string>(ToString) },
            { typeof(NodeHealthStateList), new Func<IList<NodeHealthState>, string>(ToString) },
            { typeof(DeployedApplicationHealthStateList), new Func<IList<DeployedApplicationHealthState>, string>(ToString) },
            { typeof(DeployedServicePackageHealthStateList), new Func<IList<DeployedServicePackageHealthState>, string>(ToString) },
            { typeof(List<HealthEvent>), new Func<IList<HealthEvent>, string>(ToString) },
            { typeof(List<ResolvedServiceEndpoint>), new Func<ICollection<ResolvedServiceEndpoint>, string>(ToString) },
            { typeof(Dictionary<string, ServiceTypeHealthPolicy>), new Func<IDictionary<string, ServiceTypeHealthPolicy>, string>(ToString) },
            { typeof(FabricClientSettings), new Func<FabricClientSettings, string>(ToString) },
            { typeof(GatewayInformation), new Func<GatewayInformation, string>(ToString) },
            { typeof(AzureActiveDirectoryMetadata), new Func<AzureActiveDirectoryMetadata, string>(ToString) },
            { typeof(ItemList<ServiceGroupMemberDescription>), new Func<IList<ServiceGroupMemberDescription>, string>(ToString) },
            { typeof(List<HealthEvaluation>), new Func<IList<HealthEvaluation>, string>(ToString) },
            { typeof(List<LoadMetricInformation>), new Func<IList<LoadMetricInformation>, string>(ToString) },
            { typeof(List<LoadMetricReport>), new Func<IList<LoadMetricReport>, string>(ToString) },
            { typeof(ReplicatorQueueStatus), new Func<ReplicatorQueueStatus, string>(ToString) },
            { typeof(PrimaryReplicatorStatus), new Func<PrimaryReplicatorStatus, string>(ToString) },
            { typeof(SecondaryReplicatorStatus), new Func<SecondaryReplicatorStatus, string>(ToString) },
            { typeof(List<RemoteReplicatorStatus>), new Func<IList<RemoteReplicatorStatus>, string>(ToString) },
            { typeof(KeyValueStoreReplicaStatus), new Func<KeyValueStoreReplicaStatus, string>(ToString) },
            { typeof(UpgradeDomainProgress), new Func<UpgradeDomainProgress, string>(ToString) },
            { typeof(List<NodeLoadMetricInformation>), new Func<IList<NodeLoadMetricInformation>, string>(ToString) },
            { typeof(ServiceGroupMemberMemberList), new Func<ServiceGroupMemberMemberList, string>(ToString) },
            { typeof(NodeDeactivationResult), new Func<NodeDeactivationResult, string>(ToString) },
            { typeof(ApplicationTypeHealthPolicyMap), new Func<ApplicationTypeHealthPolicyMap, string>(ToString) },
            { typeof(List<ApplicationLoadMetricInformation>), new Func<IList<ApplicationLoadMetricInformation>, string>(ToString) },
            { typeof(NodeResult), new Func<NodeResult, string>(ToString) },
            { typeof(SelectedReplica), new Func<SelectedReplica, string>(ToString) },
            { typeof(SelectedPartition), new Func<SelectedPartition, string>(ToString) },
            { typeof(NodeHealthStateChunkList), new Func<NodeHealthStateChunkList, string>(ToString) },
            { typeof(ApplicationHealthStateChunkList), new Func<ApplicationHealthStateChunkList, string>(ToString) },
            { typeof(List<ChaosEvent>), new Func<List<ChaosEvent>, string>(ToString) },
            { typeof(HealthStatistics), new Func<HealthStatistics, string>(ToString) },
            { typeof(ApplicationParameter), new Func<ApplicationParameter, string>(ToString) },
            { typeof(ReconfigurationInformation), new Func<ReconfigurationInformation, string>(ToString) },
            { typeof(DeployedStatefulServiceReplica), new Func<DeployedStatefulServiceReplica, string>(ToString) },
            { typeof(DeployedStatelessServiceInstance), new Func<DeployedStatelessServiceInstance, string>(ToString) },
            { typeof(List<ChaosScheduleJob>), new Func<List<ChaosScheduleJob>, string>(ToString) },
            { typeof(Dictionary<string, ChaosParameters>), new Func<Dictionary<string, ChaosParameters>, string>(ToString) },
            { typeof(List<ScalingPolicyDescription>), new Func<IList<ScalingPolicyDescription>, string>(ToString) }
        };

        public static string FormatObject(PSObject psObject)
        {
            var type = psObject.BaseObject.GetType();
            if (TypeToMethod.ContainsKey(type))
            {
                return TypeToMethod[type].DynamicInvoke(psObject.BaseObject) as string;
            }

            return psObject.BaseObject.ToString();
        }

        private static string ToString(CodePackageEntryPoint codePackageEntryPoint)
        {
            StringBuilder sb = new StringBuilder();

            sb.Append(Environment.NewLine);

            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.EntryPointStatusPropertyName, codePackageEntryPoint.EntryPointStatus));
            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.CodePackageInstanceIdPropertyName, codePackageEntryPoint.CodePackageInstanceId));

            if (codePackageEntryPoint.EntryPointStatus == EntryPointStatus.Started)
            {
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.EntryPointLocationPropertyName, codePackageEntryPoint.EntryPointLocation));
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.ProcessIdPropertyName, codePackageEntryPoint.ProcessId));
            }

            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.RunAsUserNamePropertyName, codePackageEntryPoint.RunAsUserName));

            if (codePackageEntryPoint.EntryPointStatus == EntryPointStatus.Pending)
            {
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.NextActivationUtcPropertyName, codePackageEntryPoint.NextActivationUtc));
            }

            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.ActivationCountPropertyName, codePackageEntryPoint.Statistics.ActivationCount));
            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.ActivationFailureCountPropertyName, codePackageEntryPoint.Statistics.ActivationFailureCount));
            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.ContinuousActivationFailureCountPropertyName, codePackageEntryPoint.Statistics.ContinuousActivationFailureCount));
            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.ContinuousExitFailureCountPropertyName, codePackageEntryPoint.Statistics.ContinuousExitFailureCount));
            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.ExitCountPropertyName, codePackageEntryPoint.Statistics.ExitCount));
            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.ExitFailureCountPropertyName, codePackageEntryPoint.Statistics.ExitFailureCount));
            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.LastActivationUtcPropertyName, GetLastUtcOutput(codePackageEntryPoint.Statistics.LastActivationUtc)));
            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.LastExitCodePropertyName, codePackageEntryPoint.Statistics.LastExitCode));
            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.LastExitUtcPropertyName, GetLastUtcOutput(codePackageEntryPoint.Statistics.LastExitUtc)));
            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.LastSuccessfulActivationUtcPropertyName, GetLastUtcOutput(codePackageEntryPoint.Statistics.LastSuccessfulActivationUtc)));
            sb.Append(Environment.NewLine);
            if ((codePackageEntryPoint.Statistics.LastSuccessfulExitUtc != DateTimeFileTimeUtcZero) ||
                (codePackageEntryPoint.Statistics.LastExitUtc != DateTimeFileTimeUtcZero))
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-32} : {1}", Constants.LastSuccessfulExitUtcPropertyName, GetLastUtcOutput(codePackageEntryPoint.Statistics.LastSuccessfulExitUtc)));
            }

            return sb.ToString();
        }

        private static object GetLastUtcOutput(DateTime utcTime)
        {
            if (utcTime == DateTimeFileTimeUtcZero)
            {
                return Constants.NeverOutput;
            }
            else
            {
                return utcTime;
            }
        }

        private static string ToString(IList<string> list)
        {
            if (list.Count == 0)
            {
                return "{}";
            }
            else
            {
                StringBuilder sb = new StringBuilder();
                if (list.Count == 1)
                {
                    sb.Append("{");
                    sb.Append(list[0]);
                    sb.Append("}");
                }
                else
                {
                    sb.Append("{");
                    for (int i = 0; i < list.Count - 1; ++i)
                    {
                        sb.Append(list[i]);
                        sb.Append(", ");
                    }

                    sb.Append(list[list.Count - 1]);
                    sb.Append("}");
                }

                return sb.ToString();
            }
        }

        private static string ToString(ApplicationParameter parameter)
        {
            var returnString = new StringBuilder();

            returnString.AppendFormat("\"{0}\" = \"{1}\"{2}", parameter.Name, parameter.Value, Environment.NewLine);

            return returnString.ToString();
        }

        private static string ToString(ApplicationParameterList parameters)
        {
            if (parameters.Count == 0)
            {
                return "{}";
            }
            else
            {
                StringBuilder sb = new StringBuilder();
                sb.Append("{ ");

                for (int i = 0; i < parameters.Count - 1; ++i)
                {
                    sb.AppendLine(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            "\"{0}\" = \"{1}\";",
                            parameters[i].Name,
                            parameters[i].Value));
                }

                sb.Append(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "\"{0}\" = \"{1}\"",
                        parameters[parameters.Count - 1].Name,
                        parameters[parameters.Count - 1].Value));

                sb.Append(" }");
                return sb.ToString();
            }
        }

        private static string ToString(NameValueCollection parameters)
        {
            if (parameters.Count == 0)
            {
                return "{}";
            }

            StringBuilder sb = new StringBuilder();
            sb.Append("{ ");

            string name;
            string value;
            for (int i = 0; i < parameters.Count - 1; ++i)
            {
                name = parameters.GetKey(i);
                value = parameters[name];
                sb.AppendLine(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "\"{0}\" = \"{1}\";",
                        name,
                        value));
            }

            name = parameters.GetKey(parameters.Count - 1);
            value = parameters[name];
            sb.Append(
                string.Format(
                    CultureInfo.InvariantCulture,
                    "\"{0}\" = \"{1}\"",
                    name,
                    value));

            sb.Append(" }");
            return sb.ToString();
        }

        private static string ToString(ReadOnlyCollection<UpgradeDomainStatus> upgradeDomains)
        {
            if (upgradeDomains.Count == 0)
            {
                return "{}";
            }

            StringBuilder sb = new StringBuilder();
            sb.Append("{ ");

            for (int i = 0; i < upgradeDomains.Count - 1; ++i)
            {
                sb.AppendLine(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "\"{0}\" = \"{1}\";",
                        upgradeDomains[i].Name,
                        upgradeDomains[i].State));
            }

            sb.Append(
                string.Format(
                    CultureInfo.InvariantCulture,
                    "\"{0}\" = \"{1}\"",
                    upgradeDomains[upgradeDomains.Count - 1].Name,
                    upgradeDomains[upgradeDomains.Count - 1].State));

            sb.Append(" }");
            return sb.ToString();
        }

        private static string ToString(IDictionary<string, string> extensions)
        {
            if (extensions.Count == 0)
            {
                return "{}";
            }

            StringBuilder sb = new StringBuilder();
            sb.Append("{ ");

            var count = 0;
            foreach (var key in extensions.Keys)
            {
                sb.AppendLine(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        (count == extensions.Count - 1) ? "\"{0}\" = \"{1}\"" : "\"{0}\" = \"{1}\";",
                        key,
                        extensions[key]));
            }

            sb.Append(" }");
            return sb.ToString();
        }

#pragma warning disable 618

        private static string ToString(KeyedCollection<string, ServiceLoadMetricDescription> loadMetrics)
        {
            if (loadMetrics.Count == 0)
            {
                return "{}";
            }

            StringBuilder sb = new StringBuilder();
            sb.Append("{ ");

            var count = 0;
            foreach (var item in loadMetrics)
            {
                if (item is StatefulServiceLoadMetricDescription)
                {
                    if (count == loadMetrics.Count - 1)
                    {
                        sb.Append(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                "\"{0},{1},{2},{3}\"",
                                item.Name,
                                item.Weight,
                                (item as StatefulServiceLoadMetricDescription).PrimaryDefaultLoad,
                                (item as StatefulServiceLoadMetricDescription).SecondaryDefaultLoad));
                    }
                    else
                    {
                        sb.AppendLine(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                "\"{0},{1},{2},{3}\";",
                                item.Name,
                                item.Weight,
                                (item as StatefulServiceLoadMetricDescription).PrimaryDefaultLoad,
                                (item as StatefulServiceLoadMetricDescription).SecondaryDefaultLoad));
                    }
                }
                else if (item is StatelessServiceLoadMetricDescription)
                {
                    if (count == loadMetrics.Count - 1)
                    {
                        sb.Append(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                "\"{0},{1},{2}\"",
                                item.Name,
                                item.Weight,
                                (item as StatelessServiceLoadMetricDescription).DefaultLoad));
                    }
                    else
                    {
                        sb.AppendLine(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                "\"{0},{1},{2}\";",
                                item.Name,
                                item.Weight,
                                (item as StatelessServiceLoadMetricDescription).DefaultLoad));
                    }
                }
                else
                {
                    if (count == loadMetrics.Count - 1)
                    {
                        sb.Append(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                "\"{0},{1},{2},{3}\"",
                                item.Name,
                                item.Weight,
                                (item as ServiceLoadMetricDescription).PrimaryDefaultLoad,
                                (item as ServiceLoadMetricDescription).SecondaryDefaultLoad));
                    }
                    else
                    {
                        sb.AppendLine(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                "\"{0},{1},{2},{3}\";",
                                item.Name,
                                item.Weight,
                                (item as ServiceLoadMetricDescription).PrimaryDefaultLoad,
                                (item as ServiceLoadMetricDescription).SecondaryDefaultLoad));
                    }
                }

                count++;
            }

            sb.Append(" }");
            return sb.ToString();
        }

#pragma warning restore 618

        private static string ToString(IList<ServiceCorrelationDescription> serviceCorrelationDescriptions)
        {
            if (serviceCorrelationDescriptions.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            StringBuilder sb = new StringBuilder();
            sb.Append("{ ");

            foreach (var item in serviceCorrelationDescriptions)
            {
                sb.Append(Environment.NewLine);
                sb.Append(OutputFormatter.ToString(item));
            }

            sb.Append(" }");
            return sb.ToString();
        }

        private static string ToString(IList<ServicePlacementPolicyDescription> servicePlacementPolicyDescriptions)
        {
            if (servicePlacementPolicyDescriptions.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            StringBuilder sb = new StringBuilder();

            foreach (var item in servicePlacementPolicyDescriptions)
            {
                sb.Append(Environment.NewLine);
                sb.Append(OutputFormatter.ToString(item));
            }

            return sb.ToString();
        }

        private static string ToString(IList<DeployedApplicationHealthState> deployedApplicationHealthStates)
        {
            if (deployedApplicationHealthStates.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var healthStatesBuilder = new StringBuilder();
            foreach (var healthState in deployedApplicationHealthStates)
            {
                healthStatesBuilder.Append(Environment.NewLine);
                healthStatesBuilder.Append(OutputFormatter.ToString(healthState));
            }

            return healthStatesBuilder.ToString();
        }

        private static string ToString(DeployedApplicationHealthState healthState)
        {
            var healthStateBuilder = new StringBuilder();

            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ApplicationNamePropertyName, healthState.ApplicationName));
            healthStateBuilder.Append(Environment.NewLine);
            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.NodeNamePropertyName, healthState.NodeName));
            healthStateBuilder.Append(Environment.NewLine);
            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.AggregatedHealthStatePropertyName, healthState.AggregatedHealthState));
            healthStateBuilder.Append(Environment.NewLine);

            return healthStateBuilder.ToString();
        }

        private static string ToString(IList<ServiceHealthState> serviceHealthStates)
        {
            if (serviceHealthStates.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var healthStatesBuilder = new StringBuilder();
            foreach (var healthState in serviceHealthStates)
            {
                healthStatesBuilder.Append(Environment.NewLine);
                healthStatesBuilder.Append(OutputFormatter.ToString(healthState));
            }

            return healthStatesBuilder.ToString();
        }

        private static string ToString(ServiceHealthState healthState)
        {
            var healthStateBuilder = new StringBuilder();

            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ServiceNamePropertyName, healthState.ServiceName));
            healthStateBuilder.Append(Environment.NewLine);
            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.AggregatedHealthStatePropertyName, healthState.AggregatedHealthState));
            healthStateBuilder.Append(Environment.NewLine);

            return healthStateBuilder.ToString();
        }

        private static string ToString(IList<PartitionHealthState> partitionHealthStates)
        {
            if (partitionHealthStates.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var healthStatesBuilder = new StringBuilder();
            foreach (var healthState in partitionHealthStates)
            {
                healthStatesBuilder.Append(Environment.NewLine);
                healthStatesBuilder.Append(OutputFormatter.ToString(healthState));
            }

            return healthStatesBuilder.ToString();
        }

        private static string ToString(PartitionHealthState healthState)
        {
            var healthStateBuilder = new StringBuilder();

            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.PartitionIdPropertyName, healthState.PartitionId));
            healthStateBuilder.Append(Environment.NewLine);
            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.AggregatedHealthStatePropertyName, healthState.AggregatedHealthState));
            healthStateBuilder.Append(Environment.NewLine);

            return healthStateBuilder.ToString();
        }

        private static string ToString(IList<ReplicaHealthState> replicaHealthStates)
        {
            if (replicaHealthStates.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var healthStatesBuilder = new StringBuilder();
            foreach (var healthState in replicaHealthStates)
            {
                healthStatesBuilder.Append(Environment.NewLine);
                healthStatesBuilder.Append(OutputFormatter.ToString(healthState));
            }

            return healthStatesBuilder.ToString();
        }

        private static string ToString(ReplicaHealthState healthState)
        {
            var healthStateBuilder = new StringBuilder();

            if (healthState is StatefulServiceReplicaHealthState)
            {
                var replicaHealthState = healthState as StatefulServiceReplicaHealthState;
                healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ReplicaIdPropertyName, replicaHealthState.ReplicaId));
                healthStateBuilder.Append(Environment.NewLine);
                healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.AggregatedHealthStatePropertyName, replicaHealthState.AggregatedHealthState));
                healthStateBuilder.Append(Environment.NewLine);
            }
            else if (healthState is StatelessServiceInstanceHealthState)
            {
                var instanceHealthState = healthState as StatelessServiceInstanceHealthState;
                healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.InstanceIdPropertyName, instanceHealthState.InstanceId));
                healthStateBuilder.Append(Environment.NewLine);
                healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.AggregatedHealthStatePropertyName, instanceHealthState.AggregatedHealthState));
                healthStateBuilder.Append(Environment.NewLine);
            }

            return healthStateBuilder.ToString();
        }

        private static string ToString(IList<DeployedServicePackageHealthState> deployedServicePackageHealthStates)
        {
            if (deployedServicePackageHealthStates.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var healthStatesBuilder = new StringBuilder();
            foreach (var healthState in deployedServicePackageHealthStates)
            {
                healthStatesBuilder.Append(Environment.NewLine);
                healthStatesBuilder.Append(OutputFormatter.ToString(healthState));
            }

            return healthStatesBuilder.ToString();
        }

        private static string ToString(DeployedServicePackageHealthState healthState)
        {
            var healthStateBuilder = new StringBuilder();

            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ServiceManifestNamePropertyName, healthState.ServiceManifestName));
            healthStateBuilder.Append(Environment.NewLine);
            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ServicePackageActivationIdPropertyName, healthState.ServicePackageActivationId));
            healthStateBuilder.Append(Environment.NewLine);
            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.NodeNamePropertyName, healthState.NodeName));
            healthStateBuilder.Append(Environment.NewLine);
            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.AggregatedHealthStatePropertyName, healthState.AggregatedHealthState));
            healthStateBuilder.Append(Environment.NewLine);

            return healthStateBuilder.ToString();
        }

        private static string ToString(IList<HealthEvent> healthEvents)
        {
            if (healthEvents.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var healthEventsBuilder = new StringBuilder();
            foreach (var healthEvent in healthEvents)
            {
                healthEventsBuilder.Append(Environment.NewLine);
                healthEventsBuilder.Append(OutputFormatter.ToString(healthEvent, string.Empty));
            }

            return healthEventsBuilder.ToString();
        }

        private static string ToString(HealthEvent healthEvent, string prefix)
        {
            var healthEventBuilder = new StringBuilder();

            healthEventBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", prefix, Constants.HealthEventSourceIdPropertyName, healthEvent.HealthInformation.SourceId));
            healthEventBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", prefix, Constants.HealthEventPropertyPropertyName, healthEvent.HealthInformation.Property));
            healthEventBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", prefix, Constants.HealthEventStatePropertyName, healthEvent.HealthInformation.HealthState));
            healthEventBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", prefix, Constants.HealthEventSequenceNumberPropertyName, healthEvent.HealthInformation.SequenceNumber));
            healthEventBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", prefix, Constants.HealthEventSourceUtcTimestampPropertyName, healthEvent.SourceUtcTimestamp));
            healthEventBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", prefix, Constants.HealthEventLastModifiedUtcTimestampPropertyName, healthEvent.LastModifiedUtcTimestamp));
            if (healthEvent.HealthInformation.TimeToLive == TimeSpan.MaxValue)
            {
                healthEventBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", prefix, Constants.HealthEventTimeToLivePropertyName, Constants.MaxTTLOutput));
            }
            else
            {
                healthEventBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", prefix, Constants.HealthEventTimeToLivePropertyName, healthEvent.HealthInformation.TimeToLive));
            }

            healthEventBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", prefix, Constants.HealthEventDescriptionPropertyName, healthEvent.HealthInformation.Description));
            healthEventBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", prefix, Constants.HealthEventRemoveWhenExpiredPropertyName, healthEvent.HealthInformation.RemoveWhenExpired));
            healthEventBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", prefix, Constants.HealthEventIsExpiredPropertyName, healthEvent.IsExpired));

            healthEventBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : ", prefix, Constants.HealthEventTransitions));

            var orderedHealthStates = new Tuple<HealthState, DateTime, string>[]
            {
                new Tuple<HealthState, DateTime, string>(HealthState.Ok, healthEvent.LastOkTransitionAt, Constants.HealthEventLastOkTransitionAt),
                new Tuple<HealthState, DateTime, string>(HealthState.Warning, healthEvent.LastWarningTransitionAt, Constants.HealthEventLastWarningTransitionAt),
                new Tuple<HealthState, DateTime, string>(HealthState.Error, healthEvent.LastErrorTransitionAt, Constants.HealthEventLastErrorTransitionAt)
            };
            Array.Sort(orderedHealthStates, (l, r) => DateTime.Compare(l.Item2, r.Item2));

            if (orderedHealthStates[1].Item2 == DateTimeFileTimeUtcZero)
            {
                // First transition, no other previous states
                healthEventBuilder.Append(string.Format(CultureInfo.CurrentCulture, "->{0} = {1}", orderedHealthStates[2].Item1, orderedHealthStates[2].Item2));
            }
            else
            {
                var previousHealthState = (orderedHealthStates[2].Item1 == healthEvent.HealthInformation.HealthState) ? orderedHealthStates[1] : orderedHealthStates[2];
                var currentHealthState = (orderedHealthStates[2].Item1 == healthEvent.HealthInformation.HealthState) ? orderedHealthStates[2] : orderedHealthStates[1];
                healthEventBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0}->{1} = {2}", previousHealthState.Item1, currentHealthState.Item1, currentHealthState.Item2));

                if (orderedHealthStates[0].Item2 != DateTimeFileTimeUtcZero)
                {
                    healthEventBuilder.Append(string.Format(CultureInfo.CurrentCulture, ", {0} = {1}", orderedHealthStates[0].Item3, orderedHealthStates[0].Item2));
                }
            }

            healthEventBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, string.Empty));

            return healthEventBuilder.ToString();
        }

        private static string ToString(ICollection<ResolvedServiceEndpoint> endPoints)
        {
            if (endPoints.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var endPointsBuilder = new StringBuilder();
            foreach (var endPoint in endPoints)
            {
                endPointsBuilder.Append(Environment.NewLine);
                endPointsBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.AddressPropertyName, endPoint.Address));
                endPointsBuilder.Append(Environment.NewLine);
                endPointsBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.RolePropertyName, endPoint.Role));
                endPointsBuilder.Append(Environment.NewLine);
            }

            return endPointsBuilder.ToString();
        }

        private static string ToString(IDictionary<string, ServiceTypeHealthPolicy> serviceTypeHealthPolicyMap)
        {
            if (serviceTypeHealthPolicyMap.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var sb = new StringBuilder();
            foreach (var entry in serviceTypeHealthPolicyMap)
            {
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ServiceTypeNamePropertyName, entry.Key));
                sb.Append(Environment.NewLine);
                sb.Append(
                    string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.MaxPercentUnhealthyPartitionsPerServicePropertyName, entry.Value.MaxPercentUnhealthyPartitionsPerService));
                sb.Append(Environment.NewLine);
                sb.Append(
                    string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.MaxPercentUnhealthyReplicasPerPartitionPropertyName, entry.Value.MaxPercentUnhealthyReplicasPerPartition));
                sb.Append(Environment.NewLine);
                sb.Append(
                    string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.MaxPercentUnhealthyServicesPropertyName, entry.Value.MaxPercentUnhealthyServices));
                sb.Append(Environment.NewLine);
            }

            return sb.ToString();
        }

        private static string ToString(HealthStateCount healthStateCount)
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0} {1}, {2} {3}, {4} {5}",
                healthStateCount.OkCount,
                Constants.OkPropertyName,
                healthStateCount.WarningCount,
                Constants.WarningPropertyName,
                healthStateCount.ErrorCount,
                Constants.ErrorPropertyName);
        }

        private static string ToString(HealthStatistics healthStats)
        {
            if (healthStats == null || healthStats.HealthStateCountList == null || healthStats.HealthStateCountList.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var sb = new StringBuilder();
            foreach (var entry in healthStats.HealthStateCountList)
            {
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", entry.EntityKind, OutputFormatter.ToString(entry.HealthStateCount)));
            }

            sb.Append(Environment.NewLine);
            return sb.ToString();
        }

        private static string ToString(ApplicationHealthStateChunkList chunks)
        {
            if (chunks.Count == 0 && chunks.TotalCount == 0)
            {
                return Constants.NoneResultOutput;
            }

            var builder = new StringBuilder();
            builder.Append(Environment.NewLine);
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.TotalCountPropertyName, chunks.TotalCount));

            foreach (var chunk in chunks)
            {
                builder.Append(Environment.NewLine);
                builder.Append(OutputFormatter.ToString(chunk));
            }

            return builder.ToString();
        }

        private static string ToString(ApplicationHealthStateChunk chunk)
        {
            var builder = new StringBuilder();

            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ApplicationNamePropertyName, chunk.ApplicationName));
            if (!string.IsNullOrEmpty(chunk.ApplicationTypeName))
            {
                builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ApplicationTypePropertyName, chunk.ApplicationTypeName));
            }

            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.HealthStatePropertyName, chunk.HealthState));

            if (chunk.ServiceHealthStateChunks.Count > 0)
            {
                builder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : ", Constants.ServiceHealthStateChunksPropertyName));
                builder.AppendLine(ToString(chunk.ServiceHealthStateChunks, 1));
            }

            if (chunk.DeployedApplicationHealthStateChunks.Count > 0)
            {
                builder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : ", Constants.DeployedApplicationHealthStateChunksPropertyName));
                builder.AppendLine(ToString(chunk.DeployedApplicationHealthStateChunks, 1));
            }

            return builder.ToString();
        }

        private static string ToString(ServiceHealthStateChunkList chunks, int indentLevel)
        {
            if (chunks.Count == 0 && chunks.TotalCount == 0)
            {
                return Constants.NoneResultOutput;
            }

            string padLeft = OutputFormatter.GetPadString(indentLevel);

            var builder = new StringBuilder();

            builder.Append(Environment.NewLine);
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.TotalCountPropertyName, chunks.TotalCount));

            foreach (var chunk in chunks)
            {
                builder.Append(Environment.NewLine);
                builder.Append(OutputFormatter.ToString(chunk, indentLevel));
            }

            return builder.ToString();
        }

        private static string ToString(ServiceHealthStateChunk chunk, int indentLevel)
        {
            var builder = new StringBuilder();

            string padLeft = OutputFormatter.GetPadString(indentLevel);

            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.ServiceNamePropertyName, chunk.ServiceName));
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.HealthStatePropertyName, chunk.HealthState));

            if (chunk.PartitionHealthStateChunks.Count > 0)
            {
                builder.Append(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : ", padLeft, Constants.PartitionHealthStateChunksPropertyName));
                builder.AppendLine(ToString(chunk.PartitionHealthStateChunks, indentLevel + 1));
            }

            return builder.ToString();
        }

        private static string ToString(PartitionHealthStateChunkList chunks, int indentLevel)
        {
            if (chunks.Count == 0 && chunks.TotalCount == 0)
            {
                return Constants.NoneResultOutput;
            }

            string padLeft = OutputFormatter.GetPadString(indentLevel);

            var builder = new StringBuilder();
            builder.Append(Environment.NewLine);
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.TotalCountPropertyName, chunks.TotalCount));

            foreach (var chunk in chunks)
            {
                builder.Append(Environment.NewLine);
                builder.Append(OutputFormatter.ToString(chunk, indentLevel));
            }

            return builder.ToString();
        }

        private static string ToString(PartitionHealthStateChunk chunk, int indentLevel)
        {
            var builder = new StringBuilder();

            string padLeft = OutputFormatter.GetPadString(indentLevel);

            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.PartitionIdPropertyName, chunk.PartitionId));
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.HealthStatePropertyName, chunk.HealthState));

            if (chunk.ReplicaHealthStateChunks.Count > 0)
            {
                builder.Append(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : ", padLeft, Constants.ReplicaHealthStateChunksPropertyName));
                builder.AppendLine(ToString(chunk.ReplicaHealthStateChunks, indentLevel + 1));
            }

            return builder.ToString();
        }

        private static string ToString(ReplicaHealthStateChunkList chunks, int indentLevel)
        {
            if (chunks.Count == 0 && chunks.TotalCount == 0)
            {
                return Constants.NoneResultOutput;
            }

            string padLeft = OutputFormatter.GetPadString(indentLevel);

            var builder = new StringBuilder();
            builder.Append(Environment.NewLine);
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.TotalCountPropertyName, chunks.TotalCount));

            foreach (var chunk in chunks)
            {
                builder.Append(Environment.NewLine);
                builder.Append(OutputFormatter.ToString(chunk, indentLevel));
            }

            return builder.ToString();
        }

        private static string ToString(ReplicaHealthStateChunk chunk, int indentLevel)
        {
            var builder = new StringBuilder();

            string padLeft = OutputFormatter.GetPadString(indentLevel);

            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.ReplicaOrInstanceIdPropertyName, chunk.ReplicaOrInstanceId));
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.HealthStatePropertyName, chunk.HealthState));

            return builder.ToString();
        }

        private static string ToString(DeployedApplicationHealthStateChunkList chunks, int indentLevel)
        {
            if (chunks.Count == 0 && chunks.TotalCount == 0)
            {
                return Constants.NoneResultOutput;
            }

            string padLeft = OutputFormatter.GetPadString(indentLevel);

            var builder = new StringBuilder();
            builder.Append(Environment.NewLine);
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.TotalCountPropertyName, chunks.TotalCount));

            foreach (var chunk in chunks)
            {
                builder.Append(Environment.NewLine);
                builder.Append(OutputFormatter.ToString(chunk, indentLevel));
            }

            return builder.ToString();
        }

        private static string ToString(DeployedApplicationHealthStateChunk chunk, int indentLevel)
        {
            var builder = new StringBuilder();

            string padLeft = OutputFormatter.GetPadString(indentLevel);

            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.NodeNamePropertyName, chunk.NodeName));
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.HealthStatePropertyName, chunk.HealthState));

            if (chunk.DeployedServicePackageHealthStateChunks.Count > 0)
            {
                builder.Append(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} :", padLeft, Constants.DeployedServicePackageHealthStateChunksPropertyName));
                builder.AppendLine(ToString(chunk.DeployedServicePackageHealthStateChunks, indentLevel + 1));
            }

            return builder.ToString();
        }

        private static string ToString(DeployedServicePackageHealthStateChunkList chunks, int indentLevel)
        {
            if (chunks.Count == 0 && chunks.TotalCount == 0)
            {
                return Constants.NoneResultOutput;
            }

            string padLeft = OutputFormatter.GetPadString(indentLevel);

            var builder = new StringBuilder();
            builder.Append(Environment.NewLine);
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.TotalCountPropertyName, chunks.TotalCount));

            foreach (var chunk in chunks)
            {
                builder.Append(Environment.NewLine);
                builder.Append(OutputFormatter.ToString(chunk, indentLevel));
            }

            return builder.ToString();
        }

        private static string ToString(DeployedServicePackageHealthStateChunk chunk, int indentLevel)
        {
            var builder = new StringBuilder();

            string padLeft = OutputFormatter.GetPadString(indentLevel);

            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.ServiceManifestNamePropertyName, chunk.ServiceManifestName));
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.ServicePackageActivationIdPropertyName, chunk.ServicePackageActivationId));
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1,-21} : {2}", padLeft, Constants.HealthStatePropertyName, chunk.HealthState));

            return builder.ToString();
        }

        private static string ToString(NodeHealthStateChunkList nodeHealthStateChunks)
        {
            if (nodeHealthStateChunks.Count == 0 && nodeHealthStateChunks.TotalCount == 0)
            {
                return Constants.NoneResultOutput;
            }

            var builder = new StringBuilder();
            builder.Append(Environment.NewLine);
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.TotalCountPropertyName, nodeHealthStateChunks.TotalCount));

            foreach (var chunk in nodeHealthStateChunks)
            {
                builder.Append(Environment.NewLine);
                builder.Append(OutputFormatter.ToString(chunk));
            }

            return builder.ToString();
        }

        private static string ToString(NodeHealthStateChunk chunk)
        {
            var builder = new StringBuilder();

            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.NodeNamePropertyName, chunk.NodeName));
            builder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.HealthStatePropertyName, chunk.HealthState));

            return builder.ToString();
        }

        private static string ToString(IList<NodeHealthState> nodeHealthStates)
        {
            if (nodeHealthStates.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var healthStatesBuilder = new StringBuilder();
            foreach (var healthState in nodeHealthStates)
            {
                healthStatesBuilder.Append(Environment.NewLine);
                healthStatesBuilder.Append(OutputFormatter.ToString(healthState));
            }

            return healthStatesBuilder.ToString();
        }

        private static string ToString(NodeHealthState healthState)
        {
            var healthStateBuilder = new StringBuilder();

            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.NodeNamePropertyName, healthState.NodeName));
            healthStateBuilder.Append(Environment.NewLine);
            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.AggregatedHealthStatePropertyName, healthState.AggregatedHealthState));
            healthStateBuilder.Append(Environment.NewLine);

            return healthStateBuilder.ToString();
        }

        private static string ToString(IList<ApplicationHealthState> applicationHealthStates)
        {
            if (applicationHealthStates.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var healthStatesBuilder = new StringBuilder();
            foreach (var healthState in applicationHealthStates)
            {
                healthStatesBuilder.Append(Environment.NewLine);
                healthStatesBuilder.Append(OutputFormatter.ToString(healthState));
            }

            return healthStatesBuilder.ToString();
        }

        private static string ToString(ApplicationHealthState healthState)
        {
            var healthStateBuilder = new StringBuilder();

            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ApplicationNamePropertyName, healthState.ApplicationName));
            healthStateBuilder.Append(Environment.NewLine);
            healthStateBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.AggregatedHealthStatePropertyName, healthState.AggregatedHealthState));
            healthStateBuilder.Append(Environment.NewLine);

            return healthStateBuilder.ToString();
        }

        private static string ToString(object obj)
        {
            var sb = new StringBuilder();

            sb.Append("{" + Environment.NewLine);
            if (obj != null)
            {
                foreach (var property in obj.GetType().GetProperties())
                {
                    sb.Append(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            "{0,-36} : {1}",
                            property.Name,
                            property.GetValue(obj, null)));
                    sb.Append(Environment.NewLine);
                }
            }

            sb.Append("}");
            return sb.ToString();
        }

        private static string ToString(IList<ServiceGroupMemberDescription> serviceGroupMemberDescriptions)
        {
            if (serviceGroupMemberDescriptions.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var memberDescriptionsStringBuilder = new StringBuilder();
            foreach (var memberDescription in serviceGroupMemberDescriptions)
            {
                memberDescriptionsStringBuilder.Append(Environment.NewLine);
                memberDescriptionsStringBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0} : {1}", Constants.ServiceNamePropertyName, memberDescription.ServiceName));
                memberDescriptionsStringBuilder.Append(Environment.NewLine);
                memberDescriptionsStringBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ServiceTypeNamePropertyName, memberDescription.ServiceTypeName));
                memberDescriptionsStringBuilder.Append(Environment.NewLine);
            }

            return memberDescriptionsStringBuilder.ToString();
        }

        private static string ToString(IList<HealthEvaluation> unhealthyEvaluations)
        {
            return OutputFormatter.ToString(unhealthyEvaluations, 0);
        }

        private static string ToString(IList<HealthEvaluation> unhealthyEvaluations, int indentLevel)
        {
            if (unhealthyEvaluations.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var healthEvaluationsBuilder = new StringBuilder();
            foreach (var healthEvaluation in unhealthyEvaluations)
            {
                healthEvaluationsBuilder.Append(Environment.NewLine);
                healthEvaluationsBuilder.Append(OutputFormatter.ToString(healthEvaluation, indentLevel));
            }

            return healthEvaluationsBuilder.ToString();
        }

        private static string ToString(HealthEvaluation healthEvaluation, int indentLevel)
        {
            var healthEvaluationBuilder = new StringBuilder();

            string padLeft = string.Empty;
            if (indentLevel > 0)
            {
                padLeft = new string('\t', indentLevel);
            }

            healthEvaluationBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1}", padLeft, healthEvaluation.Description));

            switch (healthEvaluation.Kind)
            {
                case HealthEvaluationKind.Event:
                    {
                        var eventEvaluation = healthEvaluation as EventHealthEvaluation;
                        healthEvaluationBuilder.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}{1}", padLeft, eventEvaluation.UnhealthyEvent.HealthInformation.Description));
                        break;
                    }

                case HealthEvaluationKind.Replicas:
                    {
                        var replicasEvaluation = healthEvaluation as ReplicasHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(replicasEvaluation.UnhealthyEvaluations, indentLevel));
                        break;
                    }

                case HealthEvaluationKind.Partitions:
                    {
                        var partitionsEvaluation = healthEvaluation as PartitionsHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(partitionsEvaluation.UnhealthyEvaluations, indentLevel));
                        break;
                    }

                case HealthEvaluationKind.DeployedServicePackages:
                    {
                        var deployedServicePackagesEvaluation = healthEvaluation as DeployedServicePackagesHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(deployedServicePackagesEvaluation.UnhealthyEvaluations, indentLevel));
                        break;
                    }

                case HealthEvaluationKind.DeployedApplications:
                    {
                        var deployedApplicationsEvaluation = healthEvaluation as DeployedApplicationsHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(deployedApplicationsEvaluation.UnhealthyEvaluations, indentLevel));
                        break;
                    }

                case HealthEvaluationKind.Services:
                    {
                        var servicesEvaluation = healthEvaluation as ServicesHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(servicesEvaluation.UnhealthyEvaluations, indentLevel));
                        break;
                    }

                case HealthEvaluationKind.Nodes:
                    {
                        var nodesEvaluation = healthEvaluation as NodesHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(nodesEvaluation.UnhealthyEvaluations, indentLevel));
                        break;
                    }

                case HealthEvaluationKind.Applications:
                    {
                        var applicationsEvaluation = healthEvaluation as ApplicationsHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(applicationsEvaluation.UnhealthyEvaluations, indentLevel));
                        break;
                    }

                case HealthEvaluationKind.ApplicationTypeApplications:
                    {
                        var applicationsEvaluation = healthEvaluation as ApplicationTypeApplicationsHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(applicationsEvaluation.UnhealthyEvaluations, indentLevel));
                        break;
                    }

                case HealthEvaluationKind.SystemApplication:
                    {
                        var systemApplicationEvaluation = healthEvaluation as SystemApplicationHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(systemApplicationEvaluation.UnhealthyEvaluations, indentLevel));
                        break;
                    }

                case HealthEvaluationKind.UpgradeDomainDeployedApplications:
                    {
                        var udDeployedApplicationsEvaluation = healthEvaluation as UpgradeDomainDeployedApplicationsHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(udDeployedApplicationsEvaluation.UnhealthyEvaluations, indentLevel));
                        break;
                    }

                case HealthEvaluationKind.UpgradeDomainNodes:
                    {
                        var udNodesEvaluation = healthEvaluation as UpgradeDomainNodesHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(udNodesEvaluation.UnhealthyEvaluations, indentLevel));
                        break;
                    }

                case HealthEvaluationKind.Node:
                    {
                        var nodeEvaluation = healthEvaluation as NodeHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(nodeEvaluation.UnhealthyEvaluations, indentLevel + 1));
                        break;
                    }

                case HealthEvaluationKind.Replica:
                    {
                        var replicaEvaluation = healthEvaluation as ReplicaHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(replicaEvaluation.UnhealthyEvaluations, indentLevel + 1));
                        break;
                    }

                case HealthEvaluationKind.Partition:
                    {
                        var partitionEvaluation = healthEvaluation as PartitionHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(partitionEvaluation.UnhealthyEvaluations, indentLevel + 1));
                        break;
                    }

                case HealthEvaluationKind.Service:
                    {
                        var serviceEvaluation = healthEvaluation as ServiceHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(serviceEvaluation.UnhealthyEvaluations, indentLevel + 1));
                        break;
                    }

                case HealthEvaluationKind.Application:
                    {
                        var applicationEvaluation = healthEvaluation as ApplicationHealthEvaluation;
                        healthEvaluationBuilder.AppendLine(OutputFormatter.ToString(applicationEvaluation.UnhealthyEvaluations, indentLevel + 1));
                        break;
                    }

                case HealthEvaluationKind.DeployedApplication:
                    {
                        var deployedApplicationEvaluation = healthEvaluation as DeployedApplicationHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(deployedApplicationEvaluation.UnhealthyEvaluations, indentLevel + 1));
                        break;
                    }

                case HealthEvaluationKind.DeployedServicePackage:
                    {
                        var deployedServicePackageEvaluation = healthEvaluation as DeployedServicePackageHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(deployedServicePackageEvaluation.UnhealthyEvaluations, indentLevel + 1));
                        break;
                    }

                case HealthEvaluationKind.DeltaNodesCheck:
                    {
                        var deltaNodesEvaluation = healthEvaluation as DeltaNodesCheckHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(deltaNodesEvaluation.UnhealthyEvaluations, indentLevel));
                        break;
                    }

                case HealthEvaluationKind.UpgradeDomainDeltaNodesCheck:
                    {
                        var deltaUdNodesEvaluation = healthEvaluation as UpgradeDomainDeltaNodesCheckHealthEvaluation;
                        healthEvaluationBuilder.Append(OutputFormatter.ToString(deltaUdNodesEvaluation.UnhealthyEvaluations, indentLevel));
                        break;
                    }
            }

            return healthEvaluationBuilder.ToString();
        }

        private static string ToString(IList<LoadMetricInformation> metrics)
        {
            if (metrics.Count == 0)
            {
                return "{}";
            }
            else
            {
                StringBuilder sb = new StringBuilder();

                for (int i = 0; i < metrics.Count; ++i)
                {
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationNamePropertyName, metrics[i].Name));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationIsBalancedBeforePropertyName, metrics[i].IsBalancedBefore));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationIsBalancedAfterPropertyName, metrics[i].IsBalancedAfter));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationDeviationBeforePropertyName, metrics[i].DeviationBefore));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationDeviationAfterPropertyName, metrics[i].DeviationAfter));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationBalancingThresholdPropertyName, metrics[i].BalancingThreshold));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationActionPropertyName, metrics[i].Action));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationActivityThresholdPropertyName, metrics[i].ActivityThreshold));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationClusterCapacityPropertyName, metrics[i].ClusterCapacity));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationClusterLoadPropertyName, metrics[i].CurrentClusterLoad));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationClusterRemainingCapacityPropertyName, metrics[i].ClusterCapacityRemaining));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationNodeBufferPercentagePropertyName, metrics[i].NodeBufferPercentage));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationClusterBufferedCapacityPropertyName, metrics[i].ClusterBufferedCapacity));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationClusterRemainingBufferedCapacityPropertyName, metrics[i].BufferedClusterCapacityRemaining));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationClusterCapacityViolationPropertyName, metrics[i].IsClusterCapacityViolation));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationMinNodeLoadValue, metrics[i].MinimumNodeLoad));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationMinNodeLoadNodeId, metrics[i].MinNodeLoadNodeId));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationMaxNodeLoadValue, metrics[i].MaximumNodeLoad));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.LoadMetricInformationMaxNodeLoadNodeId, metrics[i].MaxNodeLoadNodeId));
                    sb.Append(Environment.NewLine);
                }

                return sb.ToString();
            }
        }

        private static string ToString(IList<LoadMetricReport> loads)
        {
            if (loads.Count == 0)
            {
                return string.Empty;
            }

            StringBuilder sb = new StringBuilder();

            for (int i = 0; i < loads.Count; ++i)
            {
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-15} : {1}", Constants.LoadMetricsNamePropertyName, loads[i].Name));
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-15} : {1}", Constants.LoadMetricsValuePropertyName, loads[i].CurrentValue));
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-15} : {1}", Constants.LoadMetricsLastReportedUtcPropertyName, loads[i].LastReportedUtc));
                sb.Append(Environment.NewLine);
            }

            return sb.ToString();
        }

        private static string ToString(IList<RemoteReplicatorStatus> remoteReplicators)
        {
            if (remoteReplicators.Count == 0)
            {
                return string.Empty;
            }

            StringBuilder sb = new StringBuilder();

            for (int i = 0; i < remoteReplicators.Count; ++i)
            {
                sb.Append(Environment.NewLine);
                sb.AppendFormat(
                    CultureInfo.CurrentCulture,
                    "\t{0,-37} : {1}",
                    Constants.ReplicaIdPropertyName,
                    remoteReplicators[i].ReplicaId);
                sb.Append(Environment.NewLine);

                if (remoteReplicators[i].IsInBuild)
                {
                    if (remoteReplicators[i].RemoteReplicatorAcknowledgementStatus.CopyStreamAcknowledgementDetail.AverageReceiveDuration != TimeSpan.FromMilliseconds(-1))
                    {
                        sb.AppendFormat(
                            CultureInfo.CurrentCulture,
                            "\t{0:-37}",
                            Constants.CopyStreamAcknowledgementDetail);
                        sb.Append(Environment.NewLine);
                        sb.AppendFormat(
                            CultureInfo.CurrentCulture,
                            "\t\t{0,-37} : {1}",
                            Constants.AverageReceiveDuration,
                            remoteReplicators[i].RemoteReplicatorAcknowledgementStatus.CopyStreamAcknowledgementDetail.AverageReceiveDuration);
                        sb.Append(Environment.NewLine);
                        sb.AppendFormat(
                            CultureInfo.CurrentCulture,
                            "\t\t{0,-37} : {1}",
                            Constants.AverageApplyDuration,
                            remoteReplicators[i].RemoteReplicatorAcknowledgementStatus.CopyStreamAcknowledgementDetail.AverageApplyDuration);
                        sb.Append(Environment.NewLine);
                        sb.AppendFormat(
                            CultureInfo.CurrentCulture,
                            "\t\t{0,-37} : {1}",
                            Constants.NotReceivedCount,
                            remoteReplicators[i].RemoteReplicatorAcknowledgementStatus.CopyStreamAcknowledgementDetail.NotReceivedCount);
                        sb.Append(Environment.NewLine);
                        sb.AppendFormat(
                            CultureInfo.CurrentCulture,
                            "\t\t{0,-37} : {1}",
                            Constants.ReceivedAndNotAppliedCount,
                            remoteReplicators[i].RemoteReplicatorAcknowledgementStatus.CopyStreamAcknowledgementDetail.ReceivedAndNotAppliedCount);
                        sb.Append(Environment.NewLine);
                    }

                    sb.Append(string.Format(CultureInfo.CurrentCulture, "\t{0,-37} : {1}", Constants.LastReceivedCopySequenceNumberPropertyName, remoteReplicators[i].LastReceivedCopySequenceNumber));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "\t{0,-37} : {1}", Constants.LastAppliedCopySequenceNumberPropertyName, remoteReplicators[i].LastAppliedCopySequenceNumber));
                    sb.Append(Environment.NewLine);
                }
                else if (remoteReplicators[i].RemoteReplicatorAcknowledgementStatus.ReplicationStreamAcknowledgementDetail.AverageReceiveDuration !=
                         TimeSpan.FromMilliseconds(-1))
                {
                    sb.AppendFormat(
                        CultureInfo.CurrentCulture,
                        "\t{0:-37}",
                        Constants.ReplicationStreamAcknowledgementDetail);
                    sb.Append(Environment.NewLine);
                    sb.AppendFormat(
                        CultureInfo.CurrentCulture,
                        "\t\t{0,-37} : {1}",
                        Constants.AverageReceiveDuration,
                        remoteReplicators[i].RemoteReplicatorAcknowledgementStatus.ReplicationStreamAcknowledgementDetail.AverageReceiveDuration);
                    sb.Append(Environment.NewLine);
                    sb.AppendFormat(
                        CultureInfo.CurrentCulture,
                        "\t\t{0,-37} : {1}",
                        Constants.AverageApplyDuration,
                        remoteReplicators[i].RemoteReplicatorAcknowledgementStatus.ReplicationStreamAcknowledgementDetail.AverageApplyDuration);
                    sb.Append(Environment.NewLine);
                    sb.AppendFormat(
                        CultureInfo.CurrentCulture,
                        "\t\t{0,-37} : {1}",
                        Constants.NotReceivedCount,
                        remoteReplicators[i].RemoteReplicatorAcknowledgementStatus.ReplicationStreamAcknowledgementDetail.NotReceivedCount);
                    sb.Append(Environment.NewLine);
                    sb.AppendFormat(
                        CultureInfo.CurrentCulture,
                        "\t\t{0,-37} : {1}",
                        Constants.ReceivedAndNotAppliedCount,
                        remoteReplicators[i].RemoteReplicatorAcknowledgementStatus.ReplicationStreamAcknowledgementDetail.ReceivedAndNotAppliedCount);
                    sb.Append(Environment.NewLine);
                }

                if (remoteReplicators[i].LastAcknowledgementProcessedTimeUtc != DateTimeFileTimeUtcZero)
                {
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "\t{0,-37} : {1}", Constants.LastAcknowledgementProcessedTimeUtcPropertyName, remoteReplicators[i].LastAcknowledgementProcessedTimeUtc));
                }

                sb.Append(Environment.NewLine);
                sb.AppendFormat(
                    CultureInfo.CurrentCulture,
                    "\t{0,-37} : {1}",
                    Constants.LastReceivedReplicationSequenceNumberPropertyName,
                    remoteReplicators[i].LastReceivedReplicationSequenceNumber);
                sb.Append(Environment.NewLine);
                sb.AppendFormat(
                    CultureInfo.CurrentCulture,
                    "\t{0,-37} : {1}",
                    Constants.LastAppliedReplicationSequenceNumberPropertyName,
                    remoteReplicators[i].LastAppliedReplicationSequenceNumber);
                sb.Append(Environment.NewLine);
                sb.AppendFormat(
                    CultureInfo.CurrentCulture,
                    "\t{0,-37} : {1}",
                    Constants.IsInBuild,
                    remoteReplicators[i].IsInBuild);
                sb.Append(Environment.NewLine);
            }

            return sb.ToString();
        }

        private static string ToString(SecondaryReplicatorStatus secondaryStatus)
        {
            if (secondaryStatus == null)
            {
                return string.Empty;
            }

            StringBuilder sb = new StringBuilder();

            List<Tuple<string, long>> values = new List<Tuple<string, long>>
            {
                Tuple.Create(Constants.QueueUtilizationPercentagePropertyName, secondaryStatus.ReplicationQueueStatus.QueueUtilizationPercentage),
                Tuple.Create(Constants.QueueMemorySizePropertyName, secondaryStatus.ReplicationQueueStatus.QueueMemorySize),
                Tuple.Create(Constants.FirstSequenceNumberPropertyName, secondaryStatus.ReplicationQueueStatus.FirstSequenceNumber),
                Tuple.Create(Constants.CompletedSequenceNumberPropertyName, secondaryStatus.ReplicationQueueStatus.CompletedSequenceNumber),
                Tuple.Create(Constants.CommittedSequenceNumberPropertyName, secondaryStatus.ReplicationQueueStatus.CommittedSequenceNumber),
                Tuple.Create(Constants.LastSequenceNumberPropertyName, secondaryStatus.ReplicationQueueStatus.LastSequenceNumber),
            };

            string queueName = "ReplicationQueue";
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", queueName));

            foreach (var el in values)
            {
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "\t{0,-35} : {1}", el.Item1, el.Item2));
            }

            sb.Append(Environment.NewLine);
            sb.Append(Environment.NewLine);

            if (secondaryStatus.LastAcknowledgementSentTimeUtc != DateTimeFileTimeUtcZero)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-39} : {1}", Constants.LastAcknowledgementSentTimeUtcPropertyName, secondaryStatus.LastAcknowledgementSentTimeUtc));
                sb.Append(Environment.NewLine);
            }

            if (secondaryStatus.LastReplicationOperationReceivedTimeUtc != DateTimeFileTimeUtcZero)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-39} : {1}", Constants.LastReplicationOperationReceivedTimeUtcPropertyName, secondaryStatus.LastReplicationOperationReceivedTimeUtc));
                sb.Append(Environment.NewLine);
            }

            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-39} : {1}", Constants.IsInBuild, secondaryStatus.IsInBuild));

            if (secondaryStatus.IsInBuild)
            {
                sb.Append(Environment.NewLine);

                if (secondaryStatus.LastCopyOperationReceivedTimeUtc != DateTimeFileTimeUtcZero)
                {
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-39} : {1}", Constants.LastCopyOperationReceivedTimeUtcPropertyName, secondaryStatus.LastCopyOperationReceivedTimeUtc));
                    sb.Append(Environment.NewLine);
                }

                values = new List<Tuple<string, long>>
                {
                    Tuple.Create(Constants.QueueUtilizationPercentagePropertyName, secondaryStatus.CopyQueueStatus.QueueUtilizationPercentage),
                    Tuple.Create(Constants.QueueMemorySizePropertyName, secondaryStatus.CopyQueueStatus.QueueMemorySize),
                    Tuple.Create(Constants.FirstSequenceNumberPropertyName, secondaryStatus.CopyQueueStatus.FirstSequenceNumber),
                    Tuple.Create(Constants.CompletedSequenceNumberPropertyName, secondaryStatus.CopyQueueStatus.CompletedSequenceNumber),
                    Tuple.Create(Constants.CommittedSequenceNumberPropertyName, secondaryStatus.CopyQueueStatus.CommittedSequenceNumber),
                    Tuple.Create(Constants.LastSequenceNumberPropertyName, secondaryStatus.CopyQueueStatus.LastSequenceNumber),
                };

                sb.Append(Environment.NewLine);
                queueName = "CopyQueue";
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", queueName));

                foreach (var el in values)
                {
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "\t{0,-35} : {1}", el.Item1, el.Item2));
                }
            }

            return sb.ToString();
        }

        private static string ToString(PrimaryReplicatorStatus primaryStatus)
        {
            if (primaryStatus == null)
            {
                return string.Empty;
            }

            StringBuilder sb = new StringBuilder();

            List<Tuple<string, long>> values = new List<Tuple<string, long>>
            {
                Tuple.Create(Constants.QueueUtilizationPercentagePropertyName, primaryStatus.ReplicationQueueStatus.QueueUtilizationPercentage),
                Tuple.Create(Constants.QueueMemorySizePropertyName, primaryStatus.ReplicationQueueStatus.QueueMemorySize),
                Tuple.Create(Constants.FirstSequenceNumberPropertyName, primaryStatus.ReplicationQueueStatus.FirstSequenceNumber),
                Tuple.Create(Constants.CompletedSequenceNumberPropertyName, primaryStatus.ReplicationQueueStatus.CompletedSequenceNumber),
                Tuple.Create(Constants.CommittedSequenceNumberPropertyName, primaryStatus.ReplicationQueueStatus.CommittedSequenceNumber),
                Tuple.Create(Constants.LastSequenceNumberPropertyName, primaryStatus.ReplicationQueueStatus.LastSequenceNumber),
            };

            string queueName = "ReplicationQueue";
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", queueName));

            foreach (var el in values)
            {
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "\t{0,-37} : {1}", el.Item1, el.Item2));
            }

            sb.Append(Environment.NewLine);
            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-37} {1}", Constants.RemoteReplicatorsPropertyName, ToString(primaryStatus.RemoteReplicators)));

            return sb.ToString();
        }

        private static string ToString(ReplicatorQueueStatus replicatorQueueStatus)
        {
            if (replicatorQueueStatus == null)
            {
                return string.Empty;
            }

            StringBuilder sb = new StringBuilder();

            List<Tuple<string, long>> values = new List<Tuple<string, long>>
            {
                Tuple.Create(Constants.QueueUtilizationPercentagePropertyName, replicatorQueueStatus.QueueUtilizationPercentage),
                Tuple.Create(Constants.QueueMemorySizePropertyName, replicatorQueueStatus.QueueMemorySize),
                Tuple.Create(Constants.FirstSequenceNumberPropertyName, replicatorQueueStatus.FirstSequenceNumber),
                Tuple.Create(Constants.CompletedSequenceNumberPropertyName, replicatorQueueStatus.CompletedSequenceNumber),
                Tuple.Create(Constants.CommittedSequenceNumberPropertyName, replicatorQueueStatus.CommittedSequenceNumber),
                Tuple.Create(Constants.LastSequenceNumberPropertyName, replicatorQueueStatus.LastSequenceNumber),
            };

            string queueName = "ReplicationQueue";
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", queueName));

            foreach (var el in values)
            {
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "\t{0,-26} : {1}", el.Item1, el.Item2));
            }

            return sb.ToString();
        }

        private static string ToString(KeyValueStoreReplicaStatus kvsStatus)
        {
            if (kvsStatus == null)
            {
                return string.Empty;
            }

            StringBuilder sb = new StringBuilder();

            var format = "\t{0, -26} : {1}";

            sb.Append(string.Format(CultureInfo.CurrentCulture, format, Constants.ProviderKind, kvsStatus.ProviderKind));
            sb.Append(Environment.NewLine);

            if (kvsStatus.ProviderKind == KeyValueStoreProviderKind.ESE)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, format, Constants.DatabaseRowCountEstimatePropertyName, kvsStatus.DatabaseRowCountEstimate));
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, format, Constants.DatabaseLogicalSizeEstimatePropertyName, kvsStatus.DatabaseLogicalSizeEstimate));
                sb.Append(Environment.NewLine);
                
                if (kvsStatus.CopyNotificationCurrentKeyFilter != null)
                {
                    sb.Append(string.Format(CultureInfo.CurrentCulture, format, Constants.CopyNotificationCurrentKeyFilterPropertyName, kvsStatus.CopyNotificationCurrentKeyFilter));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, format, Constants.CopyNotificationCurrentProgressPropertyName, kvsStatus.CopyNotificationCurrentProgress));
                    sb.Append(Environment.NewLine);
                }
            }

            if (!string.IsNullOrEmpty(kvsStatus.StatusDetails))
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, format, Constants.StatusDetails, kvsStatus.StatusDetails));
                sb.Append(Environment.NewLine);
            }

            if (kvsStatus.MigrationStatus != null)
            {
                var migrationStatus = kvsStatus.MigrationStatus;

                sb.Append(string.Format(CultureInfo.CurrentCulture, format, Constants.CurrentMigrationPhase, migrationStatus.CurrentPhase));
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, format, Constants.CurrentMigrationState, migrationStatus.State));
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, format, Constants.NextMigrationPhase, migrationStatus.NextPhase));
                sb.Append(Environment.NewLine);
            }

            sb.Append(Environment.NewLine);

            return sb.ToString();
        }

        private static string ToString(UpgradeDomainProgress upgradeDomainProgress)
        {
            if (upgradeDomainProgress.NodeProgressList.Count == 0)
            {
                return string.Empty;
            }
            else
            {
                StringBuilder sb = new StringBuilder();
                sb.Append(upgradeDomainProgress.UpgradeDomainName);

                foreach (var nodeProgress in upgradeDomainProgress.NodeProgressList)
                {
                    sb.Append(Environment.NewLine);
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-19} : {1}", Constants.NodeNamePropertyName, nodeProgress.NodeName));

                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-19} : {1}", Constants.UpgradePhasePropertyName, nodeProgress.UpgradePhase));

                    if ((nodeProgress.PendingSafetyChecks != null) && (nodeProgress.PendingSafetyChecks.Count > 0))
                    {
                        sb.Append(Environment.NewLine);
                        sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-19} :", Constants.PendingSafetyCheckPropertyName));

                        sb.Append(Environment.NewLine);

                        for (int i = 0; i < (nodeProgress.PendingSafetyChecks.Count - 1); i++)
                        {
                            var safetyCheck = nodeProgress.PendingSafetyChecks[i];
                            sb.Append(string.Format(CultureInfo.CurrentCulture, "\t{0}", ToString(safetyCheck)));
                            sb.Append(Environment.NewLine);
                        }

                        var lastSafetyCheck = nodeProgress.PendingSafetyChecks[nodeProgress.PendingSafetyChecks.Count - 1];
                        sb.Append(string.Format(CultureInfo.CurrentCulture, "\t{0}", ToString(lastSafetyCheck)));
                    }
                }

                return sb.ToString();
            }
        }

        private static string ToString(UpgradeSafetyCheck safetyCheck)
        {
            switch (safetyCheck.Kind)
            {
            case UpgradeSafetyCheckKind.EnsureSeedNodeQuorum:
                return safetyCheck.Kind.ToString();

            case UpgradeSafetyCheckKind.EnsurePartitionQuorum:
            case UpgradeSafetyCheckKind.WaitForInbuildReplica:
            case UpgradeSafetyCheckKind.WaitForPrimaryPlacement:
            case UpgradeSafetyCheckKind.WaitForPrimarySwap:
            case UpgradeSafetyCheckKind.WaitForReconfiguration:
            case UpgradeSafetyCheckKind.EnsureAvailability:
            case UpgradeSafetyCheckKind.WaitForResourceAvailability:
            {
                var partitionSafetyCheck = (PartitionUpgradeSafetyCheck)safetyCheck;
                return string.Format(
                           CultureInfo.InvariantCulture,
                           "{1} - PartitionId: {0}",
                           partitionSafetyCheck.PartitionId,
                           safetyCheck.Kind.ToString());
            }

            default:
                return string.Format(
                           CultureInfo.InvariantCulture,
                           "Unknown SafetyCheck: {0}",
                           safetyCheck.Kind);
            }
        }

        private static string ToString(IList<NodeLoadMetricInformation> metrics)
        {
            if (metrics.Count == 0)
            {
                return "{}";
            }
            else
            {
                StringBuilder sb = new StringBuilder();

                for (int i = 0; i < metrics.Count; ++i)
                {
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.NodeLoadMetricsPropertyMetricName, metrics[i].Name));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.NodeLoadMetricsPropertyNodeCapacity, metrics[i].NodeCapacity));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.NodeLoadMetricsPropertyNodeLoad, metrics[i].CurrentNodeLoad));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.NodeLoadMetricsPropertyNodeRemainingCapacity, metrics[i].NodeCapacityRemaining));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.NodeLoadMetricsPropertyNodeBufferedCapacity, metrics[i].NodeBufferedCapacity));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.NodeLoadMetricsPropertyNodeRemainingBufferedCapacity, metrics[i].BufferedNodeCapacityRemaining));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.NodeLoadMetricsPropertyIsCapacityViolation, metrics[i].IsCapacityViolation));
                    sb.Append(Environment.NewLine);
                }

                return sb.ToString();
            }
        }

        private static string ToString(IList<ApplicationLoadMetricInformation> metrics)
        {
            if (metrics.Count == 0)
            {
                return "{}";
            }
            else
            {
                StringBuilder sb = new StringBuilder();

                for (int i = 0; i < metrics.Count; ++i)
                {
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ApplicationLoadMetricPropertyName, metrics[i].Name));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ApplicationLoadMetricPropertyApplicationLoad, metrics[i].ApplicationLoad));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ApplicationLoadMetricPropertyApplicationCapacity, metrics[i].ApplicationCapacity));
                    sb.Append(Environment.NewLine);
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ApplicationLoadMetricPropertyReservedCapacity, metrics[i].ReservationCapacity));
                    sb.Append(Environment.NewLine);
                }

                return sb.ToString();
            }
        }

        private static string ToString(ServiceGroupMemberMemberList serviceGroupMemberMemberList)
        {
            StringBuilder sb = new StringBuilder();

            sb.Append("{");
            foreach (ServiceGroupMemberMember serviceGroupMemberMember in serviceGroupMemberMemberList)
            {
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-19} : {1}", Constants.ServiceTypeNamePropertyName, serviceGroupMemberMember.ServiceTypeName));
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-19} : {1}", Constants.ServiceNamePropertyName, serviceGroupMemberMember.ServiceName));
                sb.Append(Environment.NewLine);
            }

            sb.Append("}");

            return sb.ToString();
        }

        private static string ToString(ICollection<ServiceGroupTypeMemberDescription> memberDescriptions)
        {
            StringBuilder sb = new StringBuilder();

            sb.Append("{");
            foreach (ServiceGroupTypeMemberDescription memberDescription in memberDescriptions)
            {
                sb.Append(Environment.NewLine);
                sb.Append(memberDescription.ServiceTypeName);
            }

            sb.Append(Environment.NewLine);
            sb.Append("}");

            return sb.ToString();
        }

        private static string ToString(NodeDeactivationResult nodeDeactivationInfo)
        {
            if (nodeDeactivationInfo.Tasks.Count == 0)
            {
                return string.Empty;
            }

            StringBuilder sb = new StringBuilder();

            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0} : {1}", Constants.NodeDeactivationEffectiveIntent, nodeDeactivationInfo.EffectiveIntent));
            sb.Append(Environment.NewLine);
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0} : {1}", Constants.NodeDeactivationStatus, nodeDeactivationInfo.Status));
            sb.Append(Environment.NewLine);

            foreach (NodeDeactivationTask task in nodeDeactivationInfo.Tasks)
            {
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0} : {1}", Constants.NodeDeactivationTaskType, task.TaskId.Type));
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0} : {1}", Constants.NodeDeactivationTaskId, task.TaskId.Id));
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0} : {1}", Constants.NodeDeactivationIntent, task.Intent));
                sb.Append(Environment.NewLine);
            }

            if ((nodeDeactivationInfo.PendingSafetyChecks != null) && (nodeDeactivationInfo.PendingSafetyChecks.Count > 0))
            {
                sb.Append(Environment.NewLine);
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0,-19} :", Constants.PendingSafetyCheckPropertyName));

                sb.Append(Environment.NewLine);

                for (int i = 0; i < (nodeDeactivationInfo.PendingSafetyChecks.Count - 1); i++)
                {
                    var safetyCheck = nodeDeactivationInfo.PendingSafetyChecks[i];
                    sb.Append(string.Format(CultureInfo.CurrentCulture, "\t{0}", ToString(safetyCheck)));
                    sb.Append(Environment.NewLine);
                }

                var lastSafetyCheck = nodeDeactivationInfo.PendingSafetyChecks[nodeDeactivationInfo.PendingSafetyChecks.Count - 1];
                sb.Append(string.Format(CultureInfo.CurrentCulture, "\t{0}", ToString(lastSafetyCheck)));
            }

            return sb.ToString();
        }

        private static string ToShortString(ServiceTypeHealthPolicy serviceTypeHealthPolicy)
        {
            var sb = new StringBuilder();
            sb.Append(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "{0} [{1} = {2}, {3} = {4}, {5} = {6}]",
                    Constants.ServiceTypeHealthPolicyPropertyName,
                    Constants.MaxPercentUnhealthyServicesPropertyName,
                    serviceTypeHealthPolicy.MaxPercentUnhealthyServices,
                    Constants.MaxPercentUnhealthyPartitionsPerServicePropertyName,
                    serviceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService,
                    Constants.MaxPercentUnhealthyReplicasPerPartitionPropertyName,
                    serviceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition));

            return sb.ToString();
        }

        private static string ToShortString(ApplicationHealthPolicy appHealthPolicy)
        {
            var sb = new StringBuilder();
            sb.Append(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "{0} [{1} = {2}, {3} = {4}",
                    Constants.ApplicationHealthPolicyPropertyName,
                    Constants.MaxPercentUnhealthyDeployedApplicationsPropertyName,
                    appHealthPolicy.MaxPercentUnhealthyDeployedApplications,
                    Constants.ConsiderWarningAsErrorPropertyName,
                    appHealthPolicy.ConsiderWarningAsError));

            if (appHealthPolicy.DefaultServiceTypeHealthPolicy != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}{1}", Constants.DefaultConstantName, ToShortString(appHealthPolicy.DefaultServiceTypeHealthPolicy)));
            }

            foreach (var entry in appHealthPolicy.ServiceTypeHealthPolicyMap)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", \"{0}\" = {1}", entry.Key, ToShortString(entry.Value)));
            }

            sb.Append(" ]");
            return sb.ToString();
        }

        private static string ToString(IDictionary<Uri, ApplicationHealthPolicy> appHealthPolicyMap)
        {
            if (appHealthPolicyMap == null || appHealthPolicyMap.Count == 0)
            {
                return "{}";
            }

            var sb = new StringBuilder();
            sb.Append("{");

            bool first = true;
            foreach (var entry in appHealthPolicyMap)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    sb.AppendLine(";");
                }

                sb.Append(string.Format(CultureInfo.CurrentCulture, "\"{0}\" = {1}", entry.Key, ToShortString(entry.Value)));
            }

            sb.Append("}");
            return sb.ToString();
        }

        private static string ToString(ApplicationTypeHealthPolicyMap appTypeHealthPolicyMap)
        {
            if (appTypeHealthPolicyMap == null || appTypeHealthPolicyMap.Count == 0)
            {
                return "{}";
            }

            var sb = new StringBuilder();
            sb.Append("{ ");

            bool first = true;
            foreach (var entry in appTypeHealthPolicyMap)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    sb.AppendLine(";");
                }

                sb.Append(string.Format(CultureInfo.CurrentCulture, "\"{0}\" = {1}", entry.Key, entry.Value));
            }

            sb.Append(" }");
            return sb.ToString();
        }

        private static string ToString(SafetyCheck safetyCheck)
        {
            switch (safetyCheck.Kind)
            {
            case SafetyCheckKind.EnsureSeedNodeQuorum:
                return safetyCheck.Kind.ToString();

            case SafetyCheckKind.EnsurePartitionQuorum:
            case SafetyCheckKind.WaitForInBuildReplica:
            case SafetyCheckKind.WaitForPrimaryPlacement:
            case SafetyCheckKind.WaitForPrimarySwap:
            case SafetyCheckKind.WaitForReconfiguration:
            case SafetyCheckKind.EnsureAvailability:
            {
                var partitionSafetyCheck = (PartitionSafetyCheck)safetyCheck;
                return string.Format(
                           CultureInfo.InvariantCulture,
                           "{1} - PartitionId: {0}",
                           partitionSafetyCheck.PartitionId,
                           safetyCheck.Kind.ToString());
            }

            default:
                return string.Format(
                           CultureInfo.InvariantCulture,
                           "Unknown SafetyCheck: {0}",
                           safetyCheck.Kind);
            }
        }

        private static string ToString(SelectedPartition selectedPartition)
        {
            if (selectedPartition == SelectedPartition.None)
            {
                return Constants.NoneResultOutput;
            }

            var selectedPartitionBuilder = new StringBuilder();
            selectedPartitionBuilder.Append(Environment.NewLine);
            selectedPartitionBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ServiceNamePropertyName, selectedPartition.ServiceName));
            selectedPartitionBuilder.Append(Environment.NewLine);
            selectedPartitionBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.PartitionIdPropertyName, selectedPartition.PartitionId));
            selectedPartitionBuilder.Append(Environment.NewLine);

            return selectedPartitionBuilder.ToString();
        }

        private static string ToString(SelectedReplica selectedReplica)
        {
            if (selectedReplica == SelectedReplica.None)
            {
                return Constants.NoneResultOutput;
            }

            var selectedPartitionBuilder = new StringBuilder();

            selectedPartitionBuilder.Append(Environment.NewLine);
            selectedPartitionBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.ReplicaIdPropertyName, selectedReplica.ReplicaOrInstanceId));
            selectedPartitionBuilder.Append(Environment.NewLine);
            selectedPartitionBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.SelectedPartitionPropertyName, OutputFormatter.ToString(selectedReplica.SelectedPartition)));
            selectedPartitionBuilder.Append(Environment.NewLine);

            return selectedPartitionBuilder.ToString();
        }

        private static string ToString(NodeResult nodeResult)
        {
            if (nodeResult == null)
            {
                return Constants.NoneResultOutput;
            }

            var selectedPartitionBuilder = new StringBuilder();

            selectedPartitionBuilder.Append(Environment.NewLine);
            selectedPartitionBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.NodeNamePropertyName, nodeResult.NodeName));
            selectedPartitionBuilder.Append(Environment.NewLine);
            selectedPartitionBuilder.Append(string.Format(CultureInfo.CurrentCulture, "{0,-21} : {1}", Constants.InstanceIdPropertyName, nodeResult.NodeInstance));
            selectedPartitionBuilder.Append(Environment.NewLine);

            return selectedPartitionBuilder.ToString();
        }

        private static string GetPadString(int indentLevel)
        {
            if (indentLevel > 0)
            {
                return new string('\t', indentLevel);
            }

            return string.Empty;
        }

        private static string ToString(List<ChaosEvent> chaosEvents)
        {
            if (chaosEvents.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var chaosEventsBuilder = new StringBuilder();
            foreach (var evnt in chaosEvents)
            {
                chaosEventsBuilder.Append(Environment.NewLine);
                chaosEventsBuilder.Append(evnt);
            }

            return chaosEventsBuilder.ToString();
        }

        private static string ToString(ReconfigurationInformation reconfigurationInformation)
        {
            var sb = new StringBuilder();

            sb.Append("{" + Environment.NewLine);
            if (reconfigurationInformation != null)
            {
                foreach (var property in reconfigurationInformation.GetType().GetProperties())
                {
                    if ((property.Name.CompareTo(Constants.ReconfigurationStartTimeUtc) == 0) &&
                        (reconfigurationInformation.ReconfigurationStartTimeUtc == DateTime.MinValue))
                    {
                        continue;
                    }

                    sb.Append(
                            string.Format(
                                CultureInfo.CurrentCulture,
                                "{0,-36} : {1}",
                                property.Name,
                                property.GetValue(reconfigurationInformation, null)));
                    sb.Append(Environment.NewLine);
                }
            }

            sb.Append("}");
            return sb.ToString();
        }

        private static string ToString(DeployedStatefulServiceReplica deployedStatefulServiceReplica)
        {
            var sb = new StringBuilder();

            sb.Append("{" + Environment.NewLine);
            if (deployedStatefulServiceReplica != null)
            {
                foreach (var property in deployedStatefulServiceReplica.GetType().GetProperties())
                {
                    if ((property.Name.CompareTo(Constants.ServiceName) == 0) ||
                        (property.Name.CompareTo(Constants.ServiceTypeName) == 0) ||
                        (property.Name.CompareTo(Constants.ServiceManifestVersion) == 0) ||
                        (property.Name.CompareTo(Constants.ServiceKind) == 0) ||
                        (property.Name.CompareTo(Constants.ReplicaId) == 0) ||
                        (property.Name.CompareTo(Constants.ReconfigurationInformation) == 0) ||
                        (property.Name.CompareTo(Constants.PartitionId) == 0))
                    {
                        continue;
                    }

                    sb.Append(
                            string.Format(
                                CultureInfo.CurrentCulture,
                                "{0,-36} : {1}",
                                property.Name,
                                property.GetValue(deployedStatefulServiceReplica, null)));
                    sb.Append(Environment.NewLine);
                }
            }

            sb.Append("}");
            return sb.ToString();
        }

        private static string ToString(DeployedStatelessServiceInstance deployedStatelessServiceInstance)
        {
            var sb = new StringBuilder();

            sb.Append("{" + Environment.NewLine);
            if (deployedStatelessServiceInstance != null)
            {
                foreach (var property in deployedStatelessServiceInstance.GetType().GetProperties())
                {
                    if ((property.Name.CompareTo(Constants.ServiceName) == 0) ||
                        (property.Name.CompareTo(Constants.ServiceTypeName) == 0) ||
                        (property.Name.CompareTo(Constants.ServiceManifestVersion) == 0) ||
                        (property.Name.CompareTo(Constants.InstanceId) == 0) ||
                        (property.Name.CompareTo(Constants.PartitionId) == 0))
                    {
                        continue;
                    }

                    sb.Append(
                            string.Format(
                                CultureInfo.CurrentCulture,
                                "{0,-36} : {1}",
                                property.Name,
                                property.GetValue(deployedStatelessServiceInstance, null)));
                    sb.Append(Environment.NewLine);
                }
            }

            sb.Append("}");
            return sb.ToString();
        }

        private static string ToString(List<ChaosScheduleJob> chaosScheduleJobs)
        {
            if (chaosScheduleJobs.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var description = new StringBuilder();
            foreach (var job in chaosScheduleJobs)
            {
                description.Append(job.ToString());
                description.Append(Environment.NewLine);
            }

            return description.ToString();
        }

        private static string ToString(Dictionary<string, ChaosParameters> chaosParametersDictionary)
        {
            if (chaosParametersDictionary.Count == 0)
            {
                return Constants.NoneResultOutput;
            }

            var description = new StringBuilder();
            foreach (var entry in chaosParametersDictionary)
            {
                description.AppendLine(string.Format(CultureInfo.CurrentCulture, "Key: {0}", entry.Key));
                description.AppendLine(string.Format(CultureInfo.CurrentCulture, "Value: {0}", entry.Value.ToString()));
                description.Append(Environment.NewLine);
            }

            return description.ToString();
        }

        private static string ToString(IList<ScalingPolicyDescription> scaliingPolicies)
        {
            if (scaliingPolicies == null || scaliingPolicies.Count == 0)
            {
                return "{}";
            }
            else
            {
                StringBuilder sb = new StringBuilder();
                sb.Append("{ ");

                foreach (var item in scaliingPolicies)
                {
                    sb.Append(Environment.NewLine);
                    sb.Append(OutputFormatter.ToString(item));
                }

                sb.Append(" }");
                return sb.ToString();
            }
        }
    }
}
