// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using NetFwTypeLib;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Linq;

    /// <summary>
    /// This class extends firewall rule to specify rules only for 
    /// </summary>
    class FabricNodeFirewallRules
    {
        public const string WindowsFabricGrouping = "WindowsFabric";
        private string NodeName { get; set; }
        private string LeaseDriverPort { get; set; }
        private string ApplicationPorts { get; set; }
        private string DynamicPorts { get; set; }
        private string FabricPath { get; set; }
        private string DCAPath { get; set; }
        private string FileStoreServicePath { get; set; }
        private string HttpGatewayPort { get; set; }
        private string FabricGatewayPath { get; set; }
        private string HttpAppGatewayPort { get; set; }
        private string FabricAppGatewayPath { get; set; }
        private string FaultAnalysisServicePath { get; set; }
        private string BackupRestoreServicePath { get; set; }
        private string FabricUpgradeServicePath { get; set; }
        private string FabricRepairServicePath { get; set; }
        private string FabricInfrastructureServicePath { get; set; }
        private string UpgradeOrchestrationServicePath { get; set; }
        private string CentralSecretServicePath { get; set; }
        private string EventStoreServicePath { get; set; }
        private string GatewayResourceManagerPath { get; set; }

#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
        private static string TypeProgID = "HNetCfg.FwPolicy2";
        private static Type policyType = Type.GetTypeFromProgID(TypeProgID);
        private static INetFwPolicy2 policy;
#endif
        private static HashSet<NET_FW_PROFILE_TYPE2_> fwProfileSet;

        public static bool IsFabricFirewallRule(INetFwRule fwRule)
        {
            return !string.IsNullOrEmpty(fwRule.Grouping) && fwRule.Grouping.Equals(FabricNodeFirewallRules.WindowsFabricGrouping);
        }

        public static List<FirewallRule> GetRulesForNode(string nodeName,
            string leaseDriverPort,
            string applicationPorts,
            string httpGatewayPort,
            string httpAppGatewayPort,
            string fabricPath,
            string dcaPath,
            string fileStoreServicePath,
            string fabricGatewayPath,
            string fabricAppGatewayPath,
            string faultAnalysisServicePath,
            string backupRestoreServicePath,
            string fabricUpgradeServicePath,
            string fabricRepairServicePath,
            string fabricInfrastructureServicePath,
            string upgradeOrchestrationServicePath,
            string centralSecretServicePath,
            string eventStoreServicePath,
            string gatewayResourceManagerPath,
            string dynamicPorts,
            SettingsOverridesTypeSection securitySection)

        {
            FabricNodeFirewallRules nodeRules = new FabricNodeFirewallRules()
            {
                ApplicationPorts = applicationPorts,
                FabricPath = fabricPath,
                LeaseDriverPort = leaseDriverPort,
                NodeName = nodeName,
                DCAPath = dcaPath,
                FileStoreServicePath = fileStoreServicePath,
                HttpGatewayPort = httpGatewayPort,
                FabricGatewayPath = fabricGatewayPath,
                HttpAppGatewayPort = httpAppGatewayPort,
                FabricAppGatewayPath = fabricAppGatewayPath,
                FaultAnalysisServicePath = faultAnalysisServicePath,
                BackupRestoreServicePath = backupRestoreServicePath,
                FabricUpgradeServicePath = fabricUpgradeServicePath,
                FabricRepairServicePath = fabricRepairServicePath,
                FabricInfrastructureServicePath = fabricInfrastructureServicePath,
                UpgradeOrchestrationServicePath = upgradeOrchestrationServicePath,
                CentralSecretServicePath = centralSecretServicePath,
                EventStoreServicePath = eventStoreServicePath,
                GatewayResourceManagerPath = gatewayResourceManagerPath,
                DynamicPorts = dynamicPorts
            };

#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
            policy = (INetFwPolicy2)Activator.CreateInstance(policyType);
#endif
            fwProfileSet = GetAllRequiredProfiles(securitySection);

            List<FirewallRule> rules = new List<FirewallRule>();
#if !DotNetCoreClrLinux && !DotNetCoreClrIOT // Application Path specific rules are not valid for Linux
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FabricExceptionTemplate, nodeRules.FabricPath, FabricNodeFirewallRules.inDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FabricExceptionTemplate, nodeRules.FabricPath, FabricNodeFirewallRules.outDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FabricDCaExceptionTemplate, nodeRules.DCAPath, FabricNodeFirewallRules.outDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FileStoreServiceExceptionTemplate, nodeRules.FileStoreServicePath, FabricNodeFirewallRules.inDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FileStoreServiceExceptionTemplate, nodeRules.FileStoreServicePath, FabricNodeFirewallRules.outDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FabricGatewayExceptionTemplate, nodeRules.FabricGatewayPath, FabricNodeFirewallRules.inDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FabricGatewayExceptionTemplate, nodeRules.FabricGatewayPath, FabricNodeFirewallRules.outDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FaultAnalysisServiceExceptionTemplate, nodeRules.FaultAnalysisServicePath, FabricNodeFirewallRules.inDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FaultAnalysisServiceExceptionTemplate, nodeRules.FaultAnalysisServicePath, FabricNodeFirewallRules.outDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.BackupRestoreServiceExceptionTemplate, nodeRules.BackupRestoreServicePath, FabricNodeFirewallRules.inDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.BackupRestoreServiceExceptionTemplate, nodeRules.BackupRestoreServicePath, FabricNodeFirewallRules.outDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.UpgradeOrchestrationServiceExceptionTemplate, nodeRules.UpgradeOrchestrationServicePath, FabricNodeFirewallRules.inDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.UpgradeOrchestrationServiceExceptionTemplate, nodeRules.UpgradeOrchestrationServicePath, FabricNodeFirewallRules.outDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.CentralSecretServiceExceptionTemplate, nodeRules.CentralSecretServicePath, FabricNodeFirewallRules.inDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.CentralSecretServiceExceptionTemplate, nodeRules.CentralSecretServicePath, FabricNodeFirewallRules.outDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FabricUpgradeServiceExceptionTemplate, nodeRules.FabricUpgradeServicePath, FabricNodeFirewallRules.inDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FabricUpgradeServiceExceptionTemplate, nodeRules.FabricUpgradeServicePath, FabricNodeFirewallRules.outDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FabricRepairServiceExceptionTemplate, nodeRules.FabricRepairServicePath, FabricNodeFirewallRules.inDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FabricRepairServiceExceptionTemplate, nodeRules.FabricRepairServicePath, FabricNodeFirewallRules.outDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FabricInfrastructureServiceExceptionTemplate, nodeRules.FabricInfrastructureServicePath, FabricNodeFirewallRules.inDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.FabricInfrastructureServiceExceptionTemplate, nodeRules.FabricInfrastructureServicePath, FabricNodeFirewallRules.outDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.EventStoreServiceExceptionTemplate, nodeRules.EventStoreServicePath, FabricNodeFirewallRules.inDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.EventStoreServiceExceptionTemplate, nodeRules.EventStoreServicePath, FabricNodeFirewallRules.outDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.GatewayResourceManagerExceptionTemplate, nodeRules.GatewayResourceManagerPath, FabricNodeFirewallRules.inDirection);
            AddApplicationPathExceptionProfileRules(rules, nodeRules, FabricNodeFirewallRules.GatewayResourceManagerExceptionTemplate, nodeRules.GatewayResourceManagerPath, FabricNodeFirewallRules.outDirection);

#endif
            AddLeaseDriverExceptionProfileRule(rules, nodeRules, FabricNodeFirewallRules.inDirection);
            AddLeaseDriverExceptionProfileRule(rules, nodeRules, FabricNodeFirewallRules.outDirection);

            if (!string.IsNullOrEmpty(httpGatewayPort))
            {
                AddHttpGatewayExceptionProfilesRule(rules, nodeRules);
            }

            if (!string.IsNullOrEmpty(httpAppGatewayPort))
            {
                AddHttpAppGatewayExceptionProfilesRule(rules, nodeRules);
            }

            if (!string.IsNullOrEmpty(applicationPorts))
            {
                AddApplicationPortRangeExceptionProfileRules(rules, nodeRules);
            }

            if (!string.IsNullOrEmpty(dynamicPorts))
            {
                AddDynamicPortRangeExceptionProfileRules(rules, nodeRules);
            }

            return rules;
        }

