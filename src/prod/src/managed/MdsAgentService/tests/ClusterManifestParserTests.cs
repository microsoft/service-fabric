// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace MdsAgentSvc.Tests
{
    using System.IO;
    using System.Reflection;
    using FabricMdsAgentSvc;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ClusterManifestParserTests
    {
        private const string ClusterManifestWithMdsUploaderFileName = "ClusterManifestWithUploader.xml";
        private const string ClusterManifestDisabledFileName = "ClusterManifestDisabled.xml";

        private static string clusterManifestWithMdsUploader;
        private static string clusterManifestDisabled;

        [ClassInitialize]
        public static void ClassSetup(TestContext context)
        {
            var executingAssemblyPath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location); 
            clusterManifestWithMdsUploader = File.ReadAllText(Path.Combine(executingAssemblyPath, ClusterManifestWithMdsUploaderFileName));
            clusterManifestDisabled = File.ReadAllText(Path.Combine(executingAssemblyPath, ClusterManifestDisabledFileName));
        }

        [TestMethod]
        public void ClusterManifestParser_ParseDirectoryTest()
        {
            var expectedDirectory = @"C:\WFRoot\MDS\Traces";
            var actualDirectory = ClusterManifestParser.ParseMdsUploaderPath(clusterManifestWithMdsUploader);
            Assert.AreEqual(expectedDirectory, actualDirectory, "Directory should be parsed for MdsUploader.");
        }

        [TestMethod]
        public void ClusterManifestParser_NotEnabledTest()
        {
            var actualDirectory = ClusterManifestParser.ParseMdsUploaderPath(clusterManifestDisabled);
            Assert.AreEqual(null, actualDirectory, "When no MDS consumer is present, null should be returned.");
        }
    }
}