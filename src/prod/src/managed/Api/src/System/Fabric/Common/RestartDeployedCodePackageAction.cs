// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    internal class RestartDeployedCodePackageAction : FabricTestAction<RestartDeployedCodePackageResult>
    {
        private const string TraceSource = "RestartDeployedCodePackageAction";

        public RestartDeployedCodePackageAction(
            string nodeName,
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string codePackageName,
            long codePackageInstanceId)
        {
            this.ApplicationName = applicationName;
            this.NodeName = nodeName;
            this.ServiceManifestName = serviceManifestName;
            this.ServicePackageActivationId = servicePackageActivationId ?? string.Empty;
            this.CodePackageName = codePackageName;
            this.CodePackageInstanceId = codePackageInstanceId;
            this.CompletionMode = CompletionMode.Verify;
        }

        public RestartDeployedCodePackageAction(
            Uri applicationName,
            ReplicaSelector replicaSelector)
        {
            this.ApplicationName = applicationName;
            this.ReplicaSelector = replicaSelector;
            this.CompletionMode = CompletionMode.Verify;
        }

        public Uri ApplicationName { get; set; }

        public string NodeName { get; set; }

        public string ServiceManifestName { get; set; }

        public string ServicePackageActivationId { get; set; }

        public string CodePackageName { get; set; }

        public long CodePackageInstanceId { get; set; }

        public ReplicaSelector ReplicaSelector { get; set; }

        public CompletionMode CompletionMode { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(RestartDeployedCodePackageActionHandler); }
        }

        private class RestartDeployedCodePackageActionHandler : FabricTestActionHandler<RestartDeployedCodePackageAction>
        {
            private TimeoutHelper helper;

            private enum EntryPointType
            {
                None,
                Setup,
                Main
            }

            private sealed class EntrypointAndType
            {
                public EntrypointAndType(CodePackageEntryPoint entryPoint, EntryPointType entryPointType)
                {
                    this.EntryPoint = entryPoint;
                    this.EntryPointType = entryPointType;
                }

                public CodePackageEntryPoint EntryPoint { get; }
                public EntryPointType EntryPointType { get; }
            }

            protected override async Task ExecuteActionAsync(FabricTestContext testContext, RestartDeployedCodePackageAction action, CancellationToken cancellationToken)
            {
                this.helper = new TimeoutHelper(action.ActionTimeout);

                string nodeName = action.NodeName;
                Uri applicationName = action.ApplicationName;
                string serviceManifestName = action.ServiceManifestName;
                string servicePackageActivationId = action.ServicePackageActivationId;
                string codePackageName = action.CodePackageName;
                SelectedReplica replicaSelectorResult = SelectedReplica.None;
                ThrowIf.Null(applicationName, "ApplicationName");

                if (string.IsNullOrEmpty(nodeName) ||
                    string.IsNullOrEmpty(serviceManifestName) ||
                    string.IsNullOrEmpty(codePackageName))
                {
                    ThrowIf.Null(action.ReplicaSelector, "ReplicaSelector");

                    var getReplicaStateAction = new GetSelectedReplicaStateAction(action.ReplicaSelector)
                    {
                        RequestTimeout = action.RequestTimeout,
                        ActionTimeout = this.helper.GetRemainingTime()
                    };

                    await testContext.ActionExecutor.RunAsync(getReplicaStateAction, cancellationToken).ConfigureAwait(false);
                    var replicaStateActionResult = getReplicaStateAction.Result;
                    ReleaseAssert.AssertIf(replicaStateActionResult == null, "replicaStateActionResult cannot be null");
                    replicaSelectorResult = replicaStateActionResult.Item1;
                    ReleaseAssert.AssertIf(replicaSelectorResult == null || replicaSelectorResult.SelectedPartition == null, 
                        "replicaSelectorResult cannot be null or for a non-null replicaSelectorResult, the selected partition must be non-null");
                    Guid partitionId = replicaStateActionResult.Item1.SelectedPartition.PartitionId;

                    Replica replicaStateResult = replicaStateActionResult.Item2;
                    ReleaseAssert.AssertIf(replicaStateResult == null, "replicaStateResult cannot be null");

                    nodeName = replicaStateResult.NodeName;

                    var deployedReplicaListResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<DeployedServiceReplicaList>(
                        () => testContext.FabricClient.QueryManager.GetDeployedReplicaListAsync(
                            nodeName, 
                            applicationName, 
                            null,
                            partitionId, 
                            action.RequestTimeout, 
                            cancellationToken),
                        this.helper.GetRemainingTime(),
                        cancellationToken).ConfigureAwait(false);

                    DeployedServiceReplica selectedReplica = deployedReplicaListResult.FirstOrDefault(r => r.Partitionid == partitionId);
                    if (selectedReplica == null)
                    {
                        throw new FabricException(
                            StringHelper.Format(StringResources.Error_DidNotFindDeployedReplicaOnNode, partitionId, nodeName),
                            FabricErrorCode.ReplicaDoesNotExist);
                    }

                    serviceManifestName = selectedReplica.ServiceManifestName;
                    servicePackageActivationId = selectedReplica.ServicePackageActivationId;
                    codePackageName = selectedReplica.CodePackageName;
                }

                ActionTraceSource.WriteInfo(TraceSource, "SelectedReplica: serviceManifestName: {0}, servicePackageActivationId: {1}, codePackageName: {2}", serviceManifestName, servicePackageActivationId, codePackageName);

                DeployedCodePackage deployedCodePackageListResult = await this.GetCodePackageInfoAsync(testContext, nodeName, applicationName, serviceManifestName, servicePackageActivationId, codePackageName, action, cancellationToken).ConfigureAwait(false);

                var codepackageEntrypointToRestart = GetCodepackageEntrypointToRestart(action, deployedCodePackageListResult);

                await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => testContext.FabricClient.FaultManager.RestartDeployedCodePackageUsingNodeNameAsync(
                            nodeName, 
                            applicationName, 
                            serviceManifestName,
                            servicePackageActivationId,
                            codePackageName,
                            codepackageEntrypointToRestart.EntryPoint.CodePackageInstanceId, 
                            action.RequestTimeout, 
                            cancellationToken),
                        this.helper.GetRemainingTime(),
                        cancellationToken).ConfigureAwait(false);

                if (action.CompletionMode == CompletionMode.Verify)
                {
                    bool success = false;
                    while (this.helper.GetRemainingTime() > TimeSpan.Zero)
                    {
                        var deployedCodePackageListResultAfterRestart = await this.GetCodePackageInfoAsync(testContext, nodeName, applicationName, serviceManifestName, servicePackageActivationId, codePackageName, action, cancellationToken).ConfigureAwait(false);

                        if (deployedCodePackageListResultAfterRestart != null)
                        {
                            var entryPointAfterRestart = codepackageEntrypointToRestart.EntryPointType == EntryPointType.Main ? deployedCodePackageListResultAfterRestart.EntryPoint : deployedCodePackageListResultAfterRestart.SetupEntryPoint;
                            if (entryPointAfterRestart != null && entryPointAfterRestart.CodePackageInstanceId > codepackageEntrypointToRestart.EntryPoint.CodePackageInstanceId && entryPointAfterRestart.EntryPointStatus == EntryPointStatus.Started)
                            {
                                success = true;
                                break;
                            }
                        }

                        ActionTraceSource.WriteInfo(TraceSource, "CodePackage = {0}:{1}:{2} not yet restarted. Retrying...", nodeName, applicationName, codePackageName);
                        await AsyncWaiter.WaitAsync(TimeSpan.FromSeconds(5), cancellationToken).ConfigureAwait(false);
                    }

                    if (!success)
                    {
                        throw new TimeoutException(StringHelper.Format(StringResources.Error_TestabilityActionTimeout,
                            "RestartDeployedCodePackage",
                            applicationName));
                    }
                }

                action.Result = new RestartDeployedCodePackageResult(
                    nodeName,
                    applicationName,
                    serviceManifestName,
                    servicePackageActivationId,
                    codePackageName,
                    codepackageEntrypointToRestart.EntryPoint.CodePackageInstanceId,
                    replicaSelectorResult);

                ResultTraceString = StringHelper.Format("RestartCodePackageAction succeeded for {0}:{1}:{2} with CompletionMode = {3}", nodeName, applicationName, codePackageName, action.CompletionMode);
            }

            private async Task<DeployedCodePackage> GetCodePackageInfoAsync(
                FabricTestContext testContext,
                string nodeName,
                Uri applicationName,
                string serviceManifestName,
                string servicePackageActivationId,
                string codePackageName,
                RestartDeployedCodePackageAction action,
                CancellationToken cancellationToken)
            {
                var deployedCodePackageListResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<DeployedCodePackageList>(
                        () => testContext.FabricClient.QueryManager.GetDeployedCodePackageListAsync(
                            nodeName, 
                            applicationName, 
                            serviceManifestName, 
                            codePackageName, 
                            action.RequestTimeout, 
                            cancellationToken),
                        this.helper.GetRemainingTime(),
                        cancellationToken).ConfigureAwait(false);

                if (deployedCodePackageListResult == null || deployedCodePackageListResult.Count == 0)
                {
                    throw new FabricException(
                        StringHelper.Format(StringResources.Error_CodePackageNotDeployedOnNode, applicationName, codePackageName, nodeName),
                        FabricErrorCode.CodePackageNotFound);
                }

                return deployedCodePackageListResult.FirstOrDefault(
                    (cp) => cp.CodePackageName.Equals(codePackageName) &&
                            cp.ServicePackageActivationId == servicePackageActivationId);
            }

            private static EntrypointAndType GetCodepackageEntrypointToRestart(RestartDeployedCodePackageAction action, DeployedCodePackage codepackage)
            {
                if (codepackage == null || 
                    (codepackage.EntryPoint == null &&
                    codepackage.SetupEntryPoint == null))
                {
                    throw new FabricException(
                        StringHelper.Format(StringResources.Error_CodePackageNotDeployedOnNode, action.ApplicationName, action.CodePackageName, action.NodeName),
                        FabricErrorCode.CodePackageNotFound);
                }

                CodePackageEntryPoint restartedEntryPoint = null;
                EntryPointType restartedEntryPointType = EntryPointType.None;

                if (codepackage.EntryPoint != null && codepackage.EntryPoint.EntryPointStatus == EntryPointStatus.Started)
                {
                    restartedEntryPoint = codepackage.EntryPoint;
                    restartedEntryPointType = EntryPointType.Main;
                }
                else if (codepackage.SetupEntryPoint != null && codepackage.SetupEntryPoint.EntryPointStatus == EntryPointStatus.Started)
                {
                    restartedEntryPoint = codepackage.SetupEntryPoint;
                    restartedEntryPointType = EntryPointType.Setup;
                }

                if(restartedEntryPoint == null || restartedEntryPointType == EntryPointType.None)
                {
                    throw new InvalidOperationException(StringResources.Error_CodePackageCannotBeRestarted);
                }

                if (action.CodePackageInstanceId != 0 && restartedEntryPoint.CodePackageInstanceId != action.CodePackageInstanceId)
                {
                    throw new FabricException(StringResources.Error_InstanceIdMismatch, FabricErrorCode.InstanceIdMismatch);
                }

                return new EntrypointAndType(restartedEntryPoint, restartedEntryPointType);
            }
        }
    }
}