#if DotNetCoreClrLinux
        public static List<FirewallRule> GetRulesForNode2(string nodeName,
            string clientConnectionPort,
            string serviceConnectionPort,
            string clusterConnectionPort,
            string clusterManagerReplicatorPort,
            string repairManagerReplicatorPort,
            string namingReplicatorPort,
            string failoverManagerReplicatorPort,
            string imageStoreServiceReplicatorPort,
            string upgradeServiceReplicatorPort)
        {
            List<FirewallRule> rules = new List<FirewallRule>();
            FabricNodeFirewallRules nodeRules = new FabricNodeFirewallRules()
            {
                NodeName = nodeName,
            };

            rules.Add(nodeRules.GetCustomTcpPortException(FabricNodeFirewallRules.ClientConnectionExceptionTemplate, clientConnectionPort, FabricNodeFirewallRules.inDirection));
            rules.Add(nodeRules.GetCustomTcpPortException(FabricNodeFirewallRules.ClientConnectionExceptionTemplate, clientConnectionPort, FabricNodeFirewallRules.outDirection));

            rules.Add(nodeRules.GetCustomTcpPortException(FabricNodeFirewallRules.ServiceConnectionExceptionTemplate, serviceConnectionPort, FabricNodeFirewallRules.inDirection));
            rules.Add(nodeRules.GetCustomTcpPortException(FabricNodeFirewallRules.ServiceConnectionExceptionTemplate, serviceConnectionPort, FabricNodeFirewallRules.outDirection));

            rules.Add(nodeRules.GetCustomTcpPortException(FabricNodeFirewallRules.ClusterConnectionExceptionTemplate, clusterConnectionPort, FabricNodeFirewallRules.inDirection));
            rules.Add(nodeRules.GetCustomTcpPortException(FabricNodeFirewallRules.ClusterConnectionExceptionTemplate, clusterConnectionPort, FabricNodeFirewallRules.outDirection));

            if (!string.IsNullOrEmpty(clusterManagerReplicatorPort))
            {
                rules.Add(
                    nodeRules.GetCustomTcpPortException(
                        FabricNodeFirewallRules.ClusterManagerReplicatorEndpointExceptionTemplate,
                        clusterManagerReplicatorPort, FabricNodeFirewallRules.inDirection));
                rules.Add(
                    nodeRules.GetCustomTcpPortException(
                        FabricNodeFirewallRules.ClusterManagerReplicatorEndpointExceptionTemplate,
                        clusterManagerReplicatorPort, FabricNodeFirewallRules.outDirection));
            }

            if (!string.IsNullOrEmpty(repairManagerReplicatorPort))
            {
                rules.Add(
                    nodeRules.GetCustomTcpPortException(
                        FabricNodeFirewallRules.RepairManagerReplicatorEndpointExceptionTemplate,
                        repairManagerReplicatorPort, FabricNodeFirewallRules.inDirection));
                rules.Add(
                    nodeRules.GetCustomTcpPortException(
                        FabricNodeFirewallRules.RepairManagerReplicatorEndpointExceptionTemplate,
                        repairManagerReplicatorPort, FabricNodeFirewallRules.outDirection));
            }

            if (!string.IsNullOrEmpty(namingReplicatorPort))
            {
                rules.Add(
                    nodeRules.GetCustomTcpPortException(
                        FabricNodeFirewallRules.NamingReplicatorEndpointExceptionTemplate, namingReplicatorPort,
                        FabricNodeFirewallRules.inDirection));
                rules.Add(
                    nodeRules.GetCustomTcpPortException(
                        FabricNodeFirewallRules.NamingReplicatorEndpointExceptionTemplate, namingReplicatorPort,
                        FabricNodeFirewallRules.outDirection));
            }

            if (!string.IsNullOrEmpty(failoverManagerReplicatorPort))
            {
                rules.Add(
                    nodeRules.GetCustomTcpPortException(
                        FabricNodeFirewallRules.FailoverManagerReplicatorEndpointExceptionTemplate,
                        failoverManagerReplicatorPort, FabricNodeFirewallRules.inDirection));
                rules.Add(
                    nodeRules.GetCustomTcpPortException(
                        FabricNodeFirewallRules.FailoverManagerReplicatorEndpointExceptionTemplate,
                        failoverManagerReplicatorPort, FabricNodeFirewallRules.outDirection));
            }

            if (!string.IsNullOrEmpty(imageStoreServiceReplicatorPort))
            {
                rules.Add(
                    nodeRules.GetCustomTcpPortException(
                        FabricNodeFirewallRules.ImageStoreServiceReplicatorEndpointExceptionTemplate,
                        imageStoreServiceReplicatorPort, FabricNodeFirewallRules.inDirection));
                rules.Add(
                    nodeRules.GetCustomTcpPortException(
                        FabricNodeFirewallRules.ImageStoreServiceReplicatorEndpointExceptionTemplate,
                        imageStoreServiceReplicatorPort, FabricNodeFirewallRules.outDirection));
            }

            if (!string.IsNullOrEmpty(upgradeServiceReplicatorPort))
            {
                rules.Add(
                    nodeRules.GetCustomTcpPortException(
                        FabricNodeFirewallRules.UpgradeServiceReplicatorEndpointExceptionTemplate,
                        upgradeServiceReplicatorPort, FabricNodeFirewallRules.inDirection));
                rules.Add(
                    nodeRules.GetCustomTcpPortException(
                        FabricNodeFirewallRules.UpgradeServiceReplicatorEndpointExceptionTemplate,
                        upgradeServiceReplicatorPort, FabricNodeFirewallRules.outDirection));
            }

            return rules;
        }
