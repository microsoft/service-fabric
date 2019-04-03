// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.ImageStore;
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;
    using System.Linq;
    using System.Numerics;
    using System.Threading;
    using System.Threading.Tasks;
    using Repair;

    using ThrowIf = System.Fabric.Common.ThrowIf;

    public class FabricClientHelper : IFabricTestabilityClient, IFabricClient
    {
        private readonly string name;
        private readonly FabricClientTypes fabricClientTypes;

        private readonly Dictionary<FabricClientTypes, FabricClientObject> clients;

        public FabricClientHelper(string name)
            : this(name, new FabricClient())
        {
        }

        public FabricClientHelper(string name, FabricClient fabricClient)
        {
            this.name = name;
            this.fabricClientTypes = FabricClientTypes.System;
            this.ClientState = new FabricClientState();
            this.clients = new Dictionary<FabricClientTypes, FabricClientObject>();
            this.clients[FabricClientTypes.System] = new SystemFabricClient(fabricClient);
        }

        public FabricClientHelper(
            FabricClientHelperInfo info,
            FabricClientObject powershellFabricClientObject,
            FabricClientObject restFabricClientObject)
        {
            ThrowIf.Null(info, "info");

            this.name = info.Name;
            this.fabricClientTypes = info.ClientTypes;
            this.ClientState = new FabricClientState();
            this.clients = new Dictionary<FabricClientTypes, FabricClientObject>();

            if (info.ClientTypes.HasFlag(FabricClientTypes.Rest))
            {
                ReleaseAssert.AssertIf(restFabricClientObject == null, "FabricClientTypes.Rest is set and restFabricClientObject is null");
                this.clients[FabricClientTypes.Rest] = restFabricClientObject;
            }

            if (info.ClientTypes.HasFlag(FabricClientTypes.PowerShell))
            {
                ReleaseAssert.AssertIf(powershellFabricClientObject == null, "FabricClientTypes.PowerShell is set and powershellFabricClientObject is null");
                this.clients[FabricClientTypes.PowerShell] = powershellFabricClientObject;
            }

            //If no preferred client type, the system client will be the default
            if (info.ClientTypes.HasFlag(FabricClientTypes.System) || this.clients.Count == 0)
            {
                this.clients[FabricClientTypes.System] = new SystemFabricClient(info.GatewayAddresses, info.SecurityCredentials, info.ClientSettings);
            }
        }

        public string Name
        {
            get { return this.name; }
        }

        public FabricClientObject GetFabricClientObject(FabricClientTypes clientType)
        {
            ReleaseAssert.AssertIf(this.clients[clientType] == null, "{0} not found", clientType);
            return this.clients[clientType];
        }

        private FabricClientState ClientState { get; set; }

        // Enabling powershell client for testing on those Windows Versions (WindowsServer2012R2)
        public bool WindowsVersionsDoesNotHavePowerShellModuleLoadIssue
        {
            get
            {
                return Environment.OSVersion.Platform == PlatformID.Win32NT && Environment.OSVersion.Version.Major == 6 && Environment.OSVersion.Version.Minor == 3;
            }
        }

        public Task<OperationResult> RecoverServicePartitionsAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("RecoverServicePartitionsAsync").RecoverServicePartitionsAsync(serviceName, timeout, CancellationToken.None);
        }

        public Task<OperationResult> RecoverSystemPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("RecoverSystemPartitionsAsync").RecoverSystemPartitionsAsync(timeout, CancellationToken.None);
        }

        public Task<OperationResult<DeployedApplication[]>> GetDeployedApplicationListAsync(string nodeName, Uri applicationNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetDeployedApplicationListAsync").GetDeployedApplicationListAsync(nodeName, applicationNameFilter, timeout, cancellationToken);
        }

        public Task<OperationResult<DeployedApplicationPagedList>> GetDeployedApplicationPagedListAsync(
            PagedDeployedApplicationQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetDeployedApplicationPagedListAsync").GetDeployedApplicationPagedListAsync(queryDescription, timeout, cancellationToken);
        }

        public Task<OperationResult<DeployedCodePackage[]>> GetDeployedCodePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string codePackageNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetDeployedCodePackageListAsync").GetDeployedCodePackageListAsync(nodeName, applicationName, serviceManifestNameFilter, codePackageNameFilter, timeout, cancellationToken);
        }

        public Task<OperationResult<DeployedServiceReplica[]>> GetDeployedReplicaListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, Guid? partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetDeployedReplicaListAsync").GetDeployedReplicaListAsync(nodeName, applicationName, serviceManifestNameFilter, partitionIdFilter, timeout, cancellationToken);
        }

        public Task<OperationResult<DeployedServicePackage[]>> GetDeployedServicePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetDeployedServicePackageListAsync").GetDeployedServicePackageListAsync(nodeName, applicationName, serviceManifestNameFilter, timeout, cancellationToken);
        }

        public Task<OperationResult<DeployedServiceType[]>> GetDeployedServiceTypeListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetDeployedServiceTypeListAsync").GetDeployedServiceTypeListAsync(nodeName, applicationName, serviceManifestNameFilter, serviceTypeNameFilter, timeout, cancellationToken);
        }

        public Task<OperationResult<Node>> GetNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetNodeAsync").GetNodeAsync(nodeName, timeout, cancellationToken);
        }

        public Task<OperationResult<bool>> NameExistsAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("NameExistsAsync").NameExistsAsync(name, timeout, cancellationToken);
        }

        public Task<OperationResult> ProvisionApplicationAsync(string buildPath, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.ProvisionApplicationAsync(new ProvisionApplicationTypeDescription(buildPath), timeout, cancellationToken);
        }

        public Task<OperationResult> ProvisionApplicationAsync(ProvisionApplicationTypeDescriptionBase description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("ProvisionApplicationAsync").ProvisionApplicationAsync(description, timeout, cancellationToken);
        }

        public Task<OperationResult> ProvisionFabricAsync(string codePackagePath, string clusterManifestFilePath, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("ProvisionFabricAsync").ProvisionFabricAsync(codePackagePath, clusterManifestFilePath, timeout, cancellationToken);
        }

        public Task<OperationResult> UnprovisionApplicationAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.UnprovisionApplicationAsync(new UnprovisionApplicationTypeDescription(applicationTypeName, applicationTypeVersion), timeout, cancellationToken);
        }

        public Task<OperationResult> UnprovisionApplicationAsync(UnprovisionApplicationTypeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UnprovisionApplicationAsync").UnprovisionApplicationAsync(description, timeout, cancellationToken);
        }

        public Task<OperationResult> UnprovisionFabricAsync(string codeVersion, string configVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UnprovisionFabricAsync").UnprovisionFabricAsync(codeVersion, configVersion, timeout, cancellationToken);
        }

        public Task<OperationResult> CopyWindowsFabricServicePackageToNodeAsync(string serviceManifestName, string applicationTypeName, string applicationTypeVersion, string nodeName, PackageSharingPolicyDescription[] policies, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("CopyWindowsFabricServicePackageToNodeAsync").CopyWindowsFabricServicePackageToNodeAsync(serviceManifestName, applicationTypeName, applicationTypeVersion, nodeName, policies, timeout, cancellationToken);
        }

        public Task<OperationResult> CreateApplicationAsync(
            ApplicationDescription applicationDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("CreateApplicationAsync").CreateApplicationAsync(applicationDescription, timeout, cancellationToken);
        }

        public Task<OperationResult> UpdateApplicationAsync(ApplicationUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UpdateApplicationAsync").UpdateApplicationAsync(updateDescription, timeout, cancellationToken);
        }

        public Task<OperationResult> DeleteApplicationAsync(DeleteApplicationDescription deleteApplicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("DeleteApplicationAsync").DeleteApplicationAsync(deleteApplicationDescription, timeout, cancellationToken);
        }

        public Task<OperationResult> UpdateApplicationUpgradeAsync(ApplicationUpgradeUpdateDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UpdateApplicationUpgradeAsync").UpdateApplicationUpgradeAsync(description, timeout, cancellationToken);
        }

        public Task<OperationResult> UpgradeApplicationAsync(ApplicationUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UpgradeApplicationAsync").UpgradeApplicationAsync(description, timeout, cancellationToken);
        }

        public Task<OperationResult> UpgradeFabricAsync(FabricUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UpgradeFabricAsync").UpgradeFabricAsync(description, timeout, cancellationToken);
        }

        public Task<OperationResult> UpgradeFabricStandaloneAsync(ConfigurationUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UpgradeFabricStandaloneAsync").UpgradeFabricStandaloneAsync(description, timeout, cancellationToken);
        }

        public Task<OperationResult> UpdateFabricUpgradeAsync(FabricUpgradeUpdateDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UpdateFabricUpgradeAsync").UpdateFabricUpgradeAsync(description, timeout, cancellationToken);
        }

        public async Task<OperationResult<ApplicationUpgradeProgress>> GetApplicationUpgradeProgressAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.GetFabricClient("GetApplicationUpgradeProgressAsync").GetApplicationUpgradeProgressAsync(name, timeout, cancellationToken);
        }

        public Task<OperationResult> PutApplicationResourceAsync(string applicationName, string descriptionJson, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("PutApplicationResourceAsync").PutApplicationResourceAsync(applicationName, descriptionJson, timeout, cancellationToken);
        }

        public Task<OperationResult> DeleteApplicationResourceAsync(string applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("DeleteApplicationResourceAsync").DeleteApplicationResourceAsync(applicationName, timeout, cancellationToken);
        }

        public Task<OperationResult<ApplicationResourceList>> GetApplicationResourceListAsync(string applicationName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetApplicationResourceListAsync").GetApplicationResourceListAsync(applicationName, continuationToken, timeout, cancellationToken);
        }

        public Task<OperationResult<ServiceResourceList>> GetServiceResourceListAsync(string applicationName, string serviceName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetServiceResourceListAsync").GetServiceResourceListAsync(applicationName, serviceName, continuationToken, timeout, cancellationToken);
        }

        public Task<OperationResult<ReplicaResourceList>> GetReplicaResourceListAsync(string applicationName, string serviceName, string replicaName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetReplicaResourceListAsync").GetReplicaResourceListAsync(applicationName, serviceName, replicaName, continuationToken, timeout, cancellationToken);
        }

        public async Task<OperationResult<FabricOrchestrationUpgradeProgress>> GetFabricConfigUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.GetFabricClient("GetFabricConfigUpgradeProgressAsync").GetFabricConfigUpgradeProgressAsync(timeout, cancellationToken);
        }

        public async Task<OperationResult<string>> GetFabricConfigurationAsync(string apiVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.GetFabricClient("GetFabricConfigurationAsync").GetFabricConfigurationAsync(apiVersion, timeout, cancellationToken);
        }

        public async Task<OperationResult<string>> GetUpgradeOrchestrationServiceStateAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.GetFabricClient("GetUpgradeOrchestrationServiceStateAsync").GetUpgradeOrchestrationServiceStateAsync(timeout, cancellationToken);
        }

        public async Task<OperationResult<FabricUpgradeOrchestrationServiceState>> SetUpgradeOrchestrationServiceStateAsync(string state, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.GetFabricClient("SetUpgradeOrchestrationServiceStateAsync").SetUpgradeOrchestrationServiceStateAsync(state, timeout, cancellationToken);
        }

        public async Task<OperationResult<FabricUpgradeProgress>> GetFabricUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.GetFabricClient("GetFabricUpgradeProgressAsync").GetFabricUpgradeProgressAsync(timeout, cancellationToken);
        }

        public Task<OperationResult> MoveNextApplicationUpgradeDomainAsync(ApplicationUpgradeProgress currentProgress, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(currentProgress, "currentProgress");
            return this.GetFabricClient("MoveNextApplicationUpgradeDomainAsync").MoveNextApplicationUpgradeDomainAsync(currentProgress, timeout, cancellationToken);
        }

        public Task<OperationResult> MoveNextFabricUpgradeDomainAsync(FabricUpgradeProgress currentProgress, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(currentProgress, "currentProgress");
            return this.GetFabricClient("MoveNextFabricUpgradeDomainAsync").MoveNextFabricUpgradeDomainAsync(currentProgress, timeout, cancellationToken);
        }

        public OperationResult<UpgradeDomainStatus[]> GetChangedUpgradeDomains(ApplicationUpgradeProgress previousApplicationUpgradeProgress, ApplicationUpgradeProgress currentApplicationUpgradeProgress)
        {
            ThrowIf.Null(currentApplicationUpgradeProgress, "currentApplicationUpgradeProgress");

            ReadOnlyCollection<UpgradeDomainStatus> upgradeDomains = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = FabricClientState.HandleException(() => upgradeDomains = currentApplicationUpgradeProgress.GetChangedUpgradeDomains(previousApplicationUpgradeProgress));

            OperationResult<UpgradeDomainStatus[]> result = FabricClientState.CreateOperationResultFromNativeErrorCode<UpgradeDomainStatus[]>(errorCode);
            if (errorCode == 0)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<UpgradeDomainStatus[]>(upgradeDomains.ToArray());
            }

            return result;
        }

        public OperationResult<UpgradeDomainStatus[]> GetChangedUpgradeDomains(FabricUpgradeProgress previousFabricUpgradeProgress, FabricUpgradeProgress currentFabricUpgradeProgress)
        {
            ThrowIf.Null(currentFabricUpgradeProgress, "currentFabricUpgradeProgress");

            ReadOnlyCollection<UpgradeDomainStatus> upgradeDomains = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = FabricClientState.HandleException(() => upgradeDomains = currentFabricUpgradeProgress.GetChangedUpgradeDomains(previousFabricUpgradeProgress));

            OperationResult<UpgradeDomainStatus[]> result = FabricClientState.CreateOperationResultFromNativeErrorCode<UpgradeDomainStatus[]>(errorCode);
            if (errorCode == 0)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<UpgradeDomainStatus[]>(upgradeDomains.ToArray());
            }

            return result;
        }

        public Task<OperationResult> CreateNameAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("CreateNameAsync").CreateNameAsync(name, timeout, cancellationToken);
        }

        public Task<OperationResult> DeleteNameAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("DeleteNameAsync").DeleteNameAsync(name, timeout, cancellationToken);
        }

        public Task<OperationResult> CreateServiceAsync(ServiceDescription winFabricServiceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("CreateServiceAsync").CreateServiceAsync(winFabricServiceDescription, timeout, cancellationToken);
        }

        public Task<OperationResult> CreateServiceFromTemplateAsync(
            Uri applicationName,
            Uri serviceName,
            string serviceDnsName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode,
            byte[] initializationData,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("CreateServiceFromTemplateAsync").CreateServiceFromTemplateAsync(
                applicationName,
                serviceName,
                serviceDnsName,
                serviceTypeName,
                servicePackageActivationMode,
                initializationData,
                timeout,
                cancellationToken);
        }

        public Task<OperationResult> DeleteServiceAsync(DeleteServiceDescription deleteServiceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("DeleteServiceAsync").DeleteServiceAsync(deleteServiceDescription, timeout, cancellationToken);
        }

        public Task<OperationResult> UpdateServiceAsync(Uri name, ServiceUpdateDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UpdateServiceAsync").UpdateServiceAsync(name, description, timeout, cancellationToken);
        }

        public Task<OperationResult> CreateServiceGroupAsync(ServiceGroupDescription winFabricServiceGroupDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("CreateServiceGroupAsync").CreateServiceGroupAsync(winFabricServiceGroupDescription, timeout, cancellationToken);
        }

        public Task<OperationResult<ServiceDescription>> GetServiceDescriptionAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetServiceDescriptionAsync").GetServiceDescriptionAsync(name, timeout, cancellationToken);
        }

        public Task<OperationResult> DeleteServiceGroupAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("DeleteServiceGroupAsync").DeleteServiceGroupAsync(name, timeout, cancellationToken);
        }

        public async Task<OperationResult<TestServicePartitionInfo[]>> ResolveServiceAsync(Uri name, object partitionKey, TestServicePartitionInfo previousResult, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ResolvedServicePartition resolvedPartition = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await FabricClientState.HandleExceptionAsync(
            async delegate
            {
                resolvedPartition =
                    await this.ResolveWrapper(name, partitionKey, previousResult, timeout, cancellationToken);
            });

            return this.ClientState.ConvertToServicePartitionResult(errorCode, resolvedPartition);
        }

        public Task<OperationResult> PutPropertyAsync(Uri name, string propertyName, object data, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("PutPropertyAsync").PutPropertyAsync(name, propertyName, data, timeout, cancellationToken);
        }

        public Task<OperationResult<NamedProperty>> GetPropertyAsync(Uri name, string propertyName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetPropertyAsync").GetPropertyAsync(name, propertyName, timeout, cancellationToken);
        }

        public Task<OperationResult> DeletePropertyAsync(Uri name, string propertyName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("DeletePropertyAsync").DeletePropertyAsync(name, propertyName, timeout, cancellationToken);
        }

        public Task<OperationResult> PutCustomPropertyOperationAsync(Uri name, PutCustomPropertyOperation putCustomPropertyOperation, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("PutCustomPropertyOperationAsync").PutCustomPropertyOperationAsync(name, putCustomPropertyOperation, timeout, cancellationToken);
        }

        public Task<OperationResult<NamedPropertyMetadata>> GetPropertyMetadataAsync(Uri name, string propertyName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetPropertyMetadataAsync").GetPropertyMetadataAsync(name, propertyName, timeout, cancellationToken);
        }

        public async Task<OperationResult<WinFabricPropertyBatchResult>> SubmitPropertyBatchAsync(Uri name, ICollection<PropertyBatchOperation> operations, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var result = await this.GetFabricClient("SubmitPropertyBatchAsync").SubmitPropertyBatchAsync(name, operations, timeout, cancellationToken);
            OperationResult<WinFabricPropertyBatchResult> finalResult = new OperationResult<WinFabricPropertyBatchResult>(result.ErrorCode);
            if (result.ErrorCode == 0)
            {
                finalResult = this.ClientState.ConvertToWinFabricPropertyBatchResult(result.ErrorCode, result.Result);
            }

            return finalResult;
        }

        public Task<OperationResult> RestartReplicaInternalAsync(string nodeName, Guid partitionId, long replicaOrInstanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("RestartReplicaInternalAsync").RestartReplicaInternalAsync(nodeName, partitionId, replicaOrInstanceId, timeout, cancellationToken);
        }

        public Task<OperationResult> RemoveReplicaInternalAsync(string nodeName, Guid partitionId, long replicaOrInstanceId, bool forceRemove, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("RemoveReplicaInternalAsync").RemoveReplicaInternalAsync(nodeName, partitionId, replicaOrInstanceId, forceRemove, timeout, cancellationToken);
        }

        public Task<OperationResult> ActivateNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("ActivateNodeAsync").ActivateNodeAsync(nodeName, timeout, cancellationToken);
        }

        public Task<OperationResult> DeactivateNodeAsync(string nodeName, NodeDeactivationIntent deactivationIntent, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("DeactivateNodeAsync").DeactivateNodeAsync(nodeName, FabricClientHelper.ConvertDeactivateIntent(deactivationIntent), timeout, cancellationToken);
        }

        public Task<OperationResult> StartNodeNativeAsync(string nodeName, BigInteger instanceId, string ipAddressOrFQDN, int clusterConnectionPort, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("StartNodeNativeAsync").StartNodeNativeAsync(nodeName, instanceId, ipAddressOrFQDN, clusterConnectionPort, timeout, cancellationToken);
        }

        public Task<OperationResult> StopNodeNativeAsync(string nodeName, BigInteger instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("StopNodeNativeAsync").StopNodeNativeAsync(nodeName, instanceId, timeout, cancellationToken);
        }

        public Task<OperationResult> RestartNodeNativeAsync(string nodeName, BigInteger instanceId, bool createFabricDump, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("RestartNodeNativeAsync").RestartNodeNativeAsync(nodeName, instanceId, createFabricDump, timeout, cancellationToken);
        }

        public Task<OperationResult> RestartDeployedCodePackageNativeAsync(string nodeName, Uri applicationName, string serviceManifestName, string servicePackageActivationId, string codePackageName, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("RestartDeployedCodePackageNativeAsync").RestartDeployedCodePackageNativeAsync(nodeName, applicationName, serviceManifestName, servicePackageActivationId, codePackageName, instanceId, timeout, cancellationToken);
        }

        public Task<OperationResult> RemoveNodeStateAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("RemoveNodeStateAsync").RemoveNodeStateAsync(nodeName, timeout, cancellationToken);
        }

        public Task<OperationResult> RecoverPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("RecoverPartitionsAsync").RecoverPartitionsAsync(timeout, cancellationToken);
        }

        public async Task<OperationResult<WinFabricNameEnumerationResult>> EnumerateSubNamesAsync(Uri name, WinFabricNameEnumerationResult previousResult, bool recursive, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NameEnumerationResult realPreviousResult = this.ClientState.Convert(previousResult);
            OperationResult<NameEnumerationResult> result = await this.GetFabricClient("EnumerateSubNamesAsync").EnumerateSubNamesAsync(name, realPreviousResult, recursive, timeout, cancellationToken);
            OperationResult<WinFabricNameEnumerationResult> finalResult = new OperationResult<WinFabricNameEnumerationResult>(result.ErrorCode);
            if (result.ErrorCode == (int)NativeTypes.FABRIC_ERROR_CODE.S_OK && result.Result != null)
            {
                finalResult = this.ClientState.ConvertToWinFabricNameEnumerationResult(result.Result, result.ErrorCode);
            }

            return finalResult;
        }

        public async Task<OperationResult<WinFabricPropertyEnumerationResult>> EnumeratePropertiesAsync(Uri name, WinFabricPropertyEnumerationResult previousResult, bool includeValue, TimeSpan timeout, CancellationToken cancellationToken)
        {
            PropertyEnumerationResult realPreviousResult = this.ClientState.Convert(previousResult);

            OperationResult<PropertyEnumerationResult> result = await this.GetFabricClient("EnumeratePropertiesAsync").EnumeratePropertiesAsync(name, realPreviousResult, includeValue, timeout, cancellationToken);
            OperationResult<WinFabricPropertyEnumerationResult> finalResult = new OperationResult<WinFabricPropertyEnumerationResult>(result.ErrorCode);
            if (result.ErrorCode == 0 && result.Result != null)
            {
                finalResult = this.ClientState.ConvertToWinFabricPropertyEnumerationResult(result.Result, result.ErrorCode);
            }

            return finalResult;
        }

        public Task<OperationResult<NodeList>> GetNodeListAsync(NodeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetNodeListAsync").GetNodeListAsync(queryDescription, timeout, cancellationToken);
        }

        public Task<OperationResult<ServiceType[]>> GetServiceTypeListAsync(string applicationTypeName, string applicationTypeVersion, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetServiceTypeListAsync").GetServiceTypeListAsync(applicationTypeName, applicationTypeVersion, serviceTypeNameFilter, timeout, cancellationToken);
        }

        public Task<OperationResult<ServiceReplicaList>> GetReplicaListAsync(Guid partitionId, long replicaIdOrInstanceIdFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetReplicaListAsync").GetReplicaListAsync(partitionId, replicaIdOrInstanceIdFilter, continuationToken, timeout, cancellationToken);
        }

        public Task<OperationResult<ServicePartitionList>> GetPartitionListAsync(Uri serviceName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetPartitionListAsync").GetPartitionListAsync(serviceName, continuationToken, timeout, cancellationToken);
        }

        public Task<OperationResult<Partition>> GetServicePartitionAsync(Uri serviceName, Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetServicePartitionAsync").GetServicePartitionAsync(serviceName, partitionId, timeout, cancellationToken);
        }

        public Task<OperationResult<Replica>> GetServicePartitionReplicaAsync(Guid partitionId, long id, bool isStateful, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetServicePartitionReplicaAsync").GetServicePartitionReplicaAsync(partitionId, id, isStateful, timeout, cancellationToken);
        }

        // This still calls into the same command as GetApplicationTypesList. But this specifies for only one type
        public Task<OperationResult<ApplicationType>> GetApplicationTypeAsync(string applicationTypeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetApplicationTypeAsync").GetApplicationTypeAsync(applicationTypeName, timeout, cancellationToken);
        }

        public Task<OperationResult<ApplicationType[]>> GetApplicationTypeListAsync(string applicationTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetApplicationTypeListAsync").GetApplicationTypeListAsync(applicationTypeNameFilter, timeout, cancellationToken);
        }

        public Task<OperationResult<ApplicationTypePagedList>> GetApplicationTypePagedListAsync(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetApplicationTypePagedListAsync").GetApplicationTypePagedListAsync(queryDescription, timeout, cancellationToken);
        }

        // This overload exists only to test the PowerShell specific parameters which do not exist elsewhere.
        public Task<OperationResult<ApplicationTypePagedList>> GetApplicationTypePagedListAsync(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken, bool getPagedResults = false)
        {
            return this.GetFabricClient("GetApplicationTypePagedListAsync").GetApplicationTypePagedListAsync(queryDescription, timeout, cancellationToken, getPagedResults);
        }

        public Task<OperationResult<ApplicationList>> GetApplicationPagedListAsync(ApplicationQueryDescription applicationQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetApplicationPagedListAsync").GetApplicationPagedListAsync(applicationQueryDescription, timeout, cancellationToken);
        }

        public Task<OperationResult<Application>> GetApplicationAsync(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetApplicationAsync").GetApplicationAsync(applicationName, timeout, cancellationToken);
        }

        public Task<OperationResult<Application>> GetApplicationByIdRestAsync(string applicationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetApplicationByIdRestAsync").GetApplicationByIdRestAsync(applicationId, timeout, cancellationToken);
        }

        public Task<OperationResult<Service>> GetServiceByIdRestAsync(string applicationId, string serviceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetServiceByIdRestAsync").GetServiceByIdRestAsync(applicationId, serviceId, timeout, cancellationToken);
        }

        public Task<OperationResult<ServiceList>> GetServicePagedListAsync(ServiceQueryDescription serviceQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetServicePagedListAsync").GetServicePagedListAsync(serviceQueryDescription, timeout, cancellationToken);
        }

        public Task<OperationResult<ServicePartitionList>> GetPartitionListRestAsync(string applicationName, string serviceName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetPartitionListRestAsync").GetPartitionListRestAsync(applicationName, serviceName, continuationToken, timeout, cancellationToken);
        }

        public Task<OperationResult<ServiceReplicaList>> GetReplicaListRestAsync(Uri applicationName, Uri serviceName, Guid partitionId, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetReplicaListRestAsync").GetReplicaListRestAsync(applicationName, serviceName, partitionId, continuationToken, timeout, cancellationToken);
        }

        public Task<OperationResult<string>> GetClusterManifestAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetClusterManifestAsync(new ClusterManifestQueryDescription(), timeout, cancellationToken);
        }

        public Task<OperationResult<string>> GetClusterManifestAsync(ClusterManifestQueryDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetClusterManifestAsync").GetClusterManifestAsync(description, timeout, cancellationToken);
        }

        public Task<OperationResult<string>> GetClusterVersionAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetClusterVersionAsync").GetClusterVersionAsync(timeout, cancellationToken);
        }

        public Task<OperationResult<string>> GetServiceManifestAsync(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetServiceManifestAsync").GetServiceManifestAsync(applicationTypeName, applicationTypeVersion, serviceManifestName, timeout, cancellationToken);
        }

        public Task<OperationResult<NodeLoadInformation>> GetNodeLoadAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetNodeLoadAsync").GetNodeLoadAsync(nodeName, timeout, cancellationToken);
        }

        public Task<OperationResult<ClusterLoadInformation>> GetClusterLoadAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetClusterLoadAsync").GetClusterLoadAsync(timeout, cancellationToken);
        }

        public Task<OperationResult<ReplicaLoadInformation>> GetReplicaLoadAsync(Guid partitionId, long replicaId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetReplicaLoadAsync").GetReplicaLoadAsync(partitionId, replicaId, timeout, cancellationToken);
        }

        public Task<OperationResult<PartitionLoadInformation>> GetPartitionLoadAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetPartitionLoadAsync").GetPartitionLoadAsync(partitionId, timeout, cancellationToken);
        }

        public Task<OperationResult<ApplicationLoadInformation>> GetApplicationLoadAsync(string applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetApplicationLoadAsync").GetApplicationLoadAsync(applicationName, timeout, cancellationToken);
        }

        public Task<OperationResult<ServiceNameResult>> GetServiceNameAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetServiceNameAsync").GetServiceNameAsync(partitionId, timeout, cancellationToken);
        }
        public Task<OperationResult<ApplicationNameResult>> GetApplicationNameAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetApplicationNameAsync").GetApplicationNameAsync(serviceName, timeout, cancellationToken);
        }

        public Task<OperationResult<string>> InvokeEncryptSecretAsync(string certificateThumbprint, string certificateStoreLocation, string text, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("InvokeEncryptSecretAsync").InvokeEncryptSecretAsync(certificateThumbprint, certificateStoreLocation, text, cancellationToken);
        }

        public Task<OperationResult> NewNodeConfigurationAsync(string clusterManifestPath, string computerName, string userName, string password, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("NewNodeConfigurationAsync").NewNodeConfigurationAsync(clusterManifestPath, computerName, userName, password, cancellationToken);
        }

        public Task<OperationResult> UpdateNodeConfigurationAsync(string clusterManifestPath, string computerName, string userName, string password, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UpdateNodeConfigurationAsync").UpdateNodeConfigurationAsync(clusterManifestPath, computerName, userName, password, timeout, cancellationToken);
        }

        public Task<OperationResult> RemoveNodeConfigurationAsync(string clusterManifestPath, string computerName, string userName, string password, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("RemoveNodeConfigurationAsync").RemoveNodeConfigurationAsync(clusterManifestPath, computerName, userName, password, cancellationToken);
        }

        public Task<OperationResult<bool>> TestClusterConnectionAsync(CancellationToken cancellationToken)
        {
            return this.GetFabricClient("TestClusterConnectionAsync").TestClusterConnectionAsync(cancellationToken);
        }

        public Task<OperationResult<bool>> TestClusterManifestAsync(string clusterManifestPath, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("TestClusterManifestAsync").TestClusterManifestAsync(clusterManifestPath, cancellationToken);
        }

        ////
        //// Imnage Store Client APIs
        ////
        public Task<OperationResult<ImageStoreContentResult>> ListImageStoreContentAsync(string remoteLocation, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("ListImageStoreContentAsync").ListImageStoreContentAsync(remoteLocation, imageStoreConnectionString, timeout, cancellationToken);
        }


        public Task<OperationResult<ImageStorePagedContentResult>> ListImageStorePagedContentAsync(ImageStoreListDescription listDescription, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("ListImageStorePagedContentAsync").ListImageStorePagedContentAsync(listDescription, imageStoreConnectionString, timeout, cancellationToken);
        }

        public Task<OperationResult> DeleteImageStoreContentAsync(string remoteLocation, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("DeleteImageStoreContentAsync").DeleteImageStoreContentAsync(remoteLocation, timeout, cancellationToken);
        }

        public Task<OperationResult> CopyImageStoreContentAsync(WinFabricCopyImageStoreContentDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("CopyImageStoreContentAsync").CopyImageStoreContentAsync(description, timeout, cancellationToken);
        }

        public Task<OperationResult> UploadChunkAsync(string remoteLocation, string sessionId, UInt64 startPosition, UInt64 endPosition, string filePath, UInt64 fileSize, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UploadChunkAsync").UploadChunkAsync(remoteLocation, sessionId, startPosition, endPosition, filePath, fileSize, timeout, cancellationToken);
        }

        public Task<OperationResult> DeleteUploadSessionAsync(Guid sessionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("DeleteUploadSessionAsync").DeleteUploadSessionAsync(sessionId, timeout, cancellationToken);
        }

        public Task<OperationResult<UploadSession>> ListUploadSessionAsync(Guid SessionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("ListUploadSessionAsync").ListUploadSessionAsync(SessionId, timeout, cancellationToken);
        }

        public Task<OperationResult> CommitUploadSessionAsync(Guid SessionId, TimeSpan tiemout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("CommitUploadSessionAsync").CommitUploadSessionAsync(SessionId, tiemout, cancellationToken);
        }

        ////
        //// Health Client APIs
        ////

        public Task<OperationResult<ApplicationHealth>> GetApplicationHealthAsync(ApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetApplicationHealthAsync").GetApplicationHealthAsync(queryDescription, timeout, cancellationToken);
        }

        public Task<OperationResult<ClusterHealth>> GetClusterHealthAsync(ClusterHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetClusterHealthAsync").GetClusterHealthAsync(queryDescription, timeout, cancellationToken);
        }

        public Task<OperationResult<ClusterHealthChunk>> GetClusterHealthChunkAsync(ClusterHealthChunkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetClusterHealthChunkAsync").GetClusterHealthChunkAsync(queryDescription, timeout, cancellationToken);
        }

        public Task<OperationResult<NodeHealth>> GetNodeHealthAsync(NodeHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetNodeHealthAsync").GetNodeHealthAsync(queryDescription, timeout, cancellationToken);
        }

        public Task<OperationResult<PartitionHealth>> GetPartitionHealthAsync(PartitionHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetPartitionHealthAsync").GetPartitionHealthAsync(queryDescription, timeout, cancellationToken);
        }

        public Task<OperationResult<ReplicaHealth>> GetReplicaHealthAsync(ReplicaHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetReplicaHealthAsync").GetReplicaHealthAsync(queryDescription, timeout, cancellationToken);
        }

        public Task<OperationResult<ServiceHealth>> GetServiceHealthAsync(ServiceHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetServiceHealthAsync").GetServiceHealthAsync(queryDescription, timeout, cancellationToken);
        }

        public Task<OperationResult<DeployedApplicationHealth>> GetDeployedApplicationHealthAsync(DeployedApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetDeployedApplicationHealthAsync").GetDeployedApplicationHealthAsync(queryDescription, timeout, cancellationToken);
        }

        public Task<OperationResult<DeployedServicePackageHealth>> GetDeployedServicePackageHealthAsync(DeployedServicePackageHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetDeployedServicePackageHealthAsync").GetDeployedServicePackageHealthAsync(queryDescription, timeout, cancellationToken);
        }

        public OperationResult ReportHealth(HealthReport healthReport, HealthReportSendOptions sendOptions)
        {
            return this.GetFabricClient("ReportHealth").ReportHealth(healthReport, sendOptions);
        }

        public Task<OperationResult<string>> GetApplicationManifestAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetApplicationManifestAsync").GetApplicationManifestAsync(applicationTypeName, applicationTypeVersion, timeout, cancellationToken);
        }

        public Task<OperationResult> UpdateServiceGroupAsync(ServiceKind serviceKind, Uri serviceGroupName, int targetReplicaSetSize, TimeSpan replicaRestartWaitDuration, TimeSpan quorumLossWaitDuration, int instanceCount, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UpdateServiceGroupAsync").UpdateServiceGroupAsync(serviceKind, serviceGroupName, targetReplicaSetSize, replicaRestartWaitDuration, quorumLossWaitDuration, instanceCount, timeout, cancellationToken);
        }

        public Task<OperationResult<bool>> TestApplicationPackageAsync(string applicationPackagePath, string imageStoreConnectionString, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("TestApplicationPackageAsync").TestApplicationPackageAsync(applicationPackagePath, imageStoreConnectionString, cancellationToken);
        }

        public Task<OperationResult> CopyApplicationPackageAndShowProgressAsync(string applicationPackagePath, string imageStoreConnectionString, string applicationPackagePathInImageStore, bool compress, TimeSpan timeout, CancellationToken cancellationToken)
        {
            if (this.IsNativeImageStore(imageStoreConnectionString, applicationPackagePath, applicationPackagePathInImageStore))
            {
                return this.GetFabricClient("CopyApplicationPackageAndShowProgressAsync", FabricClientTypes.Rest).CopyApplicationPackageAndShowProgressAsync(applicationPackagePath, imageStoreConnectionString, applicationPackagePathInImageStore, compress, timeout, cancellationToken);
            }
            else
            {
                return this.CopyApplicationPackageAsync(applicationPackagePath, imageStoreConnectionString, applicationPackagePathInImageStore, compress, timeout, cancellationToken);
            }
        }

        public Task<OperationResult> RemoveApplicationPackageAsync(string applicationPackagePathInImageStore, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("RemoveApplicationPackageAsync").RemoveApplicationPackageAsync(applicationPackagePathInImageStore, imageStoreConnectionString, timeout, cancellationToken);
        }

        public Task<OperationResult> CopyApplicationPackageAsync(string applicationPackagePath, string imageStoreConnectionString, string applicationPackagePathInImageStore, bool compress, TimeSpan timeout, CancellationToken cancellationToken)
        {
            if (this.IsNativeImageStore(imageStoreConnectionString, applicationPackagePath, applicationPackagePathInImageStore))
            {
                return this.GetFabricClient("CopyApplicationPackageAsync").CopyApplicationPackageAsync(applicationPackagePath, imageStoreConnectionString, applicationPackagePathInImageStore, compress, timeout, cancellationToken);
            }
            else
            {
                return this.GetFabricClient("CopyApplicationPackageAsync", FabricClientTypes.Rest).CopyApplicationPackageAsync(applicationPackagePath, imageStoreConnectionString, applicationPackagePathInImageStore, compress, timeout, cancellationToken);
            }
        }

        public Task<OperationResult> CopyClusterPackageAsync(string codePackagePath, string clusterManifestPath, string imageStoreConnectionString, string codePackagePathInImageStore, string clusterManifestPathInImageStore, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("CopyClusterPackageAsync").CopyClusterPackageAsync(codePackagePath, clusterManifestPath, imageStoreConnectionString, codePackagePathInImageStore, clusterManifestPathInImageStore, cancellationToken);
        }

        public Task<OperationResult> CopyClusterPackageAsync(string codePackagePath, string clusterManifestPath, string imageStoreConnectionString, string codePackagePathInImageStore, string clusterManifestPathInImageStore, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("CopyClusterPackageAsync").CopyClusterPackageAsync(codePackagePath, clusterManifestPath, imageStoreConnectionString, codePackagePathInImageStore, clusterManifestPathInImageStore, timeout, cancellationToken);
        }

        public Task<OperationResult> AddUnreliableTransportBehaviorAsync(string nodeName, string name, System.Fabric.Testability.UnreliableTransportBehavior behavior, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("AddUnreliableTransportBehaviorAsync").AddUnreliableTransportBehaviorAsync(nodeName, name, behavior, timeout, cancellationToken);
        }

        public Task<OperationResult> RemoveUnreliableTransportBehaviorAsync(string nodeName, string name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("RemoveUnreliableTransportBehaviorAsync").RemoveUnreliableTransportBehaviorAsync(nodeName, name, timeout, cancellationToken);
        }

        public Task<OperationResult> AddUnreliableLeaseBehaviorAsync(string nodeName, string behaviorString, string alias, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("AddUnreliableLeaseBehaviorAsync").AddUnreliableLeaseBehaviorAsync(nodeName, behaviorString, alias, timeout, cancellationToken);
        }

        public Task<OperationResult> RemoveUnreliableLeaseBehaviorAsync(string nodeName, string alias, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("RemoveUnreliableLeaseBehaviorAsync").RemoveUnreliableLeaseBehaviorAsync(nodeName, alias, timeout, cancellationToken);
        }

        public Task<OperationResult<Int64>> CreateRepairTaskAsync(WinFabricRepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("CreateRepairTaskAsync").CreateRepairTaskAsync(repairTask, timeout, cancellationToken);
        }

        public Task<OperationResult> StartRepairTaskAsync(string nodeName, SystemNodeRepairAction nodeAction, string[] nodeNames, string customAction, NodeImpactLevel nodeImpact, string taskId, string description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("StartRepairTaskAsync").StartRepairTaskAsync(nodeName, nodeAction, nodeNames, customAction, nodeImpact, taskId, description, timeout, cancellationToken);
        }

        public Task<OperationResult<Int64>> CancelRepairTaskAsync(string repairTaskId, Int64 version, bool requestAbort, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("CancelRepairTaskAsync").CancelRepairTaskAsync(repairTaskId, version, requestAbort, timeout, cancellationToken);
        }

        public Task<OperationResult<Int64>> ForceApproveRepairTaskAsync(string repairTaskId, Int64 version, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("ForceApproveRepairTaskAsync").ForceApproveRepairTaskAsync(repairTaskId, version, timeout, cancellationToken);
        }

        public Task<OperationResult> DeleteRepairTaskAsync(string repairTaskId, Int64 version, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("DeleteRepairTaskAsync").DeleteRepairTaskAsync(repairTaskId, version, timeout, cancellationToken);
        }

        public Task<OperationResult<Int64>> UpdateRepairExecutionStateAsync(WinFabricRepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UpdateRepairExecutionStateAsync").UpdateRepairExecutionStateAsync(repairTask, timeout, cancellationToken);
        }

        public Task<OperationResult<WinFabricRepairTask[]>> GetRepairTaskListAsync(string taskIdFilter, WinFabricRepairTaskStateFilter stateFilter, string executorFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetRepairTaskListAsync").GetRepairTaskListAsync(taskIdFilter, stateFilter, executorFilter, timeout, cancellationToken);
        }

        public event EventHandler<WinFabricServiceResolutionChangeEventArgs> ServiceResolutionChanged;

        public OperationResult<long> RegisterServicePartitionResolutionChangeHandler(Uri name, object partitionKey)
        {
            return this.GetFabricClient("RegisterServicePartitionResolutionChangeHandler").RegisterServicePartitionResolutionChangeHandler(name, partitionKey, this.ReceiveNotifications);
        }

        public OperationResult UnregisterServicePartitionResolutionChangeHandler(long callbackId)
        {
            return this.GetFabricClient("UnregisterServicePartitionResolutionChangeHandler").UnregisterServicePartitionResolutionChangeHandler(callbackId);
        }

        private bool IsNativeImageStore(string imageStoreConnectionString, string applicationPackagePath, string applicationPackagePathInImageStore)
        {
            return imageStoreConnectionString.StartsWith("fabric:", StringComparison.OrdinalIgnoreCase) && applicationPackagePath.EndsWith(applicationPackagePathInImageStore, StringComparison.OrdinalIgnoreCase);
        }

        private void ReceiveNotifications(FabricClient source, long handlerId, ServicePartitionResolutionChange change)
        {
            ServicePartitionResolutionChangeEventArgs servicePartitionResolutionChangeEventArgs = null;
            if (change.HasException)
            {
                // Scenarios using ServiceResolutionChanged event will handle null rsp based on their own Exception handling logic
                servicePartitionResolutionChangeEventArgs = new ServicePartitionResolutionChangeEventArgs(null, change.Exception, handlerId);
            }
            else
            {
                servicePartitionResolutionChangeEventArgs = new ServicePartitionResolutionChangeEventArgs(change.Result, null, handlerId);
            }

            this.RaiseEvent(this.ServiceResolutionChanged, FabricClientHelper.ConvertToWinFabricServiceResolutionChangeEventArgs(servicePartitionResolutionChangeEventArgs));
        }

        private void RaiseEvent(EventHandler<WinFabricServiceResolutionChangeEventArgs> handler, WinFabricServiceResolutionChangeEventArgs e)
        {
            if (handler != null)
            {
                handler(this, e);
            }
        }

        private static WinFabricServiceResolutionChangeEventArgs ConvertToWinFabricServiceResolutionChangeEventArgs(ServicePartitionResolutionChangeEventArgs e)
        {
            TestServicePartitionInfo[] testPartitionInfo = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = 0;

            if (e.Exception == null)
            {
                testPartitionInfo = FabricClientState.Convert(e.Partition, -1).Result;
            }
            else
            {
                errorCode = FabricClientState.HandleException(
                    () =>
                    {
                        if (e.Partition != null)
                        {
                            throw new InvalidOperationException(
                                "Partition must be null in ServicePartitionResolutionChangeCallback when exception is not null");
                        }
                        else
                        {
                            throw e.Exception;
                        }
                    });
            }

            return new WinFabricServiceResolutionChangeEventArgs(testPartitionInfo, (uint)errorCode, e.CallbackId);
        }

        public Task<OperationResult> RegisterServiceNotificationFilterAsync(ServiceNotificationFilterDescription filterDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("RegisterServiceNotificationFilterAsync").RegisterServiceNotificationFilterAsync(filterDescription, timeout, cancellationToken);
        }

        public Task<OperationResult> UnregisterServiceNotificationFilterAsync(long filterId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("UnregisterServiceNotificationFilterAsync").UnregisterServiceNotificationFilterAsync(filterId, timeout, cancellationToken);
        }

        private static NodeDeactivationIntent ConvertDeactivateIntent(NodeDeactivationIntent deactivationIntent)
        {
            NodeDeactivationIntent convertedIntent;
            switch (deactivationIntent)
            {
                case NodeDeactivationIntent.Pause:
                    convertedIntent = NodeDeactivationIntent.Pause;
                    break;

                case NodeDeactivationIntent.Restart:
                    convertedIntent = NodeDeactivationIntent.Restart;
                    break;

                case NodeDeactivationIntent.RemoveData:
                    convertedIntent = NodeDeactivationIntent.RemoveData;
                    break;

                default:
                    throw new InvalidOperationException("Invalid NodeDeactivationIntent");
            }

            return convertedIntent;
        }

        private Task<ResolvedServicePartition> ResolveWrapper(Uri name, object partitionKey, TestServicePartitionInfo previousResult, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ResolvedServicePartition previousServicePartition = null;
            if (previousResult != null)
            {
                previousServicePartition = this.ClientState.PartitionHolder.RetrieveObject(previousResult.PartitionIdentifier);
            }

            return this.GetFabricClient("ResolveServiceAsync").ResolveServiceAsync(name, partitionKey, previousServicePartition, timeout, cancellationToken);
        }

        private FabricClientObject GetFabricClient(string methodName, FabricClientTypes excludedTypes)
        {
            IList<FabricClientObject> possibleClients = new List<FabricClientObject>();

            if (this.fabricClientTypes.HasFlag(FabricClientTypes.System) && !excludedTypes.HasFlag(FabricClientTypes.System) && this.clients[FabricClientTypes.System].SupportedMethods.Contains(methodName))
            {
                possibleClients.Add(this.clients[FabricClientTypes.System]);
            }

            if (this.fabricClientTypes.HasFlag(FabricClientTypes.Rest) && !excludedTypes.HasFlag(FabricClientTypes.Rest) && this.clients[FabricClientTypes.Rest].SupportedMethods.Contains(methodName))
            {
                possibleClients.Add(this.clients[FabricClientTypes.Rest]);
            }

            if (this.fabricClientTypes.HasFlag(FabricClientTypes.PowerShell) && !excludedTypes.HasFlag(FabricClientTypes.PowerShell) && this.clients[FabricClientTypes.PowerShell].SupportedMethods.Contains(methodName))
            {
                //This condition is only used by Mgmt_LargeApplicationDeployment_Func.
                //When #8146617 is fixed, this if statement will be removed.
                if ((this.clients.Count == 1 && this.clients[FabricClientTypes.PowerShell] != null)
                    || WindowsVersionsDoesNotHavePowerShellModuleLoadIssue)
                {
                    possibleClients.Add(this.clients[FabricClientTypes.PowerShell]);
                }
                else if (possibleClients.Count == 0)
                {
                    throw new InvalidOperationException("Powershell client is not supported on this Environment due to the cmdlet not found issue. Powershell client is only supported on WindowsServer2012R2.");
                }
            }

            // Fall back to System if other specified weren't supported
            if (possibleClients.Count == 0)
            {
                possibleClients.Add(this.clients[FabricClientTypes.System]);
            }

            return possibleClients[RandomUtility.NextFromTimestamp(possibleClients.Count)];
        }

        private FabricClientObject GetFabricClient(string methodName)
        {
            IList<FabricClientObject> possibleClients = new List<FabricClientObject>();
            if (this.fabricClientTypes.HasFlag(FabricClientTypes.System) && this.clients[FabricClientTypes.System].SupportedMethods.Contains(methodName))
            {
                possibleClients.Add(this.clients[FabricClientTypes.System]);
            }

            if (this.fabricClientTypes.HasFlag(FabricClientTypes.Rest) && this.clients[FabricClientTypes.Rest].SupportedMethods.Contains(methodName))
            {
                possibleClients.Add(this.clients[FabricClientTypes.Rest]);
            }

            if (this.fabricClientTypes.HasFlag(FabricClientTypes.PowerShell) && this.clients[FabricClientTypes.PowerShell].SupportedMethods.Contains(methodName))
            {
                //This condition is only used by Mgmt_LargeApplicationDeployment_Func.
                //When #8146617 is fixed, this if statement will be removed.
                if ((this.clients.Count == 1 && this.clients[FabricClientTypes.PowerShell] != null)
                    || WindowsVersionsDoesNotHavePowerShellModuleLoadIssue)
                {
                    possibleClients.Add(this.clients[FabricClientTypes.PowerShell]);
                }
                else if (possibleClients.Count == 0)
                {
                    throw new InvalidOperationException("Powershell client is not supported on this Environment due to the cmdlet not found issue. Powershell client is only supported on WindowsServer2012R2.");
                }
            }

            // Fall back to System if other specified weren't supported
            if (possibleClients.Count == 0)
            {
                possibleClients.Add(this.clients[FabricClientTypes.System]);
            }

            return possibleClients[RandomUtility.NextFromTimestamp(possibleClients.Count)];
        }

        public Task<OperationResult> StartNodeAsync(string nodeName, BigInteger nodeInstance, string ipAddressOrFQDN, int clusterConnectionPort, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("StartNodeAsync").StartNodeAsync(nodeName, nodeInstance, ipAddressOrFQDN, clusterConnectionPort, completionMode, operationTimeout, token);
        }

        public Task<OperationResult> StopNodeAsync(string nodeName, BigInteger nodeInstance, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("StopNodeAsync").StopNodeAsync(nodeName, nodeInstance, completionMode, operationTimeout, token);
        }

        public Task<OperationResult<RestartNodeResult>> RestartNodeAsync(string nodeName, BigInteger nodeInstance, bool createFabricDump, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("RestartNodeAsync").RestartNodeAsync(nodeName, nodeInstance, createFabricDump, completionMode, operationTimeout, token);
        }

        public Task<OperationResult<RestartNodeResult>> RestartNodeAsync(ReplicaSelector replicaSelector, bool createFabricDump, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("RestartNodeAsync").RestartNodeAsync(replicaSelector, createFabricDump, completionMode, operationTimeout, token);
        }

        public Task<OperationResult> RestartDeployedCodePackageAsync(Uri applicationName, ReplicaSelector replicaSelector, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("RestartDeployedCodePackageAsync").RestartDeployedCodePackageAsync(applicationName, replicaSelector, completionMode, operationTimeout, token);
        }

        public Task<OperationResult> RestartDeployedCodePackageAsync(string nodeName, Uri applicationName, string serviceManifestName, string servicePackageActivationId, string codePackageName, long codePackageInstanceId, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("RestartDeployedCodePackageAsync").RestartDeployedCodePackageAsync(nodeName, applicationName, serviceManifestName, servicePackageActivationId, codePackageName, codePackageInstanceId, completionMode, operationTimeout, token);
        }

        public Task RestartReplicaAsync(ReplicaSelector replicaSelector, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("RestartReplicaAsync").RestartReplicaAsync(replicaSelector, completionMode, operationTimeout, token);
        }

        public Task RestartReplicaAsync(string nodeName, Guid partitionId, long replicaId, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("RestartReplicaAsync").RestartReplicaAsync(nodeName, partitionId, replicaId, completionMode, operationTimeout, token);
        }

        public Task RemoveReplicaAsync(ReplicaSelector replicaSelector, CompletionMode completionMode, bool forceRemove, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("RemoveReplicaAsync").RemoveReplicaAsync(replicaSelector, completionMode, forceRemove, operationTimeout, token);
        }

        public Task RemoveReplicaAsync(string nodeName, Guid partitionId, long replicaId, CompletionMode completionMode, bool forceRemove, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("RemoveReplicaAsync").RemoveReplicaAsync(nodeName, partitionId, replicaId, completionMode, forceRemove, operationTimeout, token);
        }

        public Task ValidateApplicationAsync(Uri applicationName, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("ValidateApplicationAsync").ValidateApplicationAsync(applicationName, operationTimeout, token);
        }

        public Task ValidateServiceAsync(Uri serviceName, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("ValidateServiceAsync").ValidateServiceAsync(serviceName, operationTimeout, token);
        }

        public Task InvokeDataLossAsync(PartitionSelector partitionSelector, DataLossMode datalossMode, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("InvokeDataLossAsync").InvokeDataLossAsync(partitionSelector, datalossMode, operationTimeout, token);
        }

        public Task RestartPartitionAsync(PartitionSelector partitionSelector, RestartPartitionMode restartPartitionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("RestartPartitionAsync").RestartPartitionAsync(partitionSelector, restartPartitionMode, operationTimeout, token);
        }

        public Task MovePrimaryReplicaAsync(string nodeName, PartitionSelector partitionSelector, bool ignoreConstraints, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("MovePrimaryReplicaAsync").MovePrimaryReplicaAsync(nodeName, partitionSelector, ignoreConstraints, operationTimeout, token);
        }

        public Task MoveSecondaryReplicaAsync(string currentNodeName, string newNodeName, PartitionSelector partitionSelector, bool ignoreConstraints, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("MoveSecondaryReplicaAsync").MoveSecondaryReplicaAsync(currentNodeName, newNodeName, partitionSelector, ignoreConstraints, operationTimeout, token);
        }

        public Task CleanTestStateAsync(TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("CleanTestStateAsync").CleanTestStateAsync(operationTimeout, token);
        }

        public Task InvokeQuorumLossAsync(PartitionSelector partitionSelector, QuorumLossMode quorumlossMode, TimeSpan quorumlossDuration, TimeSpan operationTimeout, CancellationToken token)
        {
            return this.GetFabricClient("InvokeQuorumLossAsync").InvokeQuorumLossAsync(partitionSelector, quorumlossMode, quorumlossDuration, operationTimeout, token);
        }

        public Task<OperationResult> StartPartitionDataLossAsync(Guid operationId, PartitionSelector partitionSelector, DataLossMode dataLossMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("StartPartitionDataLossAsync").StartPartitionDataLossAsync(operationId, partitionSelector, dataLossMode, timeout, cancellationToken);
        }

        public Task<OperationResult<PartitionDataLossProgress>> GetPartitionDataLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetPartitionDataLossProgressAsync").GetPartitionDataLossProgressAsync(operationId, timeout, cancellationToken);
        }

        public Task<OperationResult> StartPartitionQuorumLossAsync(Guid operationId, PartitionSelector partitionSelector, QuorumLossMode quorumlossMode, TimeSpan quorumlossDuration, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("StartPartitionQuorumLossAsync").StartPartitionQuorumLossAsync(operationId, partitionSelector, quorumlossMode, quorumlossDuration, operationTimeout, cancellationToken);
        }

        public Task<OperationResult<PartitionQuorumLossProgress>> GetPartitionQuorumLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetPartitionQuorumLossProgressAsync").GetPartitionQuorumLossProgressAsync(operationId, timeout, cancellationToken);
        }

        public Task<OperationResult> StartPartitionRestartAsync(Guid operationId, PartitionSelector partitionSelector, RestartPartitionMode restartPartitionMode, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("StartPartitionRestartAsync").StartPartitionRestartAsync(operationId, partitionSelector, restartPartitionMode, operationTimeout, cancellationToken);
        }

        public Task<OperationResult<PartitionRestartProgress>> GetPartitionRestartProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetPartitionRestartProgressAsync").GetPartitionRestartProgressAsync(operationId, timeout, cancellationToken);
        }

        public Task<OperationResult> CancelTestCommandAsync(Guid operationId, bool force, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("CancelTestCommandAsync").CancelTestCommandAsync(operationId, force, timeout, cancellationToken);
        }

        public Task<OperationResult<TestCommandStatusList>> GetTestCommandStatusListAsync(TestCommandStateFilter stateFilter, TestCommandTypeFilter typeFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetTestCommandStatusListAsync").GetTestCommandStatusListAsync(stateFilter, typeFilter, timeout, cancellationToken);
        }

        public Task<OperationResult> StartChaosAsync(ChaosParameters chaosTestScenarioParameters, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("StartChaosAsync").StartChaosAsync(chaosTestScenarioParameters, timeout, cancellationToken);
        }

        public Task<OperationResult<string>> InvokeInfrastructureCommandAsync(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("InvokeInfrastructureCommandAsync").InvokeInfrastructureCommandAsync(serviceName, command,  timeout, cancellationToken);
        }

        public Task<OperationResult<ChaosDescription>> GetChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetChaosAsync").GetChaosAsync(
                timeout,
                cancellationToken);
        }

        public Task<OperationResult<ChaosScheduleDescription>> GetChaosScheduleAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetChaosScheduleAsync").GetChaosScheduleAsync(
                timeout,
                cancellationToken);
        }

        public Task<OperationResult> SetChaosScheduleAsync(
            string scheduleJson,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("SetChaosScheduleAsync").SetChaosScheduleAsync(
                scheduleJson,
                operationTimeout,
                cancellationToken);
        }

        public Task<OperationResult<ChaosEventsSegment>> GetChaosEventsAsync(
            DateTime startTime,
            DateTime endTime,
            string continuationToken,
            long maxResults,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetChaosEventsAsync").GetChaosEventsAsync(
                startTime,
                endTime,
                continuationToken,
                maxResults,
                timeout,
                cancellationToken);
        }

        public Task<OperationResult<ChaosReport>> GetChaosReportAsync(
            DateTime startTime,
            DateTime endTime,
            string continuationToken,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetChaosReportAsync").GetChaosReportAsync(
                startTime,
                endTime,
                continuationToken,
                timeout,
                cancellationToken);
        }

        public Task<OperationResult> StopChaosAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetFabricClient("StopChaosAsync").StopChaosAsync(timeout, cancellationToken);
        }

        public Task<OperationResult> StartNodeTransitionAsync(
            NodeTransitionDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("StartNodeTransitionAsync").StartNodeTransitionAsync(description, timeout, cancellationToken);
        }

        public Task<OperationResult<NodeTransitionProgress>> GetNodeTransitionProgressAsync(
            Guid operationId,
            string nodeName,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetNodeTransitionProgressAsync").GetNodeTransitionProgressAsync(operationId, nodeName, timeout, cancellationToken);
        }

        public virtual Task<OperationResult> StartPartitionDataLossRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            DataLossMode dataLossMode,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("StartPartitionDataLossRestAsync").StartPartitionDataLossRestAsync(operationId, serviceName, partitionId, dataLossMode, timeout, cancellationToken);
        }

        public virtual Task<OperationResult<PartitionDataLossProgress>> GetPartitionDataLossProgressRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetPartitionDataLossProgressRestAsync").GetPartitionDataLossProgressRestAsync(operationId, serviceName, partitionId, timeout, cancellationToken);
        }

        public virtual Task<OperationResult> StartPartitionQuorumLossRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            QuorumLossMode quorumlossMode,
            TimeSpan quorumlossDuration,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("StartPartitionQuorumLossRestAsync").StartPartitionQuorumLossRestAsync(operationId, serviceName, partitionId, quorumlossMode, quorumlossDuration, timeout, cancellationToken);
        }

        public virtual Task<OperationResult<PartitionQuorumLossProgress>> GetPartitionQuorumLossProgressRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetPartitionQuorumLossProgressRestAsync").GetPartitionQuorumLossProgressRestAsync(operationId, serviceName, partitionId, timeout, cancellationToken);
        }

        public virtual Task<OperationResult> StartPartitionRestartRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            RestartPartitionMode restartPartitionMode,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("StartPartitionRestartRestAsync").StartPartitionRestartRestAsync(operationId, serviceName, partitionId, restartPartitionMode, timeout, cancellationToken);
        }

        public virtual Task<OperationResult<PartitionRestartProgress>> GetPartitionRestartProgressRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.GetFabricClient("GetPartitionRestartProgressRestAsync").GetPartitionRestartProgressRestAsync(operationId, serviceName, partitionId, timeout, cancellationToken);
        }

        public virtual Task<OperationResult> CreateBackupPolicyAsync(string policyJson)
        {
            return this.GetFabricClient("CreateBackupPolicyAsync").CreateBackupPolicyAsync(policyJson);
        }

        public virtual Task<OperationResult> EnableBackupProtectionAsync(string policyNameJson, string fabricUri, TimeSpan timeout)
        {
            return this.GetFabricClient("CreateBackupPolicyAsync").EnableBackupProtectionAsync(policyNameJson, fabricUri, timeout);
        }

        public virtual Task<OperationResult<List<string>>> GetEventsStorePartitionEventsRestAsync(
            Guid partitionId,
            DateTime startTimeUtc,
            DateTime endTimeUtc,
            IList<string> eventsTypesFilter,
            bool excludeAnalysisEvent,
            bool skipCorrelationLookup)
        {
            return this.GetFabricClient("GetEventsStorePartitionEventsRestAsync").GetEventsStorePartitionEventsRestAsync(partitionId, startTimeUtc, endTimeUtc, eventsTypesFilter, excludeAnalysisEvent, skipCorrelationLookup);
        }
    }
}


#pragma warning restore 1591
