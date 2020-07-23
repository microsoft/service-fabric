// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using Microsoft.ServiceFabric.Preview.Client.Description;
    using Microsoft.ServiceFabric.Preview.Client.Query;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Collections.Specialized;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using System.Reflection;
    using System.Text;
    using WEX.Logging.Interop;
    using WEX.TestExecution;

    internal class TestUtility
    {
        private static PowerShell powershell;

        public static void OpenPowerShell()
        {
            Log.Comment("Open powershell console");
            var sessionState = InitialSessionState.CreateDefault();
            var location = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            Assert.IsNotNull(location);
            var powershellAssemblyLocation = Path.Combine(location, TestConstants.PowershellAssemblyName);
            sessionState.ImportPSModule(new[] { powershellAssemblyLocation });

            powershell = PowerShell.Create();
            powershell.Runspace = RunspaceFactory.CreateRunspace(sessionState);
            powershell.Runspace.Open();
        }

        public static void ClosePowerShell()
        {
            Log.Comment("Close powershell console");
            powershell.Dispose();
        }

        public static Collection<PSObject> ExecuteCommand(IClusterConnection clusterConnection, Command command)
        {
            Log.Comment("Execute powershell command");
            var pipeline = powershell.Runspace.CreatePipeline();
            pipeline.Runspace.SessionStateProxy.SetVariable(Constants.ClusterConnectionVariableName, clusterConnection);
            pipeline.Commands.Add(command);
            LogCommand(command);
            var output = pipeline.Invoke();
            LogOutput(output);
            return output;
        }

        public static Collection<PSObject> ExecuteScript(IClusterConnection clusterConnection, string script)
        {
            Log.Comment("Execute powershell script");
            var pipeline = powershell.Runspace.CreatePipeline();
            pipeline.Runspace.SessionStateProxy.SetVariable(Constants.ClusterConnectionVariableName, clusterConnection);
            pipeline.Commands.AddScript(script);
            Log.Comment(script);
            var output = pipeline.Invoke();

            if (pipeline.HadErrors)
            {
                var errors = pipeline.Error.ReadToEnd();
                foreach (var item in errors)
                {
                    Log.Comment(string.Format("Error: {0}", item));
                }
            }

            LogOutput(output);
            return output;
        }

        public static void LogCommand(Command command)
        {
            var paramsBuilder = new StringBuilder();
            if (command.Parameters != null)
            {
                foreach (var param in command.Parameters)
                {
                    paramsBuilder.Append('-');
                    paramsBuilder.Append(param.Name);
                    paramsBuilder.Append(' ');

                    if (param.Value != null)
                    {
                        paramsBuilder.Append(param.Value);
                        paramsBuilder.Append(' ');
                    }
                }
            }

            Log.Comment(string.Format("{0} {1}", command.CommandText, paramsBuilder.ToString().TrimEnd(' ')));
        }

        public static void LogOutput(IEnumerable<PSObject> output)
        {
            if (output == null)
            {
                Log.Comment("Output null result");
                return;
            }

            var outputBuilder = new StringBuilder();
            foreach (var obj in output)
            {
                if (obj == null)
                {
                    outputBuilder.Append("null");
                    continue;
                }

                outputBuilder.Append(obj.ImmediateBaseObject);
                outputBuilder.Append('\n');

                foreach (var property in obj.Properties)
                {
                    outputBuilder.Append(property.Name);
                    outputBuilder.Append(':');
                    outputBuilder.Append((property.Value == null) ? "null" : property.Value.ToString());
                    outputBuilder.Append('\n');
                }
            }

            Log.Comment(string.Format("Output {0}", outputBuilder));
        }

        public static void AreEqual(IDictionary<string, string> expect, IDictionary<string, string> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            foreach (var pair in expect)
            {
                string value;
                Verify.IsTrue(actual.TryGetValue(pair.Key, out value));
                Verify.AreEqual(pair.Value, value);
            }
        }

        public static void AreEqual(IList<string> expect, IList<string> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            for (var index = 0; index < expect.Count; ++index)
            {
                Verify.AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(NameValueCollection expect, NameValueCollection actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            foreach (var key in expect.AllKeys)
            {
                Verify.AreEqual(expect[key], actual[key]);
            }
        }

        public static void AreEqual(
            ApplicationParameterList expect, ApplicationParameterList actual)
        {
            foreach (var expectParameter in expect)
            {
                var actualParameter = actual.FirstOrDefault(actualItem => actualItem.Name == expectParameter.Name);
                Assert.IsNotNull(actualParameter);
                Verify.AreEqual(expectParameter.Value, actualParameter.Value);
            }
        }

        public static void AreEqual(
            ReadOnlyCollection<UpgradeDomainStatus> expect, ReadOnlyCollection<UpgradeDomainStatus> actual)
        {
            foreach (var expectStatus in expect)
            {
                var actualStatus = actual.FirstOrDefault(actualItem => actualItem.Name == expectStatus.Name);
                Assert.IsNotNull(actualStatus);
                Verify.AreEqual(expectStatus.State, actualStatus.State);
            }
        }

        public static void AreEqual(
            IList<ServiceHealthState> expect, IList<ServiceHealthState> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            for (var index = 0; index < expect.Count; ++index)
            {
                AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(
            IList<NodeHealthState> expect, IList<NodeHealthState> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            for (var index = 0; index < expect.Count; ++index)
            {
                AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(
            IList<ApplicationHealthState> expect, IList<ApplicationHealthState> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            for (var index = 0; index < expect.Count; ++index)
            {
                AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(
            IList<DeployedApplicationHealthState> expect, IList<DeployedApplicationHealthState> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            for (var index = 0; index < expect.Count; ++index)
            {
                AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(
            IList<PartitionHealthState> expect, IList<PartitionHealthState> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            for (var index = 0; index < expect.Count; ++index)
            {
                AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(
            IList<ReplicaHealthState> expect, IList<ReplicaHealthState> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            for (var index = 0; index < expect.Count; ++index)
            {
                AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(
            IList<HealthEvent> expect, IList<HealthEvent> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            for (var index = 0; index < expect.Count; ++index)
            {
                AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(
            HealthStatistics expect, HealthStatistics actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.HealthStateCountList, actual.HealthStateCountList);
        }

        public static void AreEqual(
            IList<EntityKindHealthStateCount> expect, IList<EntityKindHealthStateCount> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            foreach (var entry in expect)
            {
                bool found = false;
                foreach (var actualEntry in actual)
                {
                    if (entry.EntityKind == actualEntry.EntityKind)
                    {
                        found = true;
                        Verify.AreEqual(entry.HealthStateCount, actualEntry.HealthStateCount);
                        break;
                    }
                }

                Assert.IsTrue(found);
            }
        }

        public static void AreEqual(HealthStateCount expect, HealthStateCount actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.OkCount, actual.OkCount);
            Assert.AreEqual(expect.WarningCount, actual.WarningCount);
            Assert.AreEqual(expect.ErrorCount, actual.ErrorCount);
        }

        public static void AreEqual(
            IList<DeployedServicePackageHealthState> expect, IList<DeployedServicePackageHealthState> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            for (var index = 0; index < expect.Count; ++index)
            {
                AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(
            ICollection<ResolvedServiceEndpoint> expect, ICollection<ResolvedServiceEndpoint> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            var expectArray = new ResolvedServiceEndpoint[1];
            var actualArray = new ResolvedServiceEndpoint[1];
            expect.CopyTo(expectArray, 0);
            actual.CopyTo(actualArray, 0);
            Verify.AreEqual(expectArray.Length, actualArray.Length);
            for (var index = 0; index < expectArray.Length; ++index)
            {
                AreEqual(expectArray[index], actualArray[index]);
            }
        }

        public static void AreEqual(Application expect, Application actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Verify.AreEqual(expect.ApplicationTypeName, actual.ApplicationTypeName);
            Verify.AreEqual(expect.ApplicationTypeVersion, actual.ApplicationTypeVersion);
            Verify.AreEqual(expect.ApplicationStatus, actual.ApplicationStatus);
            Verify.AreEqual(expect.HealthState, actual.HealthState);
            AreEqual(expect.ApplicationParameters, actual.ApplicationParameters);
        }

        public static void AreEqual(DeployedApplication expect, DeployedApplication actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Verify.AreEqual(expect.ApplicationTypeName, actual.ApplicationTypeName);
        }

        public static void AreEqual(ApplicationType expect, ApplicationType actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.ApplicationTypeName, actual.ApplicationTypeName);
            Verify.AreEqual(expect.ApplicationTypeVersion, actual.ApplicationTypeVersion);
        }

        public static void AreEqual(StatelessService expect, StatelessService actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.ServiceKind, actual.ServiceKind);
            Verify.AreEqual(expect.ServiceName, actual.ServiceName);
            Verify.AreEqual(expect.ServiceTypeName, actual.ServiceTypeName);
            Verify.AreEqual(expect.ServiceManifestVersion, actual.ServiceManifestVersion);
            Verify.AreEqual(expect.HealthState, actual.HealthState);
        }

        public static void AreEqual(StatefulService expect, StatefulService actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.ServiceKind, actual.ServiceKind);
            Verify.AreEqual(expect.ServiceName, actual.ServiceName);
            Verify.AreEqual(expect.ServiceTypeName, actual.ServiceTypeName);
            Verify.AreEqual(expect.ServiceManifestVersion, actual.ServiceManifestVersion);
            Verify.AreEqual(expect.HealthState, actual.HealthState);
            Verify.AreEqual(expect.HasPersistedState, actual.HasPersistedState);
        }

        public static void AreEqual(SingletonPartitionInformation expect, SingletonPartitionInformation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Id, actual.Kind);
            Verify.AreEqual(expect.Kind, actual.Kind);
        }

        public static void AreEqual(Int64RangePartitionInformation expect, Int64RangePartitionInformation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Id, actual.Kind);
            Verify.AreEqual(expect.Kind, actual.Kind);
            Verify.AreEqual(expect.LowKey, actual.LowKey);
            Verify.AreEqual(expect.HighKey, actual.HighKey);
        }

        public static void AreEqual(NamedPartitionInformation expect, NamedPartitionInformation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Id, actual.Kind);
            Verify.AreEqual(expect.Kind, actual.Kind);
            Verify.AreEqual(expect.Name, actual.Name);
        }

        public static void AreEqual(
            System.Fabric.Query.StatelessServicePartition expect, System.Fabric.Query.StatelessServicePartition actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.PartitionInformation, actual.PartitionInformation);
            Verify.AreEqual(expect.InstanceCount, actual.InstanceCount);
            Verify.AreEqual(expect.HealthState, actual.HealthState);
            Verify.AreEqual(expect.PartitionStatus, actual.PartitionStatus);
        }

        public static void AreEqual(
            System.Fabric.Query.StatefulServicePartition expect, System.Fabric.Query.StatefulServicePartition actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.PartitionInformation, actual.PartitionInformation);
            Verify.AreEqual(expect.MinReplicaSetSize, actual.MinReplicaSetSize);
            Verify.AreEqual(expect.TargetReplicaSetSize, actual.TargetReplicaSetSize);
            Verify.AreEqual(expect.HealthState, actual.HealthState);
            Verify.AreEqual(expect.PartitionStatus, actual.PartitionStatus);
            Verify.AreEqual(expect.LastQuorumLossDuration, actual.LastQuorumLossDuration);
        }

        public static void AreEqual(
            StatelessServiceInstance expect, StatelessServiceInstance actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Id, actual.Id);
            Verify.AreEqual(expect.ReplicaAddress, expect.ReplicaAddress);
            Verify.AreEqual(expect.HealthState, actual.HealthState);
            Verify.AreEqual(expect.NodeName, actual.NodeName);
            Verify.AreEqual(expect.ReplicaStatus, actual.ReplicaStatus);
            Verify.AreEqual(expect.LastInBuildDuration, actual.LastInBuildDuration);
        }

        public static void AreEqual(StatefulServiceReplica expect, StatefulServiceReplica actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Id, actual.Id);
            Verify.AreEqual(expect.ReplicaAddress, expect.ReplicaAddress);
            Verify.AreEqual(expect.HealthState, actual.HealthState);
            Verify.AreEqual(expect.NodeName, actual.NodeName);
            Verify.AreEqual(expect.ReplicaStatus, actual.ReplicaStatus);
            Verify.AreEqual(expect.ReplicaRole, actual.ReplicaRole);
            Verify.AreEqual(expect.LastInBuildDuration, actual.LastInBuildDuration);
        }

        public static void AreEqual(FabricUpgradeDescription expect, FabricUpgradeDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.UpgradePolicyDescription.Kind, actual.UpgradePolicyDescription.Kind);
            if (expect.UpgradePolicyDescription is RollingUpgradePolicyDescription)
            {
                var expectPolicy = expect.UpgradePolicyDescription as RollingUpgradePolicyDescription;
                var actualPolicy = actual.UpgradePolicyDescription as RollingUpgradePolicyDescription;
                Assert.IsNotNull(expectPolicy);
                Assert.IsNotNull(actualPolicy);
                Verify.AreEqual(expectPolicy.ForceRestart, actualPolicy.ForceRestart);
                Verify.AreEqual(expectPolicy.UpgradeMode, actualPolicy.UpgradeMode);
                Verify.AreEqual(expectPolicy.UpgradeReplicaSetCheckTimeout, actualPolicy.UpgradeReplicaSetCheckTimeout);
            }

            if (expect.UpgradePolicyDescription is MonitoredRollingFabricUpgradePolicyDescription)
            {
                var expectPolicy = expect.UpgradePolicyDescription as MonitoredRollingFabricUpgradePolicyDescription;
                var actualPolicy = actual.UpgradePolicyDescription as MonitoredRollingFabricUpgradePolicyDescription;
                Assert.IsNotNull(expectPolicy);
                Assert.IsNotNull(actualPolicy);
                Verify.AreEqual(expectPolicy.EnableDeltaHealthEvaluation, actualPolicy.EnableDeltaHealthEvaluation);
                AreEqual(expectPolicy.HealthPolicy, actualPolicy.HealthPolicy);
                AreEqual(expectPolicy.ApplicationHealthPolicyMap, actualPolicy.ApplicationHealthPolicyMap);
            }

            Verify.AreEqual(expect.TargetCodeVersion, actual.TargetCodeVersion);
            Verify.AreEqual(expect.TargetConfigVersion, actual.TargetConfigVersion);
        }

        public static void AreEqual(ApplicationHealthPolicyMap expected, ApplicationHealthPolicyMap actual)
        {
            Verify.IsTrue(ApplicationHealthPolicyMap.AreEqual(expected, actual), "ApplicationHealthPolicyMap.AreEqual(expected, actual)");
        }

        public static void AreEqual(FabricUpgradeUpdateDescription expect, FabricUpgradeUpdateDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.UpgradeMode, actual.UpgradeMode);
            Verify.AreEqual(expect.ForceRestart, actual.ForceRestart);
            Verify.AreEqual(expect.UpgradeReplicaSetCheckTimeout, actual.UpgradeReplicaSetCheckTimeout);
            Verify.AreEqual(expect.FailureAction, actual.FailureAction);
            Verify.AreEqual(expect.HealthCheckWaitDuration, actual.HealthCheckWaitDuration);
            Verify.AreEqual(expect.HealthCheckStableDuration, actual.HealthCheckStableDuration);
            Verify.AreEqual(expect.HealthCheckRetryTimeout, actual.HealthCheckRetryTimeout);
            Verify.AreEqual(expect.UpgradeTimeout, actual.UpgradeTimeout);
            Verify.AreEqual(expect.UpgradeDomainTimeout, actual.UpgradeDomainTimeout);
            Verify.IsTrue(ClusterHealthPolicy.AreEqual(expect.HealthPolicy, actual.HealthPolicy));
        }

        public static void AreEqual(FabricUpgradeProgress expect, FabricUpgradeProgress actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.NextUpgradeDomain, actual.NextUpgradeDomain);
            Verify.AreEqual(expect.RollingUpgradeMode, actual.RollingUpgradeMode);
            Verify.AreEqual(expect.TargetCodeVersion, actual.TargetCodeVersion);
            Verify.AreEqual(expect.TargetConfigVersion, actual.TargetConfigVersion);
            AreEqual(expect.UpgradeDomains, actual.UpgradeDomains);
            Verify.AreEqual(expect.UpgradeState, actual.UpgradeState);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);

            // TODO: AreEqual(expect.CurrentUpgradeDomainProgress, actual.CurrentUpgradeDomainProgress);
            AreEqual(expect.UpgradeDescription, actual.UpgradeDescription);
        }

        public static void AreEqual(ApplicationUpgradeDescription expect, ApplicationUpgradeDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.UpgradePolicyDescription.Kind, actual.UpgradePolicyDescription.Kind);
            if (expect.UpgradePolicyDescription is MonitoredRollingApplicationUpgradePolicyDescription)
            {
                var expectPolicy = expect.UpgradePolicyDescription as MonitoredRollingApplicationUpgradePolicyDescription;
                var actualPolicy = actual.UpgradePolicyDescription as MonitoredRollingApplicationUpgradePolicyDescription;
                Assert.IsNotNull(expectPolicy);
                Assert.IsNotNull(actualPolicy);
                Verify.AreEqual(expectPolicy.ForceRestart, actualPolicy.ForceRestart);
                Verify.AreEqual(expectPolicy.UpgradeMode, actualPolicy.UpgradeMode);
                Verify.AreEqual(expectPolicy.UpgradeReplicaSetCheckTimeout, actualPolicy.UpgradeReplicaSetCheckTimeout);
                AreEqual(expectPolicy.MonitoringPolicy, actualPolicy.MonitoringPolicy);
                AreEqual(expectPolicy.HealthPolicy, actualPolicy.HealthPolicy);
            }
            else if (expect.UpgradePolicyDescription is MonitoredRollingFabricUpgradePolicyDescription)
            {
                var expectPolicy = expect.UpgradePolicyDescription as MonitoredRollingFabricUpgradePolicyDescription;
                var actualPolicy = actual.UpgradePolicyDescription as MonitoredRollingFabricUpgradePolicyDescription;
                Assert.IsNotNull(expectPolicy);
                Assert.IsNotNull(actualPolicy);
                Verify.AreEqual(expectPolicy.ForceRestart, actualPolicy.ForceRestart);
                Verify.AreEqual(expectPolicy.UpgradeMode, actualPolicy.UpgradeMode);
                Verify.AreEqual(expectPolicy.UpgradeReplicaSetCheckTimeout, actualPolicy.UpgradeReplicaSetCheckTimeout);
                AreEqual(expectPolicy.MonitoringPolicy, actualPolicy.MonitoringPolicy);
                AreEqual(expectPolicy.HealthPolicy, actualPolicy.HealthPolicy);
            }
            else if (expect.UpgradePolicyDescription is RollingUpgradePolicyDescription)
            {
                var expectPolicy = expect.UpgradePolicyDescription as RollingUpgradePolicyDescription;
                var actualPolicy = actual.UpgradePolicyDescription as RollingUpgradePolicyDescription;
                Assert.IsNotNull(expectPolicy);
                Assert.IsNotNull(actualPolicy);
                Verify.AreEqual(expectPolicy.ForceRestart, actualPolicy.ForceRestart);
                Verify.AreEqual(expectPolicy.UpgradeMode, actualPolicy.UpgradeMode);
                Verify.AreEqual(expectPolicy.UpgradeReplicaSetCheckTimeout, actualPolicy.UpgradeReplicaSetCheckTimeout);
            }

            Verify.AreEqual(expect.ApplicationName, actual.ApplicationName);
            AreEqual(expect.ApplicationParameters, actual.ApplicationParameters);
            Verify.AreEqual(expect.TargetApplicationTypeVersion, actual.TargetApplicationTypeVersion);
        }

        public static void AreEqual(ApplicationUpgradeUpdateDescription expect, ApplicationUpgradeUpdateDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Verify.AreEqual(expect.UpgradeMode, actual.UpgradeMode);
            Verify.AreEqual(expect.ForceRestart, actual.ForceRestart);
            Verify.AreEqual(expect.UpgradeReplicaSetCheckTimeout, actual.UpgradeReplicaSetCheckTimeout);
            Verify.AreEqual(expect.FailureAction, actual.FailureAction);
            Verify.AreEqual(expect.HealthCheckWaitDuration, actual.HealthCheckWaitDuration);
            Verify.AreEqual(expect.HealthCheckStableDuration, actual.HealthCheckStableDuration);
            Verify.AreEqual(expect.HealthCheckRetryTimeout, actual.HealthCheckRetryTimeout);
            Verify.AreEqual(expect.UpgradeTimeout, actual.UpgradeTimeout);
            Verify.AreEqual(expect.UpgradeDomainTimeout, actual.UpgradeDomainTimeout);
            Verify.IsTrue(ApplicationHealthPolicy.AreEqual(expect.HealthPolicy, actual.HealthPolicy));
        }

        public static void AreEqual(ApplicationUpgradeProgress expect, ApplicationUpgradeProgress actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            AreEqual(expect.UpgradeDescription, actual.UpgradeDescription);
            Verify.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Verify.AreEqual(expect.ApplicationTypeName, actual.ApplicationTypeName);
            Verify.AreEqual(expect.TargetApplicationTypeVersion, actual.TargetApplicationTypeVersion);
            Verify.AreEqual(expect.UpgradeState, actual.UpgradeState);
            AreEqual(expect.UpgradeDomains, actual.UpgradeDomains);
            Verify.AreEqual(expect.RollingUpgradeMode, actual.RollingUpgradeMode);
            Verify.AreEqual(expect.NextUpgradeDomain, actual.NextUpgradeDomain);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);

            // TODO: AreEqual(expect.CurrentUpgradeDomainProgress, actual.CurrentUpgradeDomainProgress);
        }

        public static void AreEqual(ApplicationDescription expect, ApplicationDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Verify.AreEqual(expect.ApplicationTypeName, actual.ApplicationTypeName);
            Verify.AreEqual(expect.ApplicationTypeVersion, actual.ApplicationTypeVersion);
            AreEqual(expect.ApplicationParameters, actual.ApplicationParameters);
        }

        public static void AreEqual(ComposeDeploymentDescription expect, ComposeDeploymentDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.DeploymentName, actual.DeploymentName);
            Verify.AreEqual(expect.ComposeFilePath, actual.ComposeFilePath);
            // TODO:Verify.AreEqual(expect.Override, actual.override);
            Verify.AreEqual(expect.ContainerRegistryUserName, actual.ContainerRegistryUserName);
            Verify.AreEqual(expect.ContainerRegistryPassword, actual.ContainerRegistryPassword);
            Verify.AreEqual(expect.IsRegistryPasswordEncrypted, actual.IsRegistryPasswordEncrypted);
        }

        public static void AreEqual(ComposeDeploymentStatusQueryDescription expect, ComposeDeploymentStatusQueryDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.DeploymentNameFilter, actual.DeploymentNameFilter);
            Verify.AreEqual(expect.ContinuationToken, actual.ContinuationToken);
        }

        public static void AreEqual(ApplicationMetricDescription expect, ApplicationMetricDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Name, actual.Name);
            Verify.AreEqual(expect.NodeReservationCapacity, actual.NodeReservationCapacity);
            Verify.AreEqual(expect.MaximumNodeCapacity, actual.MaximumNodeCapacity);
        }

        public static void AreEqual(IList<ApplicationMetricDescription> expect, IList<ApplicationMetricDescription> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Verify.AreEqual(expect.Count, actual.Count);

            for (int index = 0; index < expect.Count; index++)
            {
                AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(ApplicationUpdateDescription expect, ApplicationUpdateDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Verify.AreEqual(expect.RemoveApplicationCapacity, actual.RemoveApplicationCapacity);
            Verify.AreEqual(expect.MinimumNodes.HasValue, actual.MinimumNodes.HasValue);
            if (expect.MinimumNodes.HasValue)
            {
                Verify.AreEqual(expect.MinimumNodes.Value, actual.MinimumNodes.Value);
            }

            Verify.AreEqual(expect.MaximumNodes.HasValue, actual.MaximumNodes.HasValue);
            if (expect.MaximumNodes.HasValue)
            {
                Verify.AreEqual(expect.MaximumNodes.Value, actual.MaximumNodes.Value);
            }

            AreEqual(expect.Metrics, actual.Metrics);
        }

        public static void AreEqual(ServiceDescription expect, ServiceDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Verify.AreEqual(expect.ServiceName, actual.ServiceName);
            Verify.AreEqual(expect.ServiceTypeName, actual.ServiceTypeName);
            Verify.AreEqual(expect.Kind, actual.Kind);

            switch (expect.Kind)
            {
            case ServiceDescriptionKind.Stateful:
            {
                var expectDescription = expect as StatefulServiceDescription;
                var actualDescription = actual as StatefulServiceDescription;
                Assert.IsNotNull(expectDescription);
                Assert.IsNotNull(actualDescription);
                Verify.AreEqual(expectDescription.HasPersistedState, actualDescription.HasPersistedState);
                Verify.AreEqual(expectDescription.TargetReplicaSetSize, actualDescription.TargetReplicaSetSize);
                Verify.AreEqual(expectDescription.MinReplicaSetSize, actualDescription.MinReplicaSetSize);
                Verify.AreEqual(expectDescription.ReplicaRestartWaitDuration, actualDescription.ReplicaRestartWaitDuration);
                Verify.AreEqual(expectDescription.QuorumLossWaitDuration, actualDescription.QuorumLossWaitDuration);
                Verify.AreEqual(expectDescription.StandByReplicaKeepDuration, actualDescription.StandByReplicaKeepDuration);
                Verify.AreEqual(expectDescription.Metrics.Count, actualDescription.Metrics.Count);
                for (var index = 0; index < expectDescription.Metrics.Count; ++index)
                {
                    AreEqual(expectDescription.Metrics[index] as StatefulServiceLoadMetricDescription, actualDescription.Metrics[index] as StatefulServiceLoadMetricDescription);
                }
                for (var index = 0; index < expectDescription.ScalingPolicies.Count; ++index)
                {
                    AreEqual(expectDescription.ScalingPolicies[index], actualDescription.ScalingPolicies[index]);
                }
            }

            break;
            case ServiceDescriptionKind.Stateless:
            {
                var expectDescription = expect as StatelessServiceDescription;
                var actualDescription = actual as StatelessServiceDescription;
                Assert.IsNotNull(expectDescription);
                Assert.IsNotNull(actualDescription);
                Verify.AreEqual(expectDescription.InstanceCount, actualDescription.InstanceCount);
                Verify.AreEqual(expectDescription.Metrics.Count, actualDescription.Metrics.Count);
                for (var index = 0; index < expectDescription.Metrics.Count; ++index)
                {
                    AreEqual(expectDescription.Metrics[index] as StatelessServiceLoadMetricDescription, actualDescription.Metrics[index] as StatelessServiceLoadMetricDescription);
                }
                for (var index = 0; index < expectDescription.ScalingPolicies.Count; ++index)
                {
                    AreEqual(expectDescription.ScalingPolicies[index], actualDescription.ScalingPolicies[index]);
                }
            }

            break;
            default:
                Verify.Fail("Invalid service description kind");
                break;
            }

            Verify.AreEqual(expect.PartitionSchemeDescription.Scheme, actual.PartitionSchemeDescription.Scheme);
            switch (expect.PartitionSchemeDescription.Scheme)
            {
            case PartitionScheme.Singleton:
            {
                var expectDescription = expect.PartitionSchemeDescription as SingletonPartitionSchemeDescription;
                var actualDescription = actual.PartitionSchemeDescription as SingletonPartitionSchemeDescription;
                Assert.IsNotNull(expectDescription);
                Assert.IsNotNull(actualDescription);
            }

            break;
            case PartitionScheme.UniformInt64Range:
            {
                var expectDescription =
                    expect.PartitionSchemeDescription as UniformInt64RangePartitionSchemeDescription;
                var actualDescription =
                    actual.PartitionSchemeDescription as UniformInt64RangePartitionSchemeDescription;
                Assert.IsNotNull(expectDescription);
                Assert.IsNotNull(actualDescription);
                Verify.AreEqual(expectDescription.HighKey, actualDescription.HighKey);
                Verify.AreEqual(expectDescription.LowKey, actualDescription.LowKey);
                Verify.AreEqual(expectDescription.PartitionCount, actualDescription.PartitionCount);
            }

            break;

            case PartitionScheme.Named:
            {
                var expectDescription = expect.PartitionSchemeDescription as NamedPartitionSchemeDescription;
                var actualDescription = actual.PartitionSchemeDescription as NamedPartitionSchemeDescription;
                Assert.IsNotNull(expectDescription);
                Assert.IsNotNull(actualDescription);
                AreEqual(expectDescription.PartitionNames, actualDescription.PartitionNames);
            }

            break;
            default:
                Verify.Fail("Invalid partition description scheme");
                break;
            }

            Verify.AreEqual(expect.Correlations.Count, actual.Correlations.Count);
            for (var index = 0; index < expect.Correlations.Count; ++index)
            {
                AreEqual(expect.Correlations[index], actual.Correlations[index]);
            }

            Verify.AreEqual(expect.PlacementPolicies.Count, actual.PlacementPolicies.Count);
            for (var index = 0; index < expect.PlacementPolicies.Count; ++index)
            {
                AreEqual(expect.PlacementPolicies[index], actual.PlacementPolicies[index]);
            }
        }

        public static void AreEqual(ServiceGroupDescription expect, ServiceGroupDescription actual)
        {
            AreEqual(expect.ServiceDescription, actual.ServiceDescription);
            AreEqual(expect.MemberDescriptions, actual.MemberDescriptions);
        }

        public static void AreEqual(ServiceGroupMemberDescription expect, ServiceGroupMemberDescription actual)
        {
            Assert.AreEqual(expect.ServiceName, actual.ServiceName);
            Assert.AreEqual(expect.ServiceTypeName, actual.ServiceTypeName);
        }

        public static void AreEqual(IList<ServiceGroupMemberDescription> expect, IList<ServiceGroupMemberDescription> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                AreEqual(expect[i], actual[i]);
            }
        }

        public static void AreEqual(StatefulServiceLoadMetricDescription expect, StatefulServiceLoadMetricDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Name, actual.Name);
            Verify.AreEqual(expect.Weight, actual.Weight);
            Verify.AreEqual(expect.PrimaryDefaultLoad, actual.PrimaryDefaultLoad);
            Verify.AreEqual(expect.SecondaryDefaultLoad, actual.SecondaryDefaultLoad);
        }

        public static void AreEqual(StatelessServiceLoadMetricDescription expect, StatelessServiceLoadMetricDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Name, actual.Name);
            Verify.AreEqual(expect.Weight, actual.Weight);
            Verify.AreEqual(expect.DefaultLoad, actual.DefaultLoad);
        }

        public static void AreEqual(ScalingPolicyDescription expect, ScalingPolicyDescription actual)
        {
            Verify.AreEqual(expect.ScalingMechanism, actual.ScalingMechanism);
            Verify.AreEqual(expect.ScalingTrigger, actual.ScalingTrigger);
        }

        public static void AreEqual(ScalingMechanismDescription expect, ScalingMechanismDescription actual)
        {
            if (expect.Kind == ScalingMechanismKind.ScalePartitionInstanceCount)
            {
                Verify.AreEqual(expect.Kind, actual.Kind);
                PartitionInstanceCountScaleMechanism expectDerived = expect as PartitionInstanceCountScaleMechanism;
                PartitionInstanceCountScaleMechanism actualDerived = actual as PartitionInstanceCountScaleMechanism;

                Verify.AreEqual(expectDerived.MaxInstanceCount, actualDerived.MaxInstanceCount);
                Verify.AreEqual(expectDerived.MinInstanceCount, actualDerived.MinInstanceCount);
                Verify.AreEqual(expectDerived.ScaleIncrement, actualDerived.ScaleIncrement);
            }
            else if (expect.Kind == ScalingMechanismKind.AddRemoveIncrementalNamedPartition)
            {
                Verify.AreEqual(expect.Kind, actual.Kind);
                AddRemoveIncrementalNamedPartitionScalingMechanism expectDerived = expect as AddRemoveIncrementalNamedPartitionScalingMechanism;
                AddRemoveIncrementalNamedPartitionScalingMechanism actualDerived = actual as AddRemoveIncrementalNamedPartitionScalingMechanism;

                Verify.AreEqual(expectDerived.MaxPartitionCount, actualDerived.MaxPartitionCount);
                Verify.AreEqual(expectDerived.MinPartitionCount, actualDerived.MinPartitionCount);
                Verify.AreEqual(expectDerived.ScaleIncrement, actualDerived.ScaleIncrement);
            }
        }

        public static void AreEqual(ScalingTriggerDescription expect, ScalingTriggerDescription actual)
        {
            if (expect.Kind == ScalingTriggerKind.AveragePartitionLoadTrigger)
            {
                Verify.AreEqual(expect.Kind, actual.Kind);
                AveragePartitionLoadScalingTrigger expectDerived = expect as AveragePartitionLoadScalingTrigger;
                AveragePartitionLoadScalingTrigger actualDerived = actual as AveragePartitionLoadScalingTrigger;

                Verify.AreEqual(expectDerived.LowerLoadThreshold, actualDerived.LowerLoadThreshold);
                Verify.AreEqual(expectDerived.UpperLoadThreshold, actualDerived.UpperLoadThreshold);
                Verify.AreEqual(expectDerived.MetricName, actualDerived.MetricName);
                Verify.AreEqual(expectDerived.ScaleInterval, actualDerived.ScaleInterval);
            }
            else if (expect.Kind == ScalingTriggerKind.AverageServiceLoadTrigger)
            {
                Verify.AreEqual(expect.Kind, actual.Kind);
                AverageServiceLoadScalingTrigger expectDerived = expect as AverageServiceLoadScalingTrigger;
                AverageServiceLoadScalingTrigger actualDerived = actual as AverageServiceLoadScalingTrigger;

                Verify.AreEqual(expectDerived.LowerLoadThreshold, actualDerived.LowerLoadThreshold);
                Verify.AreEqual(expectDerived.UpperLoadThreshold, actualDerived.UpperLoadThreshold);
                Verify.AreEqual(expectDerived.MetricName, actualDerived.MetricName);
                Verify.AreEqual(expectDerived.ScaleInterval, actualDerived.ScaleInterval);
                Verify.AreEqual(expectDerived.UseOnlyPrimaryLoad, actualDerived.UseOnlyPrimaryLoad);
            }
        }

        public static void AreEqual(ServiceCorrelationDescription expect, ServiceCorrelationDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.ServiceName, actual.ServiceName);
            Verify.AreEqual(expect.Scheme, actual.Scheme);
        }

        public static void AreEqual(ServicePlacementPolicyDescription expect, ServicePlacementPolicyDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);

            Verify.AreEqual(expect.Type, actual.Type);

            if (expect.Type == ServicePlacementPolicyType.RequireDomain)
            {
                var expectCasted = expect as ServicePlacementRequiredDomainPolicyDescription;
                var actualCasted = actual as ServicePlacementRequiredDomainPolicyDescription;

                Verify.IsNotNull(expectCasted);
                Verify.IsNotNull(actualCasted);

                Verify.AreEqual(expectCasted.DomainName, actualCasted.DomainName);
            }
            else if (expect.Type == ServicePlacementPolicyType.InvalidDomain)
            {
                var expectCasted = expect as ServicePlacementInvalidDomainPolicyDescription;
                var actualCasted = actual as ServicePlacementInvalidDomainPolicyDescription;

                Verify.IsNotNull(expectCasted);
                Verify.IsNotNull(actualCasted);

                Verify.AreEqual(expectCasted.DomainName, actualCasted.DomainName);
            }
            else if (expect.Type == ServicePlacementPolicyType.PreferPrimaryDomain)
            {
                var expectCasted = expect as ServicePlacementPreferPrimaryDomainPolicyDescription;
                var actualCasted = actual as ServicePlacementPreferPrimaryDomainPolicyDescription;

                Verify.IsNotNull(expectCasted);
                Verify.IsNotNull(actualCasted);

                Verify.AreEqual(expectCasted.DomainName, actualCasted.DomainName);
            }
            else if (expect.Type == ServicePlacementPolicyType.RequireDomainDistribution)
            {
                var expectCasted = expect as ServicePlacementRequireDomainDistributionPolicyDescription;
                var actualCasted = actual as ServicePlacementRequireDomainDistributionPolicyDescription;

                Verify.IsNotNull(expectCasted);
                Verify.IsNotNull(actualCasted);
            }
            else if (expect.Type == ServicePlacementPolicyType.NonPartiallyPlaceService)
            {
                var expectCasted = expect as ServicePlacementNonPartiallyPlaceServicePolicyDescription;
                var actualCasted = actual as ServicePlacementNonPartiallyPlaceServicePolicyDescription;

                Verify.IsNotNull(expectCasted);
                Verify.IsNotNull(actualCasted);
            }
            else
            {
                Assert.Fail();
            }
        }

        public static void AreEqual(ServiceUpdateDescription expect, ServiceUpdateDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Kind, actual.Kind);
            switch (expect.Kind)
            {
            case ServiceDescriptionKind.Stateful:
            {
                var expectDescription = expect as StatefulServiceUpdateDescription;
                var actualDescription = actual as StatefulServiceUpdateDescription;
                Assert.IsNotNull(expectDescription);
                Assert.IsNotNull(actualDescription);
                Verify.AreEqual(expectDescription.TargetReplicaSetSize, actualDescription.TargetReplicaSetSize);
            }

            break;
            case ServiceDescriptionKind.Stateless:
            {
                var expectDescription = expect as StatelessServiceUpdateDescription;
                var actualDescription = actual as StatelessServiceUpdateDescription;
                Assert.IsNotNull(expectDescription);
                Assert.IsNotNull(actualDescription);
                Verify.AreEqual(expectDescription.InstanceCount, actualDescription.InstanceCount);
            }

            break;
            default:
                Verify.Fail("Invalid service update description kind");
                break;
            }
        }

        public static void AreEqual(ServiceGroupUpdateDescription expect, ServiceGroupUpdateDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            AreEqual(expect.ServiceUpdateDescription, actual.ServiceUpdateDescription);
        }

        public static void AreEqual(ServiceType expect, ServiceType actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.ServiceManifestVersion, actual.ServiceManifestVersion);
            Verify.AreEqual(expect.ServiceTypeDescription, actual.ServiceTypeDescription);
        }

        public static void AreEqual(ServiceTypeDescription expect, ServiceTypeDescription actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.ServiceTypeName, actual.ServiceTypeName);
            Verify.AreEqual(expect.ServiceTypeKind, actual.ServiceTypeKind);
            if (expect.ServiceTypeKind == ServiceDescriptionKind.Stateful)
            {
                var expectDescription = expect as StatefulServiceTypeDescription;
                var actualDescription = actual as StatefulServiceTypeDescription;
                Assert.IsNotNull(expectDescription);
                Assert.IsNotNull(actualDescription);
                Verify.AreEqual(expectDescription.HasPersistedState, actualDescription.HasPersistedState);
            }
            else
            {
                var expectDescription = expect as StatelessServiceTypeDescription;
                var actualDescription = actual as StatelessServiceTypeDescription;
                Assert.IsNotNull(expectDescription);
                Assert.IsNotNull(actualDescription);
                Verify.AreEqual(expectDescription.UseImplicitHost, actualDescription.UseImplicitHost);
            }
        }

        public static void AreEqual(HealthEventsFilter expect, HealthEventsFilter actual)
        {
            if (expect == null)
            {
                Verify.IsNull(actual);
                return;
            }

            Verify.AreEqual(expect.HealthStateFilterValue, actual.HealthStateFilterValue);
        }

        public static void AreEqual(ReplicaHealthStatesFilter expect, ReplicaHealthStatesFilter actual)
        {
            if (expect == null)
            {
                Verify.IsNull(actual);
                return;
            }

            Verify.AreEqual(expect.HealthStateFilterValue, actual.HealthStateFilterValue);
        }

        public static void AreEqual(PartitionHealthStatesFilter expect, PartitionHealthStatesFilter actual)
        {
            if (expect == null)
            {
                Verify.IsNull(actual);
                return;
            }

            Verify.AreEqual(expect.HealthStateFilterValue, actual.HealthStateFilterValue);
        }

        public static void AreEqual(ServiceHealthStatesFilter expect, ServiceHealthStatesFilter actual)
        {
            if (expect == null)
            {
                Verify.IsNull(actual);
                return;
            }

            Verify.AreEqual(expect.HealthStateFilterValue, actual.HealthStateFilterValue);
        }

        public static void AreEqual(ApplicationHealthStatesFilter expect, ApplicationHealthStatesFilter actual)
        {
            if (expect == null)
            {
                Verify.IsNull(actual);
                return;
            }

            Verify.AreEqual(expect.HealthStateFilterValue, actual.HealthStateFilterValue);
        }

        public static void AreEqual(NodeHealthStatesFilter expect, NodeHealthStatesFilter actual)
        {
            if (expect == null)
            {
                Verify.IsNull(actual);
                return;
            }

            Verify.AreEqual(expect.HealthStateFilterValue, actual.HealthStateFilterValue);
        }

        public static void AreEqual(DeployedApplicationHealthStatesFilter expect, DeployedApplicationHealthStatesFilter actual)
        {
            if (expect == null)
            {
                Verify.IsNull(actual);
                return;
            }

            Verify.AreEqual(expect.HealthStateFilterValue, actual.HealthStateFilterValue);
        }

        public static void AreEqual(DeployedServicePackageHealthStatesFilter expect, DeployedServicePackageHealthStatesFilter actual)
        {
            if (expect == null)
            {
                Verify.IsNull(actual);
                return;
            }

            Verify.AreEqual(expect.HealthStateFilterValue, actual.HealthStateFilterValue);
        }

        public static void AreEqual(ApplicationHealthPolicy expect, ApplicationHealthPolicy actual)
        {
            Verify.IsTrue(ApplicationHealthPolicy.AreEqual(expect, actual));
        }

        public static void AreEqual(IList<NodeHealthStateFilter> expect, IList<NodeHealthStateFilter> actual)
        {
            if (expect == null || expect.Count == 0)
            {
                Verify.IsTrue(actual == null || actual.Count == 0);
            }

            Verify.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                Verify.AreEqual(expect[i].NodeNameFilter, actual[i].NodeNameFilter);
                Verify.AreEqual(expect[i].HealthStateFilter, actual[i].HealthStateFilter);
            }
        }

        public static void AreEqual(IList<ApplicationHealthStateFilter> expect, IList<ApplicationHealthStateFilter> actual)
        {
            if (expect == null || expect.Count == 0)
            {
                Verify.IsTrue(actual == null || actual.Count == 0);
            }

            Verify.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                Verify.AreEqual(expect[i].ApplicationNameFilter, actual[i].ApplicationNameFilter);
                Verify.AreEqual(expect[i].ApplicationTypeNameFilter, actual[i].ApplicationTypeNameFilter);
                Verify.AreEqual(expect[i].HealthStateFilter, actual[i].HealthStateFilter);
                AreEqual(expect[i].ServiceFilters, actual[i].ServiceFilters);
                AreEqual(expect[i].DeployedApplicationFilters, actual[i].DeployedApplicationFilters);
            }
        }

        public static void AreEqual(IList<DeployedApplicationHealthStateFilter> expect, IList<DeployedApplicationHealthStateFilter> actual)
        {
            if (expect == null || expect.Count == 0)
            {
                Verify.IsTrue(actual == null || actual.Count == 0);
            }

            Verify.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                Verify.AreEqual(expect[i].NodeNameFilter, actual[i].NodeNameFilter);
                Verify.AreEqual(expect[i].HealthStateFilter, actual[i].HealthStateFilter);
                AreEqual(expect[i].DeployedServicePackageFilters, actual[i].DeployedServicePackageFilters);
            }
        }

        public static void AreEqual(IList<DeployedServicePackageHealthStateFilter> expect, IList<DeployedServicePackageHealthStateFilter> actual)
        {
            if (expect == null || expect.Count == 0)
            {
                Verify.IsTrue(actual == null || actual.Count == 0);
            }

            Verify.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                Verify.AreEqual(expect[i].ServiceManifestNameFilter, actual[i].ServiceManifestNameFilter);
                Verify.AreEqual(expect[i].HealthStateFilter, actual[i].HealthStateFilter);
            }
        }

        public static void AreEqual(IList<ServiceHealthStateFilter> expect, IList<ServiceHealthStateFilter> actual)
        {
            if (expect == null || expect.Count == 0)
            {
                Verify.IsTrue(actual == null || actual.Count == 0);
            }

            Verify.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                Verify.AreEqual(expect[i].ServiceNameFilter, actual[i].ServiceNameFilter);
                Verify.AreEqual(expect[i].HealthStateFilter, actual[i].HealthStateFilter);
                AreEqual(expect[i].PartitionFilters, actual[i].PartitionFilters);
            }
        }

        public static void AreEqual(IList<PartitionHealthStateFilter> expect, IList<PartitionHealthStateFilter> actual)
        {
            if (expect == null || expect.Count == 0)
            {
                Verify.IsTrue(actual == null || actual.Count == 0);
            }

            Verify.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                Verify.AreEqual(expect[i].PartitionIdFilter, actual[i].PartitionIdFilter);
                Verify.AreEqual(expect[i].HealthStateFilter, actual[i].HealthStateFilter);
                AreEqual(expect[i].ReplicaFilters, actual[i].ReplicaFilters);
            }
        }

        public static void AreEqual(IList<ReplicaHealthStateFilter> expect, IList<ReplicaHealthStateFilter> actual)
        {
            if (expect == null || expect.Count == 0)
            {
                Verify.IsTrue(actual == null || actual.Count == 0);
            }

            Verify.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                Verify.AreEqual(expect[i].ReplicaOrInstanceIdFilter, actual[i].ReplicaOrInstanceIdFilter);
                Verify.AreEqual(expect[i].HealthStateFilter, actual[i].HealthStateFilter);
            }
        }

        public static void AreEqual(ClusterHealthPolicy expect, ClusterHealthPolicy actual)
        {
            Verify.IsTrue(ClusterHealthPolicy.AreEqual(expect, actual), "ClusterHealthPolicy.AreEqual(expect, actual)");
        }

        public static void AreEqual(RollingUpgradeMonitoringPolicy expect, RollingUpgradeMonitoringPolicy actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.FailureAction, actual.FailureAction);
            Verify.AreEqual(expect.HealthCheckRetryTimeout, actual.HealthCheckRetryTimeout);
            Verify.AreEqual(expect.HealthCheckWaitDuration, actual.HealthCheckWaitDuration);
            Verify.AreEqual(expect.HealthCheckStableDuration, actual.HealthCheckStableDuration);
            Verify.AreEqual(expect.UpgradeDomainTimeout, actual.UpgradeDomainTimeout);
            Verify.AreEqual(expect.UpgradeTimeout, actual.UpgradeTimeout);
        }

        public static void AreEqual(ApplicationHealth expect, ApplicationHealth actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Assert.AreEqual(expect.AggregatedHealthState, expect.AggregatedHealthState);
            AreEqual(expect.ServiceHealthStates, actual.ServiceHealthStates);
            AreEqual(expect.DeployedApplicationHealthStates, actual.DeployedApplicationHealthStates);
            AreEqual(expect.HealthEvents, actual.HealthEvents);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            AreEqual(expect.HealthStatistics, actual.HealthStatistics);
        }

        public static void AreEqual(ServiceHealth expect, ServiceHealth actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ServiceName, actual.ServiceName);
            Assert.AreEqual(expect.AggregatedHealthState, expect.AggregatedHealthState);
            AreEqual(expect.PartitionHealthStates, actual.PartitionHealthStates);
            AreEqual(expect.HealthEvents, actual.HealthEvents);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            AreEqual(expect.HealthStatistics, actual.HealthStatistics);
        }

        public static void AreEqual(PartitionHealth expect, PartitionHealth actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.PartitionId, actual.PartitionId);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            AreEqual(expect.ReplicaHealthStates, actual.ReplicaHealthStates);
            AreEqual(expect.HealthEvents, actual.HealthEvents);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            AreEqual(expect.HealthStatistics, actual.HealthStatistics);
        }

        public static void AreEqual(ReplicaHealth expect, ReplicaHealth actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.PartitionId, actual.PartitionId);
            Assert.AreEqual(expect.Kind, actual.Kind);
            if (expect is StatefulServiceReplicaHealth)
            {
                var expectHealth = expect as StatefulServiceReplicaHealth;
                var actualHealth = actual as StatefulServiceReplicaHealth;
                Assert.IsNotNull(actualHealth);
                Assert.IsNotNull(actualHealth);
                Assert.AreEqual(expectHealth.ReplicaId, actualHealth.ReplicaId);
            }
            else if (expect is StatelessServiceInstanceHealth)
            {
                var expectHealth = expect as StatelessServiceInstanceHealth;
                var actualHealth = actual as StatelessServiceInstanceHealth;
                Assert.IsNotNull(actualHealth);
                Assert.IsNotNull(actualHealth);
                Assert.AreEqual(expectHealth.InstanceId, actualHealth.InstanceId);
            }

            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            AreEqual(expect.HealthEvents, actual.HealthEvents);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
        }

        public static void AreEqual(NodeHealth expect, NodeHealth actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            AreEqual(expect.HealthEvents, actual.HealthEvents);
        }

        public static void AreEqual(IList<HealthEvaluation> expect, IList<HealthEvaluation> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual, "Expected null");
                return;
            }

            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                Assert.AreEqual(expect[i].Kind, actual[i].Kind);
                Assert.AreEqual(expect[i].Description, actual[i].Description);

                switch (expect[i].Kind)
                {
                case HealthEvaluationKind.Event:
                    AreEqual((EventHealthEvaluation)expect[i], (EventHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.Replicas:
                    AreEqual((ReplicasHealthEvaluation)expect[i], (ReplicasHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.Partitions:
                    AreEqual((PartitionsHealthEvaluation)expect[i], (PartitionsHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.Services:
                    AreEqual((ServicesHealthEvaluation)expect[i], (ServicesHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.DeployedApplications:
                    AreEqual((DeployedApplicationsHealthEvaluation)expect[i], (DeployedApplicationsHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.DeployedServicePackages:
                    AreEqual((DeployedServicePackagesHealthEvaluation)expect[i], (DeployedServicePackagesHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.Applications:
                    AreEqual((ApplicationsHealthEvaluation)expect[i], (ApplicationsHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.ApplicationTypeApplications:
                    AreEqual((ApplicationTypeApplicationsHealthEvaluation)expect[i], (ApplicationTypeApplicationsHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.Nodes:
                    AreEqual((NodesHealthEvaluation)expect[i], (NodesHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.SystemApplication:
                    AreEqual((SystemApplicationHealthEvaluation)expect[i], (SystemApplicationHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.UpgradeDomainNodes:
                    AreEqual((UpgradeDomainNodesHealthEvaluation)expect[i], (UpgradeDomainNodesHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.UpgradeDomainDeployedApplications:
                    AreEqual((UpgradeDomainDeployedApplicationsHealthEvaluation)expect[i], (UpgradeDomainDeployedApplicationsHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.Replica:
                    AreEqual((ReplicaHealthEvaluation)expect[i], (ReplicaHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.Partition:
                    AreEqual((PartitionHealthEvaluation)expect[i], (PartitionHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.Service:
                    AreEqual((ServiceHealthEvaluation)expect[i], (ServiceHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.DeployedApplication:
                    AreEqual((DeployedApplicationHealthEvaluation)expect[i], (DeployedApplicationHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.DeployedServicePackage:
                    AreEqual((DeployedServicePackageHealthEvaluation)expect[i], (DeployedServicePackageHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.Application:
                    AreEqual((ApplicationHealthEvaluation)expect[i], (ApplicationHealthEvaluation)actual[i]);
                    break;
                case HealthEvaluationKind.Node:
                    AreEqual((NodeHealthEvaluation)expect[i], (NodeHealthEvaluation)actual[i]);
                    break;
                default:
                    Assert.Fail("unknown kind {0}", expect[i].Kind);
                    break;
                }
            }
        }

        public static void AreEqual(EventHealthEvaluation expect, EventHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            AreEqual(expect.UnhealthyEvent, actual.UnhealthyEvent);
            Assert.AreEqual(expect.ConsiderWarningAsError, actual.ConsiderWarningAsError);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(ReplicasHealthEvaluation expect, ReplicasHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.MaxPercentUnhealthyReplicasPerPartition, actual.MaxPercentUnhealthyReplicasPerPartition);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(NodesHealthEvaluation expect, NodesHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.MaxPercentUnhealthyNodes, actual.MaxPercentUnhealthyNodes);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(PartitionsHealthEvaluation expect, PartitionsHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.MaxPercentUnhealthyPartitionsPerService, actual.MaxPercentUnhealthyPartitionsPerService);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(ServicesHealthEvaluation expect, ServicesHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.MaxPercentUnhealthyServices, actual.MaxPercentUnhealthyServices);
            Assert.AreEqual(expect.ServiceTypeName, actual.ServiceTypeName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(DeployedServicePackagesHealthEvaluation expect, DeployedServicePackagesHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(DeployedApplicationsHealthEvaluation expect, DeployedApplicationsHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.MaxPercentUnhealthyDeployedApplications, actual.MaxPercentUnhealthyDeployedApplications);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(ApplicationsHealthEvaluation expect, ApplicationsHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.MaxPercentUnhealthyApplications, actual.MaxPercentUnhealthyApplications);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(ApplicationTypeApplicationsHealthEvaluation expect, ApplicationTypeApplicationsHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.MaxPercentUnhealthyApplications, actual.MaxPercentUnhealthyApplications);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            Assert.AreEqual(expect.Description, actual.Description);
            Assert.AreEqual(expect.ApplicationTypeName, actual.ApplicationTypeName);
        }

        public static void AreEqual(SystemApplicationHealthEvaluation expect, SystemApplicationHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(UpgradeDomainNodesHealthEvaluation expect, UpgradeDomainNodesHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.UpgradeDomainName, actual.UpgradeDomainName);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.MaxPercentUnhealthyNodes, actual.MaxPercentUnhealthyNodes);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(UpgradeDomainDeployedApplicationsHealthEvaluation expect, UpgradeDomainDeployedApplicationsHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.UpgradeDomainName, actual.UpgradeDomainName);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.MaxPercentUnhealthyDeployedApplications, actual.MaxPercentUnhealthyDeployedApplications);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(ServiceHealthEvaluation expect, ServiceHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ServiceName, actual.ServiceName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(NodeHealthEvaluation expect, NodeHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(ApplicationHealthEvaluation expect, ApplicationHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(DeployedApplicationHealthEvaluation expect, DeployedApplicationHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(DeployedServicePackageHealthEvaluation expect, DeployedServicePackageHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.Description, actual.Description);
            Assert.AreEqual(expect.ServiceManifestName, actual.ServiceManifestName);
        }

        public static void AreEqual(PartitionHealthEvaluation expect, PartitionHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.PartitionId, actual.PartitionId);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(ReplicaHealthEvaluation expect, ReplicaHealthEvaluation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.PartitionId, actual.PartitionId);
            Assert.AreEqual(expect.Kind, actual.Kind);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            AreEqual(expect.UnhealthyEvaluations, actual.UnhealthyEvaluations);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        public static void AreEqual(ServiceHealthState expect, ServiceHealthState actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ServiceName, actual.ServiceName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
        }

        public static void AreEqual(NodeHealthState expect, NodeHealthState actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
        }

        public static void AreEqual(ApplicationHealthState expect, ApplicationHealthState actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
        }

        public static void AreEqual(DeployedApplicationHealthState expect, DeployedApplicationHealthState actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
        }

        public static void AreEqual(PartitionHealthState expect, PartitionHealthState actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.PartitionId, actual.PartitionId);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
        }

        public static void AreEqual(ReplicaHealthState expect, ReplicaHealthState actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.PartitionId, actual.PartitionId);
            Assert.AreEqual(expect.Kind, actual.Kind);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
        }

        public static void AreEqual(HealthEvent expect, HealthEvent actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.HealthInformation.Description, actual.HealthInformation.Description);
            Assert.AreEqual(expect.LastModifiedUtcTimestamp, actual.LastModifiedUtcTimestamp);
            Assert.AreEqual(expect.HealthInformation.Property, actual.HealthInformation.Property);
            Assert.AreEqual(expect.HealthInformation.SequenceNumber, actual.HealthInformation.SequenceNumber);
            Assert.AreEqual(expect.HealthInformation.SourceId, actual.HealthInformation.SourceId);
            Assert.AreEqual(expect.SourceUtcTimestamp, actual.SourceUtcTimestamp);
            Assert.AreEqual(expect.HealthInformation.HealthState, actual.HealthInformation.HealthState);
            Assert.AreEqual(expect.HealthInformation.TimeToLive, actual.HealthInformation.TimeToLive);
        }

        public static void AreEqual(DeployedCodePackage expect, DeployedCodePackage actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.CodePackageName, actual.CodePackageName);
            Assert.AreEqual(expect.CodePackageVersion, actual.CodePackageVersion);
            AreEqual(expect.EntryPoint, actual.EntryPoint);
            AreEqual(expect.SetupEntryPoint, actual.SetupEntryPoint);
        }

        public static void AreEqual(CodePackageEntryPoint expect, CodePackageEntryPoint actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.EntryPointLocation, actual.EntryPointLocation);
            Assert.AreEqual(expect.ProcessId, actual.ProcessId);
            Assert.AreEqual(expect.RunAsUserName, actual.RunAsUserName);
            AreEqual(expect.Statistics, actual.Statistics);
        }

        public static void AreEqual(CodePackageEntryPointStatistics expect, CodePackageEntryPointStatistics actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ActivationCount, actual.ActivationCount);
            Assert.AreEqual(expect.ActivationFailureCount, actual.ActivationFailureCount);
            Assert.AreEqual(expect.ContinuousActivationFailureCount, actual.ContinuousActivationFailureCount);
            Assert.AreEqual(expect.ContinuousExitFailureCount, actual.ContinuousExitFailureCount);
            Assert.AreEqual(expect.ExitCount, actual.ExitCount);
            Assert.AreEqual(expect.ExitFailureCount, actual.ExitFailureCount);
            Assert.AreEqual(expect.LastActivationUtc, actual.LastActivationUtc);
            Assert.AreEqual(expect.LastExitCode, actual.LastExitCode);
            Assert.AreEqual(expect.LastExitUtc, actual.LastExitUtc);
            Assert.AreEqual(expect.LastSuccessfulActivationUtc, actual.LastSuccessfulActivationUtc);
            Assert.AreEqual(expect.LastSuccessfulExitUtc, actual.LastSuccessfulExitUtc);
        }

        public static void AreEqual(DeployedStatefulServiceReplica expect, DeployedStatefulServiceReplica actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.Address, actual.Address);
            Assert.AreEqual(expect.CodePackageName, actual.CodePackageName);
            Assert.AreEqual(expect.Partitionid, actual.Partitionid);
            Assert.AreEqual(expect.ReplicaId, actual.ReplicaId);
            Assert.AreEqual(expect.ReplicaRole, actual.ReplicaRole);
            Assert.AreEqual(expect.ReplicaStatus, actual.ReplicaStatus);
            Assert.AreEqual(expect.ServiceName, actual.ServiceName);
            Assert.AreEqual(expect.ServiceTypeName, actual.ServiceTypeName);
            Assert.AreEqual(expect.ServiceManifestVersion_, actual.ServiceManifestVersion_);
            Assert.AreEqual(expect.ServiceManifestName, actual.ServiceManifestName);
        }

        public static void AreEqual(DeployedServiceReplicaDetail expect, DeployedServiceReplicaDetail actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ServiceName, actual.ServiceName);
            Assert.AreEqual(expect.CurrentServiceOperation, actual.CurrentServiceOperation);
            Assert.AreEqual(expect.CurrentServiceOperationStartTimeUtc, actual.CurrentServiceOperationStartTimeUtc);
            Assert.AreEqual(expect.PartitionId, actual.PartitionId);
        }

        public static void AreEqual(DeployedServicePackage expect, DeployedServicePackage actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ServiceManifestName, actual.ServiceManifestName);
            Assert.AreEqual(expect.ServiceManifestVersion, actual.ServiceManifestVersion);
        }

        public static void AreEqual(DeployedServiceType expect, DeployedServiceType actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.CodePackageName, actual.CodePackageName);
            Assert.AreEqual(expect.ServiceTypeName, actual.ServiceTypeName);
            Assert.AreEqual(expect.ServiceTypeRegistrationStatus, actual.ServiceTypeRegistrationStatus);
        }

        public static void AreEqual(DeployedApplicationHealth expect, DeployedApplicationHealth actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            AreEqual(expect.DeployedServicePackageHealthStates, actual.DeployedServicePackageHealthStates);
            AreEqual(expect.HealthEvents, actual.HealthEvents);
            AreEqual(expect.HealthStatistics, actual.HealthStatistics);
        }

        public static void AreEqual(DeployedServicePackageHealth expect, DeployedServicePackageHealth actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Assert.AreEqual(expect.ServiceManifestName, actual.ServiceManifestName);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            AreEqual(expect.HealthEvents, actual.HealthEvents);
        }

        public static void AreEqual(DeployedServicePackageHealthState expect, DeployedServicePackageHealthState actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Assert.AreEqual(expect.ServiceManifestName, actual.ServiceManifestName);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
        }

        public static void AreEqual(ResolvedServicePartition expect, ResolvedServicePartition actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ServiceName, actual.ServiceName);
            AreEqual(expect.Info, actual.Info);
            AreEqual(expect.Endpoints, actual.Endpoints);
        }

        public static void AreEqual(ServicePartitionInformation expect, ServicePartitionInformation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.Id, actual.Id);
            Assert.AreEqual(expect.Kind, actual.Kind);

            if (expect.Kind == ServicePartitionKind.Int64Range)
            {
                var expectInfo = expect as Int64RangePartitionInformation;
                var actualInfo = actual as Int64RangePartitionInformation;
                Assert.AreEqual(expectInfo.HighKey, actualInfo.HighKey);
                Assert.AreEqual(expectInfo.LowKey, actualInfo.LowKey);
            }
            else if (expect.Kind == ServicePartitionKind.Named)
            {
                var expectInfo = expect as NamedPartitionInformation;
                var actualInfo = actual as NamedPartitionInformation;
                Assert.AreEqual(expectInfo.Name, actualInfo.Name);
            }
        }

        public static void AreEqual(ResolvedServiceEndpoint expect, ResolvedServiceEndpoint actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.Address, actual.Address);
            Assert.AreEqual(expect.Role, actual.Role);
        }

        public static void AreEqual(ClusterHealth expect, ClusterHealth actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.AggregatedHealthState, actual.AggregatedHealthState);
            AreEqual(expect.NodeHealthStates, actual.NodeHealthStates);
            AreEqual(expect.ApplicationHealthStates, actual.ApplicationHealthStates);
            AreEqual(expect.HealthEvents, actual.HealthEvents);
            AreEqual(expect.HealthStatistics, actual.HealthStatistics);
        }

        public static void AreEqual(ClusterHealthChunk expect, ClusterHealthChunk actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.HealthState, actual.HealthState);
            AreEqual(expect.NodeHealthStateChunks, actual.NodeHealthStateChunks);
            AreEqual(expect.ApplicationHealthStateChunks, actual.ApplicationHealthStateChunks);
        }

        public static void AreEqual(NodeHealthStateChunkList expect, NodeHealthStateChunkList actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.TotalCount, actual.TotalCount);
            Assert.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                AreEqual(expect[i], actual[i]);
            }
        }

        public static void AreEqual(NodeHealthStateChunk expect, NodeHealthStateChunk actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.HealthState, actual.HealthState);
        }

        public static void AreEqual(ApplicationHealthStateChunkList expect, ApplicationHealthStateChunkList actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.TotalCount, actual.TotalCount);
            Assert.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                AreEqual(expect[i], actual[i]);
            }
        }

        public static void AreEqual(ApplicationHealthStateChunk expect, ApplicationHealthStateChunk actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Assert.AreEqual(expect.ApplicationTypeName, actual.ApplicationTypeName);
            Assert.AreEqual(expect.HealthState, actual.HealthState);
            AreEqual(expect.ServiceHealthStateChunks, actual.ServiceHealthStateChunks);
            AreEqual(expect.DeployedApplicationHealthStateChunks, actual.DeployedApplicationHealthStateChunks);
        }

        public static void AreEqual(DeployedApplicationHealthStateChunkList expect, DeployedApplicationHealthStateChunkList actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.TotalCount, actual.TotalCount);
            Assert.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                AreEqual(expect[i], actual[i]);
            }
        }

        public static void AreEqual(DeployedApplicationHealthStateChunk expect, DeployedApplicationHealthStateChunk actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.HealthState, actual.HealthState);
            AreEqual(expect.DeployedServicePackageHealthStateChunks, actual.DeployedServicePackageHealthStateChunks);
        }

        public static void AreEqual(DeployedServicePackageHealthStateChunkList expect, DeployedServicePackageHealthStateChunkList actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.TotalCount, actual.TotalCount);
            Assert.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                AreEqual(expect[i], actual[i]);
            }
        }

        public static void AreEqual(DeployedServicePackageHealthStateChunk expect, DeployedServicePackageHealthStateChunk actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ServiceManifestName, actual.ServiceManifestName);
            Assert.AreEqual(expect.HealthState, actual.HealthState);
        }

        public static void AreEqual(ServiceHealthStateChunkList expect, ServiceHealthStateChunkList actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.TotalCount, actual.TotalCount);
            Assert.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                AreEqual(expect[i], actual[i]);
            }
        }

        public static void AreEqual(ServiceHealthStateChunk expect, ServiceHealthStateChunk actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ServiceName, actual.ServiceName);
            Assert.AreEqual(expect.HealthState, actual.HealthState);
            AreEqual(expect.PartitionHealthStateChunks, actual.PartitionHealthStateChunks);
        }

        public static void AreEqual(PartitionHealthStateChunkList expect, PartitionHealthStateChunkList actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.TotalCount, actual.TotalCount);
            Assert.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                AreEqual(expect[i], actual[i]);
            }
        }

        public static void AreEqual(PartitionHealthStateChunk expect, PartitionHealthStateChunk actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.PartitionId, actual.PartitionId);
            Assert.AreEqual(expect.HealthState, actual.HealthState);
            AreEqual(expect.ReplicaHealthStateChunks, actual.ReplicaHealthStateChunks);
        }

        public static void AreEqual(ReplicaHealthStateChunkList expect, ReplicaHealthStateChunkList actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.TotalCount, actual.TotalCount);
            Assert.AreEqual(expect.Count, actual.Count);
            for (int i = 0; i < expect.Count; i++)
            {
                AreEqual(expect[i], actual[i]);
            }
        }

        public static void AreEqual(ReplicaHealthStateChunk expect, ReplicaHealthStateChunk actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.ReplicaOrInstanceId, actual.ReplicaOrInstanceId);
            Assert.AreEqual(expect.HealthState, actual.HealthState);
        }

        /// <summary>
        /// Asserts that two objects of type "HealthInformation" are equal.
        /// </summary>
        public static void AreEqual(HealthInformation expect, HealthInformation actual)
        {
            /// None of the two should be null
            AssertNotNull(expect, actual);

            /// Both objects refering to same instance? then Done!
            if (object.ReferenceEquals(expect, actual))
            {
                return;
            }

            Assert.IsTrue(expect.HealthState == actual.HealthState);
            Assert.AreEqual(expect.Property, actual.Property);
            Assert.AreEqual(expect.SourceId, actual.SourceId);
            Assert.AreEqual(expect.SourceId, actual.SourceId);
            Assert.AreEqual(expect.Description, actual.Description);
        }

        /// <summary>
        /// Asserts that two objects of type "HealthReport" are equal.
        /// </summary>
        public static void AreEqual(HealthReport expect, HealthReport actual)
        {
            /// None of the two should be null
            AssertNotNull(expect, actual);

            /// Both objects refering to same instance? then Done!
            if (object.ReferenceEquals(expect, actual))
            {
                return;
            }

            /// Compare fields of the two objects:
            Assert.IsTrue(expect.Kind == actual.Kind); ///Kind is Enum type.

            /// Call AreEqual method for relevant derived class of HealthReport.
            if (expect is ApplicationHealthReport && actual is ApplicationHealthReport)
            {
                AreEqual(expect as ApplicationHealthReport, actual as ApplicationHealthReport);
            }
            else if (expect is NodeHealthReport && actual is NodeHealthReport)
            {
                AreEqual(expect as NodeHealthReport, actual as NodeHealthReport);
            }
            else if (expect is PartitionHealthReport && actual is PartitionHealthReport)
            {
                AreEqual(expect as PartitionHealthReport, actual as PartitionHealthReport);
            }
            else if (expect is ServiceHealthReport && actual is ServiceHealthReport)
            {
                AreEqual(expect as ServiceHealthReport, actual as ServiceHealthReport);
            }
            else if (expect is StatefulServiceReplicaHealthReport && actual is StatefulServiceReplicaHealthReport)
            {
                AreEqual(expect as StatefulServiceReplicaHealthReport, actual as StatefulServiceReplicaHealthReport);
            }
            else if (expect is StatelessServiceInstanceHealthReport && actual is StatelessServiceInstanceHealthReport)
            {
                AreEqual(expect as StatelessServiceInstanceHealthReport, actual as StatelessServiceInstanceHealthReport);
            }
            else if (expect is DeployedApplicationHealthReport && actual is DeployedApplicationHealthReport)
            {
                AreEqual(expect as DeployedApplicationHealthReport, actual as DeployedApplicationHealthReport);
            }
            else if (expect is DeployedServicePackageHealthReport && actual is DeployedServicePackageHealthReport)
            {
                AreEqual(expect as DeployedServicePackageHealthReport, actual as DeployedServicePackageHealthReport);
            }
            else if (expect is ClusterHealthReport && actual is ClusterHealthReport)
            {
                AreEqual(expect as ClusterHealthReport, actual as ClusterHealthReport);
            }
            else /// No method found for this type.. comparing what can be at this point.
            {
                AreEqual(expect.HealthInformation, actual.HealthInformation);
                Assert.Fail("There is not supported AreEqual({0}, {1}) methond for healthReports yet.", expect.GetType(), actual.GetType());
            }
        }

        /// <summary>
        /// Asserts that two objects of type "ApplicationHealthReport" are equal.
        /// </summary>
        public static void AreEqual(ApplicationHealthReport expect, ApplicationHealthReport actual)
        {
            /// None of the two should be null.
            AssertNotNull(expect, actual);

            /// Both objects refering to same instance? then Done!
            if (object.ReferenceEquals(expect, actual))
            {
                return;
            }

            /// Compare fields of two objects:
            Assert.AreEqual(expect.ApplicationName, actual.ApplicationName);
            AreEqual(expect.HealthInformation, actual.HealthInformation);
        }

        /// <summary>
        /// Asserts that two objects of type "ClusterHealthReport" are equal.
        /// </summary>
        public static void AreEqual(ClusterHealthReport expect, ClusterHealthReport actual)
        {
            /// None of the two should be null.
            AssertNotNull(expect, actual);

            /// Both objects refering to same instance? then Done!
            if (object.ReferenceEquals(expect, actual))
            {
                return;
            }

            /// Compare fields of two objects:
            AreEqual(expect.HealthInformation, actual.HealthInformation);
        }

        /// <summary>
        /// Asserts that two objects of type "NodeHealthReport" are equal.
        /// </summary>
        public static void AreEqual(NodeHealthReport expect, NodeHealthReport actual)
        {
            /// None of the two should be null.
            AssertNotNull(expect, actual);

            /// Both objects refering to same instance? then Done!
            if (object.ReferenceEquals(expect, actual))
            {
                return;
            }

            /// Compare fields of two objects:
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            AreEqual(expect.HealthInformation, actual.HealthInformation);
        }

        /// <summary>
        /// Asserts that two objects of type "PartitionHealthReport" are equal.
        /// </summary>
        public static void AreEqual(PartitionHealthReport expect, PartitionHealthReport actual)
        {
            /// None of the two should be null.
            AssertNotNull(expect, actual);

            /// Both objects refering to same instance? then Done!
            if (object.ReferenceEquals(expect, actual))
            {
                return;
            }

            /// Compare fields of two objects:
            Assert.AreEqual(expect.PartitionId, actual.PartitionId);
            AreEqual(expect.HealthInformation, actual.HealthInformation);
        }

        /// <summary>
        /// Asserts that two objects of type "ServiceHealthReport" are equal.
        /// </summary>
        public static void AreEqual(ServiceHealthReport expect, ServiceHealthReport actual)
        {
            /// None of the two should be null.
            AssertNotNull(expect, actual);

            /// Both objects refering to same instance? then Done!
            if (object.ReferenceEquals(expect, actual))
            {
                return;
            }

            /// Compare fields of two objects:
            Assert.AreEqual(expect.ServiceName, actual.ServiceName);
            AreEqual(expect.HealthInformation, actual.HealthInformation);
        }

        /// <summary>
        /// Asserts that two objects of type "DeployedApplicationHealthReport" are equal.
        /// </summary>
        public static void AreEqual(DeployedApplicationHealthReport expect, DeployedApplicationHealthReport actual)
        {
            /// None of the two should be null.
            AssertNotNull(expect, actual);

            /// Both objects refering to same instance? then Done!
            if (object.ReferenceEquals(expect, actual))
            {
                return;
            }

            /// Compare fields of two objects:
            Assert.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            AreEqual(expect.HealthInformation, actual.HealthInformation);
        }

        /// <summary>
        /// Asserts that two objects of type "DeployedServicePackageHealthReport" are equal.
        /// </summary>
        public static void AreEqual(DeployedServicePackageHealthReport expect, DeployedServicePackageHealthReport actual)
        {
            /// None of the two should be null.
            AssertNotNull(expect, actual);

            /// Both objects refering to same instance? then Done!
            if (object.ReferenceEquals(expect, actual))
            {
                return;
            }

            /// Compare fields of two objects:
            Assert.AreEqual(expect.ApplicationName, actual.ApplicationName);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.ServiceManifestName, actual.ServiceManifestName);
            AreEqual(expect.HealthInformation, actual.HealthInformation);
        }

        /// <summary>
        /// Asserts that two objects of type "StatefulServiceReplicaHealthReport" are equal.
        /// </summary>
        public static void AreEqual(StatefulServiceReplicaHealthReport expect, StatefulServiceReplicaHealthReport actual)
        {
            /// None of the two should be null.
            AssertNotNull(expect, actual);

            /// Both objects refering to same instance? then Done!
            if (object.ReferenceEquals(expect, actual))
            {
                return;
            }

            /// Compare fields of two objects:
            Assert.AreEqual(expect.PartitionId, actual.PartitionId);
            Assert.AreEqual(expect.ReplicaId, actual.ReplicaId);
            AreEqual(expect.HealthInformation, actual.HealthInformation);
        }

        /// <summary>
        /// Asserts that two objects of type "StatefulServiceReplicaHealthReport" are equal.
        /// </summary>
        public static void AreEqual(StatelessServiceInstanceHealthReport expect, StatelessServiceInstanceHealthReport actual)
        {
            /// None of the two should be null.
            AssertNotNull(expect, actual);

            /// Both objects refering to same instance? then Done!
            if (object.ReferenceEquals(expect, actual))
            {
                return;
            }

            /// Compare fields of two objects:
            Assert.AreEqual(expect.PartitionId, actual.PartitionId);
            Assert.AreEqual(expect.InstanceId, actual.InstanceId);
            AreEqual(expect.HealthInformation, actual.HealthInformation);
        }

        public static void AreEqual(Node expect, Node actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            if (object.ReferenceEquals(expect, actual))
            {
                return;
            }

            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.IpAddressOrFQDN, actual.IpAddressOrFQDN);
            Assert.AreEqual(expect.NodeType, actual.NodeType);
            Assert.AreEqual(expect.CodeVersion, actual.CodeVersion);
            Assert.AreEqual(expect.ConfigVersion, actual.ConfigVersion);
            Assert.AreEqual(expect.NodeStatus, actual.NodeStatus);
            Assert.AreEqual(expect.NodeUpAt, actual.NodeUpAt);
            Assert.AreEqual(expect.NodeDownAt, actual.NodeDownAt);
            Assert.AreEqual(expect.HealthState, actual.HealthState);
            Assert.AreEqual(expect.IsSeedNode, actual.IsSeedNode);
            Assert.AreEqual(expect.UpgradeDomain, actual.UpgradeDomain);
            Assert.AreEqual(expect.FaultDomain, actual.FaultDomain);
            Assert.AreEqual(expect.NodeId, actual.NodeId);
            Assert.AreEqual(expect.NodeInstanceId, actual.NodeInstanceId);
            AreEqual(expect.NodeDeactivationInfo, actual.NodeDeactivationInfo);
        }

        public static void AreEqual(NodeDeactivationResult expect, NodeDeactivationResult actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual, "NodeDeactivationResult");
            if (object.ReferenceEquals(expect, actual))
            {
                return;
            }

            Assert.AreEqual(expect.EffectiveIntent, actual.EffectiveIntent);
            Assert.AreEqual(expect.Status, actual.Status);
            AreEqual(expect.Tasks, actual.Tasks);
        }

        public static void AreEqual(
            IList<NodeDeactivationTask> expect, IList<NodeDeactivationTask> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual, "IList<NodeDeactivationTask>");
            Verify.AreEqual(expect.Count, actual.Count);
            for (var index = 0; index < expect.Count; index++)
            {
                AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(NodeDeactivationTask expect, NodeDeactivationTask actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);

            Assert.AreEqual(expect.Intent, actual.Intent);
            AreEqual(expect.TaskId, actual.TaskId);
        }

        public static void AreEqual(NodeDeactivationTaskId expect, NodeDeactivationTaskId actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.Id, actual.Id);
            Assert.AreEqual(expect.Type, actual.Type);
        }

        public static void AreEqual(string expect, string actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Verify.AreEqual(expect, actual);
        }

        /// <summary>
        /// Asserts that the none of the two objects are null.
        /// </summary>
        public static void AssertNotNull(object object1, object object2)
        {
            Assert.IsNotNull(object1);
            Assert.IsNotNull(object2);
        }

        public static void AssertAreEqual(object object1, object object2)
        {
            AssertNotNull(object1, object2);
            Assert.AreEqual(object1.GetType(), object2.GetType());

            foreach (var propertyTypePair in GetResultObjectPropertyInfo(object1.GetType()))
            {
                var result1 = GetPropertyValue(object1, propertyTypePair.Key);
                var result2 = GetPropertyValue(object2, propertyTypePair.Key);
                Assert.AreEqual(result1, result2);
            }
        }

        public static Dictionary<string, Type> GetResultObjectPropertyInfo(Type t)
        {
            Dictionary<string, Type> properties = new Dictionary<string, Type>();
            foreach (var p in t.GetProperties())
            {
                properties.Add(p.Name, p.PropertyType);
            }

            return properties;
        }

        public static object GetPropertyValue(object result, string propertyName)
        {
            Dictionary<string, Type> properties = GetResultObjectPropertyInfo(result.GetType());
            if (properties.ContainsKey(propertyName))
            {
                return ExtractResultForQueryProperty(propertyName, result.GetType(), result);
            }
            else
            {
                return null;
            }
        }

        public static bool TryGetPropertyValue(object result, string propertyName, out object returnValue)
        {
            try
            {
                returnValue = GetPropertyValue(result, propertyName);
            }
            catch
            {
                returnValue = null;
                return false;
            }

            return true;
        }

        public static object ExtractResultForQueryProperty(string propertyName, Type t, object resultobj)
        {
            // change to use property info object, make sure property info is not taking more memory
            return t.InvokeMember(propertyName, System.Reflection.BindingFlags.GetProperty, null, resultobj, null);
        }

        public static void AreEqual(LoadMetricInformation expect, LoadMetricInformation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.Name, actual.Name);
            Assert.AreEqual(expect.IsBalancedBefore, actual.IsBalancedBefore);
            Assert.AreEqual(expect.IsBalancedAfter, actual.IsBalancedAfter);
            Assert.AreEqual(expect.DeviationBefore, actual.DeviationBefore);
            Assert.AreEqual(expect.DeviationAfter, actual.DeviationAfter);
            Assert.AreEqual(expect.BalancingThreshold, actual.BalancingThreshold);
            Assert.AreEqual(expect.Action, actual.Action);
            Assert.AreEqual(expect.ActivityThreshold, actual.ActivityThreshold);
            Assert.AreEqual(expect.ClusterCapacity, actual.ClusterCapacity);
            Assert.AreEqual(expect.ClusterLoad, actual.ClusterLoad);
            Assert.AreEqual(expect.CurrentClusterLoad, actual.CurrentClusterLoad);
            Assert.AreEqual(expect.ClusterRemainingCapacity, actual.ClusterRemainingCapacity);
            Assert.AreEqual(expect.ClusterCapacityRemaining, actual.ClusterCapacityRemaining);
            Assert.AreEqual(expect.IsClusterCapacityViolation, actual.IsClusterCapacityViolation);
            Assert.AreEqual(expect.NodeBufferPercentage, actual.NodeBufferPercentage);
            Assert.AreEqual(expect.ClusterBufferedCapacity, actual.ClusterBufferedCapacity);
            Assert.AreEqual(expect.BufferedClusterCapacityRemaining, actual.BufferedClusterCapacityRemaining);
            Assert.AreEqual(expect.ClusterRemainingBufferedCapacity, actual.ClusterRemainingBufferedCapacity);
            Assert.AreEqual(expect.MinNodeLoadValue, actual.MinNodeLoadValue);
            Assert.AreEqual(expect.MinimumNodeLoad, actual.MinimumNodeLoad);
            Assert.AreEqual(expect.MinNodeLoadNodeId, actual.MinNodeLoadNodeId);
            Assert.AreEqual(expect.MaxNodeLoadValue, actual.MaxNodeLoadValue);
            Assert.AreEqual(expect.MaximumNodeLoad, actual.MaximumNodeLoad);
            Assert.AreEqual(expect.MaxNodeLoadNodeId, actual.MaxNodeLoadNodeId);
        }

        public static void AreEqual(IList<LoadMetricInformation> expect, IList<LoadMetricInformation> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            for (var index = 0; index < expect.Count; ++index)
            {
                AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(ClusterLoadInformation expect, ClusterLoadInformation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.LastBalancingStartTimeUtc, actual.LastBalancingStartTimeUtc);
            Assert.AreEqual(expect.LastBalancingEndTimeUtc, actual.LastBalancingEndTimeUtc);
            Assert.AreEqual(expect.LoadMetricInformationList, actual.LoadMetricInformationList);
        }

        public static void AreEqual(NodeLoadMetricInformation expect, NodeLoadMetricInformation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.NodeCapacity, actual.NodeCapacity);
            Assert.AreEqual(expect.NodeLoad, actual.NodeLoad);
            Assert.AreEqual(expect.CurrentNodeLoad, actual.CurrentNodeLoad);
            Assert.AreEqual(expect.NodeRemainingCapacity, actual.NodeRemainingCapacity);
            Assert.AreEqual(expect.NodeCapacityRemaining, actual.NodeCapacityRemaining);
            Assert.AreEqual(expect.IsCapacityViolation, actual.IsCapacityViolation);
            Assert.AreEqual(expect.NodeBufferedCapacity, actual.NodeBufferedCapacity);
            Assert.AreEqual(expect.NodeRemainingBufferedCapacity, actual.NodeRemainingBufferedCapacity);
            Assert.AreEqual(expect.BufferedNodeCapacityRemaining, actual.BufferedNodeCapacityRemaining);
        }
        public static void AreEqual(IList<NodeLoadMetricInformation> expect, IList<NodeLoadMetricInformation> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            for (var index = 0; index < expect.Count; ++index)
            {
                AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(NodeLoadInformation expect, NodeLoadInformation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.NodeName, actual.NodeName);
            Assert.AreEqual(expect.NodeLoadMetricInformationList, actual.NodeLoadMetricInformationList);
        }

        public static void AreEqual(LoadMetricReport expect, LoadMetricReport actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.Name, actual.Name);
            Assert.AreEqual(expect.Value, actual.Value);
            Assert.AreEqual(expect.CurrentValue, actual.CurrentValue);
            Assert.AreEqual(expect.LastReportedUtc, actual.LastReportedUtc);
        }

        public static void AreEqual(IList<LoadMetricReport> expect, IList<LoadMetricReport> actual)
        {
            if (expect == null)
            {
                Assert.IsNull(actual);
                return;
            }

            Assert.IsNotNull(actual);
            Verify.AreEqual(expect.Count, actual.Count);
            for (var index = 0; index < expect.Count; ++index)
            {
                AreEqual(expect[index], actual[index]);
            }
        }

        public static void AreEqual(PartitionLoadInformation expect, PartitionLoadInformation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.PartitionId, actual.PartitionId);
            Assert.AreEqual(expect.PrimaryLoadMetricReports, actual.PrimaryLoadMetricReports);
            Assert.AreEqual(expect.SecondaryLoadMetricReports, actual.SecondaryLoadMetricReports);
        }

        public static void AreEqual(ReplicaLoadInformation expect, ReplicaLoadInformation actual)
        {
            Assert.IsNotNull(expect);
            Assert.IsNotNull(actual);
            Assert.AreEqual(expect.PartitionId, actual.PartitionId);
            Assert.AreEqual(expect.ReplicaOrInstanceId, actual.ReplicaOrInstanceId);
            Assert.AreEqual(expect.LoadMetricReports, actual.LoadMetricReports);
        }
    }
}