#endif

        private static void AddApplicationPathExceptionProfileRules(List<FirewallRule> rules, FabricNodeFirewallRules nodeRules, string template, string path, string direction)
        {
            foreach(NET_FW_PROFILE_TYPE2_ fwProfile in fwProfileSet)
            {
                rules.Add(nodeRules.GetApplicationPathException(template, path, direction, fwProfile));
            }
        }

        private static void AddLeaseDriverExceptionProfileRule(List<FirewallRule> rules, FabricNodeFirewallRules nodeRules, string direction)
        {
            foreach (NET_FW_PROFILE_TYPE2_ fwProfile in fwProfileSet)
            {
                rules.Add(nodeRules.GetLeaseDriverExceptionRule(direction, fwProfile));
            }
        }

        private static void AddHttpGatewayExceptionProfilesRule(List<FirewallRule> rules, FabricNodeFirewallRules nodeRules)
        {
            foreach (NET_FW_PROFILE_TYPE2_ fwProfile in fwProfileSet)
            {
                rules.Add(nodeRules.GetHttpGatewayExceptionRule(
                    FabricNodeFirewallRules.FabricHttpGatewayExceptionTemplate,
                    nodeRules.HttpGatewayPort,
                    FabricNodeFirewallRules.outDirection,
                    fwProfile));

                rules.Add(nodeRules.GetHttpGatewayExceptionRule(
                    FabricNodeFirewallRules.FabricHttpGatewayExceptionTemplate,
                    nodeRules.HttpGatewayPort,
                    FabricNodeFirewallRules.inDirection, 
                    fwProfile));
            }
        }

        private static void AddHttpAppGatewayExceptionProfilesRule(List<FirewallRule> rules, FabricNodeFirewallRules nodeRules)
        {
            foreach (NET_FW_PROFILE_TYPE2_ fwProfile in fwProfileSet)
            {
                rules.Add(nodeRules.GetHttpGatewayExceptionRule(
                    FabricNodeFirewallRules.FabricHttpAppGatewayExceptionTemplate,
                    nodeRules.HttpAppGatewayPort,
                    FabricNodeFirewallRules.outDirection, 
                    fwProfile));

                rules.Add(nodeRules.GetHttpGatewayExceptionRule(
                    FabricNodeFirewallRules.FabricHttpAppGatewayExceptionTemplate,
                    nodeRules.HttpAppGatewayPort,
                    FabricNodeFirewallRules.inDirection, 
                    fwProfile));
            }
        }

        private static void AddApplicationPortRangeExceptionProfileRules(List<FirewallRule> rules, FabricNodeFirewallRules nodeRules) 
        {
            foreach (NET_FW_PROFILE_TYPE2_ fwProfile in fwProfileSet)
            {
                rules.Add(nodeRules.GetApplicationPortRangeExceptionRule(FabricNodeFirewallRules.inDirection, FabricNodeFirewallRules.ProtocolTcp, fwProfile));
                rules.Add(nodeRules.GetApplicationPortRangeExceptionRule(FabricNodeFirewallRules.outDirection, FabricNodeFirewallRules.ProtocolTcp, fwProfile));
                rules.Add(nodeRules.GetApplicationPortRangeExceptionRule(FabricNodeFirewallRules.inDirection, FabricNodeFirewallRules.ProtocolUdp, fwProfile));
                rules.Add(nodeRules.GetApplicationPortRangeExceptionRule(FabricNodeFirewallRules.outDirection, FabricNodeFirewallRules.ProtocolUdp, fwProfile));
            }
        }

        private static void AddDynamicPortRangeExceptionProfileRules(List<FirewallRule> rules, FabricNodeFirewallRules nodeRules)
        {
            foreach (NET_FW_PROFILE_TYPE2_ fwProfile in fwProfileSet)
            {
                rules.Add(nodeRules.GetDynamicPortRangeExceptionRule(FabricNodeFirewallRules.inDirection, FabricNodeFirewallRules.ProtocolTcp, fwProfile));
                rules.Add(nodeRules.GetDynamicPortRangeExceptionRule(FabricNodeFirewallRules.outDirection, FabricNodeFirewallRules.ProtocolTcp, fwProfile));
                rules.Add(nodeRules.GetDynamicPortRangeExceptionRule(FabricNodeFirewallRules.inDirection, FabricNodeFirewallRules.ProtocolUdp, fwProfile));
                rules.Add(nodeRules.GetDynamicPortRangeExceptionRule(FabricNodeFirewallRules.outDirection, FabricNodeFirewallRules.ProtocolUdp, fwProfile));
           }
        }

        private FirewallRule GetApplicationPathException(string template, string path, string direction, NET_FW_PROFILE_TYPE2_ fwProfile)
        {
            string ruleName = string.Format(CultureInfo.InvariantCulture, template, this.NodeName, GetProfileFriendlyName(fwProfile), direction);
            NET_FW_RULE_DIRECTION_ fwDirection = direction == FabricNodeFirewallRules.inDirection ?
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN :
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT;
            return GetRule(ruleName, path, null, NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP, fwDirection, fwProfile);
        }

