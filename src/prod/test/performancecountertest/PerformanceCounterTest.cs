// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Service.Fabric.CIT.PerformanceCounter
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Xml;

    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class PerformanceCounterTest
    {
        //// see TestPerformanceProvider\TestPerformanceProvider.PerformanceCounters.h for values

        private const string Category1 = "Service Fabric CIT TestCounterSet1";
        private const string Category2 = "Service Fabric CIT TestCounterSet2";

        private const string Counter1 = "Counter 1";
        private const string Counter2 = "Counter 2";
        private const string Counter3 = "Counter 3";
        private const string Counter4 = "Counter 4";

        private static string manifestPath;

        private static string performanceProviderPath;

        [ClassInitialize]
        public static void Initialize(TestContext context)
        {
            PerformanceCounterTest.performanceProviderPath = Path.Combine(context.TestDeploymentDir, "TestPerformanceProvider.exe");

            PerformanceCounterTest.manifestPath = TestCounterManifest.Create(
                context.TestDeploymentDir,
                Directory.GetCurrentDirectory(),
                typeof(PerformanceCounterTest).Name,
                PerformanceCounterTest.performanceProviderPath
                );

            try
            {
                // if a previous test run aborted the manifest may still be loaded
                TestCounterManifest.Uninstall(PerformanceCounterTest.manifestPath);
            }
            catch
            {
                // ignore uninstall errors, logged by Uninstall
            }

            try
            {
                TestCounterManifest.Install(PerformanceCounterTest.manifestPath);
            }
            catch
            {
                // set it to null so we don't try to clean it up later
                PerformanceCounterTest.manifestPath = null;
                throw;
            }
        }

        [ClassCleanup]
        public static void Cleanup()
        {
            if (PerformanceCounterTest.manifestPath != null)
            {
                TestCounterManifest.Uninstall(PerformanceCounterTest.manifestPath);
            }
        }

        [TestMethod]
        [Owner("mattrow;shkadam")]
        public void TestPerformanceCounterSingleProviderSingleCategory()
        {
            using (var provider = CreateProvider("1", 50000, 60000, 70000, 80000))
            {
                provider.Start();

                VerifyCounterValues(PerformanceCounterTest.Category1, provider);

                provider.Stop();
            }
        }

        [TestMethod]
        [Owner("mattrow;shkadam")]
        public void TestPerformanceCounterMultipleProviderSingleCategoryConcurrent()
        {
            using (var provider1 = CreateProvider("1", 50000, 60000, 70000, 80000))
            using (var provider2 = CreateProvider("1", 50005, 60010, 70200, 83000))
            {
                provider1.Start();
                provider2.Start();

                VerifyCounterValues(PerformanceCounterTest.Category1, provider1);
                VerifyCounterValues(PerformanceCounterTest.Category1, provider2);

                provider1.Stop();
                provider2.Stop();
            }
        }

        [TestMethod]
        [Owner("mattrow;shkadam")]
        public void TestPerformanceCounterMultipleProviderSingleCategorySequential()
        {
            using (var provider1 = CreateProvider("1", 50001, 60002, 70001, 80004))
            using (var provider2 = CreateProvider("1", 50045, 60013, 70202, 83001))
            {
                provider1.Start();

                VerifyCounterValues(PerformanceCounterTest.Category1, provider1);

                provider1.Stop();

                provider2.Start();

                VerifyCounterValues(PerformanceCounterTest.Category1, provider2);

                provider2.Stop();
            }
        }

        [TestMethod]
        [Owner("mattrow;shkadam")]
        public void TestPerformanceCounterMultipleProviderMultipleCategoryConcurrent()
        {
            using (var provider1 = CreateProvider("1", 50000, 60000, 70000, 80000))
            using (var provider2 = CreateProvider("2", 50005, 60010, 70200, 83000))
            {
                provider1.Start();
                provider2.Start();

                VerifyCounterValues(PerformanceCounterTest.Category1, provider1);
                VerifyCounterValues(PerformanceCounterTest.Category2, provider2);

                provider1.Stop();
                provider2.Stop();
            }
        }

        [TestMethod]
        [Owner("mattrow;shkadam")]
        public void TestPerformanceCounterMultipleProviderMultipleCategorySequential()
        {
            using (var provider1 = CreateProvider("1", 50001, 60002, 70001, 80004))
            using (var provider2 = CreateProvider("2", 50045, 60013, 70202, 83001))
            {
                provider1.Start();

                VerifyCounterValues(PerformanceCounterTest.Category1, provider1);

                provider1.Stop();

                provider2.Start();

                VerifyCounterValues(PerformanceCounterTest.Category2, provider2);

                provider2.Stop();
            }
        }

        [TestMethod]
        [Owner("mattrow;shkadam")]
        public void TestPerformanceCounterMultipleProviderMultipleCategoryInterleaved()
        {
            using (var provider11 = CreateProvider("1", 50001, 60002, 70001, 80004))
            using (var provider12 = CreateProvider("1", 50401, 60402, 70401, 80404))
            using (var provider21 = CreateProvider("2", 50045, 60013, 70202, 83001))
            using (var provider22 = CreateProvider("2", 50845, 60813, 78202, 83801))
            {
                provider11.Start();

                VerifyCounterValues(PerformanceCounterTest.Category1, provider11);

                provider12.Start();

                VerifyCounterValues(PerformanceCounterTest.Category1, provider11);
                VerifyCounterValues(PerformanceCounterTest.Category1, provider12);

                provider21.Start();

                VerifyCounterValues(PerformanceCounterTest.Category1, provider11);
                VerifyCounterValues(PerformanceCounterTest.Category1, provider12);
                VerifyCounterValues(PerformanceCounterTest.Category2, provider21);

                provider12.Stop();

                VerifyCounterValues(PerformanceCounterTest.Category1, provider11);
                VerifyCounterValues(PerformanceCounterTest.Category2, provider21);


                provider22.Start();

                VerifyCounterValues(PerformanceCounterTest.Category1, provider11);
                VerifyCounterValues(PerformanceCounterTest.Category2, provider21);
                VerifyCounterValues(PerformanceCounterTest.Category2, provider22);

                provider21.Stop();

                VerifyCounterValues(PerformanceCounterTest.Category1, provider11);
                VerifyCounterValues(PerformanceCounterTest.Category2, provider22);

                provider11.Stop();

                VerifyCounterValues(PerformanceCounterTest.Category2, provider22);

                provider22.Stop();
            }
        }

        private static PerformanceProvider CreateProvider(string category, long t1, long t2, long t3, long t4)
        {
            var instanceId = Guid.NewGuid().ToString();

            return new PerformanceProvider(PerformanceCounterTest.performanceProviderPath)
            {
                Category = category,
                InstanceId = instanceId,

                TargetValue1 = t1,
                TargetValue2 = t2,
                TargetValue3 = t3,
                TargetValue4 = t4
            };
        }

        private static void VerifyCounterValues(string categoryName, PerformanceProvider provider)
        {
            Assert.IsTrue(PerformanceCounterTest.WaitForInstance(categoryName, provider.InstanceId));

            Assert.IsTrue(PerformanceCounterTest.WaitForCounterValue(categoryName, PerformanceCounterTest.Counter1, provider.InstanceId, provider.TargetValue1));
            Assert.IsTrue(PerformanceCounterTest.WaitForCounterValue(categoryName, PerformanceCounterTest.Counter2, provider.InstanceId, provider.TargetValue2));
            Assert.IsTrue(PerformanceCounterTest.WaitForCounterValue(categoryName, PerformanceCounterTest.Counter3, provider.InstanceId, provider.TargetValue3));
            Assert.IsTrue(PerformanceCounterTest.WaitForCounterValue(categoryName, PerformanceCounterTest.Counter4, provider.InstanceId, provider.TargetValue4));
        }

        private static bool WaitForCounterValue(string categoryName, string counterName,  string instanceId, long targetValue)
        {
            LogHelper.Log("Waiting for counter value {0}\\{1}\\{2} to reach {3}", categoryName, counterName, instanceId, targetValue);

            var counter = new PerformanceCounter(categoryName, counterName, instanceId, true);
            int count = 0;
            while (count < 10)
            {
                if (counter.RawValue == targetValue)
                {
                    LogHelper.Log("Counter value {0}\\{1}\\{2} has reached {3}", categoryName, counterName, instanceId, targetValue);
                    return true;
                }

                Thread.Sleep(200);
            }

            LogHelper.Log("Counter value {0}\\{1}\\{2} has not reached {3} is {4}", categoryName, counterName, instanceId, targetValue, counter.RawValue);
            return false;
        }

        private static bool WaitForInstance(string categoryName, string instanceId)
        {
            LogHelper.Log("Waiting for counter set instance {0}", instanceId);

            var category = new PerformanceCounterCategory(categoryName);

            int count = 0;
            while (count < 20)
            {
                count++;

                try
                {
                    if (category.GetInstanceNames().Where(i => string.Compare(i, instanceId) == 0).Any())
                    {
                        LogHelper.Log("Counter set instance {0} found", instanceId);
                        return true;
                    }
                }
                catch (InvalidOperationException e)
                {
                    LogHelper.Log("Exception {0}. Closing the performance counter resources will cause it to reinitialize.", e);
                    PerformanceCounter.CloseSharedResources();
                }

                Thread.Sleep(500);
            }

            LogHelper.Log("Counter set instance {0} not found", instanceId);
            return false;
        }

        private sealed class PerformanceProvider : IDisposable
        {
            private readonly string providerApplication;

            private Process providerProcess;

            public PerformanceProvider(string providerApplication)
            {
                this.providerApplication = providerApplication;
            }

            public long TargetValue1 { get; set; }

            public long TargetValue2 { get; set; }

            public long TargetValue3 { get; set; }

            public long TargetValue4 { get; set; }

            public string InstanceId { get; set; }

            public string Category { get; set; }

            public void Start()
            {
                this.providerProcess = new Process();

                this.providerProcess.StartInfo.UseShellExecute = false;
                this.providerProcess.StartInfo.RedirectStandardInput = true;

                this.providerProcess.StartInfo.FileName = this.providerApplication;
                this.providerProcess.StartInfo.Arguments = string.Format(
                    "{0} {1} {2} {3} {4} {5}",
                    this.Category,
                    this.InstanceId,
                    this.TargetValue1,
                    this.TargetValue2,
                    this.TargetValue3,
                    this.TargetValue4);

                LogHelper.Log("Starting provider {0} {1}", this.providerProcess.StartInfo.FileName, this.providerProcess.StartInfo.Arguments);

                this.providerProcess.Exited += (s, e) =>
                {
                    LogHelper.Log("Provider process {0} exited", this.providerProcess.Id);
                };

                this.providerProcess.Start();

                LogHelper.Log("Provider started with process id {0}", this.providerProcess.Id);
            }

            public void Stop()
            {
                LogHelper.Log("Stopping provider with process id {0}", this.providerProcess.Id);

                this.providerProcess.StandardInput.WriteLine();

                if (!this.providerProcess.WaitForExit(5000))
                {
                    LogHelper.Log("Stopping provider with process id {0} timed out", this.providerProcess.Id);
                    throw new TimeoutException();
                }
                else
                {
                    LogHelper.Log("Provider process {0} stopped", this.providerProcess.Id);
                }
            }

            public void Dispose()
            {
                if (this.providerProcess != null)
                {
                    IDisposable disposable = this.providerProcess;
                    this.providerProcess = null;

                    disposable.Dispose();
                }
            }
        }

        private class TestCounterManifest
        {
            public const string ManifestName = "WFTestPerf.man";

            public static string Create(string sourceDirectory, string outputDirectory, string testName, string performanceProvider)
            {
                var sourceManifest = Path.Combine(sourceDirectory, TestCounterManifest.ManifestName);

                Assert.IsTrue(File.Exists(sourceManifest), "Manifest file not found: {0}", sourceManifest);

                var path = Path.Combine(
                    outputDirectory,
                    testName,
                    TestCounterManifest.ManifestName);

                var targetDir = Path.GetDirectoryName(path);
                if (Directory.Exists(targetDir))
                {
                    // delete leftovers from previous runs
                    Directory.Delete(targetDir, true);
                }

                Directory.CreateDirectory(targetDir);

                LogHelper.Log("Load manifest from {0}", sourceManifest);
                XmlDocument manifest = new XmlDocument { XmlResolver = null };

                var settings = new XmlReaderSettings
                {
                    XmlResolver = null,
                    DtdProcessing = System.Xml.DtdProcessing.Prohibit
                };
                var reader = XmlTextReader.Create(sourceManifest, settings);                
                manifest.Load(reader);

                var manNs = "http://schemas.microsoft.com/win/2004/08/events";
                var counterNs = "http://schemas.microsoft.com/win/2005/12/counters";

                XmlNamespaceManager mgr = new XmlNamespaceManager(manifest.NameTable);
                mgr.AddNamespace("man", manNs);
                mgr.AddNamespace("ctr", counterNs);

                var provider = manifest.SelectSingleNode("/man:instrumentationManifest/man:instrumentation/ctr:counters/ctr:provider", mgr) as XmlElement;

                var providerApplication = Path.Combine(targetDir, Path.GetFileName(performanceProvider));

                provider.SetAttribute("applicationIdentity", providerApplication);

                LogHelper.Log("Save manifest to {0}", path);
                manifest.Save(path);

                TestCounterManifest.DeployProviderAndResources(performanceProvider, providerApplication);

                Assert.IsTrue(File.Exists(providerApplication), "Check that test performance provider application has been created during build.");

                return path;
            }

            private static void DeployProviderAndResources(string source, string target)
            {
                LogHelper.Log("Copy provider from {0} to {1}", source, target);
                File.Copy(source, target);

                var deployment = Path.GetDirectoryName(source);

                var build = Path.GetDirectoryName(deployment);

                // locate the mui file for the provider
                var mui = Path.Combine(
                    build,
                    "loc\\src\\bin",
                    Path.GetFileName(deployment),
                    Path.GetFileName(target) + ".mui");

                var targetMui = Path.Combine(
                    Path.GetDirectoryName(target),
                    "en-US",
                    Path.GetFileName(target) + ".mui");

                Directory.CreateDirectory(Path.GetDirectoryName(targetMui));

                LogHelper.Log("Copy provider mui from {0} to {1}", mui, targetMui);
                File.Copy(mui, targetMui);
            }

            public static void Install(string path)
            {
                using (var process = new Process())
                {
                    process.StartInfo.FileName = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "lodctr.exe");
                    process.StartInfo.Arguments = string.Format("/m:\"{0}\"", path);

                    LogHelper.Log("Call {0} {1}", process.StartInfo.FileName, process.StartInfo.Arguments);

                    process.Start();

                    process.WaitForExit();

                    if (process.ExitCode != 0)
                    {
                        LogHelper.Log("lodctr failed with {0}", process.ExitCode);
                        throw new Exception("lodctr failed");
                    }
                    else
                    {
                        LogHelper.Log("lodctr succeeded");
                    }
                }
            }

            public static void Uninstall(string path)
            {
                using (var process = new Process())
                {
                    process.StartInfo.FileName = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "unlodctr.exe");
                    process.StartInfo.Arguments = string.Format("/m:\"{0}\"", path);

                    LogHelper.Log("Call {0} {1}", process.StartInfo.FileName, process.StartInfo.Arguments);

                    process.Start();

                    process.WaitForExit();

                    if (process.ExitCode != 0)
                    {
                        LogHelper.Log("unlodctr failed with {0}", process.ExitCode);
                        throw new Exception("unlodctr failed");
                    }
                    else
                    {
                        LogHelper.Log("unlodctr succeeded");
                    }
                }
            }
        }

        private class LogHelper
        {
            public static void Log(string text, params object[] p)
            {
                string message = string.Format(text, p);
                DateTime time = DateTime.Now;
                WEX.Logging.Interop.Log.Comment(string.Format("({0,2:00.##}:{1,2:00.##}:{2,2:00.##}.{3,3:000.##}): {4}", time.Hour, time.Minute, time.Second, time.Millisecond, message));
            }
        }
    }
}