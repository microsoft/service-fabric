// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Globalization;
    using System.IO;
    using System.Linq;

    /// <summary>
    /// This is the class that handles the creation of server deployment on a single node based on the
    /// input manifest.
    /// </summary>
    internal class FirewallManager
    {
        #region Public Functions
        /// <summary>
        /// This function enables the firewall exceptions for all the nodes provied. The exceptions are for lease driver port, application ports and fabric process.
        /// </summary>
        /// <param name="nodes">The list of nodes for which exceptions need to be provided.</param>
        /// <param name="isScaleMin">Whether te deployment is scale min or not.</param>
        /// <param name="removeRulesIfNotRequired">Should we delete the existing rules that are not required.</param>
        /// <returns>true if exceptions are all enabled, false otherwise.</returns>
        public static bool EnableFirewallSettings(List<NodeSettings> nodes, bool isScaleMin, bool removeRulesIfNotRequired)
        {
            return EnableFirewallSettings(nodes, isScaleMin, null, removeRulesIfNotRequired);
        }

        /// <summary>
        /// This function enables the firewall exceptions for all the nodes provied. The exceptions are for lease driver port, application ports and fabric process.
        /// </summary>
        /// <param name="nodes">The list of nodes for which exceptions need to be provided.</param>
        /// <param name="isScaleMin">Whether te deployment is scale min or not.</param>
        /// <param name="securitySection"> The security section which specifies what type of firewall profiles to set.</param>
        /// <param name="removeRulesIfNotRequired">Should we delete the existing rules that are not required.</param>
        /// <returns>true if exceptions are all enabled, false otherwise.</returns>
        public static bool EnableFirewallSettings(List<NodeSettings> nodes, bool isScaleMin, SettingsOverridesTypeSection securitySection, bool removeRulesIfNotrequired)
        {
            Firewall fw = new Firewall();
            if (!fw.IsEnabled())
            {
                return true;
            }

            if (isScaleMin && NetworkApiHelper.IsAddressLoopback(nodes[0].IPAddressOrFQDN))
            {
                return true;
            }

            List<FirewallRule> newRules = GetRulesForNodes(nodes, securitySection);
            fw.UpdateRules(newRules, removeRulesIfNotrequired);
            return true;
        }

        /// <summary>
        /// This function disables all firewall exceptions for the nodes.
        /// </summary>
        /// <param name="nodes">The nodes for which exceptions are to be disabled.</param>
        /// <returns>true if exceptions are disabled false otherwise.</returns>
        public static bool DisableFirewallSettings()
        {
            Firewall fw = new Firewall();
            if (!fw.IsEnabled())
            {
                return true;
            }

            fw.RemoveWindowsFabricRules();
            return true;
        }
        #endregion Public Functions

        #region Private Functions
        private static List<FirewallRule> GetRulesForNodes(List<NodeSettings> nodes, SettingsOverridesTypeSection securitySection)
        {
            List<FirewallRule> newRules = new List<FirewallRule>();
            foreach (NodeSettings setting in nodes)
            {
                string fabricPath = Path.Combine(
                    setting.DeploymentFoldersInfo.GetCodeDeploymentDirectory(Constants.FabricService),
                    Constants.ServiceExes[Constants.FabricService]);
                string dcaPath = Path.Combine(
                    setting.DeploymentFoldersInfo.GetCodeDeploymentDirectory(Constants.DCAService),
                    Constants.ServiceExes[Constants.DCAService]);
                string fileStoreServicePath = Path.Combine(
                    GetFabricSystemApplicationCodeFolder(setting.DeploymentFoldersInfo, Constants.FileStoreService, Constants.SystemServiceCodePackageName, Constants.SystemServiceCodePackageVersion),
                    Constants.ServiceExes[Constants.FileStoreService]);
                string fabricGatewayPath = Path.Combine(
                    setting.DeploymentFoldersInfo.GetCodeDeploymentDirectory(Constants.FabricService),
                    Constants.ServiceExes[Constants.FabricGatewayService]);
                string fabricAppGatewayPath = Path.Combine(
                    setting.DeploymentFoldersInfo.GetCodeDeploymentDirectory(Constants.FabricService),
                    Constants.ServiceExes[Constants.FabricApplicationGatewayService]);
                string faultAnalysisServicePath = Path.Combine(
                    GetFabricSystemApplicationCodeFolder(setting.DeploymentFoldersInfo, Constants.FaultAnalysisService, Constants.SystemServiceCodePackageName, Constants.SystemServiceCodePackageVersion),
                    Constants.ServiceExes[Constants.FaultAnalysisService]);
#if !DotNetCoreClrLinux
                string backupRestoreServicePath = Path.Combine(
                    GetFabricSystemApplicationCodeFolder(setting.DeploymentFoldersInfo, Constants.BackupRestoreService, Constants.SystemServiceCodePackageName, Constants.SystemServiceCodePackageVersion),
                    Constants.ServiceExes[Constants.BackupRestoreService]);
                string eventStoreServicePath = Path.Combine(
                    GetFabricSystemApplicationCodeFolder(setting.DeploymentFoldersInfo, Constants.EventStoreService, Constants.SystemServiceCodePackageName, Constants.SystemServiceCodePackageVersion),
                    Constants.ServiceExes[Constants.EventStoreService]);
#else
                string backupRestoreServicePath = null;
                string eventStoreServicePath = null;
#endif
                string fabricUpgradeServicePath = Path.Combine(
                    GetFabricSystemApplicationCodeFolder(setting.DeploymentFoldersInfo, Constants.FabricUpgradeService, Constants.SystemServiceCodePackageName, Constants.SystemServiceCodePackageVersion),
                    Constants.ServiceExes[Constants.FabricUpgradeService]);
                string fabricRepairServicePath = Path.Combine(
                    GetFabricSystemApplicationCodeFolder(setting.DeploymentFoldersInfo, Constants.FabricRepairManagerService, Constants.SystemServiceCodePackageName, Constants.SystemServiceCodePackageVersion),
                    Constants.ServiceExes[Constants.FabricRepairManagerService]);
                string fabricInfrastructureServicePath = Path.Combine(
                    GetFabricSystemApplicationCodeFolder(setting.DeploymentFoldersInfo, Constants.FabricInfrastructureService, Constants.SystemServiceCodePackageName, Constants.SystemServiceCodePackageVersion),
                    Constants.ServiceExes[Constants.FabricInfrastructureService]);
                string gatewayResourceManagerPath = Path.Combine(
                    GetFabricSystemApplicationCodeFolder(setting.DeploymentFoldersInfo, Constants.GatewayResourceManager, Constants.SystemServiceCodePackageName, Constants.SystemServiceCodePackageVersion),
                    Constants.ServiceExes[Constants.GatewayResourceManager]);

#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
                string centralsecretServicePath = Path.Combine(
                    GetFabricSystemApplicationCodeFolder(setting.DeploymentFoldersInfo, Constants.CentralSecretService, Constants.SystemServiceCodePackageName, Constants.SystemServiceCodePackageVersion),
                    Constants.ServiceExes[Constants.CentralSecretService]);
#else
                string centralsecretServicePath = null;
#endif
#if !DotNetCoreClrLinux
                string upgradeOrchestrationServicePath = Path.Combine(
                    GetFabricSystemApplicationCodeFolder(setting.DeploymentFoldersInfo, Constants.UpgradeOrchestrationService, Constants.SystemServiceCodePackageName, Constants.SystemServiceCodePackageVersion),
                    Constants.ServiceExes[Constants.UpgradeOrchestrationService]);
#else
                string upgradeOrchestrationServicePath = null;
#endif

                string leaseDriverPort = null;
                string applicationPortRange = null;
                string dynamicPortRange = null;
                string httpGatewayPort = null;
                string httpAppGatewayPort = null;

                GetPorts(
                    setting, 
                    out leaseDriverPort, 
                    out applicationPortRange, 
                    out httpGatewayPort, 
                    out httpAppGatewayPort,
                    out dynamicPortRange);

                var rulesForNode = FabricNodeFirewallRules.GetRulesForNode(
                    setting.NodeName,
                    leaseDriverPort,
                    applicationPortRange,
                    httpGatewayPort,
                    httpAppGatewayPort,
                    fabricPath,
                    dcaPath,
                    fileStoreServicePath,
                    fabricGatewayPath,
                    fabricAppGatewayPath,
                    faultAnalysisServicePath,
                    backupRestoreServicePath,
                    fabricUpgradeServicePath,
                    fabricRepairServicePath,
                    fabricInfrastructureServicePath,
                    upgradeOrchestrationServicePath,
                    centralsecretServicePath,
                    eventStoreServicePath,
                    gatewayResourceManagerPath,
                    dynamicPortRange,
                    securitySection);
                newRules.AddRange(rulesForNode);

#if DotNetCoreClrLinux
                string clientConnectionPort = null;
                string clusterConnectionPort = null;
                string serviceConnectionPort = null;
                string clusterManagerReplicatorPort = null;
                string repairManagerReplicatorPort = null;
                string namingReplicatorPort = null;
                string failoverManagerReplicatorPort = null;
                string imageStoreServiceReplicatorPort = null;
                string upgradeServiceReplicatorPort = null;

                GetPorts2(
                    setting,
                    out clientConnectionPort,
                    out serviceConnectionPort,
                    out clusterConnectionPort,
                    out clusterManagerReplicatorPort,
                    out repairManagerReplicatorPort,
                    out namingReplicatorPort,
                    out failoverManagerReplicatorPort,
                    out imageStoreServiceReplicatorPort,
                    out upgradeServiceReplicatorPort);

                var rulesForNode2 = FabricNodeFirewallRules.GetRulesForNode2(
                    setting.NodeName,
                    clientConnectionPort,
                    serviceConnectionPort,
                    clusterConnectionPort,
                    clusterManagerReplicatorPort,
                    repairManagerReplicatorPort,
                    namingReplicatorPort,
                    failoverManagerReplicatorPort,
                    imageStoreServiceReplicatorPort,
                    upgradeServiceReplicatorPort);
                newRules.AddRange(rulesForNode2);
#endif
            }
            return newRules;
        }

        public static string GetFabricSystemApplicationCodeFolder(DeploymentFolders deploymentFolders, string servicePackageName, string codePackageName, string codePackageVersion)
        {            
            string applicationDeploymentFolder = deploymentFolders.ApplicationDeploymentFolder;
            RunLayoutSpecification runLayout = RunLayoutSpecification.Create();
            runLayout.SetRoot(applicationDeploymentFolder);

            return runLayout.GetCodePackageFolder(Constants.SystemApplicationId, servicePackageName, codePackageName, codePackageVersion, false);
        }

        private static void GetPorts(
            NodeSettings setting, 
            out string leaseDriverPort, 
            out string applicationPortRange, 
            out string httpGatewayPort, 
            out string httpAppGatewayPort,
            out string dynamicPortRange)
        {
            SettingsTypeSection fabricNodeSection = setting.Settings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.FabricNode, StringComparison.OrdinalIgnoreCase));
            var leaseDriverPortParameter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.LeaseAgentAddress, StringComparison.OrdinalIgnoreCase));
            leaseDriverPort = leaseDriverPortParameter.Value;

            httpGatewayPort = null;
            var httpGatewayPortParameter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.HttpGatewayListenAddress, StringComparison.OrdinalIgnoreCase));
            if (httpGatewayPortParameter != null)
            {
                httpGatewayPort = httpGatewayPortParameter.Value;
            }

            httpAppGatewayPort = null;
            var httpAppGatewayPortParameter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.HttpApplicationGatewayListenAddress, StringComparison.OrdinalIgnoreCase));
            if (httpAppGatewayPortParameter != null)
            {
                httpAppGatewayPort = httpAppGatewayPortParameter.Value;
            }

            var startApplicationPortParamter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.StartApplicationPortRange, StringComparison.OrdinalIgnoreCase));
            var endApplicationPortParamter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.EndApplicationPortRange, StringComparison.OrdinalIgnoreCase));

            if (startApplicationPortParamter != null && endApplicationPortParamter != null)
            {
                applicationPortRange = string.Format(
                    CultureInfo.InvariantCulture,
                    "{0}-{1}",
                    startApplicationPortParamter.Value,
                    endApplicationPortParamter.Value);
            }
            else
            {
                applicationPortRange = null;
            }

            var startDynamicPortParamter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.StartDynamicPortRange, StringComparison.OrdinalIgnoreCase));
            var endDynamicnPortParamter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.EndDynamicPortRange, StringComparison.OrdinalIgnoreCase));

            if (startDynamicPortParamter != null && endDynamicnPortParamter != null)
            {
                dynamicPortRange = string.Format(
                    CultureInfo.InvariantCulture,
                    "{0}-{1}",
                    startDynamicPortParamter.Value,
                    endDynamicnPortParamter.Value);
            }
            else
            {
                dynamicPortRange = null;
            }
        }