#if DotNetCoreClrLinux
        private FirewallRule GetCustomTcpPortException(string template, string port, string direction)
        {
            string ruleName = string.Format(CultureInfo.InvariantCulture, template, this.NodeName, GetProfileFriendlyName(NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_PUBLIC), direction);
            NET_FW_RULE_DIRECTION_ fwDirection = direction == FabricNodeFirewallRules.inDirection ?
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN :
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT;
            return GetRule(ruleName, null, port, NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP, fwDirection, NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_PUBLIC);
        }
#endif

        private FirewallRule GetHttpGatewayExceptionRule(string template, string port, string direction, NET_FW_PROFILE_TYPE2_ fwProfile)
        {
            // Httpgateway exception is a driver level exception for http.sys
            string ruleName = string.Format(
                CultureInfo.InvariantCulture,
                template,
                this.NodeName,
                GetProfileFriendlyName(fwProfile),
                direction);
            NET_FW_RULE_DIRECTION_ fwDirection = direction == FabricNodeFirewallRules.inDirection ?
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN :
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT;
            return GetRule(ruleName, systemApplicationPath, port, NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP, fwDirection, fwProfile);
        }

        private FirewallRule GetLeaseDriverExceptionRule(string direction, NET_FW_PROFILE_TYPE2_ fwProfile)
        {
            string ruleName = string.Format(
                CultureInfo.InvariantCulture,
                FabricNodeFirewallRules.LeaseDriverExceptionTemplate,
                this.NodeName,
                GetProfileFriendlyName(fwProfile),
                direction);
            NET_FW_RULE_DIRECTION_ fwDirection = direction == FabricNodeFirewallRules.inDirection ?
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN :
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT;
            return GetRule(ruleName, null, this.LeaseDriverPort, NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP, fwDirection, fwProfile);
        }

        private FirewallRule GetApplicationPortRangeExceptionRule(string direction, string protocol, NET_FW_PROFILE_TYPE2_ fwProfile)
        {
            string ruleName = string.Format(
                CultureInfo.InvariantCulture,
                FabricNodeFirewallRules.ApplicationPortRangeExceptionTemplate,
                this.NodeName,
                protocol,
                GetProfileFriendlyName(fwProfile),
                direction);
            NET_FW_IP_PROTOCOL_ fwProtocol = protocol == FabricNodeFirewallRules.ProtocolTcp ?
                 NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP :
                 NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_UDP;
            NET_FW_RULE_DIRECTION_ fwDirection = direction == FabricNodeFirewallRules.inDirection ?
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN :
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT;
            return GetRule(ruleName, null, this.ApplicationPorts, fwProtocol, fwDirection, fwProfile);
        }

        private FirewallRule GetDynamicPortRangeExceptionRule(string direction, string protocol, NET_FW_PROFILE_TYPE2_ fwProfile)
        {
            string ruleName = string.Format(
                CultureInfo.InvariantCulture,
                FabricNodeFirewallRules.DynamicPortRangeExceptionTemplate,
                this.NodeName,
                protocol,
                GetProfileFriendlyName(fwProfile),
                direction);
            NET_FW_IP_PROTOCOL_ fwProtocol = protocol == FabricNodeFirewallRules.ProtocolTcp ?
                 NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP :
                 NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_UDP;
            NET_FW_RULE_DIRECTION_ fwDirection = direction == FabricNodeFirewallRules.inDirection ?
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN :
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT;
            return GetRule(ruleName, null, this.DynamicPorts, fwProtocol, fwDirection, fwProfile);
        }

        private FirewallRule GetRule(string ruleName, string applicationPath, string ports, NET_FW_IP_PROTOCOL_ protocol, NET_FW_RULE_DIRECTION_ direction,  NET_FW_PROFILE_TYPE2_ profile)
        {
            FirewallRule rule = new FirewallRule()
            {
                Name = ruleName,
                Ports = ports,
                Protocol = protocol,
                Direction = direction,
                ApplicationPath = applicationPath,
                Grouping = FabricNodeFirewallRules.WindowsFabricGrouping,
                Profile = profile
            };
            return rule;
        }

        private static string GetProfileFriendlyName(NET_FW_PROFILE_TYPE2_ fwProfile)
        {
            switch (fwProfile)
            {
                case NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_ALL:
                    return AllProfileName;
                case NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_DOMAIN:
                    return DomainProfileName;
                case NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_PRIVATE:
                    return PrivateProfileName;
                case NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_PUBLIC:
                    return PublicProfileName;
                default:
                    return string.Empty;
            }
        }

        private static HashSet<NET_FW_PROFILE_TYPE2_> GetAllRequiredProfiles(SettingsOverridesTypeSection securitySection)
        {
            HashSet<NET_FW_PROFILE_TYPE2_> profileSet = new HashSet<NET_FW_PROFILE_TYPE2_>();

#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
            AddDisabledFirewallProfileRules(profileSet, securitySection);
            NET_FW_PROFILE_TYPE2_ currentProfile = ((NET_FW_PROFILE_TYPE2_)Enum.Parse(typeof(NET_FW_PROFILE_TYPE2_), (policy.CurrentProfileTypes).ToString()));
#else
            // For Linux assume the profile is always public
            NET_FW_PROFILE_TYPE2_ currentProfile = NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_PUBLIC;
#endif
            if (currentProfile == NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_ALL)
            {
                profileSet = new HashSet<NET_FW_PROFILE_TYPE2_>();
                profileSet.Add(currentProfile);
            }
            else
            {
                var currentProfileSet = new HashSet<NET_FW_PROFILE_TYPE2_>();
                currentProfileSet.Add(currentProfile);
                profileSet.UnionWith(currentProfileSet);
            }

            return profileSet;
        }

        private static void AddDisabledFirewallProfileRules(HashSet<NET_FW_PROFILE_TYPE2_> profileSet, SettingsOverridesTypeSection securitySection)
        {
            if (securitySection != null && securitySection.Parameter != null && securitySection.Parameter.Length > 1)
            {
                foreach (var parameter in securitySection.Parameter)
                {
                    if (parameter.Name.Equals(Constants.ParameterNames.DisableFirewallRuleForDomainProfile, StringComparison.OrdinalIgnoreCase) &&
                        parameter.Value.Equals(bool.FalseString, StringComparison.OrdinalIgnoreCase))
                    {
                        profileSet.Add(NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_DOMAIN);
                    }

                    if (parameter.Name.Equals(Constants.ParameterNames.DisableFirewallRuleForPrivateProfile, StringComparison.OrdinalIgnoreCase) &&
                        parameter.Value.Equals(bool.FalseString, StringComparison.OrdinalIgnoreCase))
                    {
                        profileSet.Add(NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_PRIVATE);
                    }

                    if (parameter.Name.Equals(Constants.ParameterNames.DisableFirewallRuleForPublicProfile, StringComparison.OrdinalIgnoreCase) &&
                        parameter.Value.Equals(bool.FalseString, StringComparison.OrdinalIgnoreCase))
                    {
                        profileSet.Add(NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_PUBLIC);
                    }
                }
            }
        }

