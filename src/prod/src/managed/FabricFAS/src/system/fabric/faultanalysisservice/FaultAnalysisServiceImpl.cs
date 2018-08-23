// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.FaultAnalysis.Service.Actions;
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Fabric.FaultAnalysis.Service.Chaos;
    using System.Fabric.FaultAnalysis.Service.Common;
    using System.Fabric.FaultAnalysis.Service.Engine;
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class FaultAnalysisServiceImpl : IFaultAnalysisService
    {
        private const string TraceType = "FAS.FaultAnalysisServiceImpl";

        private int minStopDurationInSeconds;
        private int maxStopDurationInSeconds;

        private Dictionary<string, ActionHandler> actionMapping;

        public FaultAnalysisServiceImpl()
        {
            System.Fabric.Common.NativeConfigStore configStore = System.Fabric.Common.NativeConfigStore.FabricGetConfigStore();
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "'{0}', '{1}'", FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.StopNodeMinDurationInSecondsName);
            string minStopDurationAsString = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.StopNodeMinDurationInSecondsName);
            this.minStopDurationInSeconds = string.IsNullOrEmpty(minStopDurationAsString) ? FASConstants.DefaultStopNodeMinDurationInSeconds : int.Parse(minStopDurationAsString);

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "'{0}', '{1}', '{2}'", FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.StopNodeMinDurationInSecondsName, this.minStopDurationInSeconds);

            string maxStopDurationAsString = configStore.ReadUnencryptedString(FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.StopNodeMaxDurationInSecondsName);
            this.maxStopDurationInSeconds = string.IsNullOrEmpty(maxStopDurationAsString) ? FASConstants.DefaultStopNodeMaxDurationInSeconds : int.Parse(maxStopDurationAsString);
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "'{0}', '{1}', '{2}'", FASConstants.FaultAnalysisServiceConfigSectionName, FASConstants.StopNodeMaxDurationInSecondsName, this.maxStopDurationInSeconds);
        }

        private delegate Task<string> ActionHandler(string inputBlob, TimeSpan timeout, CancellationToken cancellationToken);

        internal FaultAnalysisServiceMessageProcessor MessageProcessor { get; set; }

        internal ChaosMessageProcessor ChaosMessageProcessor { get; set; }

        public async Task InvokeDataLossAsync(
            InvokeDataLossDescription invokeDataLossDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            this.ThrowIfNotReady();

            TestabilityTrace.TraceSource.WriteInfo(
                TraceType,
                "{0} - Processing {1} PartitionSelector: {2}, DataLossMode: {3}",
                invokeDataLossDescription.OperationId,
                ActionType.InvokeDataLoss,
                invokeDataLossDescription.PartitionSelector,
                invokeDataLossDescription.DataLossMode);

            try
            {
                await this.MessageProcessor.ProcessDataLossCommandAsync(
                    invokeDataLossDescription.OperationId,
                    invokeDataLossDescription.PartitionSelector,
                    invokeDataLossDescription.DataLossMode,
                    timeout,
                    null);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception occurred: {1}", invokeDataLossDescription.OperationId, e.Message);
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - Intent saved for {1}", invokeDataLossDescription.OperationId, ActionType.InvokeDataLoss);
        }

        public async Task<PartitionDataLossProgress> GetInvokeDataLossProgressAsync(
           Guid operationId,
           TimeSpan timeout,
           CancellationToken cancellationToken)
        {
            this.ThrowIfNotReady();
            PartitionDataLossProgress progress = null;
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Inside GetInvokeDataLossProgressAsync, operationId = {0}", operationId);

            try
            {
                ActionStateBase actionState = await this.MessageProcessor.ProcessGetProgressAsync(operationId, timeout, cancellationToken);
                StepStateNames stateName = actionState.StateProgress.Peek();

                TestCommandProgressState state = FaultAnalysisServiceUtility.ConvertState(actionState, TraceType);

                InvokeDataLossState invokeDataLossState = actionState as InvokeDataLossState;
                if (invokeDataLossState == null)
                {
                    throw new InvalidCastException("State object could not be converted");
                }

                StepStateNames stepState = actionState.StateProgress.Peek();
                var selectedPartition = new SelectedPartition
                {
                    ServiceName = invokeDataLossState.Info.PartitionSelector.ServiceName,
                    PartitionId = invokeDataLossState.Info.PartitionId
                };

                PartitionDataLossResult result = new PartitionDataLossResult(selectedPartition, actionState.ErrorCausingRollback);

                progress = new PartitionDataLossProgress(state, result);
                TestabilityTrace.TraceSource.WriteInfo(
                    TraceType,
                    "{0} - {1} progress - {2}, Exception - {3}",
                    operationId,
                    ActionType.InvokeDataLoss,
                    progress.Result != null ? progress.Result.SelectedPartition.ToString() : FASConstants.UnavailableMessage,
                    (progress.Result != null && progress.Result.Exception != null) ? progress.Result.Exception.ToString() : FASConstants.UnavailableMessage);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Caught {1}", operationId, e.ToString());
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }

            return progress;
        }

        public async Task InvokeQuorumLossAsync(
            InvokeQuorumLossDescription invokeQuorumLossDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            this.ThrowIfNotReady();

            TestabilityTrace.TraceSource.WriteInfo(
                TraceType,
                "{0} - Processing {1} PartitionSelector: {2}, QuorumLossMode: {3}, QuorumLossDurationDuration: {4}",
                invokeQuorumLossDescription.OperationId,
                ActionType.InvokeQuorumLoss,
                invokeQuorumLossDescription.PartitionSelector,
                invokeQuorumLossDescription.QuorumLossMode,
                invokeQuorumLossDescription.QuorumLossDuration);
            try
            {
                await this.MessageProcessor.ProcessQuorumLossCommandAsync(
                    invokeQuorumLossDescription.OperationId,
                    invokeQuorumLossDescription.PartitionSelector,
                    invokeQuorumLossDescription.QuorumLossMode,
                    invokeQuorumLossDescription.QuorumLossDuration,
                    timeout,
                    null).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception occurred: {1}", invokeQuorumLossDescription.OperationId, e.Message);
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - Intent saved for {1}", invokeQuorumLossDescription.OperationId, ActionType.InvokeQuorumLoss);
        }

        public async Task<PartitionQuorumLossProgress> GetInvokeQuorumLossProgressAsync(
            Guid operationId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            this.ThrowIfNotReady();
            PartitionQuorumLossProgress progress = null;

            try
            {
                ActionStateBase actionState = await this.MessageProcessor.ProcessGetProgressAsync(operationId, timeout, cancellationToken);
                StepStateNames stateName = actionState.StateProgress.Peek();

                TestCommandProgressState state = FaultAnalysisServiceUtility.ConvertState(actionState, TraceType);
                InvokeQuorumLossState invokeQuorumLossState = actionState as InvokeQuorumLossState;

                var selectedPartition = new SelectedPartition
                {
                    ServiceName = invokeQuorumLossState.Info.PartitionSelector.ServiceName,
                    PartitionId = invokeQuorumLossState.Info.PartitionId
                };

                PartitionQuorumLossResult result = new PartitionQuorumLossResult(selectedPartition, actionState.ErrorCausingRollback);

                progress = new PartitionQuorumLossProgress(state, result);

                TestabilityTrace.TraceSource.WriteInfo(
                    TraceType,
                    "{0} - {1} progress - {2}, Exception - {3}",
                    operationId,
                    ActionType.InvokeQuorumLoss,
                    progress.Result != null ? progress.Result.SelectedPartition.ToString() : FASConstants.UnavailableMessage,
                    (progress.Result != null && progress.Result.Exception != null) ? progress.Result.Exception.ToString() : FASConstants.UnavailableMessage);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception: {1}", operationId, e.ToString());
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }

            return progress;
        }

        public async Task RestartPartitionAsync(
            RestartPartitionDescription restartPartitionDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            this.ThrowIfNotReady();

            TestabilityTrace.TraceSource.WriteInfo(
                TraceType,
                "{0} - Processing {1} PartitionSelector: {2}, Mode: {3}",
                restartPartitionDescription.OperationId,
                ActionType.RestartPartition,
                restartPartitionDescription.PartitionSelector,
                restartPartitionDescription.RestartPartitionMode);

            try
            {
                await this.MessageProcessor.ProcessRestartPartitionCommandAsync(
                    restartPartitionDescription.OperationId,
                    restartPartitionDescription.PartitionSelector,
                    restartPartitionDescription.RestartPartitionMode,
                    timeout,
                    null);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception occurred: {1}", restartPartitionDescription.OperationId, e.Message);
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - Intent saved for {1}", restartPartitionDescription.OperationId, ActionType.RestartPartition);
        }

        public async Task<PartitionRestartProgress> GetRestartPartitionProgressAsync(
            Guid operationId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            this.ThrowIfNotReady();
            PartitionRestartProgress progress = null;

            try
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "GetRestartPartitionProgressAsync calling message processor");
                ActionStateBase actionState = await this.MessageProcessor.ProcessGetProgressAsync(operationId, timeout, cancellationToken);
                StepStateNames stateName = actionState.StateProgress.Peek();

                TestCommandProgressState state = FaultAnalysisServiceUtility.ConvertState(actionState, TraceType);
                RestartPartitionState restartPartitionState = actionState as RestartPartitionState;
                TestabilityTrace.TraceSource.WriteInfo(
                    TraceType,
                    "RestartPartition - serviceName={0}, partitionId={1}",
                    restartPartitionState.Info.PartitionSelector.ServiceName.ToString(),
                    restartPartitionState.Info.PartitionId);

                var selectedPartition = new SelectedPartition
                {
                    ServiceName = restartPartitionState.Info.PartitionSelector.ServiceName,
                    PartitionId = restartPartitionState.Info.PartitionId
                };

                PartitionRestartResult result = new PartitionRestartResult(selectedPartition, actionState.ErrorCausingRollback);

                progress = new PartitionRestartProgress(state, result);
                TestabilityTrace.TraceSource.WriteInfo(
                    TraceType,
                    "{0} - {1} progress - {2}, Exception - {3}",
                    operationId,
                    ActionType.RestartPartition,
                    progress.Result != null ? progress.Result.SelectedPartition.ToString() : FASConstants.UnavailableMessage,
                    (progress.Result != null && progress.Result.Exception != null) ? progress.Result.Exception.ToString() : FASConstants.UnavailableMessage);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Caught {1}", operationId, e.ToString());
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }

            return progress;
        }

        public async Task<TestCommandQueryResult> GetTestCommandListAsync(
            TestCommandListDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            this.ThrowIfNotReady();

            TestCommandQueryResult result = null;

            try
            {
                result = await this.MessageProcessor.ProcessGetTestCommandListAsync(description).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }

            return result;
        }

        public async Task CancelTestCommandAsync(
            CancelTestCommandDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            this.ThrowIfNotReady();

            try
            {
                await this.MessageProcessor.CancelTestCommandAsync(description.OperationId, description.Force).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }
        }

        public async Task<string> GetStoppedNodeListAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            this.ThrowIfNotReady();

            string result = null;

            try
            {
                result = await this.MessageProcessor.ProcessGetStoppedNodeListAsync().ConfigureAwait(false);
            }
            catch (Exception e)
            {
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }

            return result;
        }

        #region Chaos

        public async Task StartChaosAsync(
            StartChaosDescription startChaosDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            this.ThrowIfNotReady(() => this.ChaosMessageProcessor != null);

            try
            {
                await
                    this.ChaosMessageProcessor.ProcessStartChaosAsync(
                        startChaosDescription.ChaosParameters,
                        cancellationToken).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "StartChaosAsync: Exception occurred: {0}", e.Message);
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }
        }

        public async Task StopChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter StopChaosAsync, datetimeutc={0}", DateTime.UtcNow);

            this.ThrowIfNotReady(() => this.ChaosMessageProcessor != null);

            try
            {
                await this.ChaosMessageProcessor.ProcessStopChaosAsync(cancellationToken).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "StopChaosAsync - Exception occurred: {0}", e.Message);
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }
        }

        public async Task<ChaosReport> GetChaosReportAsync(
            GetChaosReportDescription getChaosReportDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            Guid activityId = Guid.NewGuid();

            this.ThrowIfNotReady(() => this.ChaosMessageProcessor != null);

            TestabilityTrace.TraceSource.WriteNoise(TraceType, "{0}:Enter GetChaosReportAsync, description={1}", activityId, getChaosReportDescription.ToString());

            ChaosReport report = null;

            try
            {
                ChaosReportFilter reportFilter = null;
                var continuationToken = getChaosReportDescription.ContinuationToken;

                if (!string.IsNullOrEmpty(continuationToken))
                {
                    reportFilter = await this.ChaosMessageProcessor.GetReportFilterAsync(activityId, continuationToken).ConfigureAwait(false);
                }
                else
                {
                    var continuationTokenGuid = Guid.NewGuid().ToString();
                    var fileTimeUtcTicks = DateTime.UtcNow.ToFileTimeUtc();

                    continuationToken = string.Format(FASConstants.ChaosReportContinuationTokenFormat, fileTimeUtcTicks, continuationTokenGuid);
                }

                reportFilter = reportFilter ?? getChaosReportDescription.Filter;

                if (!string.IsNullOrEmpty(getChaosReportDescription.ClientType)
                    && (getChaosReportDescription.ClientType.Equals(ChaosConstants.RestClientTypeName, StringComparison.OrdinalIgnoreCase)
                        || getChaosReportDescription.ClientType.Equals(ChaosConstants.NativeClientTypeName, StringComparison.OrdinalIgnoreCase)))
                {
                    ChaosUtility.DisableOptimizationForValidationFailedEvent = true;
                }
                else
                {
                    ChaosUtility.DisableOptimizationForValidationFailedEvent = false;
                }

                report = await this.ChaosMessageProcessor.ProcessGetChaosReportAsync(
                            activityId,
                            reportFilter,
                            continuationToken).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0}:GetChaosReportAsync - Exception occurred: {1}", activityId, e.Message);
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }

            TestabilityTrace.TraceSource.WriteNoise(TraceType, "{0}:GetChaosReportAsync - returning report.", activityId);

            return report;
        }
        #endregion

        public async Task StartNodeTransitionAsync(
            NodeTransitionDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            this.ThrowIfNotReady();

            TestabilityTrace.TraceSource.WriteInfo(
                TraceType,
                "FaultAnalysisServiceImpl StartNodeTransitionAsync operationId={0} NodeTransitionType={1} - {2}:{3}",
                description.OperationId,
                description.NodeTransitionType,
                description.NodeName,
                description.NodeInstanceId);
            try
            {
                if (description.NodeTransitionType == NodeTransitionType.Start)
                {
                    NodeStartDescription translatedDescription = (NodeStartDescription)description;
                    await this.MessageProcessor.ProcessStartNodeCommandAsync(
                        translatedDescription.OperationId,
                        translatedDescription.NodeName,
                        translatedDescription.NodeInstanceId,
                        timeout,
                        null).ConfigureAwait(false);
                }
                else if (description.NodeTransitionType == NodeTransitionType.Stop)
                {
                    NodeStopDescription translatedDescription = (NodeStopDescription)description;
                    if (translatedDescription.StopDurationInSeconds < this.minStopDurationInSeconds
                        || translatedDescription.StopDurationInSeconds > this.maxStopDurationInSeconds)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(
                            TraceType,
                            "Duration in seconds '{0}' is out of range.  Min='{1}' Max='{2}'",
                            translatedDescription.StopDurationInSeconds,
                            this.minStopDurationInSeconds,
                            this.maxStopDurationInSeconds);
                        throw FaultAnalysisServiceUtility.CreateException(
                            TraceType,
                            Interop.NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_DURATION,
                            "StopDurationInSeconds is out of range");
                    }

                    await this.MessageProcessor.ProcessStopNodeCommandAsync(
                        translatedDescription.OperationId,
                        translatedDescription.NodeName,
                        translatedDescription.NodeInstanceId,
                        translatedDescription.StopDurationInSeconds,
                        timeout,
                        null).ConfigureAwait(false);
                }
                else
                {
                    throw new NotImplementedException();
                }
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception occurred: {1}", description.OperationId, e.Message);
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - Intent saved for {1}", description.OperationId, ActionType.StartNode);
        }

        public async Task<NodeTransitionProgress> GetNodeTransitionProgressAsync(
            Guid operationId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            this.ThrowIfNotReady();
            NodeTransitionProgress progress = null;

            try
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "GetNodeTransitionProgressAsync got operation id {0}", operationId);
                ActionStateBase actionState = await this.MessageProcessor.ProcessGetProgressAsync(operationId, timeout, cancellationToken);
                StepStateNames stateName = actionState.StateProgress.Peek();
                TestCommandProgressState state = FaultAnalysisServiceUtility.ConvertState(actionState, TraceType);

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - GetNodeTransitionProgressAsync reading nodecommandstate", operationId);
                NodeCommandState nodeState = actionState as NodeCommandState;
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} NodeCommandState is null={1}", operationId, nodeState == null);

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} - GetNodeTransitionProgressAsync node name={1}, node instance id={2}", operationId, nodeState.Info.NodeName, nodeState.Info.InputNodeInstanceId);
                NodeResult nodeResult = new NodeResult(nodeState.Info.NodeName, nodeState.Info.InputNodeInstanceId);
                NodeCommandResult result = new NodeCommandResult(nodeResult, actionState.ErrorCausingRollback);

                progress = new NodeTransitionProgress(state, result);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Caught {1}", operationId, e.ToString());
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }

            return progress;
        }

        public async Task<string> CallSystemService(
            string action,
            string inputBlob,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            this.ThrowIfNotReady();

            string outputBlob = null;

            try
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter CallSystemService for {0}.", action);

                if (this.actionMapping.ContainsKey(action))
                {
                    outputBlob = await this.actionMapping[action](inputBlob, timeout, cancellationToken).ConfigureAwait(false);
                }
                else
                {
                    throw FaultAnalysisServiceUtility.CreateException(TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.E_NOTIMPL, "{0} is not implemented", action);
                }
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "CallSystemService error: {0}", e);
                FaultAnalysisServiceUtility.ThrowTransientExceptionIfRetryable(e);

                throw;
            }

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Exit CallSystemService for {0} with output {1}.", action, outputBlob);
            return outputBlob;
        }

        internal void LoadActionMapping()
        {
            // The key is the action name defined in XXXTcpMessage.h
            this.actionMapping = new Dictionary<string, ActionHandler>(StringComparer.OrdinalIgnoreCase)
            {
                { "GetChaosAction",          this.ChaosMessageProcessor.ProcessGetChaosAsync },
                { "PostChaosScheduleAction", this.ChaosMessageProcessor.ProcessPostChaosScheduleAsync },
                { "GetChaosScheduleAction",  this.ChaosMessageProcessor.ProcessGetChaosScheduleAsync },
                { "GetChaosEventsAction",    this.ChaosMessageProcessor.ProcessGetChaosEventsAsync },
            };
        }

        private void ThrowIfNotReady(Func<bool> readinessCriteria = null)
        {
            if (this.MessageProcessor == null || (readinessCriteria != null && !readinessCriteria()))
            {
                throw FaultAnalysisServiceUtility.CreateException(TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.E_ABORT, "Service is not accepting client calls yet");
            }

            if (this.MessageProcessor.Partition.WriteStatus != PartitionAccessStatus.Granted)
            {
                // not primary, reconfiguration pending, and no write quorum all get translated to E_ABORT by the gateway, so there's no point in throwing a specific exception for each here.
                throw FaultAnalysisServiceUtility.CreateException(TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.E_ABORT, string.Empty);
            }
        }

        private void ThrowEAbort()
        {
            throw FaultAnalysisServiceUtility.CreateException(TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.E_ABORT, "Operation cancelled");
        }
    }
}