#if DotNetCoreClrLinux
        private static void GetPorts2(
            NodeSettings setting,
            out string clientConnectionPort,
            out string serviceConnectionPort,
            out string clusterConnectionPort,
            out string clusterManagerReplicatorPort,
            out string repairManagerReplicatorPort,
            out string namingReplicatorPort,
            out string failoverManagerReplicatorPort,
            out string imageStoreServiceReplicatorPort,
            out string upgradeServiceReplicatorPort)
        {
            SettingsTypeSection fabricNodeSection = setting.Settings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.FabricNode, StringComparison.OrdinalIgnoreCase));

            var clientConnectionPortParameter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.ClientConnectionAddress, StringComparison.OrdinalIgnoreCase));
            clientConnectionPort = clientConnectionPortParameter.Value;

            var serviceConnectionPortParameter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.RuntimeServiceAddress, StringComparison.OrdinalIgnoreCase));
            serviceConnectionPort = serviceConnectionPortParameter.Value;

            var clusterConnectionPortParameter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.NodeAddress, StringComparison.OrdinalIgnoreCase));
            clusterConnectionPort = clusterConnectionPortParameter.Value;

            clusterManagerReplicatorPort = null;
            var clusterManagerReplicatorPortParameter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.ClusterManagerReplicatorAddress, StringComparison.OrdinalIgnoreCase));
            if (clusterManagerReplicatorPortParameter != null)
            {
                clusterManagerReplicatorPort = clusterManagerReplicatorPortParameter.Value;
            }

            repairManagerReplicatorPort = null;
            var repairManagerReplicatorPortParameter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.RepairManagerReplicatorAddress, StringComparison.OrdinalIgnoreCase));
            if (repairManagerReplicatorPortParameter != null)
            {
                repairManagerReplicatorPort = repairManagerReplicatorPortParameter.Value;
            }

            namingReplicatorPort = null;
            var namingReplicatorPortParameter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.NamingReplicatorAddress, StringComparison.OrdinalIgnoreCase));
            if (namingReplicatorPortParameter != null)
            {
                namingReplicatorPort = namingReplicatorPortParameter.Value;
            }

            failoverManagerReplicatorPort = null;
            var failoverManagerReplicatorPortParameter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.FailoverManagerReplicatorAddress, StringComparison.OrdinalIgnoreCase));
            if (failoverManagerReplicatorPortParameter != null)
            {
                failoverManagerReplicatorPort = failoverManagerReplicatorPortParameter.Value;
            }

            imageStoreServiceReplicatorPort = null;
            var imageStoreServiceReplicatorPortParameter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.ImageStoreServiceReplicatorAddress, StringComparison.OrdinalIgnoreCase));
            if (imageStoreServiceReplicatorPortParameter != null)
            {
                imageStoreServiceReplicatorPort = imageStoreServiceReplicatorPortParameter.Value;
            }

            upgradeServiceReplicatorPort = null;
            var upgradeServiceReplicatorPortParameter = fabricNodeSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.UpgradeServiceReplicatorAddress, StringComparison.OrdinalIgnoreCase));
            if (upgradeServiceReplicatorPortParameter != null)
            {
                upgradeServiceReplicatorPort = upgradeServiceReplicatorPortParameter.Value;
            }
        }
#endif

        private static SettingsTypeSectionParameter[] GetFirewallSecuritySection(NodeSettings setting)
        {
            List<string> fabricFirewallSecturityParameters = new List<string>(); 
            SettingsTypeSection fabricFirewallSecuritySection = setting.Settings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.Security, StringComparison.OrdinalIgnoreCase));

            return fabricFirewallSecuritySection.Parameter;
        }

#endregion Private Functions
    }
}