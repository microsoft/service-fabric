// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ServiceFabricUpdateService.Test
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;

    using ParameterNames = Microsoft.ServiceFabric.ServiceFabricUpdateService.ProgramParameterDefinitions.ParameterNames;
    using ExecutionModes = Microsoft.ServiceFabric.ServiceFabricUpdateService.ProgramParameterDefinitions.ExecutionModes;

    [TestClass]
    public class AppConfigTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            Uri uri = new Uri("https://hiahia:100");
            string connectionString = "heihei";
            AppConfig result = new AppConfig(uri, connectionString, TestUtility.TestDirectory, null, TimeSpan.Zero, TimeSpan.Zero);
            Assert.AreEqual(uri, result.EndpointBaseAddress);
            Assert.AreEqual(connectionString, result.DiagnosticsStoreConnectionString);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TryCreateTest()
        {
            AppConfig result;
            TraceLogger logger = new TraceLogger(new MockUpTraceEventProvider());
            string validHttp = "http://hiahia:80";
            string validHttps = "https://hiahia:443";
            string invalidHttp = "ftp://hiahia:80";
            string invalidHttps = "https:/hiahia:80";
            string validConnectionString = @"\\myShare";
            string invalidConnectionString = @"http://hiahia.com";

            // positive
            result = null;
            Assert.IsTrue(AppConfig.TryCreate(
                new NameValueCollection() { { AppConfig.SettingNames.EndpointBaseAddress, validHttp } },
                logger,
                out result));
            Assert.IsNotNull(result);

            result = null;
            Assert.IsTrue(AppConfig.TryCreate(
                new NameValueCollection()
                {
                    { AppConfig.SettingNames.EndpointBaseAddress, validHttps },
                },
                logger,
                out result));
            Assert.IsNotNull(result);
            Assert.IsFalse(string.IsNullOrWhiteSpace(result.DataRootPath));
            Assert.AreEqual(TimeSpan.FromHours(12), result.PackageUpdateInterval);
            Assert.IsFalse(string.IsNullOrWhiteSpace(result.GoalStateFileUri.AbsoluteUri));
            Assert.AreEqual(TimeSpan.FromHours(24), result.TelemetryUploadInterval);

            result = null;
            Assert.IsTrue(AppConfig.TryCreate(
                new NameValueCollection()
                {
                    { AppConfig.SettingNames.EndpointBaseAddress, validHttp },
                    { AppConfig.SettingNames.DiagnosticsStoreConnectionString, validConnectionString }
                },
                logger,
                out result));
            Assert.IsNotNull(result);

            result = null;
            Assert.IsTrue(AppConfig.TryCreate(
                new NameValueCollection()
                {
                    { AppConfig.SettingNames.EndpointBaseAddress, validHttps },
                    { AppConfig.SettingNames.DiagnosticsStoreConnectionString, validConnectionString }
                },
                logger,
                out result));
            Assert.IsNotNull(result);

            //negative: 0 parameters
            Assert.IsFalse(AppConfig.TryCreate(new NameValueCollection(), logger, out result));

            // negative: 1 parameter
            Assert.IsFalse(AppConfig.TryCreate(
                new NameValueCollection() { { AppConfig.SettingNames.EndpointBaseAddress, invalidHttp } },
                logger,
                out result));

            Assert.IsFalse(AppConfig.TryCreate(
                new NameValueCollection() { { AppConfig.SettingNames.EndpointBaseAddress, invalidHttps } },
                logger,
                out result));

            // negative: 2 parameters
            Assert.IsFalse(AppConfig.TryCreate(
                new NameValueCollection()
                {
                    { AppConfig.SettingNames.EndpointBaseAddress, validHttp },
                    { AppConfig.SettingNames.DiagnosticsStoreConnectionString, invalidConnectionString }
                },
                logger,
                out result));

            Assert.IsFalse(AppConfig.TryCreate(
                new NameValueCollection()
                {
                    { AppConfig.SettingNames.EndpointBaseAddress, validHttps },
                    { AppConfig.SettingNames.DiagnosticsStoreConnectionString, invalidConnectionString }
                },
                logger,
                out result));

            Assert.IsFalse(AppConfig.TryCreate(
                new NameValueCollection()
                {
                    { AppConfig.SettingNames.EndpointBaseAddress, invalidHttp },
                    { AppConfig.SettingNames.DiagnosticsStoreConnectionString, validConnectionString }
                },
                logger,
                out result));

            Assert.IsFalse(AppConfig.TryCreate(
                new NameValueCollection()
                {
                    { AppConfig.SettingNames.EndpointBaseAddress, invalidHttps },
                    { AppConfig.SettingNames.DiagnosticsStoreConnectionString, validConnectionString }
                },
                logger,
                out result));
        }
    }
}