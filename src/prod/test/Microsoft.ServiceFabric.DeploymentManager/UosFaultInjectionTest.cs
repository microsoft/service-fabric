// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Linq;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using System.Fabric;
    using System.Fabric.UpgradeOrchestration.Service;

    [TestClass]
    public class UosFaultInjectionTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void FaultInjectionConfigTest()
        {
            UpgradeFlow upgradeFlow = UpgradeFlow.RollingBack;
            int step = 100;
            FaultInjectionConfig config = new FaultInjectionConfig(upgradeFlow, step);
            Assert.AreEqual(upgradeFlow, config.FaultFlow);
            Assert.AreEqual(step, config.FaultStep);

            Assert.AreEqual(upgradeFlow + ":" + step, config.ToString());

            Assert.IsFalse(config.Equals(null));
            Assert.IsFalse(config.Equals(step));
            Assert.IsTrue(config.Equals(config));

            FaultInjectionConfig config2 = new FaultInjectionConfig(upgradeFlow, step);
            Assert.IsTrue(config.Equals(config2));
            config2.FaultStep++;
            Assert.IsFalse(config.Equals(config2));
            config2.FaultStep--;
            Assert.IsTrue(config.Equals(config2));
            config2.FaultFlow = UpgradeFlow.RollingForward;
            Assert.IsFalse(config.Equals(config2));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TryGetFaultInjectionConfigTest()
        {
            Assert.IsNull(this.InternalTryGetFaultInjectionConfigTest(faultStep: null, faultFlow: string.Empty));
            Assert.IsNull(this.InternalTryGetFaultInjectionConfigTest(faultStep: "0", faultFlow: string.Empty));
            Assert.IsNull(this.InternalTryGetFaultInjectionConfigTest(faultStep: null, faultFlow: "RollingBack"));
            Assert.IsNull(this.InternalTryGetFaultInjectionConfigTest(faultStep: "-1", faultFlow: "RollingBack"));
            Assert.IsNull(this.InternalTryGetFaultInjectionConfigTest(faultStep: "0", faultFlow: "hiahia"));

            string faultStep = "1";
            string faultFlow = "RollingBack";
            FaultInjectionConfig config = this.InternalTryGetFaultInjectionConfigTest(faultStep, faultFlow);
            Assert.IsTrue(config.Equals(new FaultInjectionConfig((UpgradeFlow)Enum.Parse(typeof(UpgradeFlow), faultFlow), int.Parse(faultStep))));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ShouldReportUnhealthyTest()
        {
            FaultInjectionConfig config;
            StandAloneCluster cluster = Utility.PopulateStandAloneClusterWithBaselineJson("myClusterConfig.UnSecure.DevCluster.json");
            cluster.TargetNodeConfig = new ClusterNodeConfig(null, 0);

            config = new FaultInjectionConfig(UpgradeFlow.RollingForward, 0);
            
            StandAloneSimpleClusterUpgradeState upgradeState1 = new StandAloneSimpleClusterUpgradeState(cluster.TargetCsmConfig, cluster.TargetWrpConfig, cluster.TargetNodeConfig, cluster, new StandAloneTraceLogger("StandAloneDeploymentManager"));
            Assert.IsTrue(FaultInjectionHelper.ShouldReportUnhealthy(upgradeState1, UpgradeFlow.RollingForward, config));
            Assert.IsFalse(FaultInjectionHelper.ShouldReportUnhealthy(upgradeState1, UpgradeFlow.RollingBack, config));

            MockUpMultiphaseClusterUpgradeState upgradeState2 = new MockUpMultiphaseClusterUpgradeState(2, cluster);
            upgradeState2.CurrentListIndex = 0;
            Assert.IsTrue(FaultInjectionHelper.ShouldReportUnhealthy(upgradeState2, UpgradeFlow.RollingForward, config));
            Assert.IsFalse(FaultInjectionHelper.ShouldReportUnhealthy(upgradeState2, UpgradeFlow.RollingBack, config));
            upgradeState2.CurrentListIndex = 1;
            Assert.IsFalse(FaultInjectionHelper.ShouldReportUnhealthy(upgradeState2, UpgradeFlow.RollingForward, config));

            config = new FaultInjectionConfig(UpgradeFlow.RollingForward, 1);
            Assert.IsFalse(FaultInjectionHelper.ShouldReportUnhealthy(upgradeState1, UpgradeFlow.RollingForward, config));
            Assert.IsTrue(FaultInjectionHelper.ShouldReportUnhealthy(upgradeState2, UpgradeFlow.RollingForward, config));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GetUpgradeFlowTest()
        {
            StandAloneCluster cluster = Utility.PopulateStandAloneClusterWithBaselineJson("myClusterConfig.UnSecure.DevCluster.json");
            cluster.TargetNodeConfig = new ClusterNodeConfig(null, 0);

            StandAloneSimpleClusterUpgradeState upgradeState1 = new StandAloneSimpleClusterUpgradeState(cluster.TargetCsmConfig, cluster.TargetWrpConfig, cluster.TargetNodeConfig, cluster, new StandAloneTraceLogger("StandAloneDeploymentManager"));
            Assert.AreEqual(UpgradeFlow.RollingForward, FaultInjectionHelper.GetUpgradeFlow(upgradeState1));

            MockUpMultiphaseClusterUpgradeState upgradeState2 = new MockUpMultiphaseClusterUpgradeState(2, cluster);
            upgradeState2.UpgradeUnsuccessful = false;
            Assert.AreEqual(UpgradeFlow.RollingForward, FaultInjectionHelper.GetUpgradeFlow(upgradeState2));

            upgradeState2.UpgradeUnsuccessful = true;
            Assert.AreEqual(UpgradeFlow.RollingBack, FaultInjectionHelper.GetUpgradeFlow(upgradeState2));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ReportUnhealthyIfNecessaryTest()
        {
            StandAloneCluster cluster = Utility.PopulateStandAloneClusterWithBaselineJson("myClusterConfig.UnSecure.DevCluster.json");
            cluster.TargetNodeConfig = new ClusterNodeConfig(null, 0);

            FabricUpgradeOrchestrationService service = new FabricUpgradeOrchestrationService(new StatefulServiceContext(), "hiahiaEndpoint", false);
            UpgradeOrchestrator orchestrator = service.Orchestrator;

            Assert.IsFalse(orchestrator.ReportUnhealthyIfNecessary(cluster));

            cluster.TargetCsmConfig.FabricSettings = new List<SettingsSectionDescription>()
            {
                new SettingsSectionDescription()
                {
                    Name = StringConstants.SectionName.UpgradeOrchestrationService,
                    Parameters = new List<SettingsParameterDescription>()
                    {
                        new SettingsParameterDescription()
                        {
                            Name = StringConstants.ParameterName.FaultStep,
                            Value = "1"
                        },
                        new SettingsParameterDescription()
                        {
                            Name = StringConstants.ParameterName.FaultFlow,
                            Value = "RollingBack"
                        },
                    }
                }
            };

            cluster.Pending = new MockUpMultiphaseClusterUpgradeState(2, cluster)
            {
                CurrentListIndex = 1
            };

            Assert.IsFalse(orchestrator.ReportUnhealthyIfNecessary(cluster));

            cluster.Pending = new MockUpMultiphaseClusterUpgradeState(2, cluster)
            {
                CurrentListIndex = 1,
                UpgradeUnsuccessful = true,
            };
            bool healthy = true;
            Assert.IsTrue(orchestrator.ReportUnhealthyIfNecessary(cluster, isHealthy => { healthy = isHealthy; }));
            Assert.IsFalse(healthy);
        }

        internal FaultInjectionConfig InternalTryGetFaultInjectionConfigTest(string faultStep, string faultFlow)
        {
            List<SettingsSectionDescription> fabricSettings = new List<SettingsSectionDescription>();

            if (faultStep != null || faultFlow != null)
            {
                SettingsSectionDescription uosSection = new SettingsSectionDescription()
                {
                    Name = StringConstants.SectionName.UpgradeOrchestrationService
                };

                fabricSettings.Add(uosSection);
                
                if (faultStep != null)
                {
                    uosSection.Parameters.Add(new SettingsParameterDescription()
                    {
                        Name = StringConstants.ParameterName.FaultStep,
                        Value = faultStep
                    });
                }

                if (faultFlow != null)
                {
                    uosSection.Parameters.Add(new SettingsParameterDescription()
                    {
                        Name = StringConstants.ParameterName.FaultFlow,
                        Value = faultFlow
                    });
                }
            }

            return FaultInjectionHelper.TryGetFaultInjectionConfig(fabricSettings);
        }

    }
}