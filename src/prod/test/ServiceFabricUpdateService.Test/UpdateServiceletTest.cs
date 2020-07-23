// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ServiceFabricUpdateService.Test
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;

    using Microsoft.ServiceFabric.DeploymentManager.Model;

    [TestClass]
    public class UpdateServiceletTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            ProgramConfig svcConfig = new ProgramConfig(
                ProgramParameterDefinitions.ExecutionModes.Service,
                new AppConfig(new Uri("http://hiahia.net:433", UriKind.Absolute), null, TestUtility.TestDirectory, new Uri("http://goalstate"), TimeSpan.FromHours(12), TimeSpan.FromHours(24)));
            TraceLogger logger = new TraceLogger(new MockUpTraceEventProvider());
            UpdateServicelet result = new UpdateServicelet(svcConfig, logger);

            Assert.IsNotNull(result.ApiHost);
            Assert.AreSame(logger, result.ApiHost.Logger);
            Assert.AreEqual(svcConfig.AppConfig.EndpointBaseAddress.AbsoluteUri, result.ApiHost.BaseAddress);
            Assert.IsNotNull(result.ApiHost.FilePathResolver);

            Assert.IsNotNull(result.PkgMgr);

            string dataRootPath = TestUtility.TestDirectory + "\\UpdateService";
            Assert.IsTrue(Directory.Exists(dataRootPath));
            Directory.Delete(dataRootPath, recursive: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void IntegrationTest()
        {
            TraceLogger traceLogger = new TraceLogger(new MockUpTraceEventProvider());
            Uri goalStateFileUri = this.PrepareTargetGoalStateFile();
            ProgramConfig config = new ProgramConfig(
                ProgramParameterDefinitions.ExecutionModes.Service,
                new AppConfig(
                    new Uri("http://localhost", UriKind.Absolute),
                    null,
                    TestUtility.TestDirectory + "\\Data",
                    goalStateFileUri,
                    TimeSpan.FromSeconds(3000),
                    TimeSpan.FromSeconds(3000)));
            UpdateServicelet server = new UpdateServicelet(config, traceLogger);
            server.Start();

            // ensure that the packages have been downloaded
            Thread.Sleep(10000);

            GoalState goalState;
            Assert.IsTrue(GoalState.TryCreate(new Uri("http://localhost/api/files/goalstate", UriKind.Absolute), server.Logger, out goalState));
            string packageDownloadDstDirectory = TestUtility.TestDirectory + "\\DownloadDstDir";
            if (!Directory.Exists(packageDownloadDstDirectory))
            {
                Directory.CreateDirectory(packageDownloadDstDirectory);
            }

            TaskManager tskMgr = new TaskManager(traceLogger);

            foreach (PackageDetails package in goalState.Packages)
            {
                string fileName = new Uri(package.TargetPackageLocation).Segments.Last();
                tskMgr.AddTask(Utility.DownloadFileAsync(package.TargetPackageLocation, packageDownloadDstDirectory + "\\" + fileName));
            }

            Assert.IsTrue(tskMgr.WaitForAllTasks());

            Assert.AreEqual(5, Directory.GetFiles(TestUtility.TestDirectory + "\\Data\\UpdateService\\Packages", "*.cab", SearchOption.TopDirectoryOnly).Length);

            
            server.Stop();

            Assert.IsFalse(GoalState.TryCreate(new Uri("http://localhost/api/files/goalstat", UriKind.Absolute), server.Logger, out goalState));

            Directory.Delete(TestUtility.TestDirectory + "\\Data", recursive: true);
            File.Delete(TestUtility.TestDirectory + "\\WsusIntegration.json");
            Directory.Delete(packageDownloadDstDirectory, recursive: true);
        }

        private Uri PrepareTargetGoalStateFile()
        {
            Uri goalStateFileUri = new Uri(TestUtility.TestDirectory + "\\goalstateV1.json", UriKind.Absolute);
            GoalState goalState;
            Assert.IsTrue(GoalState.TryCreate(goalStateFileUri, new TraceLogger(new MockUpTraceEventProvider()), out goalState));
            foreach (PackageDetails package in goalState.Packages)
            {
                package.TargetPackageLocation = TestUtility.TestDirectory + "\\" + new Uri(package.TargetPackageLocation, UriKind.Absolute).Segments.Last();
            }

            string result = TestUtility.TestDirectory + "\\WsusIntegration.json";
            File.WriteAllText(result, goalState.Serialize());
            return new Uri(result);
        }
    }
}