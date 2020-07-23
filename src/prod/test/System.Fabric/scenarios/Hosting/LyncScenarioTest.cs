// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Scenarios.Test.Hosting
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Test;
    using System.Fabric.Test.Common.FabricUtility;
    using System.IO;
    using System.IO.Pipes;
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    class LyncScenarioTestHelper
    {
        private const string HostExeName = "System.Fabric.Test.Host.exe";
        private static TimeSpan processWaitTimeout = TimeSpan.FromMinutes(2);
        private static bool isDebugMode = false;

        public class LyncHostInfo : IDisposable
        {
            private enum HostRequest : byte
            {
                Stop = 0,
                Query = 1,
            }

            [Flags]
            private enum HostResponse : byte
            {
                Success = 0,
                Error = 1,
                FabricHasExited = 2,
                ActivationContextIsAvailable = 4,
            }

            private readonly string pipeServerAddress = Guid.NewGuid().ToString();
            private readonly NamedPipeServerStream pipe;
            private readonly IEnumerable<Type> services;
            private readonly FabricNode node;
            private readonly bool enableRecreateFabricRuntimeOnCrash;
            private Process process;

            public LyncHostInfo(FabricNode node, IEnumerable<Type> services, bool enableRecreateFabricRuntimeOnCrash)
            {
                this.node = node;
                this.services = services;
                this.pipe = new NamedPipeServerStream(
                    this.pipeServerAddress,
                    PipeDirection.InOut,
                    1 /* max number of server instances */,
                    PipeTransmissionMode.Byte,
                    PipeOptions.Asynchronous);

                this.enableRecreateFabricRuntimeOnCrash = enableRecreateFabricRuntimeOnCrash;
            }

            public Process Process
            {
                get
                {
                    return this.process;
                }
            }

            public void StartProcess(string outputFolderPath, string uniqueId)
            {
                string args = string.Format("adhoc \"{0}\"", this.pipeServerAddress);

                args += " \"" + Path.Combine(outputFolderPath, uniqueId) + "\"";

                LogHelper.Log("LyncHost - output file is {0}", args);

                foreach (var item in this.services)
                {
                    args += " ";
                    args += "\"";
                    args += item.AssemblyQualifiedName;
                    args += "\"";
                }

                if (LyncScenarioTestHelper.isDebugMode)
                {
                    args += " -debug";
                }

                if (this.enableRecreateFabricRuntimeOnCrash)
                {
                    args += " -recreateOnCrash";
                }

                var psi = new ProcessStartInfo
                {
                    FileName = Path.Combine(this.node.NodeRootPath, LyncScenarioTestHelper.HostExeName),
                    Arguments = args,
                    WorkingDirectory = this.node.BinariesPath,
                    UseShellExecute = false,
                    RedirectStandardInput = true,
                };

                psi.EnvironmentVariables["FabricConfigFileName"] = this.node.ConfigPath;

                // Start waiting for connection before client starts to ensure we are ready
                // to accept one when the client tries
                IAsyncResult asyncResult = this.pipe.BeginWaitForConnection(null /* callback */, null /* state */);

                this.process = Process.Start(psi);
                LogHelper.Log("Process started. PID {0}", this.process.Id);

                // wait for the connection with a timeout to ensure if client process died we do not wait for ever
                LogHelper.Log("Waiting for connection");
                bool connectionMade = asyncResult.AsyncWaitHandle.WaitOne(processWaitTimeout);

                if (!connectionMade)
                {
                    // force close
                    this.process.Kill();
                    this.process = null;
                    throw new TimeoutException("Client did not connect");
                }

                this.pipe.EndWaitForConnection(asyncResult);
                asyncResult.AsyncWaitHandle.Dispose();
                LogHelper.Log("Connected");
            }

            public void StopProcess()
            {
                if (this.process == null)
                {
                    return;
                }

                try
                {
                    LogHelper.Log("StopHost");
                    this.SendCommandAndHandleResponse(HostRequest.Stop);
                    this.pipe.Close();
                    Assert.IsTrue(this.process.WaitForExit((int)processWaitTimeout.TotalMilliseconds));
                    Assert.AreEqual<int>(0, this.process.ExitCode);
                    this.process = null;
                }
                finally
                {
                    if (this.process != null)
                    {
                        // force close
                        this.process.Kill();
                        this.process = null;
                    }
                }
            }

            public bool QueryHasFabricExited()
            {
                byte resp = this.SendCommandAndHandleResponse(HostRequest.Query);

                bool rv = (resp & (byte)HostResponse.FabricHasExited) != 0;
                LogHelper.Log("PID {0} - QueryHasFabricExited returning {1}", this.process.Id, rv);
                return rv;
            }

            public bool QueryIsActivationContextAvailable()
            {
                byte resp = this.SendCommandAndHandleResponse(HostRequest.Query);

                bool rv = (resp & (byte)HostResponse.ActivationContextIsAvailable) != 0;
                LogHelper.Log("PID {0} - IsActivationContextAvailable returning {1}", this.process.Id, rv);
                return rv;
            }

            private byte SendCommandAndHandleResponse(HostRequest command)
            {
                LogHelper.Log("PID {0} - Sending command: {1}", this.process.Id, command);
                this.pipe.WriteByte((byte)command);
                byte response = (byte)this.pipe.ReadByte();

                LogHelper.Log("PID {0} - Received response: {1}", this.process.Id, (HostResponse)response);
                Assert.IsTrue((response & (byte)HostResponse.Error) == 0);

                return response;
            }


            #region IDisposable Members

            public void Dispose()
            {
                this.StopProcess();
            }

            #endregion
        }

        public static FabricDeployment CreateDeployment(string testcaseName)
        {
            var parameters = FabricDeploymentParameters.CreateDefault(Constants.TestDllName, testcaseName, 1);
            string fabricFilePath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);

            // add the host related files into the node folder for each node so that we can start the host
            parameters.AdditionalFilesToDeployToNodeRoot.Add(Path.Combine(fabricFilePath, "System.Fabric.dll"));
            parameters.AdditionalFilesToDeployToNodeRoot.Add(Path.Combine(fabricFilePath, "System.Fabric.Strings.dll"));
            parameters.AdditionalFilesToDeployToNodeRoot.Add(Path.Combine(fabricFilePath, "System.Fabric.Test.Host.exe"));
            parameters.AdditionalFilesToDeployToNodeRoot.Add(Assembly.GetExecutingAssembly().Location);

            var deployment = new FabricDeployment(parameters);

            LogHelper.Log("Starting deployment");
            deployment.Start();

            return deployment;
        }
    }

    public class ServiceImplementationHelper
    {
        public const string OpenMarker = "open";
        public const string CloseMarker = "close";

        private static void AppendTextToFile(byte[] initializationData, long id, string text)
        {
            string filePath = Encoding.UTF8.GetString(initializationData);
            File.AppendAllLines(Path.Combine(filePath, id.ToString() + ".txt"), new string[] { text });
        }

        public static void HandleOpen(byte[] initializationData, long id)
        {
            ServiceImplementationHelper.AppendTextToFile(initializationData, id, ServiceImplementationHelper.OpenMarker);
        }

        public static void HandleClose(byte[] initializationData, long id)
        {
            ServiceImplementationHelper.AppendTextToFile(initializationData, id, ServiceImplementationHelper.CloseMarker);
        }
    }

    public class LyncStatelessService : IStatelessServiceInstance
    {
        private ServiceInitializationParameters initParams;
        private Guid partitionId;
        private long instanceId;

        public Threading.Tasks.Task CloseAsync(CancellationToken cancellationToken)
        {
            ServiceImplementationHelper.HandleClose(this.initParams.InitializationData, this.instanceId);
            return Task.Factory.StartNew(() => { ; });
        }

        public void Initialize(StatelessServiceInitializationParameters initializationParameters)
        {
            this.initParams = initializationParameters;
            this.partitionId = initializationParameters.PartitionId;
            this.instanceId = initializationParameters.InstanceId;
        }

        public Threading.Tasks.Task<string> OpenAsync(IStatelessServicePartition partition, CancellationToken cancellationToken)
        {
            ServiceImplementationHelper.HandleOpen(this.initParams.InitializationData, this.instanceId);
            return Task.Factory.StartNew<string>(() => "http://localhost:80");
        }

        public void Abort()
        {
            ServiceImplementationHelper.HandleClose(this.initParams.InitializationData, this.instanceId);
        }
    }

    public class LyncStatefulService : IStatefulServiceReplica, IStateProvider
    {
        private ServiceInitializationParameters initParams;
        private Guid partitionId;
        private long replicaId;

        public Task<string> ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew<string>(() => "");
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() => ServiceImplementationHelper.HandleClose(this.initParams.InitializationData, this.replicaId)); ;
        }

        public void Abort()
        {
            ServiceImplementationHelper.HandleClose(this.initParams.InitializationData, this.replicaId);
        }

        public void Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            this.initParams = initializationParameters;
            this.partitionId = initializationParameters.PartitionId;
            this.replicaId = initializationParameters.ReplicaId;
        }

        public Task<IReplicator> OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew<IReplicator>(() =>
                {
                    ServiceImplementationHelper.HandleOpen(this.initParams.InitializationData, this.replicaId);
                    return partition.CreateReplicator(this, null);
                });
        }

        public long GetLastCommittedSequenceNumber()
        {
            return -1;
        }

        public IOperationDataStream GetCopyContext()
        {
            return null;
        }

        public IOperationDataStream GetCopyState(long sequenceNumber, IOperationDataStream copyContext)
        {
            return new AsyncEnumOpData();
        }

        public Task<bool> OnDataLossAsync(CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() => false);
        }

        public Task UpdateEpochAsync(Epoch epoch, long previousEpochLastSequenceNumber, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() => { ;});
        }

        class AsyncEnumOpData : IOperationDataStream
        {
            public Task<OperationData> GetNextAsync(CancellationToken cancellationToken)
            {
                return Task.Factory.StartNew<OperationData>(() => null);
            }
        }
    }

    [TestClass]
    public class Lync_ActivationDeactivationTest
    {
        private static FabricDeployment deployment;

        [ClassInitialize]
        public static void Initialize(TestContext context)
        {
            Lync_ActivationDeactivationTest.deployment = LyncScenarioTestHelper.CreateDeployment("Lync_ActivationDeactivation");
        }

        [ClassCleanup]
        public static void Cleanup()
        {
            Lync_ActivationDeactivationTest.deployment.Dispose();

            // To validate no tasks are left behind with unhandled exceptions
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        private void TestHelper(Type serviceImplementationType, Func<ServiceDescription> serviceDescriptionFactory)
        {
            // create a fabric deployment (happens in the class Init/CLeanup)
            // define a folder where the service will write data
            // start the host exe
            // create the service and pass folder via init data (the service in its OnOpen will write "open" to a text file in the folder and "close" in OnClose)
            // wait till the output folder contains some data (i.e. service starts up)
            // delete the service
            // wait till output folder contains the close line
            // tell the host to unregister and stop
            // validate that no error occurred and host terminated gracefully

            // this is the path where the service will write its code
            string serviceOutputPath = Path.Combine(Lync_ActivationDeactivationTest.deployment.TestOutputPath, serviceImplementationType.Name);
            FileUtility.CreateDirectoryIfNotExists(serviceOutputPath);

            // start the host
            // we know that the cluster has one node so just put the host on the first node
            LogHelper.Log("Starting host");
            using (var lyncHost = new LyncScenarioTestHelper.LyncHostInfo(deployment.Nodes.First(), new Type[] { serviceImplementationType }, false))
            {
                lyncHost.StartProcess(Lync_ActivationDeactivationTest.deployment.TestOutputPath, serviceImplementationType.Name + "_host.txt");

                // create the service
                FabricClient client = new FabricClient(deployment.ClientConnectionAddresses.Select(e => e.ToString()).ToArray());

                var sd = serviceDescriptionFactory();
                sd.InitializationData = Encoding.UTF8.GetBytes(serviceOutputPath);
                sd.ServiceName = new Uri(string.Format("fabric:/my_app/{0}", serviceImplementationType.Name));
                sd.ServiceTypeName = serviceImplementationType.Name;
                sd.PartitionSchemeDescription = new SingletonPartitionSchemeDescription();

                LogHelper.Log("Creating service");
                deployment.CreateService(sd);

                // wait for the service to come up
                // when the service comes up it just writes some content to a text file
                LogHelper.Log("Waiting for service to start");
                Assert.IsTrue(MiscUtility.WaitUntil(() => Directory.GetFiles(serviceOutputPath).Length > 0, TimeSpan.FromSeconds(120)));

                // delete the service
                deployment.DeleteService(sd.ServiceName);

                // wait for the service to get deleted
                // when closeAsync is called it writes some text to a text file
                string[] contents = null;
                Assert.IsTrue(
                    MiscUtility.WaitUntil(
                        () =>
                        {
                            return FileUtility.TryReadAllLines(Directory.GetFiles(serviceOutputPath)[0], out contents)
                            && contents.Length == 2;
                        },
                        TimeSpan.FromSeconds(120),
                        250,
                        true));

                // the activation context should not be available
                Assert.IsFalse(lyncHost.QueryIsActivationContextAvailable());

                // kill the host
                lyncHost.StopProcess();

                // validate that the correct data was written to the file
                Assert.AreEqual(ServiceImplementationHelper.OpenMarker, contents[0]);
                Assert.AreEqual(ServiceImplementationHelper.CloseMarker, contents[1]);
            }
        }

        [TestMethod]
        [Owner("aprameyr")]
        public void Lync_ActivationDeactivation_Stateless()
        {
            this.TestHelper(typeof(LyncStatelessService), () =>
            {
                return new StatelessServiceDescription { InstanceCount = 1 };
            });
        }

        [TestMethod]
        [Owner("aprameyr")]
        public void Lync_ActivationDeactivation_Stateful()
        {
            this.TestHelper(typeof(LyncStatefulService), () =>
            {
                return new StatefulServiceDescription();
            });
        }
    }

    [TestClass]
    public class Lync_FabricExitedNotificationTest
    {
        [TestMethod]
        [Owner("aprameyr")]
        public void Lync_FabricExitedNotification()
        {
            const int hostCount = 10;
            bool hasFabricExited = false;
            using (var deployment = LyncScenarioTestHelper.CreateDeployment("Lync_FabricExitedNotification"))
            {
                var lyncHostList = new List<LyncScenarioTestHelper.LyncHostInfo>();
                for (int i = 0; i < hostCount; i++)
                {
                    lyncHostList.Add(new LyncScenarioTestHelper.LyncHostInfo(deployment.Nodes.First(), Enumerable.Empty<Type>(), false));
                }

                // let the host come up
                LogHelper.Log("Starting processes");
                for (int i = 0; i < hostCount; i++)
                {
                    lyncHostList[i].StartProcess(deployment.TestOutputPath, "exitnotification_" + i.ToString());
                }

                // kill the fabric node
                LogHelper.Log("Killing fabric.exe");
                deployment.Nodes.First().Dispose();

                // assert that the host received the notification
                MiscUtility.WaitUntil(() =>
                {
                    LogHelper.Log("Querying host");
                    hasFabricExited = Enumerable.All(lyncHostList, e => e.QueryHasFabricExited());
                    return hasFabricExited;
                }, TimeSpan.FromSeconds(5), 200);

                LogHelper.Log("Stopping Host");
                lyncHostList.ForEach(e => e.StopProcess());

                LogHelper.Log("Dispose");
                lyncHostList.ForEach(e => e.Dispose());
            }

            Assert.IsTrue(hasFabricExited);
        }

        [TestMethod]
        [Owner("aprameyr")]
        public void Lync_FabricExitedHandlerShouldNotCrash()
        {
            using (var deployment = LyncScenarioTestHelper.CreateDeployment("Lync_FabricExitedCrashTest"))
            {
                using (var hostInfo = new LyncScenarioTestHelper.LyncHostInfo(deployment.Nodes.First(), Enumerable.Empty<Type>(), true))
                {
                    // let the host come up
                    LogHelper.Log("Starting processes");
                    hostInfo.StartProcess(deployment.TestOutputPath, "exitnotification_crash");

                    // kill the fabric node
                    LogHelper.Log("Killing fabric.exe");
                    deployment.Nodes.First().Dispose();

                    // assert that the host received the notification
                    Assert.IsTrue(MiscUtility.WaitUntil(() => hostInfo.QueryHasFabricExited(), TimeSpan.FromSeconds(5), 200));

                    // wait for a little while
                    Thread.Sleep(2000);

                    // assert that the host is still up and has not crashed
                    Assert.IsFalse(hostInfo.Process.HasExited);

                    LogHelper.Log("Stopping Host");
                    hostInfo.StopProcess();
                }
            }
        }
    }

}
