// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client
{
    using Security;
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.JsonSerializerWrapper;
    using System.Fabric.Query;
    using System.Fabric.Repair;
    using System.Fabric.Result;
    using System.Fabric.Testability.Client.Structures;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Numerics;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading;
    using System.Threading.Tasks;
    using Newtonsoft.Json;
    using Xml;
    using Xml.Serialization;

    public sealed class SystemFabricClient : FabricClientObject
    {
        private const string TraceSource = "SystemFabricClient";

        private readonly FabricClient fabricClient;
        private IJsonSerializer jsonSerializer;

        public SystemFabricClient(FabricClient fabricClient)
        {
            this.fabricClient = fabricClient;
        }

        public SystemFabricClient(
            IEnumerable<string> gatewayAddresses,
            SecurityCredentials securityCredentials,
            FabricClientSettings clientSettings)
        {
            if (gatewayAddresses == null || gatewayAddresses.Count() == 0)
            {
                this.fabricClient = new FabricClient();
            }
            else
            {
                if (securityCredentials != null && clientSettings != null)
                {
                    this.fabricClient = new FabricClient(securityCredentials, clientSettings, gatewayAddresses.ToArray());
                }
                else if (securityCredentials != null)
                {
                    this.fabricClient = new FabricClient(securityCredentials, gatewayAddresses.ToArray());
                }
                else if (clientSettings != null)
                {
                    this.fabricClient = new FabricClient(clientSettings, gatewayAddresses.ToArray());
                }
                else
                {
                    this.fabricClient = new FabricClient(gatewayAddresses.ToArray());
                }
            }
        }

        public FabricClient FabricClient
        {
            get { return this.fabricClient; }
        }

        public override async Task<OperationResult> RecoverServicePartitionsAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
            delegate
            {
                return this.fabricClient.ClusterManager.RecoverServicePartitionsAsync(
                    serviceName,
                    timeout,
                    cancellationToken);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RecoverSystemPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
            delegate
            {
                return this.fabricClient.ClusterManager.RecoverSystemPartitionsAsync(
                    timeout,
                    cancellationToken);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<DeployedApplication[]>> GetDeployedApplicationListAsync(string nodeName, Uri applicationNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteNoise(TraceSource, "SystemFabricClient GetDeployedApplicationListAsync {0} {1}", nodeName, applicationNameFilter);

            DeployedApplicationList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     queryResult = await this.fabricClient.QueryManager.GetDeployedApplicationListAsync(
                         nodeName,
                         applicationNameFilter,
                         timeout,
                         cancellationToken);
                 });

            if (errorCode != 0 || queryResult == null)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<DeployedApplication[]>(errorCode);
            }

            return OperationResult<DeployedApplication>.GetOperationResultArray(errorCode, queryResult);
        }

        public override async Task<OperationResult<DeployedApplicationPagedList>> GetDeployedApplicationPagedListAsync(
            PagedDeployedApplicationQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteNoise(
                TraceSource,
                "SystemFabricClient GetDeployedApplicationPagedListAsync {0}",
                queryDescription);

            DeployedApplicationPagedList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     queryResult = await this.fabricClient.QueryManager.GetDeployedApplicationPagedListAsync(
                         queryDescription,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<DeployedApplicationPagedList>(errorCode, queryResult);
        }

        public override async Task<OperationResult> CopyApplicationPackageAndShowProgressAsync(
            string applicationPackagePath,
            string imageStoreConnectionString,
            string applicationPackagePathInImageStore,
            bool compress,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ThrowIf.Null(applicationPackagePath, "applicationPackagePath");
            ThrowIf.Null(imageStoreConnectionString, "imageStoreConnectionString");

            if (string.IsNullOrEmpty(applicationPackagePathInImageStore))
            {
                DirectoryInfo packageDirecotry = new DirectoryInfo(applicationPackagePath);
                applicationPackagePathInImageStore = packageDirecotry.Name;
            }

            TestabilityTrace.TraceSource.WriteNoise(TraceSource, "SystemFabricClient CopyApplicationPackageAsync with progress handler {0} {1} {2}", applicationPackagePath, imageStoreConnectionString, applicationPackagePathInImageStore);
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    if (string.IsNullOrWhiteSpace(imageStoreConnectionString))
                    {
                        imageStoreConnectionString = GetClusterManifestSetting(
                            await fabricClient.ClusterManager.GetClusterManifestAsync(timeout, cancellationToken),
                            "Management",
                            "ImageStoreConnectionString");
                    }

#if !DotNetCoreClr
                    // Todo: RDBug 10584947:Application package compress currently disabled for CoreClr
                    if (compress)
                    {
                        ImageStore.ImageStoreUtility.ArchiveApplicationPackage(applicationPackagePath, null);
                    }
#endif

                    TimeSpan interval = TimeSpan.FromSeconds(5);
                    long completedFileCount = 0;
                    long totalFileCount = 0;
                    IImageStoreProgressHandler progressHandler = new UploadProgressHandler(ref completedFileCount, ref totalFileCount, interval);

                    this.fabricClient.ApplicationManager.CopyApplicationPackage(
                        imageStoreConnectionString,
                        applicationPackagePath,
                        applicationPackagePathInImageStore,
                        progressHandler,
                        timeout);

                    await Task.Delay(interval);

                    if (completedFileCount != totalFileCount)
                    {
                        throw new InvalidOperationException("FileCount mismatch");
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CopyApplicationPackageAsync(
            string applicationPackagePath,
            string imageStoreConnectionString,
            string applicationPackagePathInImageStore,
            bool compress,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ThrowIf.Null(applicationPackagePath, "applicationPackagePath");

            if (string.IsNullOrEmpty(applicationPackagePathInImageStore))
            {
                DirectoryInfo packageDirecotry = new DirectoryInfo(applicationPackagePath);
                applicationPackagePathInImageStore = packageDirecotry.Name;
            }

            TestabilityTrace.TraceSource.WriteNoise(TraceSource, "SystemFabricClient CopyApplicationPackageAsync {0} {1} {2}", applicationPackagePath, imageStoreConnectionString, applicationPackagePathInImageStore);

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     if (string.IsNullOrWhiteSpace(imageStoreConnectionString))
                     {
                         imageStoreConnectionString = GetClusterManifestSetting(
                             await fabricClient.ClusterManager.GetClusterManifestAsync(timeout, cancellationToken),
                             "Management",
                             "ImageStoreConnectionString");
                     }

#if !DotNetCoreClr
                     // Todo: RDBug 10584947:Application package compress currently disabled for CoreClr
                     if (compress)
                     {
                         ImageStore.ImageStoreUtility.ArchiveApplicationPackage(applicationPackagePath, null);
                     }
#endif

                     this.fabricClient.ApplicationManager.CopyApplicationPackage(
                                 imageStoreConnectionString,
                                 applicationPackagePath,
                                 applicationPackagePathInImageStore,
                                 timeout);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RemoveApplicationPackageAsync(string applicationPackagePathInImageStore, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(applicationPackagePathInImageStore, "applicationPackagePathInImageStore");
            ThrowIf.Null(imageStoreConnectionString, "imageStoreConnectionString");

            TestabilityTrace.TraceSource.WriteNoise(TraceSource, "SystemFabricClient RemoveApplicationPackageAsync {0} {1}", imageStoreConnectionString, applicationPackagePathInImageStore);

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    if (string.IsNullOrWhiteSpace(imageStoreConnectionString))
                    {
                        imageStoreConnectionString = GetClusterManifestSetting(
                            await fabricClient.ClusterManager.GetClusterManifestAsync(timeout, cancellationToken),
                            "Management",
                            "ImageStoreConnectionString");
                    }

                    this.fabricClient.ApplicationManager.RemoveApplicationPackage(imageStoreConnectionString, applicationPackagePathInImageStore);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CopyClusterPackageAsync(string codePackagePath, string clusterManifestPath, string imageStoreConnectionString, string codePackagePathInImageStore, string clusterManifestPathInImageStore, CancellationToken cancellationToken)
        {
            ThrowIf.Null(imageStoreConnectionString, "imageStoreConnectionString");

            TestabilityTrace.TraceSource.WriteInfo(
                  TraceSource,
                  "SystemFabricClient CopyClusterPackageAsync codePackagePath: '{0}', clusterManifestPath: '{1}', imageStoreConnectionString: '{2}', codePackagePathInImageStore: '{3}', clusterManifestPathInImageStore: '{4}'",
                  codePackagePath,
                  clusterManifestPath,
                  imageStoreConnectionString,
                  codePackagePathInImageStore,
                  clusterManifestPathInImageStore);

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    if (string.IsNullOrWhiteSpace(imageStoreConnectionString))
                    {
                        imageStoreConnectionString = GetClusterManifestSetting(
                            await fabricClient.ClusterManager.GetClusterManifestAsync(),
                            "Management",
                            "ImageStoreConnectionString");
                    }

                    this.fabricClient.ClusterManager.CopyClusterPackage(imageStoreConnectionString, clusterManifestPath, clusterManifestPathInImageStore, codePackagePath, codePackagePathInImageStore);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CopyClusterPackageAsync(string codePackagePath, string clusterManifestPath, string imageStoreConnectionString, string codePackagePathInImageStore, string clusterManifestPathInImageStore,
            TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(imageStoreConnectionString, "imageStoreConnectionString");

            TestabilityTrace.TraceSource.WriteInfo(
                  TraceSource,
                  "SystemFabricClient CopyClusterPackageAsync codePackagePath: '{0}', clusterManifestPath: '{1}', imageStoreConnectionString: '{2}', codePackagePathInImageStore: '{3}', clusterManifestPathInImageStore: '{4}' timeout '{5}'",
                  codePackagePath,
                  clusterManifestPath,
                  imageStoreConnectionString,
                  codePackagePathInImageStore,
                  clusterManifestPathInImageStore,
                  timeout);

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    if (string.IsNullOrWhiteSpace(imageStoreConnectionString))
                    {
                        imageStoreConnectionString = GetClusterManifestSetting(
                            await fabricClient.ClusterManager.GetClusterManifestAsync(),
                            "Management",
                            "ImageStoreConnectionString");
                    }

                    this.fabricClient.ClusterManager.CopyClusterPackage(imageStoreConnectionString, clusterManifestPath, clusterManifestPathInImageStore, codePackagePath, codePackagePathInImageStore, timeout);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<DeployedCodePackage[]>> GetDeployedCodePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string codePackageNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteNoise(TraceSource, "SystemFabricClient GetDeployedCodePackageListAsync {0} {1}", nodeName, applicationName);

            DeployedCodePackageList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    queryResult = await this.fabricClient.QueryManager.GetDeployedCodePackageListAsync(
                        nodeName,
                        applicationName,
                        serviceManifestNameFilter,
                        codePackageNameFilter,
                        timeout,
                        cancellationToken);
                });

            if (errorCode != 0 || queryResult == null)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<DeployedCodePackage[]>(errorCode);
            }

            return OperationResult<DeployedCodePackage>.GetOperationResultArray(errorCode, queryResult);
        }

        public override async Task<OperationResult<DeployedServiceReplica[]>> GetDeployedReplicaListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, Guid? partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteNoise(TraceSource, "SystemFabricClient GetDeployedReplicaListAsync {0} {1} {2} {3} {4} minutes", nodeName, applicationName, serviceManifestNameFilter, partitionIdFilter, timeout.TotalMinutes);

            DeployedServiceReplicaList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    queryResult = await this.fabricClient.QueryManager.GetDeployedReplicaListAsync(
                        nodeName,
                        applicationName,
                        serviceManifestNameFilter,
                        partitionIdFilter,
                        timeout,
                        cancellationToken);
                });

            return OperationResult<DeployedServiceReplica>.GetOperationResultArray(errorCode, queryResult);
        }

        public override async Task<OperationResult<DeployedServicePackage[]>> GetDeployedServicePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteNoise(TraceSource, "SystemFabricClient  GetDeployedServicePackageListAsync {0} {1}", nodeName, applicationName);

            DeployedServicePackageList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    queryResult = await this.fabricClient.QueryManager.GetDeployedServicePackageListAsync(
                        nodeName,
                        applicationName,
                        serviceManifestNameFilter,
                        timeout,
                        cancellationToken);
                });

            return OperationResult<DeployedServicePackage>.GetOperationResultArray(errorCode, queryResult);
        }

        public override async Task<OperationResult<DeployedServiceType[]>> GetDeployedServiceTypeListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteNoise(TraceSource, "SystemFabricClient GetDeployedServiceTypeListAsync {0} {1}", nodeName, applicationName);

            DeployedServiceTypeList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    queryResult = await this.fabricClient.QueryManager.GetDeployedServiceTypeListAsync(
                        nodeName,
                        applicationName,
                        serviceManifestNameFilter,
                        serviceTypeNameFilter,
                        timeout,
                        cancellationToken);
                });

            return OperationResult<DeployedServiceType>.GetOperationResultArray(errorCode, queryResult);
        }

        public override OperationResult<long> RegisterServicePartitionResolutionChangeHandler(Uri name, object partitionKey, ServicePartitionResolutionChangeHandler callback)
        {
            long callbackIdentifier = 0;
            NativeTypes.FABRIC_ERROR_CODE errorCode = FabricClientState.HandleException(
                () => callbackIdentifier = this.RegisterServicePartitionResolutionChangeHandlerInner(name, partitionKey, callback));

            OperationResult<long> result = FabricClientState.CreateOperationResultFromNativeErrorCode<long>(errorCode);
            if (errorCode == 0)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<long>(callbackIdentifier);
            }

            return result;
        }

        public override OperationResult UnregisterServicePartitionResolutionChangeHandler(long callbackId)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = FabricClientState.HandleException(
                () => this.UnregisterServicePartitionResolutionChangeHandlerInner(callbackId));

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RegisterServiceNotificationFilterAsync(ServiceNotificationFilterDescription filterDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            long filterId = 0;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    filterId = await this.fabricClient.ServiceManager.RegisterServiceNotificationFilterAsync(filterDescription, timeout, cancellationToken);
                });

            if (errorCode != 0)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<long>(errorCode);
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode<long>(filterId);
        }

        public override async Task<OperationResult> UnregisterServiceNotificationFilterAsync(long filterId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ServiceManager.UnregisterServiceNotificationFilterAsync(filterId, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<bool>> NameExistsAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            bool nameExists = false;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    nameExists = await this.fabricClient.PropertyManager.NameExistsAsync(name, timeout, cancellationToken);
                });

            if (errorCode != 0)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<bool>(errorCode);
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode<bool>(nameExists);
        }

        public override async Task<OperationResult> ProvisionApplicationAsync(string buildPath, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await ProvisionApplicationAsync(new ProvisionApplicationTypeDescription(buildPath), timeout, cancellationToken);
        }

        public override async Task<OperationResult> ProvisionApplicationAsync(ProvisionApplicationTypeDescriptionBase description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
            delegate
            {
                return this.fabricClient.ApplicationManager.ProvisionApplicationAsync(
                    description,
                    timeout,
                    cancellationToken);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> ProvisionFabricAsync(string codePackagePath, string clusterManifestFilePath, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ClusterManager.ProvisionFabricAsync(
                        codePackagePath,
                        clusterManifestFilePath,
                        timeout,
                        cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UnprovisionApplicationAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.UnprovisionApplicationAsync(new UnprovisionApplicationTypeDescription(applicationTypeName, applicationTypeVersion), timeout, cancellationToken);
        }

        public override async Task<OperationResult> UnprovisionApplicationAsync(UnprovisionApplicationTypeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ApplicationManager.UnprovisionApplicationAsync(
                        description,
                        timeout,
                        cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UnprovisionFabricAsync(string codeVersion, string configVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ClusterManager.UnprovisionFabricAsync(
                        codeVersion,
                        configVersion,
                        timeout,
                        cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> StartNodeAsync(
             string nodeName,
             BigInteger nodeInstance,
             string ipAddressOrFQDN,
             int clusterConnectionPort,
             CompletionMode completionMode,
             TimeSpan operationTimeout,
             CancellationToken token)
        {
#pragma warning disable 618
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.FaultManager.StartNodeAsync(
                        nodeName,
                        nodeInstance,
                        ipAddressOrFQDN,
                        clusterConnectionPort,
                        completionMode,
                        operationTimeout,
                        token);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
#pragma warning restore 618
        }

        public override async Task<OperationResult> StopNodeAsync(
            string nodeName,
            BigInteger nodeInstance,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
#pragma warning disable 618
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.FaultManager.StopNodeAsync(
                        nodeName,
                        nodeInstance,
                        completionMode,
                        operationTimeout,
                        token);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
#pragma warning restore 618
        }

        public override async Task<OperationResult<RestartNodeResult>> RestartNodeAsync(
            string nodeName,
            BigInteger nodeInstance,
            bool createFabricDump,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            RestartNodeResult result = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
            async delegate
            {
                result = await this.fabricClient.FaultManager.RestartNodeAsync(
                    nodeName,
                    nodeInstance,
                    createFabricDump,
                    completionMode,
                    operationTimeout,
                    token);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<RestartNodeResult>(errorCode, result);
        }

        public override async Task<OperationResult<RestartNodeResult>> RestartNodeAsync(
            ReplicaSelector replicaSelector,
            bool createFabricDump,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            RestartNodeResult result = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
            async delegate
            {
                result = await this.fabricClient.FaultManager.RestartNodeAsync(
                    replicaSelector,
                    createFabricDump,
                    completionMode,
                    operationTimeout,
                    token);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<RestartNodeResult>(errorCode, result);
        }

        public override async Task<OperationResult> RestartDeployedCodePackageAsync(
            Uri applicationName,
            ReplicaSelector replicaSelector,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
            delegate
            {
                return this.fabricClient.FaultManager.RestartDeployedCodePackageAsync(
                    applicationName,
                    replicaSelector,
                    completionMode,
                    operationTimeout,
                    token);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RestartDeployedCodePackageAsync(
            string nodeName,
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string codePackageName,
            long codePackageInstanceId,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
            delegate
            {
                return this.fabricClient.FaultManager.RestartDeployedCodePackageAsync(
                    nodeName,
                    applicationName,
                    serviceManifestName,
                    servicePackageActivationId,
                    codePackageName,
                    codePackageInstanceId,
                    completionMode,
                    operationTimeout,
                    token);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RestartReplicaAsync(
            ReplicaSelector replicaSelector,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                                          () =>
                                                              this.fabricClient.FaultManager.RestartReplicaAsync(
                                                                  replicaSelector,
                                                                  completionMode,
                                                                  operationTimeout,
                                                                  token)).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RestartReplicaAsync(
            string nodeName,
            Guid partitionId,
            long replicaId,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                                        () =>
                                                            this.fabricClient.FaultManager
                                                                .RestartReplicaAsync(
                                                                    nodeName,
                                                                    partitionId,
                                                                    replicaId,
                                                                    completionMode,
                                                                    operationTimeout.TotalSeconds,
                                                                    token)).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RemoveReplicaAsync(
            ReplicaSelector replicaSelector,
            CompletionMode completionMode,
            bool forceRemove,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                                        () =>
                                                            this.fabricClient.FaultManager
                                                                .RemoveReplicaAsync(
                                                                    replicaSelector,
                                                                    completionMode,
                                                                    forceRemove,
                                                                    operationTimeout,
                                                                    token)).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RemoveReplicaAsync(
            string nodeName,
            Guid partitionId,
            long replicaId,
            CompletionMode completionMode,
            bool forceRemove,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                                        () =>
                                                            this.fabricClient.FaultManager
                                                                .RemoveReplicaAsync(
                                                                    nodeName,
                                                                    partitionId,
                                                                    replicaId,
                                                                    completionMode,
                                                                    forceRemove,
                                                                    operationTimeout.TotalSeconds,
                                                                    token)).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> ValidateApplicationAsync(
            Uri applicationName,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                                        () =>
                                                            this.fabricClient.TestManager
                                                                .ValidateApplicationAsync(
                                                                    applicationName,
                                                                    operationTimeout,
                                                                    token)).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> ValidateServiceAsync(
            Uri serviceName,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                                        () =>
                                                            this.fabricClient.TestManager
                                                                .ValidateServiceAsync(
                                                                    serviceName,
                                                                    operationTimeout,
                                                                    token)).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> InvokeDataLossAsync(
            PartitionSelector partitionSelector,
            DataLossMode datalossMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
#pragma warning disable 618

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                                          () =>
                                                              this.fabricClient.TestManager.InvokeDataLossAsync(
                                                                  partitionSelector,
                                                                  datalossMode,
                                                                  operationTimeout,
                                                                  token)).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);

#pragma warning restore 618
        }

        public override async Task<OperationResult> RestartPartitionAsync(
            PartitionSelector partitionSelector,
            RestartPartitionMode restartPartitionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
#pragma warning disable 618

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                                          () =>
                                                              this.fabricClient.TestManager.RestartPartitionAsync(
                                                                  partitionSelector,
                                                                  restartPartitionMode,
                                                                  operationTimeout,
                                                                  token)).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);

#pragma warning restore 618
        }

        public override async Task<OperationResult> MovePrimaryReplicaAsync(
            string nodeName,
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                                          () =>
                                                              this.fabricClient.FaultManager.MovePrimaryAsync(
                                                                  nodeName,
                                                                  partitionSelector,
                                                                  ignoreConstraints,
                                                                  operationTimeout,
                                                                  token)).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> MoveSecondaryReplicaAsync(
            string currentNodeName,
            string newNodeName,
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                                          () =>
                                                              this.fabricClient.FaultManager.MoveSecondaryAsync(
                                                                  currentNodeName,
                                                                  newNodeName,
                                                                  partitionSelector,
                                                                  ignoreConstraints,
                                                                  operationTimeout,
                                                                  token)).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> StartNodeNativeAsync(string nodeName, BigInteger instanceId, string ipAddressOrFQDN, int clusterConnectionPort, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.FaultManager.StartNodeUsingNodeNameAsync(nodeName, instanceId, ipAddressOrFQDN, clusterConnectionPort, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CreateApplicationAsync(ApplicationDescription applicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(applicationDescription, "applicationDescription");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ApplicationManager.CreateApplicationAsync(
                            applicationDescription,
                            timeout,
                            cancellationToken);
                });
            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UpdateApplicationAsync(ApplicationUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(updateDescription, "updateDescription");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ApplicationManager.UpdateApplicationAsync(
                            updateDescription,
                            timeout,
                            cancellationToken);
                });
            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> DeleteApplicationAsync(DeleteApplicationDescription deleteApplicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ApplicationManager.DeleteApplicationAsync(deleteApplicationDescription, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UpgradeApplicationAsync(ApplicationUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ApplicationManager.UpgradeApplicationAsync(
                        description,
                        timeout,
                        cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<string>> GetApplicationManifestAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string applicationManifest = string.Empty;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    applicationManifest = await this.fabricClient.ApplicationManager.GetApplicationManifestAsync(applicationTypeName, applicationTypeVersion, timeout, cancellationToken);
                });

            if (errorCode != 0)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<string>(errorCode);
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode<string>(applicationManifest);
        }

        public override async Task<OperationResult> UpgradeFabricAsync(FabricUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ClusterManager.UpgradeFabricAsync(
                        description,
                        timeout,
                        cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UpgradeFabricStandaloneAsync(ConfigurationUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ClusterManager.UpgradeConfigurationAsync(
                        description,
                        timeout,
                        cancellationToken);
                });
            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UpdateFabricUpgradeAsync(FabricUpgradeUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ClusterManager.UpdateFabricUpgradeAsync(
                        updateDescription,
                        timeout,
                        cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<ApplicationUpgradeProgress>> GetApplicationUpgradeProgressAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ApplicationUpgradeProgress upgradeProgress = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    upgradeProgress = await this.fabricClient.ApplicationManager.GetApplicationUpgradeProgressAsync(
                        name,
                        timeout,
                        cancellationToken);
                });

            OperationResult<ApplicationUpgradeProgress> operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationUpgradeProgress>(errorCode);
            if (upgradeProgress != null)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationUpgradeProgress>(upgradeProgress);
            }

            return operationResult;
        }

        public override async Task<OperationResult<FabricOrchestrationUpgradeProgress>> GetFabricConfigUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            FabricOrchestrationUpgradeProgress upgradeProgress = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    upgradeProgress = await this.fabricClient.ClusterManager.GetClusterConfigurationUpgradeStatusAsync(timeout, cancellationToken);
                });

            OperationResult<FabricOrchestrationUpgradeProgress> operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<FabricOrchestrationUpgradeProgress>(errorCode);
            if (upgradeProgress != null)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<FabricOrchestrationUpgradeProgress>(upgradeProgress);
            }

            return operationResult;
        }

        public override async Task<OperationResult<string>> GetFabricConfigurationAsync(string apiVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string result = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    result = await this.fabricClient.ClusterManager.GetClusterConfigurationAsync(apiVersion, timeout, cancellationToken);
                });

            if (errorCode != 0)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<string>(errorCode);
            }
            else
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<string>(result);
            }
        }

        public override async Task<OperationResult<string>> GetUpgradeOrchestrationServiceStateAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            string result = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    result = await this.fabricClient.ClusterManager.GetUpgradeOrchestrationServiceStateAsync(timeout, cancellationToken);
                });

            if (errorCode != 0)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<string>(errorCode);
            }
            else
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<string>(result);
            }
        }

        public override async Task<OperationResult<FabricUpgradeOrchestrationServiceState>> SetUpgradeOrchestrationServiceStateAsync(string state, TimeSpan timeout, CancellationToken cancellationToken)
        {
            FabricUpgradeOrchestrationServiceState result = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    result = await this.fabricClient.ClusterManager.SetUpgradeOrchestrationServiceStateAsync(state, timeout, cancellationToken);
                });

            if (errorCode != 0)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<FabricUpgradeOrchestrationServiceState>(errorCode);
            }
            else
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<FabricUpgradeOrchestrationServiceState>(result);
            }
        }

        public override async Task<OperationResult<FabricUpgradeProgress>> GetFabricUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            FabricUpgradeProgress upgradeProgress = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    upgradeProgress = await this.fabricClient.ClusterManager.GetFabricUpgradeProgressAsync(timeout, cancellationToken);
                });

            OperationResult<FabricUpgradeProgress> operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<FabricUpgradeProgress>(errorCode);
            if (upgradeProgress != null)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<FabricUpgradeProgress>(upgradeProgress);
            }

            return operationResult;
        }

        public override async Task<OperationResult> MoveNextApplicationUpgradeDomainAsync(ApplicationUpgradeProgress realProgress, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(realProgress, "realProgress");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ApplicationManager.MoveNextApplicationUpgradeDomainAsync(
                        realProgress,
                        timeout,
                        cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> MoveNextFabricUpgradeDomainAsync(FabricUpgradeProgress realProgress, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(realProgress, "realProgress");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ClusterManager.MoveNextFabricUpgradeDomainAsync(
                        realProgress,
                        timeout,
                        cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CreateNameAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
               delegate
               {
                   return this.fabricClient.PropertyManager.CreateNameAsync(name, timeout, cancellationToken);
               });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> DeleteNameAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
               delegate
               {
                   return this.fabricClient.PropertyManager.DeleteNameAsync(name, timeout, cancellationToken);
               });
            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RestartReplicaInternalAsync(string nodeName, Guid partitionId, long replicaOrInstanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ServiceManager.RestartReplicaAsync(nodeName, partitionId, replicaOrInstanceId, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RemoveReplicaInternalAsync(string nodeName, Guid partitionId, long replicaOrInstanceId, bool forceRemove, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ServiceManager.RemoveReplicaAsync(nodeName, partitionId, replicaOrInstanceId, forceRemove, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CreateServiceAsync(ServiceDescription winFabricServiceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ServiceManager.CreateServiceAsync(
                        winFabricServiceDescription,
                        timeout,
                        cancellationToken);
                });
            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CreateServiceFromTemplateAsync(
            Uri applicationName,
            Uri serviceName,
            string serviceDnsName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode,
            byte[] initializationData,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ServiceManager.CreateServiceFromTemplateAsync(
                        new ServiceFromTemplateDescription(
                            applicationName,
                            serviceName,
                            serviceDnsName,
                            serviceTypeName,
                            servicePackageActivationMode,
                            initializationData),
                        timeout,
                        cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> DeleteServiceAsync(DeleteServiceDescription deleteServiceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ServiceManager.DeleteServiceAsync(deleteServiceDescription, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UpdateServiceAsync(Uri name, ServiceUpdateDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ServiceManager.UpdateServiceAsync(name, description, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CreateServiceGroupAsync(ServiceGroupDescription winFabricServiceGroupDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ServiceGroupManager.CreateServiceGroupAsync(
                        winFabricServiceGroupDescription,
                        timeout,
                        cancellationToken);
                });
            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<ServiceDescription>> GetServiceDescriptionAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ServiceDescription serviceDescription = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    serviceDescription = await this.fabricClient.ServiceManager.GetServiceDescriptionAsync(
                        name,
                        timeout,
                        cancellationToken);
                });

            OperationResult<ServiceDescription> result = FabricClientState.CreateOperationResultFromNativeErrorCode<ServiceDescription>(errorCode);
            if (serviceDescription != null)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<ServiceDescription>(serviceDescription);
            }

            return result;
        }

        public override async Task<OperationResult> DeleteServiceGroupAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ServiceGroupManager.DeleteServiceGroupAsync(name, timeout, cancellationToken);
                });
            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override Task<ResolvedServicePartition> ResolveServiceAsync(Uri name, object partitionKey, ResolvedServicePartition previousServicePartition, TimeSpan timeout, CancellationToken cancellationToken)
        {
            if (partitionKey == null)
            {
                return this.fabricClient.ServiceManager.ResolveServicePartitionAsync(name, previousServicePartition, timeout);
            }
            else if (partitionKey is long || partitionKey is int)
            {
                return this.fabricClient.ServiceManager.ResolveServicePartitionAsync(name, System.Convert.ToInt64(partitionKey, CultureInfo.InvariantCulture), previousServicePartition, timeout, cancellationToken);
            }

            string partitionKeyAsString = partitionKey as string;
            if (partitionKeyAsString != null)
            {
                return this.fabricClient.ServiceManager.ResolveServicePartitionAsync(name, partitionKeyAsString, previousServicePartition, timeout, cancellationToken);
            }
            else
            {
                throw new ArgumentException("partitionKey is of invalid type");
            }
        }

        public override async Task<OperationResult> PutPropertyAsync(Uri name, string propertyName, object data, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(data, "data");
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    if (data.GetType().Equals(typeof(string)))
                    {
                        return this.fabricClient.PropertyManager.PutPropertyAsync(name, propertyName, (string)data, timeout, cancellationToken);
                    }
                    else if (data.GetType().Equals(typeof(byte[])))
                    {
                        return this.fabricClient.PropertyManager.PutPropertyAsync(name, propertyName, (byte[])data, timeout, cancellationToken);
                    }
                    else if (data.GetType().Equals(typeof(double)))
                    {
                        return this.fabricClient.PropertyManager.PutPropertyAsync(name, propertyName, (double)data, timeout, cancellationToken);
                    }
                    else if (data.GetType().Equals(typeof(long)))
                    {
                        return this.fabricClient.PropertyManager.PutPropertyAsync(name, propertyName, (long)data, timeout, cancellationToken);
                    }
                    else if (data.GetType().Equals(typeof(Guid)))
                    {
                        return this.fabricClient.PropertyManager.PutPropertyAsync(name, propertyName, (Guid)data, timeout, cancellationToken);
                    }
                    else
                    {
                        throw new InvalidCastException(string.Format(CultureInfo.InvariantCulture, "PropertyValue {0} cannot be converted.", data.GetType()));
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> PutCustomPropertyOperationAsync(Uri name, PutCustomPropertyOperation putCustomPropertyOperation, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(name, "name");
            ThrowIf.Null(putCustomPropertyOperation, "putCustomPropertyOperation");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.PropertyManager.PutCustomPropertyOperationAsync(
                        name,
                        putCustomPropertyOperation,
                        timeout,
                        cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<NamedProperty>> GetPropertyAsync(Uri name, string propertyName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(name, "name");

            NamedProperty property = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    property = await this.fabricClient.PropertyManager.GetPropertyAsync(
                        name,
                        propertyName,
                        timeout,
                        cancellationToken);
                });

            OperationResult<NamedProperty> result = FabricClientState.CreateOperationResultFromNativeErrorCode<NamedProperty>(errorCode);
            if (property != null)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<NamedProperty>(property);
            }

            return result;
        }

        public override async Task<OperationResult> DeletePropertyAsync(Uri name, string propertyName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(name, "name");
            ThrowIf.NullOrEmpty(propertyName, "propertyName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.PropertyManager.DeletePropertyAsync(
                        name,
                        propertyName,
                        timeout,
                        cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<NamedPropertyMetadata>> GetPropertyMetadataAsync(Uri name, string propertyName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(name, "name");

            NamedPropertyMetadata metadata = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    metadata = await this.fabricClient.PropertyManager.GetPropertyMetadataAsync(
                        name,
                        propertyName,
                        timeout,
                        cancellationToken);
                });

            OperationResult<NamedPropertyMetadata> result = FabricClientState.CreateOperationResultFromNativeErrorCode<NamedPropertyMetadata>(errorCode);
            if (metadata != null)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<NamedPropertyMetadata>(metadata);
            }

            return result;
        }

        public override async Task<OperationResult<PropertyBatchResult>> SubmitPropertyBatchAsync(Uri name, ICollection<PropertyBatchOperation> operations, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(name, "name");

            PropertyBatchResult propertyBatchResult = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    propertyBatchResult = await this.fabricClient.PropertyManager.SubmitPropertyBatchAsync(
                        name,
                        operations,
                        timeout,
                        cancellationToken);
                });
            OperationResult<PropertyBatchResult> result = FabricClientState.CreateOperationResultFromNativeErrorCode<PropertyBatchResult>(errorCode);
            if (propertyBatchResult != null)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<PropertyBatchResult>(propertyBatchResult);
            }

            return result;
        }

        public override async Task<OperationResult> ActivateNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ClusterManager.ActivateNodeAsync(nodeName, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> DeactivateNodeAsync(string nodeName, NodeDeactivationIntent deactivationIntent, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ClusterManager.DeactivateNodeAsync(nodeName, deactivationIntent, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RemoveNodeStateAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ClusterManager.RemoveNodeStateAsync(nodeName, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RecoverPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.ClusterManager.RecoverPartitionsAsync(timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<NameEnumerationResult>> EnumerateSubNamesAsync(Uri name, NameEnumerationResult previousResult, bool recursive, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NameEnumerationResult enumerationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    enumerationResult = await this.fabricClient.PropertyManager.EnumerateSubNamesAsync(
                        name,
                        previousResult,
                        recursive,
                        timeout,
                        cancellationToken);
                });
            OperationResult<NameEnumerationResult> result = FabricClientState.CreateOperationResultFromNativeErrorCode<NameEnumerationResult>(errorCode);
            if (enumerationResult != null)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<NameEnumerationResult>(enumerationResult);
            }

            return result;
        }

        public override async Task<OperationResult<PropertyEnumerationResult>> EnumeratePropertiesAsync(Uri name, PropertyEnumerationResult previousResult, bool includeValue, TimeSpan timeout, CancellationToken cancellationToken)
        {
            PropertyEnumerationResult enumerationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    enumerationResult = await this.fabricClient.PropertyManager.EnumeratePropertiesAsync(
                        name,
                        includeValue,
                        previousResult,
                        timeout,
                        cancellationToken);
                });

            OperationResult<PropertyEnumerationResult> result = FabricClientState.CreateOperationResultFromNativeErrorCode<PropertyEnumerationResult>(errorCode);
            if (enumerationResult != null)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<PropertyEnumerationResult>(enumerationResult);
            }

            return result;
        }

        public override async Task<OperationResult<Node>> GetNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NodeList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     queryResult = await this.fabricClient.QueryManager.GetNodeListAsync(
                         nodeName,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<Node>(
                errorCode,
                queryResult != null ? queryResult.FirstOrDefault() : null);
        }

        public override async Task<OperationResult<NodeList>> GetNodeListAsync(NodeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NodeList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     queryResult = await this.fabricClient.QueryManager.GetNodePagedListAsync(
                         queryDescription,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<NodeList>(errorCode, queryResult);
        }

        public override async Task<OperationResult<ServiceType[]>> GetServiceTypeListAsync(string applicationTypeName, string applicationTypeVersion, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            IList<ServiceType> queryResult = new List<ServiceType>();

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     queryResult = await this.fabricClient.QueryManager.GetServiceTypeListAsync(
                         applicationTypeName,
                         applicationTypeVersion,
                         serviceTypeNameFilter,
                         timeout,
                         cancellationToken);
                 });

            return OperationResult<ServiceType>.GetOperationResultArray(errorCode, queryResult);
        }

        public override async Task<OperationResult<ServicePartitionList>> GetPartitionListAsync(Uri serviceName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ServicePartitionList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     queryResult = await this.fabricClient.QueryManager.GetPartitionListAsync(
                         serviceName,
                         null,
                         continuationToken,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ServicePartitionList>(errorCode, queryResult);
        }

        public override async Task<OperationResult<ApplicationType[]>> GetApplicationTypeListAsync(string applicationTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ApplicationTypeList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     queryResult = await this.fabricClient.QueryManager.GetApplicationTypeListAsync(
                         applicationTypeNameFilter,
                         timeout,
                         cancellationToken);
                 });

            return OperationResult<ApplicationType>.GetOperationResultArray(errorCode, queryResult);
        }

        public override async Task<OperationResult<ApplicationTypePagedList>> GetApplicationTypePagedListAsync(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ApplicationTypePagedList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     queryResult = await this.fabricClient.QueryManager.GetApplicationTypePagedListAsync(
                         queryDescription,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationTypePagedList>(errorCode, queryResult);
        }

        public override async Task<OperationResult<ApplicationList>> GetApplicationPagedListAsync(ApplicationQueryDescription applicationQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ApplicationList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     queryResult = await this.fabricClient.QueryManager.GetApplicationPagedListAsync(
                         applicationQueryDescription,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationList>(errorCode, queryResult);
        }

        public override async Task<OperationResult<ServiceReplicaList>> GetReplicaListAsync(Guid partitionId, long replicaIdOrInstanceId, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ServiceReplicaList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     queryResult = await this.fabricClient.QueryManager.GetReplicaListAsync(
                         partitionId,
                         replicaIdOrInstanceId,
                         ServiceReplicaStatusFilter.All,
                         continuationToken,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ServiceReplicaList>(errorCode, queryResult);
        }

        public override async Task<OperationResult<ServiceList>> GetServicePagedListAsync(ServiceQueryDescription serviceQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ServiceList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     queryResult = await this.fabricClient.QueryManager.GetServicePagedListAsync(
                         serviceQueryDescription,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ServiceList>(errorCode, queryResult);
        }

        public override async Task<OperationResult<string>> GetClusterManifestAsync(ClusterManifestQueryDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string clusterManifest = string.Empty;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    clusterManifest = await this.fabricClient.ClusterManager.GetClusterManifestAsync(description, timeout, cancellationToken);
                });

            if (errorCode != 0)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<string>(errorCode);
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode<string>(clusterManifest);
        }

        public override async Task<OperationResult<string>> GetServiceManifestAsync(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string serviceManifest = string.Empty;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    serviceManifest = await this.fabricClient.ServiceManager.GetServiceManifestAsync(applicationTypeName, applicationTypeVersion, serviceManifestName, timeout, cancellationToken);
                });

            if (errorCode != 0)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<string>(errorCode);
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode<string>(serviceManifest);
        }

        public override async Task<OperationResult<NodeLoadInformation>> GetNodeLoadAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NodeLoadInformation nodeLoad = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    nodeLoad = await this.fabricClient.QueryManager.GetNodeLoadInformationAsync(nodeName, timeout, cancellationToken);
                });

            if (errorCode != 0)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<NodeLoadInformation>(errorCode);
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode<NodeLoadInformation>(errorCode, nodeLoad);
        }

        public override async Task<OperationResult<ClusterLoadInformation>> GetClusterLoadAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            ClusterLoadInformation clusterLoad = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    clusterLoad = await this.fabricClient.QueryManager.GetClusterLoadInformationAsync(timeout, cancellationToken);
                });

            if (errorCode != 0)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<ClusterLoadInformation>(errorCode);
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ClusterLoadInformation>(errorCode, clusterLoad);
        }

        public override async Task<OperationResult<ReplicaLoadInformation>> GetReplicaLoadAsync(Guid partitionId, long replicaId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ReplicaLoadInformation replicaLoad = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    replicaLoad = await this.fabricClient.QueryManager.GetReplicaLoadInformationAsync(partitionId, replicaId, timeout, cancellationToken);
                });

            if (errorCode != 0)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<ReplicaLoadInformation>(errorCode);
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ReplicaLoadInformation>(errorCode, replicaLoad);
        }

        public override async Task<OperationResult<PartitionLoadInformation>> GetPartitionLoadAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            PartitionLoadInformation partitionLoad = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    partitionLoad = await this.fabricClient.QueryManager.GetPartitionLoadInformationAsync(partitionId, timeout, cancellationToken);
                });

            if (errorCode != 0)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<PartitionLoadInformation>(errorCode);
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode<PartitionLoadInformation>(errorCode, partitionLoad);
        }

        public override async Task<OperationResult<ApplicationLoadInformation>> GetApplicationLoadAsync(string applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ApplicationLoadInformation applicationLoad = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    applicationLoad = await this.fabricClient.QueryManager.GetApplicationLoadInformationAsync(applicationName, timeout, cancellationToken);
                });

            if (errorCode != 0)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationLoadInformation>(errorCode);
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationLoadInformation>(errorCode, applicationLoad);
        }

        ////
        //// Health Client APIs
        ////

        public override async Task<OperationResult<ApplicationHealth>> GetApplicationHealthAsync(ApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");
            ApplicationHealth applicationHealth = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    // Call multiple overloads for better coverage
                    if (queryDescription.DeployedApplicationsFilter != null || queryDescription.EventsFilter != null || queryDescription.ServicesFilter != null || queryDescription.HealthStatisticsFilter != null)
                    {
                        applicationHealth = await this.fabricClient.HealthManager.GetApplicationHealthAsync(queryDescription, timeout, cancellationToken);
                    }
                    else if (queryDescription.HealthPolicy == null)
                    {
                        applicationHealth = await this.fabricClient.HealthManager.GetApplicationHealthAsync(queryDescription.ApplicationName, timeout, cancellationToken);
                    }
                    else
                    {
                        applicationHealth = await this.fabricClient.HealthManager.GetApplicationHealthAsync(queryDescription.ApplicationName, queryDescription.HealthPolicy, timeout, cancellationToken);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationHealth>(errorCode, applicationHealth);
        }

        public override async Task<OperationResult<ClusterHealth>> GetClusterHealthAsync(ClusterHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");
            ClusterHealth clusterHealth = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    if ((queryDescription.ApplicationHealthPolicyMap != null && queryDescription.ApplicationHealthPolicyMap.Count > 0) ||
                        queryDescription.ApplicationsFilter != null || queryDescription.EventsFilter != null || queryDescription.NodesFilter != null || queryDescription.HealthStatisticsFilter != null)
                    {
                        clusterHealth = await this.fabricClient.HealthManager.GetClusterHealthAsync(queryDescription, timeout, cancellationToken);
                    }
                    else if (queryDescription.HealthPolicy == null)
                    {
                        clusterHealth = await this.fabricClient.HealthManager.GetClusterHealthAsync(timeout, cancellationToken);
                    }
                    else
                    {
                        clusterHealth = await this.fabricClient.HealthManager.GetClusterHealthAsync(queryDescription.HealthPolicy, timeout, cancellationToken);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ClusterHealth>(errorCode, clusterHealth);
        }

        public override async Task<OperationResult<ClusterHealthChunk>> GetClusterHealthChunkAsync(ClusterHealthChunkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");
            ClusterHealthChunk clusterHealthChunk = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    if (queryDescription.ApplicationHealthPolicies.Count > 0 ||
                         queryDescription.ApplicationFilters.Count > 0 ||
                         queryDescription.NodeFilters.Count > 0 ||
                         queryDescription.ClusterHealthPolicy != null)
                    {
                        clusterHealthChunk = await this.fabricClient.HealthManager.GetClusterHealthChunkAsync(queryDescription, timeout, cancellationToken);
                    }
                    else
                    {
                        clusterHealthChunk = await this.fabricClient.HealthManager.GetClusterHealthChunkAsync(timeout, cancellationToken);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ClusterHealthChunk>(errorCode, clusterHealthChunk);
        }

        public override async Task<OperationResult<NodeHealth>> GetNodeHealthAsync(NodeHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");
            NodeHealth nodeHealth = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    if (queryDescription.EventsFilter != null)
                    {
                        nodeHealth = await this.fabricClient.HealthManager.GetNodeHealthAsync(queryDescription, timeout, cancellationToken);
                    }
                    else if (queryDescription.HealthPolicy == null)
                    {
                        nodeHealth = await this.fabricClient.HealthManager.GetNodeHealthAsync(queryDescription.NodeName, timeout, cancellationToken);
                    }
                    else
                    {
                        nodeHealth = await this.fabricClient.HealthManager.GetNodeHealthAsync(queryDescription.NodeName, queryDescription.HealthPolicy, timeout, cancellationToken);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<NodeHealth>(errorCode, nodeHealth);
        }

        public override async Task<OperationResult<PartitionHealth>> GetPartitionHealthAsync(PartitionHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");
            PartitionHealth partitionHealth = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    if (queryDescription.EventsFilter != null || queryDescription.ReplicasFilter != null || queryDescription.HealthStatisticsFilter != null)
                    {
                        partitionHealth = await this.fabricClient.HealthManager.GetPartitionHealthAsync(queryDescription, timeout, cancellationToken);
                    }
                    else if (queryDescription.HealthPolicy == null)
                    {
                        partitionHealth = await this.fabricClient.HealthManager.GetPartitionHealthAsync(queryDescription.PartitionId, timeout, cancellationToken);
                    }
                    else
                    {
                        partitionHealth = await this.fabricClient.HealthManager.GetPartitionHealthAsync(queryDescription.PartitionId, queryDescription.HealthPolicy, timeout, cancellationToken);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<PartitionHealth>(errorCode, partitionHealth);
        }

        public override async Task<OperationResult<ReplicaHealth>> GetReplicaHealthAsync(ReplicaHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");
            ReplicaHealth replicaHealth = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    if (queryDescription.EventsFilter != null)
                    {
                        replicaHealth = await this.fabricClient.HealthManager.GetReplicaHealthAsync(queryDescription, timeout, cancellationToken);
                    }
                    else if (queryDescription.HealthPolicy == null)
                    {
                        replicaHealth = await this.fabricClient.HealthManager.GetReplicaHealthAsync(queryDescription.PartitionId, queryDescription.ReplicaOrInstanceId, timeout, cancellationToken);
                    }
                    else
                    {
                        replicaHealth = await this.fabricClient.HealthManager.GetReplicaHealthAsync(queryDescription.PartitionId, queryDescription.ReplicaOrInstanceId, queryDescription.HealthPolicy, timeout, cancellationToken);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ReplicaHealth>(errorCode, replicaHealth);
        }

        public override async Task<OperationResult<ServiceHealth>> GetServiceHealthAsync(ServiceHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");

            ServiceHealth serviceHealth = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    if (queryDescription.EventsFilter != null || queryDescription.PartitionsFilter != null || queryDescription.HealthStatisticsFilter != null)
                    {
                        serviceHealth = await this.fabricClient.HealthManager.GetServiceHealthAsync(queryDescription, timeout, cancellationToken);
                    }
                    else if (queryDescription.HealthPolicy == null)
                    {
                        serviceHealth = await this.fabricClient.HealthManager.GetServiceHealthAsync(queryDescription.ServiceName, timeout, cancellationToken);
                    }
                    else
                    {
                        serviceHealth = await this.fabricClient.HealthManager.GetServiceHealthAsync(queryDescription.ServiceName, queryDescription.HealthPolicy, timeout, cancellationToken);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ServiceHealth>(errorCode, serviceHealth);
        }

        public override async Task<OperationResult<DeployedServicePackageHealth>> GetDeployedServicePackageHealthAsync(DeployedServicePackageHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");

            DeployedServicePackageHealth deployedServicePackageHealth = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    if (queryDescription.EventsFilter != null)
                    {
                        deployedServicePackageHealth = await this.fabricClient.HealthManager.GetDeployedServicePackageHealthAsync(queryDescription, timeout, cancellationToken);
                    }
                    else if (queryDescription.HealthPolicy == null)
                    {
                        deployedServicePackageHealth = await this.fabricClient.HealthManager.GetDeployedServicePackageHealthAsync(queryDescription.ApplicationName, queryDescription.ServiceManifestName, queryDescription.NodeName, timeout, cancellationToken);
                    }
                    else
                    {
                        deployedServicePackageHealth = await this.fabricClient.HealthManager.GetDeployedServicePackageHealthAsync(queryDescription.ApplicationName, queryDescription.ServiceManifestName, queryDescription.NodeName, queryDescription.HealthPolicy, timeout, cancellationToken);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<DeployedServicePackageHealth>(errorCode, deployedServicePackageHealth);
        }

        public override async Task<OperationResult<DeployedApplicationHealth>> GetDeployedApplicationHealthAsync(DeployedApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");

            DeployedApplicationHealth deployedApplicationHealth = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    if (queryDescription.EventsFilter != null || queryDescription.DeployedServicePackagesFilter != null || queryDescription.HealthStatisticsFilter != null)
                    {
                        deployedApplicationHealth = await this.fabricClient.HealthManager.GetDeployedApplicationHealthAsync(queryDescription, timeout, cancellationToken);
                    }
                    else if (queryDescription.HealthPolicy == null)
                    {
                        deployedApplicationHealth = await this.fabricClient.HealthManager.GetDeployedApplicationHealthAsync(queryDescription.ApplicationName, queryDescription.NodeName, timeout, cancellationToken);
                    }
                    else
                    {
                        deployedApplicationHealth = await this.fabricClient.HealthManager.GetDeployedApplicationHealthAsync(queryDescription.ApplicationName, queryDescription.NodeName, queryDescription.HealthPolicy, timeout, cancellationToken);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<DeployedApplicationHealth>(errorCode, deployedApplicationHealth);
        }

        public override OperationResult ReportHealth(HealthReport healthReport, HealthReportSendOptions sendOptions)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = FabricClientState.HandleException(
                delegate
                {
                    this.fabricClient.HealthManager.ReportHealth(healthReport, sendOptions);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> AddUnreliableTransportBehaviorAsync(string nodeName, string name, System.Fabric.Testability.UnreliableTransportBehavior behavior, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.TestManager.AddUnreliableTransportBehaviorAsync(nodeName, name, behavior.Convert(), timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RemoveUnreliableTransportBehaviorAsync(string nodeName, string name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.TestManager.RemoveUnreliableTransportBehaviorAsync(nodeName, name, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> AddUnreliableLeaseBehaviorAsync(string nodeName, string behaviorString, string alias, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.TestManager.AddUnreliableLeaseBehaviorAsync(nodeName, behaviorString, alias, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RemoveUnreliableLeaseBehaviorAsync(string nodeName, string alias, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.TestManager.RemoveUnreliableLeaseBehaviorAsync(nodeName, alias, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<Int64>> CreateRepairTaskAsync(WinFabricRepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Int64 commitVersion = Int64.MaxValue;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    RepairTask fabricRepairTask = FabricClientState.Convert(repairTask);
                    commitVersion = await this.fabricClient.RepairManager.CreateRepairTaskAsync(fabricRepairTask, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<Int64>(errorCode, commitVersion);
        }

        public override async Task<OperationResult<Int64>> CancelRepairTaskAsync(string repairTaskId, Int64 version, bool requestAbort, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Int64 commitVersion = Int64.MaxValue;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    commitVersion = await this.fabricClient.RepairManager.CancelRepairTaskAsync(repairTaskId, version, requestAbort, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<Int64>(errorCode, commitVersion);
        }

        public override async Task<OperationResult<Int64>> ForceApproveRepairTaskAsync(string repairTaskId, Int64 version, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Int64 commitVersion = Int64.MaxValue;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    commitVersion = await this.fabricClient.RepairManager.ForceApproveRepairTaskAsync(repairTaskId, version, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<Int64>(errorCode, commitVersion);
        }

        public override async Task<OperationResult> DeleteRepairTaskAsync(string repairTaskId, Int64 version, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
            delegate
            {
                return this.fabricClient.RepairManager.DeleteRepairTaskAsync(repairTaskId, version, timeout, cancellationToken);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<Int64>> UpdateRepairExecutionStateAsync(WinFabricRepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Int64 commitVersion = Int64.MaxValue;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    // Need to get the repair task from RM and update it because some fields, such as Flags, are readonly in RepairTask
                    // and in some state transition those fields are immutable
                    var repairTasks = await this.fabricClient.RepairManager.GetRepairTaskListAsync(repairTask.TaskId, RepairTaskStateFilter.All, null, timeout, cancellationToken);
                    ThrowIf.IsFalse(repairTasks.Count == 1, "repairTasks.Count");
                    var fabricRepairTask = repairTasks[0];
                    FabricClientState.UpdateRepairTask(repairTask, fabricRepairTask);

                    commitVersion = await this.fabricClient.RepairManager.UpdateRepairExecutionStateAsync(fabricRepairTask, timeout, cancellationToken);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<Int64>(errorCode, commitVersion);
        }

        public override async Task<OperationResult<WinFabricRepairTask[]>> GetRepairTaskListAsync(string taskIdFilter, WinFabricRepairTaskStateFilter stateFilter, string executorFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceSource, "SystemFabricClient  GetRepairTaskListAsync {0} {1} {2}", taskIdFilter, stateFilter, executorFilter);

            RepairTaskList queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    queryResult = await this.fabricClient.RepairManager.GetRepairTaskListAsync(
                        taskIdFilter,
                        FabricClientState.Convert(stateFilter),
                        executorFilter,
                        timeout,
                        cancellationToken);
                });

            return FabricClientState.ConvertToWinFabricRepairTaskList(queryResult, errorCode);
        }

        public override async Task<OperationResult> InvokeQuorumLossAsync(
            PartitionSelector partitionSelector,
            QuorumLossMode quorumlossMode,
            TimeSpan quorumlossDuration,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
#pragma warning disable 618

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                              () =>
                                                  this.fabricClient.TestManager.InvokeQuorumLossAsync(
                                                                                                    partitionSelector,
                                                                                                    quorumlossMode,
                                                                                                    quorumlossDuration,
                                                                                                    operationTimeout,
                                                                                                    token)).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);

#pragma warning restore 618
        }

        public override async Task<OperationResult> StartPartitionDataLossAsync(Guid operationId, PartitionSelector partitionSelector, DataLossMode dataLossMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceSource, "SystemFabricClient StartPartitionDataLossAsync operationId={0} dataLossMode={1}", operationId, dataLossMode);

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.TestManager.StartPartitionDataLossAsync(operationId, partitionSelector, dataLossMode, timeout, cancellationToken);
                });
            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<PartitionDataLossProgress>> GetPartitionDataLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            PartitionDataLossProgress result = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     result = await this.fabricClient.TestManager.GetPartitionDataLossProgressAsync(
                         operationId,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<PartitionDataLossProgress>(errorCode, result);
        }

        public override async Task<OperationResult> StartPartitionQuorumLossAsync(Guid operationId, PartitionSelector partitionSelector, QuorumLossMode quorumlossMode, TimeSpan quorumlossDuration, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceSource, "SystemFabricClient StartPartitionQuorumLossAsync operationId={0} quorumlossMode={1}", operationId, quorumlossMode);

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.TestManager.StartPartitionQuorumLossAsync(operationId, partitionSelector, quorumlossMode, quorumlossDuration, operationTimeout, cancellationToken);
                });
            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<PartitionQuorumLossProgress>> GetPartitionQuorumLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            PartitionQuorumLossProgress result = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     result = await this.fabricClient.TestManager.GetPartitionQuorumLossProgressAsync(
                         operationId,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<PartitionQuorumLossProgress>(errorCode, result);
        }

        public override async Task<OperationResult> StartPartitionRestartAsync(Guid operationId, PartitionSelector partitionSelector, RestartPartitionMode restartPartitionMode, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceSource, "SystemFabricClient StartPartitionRestartAsync operationId={0} restartPartitionMode={1}", operationId, restartPartitionMode);

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                delegate
                {
                    return this.fabricClient.TestManager.StartPartitionRestartAsync(operationId, partitionSelector, restartPartitionMode, operationTimeout, cancellationToken);
                });
            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<PartitionRestartProgress>> GetPartitionRestartProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            PartitionRestartProgress result = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     result = await this.fabricClient.TestManager.GetPartitionRestartProgressAsync(
                         operationId,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<PartitionRestartProgress>(errorCode, result);
        }

        public override async Task<OperationResult> CancelTestCommandAsync(Guid operationId, bool force, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     await this.fabricClient.TestManager.CancelTestCommandAsync(
                         operationId,
                         force,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<TestCommandStatusList>> GetTestCommandStatusListAsync(TestCommandStateFilter stateFilter, TestCommandTypeFilter typeFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            TestCommandStatusList result = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     result = await this.fabricClient.TestManager.GetTestCommandStatusListAsync(
                         stateFilter,
                         typeFilter,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<TestCommandStatusList>(errorCode, result);
        }

        public override async Task<OperationResult> StartChaosAsync(ChaosParameters chaosTestScenarioParameters, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                                          async delegate
                                                              {
                                                                  await this.fabricClient.TestManager.StartChaosAsync(
                                                                      chaosTestScenarioParameters,
                                                                      timeout,
                                                                      cancellationToken).ConfigureAwait(false);
                                                              }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<ChaosDescription>> GetChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ChaosDescription result = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    result = await this.fabricClient.TestManager.GetChaosAsync(
                        timeout,
                        cancellationToken).ConfigureAwait(false);
                }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ChaosDescription>(errorCode, result);
        }

        public override async Task<OperationResult<ChaosScheduleDescription>> GetChaosScheduleAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ChaosScheduleDescription result = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    result = await this.fabricClient.TestManager.GetChaosScheduleAsync(
                        timeout,
                        cancellationToken).ConfigureAwait(false);
                }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ChaosScheduleDescription>(errorCode, result);
        }

        public override async Task<OperationResult> SetChaosScheduleAsync(
            string scheduleJson,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            var schedule = this.GetDeserializedObject<ChaosScheduleDescription>(scheduleJson);

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    await this.fabricClient.TestManager.SetChaosScheduleAsync(
                        schedule,
                        timeout,
                        cancellationToken).ConfigureAwait(false);
                }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<ChaosEventsSegment>> GetChaosEventsAsync(
            DateTime startTimeUtc,
            DateTime endTimeUtc,
            string continuationToken,
            long maxResults,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            var filter = new ChaosEventsSegmentFilter(startTimeUtc, endTimeUtc);
            ChaosEventsSegment result = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                async delegate
                {
                    if (string.IsNullOrEmpty(continuationToken))
                    {
                        result = await this.fabricClient.TestManager.GetChaosEventsAsync(
                                        filter,
                                        maxResults,
                                        timeout,
                                        cancellationToken).ConfigureAwait(false);
                    }
                    else
                    {
                        result = await this.fabricClient.TestManager.GetChaosEventsAsync(
                                        continuationToken,
                                        maxResults,
                                        timeout,
                                        cancellationToken).ConfigureAwait(false);
                    }
                }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ChaosEventsSegment>(errorCode, result);
        }

        public override async Task<OperationResult<ChaosReport>> GetChaosReportAsync(
            DateTime startTimeUtc,
            DateTime endTimeUtc,
            string continuationToken,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ChaosReportFilter filter = new ChaosReportFilter(startTimeUtc, endTimeUtc);
            ChaosReport result = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                                          async delegate
                                                              {
                                                                  if (string.IsNullOrEmpty(continuationToken))
                                                                  {
                                                                      result = await this.fabricClient.TestManager.GetChaosReportAsync(
                                                                                   filter,
                                                                                   timeout,
                                                                                   cancellationToken).ConfigureAwait(false);
                                                                  }
                                                                  else
                                                                  {
                                                                      result = await this.fabricClient.TestManager.GetChaosReportAsync(
                                                                                   continuationToken,
                                                                                   timeout,
                                                                                   cancellationToken).ConfigureAwait(false);
                                                                  }
                                                              }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ChaosReport>(errorCode, result);
        }

        public override async Task<OperationResult> StopChaosAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                                                          async delegate
                                                              {
                                                                  await this.fabricClient.TestManager.StopChaosAsync(
                                                                      timeout,
                                                                      cancellationToken).ConfigureAwait(false);
                                                              }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<ServiceNameResult>> GetServiceNameAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ServiceNameResult queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     queryResult = await this.fabricClient.QueryManager.GetServiceNameAsync(
                         partitionId,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode, queryResult);
        }

        public override async Task<OperationResult<ApplicationNameResult>> GetApplicationNameAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ApplicationNameResult queryResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     queryResult = await this.fabricClient.QueryManager.GetApplicationNameAsync(
                         serviceName,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode, queryResult);
        }

        public override async Task<OperationResult> StartNodeTransitionAsync(
            NodeTransitionDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     await this.fabricClient.TestManager.StartNodeTransitionAsync(
                         description,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        // "nodeName" is not used.  Included to match RestFabricClient version.
        public override async Task<OperationResult<NodeTransitionProgress>> GetNodeTransitionProgressAsync(
            Guid operationId,
            string nodeName,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            NodeTransitionProgress result = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     result = await this.fabricClient.TestManager.GetNodeTransitionProgressAsync(
                         operationId,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<NodeTransitionProgress>(errorCode, result);
        }

        // DeployServicePackageToNode
        public override async Task<OperationResult> CopyWindowsFabricServicePackageToNodeAsync(string serviceManifestName, string applicationTypeName, string applicationTypeVersion, string nodeName, PackageSharingPolicyDescription[] policies, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(serviceManifestName, "serviceManifestName");
            ThrowIf.NullOrEmpty(applicationTypeName, "applicationTypeName");
            ThrowIf.NullOrEmpty(applicationTypeVersion, "applicationTypeVersion");
            ThrowIf.NullOrEmpty(nodeName, "nodeName");
            var sharingPolicies = new PackageSharingPolicyList();
            foreach (var policy in policies)
            {
                sharingPolicies.PackageSharingPolicies.Add(new Fabric.PackageSharingPolicy(policy.PackageName, (PackageSharingPolicyScope)policy.Scope));
            }

            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     await fabricClient.ApplicationManager.DeployServicePackageToNode(
                          applicationTypeName,
                          applicationTypeVersion,
                          serviceManifestName,
                          sharingPolicies,
                          nodeName,
                          timeout,
                          cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<NodeTransitionProgress>(errorCode);
        }

        public override Task<OperationResult<string>> InvokeEncryptSecretAsync(string certificateThumbprint, string certificateStoreLocation, string text, CancellationToken cancellationToken)
        {
            string result = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = FabricClientState.HandleException(
                delegate
                {
                    result = EncryptionUtility.EncryptText(
                        text,
                        certificateThumbprint,
                        X509Credentials.StoreNameDefault,
                        (StoreLocation)Enum.Parse(typeof(StoreLocation),
                        certificateStoreLocation), null);
                });

            return Task.FromResult(FabricClientState.CreateOperationResultFromNativeErrorCode<string>(errorCode, result));
        }

        public override async Task<OperationResult<string>> InvokeInfrastructureCommandAsync(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string result = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
                 async delegate
                 {
                     result = await this.fabricClient.InfrastructureManager.InvokeInfrastructureCommandAsync(
                         serviceName,
                         command,
                         timeout,
                         cancellationToken);
                 });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<string>(errorCode, result);
        }

        private static bool IsPartitionKeyInRange(ServicePartitionInformation info, object partitionKey)
        {
            bool result = false;

            if (info.Kind == ServicePartitionKind.Int64Range)
            {
                Int64RangePartitionInformation actualPartitionInfo = info as Int64RangePartitionInformation;
                long actualPartitionKey = (long)partitionKey;

                if (actualPartitionInfo.LowKey <= actualPartitionKey && actualPartitionKey <= actualPartitionInfo.HighKey)
                {
                    result = true;
                }
            }
            else if (info.Kind == ServicePartitionKind.Named)
            {
                NamedPartitionInformation actualPartitionInfo = info as NamedPartitionInformation;
                string actualPartitionKey = (string)partitionKey;

                if (actualPartitionInfo.Name == actualPartitionKey)
                {
                    result = true;
                }
            }
            else if (info.Kind == ServicePartitionKind.Singleton)
            {
                if (partitionKey == null)
                {
                    result = true;
                }
            }

            return result;
        }

        private long RegisterServicePartitionResolutionChangeHandlerInner(Uri name, object partitionKey, ServicePartitionResolutionChangeHandler callback)
        {
            if (partitionKey == null)
            {
#pragma warning disable 0618 // obsolete
                return this.fabricClient.ServiceManager.RegisterServicePartitionResolutionChangeHandler(name, callback);
            }
            else if (partitionKey is long || partitionKey is int)
            {
                return this.fabricClient.ServiceManager.RegisterServicePartitionResolutionChangeHandler(name, System.Convert.ToInt64(partitionKey, CultureInfo.InvariantCulture), callback);
            }

            string partitionKeyAsString = partitionKey as string;
            if (partitionKeyAsString != null)
            {
                return this.fabricClient.ServiceManager.RegisterServicePartitionResolutionChangeHandler(name, partitionKeyAsString, callback);
#pragma warning restore 0618
            }
            else
            {
                throw new ArgumentException("partitionKey is of invalid type");
            }
        }

        private void UnregisterServicePartitionResolutionChangeHandlerInner(long callbackId)
        {
            this.fabricClient.ServiceManager.UnregisterServicePartitionResolutionChangeHandler(callbackId);
        }

        private T GetDeserializedObject<T>(string json)
        {
            this.jsonSerializer = JsonSerializerImplDllLoader.TryGetJsonSerializerImpl();

            if (this.jsonSerializer == null)
            {
                // Fail if serializer was not loaded successfully.
                throw new DllNotFoundException("SystemFabricClient..ctor(): Call to JsonSerializerImplDllLoader.TryGetJsonSerializerImpl() failed.");
            }

            var obj = this.jsonSerializer.Deserialize(json, typeof(T));
            return (T)obj;
        }

        static string GetClusterManifestSetting(string clusterManifestXml, string sectionName, string paramName)
        {
            using (var stringReader = new StringReader(clusterManifestXml))
            {
                using (XmlReader reader = XmlReader.Create(stringReader, new XmlReaderSettings() { XmlResolver = null }))
                {
                    var clusterManifest = new XmlDocument() { XmlResolver = null };
                    clusterManifest.Load(reader);
                    var xpath = string.Format(@"/x:ClusterManifest/x:FabricSettings/x:Section[@Name='{0}']/x:Parameter[@Name='{1}']/@Value", sectionName, paramName);
                    var ns = new XmlNamespaceManager(clusterManifest.NameTable);
                    ns.AddNamespace("x", "http://schemas.microsoft.com/2011/01/fabric");
                    return clusterManifest.SelectSingleNode(xpath, ns).Value;
                }
            }
        }

        private class UploadProgressHandler : IImageStoreProgressHandler
        {
            private TimeSpan interval;
            private long previousTotalItems;
            private long previousCompletedItems;
            private long completeFileCount;
            private long totalFileCount;

            public UploadProgressHandler(ref long completeFileCount, ref long totalFileCount, TimeSpan interval)
            {
                this.completeFileCount = completeFileCount;
                this.totalFileCount = totalFileCount;
                this.interval = interval;
            }

            TimeSpan IImageStoreProgressHandler.GetUpdateInterval()
            {
                return this.interval;
            }

            void IImageStoreProgressHandler.UpdateProgress(long completedItems, long totalItems, ProgressUnitType itemType)
            {
                if (completedItems < this.previousCompletedItems)
                {
                    throw new InvalidOperationException("Invalid progress completed count received");
                }
                else
                {
                    this.previousCompletedItems = completedItems;
                    this.completeFileCount = completedItems;
                }

                if (totalItems < this.previousTotalItems)
                {
                    throw new InvalidOperationException("Invalid progress total count received");
                }
                else
                {
                    this.previousTotalItems = totalItems;
                    this.totalFileCount = totalItems;
                }
            }
        }
    }
}


#pragma warning restore 1591