// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using NetFwTypeLib;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.IO;
    using WEX.TestExecution;
    using System.Fabric.Common.ImageModel;
    using System.Security.Principal;
    using System.ServiceProcess;

    /// <summary>
    /// This is the class that tests the validation of the files that we deploy.
    /// </summary>
    [TestClass]
    internal class FirewallTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void BasicInstallationAndUninstallation()
        {
            List<TestFirewallRule> startRules = TestFirewallManiuplator.GetRules();
            var nodes = GetNodeSettings(3, 0, true, "test.microsoft.com");
            FirewallManager.EnableFirewallSettings(nodes, true, true);
            List<TestFirewallRule> newRules = GetRules(3, 0, true);
            List<TestFirewallRule> addedRules = TestFirewallManiuplator.GetRules();
            VerifyRulesDiff(startRules, newRules, addedRules);
            FirewallManager.DisableFirewallSettings();
            List<TestFirewallRule> finalRules = TestFirewallManiuplator.GetRules();
            VerifyRulesEqual(finalRules, startRules);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void V1RuleRemoval()
        {
            List<TestFirewallRule> startRules = TestFirewallManiuplator.GetRules();
            List<TestFirewallRule> v1Rules = new List<TestFirewallRule>();
            TestFirewallManiuplator.AddRules(v1Rules);
            var nodes = GetNodeSettings(3, 0, true, "test.microsoft.com");
            var newRules = GetRules(3, 0, true);
            FirewallManager.EnableFirewallSettings(nodes, true, true);
            List<TestFirewallRule> addedRules = TestFirewallManiuplator.GetRules();
            VerifyRulesDiff(startRules, newRules, addedRules);
            FirewallManager.DisableFirewallSettings();
            List<TestFirewallRule> finalRules = TestFirewallManiuplator.GetRules();
            VerifyRulesEqual(finalRules, startRules);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void UpdateTest()
        {
            List<TestFirewallRule> startRules = TestFirewallManiuplator.GetRules();
            var nodes = GetNodeSettings(3, 0, true, "test.microsoft.com");
            FirewallManager.EnableFirewallSettings(nodes, true, true);
            List<TestFirewallRule> newRules = GetRules(3, 0, true);
            List<TestFirewallRule> addedRules = TestFirewallManiuplator.GetRules();
            VerifyRulesDiff(startRules, newRules, addedRules);

            //
            // Remove a node and the corresponding firewall rules for that node.
            //
            nodes.RemoveAt(1);
            newRules.RemoveRange(35, 35);

            nodes.AddRange(GetNodeSettings(2, 3, true, "test.microsoft.com"));
            newRules.AddRange(GetRules(2, 3, true));
            FirewallManager.EnableFirewallSettings(nodes, true, true);
            addedRules = TestFirewallManiuplator.GetRules();
            VerifyRulesDiff(startRules, newRules, addedRules);
            FirewallManager.DisableFirewallSettings();
            var finalRules = TestFirewallManiuplator.GetRules();
            VerifyRulesEqual(startRules, finalRules);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void VerifyChangeOnLocalhostNonScaleMin()
        {
            var startRules = TestFirewallManiuplator.GetRules();
            var nodes = GetNodeSettings(1, 0, true, "localhost");
            var newRules = GetRules(1, 0, true);
            FirewallManager.EnableFirewallSettings(nodes, false, true);
            var finalRules = TestFirewallManiuplator.GetRules();
            VerifyRulesDiff(startRules, newRules, finalRules);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void VerifyNoChangeOnLocalhostScaleMin()
        {
            var startRules = TestFirewallManiuplator.GetRules();
            var nodes = GetNodeSettings(3, 0, true, "localhost");
            var newRules = GetRules(3, 0, true);
            FirewallManager.EnableFirewallSettings(nodes, true, true);
            var finalRules = TestFirewallManiuplator.GetRules();
            VerifyRulesEqual(startRules, finalRules);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void VerifyWindowsFirewallDisabledScenario()
        {
            string machineName = ".";
            string serviceName = "MpsSvc";

            Verify.IsTrue(FirewallUtility.IsAdminUser(), "user session is elevated.");

            ServiceController windowsFirewallService = ServiceController.GetServices(machineName)
                .SingleOrDefault(s => string.Equals(s.ServiceName, serviceName, StringComparison.OrdinalIgnoreCase));
            if (windowsFirewallService == null)
            {
                Verify.Fail("Cannot proceed if windowsFirewallService is null");
            }

            if (windowsFirewallService.Status != ServiceControllerStatus.Stopped)
            {
                if (!windowsFirewallService.CanStop)
                {
                    Console.WriteLine("Windows Firewall Service can not be stopped");
                }
                else
                {
                    try
                    {
                        windowsFirewallService.Stop();
                    }
                    catch (InvalidOperationException e)
                    {
                        Verify.Fail(string.Format("Hit InvalidOperationException trying to call windowsFirewallService.Stop() : {0}", e));
                        throw;
                    }

                    windowsFirewallService.WaitForStatus(ServiceControllerStatus.Stopped);
                }
            }

            try
            {
                Firewall fsFirewall = new Firewall();
            }
            catch (Exception ex)
            {
                Verify.Fail(ex.ToString());
            }
            
            if (windowsFirewallService.Status == ServiceControllerStatus.Stopped)
            {
                windowsFirewallService.Start();
                windowsFirewallService.WaitForStatus(ServiceControllerStatus.Running);
            }
        }

        [TestCleanup]
        [TestProperty("ThreadingModel", "MTA")]
        public void Cleanup()
        {
            FirewallManager.DisableFirewallSettings();
            TestFirewallManiuplator.Enable();
        }

        private void VerifyRulesDiff(List<TestFirewallRule> subSet, List<TestFirewallRule> diff, List<TestFirewallRule> total)
        {
            List<TestFirewallRule> newRulesFound = new List<TestFirewallRule>(total.Except(subSet));
            newRulesFound.Sort();
            diff.Sort();
            VerifyRulesEqual(diff, newRulesFound);
        }

        private static void VerifyRulesEqual(List<TestFirewallRule> list1, List<TestFirewallRule> list2)
        {
            Verify.AreEqual(list2.Count, list1.Count, string.Format("new rules added {0}, new rules found {1}", list1.Count, list2.Count));
            for (int i = 0; i < list2.Count; i++)
            {
                Verify.IsTrue(list2[i].Equals(list1[i]), string.Format("newRules [{2}] = {0}, newRulesFound [{2}] = {1}", list1[i], list2[i], i));
            }
        }

        List<NodeSettings> GetNodeSettings(int nodeCount, int start, bool addAppPorts, string ipAddressOrFQDN)
        {
            List<NodeSettings> nodes = new List<NodeSettings>();
            int leasedriverPort = BaseLeaseDriverPort + start;
            int appPortStart = BaseAppStartport + start * AppPortRange;
            int appPortEnd = appPortStart + AppPortRange;
            int httpGatewayPort = BaseHttpGatewayport + start;
            int httpAppGatewayPort = BaseHttpAppGatewayport + start;
            for (int i = 0; i < nodeCount; i++)
            {
                string nodeName = string.Format(NodeNameTemplate, i + start);
                FabricNodeType fabricNode = new FabricNodeType() { NodeName = nodeName, IPAddressOrFQDN = ipAddressOrFQDN, NodeTypeRef = "Test" };
                var endpoints = new FabricEndpointsType()
                    {
                        LeaseDriverEndpoint = new InternalEndpointType() { Port = string.Format("{0}", leasedriverPort) },
                        ClientConnectionEndpoint = new InputEndpointType { Port = "3999" },
                        ClusterConnectionEndpoint = new InternalEndpointType() { Port = "3998" },
                        HttpGatewayEndpoint = new InputEndpointType { Port = string.Format("{0}", httpGatewayPort) },
                        HttpApplicationGatewayEndpoint = new InputEndpointType { Port = string.Format("{0}", httpAppGatewayPort) },
                        ApplicationEndpoints = addAppPorts ?
                            new FabricEndpointsTypeApplicationEndpoints()
                            {
                                StartPort = appPortStart,
                                EndPort = appPortEnd
                            }
                            : null
                    };
                ClusterManifestTypeNodeType type = new ClusterManifestTypeNodeType();
                DeploymentFolders folders = new DeploymentFolders("", null, FabricDeploymentSpecification.Create(), nodeName, new Versions("", "", "", ""), false);
                NodeSettings setting = new NodeSettings(fabricNode, type, folders, true, endpoints);
                nodes.Add(setting);
                leasedriverPort++;
                appPortStart += AppPortRange;
                appPortEnd += AppPortRange;
            }
            return nodes;
        }

        List<TestFirewallRule> GetRules(int nodeCount, int start, bool addAppPorts)
        {
            List<TestFirewallRule> rules = new List<TestFirewallRule>();
            int leaseDriverPort = BaseLeaseDriverPort + start;
            int httpGatewayPort = BaseHttpGatewayport + start;
            int httpAppGatewayPort = BaseHttpAppGatewayport + start;
            int appPortStart = BaseAppStartport + start * AppPortRange;
            int appPortEnd = appPortStart + AppPortRange;
            var fwProfile = TestFirewallManiuplator.GetCurrentProfile();

            for (int i = 0; i < nodeCount; i++)
            {
                string nodeName = string.Format(NodeNameTemplate, i + start);
                AddRuleForEachProfile(rules, nodeName, httpGatewayPort, httpAppGatewayPort, leaseDriverPort, fwProfile);


                if (addAppPorts)
                {
                    AddApplicationPortsForProfile(rules, nodeName, appPortStart, appPortEnd, fwProfile);
                    appPortStart += AppPortRange;
                    appPortEnd += AppPortRange;
                }

                leaseDriverPort++;
            }
            return rules;
        }

        List<TestFirewallRule> GetV1Rules(int nodeCount)
        {
            List<TestFirewallRule> rules = new List<TestFirewallRule>();
            int leaseDriverPort = BaseLeaseDriverPort;
            int appPortStart = BaseAppStartport;
            int appPortEnd = appPortStart + AppPortRange;
            for (int i = 0; i < nodeCount; i++)
            {
                string nodeName = string.Format(NodeNameTemplate, i);
                string fabricPath = Path.Combine(nodeName, "Fabric", "Fabric.Code", "Fabric.exe");
                rules.Add(new TestFirewallRule(
                    String.Format("{0} Fabric Process Exception", nodeName),
                    null,
                    NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                    NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                   fabricPath,
                   null
                    ));
                rules.Add(new TestFirewallRule(
                    String.Format("{0} Lease Driver Port Exception", nodeName),
                    string.Format("{0}", leaseDriverPort),
                    NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                    NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                    null,
                    null
                    ));
                rules.Add(new TestFirewallRule(
                String.Format("{0} Application Port Range Exception for TCP", nodeName),
                string.Format("{0}-{1}", appPortStart, appPortEnd),
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                null,
               "WindowsFabric"
                ));
                rules.Add(new TestFirewallRule(
                String.Format("{0} Application Port Range Exception for UDP", nodeName),
                string.Format("{0}-{1}", appPortStart, appPortEnd),
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_UDP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                null,
               "WindowsFabric"
                ));
            }
            return rules;
        }

        private void AddRuleForEachProfile(List<TestFirewallRule> rules,
            string nodeName,
            int httpGatewayPort,
            int httpAppGatewayPort,
            int leaseDriverPort,
            NET_FW_PROFILE_TYPE2_ fwProfile)
        {
            string fabricPath = Path.Combine("Fabric", "Fabric.Code", "Fabric.exe");
            string fabricGatewayPath = Path.Combine("Fabric", "Fabric.Code", "FabricGateway.exe");
            string systemPath = "System";
            string dcaPath = Path.Combine("Fabric", "DCA.Code", "FabricDCA.exe");
            string fileStoreServicePath = Path.Combine(nodeName, @"Fabric\work\Applications\__FabricSystem_App4294967295\FileStoreService.Code.Current\FileStoreService.exe");
            string faultAnalysisServicePath = Path.Combine(nodeName, @"Fabric\work\Applications\__FabricSystem_App4294967295\FAS.Code.Current\FabricFAS.exe");
            string upgradeOrchestrationServicePath = Path.Combine(nodeName, @"Fabric\work\Applications\__FabricSystem_App4294967295\UOS.Code.Current\FabricUOS.exe");
            string upgradeServicePath = Path.Combine(nodeName, @"Fabric\work\Applications\__FabricSystem_App4294967295\US.Code.Current\FabricUS.exe");
            string repairServicePath = Path.Combine(nodeName, @"Fabric\work\Applications\__FabricSystem_App4294967295\RM.Code.Current\FabricRM.exe");
            string infraServicePath = Path.Combine(nodeName, @"Fabric\work\Applications\__FabricSystem_App4294967295\IS.Code.Current\FabricIS.exe");
            string backupRestoreServicePath = Path.Combine(nodeName, @"Fabric\work\Applications\__FabricSystem_App4294967295\BRS.Code.Current\FabricBRS.exe");
            string centralSecretServicePath = Path.Combine(nodeName, @"Fabric\work\Applications\__FabricSystem_App4294967295\CSS.Code.Current\FabricCSS.exe");
            string eventStoreServicePath = Path.Combine(nodeName, @"Fabric\work\Applications\__FabricSystem_App4294967295\ES.Code.Current\EventStore.Service.exe");
            string gatewayResourceManagerPath = Path.Combine(nodeName, @"Fabric\work\Applications\__FabricSystem_App4294967295\GRM.Code.Current\FabricGRM.exe");
            string profileName = GetProfileFriendlyName(fwProfile);

            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric Fabric Process (TCP{1}-Out)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                fabricPath,
                "WindowsFabric",
                fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric Fabric Process (TCP{1}-In)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
               fabricPath,
               "WindowsFabric",
               fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric FileStoreService Process (TCP{1}-Out)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                fileStoreServicePath,
                "WindowsFabric",
                fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric FileStoreService Process (TCP{1}-In)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                fileStoreServicePath,
                "WindowsFabric",
                fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric DCA Process (TCP{1}-Out)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
               dcaPath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric Http Gateway (TCP{1}-Out)", nodeName, profileName),
                string.Format("{0}", httpGatewayPort),
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                systemPath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric Http Gateway (TCP{1}-In)", nodeName, profileName),
                string.Format("{0}", httpGatewayPort),
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                systemPath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric Http App Gateway (TCP{1}-Out)", nodeName, profileName),
                string.Format("{0}", httpAppGatewayPort),
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                systemPath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric Http App Gateway (TCP{1}-In)", nodeName, profileName),
                string.Format("{0}", httpAppGatewayPort),
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                systemPath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric Lease Driver (TCP{1}-Out)", nodeName, profileName),
                string.Format("{0}", leaseDriverPort),
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                null,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric Lease Driver (TCP{1}-In)", nodeName, profileName),
                string.Format("{0}", leaseDriverPort),
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                null,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric Fabric Gateway Process (TCP{1}-Out)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                fabricGatewayPath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric Fabric Gateway Process (TCP{1}-In)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                fabricGatewayPath,
               "WindowsFabric",
                   fwProfile
                ));

            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric FaultAnalysisService Process (TCP{1}-Out)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                faultAnalysisServicePath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric FaultAnalysisService Process (TCP{1}-In)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                faultAnalysisServicePath,
               "WindowsFabric",
                   fwProfile
                ));

            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric UpgradeOrchestrationService Process (TCP{1}-Out)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                upgradeOrchestrationServicePath,
                "WindowsFabric",
                fwProfile
            ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric UpgradeOrchestrationService Process (TCP{1}-In)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                upgradeOrchestrationServicePath,
               "WindowsFabric",
                   fwProfile
                ));

            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric CentralSecretService Process (TCP{1}-Out)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                centralSecretServicePath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric CentralSecretService Process (TCP{1}-In)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                centralSecretServicePath,
               "WindowsFabric",
                   fwProfile
                ));

            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric FabricUpgradeService Process (TCP{1}-Out)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                upgradeServicePath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric FabricUpgradeService Process (TCP{1}-In)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                upgradeServicePath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric RepairManagerService Process (TCP{1}-Out)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                repairServicePath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric RepairManagerService Process (TCP{1}-In)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                repairServicePath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
               String.Format("{0} WindowsFabric BackupRestoreService Process (TCP{1}-Out)", nodeName, profileName),
               "*",
               NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
               NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
               backupRestoreServicePath,
              "WindowsFabric",
                  fwProfile
               ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric BackupRestoreService Process (TCP{1}-In)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                backupRestoreServicePath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
               String.Format("{0} WindowsFabric InfrastructureService Process (TCP{1}-Out)", nodeName, profileName),
               "*",
               NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
               NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
               infraServicePath,
              "WindowsFabric",
                  fwProfile
               ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric InfrastructureService Process (TCP{1}-In)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                infraServicePath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
               String.Format("{0} WindowsFabric EventStoreService Process (TCP{1}-Out)", nodeName, profileName),
               "*",
               NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
               NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
               eventStoreServicePath,
              "WindowsFabric",
                  fwProfile
               ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric EventStoreService Process (TCP{1}-In)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                eventStoreServicePath,
               "WindowsFabric",
                   fwProfile
                ));

            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric GatewayResourceManager Process (TCP{1}-Out)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                gatewayResourceManagerPath,
               "WindowsFabric",
                   fwProfile
                ));
            rules.Add(new TestFirewallRule(
                String.Format("{0} WindowsFabric GatewayResourceManager Process (TCP{1}-In)", nodeName, profileName),
                "*",
                NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                gatewayResourceManagerPath,
               "WindowsFabric",
                   fwProfile
                ));
        }

        private void AddApplicationPortsForProfile(List<TestFirewallRule> rules,
            string nodeName,
            int appPortStart,
            int appPortEnd,
            NET_FW_PROFILE_TYPE2_ fwProfile)
        {
            string profileName = GetProfileFriendlyName(fwProfile);

            rules.Add(new TestFirewallRule(
                     String.Format("{0} WindowsFabric Application Processes (TCP{1}-Out)", nodeName, profileName),
                     string.Format("{0}-{1}", appPortStart, appPortEnd),
                     NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
                     NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
                     null,
                    "WindowsFabric",
                    fwProfile
                     ));
            rules.Add(new TestFirewallRule(
            String.Format("{0} WindowsFabric Application Processes (UDP{1}-Out)", nodeName, profileName),
            string.Format("{0}-{1}", appPortStart, appPortEnd),
            NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_UDP,
            NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT,
            null,
           "WindowsFabric",
           fwProfile
            ));
            rules.Add(new TestFirewallRule(
            String.Format("{0} WindowsFabric Application Processes (TCP{1}-In)", nodeName, profileName),
            string.Format("{0}-{1}", appPortStart, appPortEnd),
            NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP,
            NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
            null,
           "WindowsFabric",
           fwProfile
            ));
            rules.Add(new TestFirewallRule(
            String.Format("{0} WindowsFabric Application Processes (UDP{1}-In)", nodeName, profileName),
            string.Format("{0}-{1}", appPortStart, appPortEnd),
            NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_UDP,
            NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
            null,
           "WindowsFabric",
           fwProfile
            ));
        }

        private string GetProfileFriendlyName(NET_FW_PROFILE_TYPE2_ fwProfile)
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

        private const string DomainProfileName = "-Domain";
        private const string PrivateProfileName = "-Private";
        private const string PublicProfileName = "-Public";
        private const string AllProfileName = "-All";
        private const int BaseLeaseDriverPort = 4000;
        private const int BaseAppStartport = 30000;
        private const int BaseHttpGatewayport = 5000;
        private const int BaseHttpAppGatewayport = 6000;
        private const int AppPortRange = 100;
        private const string NodeNameTemplate = "TestNode{0}";
    }

    class TestFirewallRule : IComparable
    {
        public string Name { get; set; }
        public string Ports { get; set; }
        public NET_FW_IP_PROTOCOL_ Protocol { get; set; }
        public NET_FW_RULE_DIRECTION_ Direction { get; set; }
        public NET_FW_PROFILE_TYPE2_ Profile { get; set; }
        public string ApplicationPath { get; set; }
        public string Grouping { get; set; }

        public TestFirewallRule(string name, string ports, NET_FW_IP_PROTOCOL_ protocol, NET_FW_RULE_DIRECTION_ direction, string appPath, string grouping, NET_FW_PROFILE_TYPE2_ profile)
            : this(name, ports, protocol, direction, appPath, grouping)
        {
            this.Profile = profile;
        }

        public TestFirewallRule(string name, string ports, NET_FW_IP_PROTOCOL_ protocol, NET_FW_RULE_DIRECTION_ direction,
            string appPath, string grouping)
        {
            this.Name = name;
            this.Ports = ports;
            this.Protocol = protocol;
            this.Direction = direction;
            this.ApplicationPath = appPath;
            this.Grouping = grouping;
        }

        public TestFirewallRule(NetFwRule rule)
        {
            this.Name = rule.Name;
            this.Protocol = (NET_FW_IP_PROTOCOL_)rule.Protocol;
            this.Ports = rule.LocalPorts;
            this.Direction = rule.Direction;
            this.ApplicationPath = rule.ApplicationName;
            this.Grouping = rule.Grouping;
        }

        public override int GetHashCode()
        {
            return this.Name.GetHashCode();
        }

        public override bool Equals(object obj)
        {
            TestFirewallRule rule = (TestFirewallRule)obj;
            return rule.Name == this.Name
                && rule.Ports == this.Ports
                && rule.Protocol == this.Protocol
                && rule.Direction == this.Direction
                && rule.ApplicationPath == this.ApplicationPath
                && rule.Grouping == this.Grouping;
        }

        public override string ToString()
        {
            return string.Format("Name = {0}, Protocol = {1}, Ports = {2}, Direction = {3}, Application Path = {4}, Grouping = {5}",
                this.Name,
                this.Protocol,
                string.IsNullOrEmpty(this.Ports) ? "" : this.Ports,
                this.Direction,
                string.IsNullOrEmpty(this.ApplicationPath) ? "" : this.ApplicationPath,
                this.Grouping);
        }

        public int CompareTo(object obj)
        {
            TestFirewallRule rule = (TestFirewallRule)obj;
            return rule.Name.CompareTo(this.Name);
        }
    }

    class TestFirewallManiuplator
    {
        public static void Disable()
        {
            var NetFWPolicyType = Type.GetTypeFromProgID("HNetCfg.FwPolicy2", false);
            var mgr = (INetFwPolicy2)Activator.CreateInstance(NetFWPolicyType);
            mgr.set_FirewallEnabled(NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_DOMAIN, false);
            mgr.set_FirewallEnabled(NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_PRIVATE, false);
            mgr.set_FirewallEnabled(NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_PUBLIC, false);
        }

        public static void Enable()
        {
            var NetFWPolicyType = Type.GetTypeFromProgID("HNetCfg.FwPolicy2", false);
            var mgr = (INetFwPolicy2)Activator.CreateInstance(NetFWPolicyType);
            mgr.set_FirewallEnabled(NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_DOMAIN, true);
            mgr.set_FirewallEnabled(NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_PRIVATE, true);
            mgr.set_FirewallEnabled(NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_PUBLIC, true);
        }

        public static void AddRules(List<TestFirewallRule> rules)
        {
            var NetFWPolicyType = Type.GetTypeFromProgID("HNetCfg.FwPolicy2", false);
            var mgr = (INetFwPolicy2)Activator.CreateInstance(NetFWPolicyType);

            foreach (var rule in rules)
            {
                var NetFwRuleType = Type.GetTypeFromProgID("HNetCfg.FwRule", false);
                NetFwRule newRule = (NetFwRule)Activator.CreateInstance(NetFwRuleType);
                newRule.Name = rule.Name;
                newRule.Description = rule.Name;
                newRule.Protocol = (int)rule.Protocol;
                newRule.ApplicationName = rule.ApplicationPath;
                newRule.LocalPorts = rule.Ports;
                newRule.Grouping = rule.Grouping;
                mgr.Rules.Add(newRule);
            }
        }

        public static void RemoveRules(List<TestFirewallRule> rules)
        {
            var NetFWPolicyType = Type.GetTypeFromProgID("HNetCfg.FwPolicy2", false);
            var mgr = (INetFwPolicy2)Activator.CreateInstance(NetFWPolicyType);

            foreach (var rule in rules)
            {
                mgr.Rules.Remove(rule.Name);
            }
        }

        public static void RemoveWinFabRules()
        {
            var NetFWPolicyType = Type.GetTypeFromProgID("HNetCfg.FwPolicy2", false);
            var mgr = (INetFwPolicy2)Activator.CreateInstance(NetFWPolicyType);

            List<TestFirewallRule> rules = new List<TestFirewallRule>();
            foreach (var rule in mgr.Rules)
            {
                NetFwRule fwRule = (NetFwRule)rule;
                if (!String.IsNullOrEmpty(fwRule.Grouping) && ((NetFwRule)rule).Grouping.Equals("WindowsFabric"))
                {
                    rules.Add(new TestFirewallRule((NetFwRule)rule));
                }
            }
            TestFirewallManiuplator.RemoveRules(rules);
        }

        public static List<TestFirewallRule> GetRules()
        {
            var NetFWPolicyType = Type.GetTypeFromProgID("HNetCfg.FwPolicy2", false);
            var mgr = (INetFwPolicy2)Activator.CreateInstance(NetFWPolicyType);
            List<TestFirewallRule> rules = new List<TestFirewallRule>();
            foreach (var rule in mgr.Rules)
            {
                rules.Add(new TestFirewallRule((NetFwRule)rule));
            }
            return rules;
        }

        public static NET_FW_PROFILE_TYPE2_ GetCurrentProfile()
        {
            var NetFWPolicyType = Type.GetTypeFromProgID("HNetCfg.FwPolicy2", false);
            var policy = (INetFwPolicy2)Activator.CreateInstance(NetFWPolicyType);
            return ((NET_FW_PROFILE_TYPE2_)Enum.Parse(typeof(NET_FW_PROFILE_TYPE2_), (policy.CurrentProfileTypes).ToString()));
        }

        public static TestFirewallRule GetServiceFabricDNSRule()
        {
            string FirewallGroupName = FabricNodeFirewallRules.WindowsFabricGrouping;
            string FirewallRuleName = FirewallGroupName + ".Dns.v1";
            string FirewallRuleDescription = "Inbound rule for ServiceFabric DNS operations";
            int PortNumber = 53;

            NetFwRule rule = new NetFwRuleClass
            {
                Name = FirewallRuleName,
                Grouping = FirewallGroupName,
                Protocol = (int)NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_UDP,
                Direction = NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN,
                LocalPorts = PortNumber.ToString(),
                Profiles = (int)NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_ALL,
                Description = FirewallRuleDescription,
                LocalAddresses = "*",
                RemoteAddresses = "*",
                Action = NET_FW_ACTION_.NET_FW_ACTION_ALLOW,
                Enabled = true,
            };

            TestFirewallRule dnsRule = new TestFirewallRule(rule);
            return dnsRule;
        }
    }

    class FirewallUtility
    {
        public static bool IsAdminUser()
        {
            try
            {
                using (WindowsIdentity identity = WindowsIdentity.GetCurrent())
                {
                    WindowsPrincipal principal = new WindowsPrincipal(identity);
                    SecurityIdentifier sidAdmin = new SecurityIdentifier(WellKnownSidType.BuiltinAdministratorsSid, null);
                    return principal.IsInRole(sidAdmin);
                }
            }
            catch (Exception ex)
            {
                Verify.Fail(ex.ToString());
            }

            return false; // To satisfy compiler
        }
    }
}
