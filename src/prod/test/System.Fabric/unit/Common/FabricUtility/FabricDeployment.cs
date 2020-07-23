// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.FabricUtility
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Diagnostics;
    using System.Fabric.Description;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Management.ServiceModel;
    using System.Xml;
    using System.Xml.Serialization;

    public class FabricDeployment : IDisposable
    {
        [Flags]
        public enum ServiceState
        {
            Unknown = 0,
            Primary = 1,
            Secondary = 2,
            Stateless = 4,
        }

        private const string ImageStoreIncomingFolderName = "Incoming";
        private static TimeSpan processWaitTimeout = TimeSpan.FromMinutes(2);
        private const int PortsUsedPerNode = 50;        

        private readonly FabricDeploymentParameters parameters;
        private readonly List<FabricNode> nodes = new List<FabricNode>();
        private readonly ProcessMonitor processMonitor = new ProcessMonitor(new string[] { "fabric", "system.fabric.test.host" });
        private readonly Dictionary<string, ResolvedServicePartition> previousResolveResults = new Dictionary<string, ResolvedServicePartition>();

        private FabricClient client;
        private ServiceStateCache serviceStateCache;
        FabricHost fabricHost;

        public FabricDeployment(FabricDeploymentParameters parameters)
        {
            Debug.Assert(parameters != null);
            Debug.Assert(!string.IsNullOrWhiteSpace(parameters.DeploymentRootPath));
            Debug.Assert(parameters.NodeCount > 0);
            Debug.Assert(parameters.StartPort > 0);

            this.parameters = parameters;

            LogManUtil.StopTracing();
            ProcessUtility.KillRunningFabricExe();
            FileUtility.RecursiveDelete(this.parameters.DeploymentRootPath);
            FileUtility.CreateDirectoryIfNotExists(this.parameters.DeploymentRootPath);
            FileUtility.CreateDirectoryIfNotExists(this.TestOutputPath);

            var addresses = this.GenerateAllNodeRuntimeAddresses();
            var voters = addresses.Count() == 1 ? addresses : addresses.Take(addresses.Count() / 2);
                       
            int nodeIdStart = 1;
            using (new LogHelper("FabricDeployment.Ctor"))
            {
                LogHelper.Log("ImageStorePath: {0}", this.ImageStoreRootPath);
                FileUtility.CreateDirectoryIfNotExists(this.ImageStoreRootPath);
                FileUtility.CreateDirectoryIfNotExists(this.ImageStoreIncomingPath);

                // deploy image store
                LogHelper.Log("Deploying image store");
                this.DeployImageStoreClient();

                // creaet logs
                LogHelper.Log("Log folder {0}", this.LogPath);
                FileUtility.CreateDirectoryIfNotExists(this.LogPath);

                for (int i = 0; i < this.parameters.NodeCount; i++)
                {
                    string nodeId = (nodeIdStart + i).ToString();
                    string nodeName = "nodeid:" + nodeId;
                    string nodeRootPath = Path.Combine(this.parameters.DeploymentRootPath, "Node" + nodeId);
                    LogHelper.Log("Creating: {0}", nodeId);

                    var nodeParameters = new FabricNodeParameters
                    {
                        NodeId = nodeId,
                        NodeName = nodeName,
                        NodeRootPath = nodeRootPath,

                        ImageStoreConnectionString = "file:" + this.ImageStoreRootPath,

                        Voters = voters,
                        Address = addresses.ElementAt(i),
                        StartPort = addresses.ElementAt(i).Port,
                        EndPort = addresses.ElementAt(i).Port + FabricDeployment.PortsUsedPerNode - 1, // subtract one because the endPOrt is inclusieve
                        FaultDomainId = new Uri(string.Format("fd:/fd_{0}", nodeId)),
                        UpgradeDomainId = string.Format("UD_{0}", nodeId)
                    };

                    var node = new FabricNode(nodeParameters);
                    node.DeployFabric(this.GetFabricBinaries(), this.parameters.AdditionalFilesToDeployToNodeRoot, this.GetFabricDirectories());
                    this.nodes.Add(new FabricNode(nodeParameters));
                }
                if (nodes.Count > 0)
                {
                    string activatorAddress = String.Format("localhost:{0}", this.parameters.StartPort);
                    this.fabricHost = new FabricHost(this.parameters.FabricBinariesPath,
                        this.parameters.DeploymentRootPath,
                        activatorAddress,
                        this.nodes[0].BinariesPath);
                    this.fabricHost.DeployFabricHost();
                }
            }
        }

        public string ImageStoreRootPath
        {
            get
            {
                return Path.Combine(this.parameters.DeploymentRootPath, "ImageStore");
            }
        }

        public IEnumerable<FabricNode> Nodes
        {
            get
            {
                return this.nodes;
            }
        }

        /// <summary>
        /// A folder under deployment root which will not be cleaned up
        /// </summary>
        public string TestOutputPath
        {
            get
            {
                return Path.Combine(this.LogPath, "TestOutput");
            }
        }

        public string ApplicationStagingPath
        {
            get
            {
                return Path.Combine(this.parameters.DeploymentRootPath, "ApplicationStaging");
            }
        }

        public string LogPath
        {
            get
            {
                return Path.Combine(this.parameters.DeploymentRootPath, "Logs");
            }
        }

        public string ImageStoreIncomingPath
        {
            get
            {
                return Path.Combine(this.ImageStoreRootPath, FabricDeployment.ImageStoreIncomingFolderName);
            }
        }

        public string ImageStoreClientPath
        {
            get
            {
                return Path.Combine(this.parameters.DeploymentRootPath, "imagestoreclient.exe");
            }
        }

        public IPEndPoint[] ClientConnectionAddresses
        {
            get
            {
                return this.nodes.Select(e => e.ClientConnectionAddress).ToArray();
            }
        }

        public FabricClient FabricClient
        {
            get
            {
                return this.client;
            }
        }

        public void Start()
        {
            Debug.Assert(this.nodes.Count != 0);

            using (new LogHelper("FabricDeployment.Start"))
            {
                for (var ix=1; ix<=this.nodes.Count; ++ix)
                {
                    var logDir = Path.Combine(this.LogPath, "Node" + ix);

                    if (!Directory.Exists(logDir))
                    {
                        LogHelper.Log("Creating directory: {0}", logDir);
                        Directory.CreateDirectory(logDir);
                    }
                }

                // this is incase an earlier run terminated without stopping the session
                LogHelper.Log("Stopping trace");
                LogManUtil.StopTracing();
                ProcessUtility.KillRunningFabricExe();

                LogHelper.Log("Start trace");
                LogManUtil.StartTracing(this.LogPath);

                this.fabricHost.Start();
                //Sleep after starting FabricHost to provide time for IPCServer to be ready
                Thread.Sleep(5000);
                foreach (var node in this.nodes)
                {
                    node.StartNode();
                }
                LogHelper.Log("All nodes up");
                foreach (var node in this.nodes)
                {
                    if (!node.IsFabricRunning)
                    {
                        LogHelper.Log("Node is not up: {0}", node.NodeId);
                    }
                    else
                    {
                        LogHelper.Log("Node {0} has PID {1}", node.NodeId, node.Pid);
                    }
                }

                if (this.nodes.Any((n) => !n.IsFabricRunning))
                {
                    throw new InvalidOperationException("A fabric node is not up");
                }
             
                var clientSettings = new FabricClientSettings() { ServiceChangePollInterval = TimeSpan.FromSeconds(5) };

                this.client = new FabricClient(clientSettings, this.nodes.Select(e => e.ClientConnectionAddress.ToString()).ToArray());

                if (this.parameters.EnableServiceStateCache)
                {
                    this.serviceStateCache = new ServiceStateCache(this.client);
                }

                // try to create the fabric test client and see if the CM is up yet                
                bool isClusterManagerUp = false;
                FabricDeployment.InvokeInMTA(() =>
                    {
                        FabricTestClient testClient = new FabricTestClient(this.client);
                        Guid clusterManagerGuid = new Guid("00000000-0000-0000-0000-000000002000");

                        isClusterManagerUp = MiscUtility.WaitUntil(() =>
                        {
                            var partitionTask = testClient.ResolvePartitionAsync(clusterManagerGuid, TimeSpan.FromSeconds(30));
                            partitionTask.Wait();

                            var partition = partitionTask.Result;

                            LogHelper.Log("FabricTestClient uses gateway {0}", testClient.GetCurrentGatewayAddress());

                            unsafe
                            {
                                var obj = (System.Fabric.Interop.NativeTypes.FABRIC_RESOLVED_SERVICE_PARTITION*)partition.get_Partition();
                                return obj->EndpointCount == 1;
                            }                            
                        }, TimeSpan.FromSeconds(120), 5000, true);
                    });

                if (!isClusterManagerUp)
                {
                    throw new TimeoutException("Cluster manager not started");
                }
            }
        }

        public void UploadProvisionAndStartApplication(ApplicationDeployer deployer)
        {
            using (new LogHelper("UploadProvisionAndStart for: " + deployer.Name))
            {
                LogHelper.Log("Packaging");
                string appDeploymentDir = Path.Combine(this.ApplicationStagingPath, deployer.Name);
                FileUtility.CreateDirectoryIfNotExists(appDeploymentDir);
                deployer.Deploy(appDeploymentDir);

                LogHelper.Log("Uploading");
                this.UploadApplication(appDeploymentDir, deployer.Name);

                LogHelper.Log("Provisioning");
                this.ProvisionApplication(deployer.Name, deployer.Version);

                LogHelper.Log("Creating");
                this.CreateApplication(deployer.Name, deployer.Version, deployer.Parameters);
            }
        }

        public void UploadApplication(string applicationPackagePath, string applicationName)
        {
            // sample command line - Upload -c "file:C:\WindowsFabric\work\ImageStoreRoot\incoming" -x "CalculatorApp" -l "G:\winfab_cashmere\WindowsFabric\sdk\samples\managed\base\bin\FabricManagementConsole\SampleDeploymentPackages\CalculatorApp" -g AtomicCopy
            LogHelper.Log("Loading from {0}", applicationPackagePath);

            string applicationUriInImageStore = "file:" + this.ImageStoreIncomingPath;
            string imageStoreFmt = "Upload -c \"{0}\" -x \"{1}\" -l \"{2}\" -g AtomicCopy";

            string imageStoreArgs = string.Format(imageStoreFmt, applicationUriInImageStore, applicationName, applicationPackagePath);

            LogHelper.Log("Command line args: " + imageStoreArgs);

            using (Process p = new Process())
            {
                var log = new System.Text.StringBuilder();

                p.StartInfo.FileName = this.ImageStoreClientPath;
                p.StartInfo.Arguments = imageStoreArgs;
                p.StartInfo.UseShellExecute = false;
                p.StartInfo.RedirectStandardError = true;
                p.StartInfo.RedirectStandardOutput = true;

                p.ErrorDataReceived += (s, e) =>
                {
                    if (e.Data != null)
                    {
                        log.AppendLine("E: " + e.Data);
                    }
                };

                p.OutputDataReceived += (s, e) =>
                {
                    if (e.Data != null)
                    {
                        log.AppendLine(e.Data);
                    }
                };

                p.Start();

                p.BeginErrorReadLine();
                p.BeginOutputReadLine();

                LogHelper.Log("Started process");
                if (!p.WaitForExit((int)processWaitTimeout.TotalMilliseconds))
                {
                    LogHelper.Log("Wait failed");
                    throw new InvalidOperationException("Wait for imagestoreclient failed");
                }

                LogHelper.Log("ExitCode: {0}", p.ExitCode);
                if (p.ExitCode != 0)
                {
                    LogHelper.Log(log.ToString());
                    throw new InvalidOperationException("ImageStoreClientUpload Failed");
                }
            }
        }

        public void ProvisionApplication(string applicationName, string applicationVersion)
        {
            // provision the application type into cluster
            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                string appPath = FabricDeployment.ImageStoreIncomingFolderName + @"\" + applicationName;
                return this.client.ApplicationManager.ProvisionApplicationAsync(appPath, TimeSpan.FromSeconds(240), CancellationToken.None);
            },
            "ProvisionApplication",
            FabricErrorCode.ApplicationAlreadyExists);
        }

        public void UpgradeApplication(string applicationName, string applicationVersion, int getStatusRetryCount)
        {
            LogHelper.Log("UpgradeApplication {0}, Version={1}", applicationName, applicationVersion);

            FabricDeployment.InvokeInMTA(() =>
            {
                ApplicationUpgradeDescription description = new ApplicationUpgradeDescription
                {
                    ApplicationName = new Uri("fabric:/" + applicationName),
                    TargetApplicationTypeVersion = applicationVersion,
                    UpgradePolicyDescription = new RollingUpgradePolicyDescription
                    {
                        ForceRestart = false,
                        UpgradeMode = RollingUpgradeMode.UnmonitoredAuto,
                        UpgradeReplicaSetCheckTimeout = TimeSpan.FromSeconds(100)
                    }
                };

                Task upgrade = this.client.ApplicationManager.UpgradeApplicationAsync(description);
                upgrade.Wait();

                do
                {
                    LogHelper.Log("UpgradeApplication - Getting UpgradeState");
                    var status = this.client.ApplicationManager.GetApplicationUpgradeProgressAsync(description.ApplicationName);
                    status.Wait();

                    LogHelper.Log("UpgradeApplication - UpgradeState = {0}", status.Result.UpgradeState);
                    if (status.Result.UpgradeState == ApplicationUpgradeState.RollingForwardCompleted)
                    {
                        break;
                    }
                    else
                    {
                        getStatusRetryCount--;
                        Thread.Sleep(2000);
                    }
                }
                while (getStatusRetryCount > 0);
            });
        }

        public void CreateApplication(string applicationName, string applicationVersion)
        {
            this.CreateApplication(applicationName, applicationVersion, new Dictionary<string, string>());
        }

        public void CreateApplication(string applicationName, string applicationVersion, IDictionary<string,string> applicationParameters)
        {
            NameValueCollection nameValueCollection = new NameValueCollection(applicationParameters.Count);
            foreach (var item in applicationParameters)
            {
                nameValueCollection.Add(item.Key, item.Value);
            }
            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return this.client.ApplicationManager.CreateApplicationAsync(
                    new ApplicationDescription(new Uri("fabric:/" + applicationName),
                    applicationName,
                    applicationVersion,
                    nameValueCollection));
            },
            "CreateApplication",
            FabricErrorCode.ApplicationAlreadyExists);

            if (this.parameters.EnableServiceStateCache)
            {
                this.serviceStateCache.CreateApplication(applicationName);
            }
        }

        public void DeleteApplication(string applicationName)
        {
            LogHelper.Log("DeleteApplication {0}", applicationName);

            if (this.parameters.EnableServiceStateCache)
            {
                this.serviceStateCache.DeleteApplication(applicationName);
            }

            FabricDeployment.InvokeFabricClientOperationWithRetry(() => 
            {
                return this.client.ApplicationManager.DeleteApplicationAsync(
                    new DeleteApplicationDescription(
                        new Uri("fabric:/" + applicationName)));
            },
            "DeleteApplication",
            FabricErrorCode.ApplicationNotFound);
        }

        public void CreateName(Uri name)
        {
            LogHelper.Log("CreateName {0}", name);

            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return this.client.PropertyManager.CreateNameAsync(name);
            }, "CreateName", FabricErrorCode.NameAlreadyExists);
        }

        public void DeleteName(Uri name)
        {
            LogHelper.Log("DeleteName {0}", name);

            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return this.client.PropertyManager.DeleteNameAsync(name);
            }, "DeleteName", FabricErrorCode.NameNotFound);
        }

        public void CreateService(ServiceDescription description)
        {
            LogHelper.Log("CreateService {0}", description.ServiceName);

            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return this.client.ServiceManager.CreateServiceAsync(description);
            }, "CreateService", FabricErrorCode.ServiceAlreadyExists);
        }

        public void DeleteService(Uri serviceUri)
        {
            LogHelper.Log("DeleteService {0}", serviceUri);

            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return this.client.ServiceManager.DeleteServiceAsync(
                    new DeleteServiceDescription(serviceUri));
            }, "DeleteService", FabricErrorCode.ServiceNotFound);
        }

        public void UpdateService(string applicationName, string serviceName, ServiceUpdateDescription updateDescription)
        {
            var serviceUri = GetServiceUri(applicationName, serviceName);
            LogHelper.Log("UpdateService {0}", serviceUri);

            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return this.client.ServiceManager.UpdateServiceAsync(new Uri(serviceUri), updateDescription);
            }, "UpdateService");
        }

        public void CreateServiceGroup(ServiceGroupDescription description)
        {
            LogHelper.Log("CreateServiceGroup {0}", description.ServiceDescription.ServiceName);

            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return this.client.ServiceGroupManager.CreateServiceGroupAsync(description);
            }, "CreateServiceGroup", FabricErrorCode.ServiceGroupAlreadyExists);
        }

        public void DeleteServiceGroup(Uri serviceGroupName)
        {
            LogHelper.Log("DeleteServiceGroup {0}", serviceGroupName);

            FabricDeployment.InvokeFabricClientOperationWithRetry(() =>
            {
                return this.client.ServiceGroupManager.DeleteServiceGroupAsync(serviceGroupName);
            }, "DeleteServiceGroup", FabricErrorCode.ServiceGroupNotFound);
        }

        public ServiceGroupDescription GetServiceGroupDescription(Uri serviceGroupName)
        {
            LogHelper.Log("GetServiceGroupDescription {0}", serviceGroupName);

            ServiceGroupDescription description = null;

            FabricDeployment.InvokeInMTA(() =>
            {
                var task = this.client.ServiceGroupManager.GetServiceGroupDescriptionAsync(serviceGroupName);
                task.Wait();
                Debug.Assert(!task.IsFaulted);
                description = task.Result;
            });

            return description;
        }

        public ServiceDescription GetServiceDescription(Uri serviceName)
        {
            LogHelper.Log("GetServiceDescription {0}", serviceName);

            ServiceDescription description = null;

            FabricDeployment.InvokeInMTA(() =>
            {
                var task = this.client.ServiceManager.GetServiceDescriptionAsync(serviceName);
                task.Wait();
                Debug.Assert(!task.IsFaulted);
                description = task.Result;
            });

            return description;
        }

        public bool HasName(Uri name)
        {
            LogHelper.Log("HasName {0}", name);

            bool hasName = false;

            FabricDeployment.InvokeInMTA(() =>
            {
                var task = this.client.PropertyManager.NameExistsAsync(name);
                task.Wait();
                Debug.Assert(!task.IsFaulted);
                hasName = task.Result;
            });

            return hasName;
        }
        
        /// <summary>
        /// waits for a partition to become stable
        /// for stateful this means it will wait till both primary and secondary are up
        /// for stateless it means this will wait till it is up
        /// </summary>
        public bool WaitForPartitionToBeStable(
            string applicationName, 
            string serviceName, 
            DateTime startTimeUtc,
            TimeSpan timeout, 
            Func<ServicePartitionInformation, bool> predicate)
        {
            return this.WaitForPartitionsToEnterState(
                applicationName,
                serviceName,
                startTimeUtc,
                timeout,
                predicate,
                ServiceState.Stateless,
                ServiceState.Primary | ServiceState.Secondary);            
        }

        public bool WaitForPartitionsToEnterState(
            string applicationName,
            string serviceName,
            DateTime startTimeUtc,
            TimeSpan timeout,
            Func<ServicePartitionInformation, bool> predicate,
            params ServiceState[] targetState)
        {
            Debug.Assert(targetState.Length > 0);

            if (!this.parameters.EnableServiceStateCache)
            {
                LogHelper.Log("ServiceStateCache not enabled - wait for partitions is not available");
                throw new InvalidOperationException("ServiceStateCache not enabled - wait for partitions is not available");
            }

            // return true if all the partitions of the specified service (filtered by the predicate)
            // are in the targetState
            //
            // only consider notifications after the startTimeUtc
            return MiscUtility.WaitUntil(() =>
            {
                var partitions = this.serviceStateCache.GetServiceState(applicationName, serviceName);

                if (partitions.Count() == 0)
                {
                    return false;
                }

                var filteredPartitions = partitions.Where(e => predicate(e.PartitionInfo));
                if (filteredPartitions.Any(p => p.LastRefreshedTimeUtc < startTimeUtc))
                {
                    // there are still partitions that have not received an event since the startTimeUtc
                    return false;
                }

                // return true if all the partitions are in the desired state
                return filteredPartitions.All(p => targetState.Contains(p.State));
            }, timeout, 200);
        }

        public long RegisterServicePartitionResolutionChangeHandler(string applicationName, string serviceName, object partitionKey, ServicePartitionResolutionChangeHandler cb)
        {
            long callbackRegistrationHandle = -1;
            string serviceUri = GetServiceUri(applicationName, serviceName);
            LogHelper.Log("RegisterServicePartitionResolutionChangeHandler for {0}", serviceUri);
#pragma warning disable 0618 // obsolete
            if (partitionKey == null)
            {
                callbackRegistrationHandle = client.ServiceManager.RegisterServicePartitionResolutionChangeHandler(new Uri(serviceUri), cb);
            }
            else if (partitionKey is long)
            {
                callbackRegistrationHandle = client.ServiceManager.RegisterServicePartitionResolutionChangeHandler(new Uri(serviceUri), (long)partitionKey, cb);
            }
            else if (partitionKey is string)
            {
                callbackRegistrationHandle = client.ServiceManager.RegisterServicePartitionResolutionChangeHandler(new Uri(serviceUri), (string)partitionKey, cb);
#pragma warning restore 0618
            }
            else
            {
                throw new ArgumentException();
            }

            LogHelper.Log("Service {0}, pertition key {1}: Registered handle {2}", serviceUri, partitionKey, callbackRegistrationHandle);
            return callbackRegistrationHandle;
        }

        public void UnregisterServicePartitionResolutionChangeHandler(long callbackRegistrationHandle)
        {
            LogHelper.Log("Unregister callback handle {0}", callbackRegistrationHandle);
            this.client.ServiceManager.UnregisterServicePartitionResolutionChangeHandler(callbackRegistrationHandle);
        }
        
        public ResolvedServicePartition Resolve(string applicationName, string serviceName, object partitionKey, ResolvedServicePartition previousResult)
        {
            using (new LogHelper("Resolve"))
            {
                string serviceUri = GetServiceUri(applicationName, serviceName);
                return this.Resolve(serviceUri, partitionKey, previousResult);
            }
        }

        public ResolvedServicePartition Resolve(string serviceUri, object partitionKey, ResolvedServicePartition previousResult)
        {
            if (previousResult == null)
            {
                LogHelper.Log("Resolving {0} with NULL previousResult", serviceUri);
            }
            else
            {
                LogHelper.Log("Resolving {0} with previousResult with version {1}.{2}", serviceUri, previousResult.Generation, previousResult.FMVersion);
            }

            ResolvedServicePartition result = null;

            FabricDeployment.InvokeInMTA(() =>
                {
                    Task<ResolvedServicePartition> resolveTask = null;

                    if (partitionKey == null)
                    {
                        resolveTask = this.client.ServiceManager.ResolveServicePartitionAsync(new Uri(serviceUri), previousResult);
                    }
                    else if (partitionKey is long)
                    {
                        resolveTask = this.client.ServiceManager.ResolveServicePartitionAsync(new Uri(serviceUri), (long)partitionKey, previousResult);
                    }
                    else if (partitionKey is string)
                    {
                        resolveTask = this.client.ServiceManager.ResolveServicePartitionAsync(new Uri(serviceUri), (string)partitionKey, previousResult);
                    }
                    else
                    {
                        throw new ArgumentException("invalid partition key");
                    }

                    resolveTask.Wait();

                    this.previousResolveResults[serviceUri] = resolveTask.Result;

                    LogHelper.Log("Resolve {0} returned {1} endpoints ({2}.{3})", serviceUri, resolveTask.Result.Endpoints.Count, resolveTask.Result.Generation, resolveTask.Result.FMVersion);
                    result = resolveTask.Result;
                });

            return result;
        }

        public ResolvedServicePartition ResolveWithRetry(string applicationName, string serviceName, object partitionKey, ResolvedServicePartition previousResult, int retryCount)
        {
            return ResolveWithRetryInternal(applicationName, serviceName, partitionKey, previousResult, retryCount);
        }

        public ResolvedServicePartition ResolveUntilEndpointCountHit(string applicationName, string serviceName, object partitionKey, ResolvedServicePartition previousResult, int retryCount, int expectedEndpointCount)
        {
            return ResolveWithRetryInternal(applicationName, serviceName, partitionKey, previousResult, retryCount, (resolvedPartition) => resolvedPartition.Endpoints.Count == expectedEndpointCount);
        }

        private ResolvedServicePartition ResolveWithRetryInternal(string applicationName, string serviceName, object partitionKey, ResolvedServicePartition previousResult, int retryCount, Func<ResolvedServicePartition, bool> condition = null)
        {
            using (new LogHelper("ResolveWithRetry"))
            {
                LogHelper.Log("Resolving {0}.{1} with retryCount {2}", applicationName, serviceName, retryCount);
                string serviceUri = GetServiceUri(applicationName, serviceName);
                
                int sleepTime = 1000;
                for (int i = 0; i < retryCount; i++)
                {
                    try
                    {
                        var resolvedServicePartition = this.Resolve(serviceUri, partitionKey, previousResult);
                        if (condition == null || (condition != null && condition(resolvedServicePartition)))
                        {
                            return resolvedServicePartition;
                        }

                        previousResult = resolvedServicePartition;
                    }
                    catch (Exception ex)
                    {
                        LogHelper.Log("Exception encoutered while resolving: {0}", ex.Message);
                    }

                    LogHelper.Log("Failed to resolve. Sleeping for {0}", sleepTime);
                    Thread.Sleep(sleepTime);
                    sleepTime += 500;
                }

                LogHelper.Log("Failed to resolve {0}.{1}", applicationName, serviceName);
                throw new TimeoutException("Failed to resolve service");
            }
        }

        public string ResolveWithRetry(string applicationName, string serviceName, object partitionKey, int retryCount)
        {
            using (new LogHelper("ResolveWithRetry"))
            {
                LogHelper.Log("Resolving {0}.{1} with retryCount {2}", applicationName, serviceName, retryCount);
            
                int sleepTime = 1000;

                string serviceUri = GetServiceUri(applicationName, serviceName);
                
                ResolvedServicePartition previousResult = null;
                if (this.previousResolveResults.ContainsKey(serviceUri))
                {
                    previousResult = this.previousResolveResults[serviceUri];
                }

                for (int i = 0; i < retryCount; i++)
                {
                    try
                    {
                        var resolvedServicePartition = this.Resolve(serviceUri, partitionKey, previousResult);
                        string address = resolvedServicePartition.GetEndpoint().Address;
                        LogHelper.Log("Resolve.GetEndpoint {0}.{1}: {2}", applicationName, serviceName, address);
                        return address;
                    }
                    catch (Exception ex)
                    {
                        LogHelper.Log("Exception encoutered while resolving: {0}", ex.Message);
                    }

                    LogHelper.Log("Failed to resolve. Sleeping for {0}", sleepTime);
                    Thread.Sleep(sleepTime);
                    sleepTime += 500;
                }

                LogHelper.Log("Failed to resolve {0}.{1}", applicationName, serviceName);
                throw new TimeoutException("Failed to resolve service");
            }
        }
                        
        private string GetServiceUri(string applicationName, string serviceName)
        {
            if (serviceName.ToLowerInvariant().StartsWith("fabric:/"))
            {
                return serviceName;
            }
            else
            {
                return string.Format("fabric:/{0}/{1}", applicationName.ToLower(), serviceName);
            }
        }

        private static IEnumerable<IComparable> GetPartitionKeys(PartitionSchemeDescription description)
        {
            var namedPartitionDescription = description as NamedPartitionSchemeDescription;
            if (namedPartitionDescription != null)
            {
                return namedPartitionDescription.PartitionNames;
            }

            var singletonPartitionDescription = description as SingletonPartitionSchemeDescription;
            if (singletonPartitionDescription != null)
            {                
                // the partition key for a singleton partition description is null
                return new List<IComparable> { null };
            }

            var uniformIntPartitionDescription = description as UniformInt64RangePartitionSchemeDescription;
            if (uniformIntPartitionDescription != null)
            {
                long range = 1 + uniformIntPartitionDescription.HighKey - uniformIntPartitionDescription.LowKey;
                long intervalSize = range / uniformIntPartitionDescription.PartitionCount;
                var rv = new List<IComparable>();

                for (int i = 0; i < uniformIntPartitionDescription.PartitionCount; i++)
                {
                    rv.Add(uniformIntPartitionDescription.LowKey + intervalSize * i);
                }

                return rv;
            }

            throw new InvalidOperationException("Unknown partition type");
        }

        public static TResult InvokeFabricClientOperationWithRetry<TResult>(Func<Task<TResult>> func, string tag, FabricErrorCode expectedError = FabricErrorCode.Unknown)
        {
            Task<TResult> task = null;
            InvokeFabricClientOperationWithRetry(
                () => { return (Task)(task = func()); },
                tag,
                expectedError);

            return task.Result;
        }

        public static void InvokeFabricClientOperationWithRetry(Func<Task> func, string tag, FabricErrorCode idempotentError = FabricErrorCode.Unknown)
        {
            for (int i = 0; i < 10; i++)
            {
                LogHelper.Log("Invoking operation {0}. Retry Count: {1}", tag, i);

                try
                {
                    func().Wait();
                    LogHelper.Log("Success {0}", tag);
                    return;
                }
                catch (AggregateException ex)
                {
                    if (ex.InnerException is FabricTransientException)
                    {
                        Thread.Sleep(5000);
                        LogHelper.Log("{0} while invoking {1}. Will Retry", ex.InnerException.GetType(), tag);
                        continue;
                    }
                    else if (ex.InnerException is FabricException)
                    {
                        if (idempotentError != FabricErrorCode.Unknown &&
                            (ex.InnerException as FabricException).ErrorCode == idempotentError)
                        {
                            LogHelper.Log("Success on idempotent error {0} invoking {1}.", idempotentError, tag);
                            return;
                        }

                        throw ex.InnerException;
                    }
                    else
                    {
                        throw ex.InnerException;
                    }
                }
            }

            LogHelper.Log("Failed after retry {0}", tag);
            throw new InvalidOperationException();
        }

        private static void InvokeInMTA(Action action)
        {
            if (Thread.CurrentThread.GetApartmentState() == ApartmentState.MTA)
            {
                action();
            }
            else
            {
                Exception thrownException = null;
                Thread t = new Thread((o) =>
                {
                    try
                    {
                        action();
                    }
                    catch (Exception ex)
                    {
                        thrownException = ex;
                    }
                });
                t.SetApartmentState(ApartmentState.MTA);
                t.Start();
                t.Join();

                if (thrownException != null)
                {
                    throw thrownException;
                }    
            }
        }

        private IEnumerable<string> GetFabricBinaries()
        {
            var fabricFiles = new string[]
            {
                "Fabric.exe",
                "ImageBuilder.exe",
                "FabricClient.dll",
                "FabricCommon.dll",
                "FabricRuntime.dll",
                "FabricResources.dll",
                "System.Fabric.Management.dll",
                "System.Fabric.Management.ServiceModel.dll",
                "System.Fabric.Management.ServiceModel.XmlSerializers.dll",
                "Microsoft.WindowsAzure.StorageClient.dll",
                "LeaseLayer.dll",
                "ServiceFabricServiceModel.xsd",
                "ese.dll",
                "System.Fabric.dll",
                "System.Fabric.Strings.dll",
                "CtrlCSender.exe",
                "System.Fabric.Test.Host.exe",
                "Newtonsoft.Json.dll"
            };

            LogHelper.Log("Copying fabric files");
            foreach (var item in fabricFiles.Select(e => Path.Combine(this.parameters.FabricBinariesPath, e)))
            {
                if (!File.Exists(item))
                {
                    LogHelper.Log("Required file for fabric not found in fabric binaries {0}", item);
                    throw new InvalidOperationException("Required file not found");
                }
            }

            return fabricFiles.Select(e => Path.Combine(this.parameters.FabricBinariesPath, e));
        }

        private IEnumerable<string> GetFabricDirectories()
        {
            var fabricDirs = new string[]
            {
                "en-US"
            };

            LogHelper.Log("Copying fabric directories");
            foreach (var item in fabricDirs.Select(e => Path.Combine(this.parameters.FabricBinariesPath, e)))
            {
                if (!Directory.Exists(item))
                {
                    LogHelper.Log("Required directory for fabric not found in fabric binaries {0}", item);
                    throw new InvalidOperationException("Required directory not found");
                }
            }

            return fabricDirs.Select(e => Path.Combine(this.parameters.FabricBinariesPath, e));
        }

        private void DeployImageStoreClient()
        {
            string[] ImageStoreClientFiles = new string[]
            {
                "imagestoreclient.exe",
                "System.Fabric.Management.dll",
                "System.Fabric.Management.ServiceModel.dll",
                "System.Fabric.Management.ServiceModel.XmlSerializers.dll",
                "FabricCommon.dll",
                "System.Fabric.dll",
                "System.Fabric.Strings.dll",
            };

            LogHelper.Log("Copying image store");

            foreach (var item in ImageStoreClientFiles)
            {
                if (!File.Exists(Path.Combine(this.parameters.FabricBinariesPath, item)))
                {
                    LogHelper.Log("Required file for imagestore not found in fabric binaries {0}", item);
                    throw new InvalidOperationException("Required file for image store not found");
                }
            }

            FileUtility.CopyFilesToDirectory(
                ImageStoreClientFiles.Select(e => Path.Combine(this.parameters.FabricBinariesPath, e)),
                this.parameters.DeploymentRootPath);
        }

        private IEnumerable<IPEndPoint> GenerateAllNodeRuntimeAddresses()
        {
            //Get the first address for FabricHost
            int startAddress = this.parameters.StartPort + 1;
            for (int i = 0; i < this.parameters.NodeCount; i++)
            {
                yield return new IPEndPoint(IPAddress.Loopback, startAddress + i * FabricDeployment.PortsUsedPerNode);
            }
        }

        #region IDisposable Members

        public void Dispose()
        {
            foreach (var item in this.nodes)
            {
                item.Dispose();
            }
            this.nodes.Clear();
            if (this.fabricHost != null)
            {
                this.fabricHost.Dispose();
            }
            LogManUtil.StopTracing();

            FileUtility.RecursiveDelete(this.parameters.DeploymentRootPath, new string[] { this.LogPath, this.TestOutputPath });

            this.processMonitor.Dispose();
        }

        #endregion

        /// <summary>
        /// Manitains a cache of all the services currently deployed and their states
        /// </summary>
        private class ServiceStateCache
        {
            // a service is identified by a tuple of (appName, serviceTypeName)
            // the value is the list of the state of each partition for that service
            private readonly Dictionary<Tuple<string, string>, List<ServiceStateInformation>> state = new Dictionary<Tuple<string, string>, List<ServiceStateInformation>>();

            private readonly object syncLock = new object();
            private readonly FabricClient client;

            public ServiceStateCache(FabricClient client)
            {
                this.client = client;
            }

            /// <summary>
            /// Return the state of each partition for a service in an app
            /// </summary>
            public IEnumerable<ServiceStateInformation> GetServiceState(string applicationName, string serviceName)
            {
                lock (this.syncLock)
                {
                    var key = Tuple.Create(applicationName.ToLower(), serviceName.ToLower());
                    if (!this.state.ContainsKey(key))
                    {
                        return Enumerable.Empty<ServiceStateInformation>();
                    }

                    return new List<ServiceStateInformation>(this.state[key]);
                }
            }

            public void CreateApplication(string applicationName)
            {
                applicationName = applicationName.ToLower();
                FabricDeployment.InvokeInMTA(() =>
                {
                    // enumerate the child objects
                    var task = this.client.PropertyManager.EnumerateSubNamesAsync(new Uri("fabric:/" + applicationName), null, false);
                    task.Wait();

                    // read all the service descriptions
                    List<ServiceDescription> sdList = new List<ServiceDescription>();
                    List<Task> sdTasks = new List<Task>();
                    foreach (var name in task.Result)
                    {
                        sdTasks.Add(this.client.ServiceManager.GetServiceDescriptionAsync(name).ContinueWith((t) => sdList.Add(t.Result)));
                    }

                    Task.WaitAll(sdTasks.ToArray());

                    // set the initial state
                    lock (this.syncLock)
                    {
                        foreach (var sd in sdList)
                        {
                            this.state[Tuple.Create(applicationName, sd.ServiceTypeName.ToLower())] = new List<ServiceStateInformation>();
                        }
                    }

                    var resolutionTasks = new List<Task<bool>>();
                    foreach (var sd in sdList)
                    {
                        foreach (var partitionKey in FabricDeployment.GetPartitionKeys(sd.PartitionSchemeDescription))
                        {
                            TimeSpan ts = TimeSpan.FromSeconds(Debugger.IsAttached ? 600 : 120);
                            resolutionTasks.Add(this.ResolveInitial(applicationName, sd, sd.ServiceName, ts, partitionKey));
                        }
                    }

                    // TODO: Fix this up
                    // if the resolutions fail then this is going to blow up because the task's exception property would not be accessed at all
                    Task.WaitAll(resolutionTasks.ToArray());

                    if (resolutionTasks.Any(e => !e.Result))
                    {
                        LogHelper.Log("Initial resolution failed");
                        throw new InvalidOperationException("Failed to wait");
                    }
                });
            }

            private Task<bool> ResolveInitial(string appName, ServiceDescription sd, Uri uri, TimeSpan totalTime, IComparable partitionKey)
            {
                var tcs = new TaskCompletionSource<bool>();

                this.ResolveInitial(appName, sd, uri, totalTime, partitionKey, tcs);

                return tcs.Task;
            }

            private void ResolveInitial(string applicationName, ServiceDescription sd, Uri uri, TimeSpan time, IComparable partitionKey, TaskCompletionSource<bool> tcs)
            {
                DateTime start = DateTime.UtcNow;

                Func<Task<ResolvedServicePartition>> resolveFunc = null;
                if (partitionKey == null)
                {
                    resolveFunc = () => this.client.ServiceManager.ResolveServicePartitionAsync(uri, TimeSpan.FromSeconds(5));
                }
                else if (partitionKey is long)
                {
                    resolveFunc = () => this.client.ServiceManager.ResolveServicePartitionAsync(uri, (long)partitionKey, TimeSpan.FromSeconds(5));
                }
                else if (partitionKey is string)
                {
                    resolveFunc = () => this.client.ServiceManager.ResolveServicePartitionAsync(uri, (string)partitionKey, TimeSpan.FromSeconds(5));
                }
                else
                {
                    throw new ArgumentException();
                }

                resolveFunc().ContinueWith((t) =>
                {
                    if (t.IsFaulted)
                    {
                        // try again                        
                        Thread.Sleep(500);

                        TimeSpan elapsed = DateTime.UtcNow - start;
                        LogHelper.Log("Temporary failure during initial resolution for {0}. Current Time available: {1}. Time taken for this call: {2} {3}", uri.ToString(), time.TotalMilliseconds, elapsed.TotalMilliseconds, t.Exception);
                        if (time.TotalMilliseconds - elapsed.TotalMilliseconds <= 0)
                        {
                            tcs.SetResult(false);
                        }
                        else
                        {
                            this.ResolveInitial(applicationName, sd, uri, time - elapsed, partitionKey, tcs);
                        }
                    }
                    else
                    {
                        // success
                        LogHelper.Log("Initial resolution for {0} succeeded", uri.ToString());
                        var ssi = new ServiceStateInformation(this.client, sd, t.Result, partitionKey);
                        lock (this.syncLock)
                        {
                            this.state[Tuple.Create(applicationName, sd.ServiceTypeName.ToLower())].Add(ssi);
                        }

                        tcs.SetResult(true);
                    }
                });
            }

            public void DeleteApplication(string applicationName)
            {
                applicationName = applicationName.ToLower();

                lock (this.syncLock)
                {
                    foreach (var appServiceTuple in this.state.Keys)
                    {
                        if (appServiceTuple.Item1 == applicationName)
                        {
                            foreach (var registration in this.state[appServiceTuple])
                            {
                                registration.Dispose();                                
                            }
                        }
                    }

                    var keysToRemove = this.state.Keys.Where(e => e.Item1 == applicationName).ToArray();

                    foreach (var key in keysToRemove)
                    {
                        this.state.Remove(key);
                    }
                }
            }

            /// <summary>
            /// an instance of this class is created for each partition
            /// this is where the notification handler to the service description is actually hooked up
            /// </summary>
            public class ServiceStateInformation : IDisposable
            {
                private readonly object syncLock = new object();
                private readonly ServicePartitionInformation partitionInfo;
                private long callbackRegistrationHandle;
                private FabricDeployment.ServiceState state;
                private bool isDisposed;
                private DateTime lastRefreshedTimeUtc;
                private readonly FabricClient client;
                
                public ServiceStateInformation(FabricClient client, Fabric.Description.ServiceDescription sd, ResolvedServicePartition resolvedPartition, IComparable partitionKey)
                {
                    this.ServiceDescription = sd;
                    this.partitionInfo = resolvedPartition.Info;
                    this.state = ServiceStateInformation.GetState(resolvedPartition);
                    this.PartitionKey = partitionKey;
                    this.lastRefreshedTimeUtc = DateTime.UtcNow;
                    this.client = client;

                    if (partitionKey == null)
                    {
#pragma warning disable 0618 // obsolete
                        this.callbackRegistrationHandle = client.ServiceManager.RegisterServicePartitionResolutionChangeHandler(sd.ServiceName, this.OnServiceChange);
                    }
                    else if (partitionKey is long)
                    {
                        this.callbackRegistrationHandle = client.ServiceManager.RegisterServicePartitionResolutionChangeHandler(sd.ServiceName, (long)partitionKey, this.OnServiceChange);
                    }
                    else if (partitionKey is string)
                    {
                        this.callbackRegistrationHandle = client.ServiceManager.RegisterServicePartitionResolutionChangeHandler(sd.ServiceName, (string)partitionKey, this.OnServiceChange);
#pragma warning restore 0618
                    }
                    else
                    {
                        throw new ArgumentException();
                    }

                    LogHelper.Log("Registered service change handler {0} for partition key '{1}'", this.callbackRegistrationHandle, partitionKey);
                }

                public ServiceDescription ServiceDescription { get; private set; }
                public IComparable PartitionKey { get; private set; }

                public ServicePartitionInformation PartitionInfo
                {
                    get
                    {
                        lock (this.syncLock)
                        {
                            return this.partitionInfo;
                        }
                    }
                }

                public FabricDeployment.ServiceState State
                {
                    get
                    {
                        lock (this.syncLock)
                        {
                            return this.state;
                        }
                    }
                }

                public DateTime LastRefreshedTimeUtc
                {
                    get
                    {
                        lock (this.syncLock)
                        {
                            return this.lastRefreshedTimeUtc;
                        }
                    }
                }

                private void OnServiceChange(FabricClient client, long handlerId, ServicePartitionResolutionChange args)
                {
                    lock (this.syncLock)
                    {
                        if (args.HasException)
                        {
                            LogHelper.Log("OnServiceChange, handler {0}: error {1}", handlerId, args.Exception.Message);
                            return;
                        }

                        LogHelper.Log("OnServiceChange for {0}, handler {1}: addresses changed", args.Result.ServiceName, handlerId);
                        
                        // figure out the new state
                        ServiceState newState = ServiceStateInformation.GetState(args.Result);

                        // set the new state and return
                        LogHelper.Log("OnServiceChange for {0}. Old State = {1}. New State = {2}", args.Result.ServiceName.ToString(), this.state, newState);
                        this.state = newState;
                        this.lastRefreshedTimeUtc = DateTime.UtcNow;
                    }
                }

                #region IDisposable Members

                public void Dispose()
                {
                    if (this.isDisposed == false)
                    {
                        lock (this.syncLock)
                        {
                            LogHelper.Log("Unregister callback handle: {0} for {1}", this.callbackRegistrationHandle, this.PartitionKey);
                            this.client.ServiceManager.UnregisterServicePartitionResolutionChangeHandler(this.callbackRegistrationHandle);

                            this.isDisposed = true;
                        }
                    }                    
                }

                #endregion

                private static FabricDeployment.ServiceState GetState(ResolvedServicePartition resolvedPartition)
                {
                    ServiceState state = ServiceState.Unknown;
                    foreach (var endPoint in resolvedPartition.Endpoints)
                    {
                        switch (endPoint.Role)
                        {
                            case ServiceEndpointRole.StatefulPrimary:
                                state |= ServiceState.Primary;
                                break;
                            case ServiceEndpointRole.StatefulSecondary:
                                state |= ServiceState.Secondary;
                                break;
                            case ServiceEndpointRole.Stateless:
                                state |= ServiceState.Stateless;
                                break;
                            default:
                                break;
                        }
                    }

                    return state;
                }
            }
        }
    }
}
