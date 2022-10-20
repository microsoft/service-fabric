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
    public class PackageManagerTest
    {
        private int goalStateFileIdx;

        private string[] goalStateFiles = new string[]
        {
            TestUtility.TestDirectory + "\\goalstateV1.json",
            TestUtility.TestDirectory + "\\goalstateV2.json",
        };

        private AutoResetEvent syncEvent = new AutoResetEvent(initialState: false);

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void FullCycleTest()
        {
            this.goalStateFileIdx = 0;

            string webApiBaseAddress = "http://hiahia.com:8080";
            string appDataRootPath = TestUtility.TestDirectory + "\\Data";
            string localFileRootPath = UpdateServicelet.GetUpdateServiceRootPath(appDataRootPath);
            PackageManager pkgMgr = new PackageManager(
                new FilePathResolver(webApiBaseAddress, localFileRootPath),
                new Uri("http://dummy"),
                TimeSpan.FromSeconds(10),
                new TraceLogger(new MockUpTraceEventProvider()));
            pkgMgr.TimerFired += this.OnTimerFired;
            pkgMgr.TimerCompleted += this.OnTimerCompleted;

            pkgMgr.Start();
            this.syncEvent.WaitOne();

            // allow the last no-op timer to be scheduled if allowed by the system
            Thread.Sleep(3000);

            pkgMgr.Stop();

            // verify that the timer never gets fired again.
            Thread.Sleep(20000);
            Assert.AreEqual(3, this.goalStateFileIdx);
            Directory.Delete(appDataRootPath, recursive: true);
        }

        internal void OnTimerFired(object sender, EventArgs e)
        {
            int idx = this.goalStateFileIdx;
            Assert.IsTrue(idx <= this.goalStateFiles.Length + 1);
            if (idx >= this.goalStateFiles.Length)
            {
                return;
            }

            PackageManager pkgMgr = (PackageManager)sender;
            Uri goalStateFileUri = new Uri(this.goalStateFiles[this.goalStateFileIdx], UriKind.Absolute);
            GoalState goalState;
            Assert.IsTrue(GoalState.TryCreate(goalStateFileUri, pkgMgr.Logger, out goalState));
            foreach (PackageDetails package in goalState.Packages)
            {
                package.TargetPackageLocation = TestUtility.TestDirectory + "\\" + new Uri(package.TargetPackageLocation, UriKind.Absolute).Segments.Last();
            }

            File.WriteAllText(goalStateFileUri.LocalPath, goalState.Serialize());
            pkgMgr.GoalStateFileUri = goalStateFileUri;
        }

        internal void OnTimerCompleted(object sender, EventArgs e)
        {
            int idx = this.goalStateFileIdx;
            Assert.IsTrue(idx <= this.goalStateFiles.Length);
            if (idx == this.goalStateFiles.Length)
            {
                idx--;
            }

            PackageManager pkgMgr = (PackageManager)sender;
            IEnumerable<string> localPackages = pkgMgr.FilePathResolver.GetAllLocalPackages();
            if (idx == 0)
            {
                Assert.AreEqual(5, localPackages.Count());
            }
            else if (idx == 1)
            {
                Assert.AreEqual(6, localPackages.Count());
            }

            Uri goalStateFileUri = new Uri(this.goalStateFiles[idx], UriKind.Absolute);
            GoalState goalState;
            Assert.IsTrue(GoalState.TryCreate(goalStateFileUri, pkgMgr.Logger, out goalState));
            IEnumerable<string> targetPackages = goalState.Packages.Select(p => p.Version);
            Assert.IsTrue(targetPackages.All(version => localPackages.Count(local => local.Contains(version)) == 1));

            this.goalStateFileIdx++;
            if (this.goalStateFileIdx == this.goalStateFiles.Length + 1)
            {
                this.syncEvent.Set();
            }
        }
    }
}