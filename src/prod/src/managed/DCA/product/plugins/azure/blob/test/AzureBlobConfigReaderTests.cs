// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace AzureFileUploader.Test
{
    using System;
    using System.Diagnostics.Tracing;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;

    using FabricDCA;
    using FabricDCA.Test;

    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Moq;

    /// <summary>
    /// Provides unit tests for the <see cref="AzureBlobConfigReader"/> class.
    /// </summary>
    [TestClass]
    public class AzureBlobConfigReaderTests
    {
        private const string TestSectionName = "SectionName";
        private const string TestLogSourceId = "AzureBlobConfigReaderTests";
        private FabricEvents.ExtensionsEvents testTraceSource;

        /// <summary>
        /// Setup before each test method.
        /// </summary>
        [TestInitialize]
        public void TestSetup()
        {
            this.testTraceSource = new ErrorAndWarningFreeTraceEventSource();
        }

        [TestMethod]
        public void TestGetEnabled()
        {
            const bool ExpectedValue = true;
            var mockConfigReader = new Mock<IConfigReader>(MockBehavior.Strict);
            mockConfigReader
                .Setup(cr => cr.GetUnencryptedConfigValue(TestSectionName, "IsEnabled", false))
                .Returns(ExpectedValue);

            var reader = new AzureBlobConfigReader(mockConfigReader.Object, TestSectionName, this.testTraceSource, TestLogSourceId);
            var actual = reader.GetEnabled();
            Assert.AreEqual(ExpectedValue, actual);
        }

        [TestMethod]
        public void TestGetUploadInterval()
        {
            const int TestConfigValue = 1234;
            var expectedValue = TimeSpan.FromMinutes(TestConfigValue);
            var mockConfigReader = new Mock<IConfigReader>(MockBehavior.Strict);
            mockConfigReader
                .Setup(cr => cr.GetUnencryptedConfigValue(TestSectionName, "UploadIntervalInMinutes", (int)AzureConstants.DefaultBlobUploadIntervalMinutes.TotalMinutes))
                .Returns(TestConfigValue);

            var reader = new AzureBlobConfigReader(mockConfigReader.Object, TestSectionName, this.testTraceSource, TestLogSourceId);
            var actual = reader.GetUploadInterval();
            Assert.AreEqual(expectedValue, actual);
        }
    }
}