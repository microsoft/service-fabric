// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Tests
{
    using System;
    using System.Configuration;
    using System.Fabric.Health;
    using Filters;
    using VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ServiceConfigurationTests
    {
        [TestMethod]
        public void ServiceFilter_ReportsFor_AllHealthStates_When_ReportServiceHealth_IsSetTo_Always()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "Always";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled(HealthState.Ok));
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled(HealthState.Error));
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled(HealthState.Warning));
        }

        [TestMethod]
        public void ServiceFilter_ReportsFor_WarningOrError_HealthStates_When_ReportServiceHealth_IsSetTo_OnWarningOrError()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "OnWarningOrError";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled(HealthState.Ok));
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled(HealthState.Error));
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled(HealthState.Warning));
        }

        [TestMethod]
        public void ServiceFilter_ReportsFor_None_When_ReportServiceHealth_IsSetTo_Never()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "Never";
            configData.ReportPartitionHealth = "Never";
            configData.ReportReplicaHealth = "Never";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled(HealthState.Ok));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled(HealthState.Error));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled(HealthState.Warning));
        }

        [TestMethod]
        public void PartitionFilter_ReportsFor_AllHealthStates_When_ReportPartitionHealth_IsSetTo_Always()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportPartitionHealth = "Always";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled(HealthState.Ok));
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled(HealthState.Error));
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled(HealthState.Warning));
        }

        [TestMethod]
        public void PartitionFilter_ReportsFor_WarningOrError_HealthStates_When_ReportPartitionHealth_IsSetTo_OnWarningOrError()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportPartitionHealth = "OnWarningOrError";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled(HealthState.Ok));
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled(HealthState.Error));
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled(HealthState.Warning));
        }

        [TestMethod]
        public void PartitionFilter_Never_Reports_HealthStates_When_ReportPartitionHealth_IsSetTo_Never()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportPartitionHealth = "Never";
            configData.ReportReplicaHealth = "Never";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled(HealthState.Ok));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled(HealthState.Error));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled(HealthState.Warning));
        }

        [TestMethod]
        public void ReplicaFilter_ReportsFor_AllHealthStates_When_ReportReplicaHealth_IsSetTo_Always()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportReplicaHealth = "Always";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled(HealthState.Ok));
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled(HealthState.Error));
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled(HealthState.Warning));
        }

        [TestMethod]
        public void ReplicaFilter_ReportsFor_WarningOrError_HealthStates_When_ReportReplicaHealth_IsSetTo_OnWarningOrError()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportReplicaHealth = "OnWarningOrError";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled(HealthState.Ok));
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled(HealthState.Error));
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled(HealthState.Warning));
        }

        [TestMethod]
        public void ReplicaFilter_Never_Reports_HealthStates_When_ReportReplicaHealth_IsSetTo_Never()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportReplicaHealth = "Never";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled(HealthState.Ok));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled(HealthState.Error));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled(HealthState.Warning));
        }

        [TestMethod]
        public void HealthEventsFilter_ReportsFor_AllHealthStates_When_ReportHealthEvents_IsSetTo_Always()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportHealthEvents = "Always";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled(HealthState.Ok));
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled(HealthState.Error));
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled(HealthState.Warning));
        }

        [TestMethod]
        public void HealthEventsFilter_ReportsFor_WarningOrError_HealthStates_When_ReportHealthEvents_IsSetTo_OnWarningOrError()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportHealthEvents = "OnWarningOrError";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled(HealthState.Ok));
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled(HealthState.Error));
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled(HealthState.Warning));
        }

        [TestMethod]
        public void HealthEventsFilter_ReportsFor_None_When_ReportHealthEvents_IsSetTo_Never()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportHealthEvents = "Never";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled(HealthState.Ok));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled(HealthState.Error));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled(HealthState.Warning));
        }

        [TestMethod]
        public void ServiceFilter_ReportsFor_WhitelistedApps()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "Always";
            configData.ApplicationsThatReportServiceHealth = "fabric:/System,fabric:/Monitoring";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void ServiceFilter_ReportsFor_AllApps_WhenWhitelist_IsSetTo_All()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "Always";
            configData.ApplicationsThatReportServiceHealth = "All";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled(null));
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled("System"));
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void ServiceFilter_ReportsFor_NoneButSystemApp_WhenWhitelist_IsEmpty()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "Always";
            configData.ReportPartitionHealth = "Never";
            configData.ReportReplicaHealth = "Never";
            configData.ApplicationsThatReportServiceHealth = string.Empty;

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void ServiceFilter_ReportsFor_NoneButSystemApp_WhenWhitelist_IsNull()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "Always";
            configData.ReportPartitionHealth = "Never";
            configData.ReportReplicaHealth = "Never";
            configData.ApplicationsThatReportServiceHealth = null;

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void ServiceFilter_ReportsFor_None_ReportServiceHealth_IsSetTo_Never()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "Never";
            configData.ReportPartitionHealth = "Never";
            configData.ReportReplicaHealth = "Never";
            configData.ApplicationsThatReportServiceHealth = "fabric:/System,fabric:/Monitoring";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.ServiceHealthFilter.IsEntityEnabled("All"));

            configData.ApplicationsThatReportServiceHealth = "All";

            var filterRepo2 = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo2.ServiceHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo2.ServiceHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo2.ServiceHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo2.ServiceHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo2.ServiceHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo2.ServiceHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo2.ServiceHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void PartitionFilter_ReportsFor_WhitelistedApps()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportPartitionHealth = "Always";
            configData.ApplicationsThatReportPartitionHealth = "fabric:/System,fabric:/Monitoring";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void PartitionFilter_ReportsFor_AllApps_WhenWhitelist_IsSetTo_All()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportPartitionHealth = "Always";
            configData.ApplicationsThatReportServiceHealth = "All";
            configData.ApplicationsThatReportPartitionHealth = "All";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled(null));
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled("System"));
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void PartitionFilter_ReportsFor_NoneButSystemApp_WhenWhitelist_IsEmpty()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "Always";
            configData.ReportPartitionHealth = "Always";
            configData.ReportReplicaHealth = "Never";
            configData.ApplicationsThatReportPartitionHealth = string.Empty;

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void PartitionFilter_ReportsFor_NoneButSystemApp_WhenWhitelist_IsNull()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "Always";
            configData.ReportPartitionHealth = "Always";
            configData.ReportReplicaHealth = "Never";
            configData.ApplicationsThatReportPartitionHealth = null;

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void PartitionFilter_ReportsFor_None_ReportPartitionHealth_IsSetTo_Never()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "Never";
            configData.ReportPartitionHealth = "Never";
            configData.ReportReplicaHealth = "Never";
            configData.ApplicationsThatReportPartitionHealth = "fabric:/System,fabric:/Monitoring";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.PartitionHealthFilter.IsEntityEnabled("All"));

            configData.ApplicationsThatReportPartitionHealth = "All";

            var filterRepo2 = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo2.PartitionHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo2.PartitionHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo2.PartitionHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo2.PartitionHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo2.PartitionHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo2.PartitionHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo2.PartitionHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void ReplicaFilter_ReportsFor_WhitelistedApps()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportReplicaHealth = "Always";
            configData.ApplicationsThatReportReplicaHealth = "fabric:/System,fabric:/Monitoring";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void ReplicaFilter_ReportsFor_AllApps_WhenWhitelist_IsSetTo_All()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportReplicaHealth = "Always";
            configData.ApplicationsThatReportServiceHealth = "All";
            configData.ApplicationsThatReportPartitionHealth = "All";
            configData.ApplicationsThatReportReplicaHealth = "All";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled(null));
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled("System"));
            Assert.IsTrue(filterRepo.ReplicaHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void ReplicaFilter_ReportsFor_None_WhenWhitelist_IsEmpty()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "Always";
            configData.ReportPartitionHealth = "Always";
            configData.ReportReplicaHealth = "Never";
            configData.ApplicationsThatReportReplicaHealth = string.Empty;

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void ReplicaFilter_ReportsFor_NoneButSystemApp_WhenWhitelist_IsNull()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "Always";
            configData.ReportPartitionHealth = "Always";
            configData.ReportReplicaHealth = "Never";
            configData.ApplicationsThatReportReplicaHealth = null;

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void ReplicaFilter_ReportsFor_None_ReportReplicaHealth_IsSetTo_Never()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportServiceHealth = "Never";
            configData.ReportPartitionHealth = "Never";
            configData.ReportReplicaHealth = "Never";
            configData.ApplicationsThatReportReplicaHealth = "fabric:/System,fabric:/Monitoring";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.ReplicaHealthFilter.IsEntityEnabled("All"));

            configData.ApplicationsThatReportReplicaHealth = "All";

            var filterRepo2 = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo2.ReplicaHealthFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo2.ReplicaHealthFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo2.ReplicaHealthFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo2.ReplicaHealthFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo2.ReplicaHealthFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo2.ReplicaHealthFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo2.ReplicaHealthFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void HealthEventsFilter_ReportsFor_WhitelistedApps()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportHealthEvents = "Always";
            configData.ApplicationsThatReportHealthEvents = "fabric:/System,fabric:/Monitoring";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void HealthEventsFilter_ReportsFor_AllApps_WhenWhitelist_IsSetTo_All()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportHealthEvents = "Always";
            configData.ApplicationsThatReportHealthEvents = "All";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled(string.Empty));
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled(null));
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled("System"));
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void HealthEventsFilter_ReportsFor_NoneButSystemApp_WhenWhitelist_IsEmpty()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportHealthEvents = "Always";
            configData.ApplicationsThatReportHealthEvents = string.Empty;

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void HealthEventsFilter_ReportsFor_NoneButSystemApp_WhenWhitelist_IsNull()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportHealthEvents = "Always";
            configData.ApplicationsThatReportHealthEvents = null;

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsTrue(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void HealthEventsFilter_ReportsFor_None_ReportHealthEvents_IsSetTo_Never()
        {
            var configData = new ServiceConfigurationData();
            configData.ReportHealthEvents = "Never";
            configData.ApplicationsThatReportHealthEvents = "fabric:/System,fabric:/Monitoring";

            var filterRepo = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo.HealthEventsFilter.IsEntityEnabled("All"));

            configData.ApplicationsThatReportHealthEvents = "All";

            var filterRepo2 = new EntityFilterRepository(configData);
            Assert.IsFalse(filterRepo2.HealthEventsFilter.IsEntityEnabled("fabric:/System"));
            Assert.IsFalse(filterRepo2.HealthEventsFilter.IsEntityEnabled("fabric:/Monitoring"));
            Assert.IsFalse(filterRepo2.HealthEventsFilter.IsEntityEnabled("fabric:/TestApp"));
            Assert.IsFalse(filterRepo2.HealthEventsFilter.IsEntityEnabled(string.Empty));
            Assert.IsFalse(filterRepo2.HealthEventsFilter.IsEntityEnabled(null));
            Assert.IsFalse(filterRepo2.HealthEventsFilter.IsEntityEnabled("System"));
            Assert.IsFalse(filterRepo2.HealthEventsFilter.IsEntityEnabled("All"));
        }

        [TestMethod]
        public void PartitionFilter_MustBe_SetTo_Never_When_ServiceFilter_IsSetTo_Never()
        {
            try
            {
                var configData = new ServiceConfigurationData();
                configData.ReportServiceHealth = "Never";
                configData.ReportPartitionHealth = "Always";

                var filterRepo = new EntityFilterRepository(configData);
            }
            catch (ConfigurationErrorsException)
            {
                return;
            }

            Assert.Fail("Partition filter must be subset of Service filter. Expected ConfigurationErrorsException was not thrown.");
        }

        [TestMethod]
        public void ReplicaFilter_MustBe_SetTo_Never_When_PartitionFilter_IsSetTo_Never()
        {
            try
            {
                var configData = new ServiceConfigurationData();
                configData.ReportServiceHealth = "Always";
                configData.ReportPartitionHealth = "Never";
                configData.ReportReplicaHealth = "OnWarningOrError";

                var filterRepo = new EntityFilterRepository(configData);
            }
            catch (ConfigurationErrorsException)
            {
                return;
            }

            Assert.Fail("Replica filter must be subset of Partition filter. Expected ConfigurationErrorsException was not thrown.");
        }

        [TestMethod]
        public void PartitionFilter_Whitelist_MustBe_SubsetOf_ServiceFilter()
        {
            try
            {
                var configData = new ServiceConfigurationData();
                configData.ReportServiceHealth = "Always";
                configData.ReportPartitionHealth = "Always";
                configData.ApplicationsThatReportServiceHealth = "fabric:/System,fabric:/Monitoring";
                configData.ApplicationsThatReportPartitionHealth = "All";

                var filterRepo = new EntityFilterRepository(configData);
            }
            catch (ConfigurationErrorsException)
            {
                return;
            }

            Assert.Fail("Partition filter must be subset of Service filter. Expected ConfigurationErrorsException was not thrown.");
        }

        [TestMethod]
        public void ReplicaFilter_Whitelist_MustBe_SubsetOf_PartitionFilter()
        {
            try
            {
                var configData = new ServiceConfigurationData();
                configData.ReportServiceHealth = "Always";
                configData.ReportPartitionHealth = "Always";
                configData.ReportReplicaHealth = "Always";
                configData.ApplicationsThatReportPartitionHealth = "fabric:/System,fabric:/Monitoring";
                configData.ApplicationsThatReportReplicaHealth = "fabric:/System,fabric:/TestApp,fabric:/Monitoring";

                var filterRepo = new EntityFilterRepository(configData);
            }
            catch (ConfigurationErrorsException)
            {
                return;
            }

            Assert.Fail("Replica filter must be subset of Partition filter. Expected ConfigurationErrorsException was not thrown.");
        }

        [TestMethod]
        public void ServiceFilter_Throws_ForInvalidValueOf_ReportServiceHealth()
        {
            try
            {
                var configData = new ServiceConfigurationData();
                configData.ReportServiceHealth = "Blah";

                var filterRepo = new EntityFilterRepository(configData);
            }
            catch (ConfigurationErrorsException)
            {
                return;
            }

            Assert.Fail("Invalid value for ReportServiceHealth. Expected ConfigurationErrorsException was not thrown.");
        }

        [TestMethod]
        public void PartitionFilter_Throws_ForInvalidValueOf_ReportPartitionHealth()
        {
            try
            {
                var configData = new ServiceConfigurationData();
                configData.ReportPartitionHealth = "Alll";

                var filterRepo = new EntityFilterRepository(configData);
            }
            catch (ConfigurationErrorsException)
            {
                return;
            }

            Assert.Fail("Invalid value for ReportPartitionHealth. Expected ConfigurationErrorsException was not thrown.");
        }

        [TestMethod]
        public void ReplicaFilter_Throws_ForInvalidValueOf_ReportReplicaHealth()
        {
            try
            {
                var configData = new ServiceConfigurationData();
                configData.ReportReplicaHealth = "OnError";

                var filterRepo = new EntityFilterRepository(configData);
            }
            catch (ConfigurationErrorsException)
            {
                return;
            }

            Assert.Fail("Invalid value for ReportReplicaHealth. Expected ConfigurationErrorsException was not thrown.");
        }

        [TestMethod]
        public void HealthEventsFilter_Throws_ForInvalidValueOf_ReportHealthEvents()
        {
            try
            {
                var configData = new ServiceConfigurationData();
                configData.ReportHealthEvents = "OnError";

                var filterRepo = new EntityFilterRepository(configData);
            }
            catch (ConfigurationErrorsException)
            {
                return;
            }

            Assert.Fail("Invalid value for ReportHealthEvents. Expected ConfigurationErrorsException was not thrown.");
        }
    }
}