#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
        private static readonly string FabricExceptionTemplate = "{0} WindowsFabric Fabric Process (TCP{1}{2})";
        private static readonly string FileStoreServiceExceptionTemplate = "{0} WindowsFabric FileStoreService Process (TCP{1}{2})";
        private static readonly string FabricDCaExceptionTemplate = "{0} WindowsFabric DCA Process (TCP{1}{2})";
        private static readonly string FabricGatewayExceptionTemplate = "{0} WindowsFabric Fabric Gateway Process (TCP{1}{2})";
        private static readonly string FaultAnalysisServiceExceptionTemplate = "{0} WindowsFabric FaultAnalysisService Process (TCP{1}{2})";
        private static readonly string BackupRestoreServiceExceptionTemplate = "{0} WindowsFabric BackupRestoreService Process (TCP{1}{2})";

        private static readonly string FabricUpgradeServiceExceptionTemplate = "{0} WindowsFabric FabricUpgradeService Process (TCP{1}{2})";
        private static readonly string FabricRepairServiceExceptionTemplate = "{0} WindowsFabric RepairManagerService Process (TCP{1}{2})";
        private static readonly string FabricInfrastructureServiceExceptionTemplate = "{0} WindowsFabric InfrastructureService Process (TCP{1}{2})";
        private static readonly string UpgradeOrchestrationServiceExceptionTemplate = "{0} WindowsFabric UpgradeOrchestrationService Process (TCP{1}{2})";
        private static readonly string CentralSecretServiceExceptionTemplate = "{0} WindowsFabric CentralSecretService Process (TCP{1}{2})";
        private static readonly string EventStoreServiceExceptionTemplate = "{0} WindowsFabric EventStoreService Process (TCP{1}{2})";
        private static readonly string GatewayResourceManagerExceptionTemplate = "{0} WindowsFabric GatewayResourceManager Process (TCP{1}{2})";
