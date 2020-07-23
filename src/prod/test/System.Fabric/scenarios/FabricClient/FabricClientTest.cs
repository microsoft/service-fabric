// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Scenarios.Test.FabricClient
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Fabric.Test;
    using System.Fabric.Test.Common.FabricUtility;
    using System.Fabric.Test.Description;
    using System.Globalization;
    using System.Linq;
    using System.Numerics;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using VS = Microsoft.VisualStudio.TestTools.UnitTesting;

    [VS.TestClass]
    public class FabricClientTest
    {
        // Use the same timeout as system.fabric default
        private static TimeSpan taskTimeout = TimeSpan.FromMinutes(1);
        private const string TestCaseName = "FabricClientUnitTest";

        private static FabricDeployment deployment;

        private static string GeneateNodeName(NodeId nodeId)
        {
            return "nodeid:" + nodeId.ToString();
        }

        [VS.ClassInitialize]
        public static void ClassInitialize(VS.TestContext context)
        {
            LogHelper.Log("Creating deployment");
            var parameters = FabricDeploymentParameters.CreateDefault(Constants.TestDllName, FabricClientTest.TestCaseName, 1);
            //parameters.SystemFabricConfiguration = SystemFabricConfigurationParameters.CreateWithPartialTrustEnabled();

            FabricClientTest.deployment = new FabricDeployment(parameters);

            LogHelper.Log("Starting nodes");                
            FabricClientTest.deployment.Start();
        }

        [VS.ClassCleanup]
        public static void ClassCleanup()
        {
            FabricClientTest.deployment.Dispose();

            // To validate no tasks are left behind with unhandled exceptions
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        private static void ExpectExceptionTestHelper<TException, A0>(Func<A0, Task> func, string tag, params Tuple<A0>[] args) where TException : Exception
        {
            foreach (var item in args)
            {
                TestUtility.ExpectException<TException>(() => func(item.Item1), tag);
            }
        }

        private static void ExpectExceptionTestHelper<TException, A0, A1>(Func<A0, A1, Task> func, string tag, params Tuple<A0, A1>[] args) where TException : Exception
        {
            foreach (var item in args)
            {
                TestUtility.ExpectException<TException>(() => func(item.Item1, item.Item2), tag);
            }
        }

        private static void ExpectExceptionTestHelperAction<TException, A0, A1>(Action<A0, A1> action, string tag, params Tuple<A0, A1>[] args) where TException : Exception
        {
            foreach (var item in args)
            {
                TestUtility.ExpectException<TException>(() => action(item.Item1, item.Item2), tag);
            }
        }

        private static void ExpectExceptionTestHelper<TException, A0, A1, A2>(Func<A0, A1, A2, Task> func, string tag, params Tuple<A0, A1, A2>[] args) where TException : Exception
        {
            foreach (var item in args)
            {
                TestUtility.ExpectException<TException>(() => func(item.Item1, item.Item2, item.Item3), tag);
            }
        }

        private static void ExpectExceptionTestHelper<TException, A0, A1, A2, A3>(Func<A0, A1, A2, A3, Task> func, string tag, params Tuple<A0, A1, A2, A3>[] args) where TException : Exception
        {
            foreach (var item in args)
            {
                TestUtility.ExpectException<TException>(() => func(item.Item1, item.Item2, item.Item3, item.Item4), tag);
            }
        }

        private static void ExpectExceptionTestHelper<TException, A0, A1, A2, A3, A4>(Func<A0, A1, A2, A3, A4, Task> func, string tag, params Tuple<A0, A1, A2, A3, A4>[] args) where TException : Exception
        {
            foreach (var item in args)
            {
                TestUtility.ExpectException<TException>(() => func(item.Item1, item.Item2, item.Item3, item.Item4, item.Item5), tag);
            }
        }

        private static void PutPropertyExceptionTestHelper<TValue>(Func<Uri, string, TValue, Task> func, TValue value, string tag = "") 
        {
            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, Uri, string>((uri, name) => func(uri, name, value), tag,
                Tuple.Create<Uri, string>(null, "a"),
                Tuple.Create<Uri, string>(new Uri("fabric:/a"), null));
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("aprameyr")]
        public void FabricClient_ParameterValidation()
        {
            Uri validUri = new Uri("fabric:/a/b");
            string validTypeName = "my_type";
            string validVersion = "1.1";

            FabricClient fc = FabricClientTest.CreateClient();

            var localClient = new FabricClient();

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, ApplicationDescription>(fc.ApplicationManager.CreateApplicationAsync,
                "AppManager.CreateApplication",
                Tuple.Create<ApplicationDescription>(null));

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, DeleteApplicationDescription>(fc.ApplicationManager.DeleteApplicationAsync,
                "AppManager.DeleteApplication",
                Tuple.Create<DeleteApplicationDescription>(null));

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, string>(fc.ApplicationManager.ProvisionApplicationAsync,
                "AppManager.ProvisionApplication",
                Tuple.Create<string>(null));

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, string, string>(fc.ApplicationManager.UnprovisionApplicationAsync,
                "AppManager.UnProvisionApplication",
                Tuple.Create<string, string>(null, validVersion),
                Tuple.Create<string, string>(validTypeName, null));

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, ApplicationUpgradeDescription>(fc.ApplicationManager.UpgradeApplicationAsync,
                "AppManager.Upgrade",
                Tuple.Create<ApplicationUpgradeDescription>(null));

            TestUtility.ExpectException<ArgumentNullException>(() => fc.ServiceManager.CreateServiceAsync(null), "CreateService");

            var sd1 = new StatelessServiceDescription()
            {
                ApplicationName = new Uri("fabric:/app"),
                Correlations = { null },
                InstanceCount = 1,
                PartitionSchemeDescription = new SingletonPartitionSchemeDescription(),
                ServiceName = new Uri("fabrc:/svc"),
                ServiceTypeName = "x"
            };

            TestUtility.ExpectException<ArgumentNullException>(() => fc.ServiceManager.CreateServiceAsync(sd1), "CreateService");

            var sd3 = new StatelessServiceDescription()
            {
                ApplicationName = new Uri("fabric:/app"),
                InstanceCount = 1,
                PartitionSchemeDescription = new NamedPartitionSchemeDescription(),
                ServiceName = new Uri("fabrc:/svc"),
                ServiceTypeName = "x"
            };

            ((NamedPartitionSchemeDescription)sd3.PartitionSchemeDescription).PartitionNames.Add("My partition");
            // Duplicate partition names are not allowed
            TestUtility.ExpectException<ArgumentException>(
                () => ((NamedPartitionSchemeDescription)sd3.PartitionSchemeDescription).PartitionNames.Add("My partition"), "PartitionNames.Add duplicate");

            TestUtility.ExpectException<ArgumentNullException>(() => fc.ServiceGroupManager.CreateServiceGroupAsync(null), "CreateServiceGroup");

            TestUtility.ExpectException<ArgumentNullException>(() => fc.ServiceManager.DeleteServiceAsync((DeleteServiceDescription)null), "DeleteService");

            TestUtility.ExpectException<ArgumentNullException>(() => fc.ServiceGroupManager.DeleteServiceGroupAsync(null), "DeleteServiceGroup");

            TestUtility.ExpectException<ArgumentNullException>(() => fc.ServiceGroupManager.GetServiceGroupDescriptionAsync(null), "GetServiceGroupDescription");

            TestUtility.ExpectException<ArgumentNullException>(() => fc.ServiceManager.ResolveServicePartitionAsync(null), "ResolveService");

            TestUtility.ExpectException<ArgumentNullException>(() => fc.ServiceManager.ResolveServicePartitionAsync(null, (long)1), "ResolveService");

            TestUtility.ExpectException<ArgumentNullException>(() => fc.ServiceManager.ResolveServicePartitionAsync(null, (string)"foo"), "ResolveService");

            ServicePartitionResolutionChangeHandler cb = (c, handlerId, args) => { ; };

#pragma warning disable 0618 // obsolete
            FabricClientTest.ExpectExceptionTestHelperAction<ArgumentNullException, Uri, ServicePartitionResolutionChangeHandler>((a, b) => fc.ServiceManager.RegisterServicePartitionResolutionChangeHandler(a, null, b),
               "ServiceManager.RegisterServicePartitionResolutionChangeHandler",
               Tuple.Create<Uri, ServicePartitionResolutionChangeHandler>(null, cb),
               Tuple.Create<Uri, ServicePartitionResolutionChangeHandler>(validUri, null));
#pragma warning restore 0618

            TestUtility.ExpectException<ArgumentNullException>(() => fc.PropertyManager.CreateNameAsync(null), "CreateName");

            TestUtility.ExpectException<ArgumentNullException>(() => fc.PropertyManager.DeleteNameAsync(null), "DeleteName");

            TestUtility.ExpectException<ArgumentNullException>(() => fc.PropertyManager.EnumerateSubNamesAsync(null, null, false), "EnumerateSubNamesAsync");

            TestUtility.ExpectException<ArgumentNullException>(() => fc.PropertyManager.NameExistsAsync(null), "NameExists");

            FabricClientTest.PutPropertyExceptionTestHelper(fc.PropertyManager.PutPropertyAsync, (double)123.2, "PutProperty double");
            FabricClientTest.PutPropertyExceptionTestHelper(fc.PropertyManager.PutPropertyAsync, (long)123, "PutProperty long");
            FabricClientTest.PutPropertyExceptionTestHelper(fc.PropertyManager.PutPropertyAsync, "string", "PutProperty string");
            FabricClientTest.PutPropertyExceptionTestHelper(fc.PropertyManager.PutPropertyAsync, Guid.NewGuid(), "PutProperty guid");
            FabricClientTest.PutPropertyExceptionTestHelper(fc.PropertyManager.PutPropertyAsync, new byte[] { 0} , "PutProperty byte[]");


            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, Uri, string>(fc.PropertyManager.DeletePropertyAsync, "DeleteProperty",
                Tuple.Create<Uri, string>(null, "a"),
                Tuple.Create<Uri, string>(validUri, null));

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, Uri, string>(fc.PropertyManager.GetPropertyAsync, "GetProperty",
                Tuple.Create<Uri, string>(null, "a"),
                Tuple.Create<Uri, string>(validUri, null));

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, Uri, string>(fc.PropertyManager.GetPropertyMetadataAsync, "GetPropertyMetadata",
                Tuple.Create<Uri, string>(null, "a"),
                Tuple.Create<Uri, string>(validUri, null));
                        
            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, Uri, ICollection<PropertyBatchOperation>>(fc.PropertyManager.SubmitPropertyBatchAsync, "SubmitPropertyBatchAsync",
                Tuple.Create<Uri, ICollection<PropertyBatchOperation>>(null, new List<PropertyBatchOperation> { new GetPropertyOperation("propertyName")} ),
                Tuple.Create<Uri, ICollection<PropertyBatchOperation>>(validUri, null));

            FabricClientTest.ExpectExceptionTestHelper<ArgumentException, Uri, ICollection<PropertyBatchOperation>>(fc.PropertyManager.SubmitPropertyBatchAsync, "SubmitPropertyBatchAsync",
                Tuple.Create<Uri, ICollection<PropertyBatchOperation>>(validUri, new List<PropertyBatchOperation>() { null }));

            FabricClientTest.ExpectExceptionTestHelper<ArgumentException, Uri, ICollection<PropertyBatchOperation>>(fc.PropertyManager.SubmitPropertyBatchAsync, "SubmitPropertyBatchAsync",
                Tuple.Create<Uri, ICollection<PropertyBatchOperation>>(validUri, new List<PropertyBatchOperation>()));

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, Uri, bool, PropertyEnumerationResult>(fc.PropertyManager.EnumeratePropertiesAsync, "EnumProperties",
                Tuple.Create<Uri, bool, PropertyEnumerationResult>(null, true, null));

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, string>(fc.ClusterManager.RemoveNodeStateAsync, "RemoveNode",
                Tuple.Create<string>(null));

            TestUtility.ExpectException<ArgumentNullException>(() => fc.HealthManager.ReportHealth(null), "ReportHealth");            
        }

        // Run this test manually to verify that cancellation is not leaking
        // [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("aprameyr")]
        public void FabricClient_MemoryLeak()
        {
            CancellationTokenSource cts = new CancellationTokenSource();

            TimeSpan defaultTS = TimeSpan.FromSeconds(1);

            CancellationTokenSource threadCancel = new CancellationTokenSource();
            var client = FabricClientTest.CreateClient();

            List<Task> tasks = new List<Task>();
            for (int i = 0; i < 20; i++)
            {
                tasks.Add(Task.Factory.StartNew(
                    () =>
                    {
                        while (!threadCancel.Token.IsCancellationRequested)
                        {
                            client.HealthManager.GetClusterHealthAsync(
                                ClusterHealthPolicy.Default,
                                defaultTS,
                                cts.Token).Wait();
                        }
                    }));
            }

            while (!Debugger.IsAttached)
            {
                Thread.Sleep(100);
            }

            threadCancel.Cancel();

            Task.WaitAll(tasks.ToArray());
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("alexwun")]
        public void FabricClient_UriPercentEncoding()
        {
            FabricClient fc = FabricClientTest.CreateClient();

            foreach (var toEscape in new string[] { "[", "]", "<", ">", "|", "\'" })
            {
                var uri = new Uri(string.Format("fabric:/UriPercentEncodingTest{0}", Uri.EscapeDataString(toEscape)));
                LogHelper.Log(string.Format("CreateName({0}, {1})", uri.ToString(), uri.OriginalString));

                FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fc.PropertyManager.CreateNameAsync(uri);
                }, "CreateName");

                var result = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fc.PropertyManager.NameExistsAsync(uri);
                }, "NameExists");

                VS.Assert.IsTrue(result, string.Format("NameExists({0})", result));
            }
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("alexwun")]
        public void FabricClient_TaskCancel()
        {
            FabricClient fc = FabricClientTest.CreateClient();

            var cts = new CancellationTokenSource();
            var task = fc.PropertyManager.CreateNameAsync(new Uri("fabric:/TaskCancel"), TimeSpan.MaxValue, cts.Token);

            // Should execute in MTA
            cts.Cancel();
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("alexwun")]
        public void FabricClient_ApplicationUpgradeDescriptionValidation()
        {
            Uri validUri = new Uri("fabric:/a/b");
            string validVersion = "v1";

            FabricClient fc = FabricClientTest.CreateClient();

            // Undefined application name
            var appUpgradeDescription = new ApplicationUpgradeDescription();

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, ApplicationUpgradeDescription>(fc.ApplicationManager.UpgradeApplicationAsync,
                "AppManager.Upgrade",
                Tuple.Create<ApplicationUpgradeDescription>(appUpgradeDescription));

            // Undefined application type
            appUpgradeDescription.ApplicationName = validUri;

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, ApplicationUpgradeDescription>(fc.ApplicationManager.UpgradeApplicationAsync,
                "AppManager.Upgrade",
                Tuple.Create<ApplicationUpgradeDescription>(appUpgradeDescription));

            // Undefined upgrade policy
            appUpgradeDescription.TargetApplicationTypeVersion = validVersion;

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, ApplicationUpgradeDescription>(fc.ApplicationManager.UpgradeApplicationAsync,
                "AppManager.Upgrade",
                Tuple.Create<ApplicationUpgradeDescription>(appUpgradeDescription));

            // Undefined upgrade mode
            appUpgradeDescription.UpgradePolicyDescription = new RollingUpgradePolicyDescription();

            FabricClientTest.ExpectExceptionTestHelper<ArgumentException, ApplicationUpgradeDescription>(fc.ApplicationManager.UpgradeApplicationAsync,
                "AppManager.Upgrade",
                Tuple.Create<ApplicationUpgradeDescription>(appUpgradeDescription));
            
            // Undefined monitoring policy
            appUpgradeDescription.UpgradePolicyDescription = new RollingUpgradePolicyDescription()
            {
                UpgradeMode = RollingUpgradeMode.Monitored,
            };

            FabricClientTest.ExpectExceptionTestHelper<ArgumentException, ApplicationUpgradeDescription>(fc.ApplicationManager.UpgradeApplicationAsync,
                "AppManager.Upgrade",
                Tuple.Create<ApplicationUpgradeDescription>(appUpgradeDescription));
            
            // Undefined monitoring policy 2
            appUpgradeDescription.UpgradePolicyDescription = new MonitoredRollingApplicationUpgradePolicyDescription();

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, ApplicationUpgradeDescription>(fc.ApplicationManager.UpgradeApplicationAsync,
                "AppManager.Upgrade",
                Tuple.Create<ApplicationUpgradeDescription>(appUpgradeDescription));
            
            // Undefined failure action
            ((MonitoredRollingUpgradePolicyDescription)appUpgradeDescription.UpgradePolicyDescription).MonitoringPolicy = new RollingUpgradeMonitoringPolicy()
            {
                FailureAction = UpgradeFailureAction.Invalid,
            };

            FabricClientTest.ExpectExceptionTestHelper<ArgumentException, ApplicationUpgradeDescription>(fc.ApplicationManager.UpgradeApplicationAsync,
                "AppManager.Upgrade",
                Tuple.Create<ApplicationUpgradeDescription>(appUpgradeDescription));
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("alexwun")]
        public void FabricClient_FabricUpgradeDescriptionValidation()
        {
            FabricClient fc = FabricClientTest.CreateClient();

            // Undefined upgrade policy
            var fabricUpgradeDescription = new FabricUpgradeDescription();

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, FabricUpgradeDescription>(fc.ClusterManager.UpgradeFabricAsync,
                "ClusterManager.Upgrade",
                Tuple.Create<FabricUpgradeDescription>(fabricUpgradeDescription));

            // Undefined upgrade mode
            fabricUpgradeDescription.UpgradePolicyDescription = new RollingUpgradePolicyDescription();

            FabricClientTest.ExpectExceptionTestHelper<ArgumentException, FabricUpgradeDescription>(fc.ClusterManager.UpgradeFabricAsync,
                "ClusterManager.Upgrade",
                Tuple.Create<FabricUpgradeDescription>(fabricUpgradeDescription));
            
            // Undefined monitoring policy
            fabricUpgradeDescription.UpgradePolicyDescription = new RollingUpgradePolicyDescription()
            {
                UpgradeMode = RollingUpgradeMode.Monitored,
            };

            FabricClientTest.ExpectExceptionTestHelper<ArgumentException, FabricUpgradeDescription>(fc.ClusterManager.UpgradeFabricAsync,
                "ClusterManager.Upgrade",
                Tuple.Create<FabricUpgradeDescription>(fabricUpgradeDescription));
            
            // Undefined monitoring policy 2
            fabricUpgradeDescription.UpgradePolicyDescription = new MonitoredRollingFabricUpgradePolicyDescription();

            FabricClientTest.ExpectExceptionTestHelper<ArgumentNullException, FabricUpgradeDescription>(fc.ClusterManager.UpgradeFabricAsync,
                "ClusterManager.Upgrade",
                Tuple.Create<FabricUpgradeDescription>(fabricUpgradeDescription));
            
            // Undefined failure action
            ((MonitoredRollingUpgradePolicyDescription)fabricUpgradeDescription.UpgradePolicyDescription).MonitoringPolicy = new RollingUpgradeMonitoringPolicy()
            {
                FailureAction = UpgradeFailureAction.Invalid,
            };

            FabricClientTest.ExpectExceptionTestHelper<ArgumentException, FabricUpgradeDescription>(fc.ClusterManager.UpgradeFabricAsync,
                "ClusterManager.Upgrade",
                Tuple.Create<FabricUpgradeDescription>(fabricUpgradeDescription));
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("motanv")]
        public void FabricClient_RollingUpgradeMonitoringPolicyDefaultValue()
        {
            var policy = new RollingUpgradeMonitoringPolicy();
            VS.Assert.AreEqual(UpgradeFailureAction.Manual, policy.FailureAction); 
            VS.Assert.AreEqual(0, policy.HealthCheckWaitDuration.TotalSeconds);
            VS.Assert.AreEqual(120, policy.HealthCheckStableDuration.TotalSeconds);
            VS.Assert.AreEqual(600, policy.HealthCheckRetryTimeout.TotalSeconds);
            VS.Assert.AreEqual(TimeSpan.MaxValue, policy.UpgradeTimeout);
            VS.Assert.AreEqual(TimeSpan.MaxValue, policy.UpgradeDomainTimeout);
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("alexwun")]
        public void FabricClient_ApplicationHealthPolicyToNative()
        {
            FabricClient fc = FabricClientTest.CreateClient();

            // with service type policies
            {
                var managedPolicy = new ApplicationHealthPolicy();
                managedPolicy.ConsiderWarningAsError = true;
                managedPolicy.MaxPercentUnhealthyDeployedApplications = 3;
                managedPolicy.DefaultServiceTypeHealthPolicy = new ServiceTypeHealthPolicy()
                {
                    MaxPercentUnhealthyServices = 5,
                    MaxPercentUnhealthyPartitionsPerService = 7,
                    MaxPercentUnhealthyReplicasPerPartition = 9,
                };
                managedPolicy.ServiceTypeHealthPolicyMap.Add("ServiceTypeA", new ServiceTypeHealthPolicy()
                {
                    MaxPercentUnhealthyServices = 11,
                    MaxPercentUnhealthyPartitionsPerService = 13,
                    MaxPercentUnhealthyReplicasPerPartition = 15,
                });
                managedPolicy.ServiceTypeHealthPolicyMap.Add("ServiceTypeB", new ServiceTypeHealthPolicy()
                {
                    MaxPercentUnhealthyServices = 17,
                    MaxPercentUnhealthyPartitionsPerService = 19,
                    MaxPercentUnhealthyReplicasPerPartition = 21,
                });

                var appUpgradeDescription = new ApplicationUpgradeDescription()
                {
                    ApplicationName = new Uri("fabric:/AppNotFound"),
                    TargetApplicationTypeVersion = "v1",
                };
                appUpgradeDescription.UpgradePolicyDescription = new MonitoredRollingApplicationUpgradePolicyDescription()
                {
                    MonitoringPolicy = new RollingUpgradeMonitoringPolicy()
                    {
                        FailureAction = UpgradeFailureAction.Manual,
                    },
                    HealthPolicy = managedPolicy,
                };

                IntPtr nativePolicy = IntPtr.Zero;
                ApplicationHealthPolicy rebuiltPolicy = null;
                using (var pin = new PinCollection())
                {
                    nativePolicy = managedPolicy.ToNative(pin);
                    rebuiltPolicy = ApplicationHealthPolicy.FromNative(nativePolicy);
                }

                VS.Assert.IsTrue(
                    ApplicationHealthPolicy.AreEqual(managedPolicy, rebuiltPolicy),
                    "ApplicationHealthPolicy mismatch");

                FabricClientTest.ExpectExceptionTestHelper<FabricElementNotFoundException, ApplicationUpgradeDescription>(fc.ApplicationManager.UpgradeApplicationAsync,
                    "AppManager.Upgrade",
                    Tuple.Create<ApplicationUpgradeDescription>(appUpgradeDescription));
            }

            // without service type policies
            {
                var managedPolicy = new ApplicationHealthPolicy();
                managedPolicy.ConsiderWarningAsError = true;
                managedPolicy.MaxPercentUnhealthyDeployedApplications = 37;
                managedPolicy.DefaultServiceTypeHealthPolicy = null;

                var appUpgradeDescription = new ApplicationUpgradeDescription()
                {
                    ApplicationName = new Uri("fabric:/AppNotFound"),
                    TargetApplicationTypeVersion = "v1",
                };
                appUpgradeDescription.UpgradePolicyDescription = new MonitoredRollingApplicationUpgradePolicyDescription()
                {
                    MonitoringPolicy = new RollingUpgradeMonitoringPolicy()
                    {
                        FailureAction = UpgradeFailureAction.Manual,
                    },
                    HealthPolicy = managedPolicy,
                };

                IntPtr nativePolicy = IntPtr.Zero;
                ApplicationHealthPolicy rebuiltPolicy = null;
                using (var pin = new PinCollection())
                {
                    nativePolicy = managedPolicy.ToNative(pin);
                    rebuiltPolicy = ApplicationHealthPolicy.FromNative(nativePolicy);
                }

                VS.Assert.IsTrue(
                    ApplicationHealthPolicy.AreEqual(managedPolicy, rebuiltPolicy),
                    "ApplicationHealthPolicy mismatch");

                FabricClientTest.ExpectExceptionTestHelper<FabricElementNotFoundException, ApplicationUpgradeDescription>(fc.ApplicationManager.UpgradeApplicationAsync,
                    "AppManager.Upgrade",
                    Tuple.Create<ApplicationUpgradeDescription>(appUpgradeDescription));
            }
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("bikang")]
        public void FabricClient_ApplicationHealthPolicyTest()
        {
            // Service type health policy tests
            ServiceTypeHealthPolicy sNull = null; 
            ServiceTypeHealthPolicy s1 = new ServiceTypeHealthPolicy()
            {
                MaxPercentUnhealthyPartitionsPerService = 10,
                MaxPercentUnhealthyReplicasPerPartition = 23,
                MaxPercentUnhealthyServices = 5,
            };
            ServiceTypeHealthPolicy s2 = new ServiceTypeHealthPolicy();

            VS.Assert.IsFalse(ServiceTypeHealthPolicy.AreEqual(sNull, s1));
            VS.Assert.IsFalse(ServiceTypeHealthPolicy.AreEqual(sNull, s1));
            VS.Assert.IsFalse(ServiceTypeHealthPolicy.AreEqual(s1, s2));

            s2.MaxPercentUnhealthyServices = 5;
            VS.Assert.IsFalse(ServiceTypeHealthPolicy.AreEqual(s1, s2));

            s2.MaxPercentUnhealthyReplicasPerPartition = 23;
            VS.Assert.IsFalse(ServiceTypeHealthPolicy.AreEqual(s1, s2));

            s2.MaxPercentUnhealthyPartitionsPerService = 10;
            VS.Assert.IsTrue(ServiceTypeHealthPolicy.AreEqual(s1, s2));

            // Application health policy tests
            ApplicationHealthPolicy pNull = null;
            ApplicationHealthPolicy p1 = new ApplicationHealthPolicy()
            {
                ConsiderWarningAsError = true,
                MaxPercentUnhealthyDeployedApplications = 10,
            };
            ApplicationHealthPolicy p2 = new ApplicationHealthPolicy();

            VS.Assert.IsFalse(ApplicationHealthPolicy.AreEqual(p1, pNull));
            VS.Assert.IsFalse(ApplicationHealthPolicy.AreEqual(p1, p2));

            p2.ConsiderWarningAsError = true;
            VS.Assert.IsFalse(ApplicationHealthPolicy.AreEqual(p1, p2));

            p2.MaxPercentUnhealthyDeployedApplications = 10;
            VS.Assert.IsTrue(ApplicationHealthPolicy.AreEqual(p1, p2));

            p1.DefaultServiceTypeHealthPolicy = s1;
            VS.Assert.IsFalse(ApplicationHealthPolicy.AreEqual(p1, p2));

            p2.DefaultServiceTypeHealthPolicy = s2;
            VS.Assert.IsTrue(ApplicationHealthPolicy.AreEqual(p1, p2));

            p1.ServiceTypeHealthPolicyMap.Add("servicetype1", s1);
            VS.Assert.IsFalse(ApplicationHealthPolicy.AreEqual(p1, p2));

            p2.ServiceTypeHealthPolicyMap.Add("servicetype1", s2);
            VS.Assert.IsTrue(ApplicationHealthPolicy.AreEqual(p1, p2));

            LogHelper.Log("Clear service type policy map");
            p1.ServiceTypeHealthPolicyMap.Clear();
            p2.ServiceTypeHealthPolicyMap.Clear();
            VS.Assert.IsTrue(ApplicationHealthPolicy.AreEqual(p1, p2));
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("bikang")]
        public void FabricClient_ClusterUpgradeHealthPolicyToNative()
        {
            var managedPolicy = new ClusterUpgradeHealthPolicy();

            // Check default values
            VS.Assert.AreEqual(10, managedPolicy.MaxPercentDeltaUnhealthyNodes);
            VS.Assert.AreEqual(15, managedPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes);

            // Change values and pass it to native and back
            managedPolicy.MaxPercentDeltaUnhealthyNodes = 37;
            managedPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes = 28;

            IntPtr nativePolicy = IntPtr.Zero;
            ClusterUpgradeHealthPolicy rebuiltPolicy = null;
            using (var pin = new PinCollection())
            {
                nativePolicy = managedPolicy.ToNative(pin);
                rebuiltPolicy = ClusterUpgradeHealthPolicy.FromNative(nativePolicy);
            }

            VS.Assert.IsTrue(
                ClusterUpgradeHealthPolicy.AreEqual(managedPolicy, rebuiltPolicy),
                "ClusterUpgradeHealthPolicy mismatch");
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("bikang")]
        public void FabricClient_ClusterUpgradeHealthPolicyTest()
        {
            ClusterUpgradeHealthPolicy cNull = null;
            ClusterUpgradeHealthPolicy c1 = new ClusterUpgradeHealthPolicy()
            {
                MaxPercentDeltaUnhealthyNodes = 14,
                MaxPercentUpgradeDomainDeltaUnhealthyNodes = 23,
            };

            ClusterUpgradeHealthPolicy c2 = new ClusterUpgradeHealthPolicy();

            VS.Assert.IsFalse(ClusterUpgradeHealthPolicy.AreEqual(cNull, c1));
            VS.Assert.IsFalse(ClusterUpgradeHealthPolicy.AreEqual(c1, c2));

            c2.MaxPercentDeltaUnhealthyNodes = 14;
            VS.Assert.IsFalse(ClusterUpgradeHealthPolicy.AreEqual(c1, c2));

            c2.MaxPercentUpgradeDomainDeltaUnhealthyNodes = 23;
            VS.Assert.IsTrue(ClusterUpgradeHealthPolicy.AreEqual(c1, c2));
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("alexwun")]
        public void FabricClient_ClusterHealthPolicyToNative()
        {
            var managedPolicy = new ClusterHealthPolicy();
            managedPolicy.ConsiderWarningAsError = true;
            managedPolicy.MaxPercentUnhealthyNodes = 23;
            managedPolicy.MaxPercentUnhealthyApplications = 25;

            IntPtr nativePolicy = IntPtr.Zero;
            ClusterHealthPolicy rebuiltPolicy = null;
            using (var pin = new PinCollection())
            {
                nativePolicy = managedPolicy.ToNative(pin);
                rebuiltPolicy = ClusterHealthPolicy.FromNative(nativePolicy);
            }

            VS.Assert.IsTrue(
                ClusterHealthPolicy.AreEqual(managedPolicy, rebuiltPolicy),
                "ClusterHealthPolicy mismatch");

            // Add per app type health policy
            managedPolicy.ApplicationTypeHealthPolicyMap.Add("AppType1", 34);
            managedPolicy.ApplicationTypeHealthPolicyMap.Add("AppType2", 12);

            nativePolicy = IntPtr.Zero;
            rebuiltPolicy = null;
            using (var pin = new PinCollection())
            {
                nativePolicy = managedPolicy.ToNative(pin);
                rebuiltPolicy = ClusterHealthPolicy.FromNative(nativePolicy);
            }

            VS.Assert.IsTrue(
                ClusterHealthPolicy.AreEqual(managedPolicy, rebuiltPolicy),
                "ClusterHealthPolicy with app type mismatch");
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("bikang")]
        public void FabricClient_ClusterHealthPolicyTest()
        {
            TestUtility.ExpectException<ArgumentOutOfRangeException>(() => new ClusterHealthPolicy() { MaxPercentUnhealthyApplications = 103 }, "ClusterHealthPolicy with MaxPercentUnhealthyApplications invalid");
            TestUtility.ExpectException<ArgumentOutOfRangeException>(() =>
            {
                var c = new ClusterHealthPolicy();
                c.ApplicationTypeHealthPolicyMap.Add("AppType", 134);
            }, "ClusterHealthPolicy MaxPercentUnhealthyApplications invalid");
            
            ClusterHealthPolicy cNull = null;
            ClusterHealthPolicy c1 = new ClusterHealthPolicy()
            {
                ConsiderWarningAsError = true,
                MaxPercentUnhealthyApplications = 10,
                MaxPercentUnhealthyNodes = 23,
            };

            ClusterHealthPolicy c2 = new ClusterHealthPolicy();

            VS.Assert.IsFalse(ClusterHealthPolicy.AreEqual(cNull, c1));
            VS.Assert.IsFalse(ClusterHealthPolicy.AreEqual(c1, c2));

            c2.ConsiderWarningAsError = true;
            VS.Assert.IsFalse(ClusterHealthPolicy.AreEqual(c1, c2));

            c2.MaxPercentUnhealthyNodes = 23;
            VS.Assert.IsFalse(ClusterHealthPolicy.AreEqual(c1, c2));

            c2.MaxPercentUnhealthyApplications = 10;
            VS.Assert.IsTrue(ClusterHealthPolicy.AreEqual(c1, c2));

            c1.ApplicationTypeHealthPolicyMap.Add("AppType1", 10);
            c1.ApplicationTypeHealthPolicyMap.Add("AppType2", 14);
            VS.Assert.IsFalse(ClusterHealthPolicy.AreEqual(c1, c2));

            c2.ApplicationTypeHealthPolicyMap.Add("AppType2", 14);
            VS.Assert.IsFalse(ClusterHealthPolicy.AreEqual(c1, c2));

            c2.ApplicationTypeHealthPolicyMap.Add("AppType1", 10);
            VS.Assert.IsTrue(ClusterHealthPolicy.AreEqual(c1, c2));
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("alexwun")]
        public void FabricClient_MonitoringPolicyToNative()
        {
            var managedPolicy = new RollingUpgradeMonitoringPolicy();
            managedPolicy.FailureAction = UpgradeFailureAction.Rollback;
            managedPolicy.HealthCheckWaitDuration = TimeSpan.FromSeconds(27);
            managedPolicy.HealthCheckRetryTimeout = TimeSpan.FromSeconds(29);
            managedPolicy.UpgradeTimeout = TimeSpan.FromSeconds(31);
            managedPolicy.UpgradeDomainTimeout = TimeSpan.FromSeconds(33);

            IntPtr nativePolicy = IntPtr.Zero;
            RollingUpgradeMonitoringPolicy rebuiltPolicy = null;
            using (var pin = new PinCollection())
            {
                nativePolicy = managedPolicy.ToNative(pin);
                rebuiltPolicy = RollingUpgradeMonitoringPolicy.FromNative(nativePolicy);
            }

            VS.Assert.IsTrue(
                RollingUpgradeMonitoringPolicy.AreEqual(managedPolicy, rebuiltPolicy),
                "RollingUpgradeMonitoringPolicy mismatch");
        }

        // round trip SF service description and validate that correct data is returned
        private void StatefulServiceDescriptionE2ETestHelper(FabricClient fc, Action<StatefulServiceDescription> setup)
        {
            Guid name = Guid.NewGuid();
            Guid type = Guid.NewGuid();

            StatefulServiceDescription sd = new StatefulServiceDescription
            {
                ServiceName = new Uri("fabric:/" + name.ToString()),
                ServiceTypeName = type.ToString(),
                PlacementConstraints = "",
                PartitionSchemeDescription = new SingletonPartitionSchemeDescription(),
                TargetReplicaSetSize = 3,
                MinReplicaSetSize = 2,
            };

            if (setup != null)
            {
                setup(sd);
            }

            FabricClientTest.deployment.CreateService(sd);

            ServiceDescription sdResult = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fc.ServiceManager.GetServiceDescriptionAsync(sd.ServiceName);
            }, "GetServiceDescription");

            // the same description should be round tripped
            ServiceDescriptionTest.HelperInstance.Compare(sd, sdResult);
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("aprameyr")]
        public void FabricClient_StatefulServiceDescriptionE2E()
        {
            FabricClient fc = FabricClientTest.CreateClient();

            // singleton service
            this.StatefulServiceDescriptionE2ETestHelper(fc, (sd) => sd.PartitionSchemeDescription = new SingletonPartitionSchemeDescription());
                
            // uint64
            this.StatefulServiceDescriptionE2ETestHelper(fc, (sd) => sd.PartitionSchemeDescription = new UniformInt64RangePartitionSchemeDescription
            {
                HighKey = 41,
                LowKey = 1,
                PartitionCount = 3
            });

            // named 
            this.StatefulServiceDescriptionE2ETestHelper(fc, (sd) => sd.PartitionSchemeDescription = new NamedPartitionSchemeDescription
            {
                PartitionNames =
                {
                    "abc",
                    "def",
                }
            });
        }

        private static StatefulServiceDescription CreateDefaultStatefulServiceDescription(Guid name, Guid type)
        {
            return new StatefulServiceDescription
            {
                ServiceName = new Uri("fabric:/" + name.ToString()),
                ServiceTypeName = type.ToString(),
                PartitionSchemeDescription = new SingletonPartitionSchemeDescription(),
                TargetReplicaSetSize = 3,
                MinReplicaSetSize = 2,
            };
        }

        private void ServiceDescriptionErrorE2ETestHelper<TException, TDescription>(FabricClient fc, Action<TDescription> setup, string tag) where TException : Exception where TDescription : ServiceDescription
        {
            Guid name = Guid.NewGuid();
            Guid type = Guid.NewGuid();

            TDescription sd = null;
            if (typeof(TDescription) == typeof(StatefulServiceDescription))
            {
                sd = new StatefulServiceDescription
                {
                    ServiceName = new Uri("fabric:/" + name.ToString()),
                    ServiceTypeName = type.ToString(),
                    PartitionSchemeDescription = new SingletonPartitionSchemeDescription(),
                    TargetReplicaSetSize = 3,
                    MinReplicaSetSize = 2,
                } as TDescription;
            }
            else if (typeof(TDescription) == typeof(StatelessServiceDescription))
            {
                sd = new StatelessServiceDescription
                {
                    ServiceName = new Uri("fabric:/" + name.ToString()),
                    ServiceTypeName = type.ToString(),
                    PartitionSchemeDescription = new SingletonPartitionSchemeDescription(),
                    InstanceCount = 2,
                } as TDescription;
            }
            else
            {
                throw new InvalidOperationException("invalid sd type");
            }
            
            if (setup != null)
            {
                setup(sd);
            }

            TestUtility.ExpectException<TException>(() => fc.ServiceManager.CreateServiceAsync(sd), tag);
        }

        private void ServiceDescriptionErrorE2ETestHelper<TException>(FabricClient fc, Action<ServiceDescription> setup, string tag) where TException : Exception
        {
            this.ServiceDescriptionErrorE2ETestHelper<TException, StatefulServiceDescription>(fc, setup, tag + "_Stateful");
            this.ServiceDescriptionErrorE2ETestHelper<TException, StatelessServiceDescription>(fc, setup, tag + "_Stateless");
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("bharatn")]
        public void FabricClient_FabricClientSettingsTest()
        {
            // Default FabricClientSettings in ctor should result in reading default underlying client settings
            // from Settings property
            //
            FabricClientSettings settings = new FabricClientSettings();
            FabricClient c1 = new FabricClient(
                settings,
                FabricClientTest.deployment.ClientConnectionAddresses.Select(e => e.ToString()).ToArray());
            VS.Assert.IsNotNull(c1.Settings, "c1.Settings not set");
            VS.Assert.IsTrue(AreConfigLoadedSettingsEqual(settings, c1.Settings), "c1.Settings loaded from config mis-match");
            VS.Assert.IsTrue(AreSettingsEqual(settings, c1.Settings), "c1.Settings not loaded from config mis-match");

            // Verify defaults of settings not loaded from config. These verifications need to be updated if
            // the defaults change in the product.
            //
            VS.Assert.IsTrue(c1.Settings.ClientFriendlyName == null, "default ClientFriendlyName not null");
            VS.Assert.IsTrue(c1.Settings.PartitionLocationCacheBucketCount == 1024, "default PartitionLocationCacheBucketCount mismatch");
            VS.Assert.IsTrue(c1.Settings.ConnectionIdleTimeout == TimeSpan.Zero, "default ConnectionIdleTimeout mismatch");

            // Default FabricClient ctor should also result in default underlying client settings
            //
            FabricClient c2 = new FabricClient(
                FabricClientTest.deployment.ClientConnectionAddresses.Select(e => e.ToString()).ToArray());
            VS.Assert.IsNotNull(c2.Settings, "c2.Settings not set");
            VS.Assert.IsTrue(AreSettingsEqual(c1.Settings, c2.Settings), "c2.Settings have default values");

            var clientFriendlyName = "MyTestClient";

            // Settings applied through ctor can be read back from Settings property
            //
            FabricClientSettings clientSettingOverrides = new FabricClientSettings()
            {
                ClientFriendlyName = clientFriendlyName,
                KeepAliveInterval = TimeSpan.FromSeconds(22),
                PartitionLocationCacheLimit = 222,
                PartitionLocationCacheBucketCount = 2048,
                ServiceChangePollInterval = TimeSpan.FromSeconds(55),
                ConnectionInitializationTimeout = TimeSpan.FromSeconds(15),
                HealthOperationTimeout = TimeSpan.FromSeconds(40),
                HealthReportSendInterval = TimeSpan.FromSeconds(50),
                NotificationGatewayConnectionTimeout = TimeSpan.FromSeconds(61),
                NotificationCacheUpdateTimeout = TimeSpan.FromSeconds(72),
                AuthTokenBufferSize = 83
            };

            FabricClient c3 = new FabricClient(
                clientSettingOverrides,
                FabricClientTest.deployment.ClientConnectionAddresses.Select(e => e.ToString()).ToArray());
            VS.Assert.IsNotNull(c3.Settings, "c3.Settings not set");
            VS.Assert.IsTrue(AreSettingsEqual(clientSettingOverrides, c3.Settings), "c3.Settings have non-default values");

            // Change settings before open - this should succeed
            //
            settings = c3.Settings;
            c1.UpdateSettings(settings);
            VS.Assert.IsTrue(AreSettingsEqual(settings, c1.Settings), "c1.Settings have expected value");
            VS.Assert.IsTrue(AreSettingsEqual(c3.Settings, c1.Settings), "c1.Settings match c3.Settings");

            // Open the client
            Uri nameUri = new Uri(@"fabric:/FabricClientSettingsTest");
            c1.PropertyManager.CreateNameAsync(nameUri).Wait(taskTimeout);

            // Change settings after open

            // Client friendly name can't be changed
            {
                FabricClientSettings invalidSettings = c1.Settings;
                invalidSettings.ClientFriendlyName = "MyNewTestClient";

                TestUtility.ExpectException<InvalidOperationException>(() => c1.UpdateSettings(invalidSettings), "UpdateSettings after open - client friendly name changed");
                VS.Assert.IsTrue(AreSettingsEqual(settings, c1.Settings), "c1.Settings have expected value");
            }

            // KeepAlive can't be changed
            {
                FabricClientSettings invalidSettings = c1.Settings;
                invalidSettings.KeepAliveInterval = TimeSpan.FromSeconds(33);

                TestUtility.ExpectException<InvalidOperationException>(() => c1.UpdateSettings(invalidSettings), "UpdateSettings after open - keepAlive changed");
                VS.Assert.IsTrue(AreSettingsEqual(settings, c1.Settings), "c1.Settings have expected value");
            }

            // CacheSize can't be changed
            {
                FabricClientSettings invalidSettings = c1.Settings;
                invalidSettings.PartitionLocationCacheLimit = 444;

                TestUtility.ExpectException<InvalidOperationException>(() => c1.UpdateSettings(invalidSettings), "UpdateSettings after open - cacheSize changed");
                VS.Assert.IsTrue(AreSettingsEqual(settings, c1.Settings), "c1.Settings have expected value");
            }
            
            // Cache bucket count can't be changed
            {
                FabricClientSettings invalidSettings = c1.Settings;
                invalidSettings.PartitionLocationCacheBucketCount = 4096;

                TestUtility.ExpectException<InvalidOperationException>(() => c1.UpdateSettings(invalidSettings), "UpdateSettings after open - cache bucket count changed");
                VS.Assert.IsTrue(AreSettingsEqual(settings, c1.Settings), "c1.Settings have expected value");
            }

            // ConnectionInitializationTimeout must be less than poll interval
            {
                FabricClientSettings invalidSettings = c1.Settings;
                invalidSettings.ServiceChangePollInterval = TimeSpan.FromSeconds(54);
                invalidSettings.ConnectionInitializationTimeout = TimeSpan.FromSeconds(55);

                TestUtility.ExpectException<ArgumentException>(() => c1.UpdateSettings(invalidSettings), "UpdateSettings after open - connection timeout > poll interval");
                VS.Assert.IsTrue(AreSettingsEqual(settings, c1.Settings), "c1.Settings have expected value");
            }

            // Valid settings: change the timeouts
            {
                FabricClientSettings newSettings = c1.Settings;
                newSettings.ServiceChangePollInterval = TimeSpan.FromSeconds(123);
                newSettings.ConnectionInitializationTimeout = TimeSpan.FromSeconds(25);
                newSettings.HealthReportSendInterval = TimeSpan.FromSeconds(122);
                newSettings.HealthOperationTimeout = TimeSpan.FromSeconds(110);
                c1.UpdateSettings(newSettings);
                VS.Assert.IsTrue(AreSettingsEqual(newSettings, c1.Settings), "c1.Settings have expected value");
            }
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("aprameyr")]
        public void FabricClient_CommonCreateServiceErrors()
        {
            FabricClient fc = FabricClientTest.CreateClient();

            this.ServiceDescriptionErrorE2ETestHelper<ArgumentNullException>(fc, (sd) => sd.ServiceName = null, "Null Service Name");
            this.ServiceDescriptionErrorE2ETestHelper<ArgumentNullException>(fc, (sd) => sd.PartitionSchemeDescription = null, "Null Partition Description");
            this.ServiceDescriptionErrorE2ETestHelper<ArgumentNullException>(fc, (sd) => sd.ServiceTypeName = null, "null service type");
            this.ServiceDescriptionErrorE2ETestHelper<ArgumentException>(fc, (sd) => sd.PlacementConstraints = "1==", "ServiceDescription.InvalidPlacementConstraints");
            this.ServiceDescriptionErrorE2ETestHelper<ArgumentException>(fc, (sd) => sd.PlacementConstraints = "==1", "ServiceDescription.InvalidPlacementConstraints");
            this.ServiceDescriptionErrorE2ETestHelper<ArgumentException>(fc, (sd) => sd.PartitionSchemeDescription = new UniformInt64RangePartitionSchemeDescription { LowKey = 0, HighKey = -1, PartitionCount = 3 }, "Invalid Low/High key");
            this.ServiceDescriptionErrorE2ETestHelper<ArgumentException>(fc, (sd) => sd.PartitionSchemeDescription = new UniformInt64RangePartitionSchemeDescription { LowKey = 0, HighKey = 1, PartitionCount = 3 }, "Invalid partition count key");
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("aprameyr")]
        public void FabricClient_CreateStatefulServiceErrors()
        {
            FabricClient fc = FabricClientTest.CreateClient();

            this.ServiceDescriptionErrorE2ETestHelper<ArgumentException, StatefulServiceDescription>(fc, (sd) => sd.MinReplicaSetSize = -1, "Invalid MinReplicaSetSize");
            this.ServiceDescriptionErrorE2ETestHelper<ArgumentException, StatefulServiceDescription>(fc, (sd) => sd.MinReplicaSetSize = 0, "Invalid MinReplicaSetSize");
            this.ServiceDescriptionErrorE2ETestHelper<ArgumentException, StatefulServiceDescription>(fc, (sd) => { sd.MinReplicaSetSize = 4; sd.TargetReplicaSetSize = 3; }, "Invalid MinReplicaSetSize");
            this.ServiceDescriptionErrorE2ETestHelper<ArgumentException, StatefulServiceDescription>(fc, (sd) => sd.TargetReplicaSetSize = -1, "Invalid target replica set size");
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("aprameyr")]
        public void FabricClientTest_ManipulateApplications()
        {
            FabricClient fabricClient = FabricClientTest.CreateClient();

            var appParameters = new NameValueCollection();
            appParameters.Add("name", "value");

            TestUtility.ExpectExceptionAndMessage<FabricElementNotFoundException>(
                () => fabricClient.ApplicationManager.CreateApplicationAsync(new ApplicationDescription(new Uri(@"fabric:/SomeApplication"), "type1", "version1", appParameters)),
                "CreateApplication",
                "type and version not found");

            LogHelper.Log("Successfully checked application create api");

            TestUtility.ExpectExceptionAndMessage<FabricElementNotFoundException>(
                () => fabricClient.ApplicationManager.DeleteApplicationAsync(new DeleteApplicationDescription(new Uri(@"fabric:/SomeApplication"))),
                "DeleteApplication",
                "Application not found");

            LogHelper.Log("Successfully checked application delete api");
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("alexwun")]
        public void FabricClientTest_ManipulateFabric()
        {
            FabricClient fabricClient = FabricClientTest.CreateClient();

            // Provision
            TestUtility.ExpectException<System.IO.FileNotFoundException>(
                () => fabricClient.ClusterManager.ProvisionFabricAsync("invalidMspFilepath", "invalidClusterManifestFilepath"),
                "ProvisionFabric");

            // Upgrade
            FabricUpgradeDescription upgradeDescription = new FabricUpgradeDescription();
            upgradeDescription.TargetCodeVersion = null;
            upgradeDescription.TargetConfigVersion = "invalidConfigVersion";

            RollingUpgradePolicyDescription policyDescription = new RollingUpgradePolicyDescription();
            policyDescription.UpgradeMode = RollingUpgradeMode.UnmonitoredManual;
            upgradeDescription.UpgradePolicyDescription = policyDescription;

            TestUtility.ExpectExceptionAndMessage<FabricElementNotFoundException>(
                () => fabricClient.ClusterManager.UpgradeFabricAsync(upgradeDescription),
                "UpgradeFabric",
                "version has not been registered");

            // Get progress
            FabricUpgradeProgress upgradeProgress = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fabricClient.ClusterManager.GetFabricUpgradeProgressAsync();
            }, "GetFabricUpgradeProgress");

            VS.Assert.IsTrue(upgradeProgress.UpgradeState == FabricUpgradeState.RollingForwardCompleted);

            // Move next
            TestUtility.ExpectExceptionAndMessage<FabricException>(
                () => fabricClient.ClusterManager.MoveNextFabricUpgradeDomainAsync(upgradeProgress),
                "MoveNext",
                "no pending Fabric upgrade");

            // Unprovision
            TestUtility.ExpectExceptionAndMessage<FabricElementNotFoundException>(
                () => fabricClient.ClusterManager.UnprovisionFabricAsync(null, "invalidVersion"),
                "Unprovision",
                "version has not been registered");

            LogHelper.Log("Successfully verified Fabric upgrade API.");
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("aprameyr")]
        public void FabricClientTest_ManipulateServiceDescription()
        {
            Uri serviceName = new Uri(@"fabric:/servicedescription1");

            FabricClient fabricClient = FabricClientTest.CreateClient();
            StatelessServiceDescription stateless = new StatelessServiceDescription
            {
                ServiceName = serviceName,
                ServiceTypeName = "ServiceType1",
                ApplicationName = null,
                PlacementConstraints = null,
                PartitionSchemeDescription = new SingletonPartitionSchemeDescription(),
                InstanceCount = 1,
                InitializationData = new byte[] { 0 }
            };

            FabricClientTest.deployment.CreateService(stateless);

            TestUtility.ExpectExceptionAndMessage<FabricElementAlreadyExistsException>(
                () => fabricClient.ServiceManager.CreateServiceAsync(stateless),
                "CreateService",
                "Service already exists");

            LogHelper.Log("Successfully created stateless service description");

            var readServiceDesc = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fabricClient.ServiceManager.GetServiceDescriptionAsync(serviceName);
            }, "GetServiceDescription");

            CompareStrings(serviceName.ToString(), readServiceDesc.ServiceName.ToString());

            LogHelper.Log("Successfully read stateless service description");

            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fabricClient.ServiceManager.DeleteServiceAsync(new DeleteServiceDescription(serviceName));
            }, "DeleteService", FabricErrorCode.ServiceNotFound);

            LogHelper.Log("Successfully deleted stateless service description");

            TestUtility.ExpectExceptionAndMessage<FabricServiceNotFoundException>(
                () => fabricClient.ServiceManager.DeleteServiceAsync(new DeleteServiceDescription(serviceName)),
                "DeleteService",
                "does not exist");

            TestUtility.ExpectExceptionAndMessage<FabricElementNotFoundException>(
                () => fabricClient.ServiceManager.CreateServiceFromTemplateAsync(new Uri(@"fabric:/application1"), new Uri(@"fabric:/service1"), "type1", new byte[1]),
                "CreateServiceFromTemplate",
                "Application not found");
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("aprameyr")]
        public void FabricClientTest_ManipulateNameAndProperties()
        {
            Uri firstNameUri = new Uri(@"fabric:/MyFirstname");
            Uri secondNameUri = new Uri(@"fabric:/MySecondname");
            Uri firstNameUri_child1 = new Uri(@"fabric:/MyFirstname/def");
            string property1 = "property1";
            string property2 = "property2";
            string property3 = "property3";
            string value1 = "value1";
            string value2 = "value2";

            FabricClient fabricClient = FabricClientTest.CreateClient();

            FabricClientTest.deployment.CreateName(firstNameUri);
            FabricClientTest.deployment.CreateName(firstNameUri_child1);
            
            TestUtility.ExpectExceptionAndMessage<FabricElementAlreadyExistsException>(
                () => fabricClient.PropertyManager.CreateNameAsync(firstNameUri),
                "CreateName",
                "Name already exists");

            LogHelper.Log("Successfully created all names");

            bool checkName = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fabricClient.PropertyManager.NameExistsAsync(firstNameUri);
            }, "NameExists");

            VS.Assert.IsTrue(checkName, "Name must exist");

            LogHelper.Log(string.Format(CultureInfo.InvariantCulture, "Successfully read a name {0}", firstNameUri));

            NameEnumerationResult readChildren = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fabricClient.PropertyManager.EnumerateSubNamesAsync(firstNameUri, null, true);
            }, "EnumerateSubNames");

            VS.Assert.AreEqual<int>(1, readChildren.Count());

            CompareStrings(@"fabric:/MyFirstname/def", readChildren.FirstOrDefault().ToString());

            LogHelper.Log("Successfully read children for a name");

            FabricClientTest.deployment.DeleteName(firstNameUri_child1);
            FabricClientTest.deployment.DeleteName(firstNameUri);

            TestUtility.ExpectExceptionAndMessage<FabricElementNotFoundException>(
                () => fabricClient.PropertyManager.DeleteNameAsync(firstNameUri),
                "DeleteName",
                "does not exist");

            LogHelper.Log("Successfully deleted a name");

            checkName = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fabricClient.PropertyManager.NameExistsAsync(firstNameUri);
            }, "NameExists");

            VS.Assert.IsTrue(!checkName, "Name should not exist");

            // Manipulate name and properties
            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fabricClient.PropertyManager.CreateNameAsync(secondNameUri);
            }, "CreateName", FabricErrorCode.NameAlreadyExists);

            LogHelper.Log("Successfully created a name");

            CreateProperty(fabricClient, secondNameUri, property1, value1);
            CreateProperty(fabricClient, secondNameUri, property2, value2);

            LogHelper.Log("Successfully created properties");

            TestUtility.ExpectExceptionAndMessage<FabricException>(
                () => fabricClient.PropertyManager.DeleteNameAsync(secondNameUri),
                "DeleteName",
                "not empty");

            DeleteProperty(fabricClient, secondNameUri, property2);
            DeleteProperty(fabricClient, secondNameUri, property1);

            LogHelper.Log("Successfully deleted properties");

            LogHelper.Log("Validating data types");

            FabricClientTest.CreateReadAndDeleteProperty<long>(fabricClient, secondNameUri, property3, 123, (mgr, name, value, parent) => mgr.PutPropertyAsync(parent, name, value));
            FabricClientTest.CreateReadAndDeleteProperty<Guid>(fabricClient, secondNameUri, property3, Guid.NewGuid(), (mgr, name, value, parent) => mgr.PutPropertyAsync(parent, name, value));
            FabricClientTest.CreateReadAndDeleteProperty<double>(fabricClient, secondNameUri, property3, 0.01, (mgr, name, value, parent) => mgr.PutPropertyAsync(parent, name, value));
            FabricClientTest.CreateReadAndDeleteProperty<string>(fabricClient, secondNameUri, property3, "hi", (mgr, name, value, parent) => mgr.PutPropertyAsync(parent, name, value));

            byte[] array = new byte[] { 0, 1, 2 };
            FabricClientTest.CreateReadAndDeleteProperty<byte[]>(fabricClient, secondNameUri, property3, array, (mgr, name, value, parent) => mgr.PutPropertyAsync(parent, name, value), (actual) => MiscUtility.CompareEnumerables(array, actual));

            FabricClientTest.deployment.DeleteName(secondNameUri);
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("bharatn")]
        public void FabricClientTest_SubmitPropertyBatch()
        {
            Uri nameUri = new Uri(@"fabric:/SubmitPropertyBatchName");
            string property1 = "property1";
            string value1 = "value1";
            string property2 = "property2";
            byte[] value2 = new byte[] { 0, 1, 2 };
            string property3 = "property3";
            Guid value3 = Guid.NewGuid();
            string property4 = "property4";
            string value4 = "3.14";

            FabricClient fabricClient = FabricClientTest.CreateClient();

            FabricClientTest.deployment.CreateName(nameUri);
            LogHelper.Log("Successfully created a name");
            
            FabricClientTest.CreateProperty(fabricClient, nameUri, property1, value1);
            FabricClientTest.CreateProperty(fabricClient, nameUri, property4, value4);

            LogHelper.Log("Successfully created properties");

            {
                // Create batch
                ICollection<PropertyBatchOperation> operations = new List<PropertyBatchOperation>();

                operations.Add(new PutPropertyOperation(property2, value2));
                operations.Add(new GetPropertyOperation(property1));
                operations.Add(new GetPropertyOperation(property4, false));
                operations.Add(new CheckExistsPropertyOperation(property2, true));
                operations.Add(new PutPropertyOperation(property3, value3));
                operations.Add(new DeletePropertyOperation(property4));

                PropertyBatchResult result = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.SubmitPropertyBatchAsync(nameUri, operations);
                }, "SubmitPropertyBatch");

                VS.Assert.AreEqual<int>(-1, result.FailedOperationIndex, "No error should have occurred. Instead {0}", result.FailedOperationIndex);
                VS.Assert.IsNull(result.FailedOperationException, "No exception should be set");

                // Check that property2 was correctly put
                NamedProperty readProperty = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.GetPropertyAsync(nameUri, property2);
                }, "GetProperty");

                CompareStrings(property2, readProperty.Metadata.PropertyName);
                var actualPropertyValue = readProperty.GetValue<byte[]>();
                VS.Assert.AreEqual(value2.Length, actualPropertyValue.Length);
                for (int i = 0; i < value2.Length; i++)
                {
                    VS.Assert.AreEqual(value2[i], actualPropertyValue[i]);
                }

                // Check that property3 was correctly put
                NamedProperty readProperty2 = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.GetPropertyAsync(nameUri, property3);
                }, "GetProperty");

                CompareStrings(property3, readProperty2.Metadata.PropertyName);
                var actualPropertyValue2 = readProperty2.GetValue<Guid>();
                VS.Assert.AreEqual(value3, actualPropertyValue2);

                // Check that property 4 was deleted
                TestUtility.ExpectExceptionAndMessage<FabricElementNotFoundException>(
                    () => fabricClient.PropertyManager.GetPropertyAsync(nameUri, property4),
                    "GetProperty",
                    "does not exist");
                
                // At index 1 we have valid named property
                NamedProperty getProperty1 = result.GetProperty(1);
                VS.Assert.AreEqual(getProperty1.Metadata.PropertyName, property1, "Property name is correct");
                VS.Assert.AreEqual(getProperty1.GetValue<string>(), value1, "Property value is correct");

                // At index 2 we have valid named property with a null value
                NamedProperty getProperty4 = result.GetProperty(2);
                VS.Assert.AreEqual(getProperty4.Metadata.PropertyName, property4, "Property name is correct");
                try
                {
                    getProperty4.GetValue<string>();
                    VS.Assert.Fail("Property property4 should have a null value");
                }
                catch (FabricException ex)
                {
                    VS.Assert.AreEqual(ex.ErrorCode, FabricErrorCode.PropertyValueEmpty, "Property value is null");
                }

                LogHelper.Log("Successfully submitted batch");
            }

            {
                List<PropertyBatchOperation> operations2 = new List<PropertyBatchOperation>();
                operations2.Add(new CheckExistsPropertyOperation(property1, false));

                PropertyBatchResult result2 = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.SubmitPropertyBatchAsync(nameUri, operations2);
                }, "SubmitPropertyBatch");

                VS.Assert.AreEqual<int>(0, result2.FailedOperationIndex, "Failed operation index mismatch");
                VS.Assert.IsNotNull(result2.FailedOperationException, "Batch should contain exception");
                VS.Assert.IsTrue(result2.FailedOperationException.Message.Contains("The property check failed"), "Expected exception encountered");

                try
                {
                    result2.GetProperty(0);
                    VS.Assert.Fail("Existence check should fail");
                }
                catch (FabricException ex)
                {
                    VS.Assert.AreEqual(ex.ErrorCode, FabricErrorCode.PropertyCheckFailed, "Expected exception encountered");
                }

                LogHelper.Log("Successfully submitted batch2");
            }

            {
                List<PropertyBatchOperation> operations2 = new List<PropertyBatchOperation>();
                operations2.Add(new CheckSequencePropertyOperation(property1, 999));

                PropertyBatchResult result2 = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.SubmitPropertyBatchAsync(nameUri, operations2);
                }, "SubmitPropertyBatch");
                
                VS.Assert.AreEqual<int>(0, result2.FailedOperationIndex, "Failed operation index mismatch");
                VS.Assert.IsNotNull(result2.FailedOperationException, "Batch should contain exception");
                VS.Assert.IsTrue(result2.FailedOperationException.Message.Contains("The property check failed"), "Expected exception encountered");

                try
                {
                    result2.GetProperty(0);
                    VS.Assert.Fail("Sequence number check should fail");
                }
                catch (FabricException ex)
                {
                    VS.Assert.AreEqual(ex.ErrorCode, FabricErrorCode.PropertyCheckFailed, "Expected exception encountered");
                }

                LogHelper.Log("Successfully submitted batch2");
            }
                        
            DeleteProperty(fabricClient, nameUri, property3);
            DeleteProperty(fabricClient, nameUri, property2);
            DeleteProperty(fabricClient, nameUri, property1);

            FabricClientTest.deployment.DeleteName(nameUri);
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("bharatn")]
        public void FabricClientTest_CustomTypeProperty()
        {
            Uri nameUri = new Uri(@"fabric:/CustomTypeProperty");
            string property1 = "property1";

            FabricClient fabricClient = FabricClientTest.CreateClient();

            FabricClientTest.deployment.CreateName(nameUri);

            LogHelper.Log("Successfully created a name");

            {
                // Put a property with custom type, then check the custom type is preserved
                // both when getting the property through batch and through GetProperty
                byte[] value1 = new byte[] { 0, 1, 2 };
                string customTypeId1 = "MyBytesType";

                // Create batch
                ICollection<PropertyBatchOperation> operations = new List<PropertyBatchOperation>();

                // Put property with custom type
                operations.Add(new PutCustomPropertyOperation(property1, value1, customTypeId1));
                operations.Add(new GetPropertyOperation(property1, true));
                operations.Add(new GetPropertyOperation(property1, false));

                var result = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.SubmitPropertyBatchAsync(nameUri, operations);
                }, "SubmitPropertyBatch");

                VS.Assert.AreEqual<int>(-1, result.FailedOperationIndex, "No error should have occurred. Instead {0}", result.FailedOperationIndex);
                VS.Assert.IsNull(result.FailedOperationException, "No exception should be set");

                LogHelper.Log("Successfully submitted batch. Checking results");

                // Check that property1 was correctly put,
                // including the desired custom type.

                // First read directly with GetPropertyAsync,
                // then check the property/metadata returned in batch
                var readProperty = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.GetPropertyAsync(nameUri, property1);
                }, "GetProperty");

                CompareStrings(property1, readProperty.Metadata.PropertyName);
                CompareStrings(customTypeId1, readProperty.Metadata.CustomTypeId);
                CompareBytes(value1, readProperty.GetValue<byte[]>());

                LogHelper.Log("PutProperty verified with GetProperty");

                // At index 1 we should have same valid named property
                var getProperty1 = result.GetProperty(1);
                CompareStrings(property1, getProperty1.Metadata.PropertyName);
                CompareStrings(customTypeId1, getProperty1.Metadata.CustomTypeId);
                CompareBytes(value1, getProperty1.GetValue<byte[]>());
                LogHelper.Log("GetProperty with value returned correct result");

                // At index 2 we should have same valid named property metadata
                var getProperty2 = result.GetProperty(2);
                CompareStrings(property1, getProperty2.Metadata.PropertyName);
                CompareStrings(customTypeId1, getProperty2.Metadata.CustomTypeId);
                try
                {
                    getProperty2.GetValue<byte[]>();
                    VS.Assert.Fail("Property should have a null value");
                }
                catch (FabricException ex)
                {
                    VS.Assert.AreEqual(ex.ErrorCode, FabricErrorCode.PropertyValueEmpty, "Property value is null");
                }

                LogHelper.Log("GetProperty without value returned correct result");
            }

            string property2 = "property2";

            {
                string value = "valueForProperty2";
                string customTypeId = "MyStringType";

                // Put property with custom type and verify that get returns the custom type properly
                var operation = new PutCustomPropertyOperation(property2, value, customTypeId);
                FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.PutCustomPropertyOperationAsync(nameUri, operation);
                }, "PutCustomProperty");

                LogHelper.Log("PutCustomProperty property2 with first value returned successfully");

                var readProperty = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.GetPropertyAsync(nameUri, property2);
                }, "GetProperty");

                CompareStrings(property2, readProperty.Metadata.PropertyName);
                CompareStrings(customTypeId, readProperty.Metadata.CustomTypeId);
                CompareStrings(value, readProperty.GetValue<string>());
                LogHelper.Log("GetProperty property2 with first value returned correct results");
            }

            {
                // Replace property2 with new value and empty custom type, check that get returns empty custom type
                string value = "newValueForProperty2";
                string customTypeId = "newCustomTypeForProperty2";
                var operation = new PutCustomPropertyOperation(property2, value, customTypeId);
                FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.PutCustomPropertyOperationAsync(nameUri, operation);
                }, "PutCustomProperty");
                LogHelper.Log("PutCustomProperty property2 with second value returned successfully");

                var readPropertyMetadata = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.GetPropertyMetadataAsync(nameUri, property2);
                }, "GetProperty");

                CompareStrings(property2, readPropertyMetadata.PropertyName);
                CompareStrings(customTypeId, readPropertyMetadata.CustomTypeId);
                LogHelper.Log("GetPropertyMetadata property2 with second value returned correct results");

                var readProperty = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.GetPropertyAsync(nameUri, property2);
                }, "GetProperty");

                CompareStrings(property2, readProperty.Metadata.PropertyName);
                CompareStrings(customTypeId, readProperty.Metadata.CustomTypeId);
                CompareStrings(value, readProperty.GetValue<string>());
                LogHelper.Log("GetProperty property2 with second value returned correct results");
            }
            
            string property3 = "property3";
            {
                // PutProperty and check that the custom type id is null
                string value = "valueForPutProperty";

                FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.PutPropertyAsync(nameUri, property3, value);
                }, "PutProperty");
                LogHelper.Log("PutProperty property3 returned successfully");

                var readPropertyMetadata = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.GetPropertyMetadataAsync(nameUri, property3);
                }, "GetProperty");

                CompareStrings(property3, readPropertyMetadata.PropertyName);
                VS.Assert.IsNull(readPropertyMetadata.CustomTypeId, "CustomTypeId should not be set");
                LogHelper.Log("GetPropertyMetadata property3 returned correct results");
            }
            
            DeleteProperty(fabricClient, nameUri, property1);
            DeleteProperty(fabricClient, nameUri, property2);
            DeleteProperty(fabricClient, nameUri, property3);

            FabricClientTest.deployment.DeleteName(nameUri);
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("bharatn")]
        public void FabricClientTest_SubmitPropertyBatchWithCheckValue()
        {
            Uri nameUri = new Uri(@"fabric:/SubmitPropertyBatchWithCheckValueName");
            // string property
            string property1 = "property1";
            string value1 = "value1";
            string nonValue1 = "nonValue1";

            // binary property
            string property2 = "property2";
            byte[] value2 = new byte[] { 0, 1, 2 };

            // GUID property
            string property3 = "property3";
            Guid value3 = Guid.NewGuid();

            // double property
            string property4 = "property4";
            double value4 = 3.14;

            // Int property
            string property5 = "property5";
            long value5 = 5;

            FabricClient fabricClient = FabricClientTest.CreateClient();

            FabricClientTest.deployment.CreateName(nameUri);
            LogHelper.Log("Successfully created a name");

            // Create batch to put and get properties of different types
            {
                ICollection<PropertyBatchOperation> operations = new List<PropertyBatchOperation>();

                operations.Add(new PutPropertyOperation(property1, value1));
                operations.Add(new CheckValuePropertyOperation(property1, value1));
                operations.Add(new GetPropertyOperation(property1)); // index 2

                operations.Add(new PutPropertyOperation(property2, value2));
                operations.Add(new CheckValuePropertyOperation(property2, value2));
                operations.Add(new GetPropertyOperation(property2)); // index 5

                operations.Add(new PutPropertyOperation(property3, value3));
                operations.Add(new CheckValuePropertyOperation(property3, value3));
                operations.Add(new GetPropertyOperation(property3)); // index 8

                operations.Add(new PutPropertyOperation(property4, value4));
                operations.Add(new CheckValuePropertyOperation(property4, value4));
                operations.Add(new GetPropertyOperation(property4)); // index 11

                operations.Add(new PutPropertyOperation(property5, value5));
                operations.Add(new CheckValuePropertyOperation(property5, value5));
                operations.Add(new GetPropertyOperation(property5)); // index 14

                PropertyBatchResult result = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.SubmitPropertyBatchAsync(nameUri, operations);
                }, "SubmitPropertyBatch");

                VS.Assert.AreEqual<int>(-1, result.FailedOperationIndex, "No error should have occurred. Instead {0}", result.FailedOperationIndex);
                VS.Assert.IsNull(result.FailedOperationException, "No exception should be set");

                // Check that properties were correctly returned in batch at correct indexes
                NamedProperty getProperty1 = result.GetProperty(2); // property1
                VS.Assert.AreEqual(property1, getProperty1.Metadata.PropertyName, "Property1 name is correct");
                VS.Assert.AreEqual(value1, getProperty1.GetValue<string>(), "Property1 value is correct");

                NamedProperty getProperty2 = result.GetProperty(5); // property2
                VS.Assert.AreEqual(property2, getProperty2.Metadata.PropertyName, "Property2 name is correct");
                CompareBytes(value2, getProperty2.GetValue<byte[]>());

                NamedProperty getProperty3 = result.GetProperty(8); // property3
                VS.Assert.AreEqual(property3, getProperty3.Metadata.PropertyName, "Property3 name is correct");
                VS.Assert.AreEqual(value3, getProperty3.GetValue<Guid>(), "Property3 value is correct");

                NamedProperty getProperty4 = result.GetProperty(11); // property4
                VS.Assert.AreEqual(property4, getProperty4.Metadata.PropertyName, "Property4 name is correct");
                VS.Assert.AreEqual(value4, getProperty4.GetValue<double>(), "Property4 value is correct");

                NamedProperty getProperty5 = result.GetProperty(14); // property5
                VS.Assert.AreEqual(property5, getProperty5.Metadata.PropertyName, "Property5 name is correct");
                VS.Assert.AreEqual(value5, getProperty5.GetValue<long>(), "Property5 value is correct");

                LogHelper.Log("Successfully submitted batch");
            }

            // Create batch with check property with non-matching types - expected to fail
            {
                ICollection<PropertyBatchOperation> operations = new List<PropertyBatchOperation>();
                operations.Add(new GetPropertyOperation(property1));
                operations.Add(new CheckValuePropertyOperation(property1, value3));

                PropertyBatchResult result = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.SubmitPropertyBatchAsync(nameUri, operations);
                }, "SubmitPropertyBatch");

                VS.Assert.AreEqual<int>(1, result.FailedOperationIndex, "Failed operation index mismatch");
                VS.Assert.IsNotNull(result.FailedOperationException, "Batch should contain exception");
                VS.Assert.IsTrue(result.FailedOperationException.Message.Contains("The property check failed"), "Expected exception encountered");

                LogHelper.Log("Successfully submitted batch");
            }

            // Create batch with check property with non-matching value - expected to fail
            {
                ICollection<PropertyBatchOperation> operations = new List<PropertyBatchOperation>();
                operations.Add(new CheckValuePropertyOperation(property1, nonValue1));

                PropertyBatchResult result = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return fabricClient.PropertyManager.SubmitPropertyBatchAsync(nameUri, operations);
                }, "SubmitPropertyBatch");

                VS.Assert.AreEqual<int>(0, result.FailedOperationIndex, "Failed operation index mismatch");
                VS.Assert.IsNotNull(result.FailedOperationException, "Batch should contain exception");
                VS.Assert.IsTrue(result.FailedOperationException.Message.Contains("The property check failed"), "Expected exception encountered");

                LogHelper.Log("Successfully submitted batch");
            }

            DeleteProperty(fabricClient, nameUri, property1);
            DeleteProperty(fabricClient, nameUri, property2);
            DeleteProperty(fabricClient, nameUri, property3);
            DeleteProperty(fabricClient, nameUri, property4);
            DeleteProperty(fabricClient, nameUri, property5);

            FabricClientTest.deployment.DeleteName(nameUri);
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("bharatn")]
        public void FabricClientTest_PropertyBatchInvalidOperations()
        {
            TestUtility.ExpectException<ArgumentNullException>(() => new PutPropertyOperation(null, "test"), "PutPropertyOperation");
            TestUtility.ExpectException<ArgumentNullException>(() => new PutCustomPropertyOperation(null, "test", "customType"), "PutCustomPropertyOperation");
            TestUtility.ExpectException<ArgumentNullException>(() => new PutCustomPropertyOperation("property", "test", null), "PutCustomPropertyOperation");
            TestUtility.ExpectException<ArgumentNullException>(() => new GetPropertyOperation(null), "GetPropertyOperation");
            TestUtility.ExpectException<ArgumentNullException>(() => new CheckExistsPropertyOperation(null, true), "CheckExistsPropertyOperation");
            TestUtility.ExpectException<ArgumentNullException>(() => new CheckSequencePropertyOperation(null, 121), "CheckSequencePropertyOperation");
            TestUtility.ExpectException<ArgumentNullException>(() => new DeletePropertyOperation(null), "DeletePropertyOperation");
            TestUtility.ExpectException<ArgumentNullException>(() => new CheckValuePropertyOperation(null, "test"), "CheckValuePropertyOperation");
            
            TestUtility.ExpectException<ArgumentException>(() => new PutPropertyOperation(string.Empty, "test"), "PutPropertyOperation");
            TestUtility.ExpectException<ArgumentException>(() => new PutCustomPropertyOperation(string.Empty, "test", "customType"), "PutCustomPropertyOperation");
            TestUtility.ExpectException<ArgumentException>(() => new PutCustomPropertyOperation("property", "test", string.Empty), "PutCustomPropertyOperation");
            TestUtility.ExpectException<ArgumentException>(() => new GetPropertyOperation(string.Empty), "GetPropertyOperation");
            TestUtility.ExpectException<ArgumentException>(() => new CheckExistsPropertyOperation(string.Empty, true), "CheckExistsPropertyOperation");
            TestUtility.ExpectException<ArgumentException>(() => new CheckSequencePropertyOperation(string.Empty, 121), "CheckSequencePropertyOperation");
            TestUtility.ExpectException<ArgumentException>(() => new DeletePropertyOperation(string.Empty), "DeletePropertyOperation");
            TestUtility.ExpectException<ArgumentException>(() => new CheckValuePropertyOperation(string.Empty, "test"), "CheckValuePropertyOperation");
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("bikang")]
        public void FabricClientTest_HealthInformationParameterChecks()
        {
            string source = "sourceId";
            string property = "property";

            string invalidSource = "";
            string invalidProperty = "";
            long invalidSequenceNumber = -2;
            TimeSpan invalidTTL = TimeSpan.FromSeconds(-10);

            LogHelper.Log("Check health information validation");

            // invalid source
            TestUtility.ExpectException<ArgumentException>(
                () =>
                {
                    var healthInformation = new HealthInformation(invalidSource, property, HealthState.Ok);
                },
                "HealthInformationValidation-InvalidSource");

            // invalid source
            TestUtility.ExpectException<ArgumentException>(
                () =>
                {
                    var healthInformation = new HealthInformation(source, invalidProperty, HealthState.Ok);
                },
                "HealthInformationValidation-InvalidProperty");

            {
                var healthInformation = new HealthInformation(source, property, HealthState.Ok);
                TestUtility.ExpectException<ArgumentException>(
                    () =>
                    {

                        healthInformation.SequenceNumber = invalidSequenceNumber;
                    },
                    "HealthInformationValidation-InvalidSequenceNumber");
            }

            {
                var healthInformation = new HealthInformation(source, property, HealthState.Ok);
                TestUtility.ExpectException<ArgumentException>(
                    () =>
                    {
                        healthInformation.TimeToLive = invalidTTL;
                    },
                    "HealthInformationValidation-InvalidTTL");
            }
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("bikang")]
        public void FabricClientTest_HealthQueryDescriptionChecks()
        {
            TestUtility.ExpectException<ArgumentNullException>(() => { var queryDescription = new NodeHealthQueryDescription(null); }, "NodeHealthQueryDescription.NullName");
            TestUtility.ExpectException<ArgumentException>(() => { var queryDescription = new NodeHealthQueryDescription(""); }, "NodeHealthQueryDescription.EmptyName");

            TestUtility.ExpectException<ArgumentNullException>(() => { var queryDescription = new ApplicationHealthQueryDescription(null); }, "ApplicationHealthQueryDescription.NullAppName");

            TestUtility.ExpectException<ArgumentNullException>(() => { var queryDescription = new DeployedApplicationHealthQueryDescription(null, null); }, "DeployedApplicationHealthQueryDescription");
            TestUtility.ExpectException<ArgumentNullException>(() => { var queryDescription = new DeployedApplicationHealthQueryDescription(null, "node"); }, "DeployedApplicationHealthQueryDescription");
            TestUtility.ExpectException<ArgumentNullException>(() => { var queryDescription = new DeployedApplicationHealthQueryDescription(new Uri("fabric:/app1"), null); }, "DeployedApplicationHealthQueryDescription");
            TestUtility.ExpectException<ArgumentException>(() => { var queryDescription = new DeployedApplicationHealthQueryDescription(new Uri("fabric:/app1"), ""); }, "DeployedApplicationHealthQueryDescription");

            TestUtility.ExpectException<ArgumentNullException>(() => { var queryDescription = new DeployedServicePackageHealthQueryDescription(null, "node", "manifest"); }, "DeployedServicePackageHealthQueryDescription");
            TestUtility.ExpectException<ArgumentNullException>(() => { var queryDescription = new DeployedServicePackageHealthQueryDescription(new Uri("fabric:/app1"), "node", null); }, "DeployedServicePackageHealthQueryDescription");
            TestUtility.ExpectException<ArgumentException>(() => { var queryDescription = new DeployedServicePackageHealthQueryDescription(new Uri("fabric:/app1"), "", null); }, "DeployedServicePackageHealthQueryDescription");
            TestUtility.ExpectException<ArgumentNullException>(() => { var queryDescription = new DeployedServicePackageHealthQueryDescription(new Uri("fabric:/app1"), null, "manifest"); }, "DeployedServicePackageHealthQueryDescription");
            TestUtility.ExpectException<ArgumentException>(() => { var queryDescription = new DeployedServicePackageHealthQueryDescription(new Uri("fabric:/app1"), "node", ""); }, "DeployedServicePackageHealthQueryDescription");
            TestUtility.ExpectException<ArgumentNullException>(() => { var queryDescription = new DeployedServicePackageHealthQueryDescription(new Uri("fabric:/app1"), "node", null); }, "DeployedServicePackageHealthQueryDescription");

            TestUtility.ExpectException<ArgumentNullException>(() => { var queryDescription = new ServiceHealthQueryDescription(null); }, "ServiceHealthQueryDescription.NullAppName");
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("aprameyr")]
        public void FabricClientTest_EnumerateProperties()
        {
            Uri nameUri = new Uri(@"fabric:/EnumeratePropertiesName");

            Dictionary<string, string> properties = new Dictionary<string, string>();
            properties.Add("property1", "value1");
            properties.Add("property2", "value2");
            properties.Add("differentPrefixProperty", "value3");

            FabricClient fabricClient = FabricClientTest.CreateClient();

            FabricClientTest.deployment.CreateName(nameUri);
            LogHelper.Log("Successfully created a name");

            foreach (var entry in properties)
            {
                CreateProperty(fabricClient, nameUri, entry.Key, entry.Value);
            }

            LogHelper.Log("Successfully created properties");

            // Enumerate properties with values
            PropertyEnumerationResult readProperties = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fabricClient.PropertyManager.EnumeratePropertiesAsync(nameUri, true, null);
            }, "EnumerateProperties");
            
            VS.Assert.AreEqual<int>(properties.Count, readProperties.Count());
            VS.Assert.IsFalse(readProperties.HasMoreData);

            foreach (NamedProperty p in readProperties)
            {
                if (properties.ContainsKey(p.Metadata.PropertyName))
                {
                    VS.Assert.AreEqual(p.GetValue<string>(), properties[p.Metadata.PropertyName], "Property value is correct");
                }
                else
                {
                    VS.Assert.Fail("Unexpected property name {0}, value {1}", p.Metadata.PropertyName, p.GetValue<string>());
                }
            }

            LogHelper.Log("Successfully read properties for a name with values");

            // Enumerate properties without values (metadata only)
            PropertyEnumerationResult readPropertiesWithPrefix = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fabricClient.PropertyManager.EnumeratePropertiesAsync(nameUri, false, null);
            }, "EnumerateProperties");
            
            VS.Assert.AreEqual<int>(properties.Count, readPropertiesWithPrefix.Count());
            VS.Assert.IsFalse(readPropertiesWithPrefix.HasMoreData);

            foreach (NamedProperty p in readPropertiesWithPrefix)
            {
                if (properties.ContainsKey(p.Metadata.PropertyName))
                {
                    try
                    {
                        string propertyValue = p.GetValue<string>();
                        VS.Assert.Fail("Property should not contain value: name {0}, value {1}", p.Metadata.PropertyName, propertyValue);
                    }
                    catch (FabricException fe)
                    {
                        LogHelper.Log(string.Format(CultureInfo.InvariantCulture, "Expected Error: empty property value - {0}", fe.Message));
                        VS.Assert.IsTrue(fe.Message.Contains("not to retrieve"), "Exception expected");
                        VS.Assert.IsTrue(fe.ErrorCode == FabricErrorCode.PropertyValueEmpty, "Exception error code expected");
                    }
                }
                else
                {
                    VS.Assert.Fail("Unexpected property name {0}", p.Metadata.PropertyName);
                }
            }

            LogHelper.Log("Successfully read properties for a name without values");

            foreach (var entry in properties)
            {
                DeleteProperty(fabricClient, nameUri, entry.Key);
            }

            LogHelper.Log("Successfully deleted properties");

            FabricClientTest.deployment.DeleteName(nameUri);
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("aprameyr")]
        public void FabricClientTest_NamedPropertyMetadataSupport()
        {
            LogHelper.Log("Creating client");
            var client = FabricClientTest.CreateClient();

            LogHelper.Log("Creating entity");
            Uri parentEntity = new Uri("fabric:/foooooo");
            FabricClientTest.deployment.CreateName(parentEntity);

            LogHelper.Log("Testing support for long");
            FabricClientTest.NamedPropertyMetadataTestHelper<long>(client, parentEntity, "long", PropertyTypeId.Int64, sizeof(long), (mgr, name, parent) => mgr.PutPropertyAsync(parent, name, (long)123));

            LogHelper.Log("Testing support for guid");
            FabricClientTest.NamedPropertyMetadataTestHelper<Guid>(client, parentEntity, "guid", PropertyTypeId.Guid, Marshal.SizeOf(typeof(Guid)), (mgr, name, parent) => mgr.PutPropertyAsync(parent, name, Guid.NewGuid()));

            LogHelper.Log("Testing support for string");
            var stringValue = "abc";
            FabricClientTest.NamedPropertyMetadataTestHelper<string>(client, parentEntity, "string", PropertyTypeId.String, (stringValue.Length + 1) * 2, (mgr, name, parent) => mgr.PutPropertyAsync(parent, name, stringValue)); // sizeof(WCHAR) * (length + 1) [null-terminated]

            LogHelper.Log("Testing support for byte[]");
            var byteArrayValue = new byte[] { 0, 1, 2 };
            FabricClientTest.NamedPropertyMetadataTestHelper<byte[]>(client, parentEntity, "byte_array", PropertyTypeId.Binary, byteArrayValue.Length, (mgr, name, parent) => mgr.PutPropertyAsync(parent, name, byteArrayValue));

            LogHelper.Log("Testing support for double");
            FabricClientTest.NamedPropertyMetadataTestHelper<double>(client, parentEntity, "double", PropertyTypeId.Double, sizeof(double), (mgr, name, parent) => mgr.PutPropertyAsync(parent, name, (double)123.0));
        }

        [VS.TestMethod]
        [VS.TestProperty("ThreadingModel", "STA")]
        [VS.Owner("aprameyr")]
        public void FabricClientTest_BatchedChildrenReadSupport()
        {
            // create a name
            // create many subnames under that name
            // reading the subnames is batched - ensure that fabric client under the covers actually 
            // makes multiple calls and returns all the children

            const string nameUri = @"fabric:/batched_name";
            const string childPrefix  = "/c_";
            const int propertiesToCreate = 30;

            var client = FabricClientTest.CreateClient();

            // create the name            
            FabricClientTest.deployment.CreateName(new Uri(nameUri));

            // query the name
            FabricClientTest.deployment.HasName(new Uri(nameUri));

            // start creating properties
            List<Task> tasks = new List<Task>();
            List<Uri> childNames = new List<Uri>();
            for (int i = 10; i < propertiesToCreate; i++)
            {
                Uri childUri = new Uri(nameUri + childPrefix + i.ToString());
                childNames.Add(childUri);
                tasks.Add(Task.Factory.StartNew(() =>
                {
                    FabricClientTest.deployment.CreateName(childUri);
                }));
            }

            Task.WaitAll(tasks.ToArray());

            List<Uri> results = new List<Uri>();
            NameEnumerationResult result = null;
            bool hasMoreData = false;

            do
            {
                // enumerate the subnames
                result = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
                {
                    return client.PropertyManager.EnumerateSubNamesAsync(new Uri(nameUri), result, true);
                }, "EnumerateSubNames");
                results.AddRange(result);
                hasMoreData = result.HasMoreData;
            } while (hasMoreData);

            MiscUtility.CompareEnumerables(childNames, results);
        }

        private static bool AreConfigLoadedSettingsEqual(FabricClientSettings left, FabricClientSettings right)
        {
            if (left == null) { return right == null; }

            if (right == null) { return false; }

            if (left.PartitionLocationCacheLimit != right.PartitionLocationCacheLimit ||
                left.ServiceChangePollInterval != right.ServiceChangePollInterval ||
                left.ConnectionInitializationTimeout != right.ConnectionInitializationTimeout ||
                left.HealthReportSendInterval != right.HealthReportSendInterval ||
                left.HealthOperationTimeout != right.HealthOperationTimeout ||
                left.KeepAliveInterval != right.KeepAliveInterval ||
                left.ConnectionIdleTimeout != right.ConnectionIdleTimeout)
            {
                return false;
            }

            return true;
        }

        private static bool AreSettingsEqual(FabricClientSettings left, FabricClientSettings right)
        {
            if (!AreConfigLoadedSettingsEqual(left, right))
            {
                return false;
            }
            
            if (left.ClientFriendlyName != right.ClientFriendlyName)
            {
                LogHelper.Log("ClientFriendlyName mismatch: {0} != {1}", left.ClientFriendlyName != null ? left.ClientFriendlyName : "null", right.ClientFriendlyName != null ? right.ClientFriendlyName : "null");
                return false;
            }

            if (left.HealthReportRetrySendInterval != right.HealthReportRetrySendInterval)
            {
                LogHelper.Log("HealthReportRetrySendInterval mismatch: {0} != {1}", left.HealthReportRetrySendInterval, right.HealthReportRetrySendInterval);
                return false;
            }

            if (left.PartitionLocationCacheBucketCount != right.PartitionLocationCacheBucketCount)
            {
                LogHelper.Log("PartitionLocationCacheBucketCount mismatch: {0} != {1}", left.PartitionLocationCacheBucketCount, right.PartitionLocationCacheBucketCount);
                return false;
            }

            return true;
        }

        private static void NamedPropertyMetadataTestHelper<T>(
            FabricClient client, 
            Uri parent, 
            string propertyName, 
            PropertyTypeId typeIdExpected, 
            int valueSizeExpected, 
            Func<FabricClient.PropertyManagementClient, string, Uri, Task> creationFunc)
        {
            DateTime lastModifiedTimeMinUTC = DateTime.UtcNow;

            Thread.Sleep(100);
            
            // create the property
            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return creationFunc(client.PropertyManager, propertyName, parent);
            }, "CreateProperty");

            // read the metadata
            var metadata = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return client.PropertyManager.GetPropertyMetadataAsync(parent, propertyName);
            }, "GetPropertyMetadata");

            LogHelper.Log("Creation time: {0}", lastModifiedTimeMinUTC.Ticks);
            LogHelper.Log("Modified time: {0}", metadata.LastModifiedUtc.Ticks);
            VS.Assert.AreEqual<PropertyTypeId>(typeIdExpected, metadata.TypeId, "typeid expected");
            VS.Assert.AreEqual<int>(valueSizeExpected, metadata.ValueSize, "value size");
            VS.Assert.IsTrue(metadata.SequenceNumber >= 0, "Sequence number");
            VS.Assert.IsTrue(metadata.LastModifiedUtc.Ticks > lastModifiedTimeMinUTC.Ticks, "time");
            VS.Assert.AreEqual<Uri>(parent, metadata.Parent);
        }

        private static FabricClient CreateClient()
        {
            return new FabricClient(FabricClientTest.deployment.ClientConnectionAddresses.Select(e => e.ToString()).ToArray());
        }

        private static void ExpectException(Type expectedException, string messageContent, Action action)
        {
            bool exceptionThrown = true;
            try
            {
                action();
                exceptionThrown = false;
            }
            catch (Exception ex)
            {
                Exception exceptionToEvaluate = ex;
                if (ex is AggregateException)
                {
                    exceptionToEvaluate = ((AggregateException)ex).InnerExceptions[0];
                    while (exceptionToEvaluate is AggregateException)
                    {
                        exceptionToEvaluate = ((AggregateException)exceptionToEvaluate).InnerExceptions[0];
                    }
                }

                if (exceptionToEvaluate.GetType() != expectedException)
                {
                    VS.Assert.Fail("Expected exception of type {0} but got {1}", expectedException, exceptionToEvaluate);
                }

                if (!string.IsNullOrWhiteSpace(messageContent))
                {
                    VS.Assert.IsTrue(exceptionToEvaluate.Message.ToUpperInvariant().Contains(messageContent.ToUpperInvariant()),
                        string.Format(CultureInfo.InvariantCulture, "Expected message to contain [{0}]. But it says [{1}]", messageContent, exceptionToEvaluate.Message));
                }
            }

            if (!exceptionThrown)
            {
                VS.Assert.Fail("Expected exception of type {0} but no exception was thrown", expectedException);
            }
        }
        
        private static void CompareStrings(string expectedValue, string actualValue, StringComparison comparisonMode = StringComparison.OrdinalIgnoreCase)
        {
            VS.Assert.IsTrue(
                string.Compare(expectedValue, actualValue, comparisonMode) == 0,
                string.Format("Expected '{0}' and got '{1}'", expectedValue, actualValue));
        }

        private static void CompareBytes(byte[] expectedValue, byte[] actualValue)
        {
            VS.Assert.AreEqual(expectedValue.Length, actualValue.Length);
            for (int i = 0; i < expectedValue.Length; i++)
            {
                VS.Assert.AreEqual(expectedValue[i], actualValue[i]);
            }
        }

        private static void CreateReadAndDeleteProperty<T>(FabricClient fc, Uri parent, string propertyName, T value, Func<FabricClient.PropertyManagementClient, string, T, Uri, Task> factory, Action<T> validator = null)
        {
            // var namedPropertyExpected = factory(propertyName, value, entity);
            factory(fc.PropertyManager, propertyName, value, parent).Wait(taskTimeout);

            var namedPropertyActual = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fc.PropertyManager.GetPropertyAsync(parent, propertyName);
            }, "GetProperty");

            if (validator == null)
            {
                VS.Assert.AreEqual<T>(value, namedPropertyActual.GetValue<T>());
            }
            else
            {
                validator(namedPropertyActual.GetValue<T>());
            }

            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fc.PropertyManager.DeletePropertyAsync(parent, propertyName);
            }, "DeleteProperty");
        }

        private static void CreateProperty(FabricClient fc, Uri parentUri, string propertyName, string value)
        {
            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fc.PropertyManager.PutPropertyAsync(parentUri, propertyName, value);
            }, "PutProperty");

            //// Can recreate the property
            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fc.PropertyManager.PutPropertyAsync(parentUri, propertyName, value);
            }, "PutProperty");

            NamedProperty retrievedProperty = FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fc.PropertyManager.GetPropertyAsync(parentUri, propertyName);
            }, "GetProperty");

            CompareStrings(propertyName, retrievedProperty.Metadata.PropertyName);
            CompareStrings(value, retrievedProperty.GetValue<string>());
        }

        private static void DeleteProperty(FabricClient fc, Uri parent, string propertyName)
        {
            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return fc.PropertyManager.DeletePropertyAsync(parent, propertyName);
            }, "DeleteProperty");

            TestUtility.ExpectExceptionAndMessage<FabricElementNotFoundException>(
                () => fc.PropertyManager.GetPropertyAsync(parent, propertyName),
                "GetProperty",
                "does not exist");
        }
    }
}