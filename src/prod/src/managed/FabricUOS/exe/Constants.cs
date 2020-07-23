// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Common;

    internal class Constants
    {
        #region Fabric Orchestration Service Constants

        public const string AdminConfigUpgradeAttemptedDictionaryKey = "AdminConfigUpgradeAttempted";
        public const string ClusterReliableDictionaryName = "SAClusterTable";
        public const string ClusterReliableDictionaryKey = "SAClusterKey";
        public const string ClusterManifestTempFileName = "ClusterManifest.xml";
        public const string SectionName = "UpgradeOrchestrationService";
        public const string ClusterIdParameterName = "ClusterId";
        public const string ClusterInitializationCodeVersion = "0.0.0.0";
        public const string WindowsFabricStoreName = "WindowsFabricStore";
        public const string FASServiceName = "fabric:/System/FAS";
        public const string FASServiceNameUri = "fabric:/System/FaultAnalysisService";
        public const string UOSServiceName = "fabric:/System/UOS";
        public const string UOSServiceNameUri = "fabric:/System/UpgradeOrchestrationService";
        public const string UpgradeOrchestrationHealthSourceId = "System.UpgradeOrchestrationService";
        public const string DatalossHealthProperty = "Dataloss";
        public const string SecuritySectionName = "Security";
        public const string DisableFirewallRuleForPublicProfilePropertyName = "DisableFirewallRuleForPublicProfile";
        public const string ManifestVersionMismatchHealthProperty = "ManifestVersionMismatch";
        public const string SkipInitialGoalStateCheckProperty = "SkipInitialGoalStateCheck";

        public const string CodeUpgradePackageDetails = "CodeUpgradePackageDetails";
        public const string ConfigUpgradeErrorDetails = "ConfigUpgradeErrorDetails";
        public const string PathToFabricCode = @"bin\Fabric\Fabric.Code";
        public const string Configurations = "Configurations.csv";

        public static readonly TimeSpan FabricQueryRetryTimeoutInMinutes = TimeSpan.FromMinutes(5);
        public static readonly TimeSpan FabricQueryTimeoutInMinutes = TimeSpan.FromMinutes(1);
        public static readonly TimeSpan FabricPollDeactivatedNodesTimeoutInMinutes = TimeSpan.FromMinutes(10);
        public static readonly TimeSpan FabricPollActivationNodesTimeoutInMinutes = TimeSpan.FromMinutes(30);
        public static readonly TimeSpan DefaultDictionaryTimeout = TimeSpan.FromSeconds(4.0d);
        public static readonly TimeSpan UpgradeServiceMaxOperationTimeout = TimeSpan.FromMinutes(10);

        #endregion

        public static readonly Dictionary<string, Assembly> AssemblyResolveTable = new Dictionary<string, Assembly>(StringComparer.OrdinalIgnoreCase)
        {
            { "Microsoft.ServiceFabric.DeploymentManager", typeof(StandAloneCluster).Assembly },
            { "Microsoft.ServiceFabric.ClusterManagementCommon", typeof(ICluster).Assembly },
            { "System.Fabric.Management.ServiceModel", typeof(ClusterManifestType).Assembly },
        };
    }
}