#endif 

        private static readonly string FabricHttpGatewayExceptionTemplate = "{0} WindowsFabric Http Gateway (TCP{1}{2})";
        private static readonly string FabricHttpAppGatewayExceptionTemplate = "{0} WindowsFabric Http App Gateway (TCP{1}{2})";

        private static readonly string LeaseDriverExceptionTemplate = "{0} WindowsFabric Lease Driver (TCP{1}{2})";
        private static readonly string ApplicationPortRangeExceptionTemplate = "{0} WindowsFabric Application Processes ({1}{2}{3})";
        private static readonly string DynamicPortRangeExceptionTemplate = "{0} WindowsFabric Dynamic Ports - ({1}{2}{3})";

#if DotNetCoreClrLinux
        private static readonly string ClientConnectionExceptionTemplate = "{0} WindowsFabric Client Connection - (TCP{1}{2})";
        private static readonly string ServiceConnectionExceptionTemplate = "{0} WindowsFabric Service Connection - (TCP{1}{2})";
        private static readonly string ClusterConnectionExceptionTemplate = "{0} WindowsFabric Cluster Connection - (TCP{1}{2})";
        private static readonly string ClusterManagerReplicatorEndpointExceptionTemplate = "{0} WindowsFabric Cluster Manager Replicator Port - (TCP{1}{2})";
        private static readonly string RepairManagerReplicatorEndpointExceptionTemplate = "{0} WindowsFabric Repair Manager Replicator Port - (TCP{1}{2})";
        private static readonly string NamingReplicatorEndpointExceptionTemplate = "{0} WindowsFabric Naming Replicator Port - (TCP{1}{2})";
        private static readonly string FailoverManagerReplicatorEndpointExceptionTemplate = "{0} WindowsFabric Failover Manager Replicator Port - (TCP{1}{2})";
        private static readonly string ImageStoreServiceReplicatorEndpointExceptionTemplate = "{0} WindowsFabric ImageStore Service Replicator Port - (TCP{1}{2})";
        private static readonly string UpgradeServiceReplicatorEndpointExceptionTemplate = "{0} WindowsFabric Upgrade Service Replicator Port - (TCP{1}{2})";
#endif

        private const string ProtocolTcp = "TCP";
        private const string ProtocolUdp = "UDP";

        private const string inDirection = "-In";
        private const string outDirection = "-Out";

        private const string Domain = "NET_FW_PROFILE2_DOMAIN";
        private const string Private = "NET_FW_PROFILE2_PRIVATE";
        private const string Public = "NET_FW_PROFILE2_PUBLIC";

        private static readonly string DomainProfileName = "-Domain";
        private static readonly string PrivateProfileName = "-Private";
        private static readonly string PublicProfileName = "-Public";
        private static readonly string AllProfileName = "-All";

        private const string systemApplicationPath = "System";
    }
}