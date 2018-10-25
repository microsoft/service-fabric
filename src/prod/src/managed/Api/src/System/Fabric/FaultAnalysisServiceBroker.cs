// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Threading;
    using System.Threading.Tasks;

    using BOOLEAN = System.SByte;

    [SuppressMessage(FxCop.Category.Design, FxCop.Rule.TypesThatOwnDisposableFieldsShouldBeDisposable, Justification = "// TODO: Implement an IDisposable, or ensure cleanup.")]
    internal sealed class FaultAnalysisServiceBroker : NativeFaultAnalysisService.IFabricFaultAnalysisService
    {
        private const string TraceType = "FaultAnalysisServiceBroker";

        private static readonly InteropApi ThreadErrorMessageSetter = new InteropApi
                                                                            {
                                                                                CopyExceptionDetailsToThreadErrorMessage = true,
                                                                                ShouldIncludeStackTraceInThreadErrorMessage = false
                                                                            };

        private readonly IFaultAnalysisService service;

        internal FaultAnalysisServiceBroker(IFaultAnalysisService service)
        {
            this.service = service;
        }

        internal IFaultAnalysisService Service
        {
            get
            {
                return this.service;
            }
        }

        NativeCommon.IFabricAsyncOperationContext NativeFaultAnalysisService.IFabricFaultAnalysisService.BeginStartPartitionDataLoss(IntPtr invokeDataLossDescription, uint timeoutInMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedDataLossDescription = InvokeDataLossDescription.CreateFromNative(invokeDataLossDescription);

            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.InvokeDataLossAsync(
                        managedDataLossDescription,
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "FaultAnalysisServiceBroker.InvokeDataLossAsync",
                    ThreadErrorMessageSetter);
        }

        void NativeFaultAnalysisService.IFabricFaultAnalysisService.EndPartitionDataLoss(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        private Task InvokeDataLossAsync(InvokeDataLossDescription dataLossDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.InvokeDataLossAsync(dataLossDescription, timeout, cancellationToken);
        }

        NativeCommon.IFabricAsyncOperationContext NativeFaultAnalysisService.IFabricFaultAnalysisService.BeginGetPartitionDataLossProgress(Guid operationId, uint timeoutInMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.GetInvokeDataLossProgressAsync(
                        operationId,
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "FaultAnalysisServiceBroker.GetInvokeDataLossProgressAsync",
                    ThreadErrorMessageSetter);
        }

        NativeClient.IFabricPartitionDataLossProgressResult NativeFaultAnalysisService.IFabricFaultAnalysisService.EndGetPartitionDataLossProgress(NativeCommon.IFabricAsyncOperationContext context)
        {
            var dataLossProgress = AsyncTaskCallInAdapter.End<PartitionDataLossProgress>(context);
            return new InvokeDataLossProgressResult(dataLossProgress);
        }

        private Task<PartitionDataLossProgress> GetInvokeDataLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.GetInvokeDataLossProgressAsync(operationId, timeout, cancellationToken);
        }

        /// InvokeQuorumLoss
        NativeCommon.IFabricAsyncOperationContext NativeFaultAnalysisService.IFabricFaultAnalysisService.BeginStartPartitionQuorumLoss(IntPtr invokeQuorumLossDescription, uint timeoutInMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedQuorumLossDescription = InvokeQuorumLossDescription.CreateFromNative(invokeQuorumLossDescription);
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.InvokeQuorumLossAsync(
                        managedQuorumLossDescription,
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "FaultAnalysisServiceBroker.InvokeQuorumLossAsync",
                    ThreadErrorMessageSetter);
        }

        void NativeFaultAnalysisService.IFabricFaultAnalysisService.EndPartitionQuorumLoss(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        private Task InvokeQuorumLossAsync(InvokeQuorumLossDescription quorumLossDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.InvokeQuorumLossAsync(quorumLossDescription, timeout, cancellationToken);
        }

        NativeCommon.IFabricAsyncOperationContext NativeFaultAnalysisService.IFabricFaultAnalysisService.BeginGetPartitionQuorumLossProgress(Guid operationId, uint timeoutInMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.GetInvokeQuorumLossProgressAsync(
                        operationId,
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "FaultAnalysisServiceBroker.GetInvokeQuorumLossProgressAsync",
                    ThreadErrorMessageSetter);
        }

        NativeClient.IFabricPartitionQuorumLossProgressResult NativeFaultAnalysisService.IFabricFaultAnalysisService.EndGetPartitionQuorumLossProgress(NativeCommon.IFabricAsyncOperationContext context)
        {
            var quorumLossProgress = AsyncTaskCallInAdapter.End<PartitionQuorumLossProgress>(context);
            return new InvokeQuorumLossProgressResult(quorumLossProgress);
        }

        private Task<PartitionQuorumLossProgress> GetInvokeQuorumLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.GetInvokeQuorumLossProgressAsync(operationId, timeout, cancellationToken);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginStartPartitionRestart(IntPtr restartPartitionDescription, uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedRestartPartitionDescription = RestartPartitionDescription.CreateFromNative(restartPartitionDescription);

            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                        this.RestartPartitionAsync(
                        managedRestartPartitionDescription,
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "FaultAnalysisServiceBroker.BeginStartPartitionRestart",
                    ThreadErrorMessageSetter);
        }

        public void EndStartPartitionRestart(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        private Task RestartPartitionAsync(RestartPartitionDescription restartPartitionDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.RestartPartitionAsync(restartPartitionDescription, timeout, cancellationToken);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginGetPartitionRestartProgress(Guid operationId, uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                        this.GetRestartPartitionProgressAsync(
                        operationId,
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "FaultAnalysisServiceBroker.BeginGetPartitionRestartProgress",
                    ThreadErrorMessageSetter);
        }

        public NativeClient.IFabricPartitionRestartProgressResult EndGetPartitionRestartProgress(NativeCommon.IFabricAsyncOperationContext context)
        {
            var restartPartitionProgress = AsyncTaskCallInAdapter.End<PartitionRestartProgress>(context);
            return new RestartPartitionProgressResult(restartPartitionProgress);
        }

        private Task<PartitionRestartProgress> GetRestartPartitionProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.GetRestartPartitionProgressAsync(operationId, timeout, cancellationToken);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginGetTestCommandStatusList(IntPtr description, uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedDescription = TestCommandListDescription.CreateFromNative(description);
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                        this.GetTestCommandListAsync(
                        managedDescription,
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "FaultAnalysisServiceBroker.BeginGetTestCommandStatusList",
                    ThreadErrorMessageSetter);
        }

        public NativeClient.IFabricTestCommandStatusResult EndGetTestCommandStatusList(NativeCommon.IFabricAsyncOperationContext context)
        {
            return AsyncTaskCallInAdapter.End<TestCommandQueryResult>(context);
        }

        private Task<TestCommandQueryResult> GetTestCommandListAsync(TestCommandListDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.GetTestCommandListAsync(description, timeout, cancellationToken);
        }

        NativeCommon.IFabricAsyncOperationContext NativeFaultAnalysisService.IFabricFaultAnalysisService.BeginCancelTestCommand(IntPtr description, uint timeoutInMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedDescription = CancelTestCommandDescription.CreateFromNative(description);

            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                        this.CancelTestCommandAsync(
                        managedDescription,
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "FaultAnalysisServiceBroker.CancelTestCommandAsync",
                    ThreadErrorMessageSetter);
        }

        void NativeFaultAnalysisService.IFabricFaultAnalysisService.EndCancelTestCommand(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        private Task CancelTestCommandAsync(CancelTestCommandDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.CancelTestCommandAsync(description, timeout, cancellationToken);
        }

        private Task<NodeTransitionProgress> GetNodeTransitionProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.GetNodeTransitionProgressAsync(operationId, timeout, cancellationToken);
        }

        #region Chaos

        public NativeCommon.IFabricAsyncOperationContext BeginStartChaos(
            IntPtr startChaosDescription,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedStartChaosDescription = StartChaosDescription.CreateFromNative(startChaosDescription);
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.StartChaosAsync(
                        managedStartChaosDescription,
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "FaultAnalysisServiceBroker.StartChaosAsync",
                    ThreadErrorMessageSetter);
        }

        public void EndStartChaos(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        private Task StartChaosAsync(
            StartChaosDescription startChaosDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.service.StartChaosAsync(
                startChaosDescription,
                timeout,
                cancellationToken);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginStopChaos(
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.StopChaosAsync(managedTimeout, cancellationToken),
                    callback,
                    "FaultAnalysisServiceBroker.StopChaosAsync",
                    ThreadErrorMessageSetter);
        }

        public void EndStopChaos(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        private Task StopChaosAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.StopChaosAsync(timeout, cancellationToken);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginGetChaosReport(
            IntPtr getChaosReportDescription,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedGetChaosReportDescription = GetChaosReportDescription.CreateFromNative(getChaosReportDescription);
            var managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.GetChaosReportAsync(
                        managedGetChaosReportDescription,
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "FaultAnalysisServiceBroker.GetChaosReportAsync",
                    ThreadErrorMessageSetter);
        }

        public NativeClient.IFabricChaosReportResult EndGetChaosReport(NativeCommon.IFabricAsyncOperationContext context)
        {
            var chaosReport = AsyncTaskCallInAdapter.End<ChaosReport>(context);
            return new ChaosReportResult(chaosReport);
        }

        private Task<ChaosReport> GetChaosReportAsync(
            GetChaosReportDescription getChaosReportDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.service.GetChaosReportAsync(
                getChaosReportDescription,
                timeout,
                cancellationToken);
        }

        #endregion Chaos

        public NativeCommon.IFabricAsyncOperationContext BeginGetStoppedNodeList(uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "In FaultAnalysisServiceBroker.BeginGetStoppedNodeList");

            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                        this.GetStoppedNodeListAsync(
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "FaultAnalysisServiceBroker.GetStoppedNodeListAsync",
                    ThreadErrorMessageSetter);
        }

        public NativeCommon.IFabricStringResult EndGetStoppedNodeList(NativeCommon.IFabricAsyncOperationContext context)
        {
            string result = AsyncTaskCallInAdapter.End<string>(context);

            return new StringResult(result);
        }

        private Task<string> GetStoppedNodeListAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.GetStoppedNodeListAsync(timeout, cancellationToken);
        }

        NativeCommon.IFabricAsyncOperationContext NativeFaultAnalysisService.IFabricFaultAnalysisService.BeginStartNodeTransition(IntPtr description, uint timeoutInMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "In FaultAnalysisServiceBroker.BeginStartNodeTransition");

            var managedDescription = NodeTransitionDescription.CreateFromNative(description);

            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            TestabilityTrace.TraceSource.WriteInfo(
                TraceType,
                "In FaultAnalysisServiceBroker.BeginStartNodeTransition values are: {0} | {1} | {2} | {3}", managedDescription.NodeTransitionType, managedDescription.OperationId, managedDescription.NodeName, managedDescription.NodeInstanceId);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.StartNodeTransitionAsync(
                        managedDescription,
                        managedTimeout,
                        cancellationToken),
                callback,
                "FaultAnalysisServiceBroker.StartNodeTransitionAsync",
                ThreadErrorMessageSetter);
        }

        void NativeFaultAnalysisService.IFabricFaultAnalysisService.EndStartNodeTransition(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        private Task StartNodeTransitionAsync(NodeTransitionDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.StartNodeTransitionAsync(description, timeout, cancellationToken);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginGetNodeTransitionProgress(Guid operationId, uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            TestabilityTrace.TraceSource.WriteNoise(TraceType, "Enter BeginGetNodeTransitionProgress");
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            NativeCommon.IFabricAsyncOperationContext temp = null;
            try
            {
                temp = Utility.WrapNativeAsyncMethodImplementation(
                    (cancellationToken) =>
                            this.GetNodeTransitionProgressAsync(
                            operationId,
                            managedTimeout,
                            cancellationToken),
                        callback,
                        "FaultAnalysisServiceBroker.GetNodeTransitionProgressAsync",
                        ThreadErrorMessageSetter);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "BeginGetTransitionStartProgress caught {0}", e);
                throw;
            }

            return temp;
        }

        public NativeClient.IFabricNodeTransitionProgressResult EndGetNodeTransitionProgress(NativeCommon.IFabricAsyncOperationContext context)
        {
            try
            {
                var nodeTransitionProgress = AsyncTaskCallInAdapter.End<NodeTransitionProgress>(context);
                return new NodeTransitionProgressResult(nodeTransitionProgress);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, "EndGetNodeTransitionProgress caught {0}", e);
                throw;
            }
        }

        #region CallSystemService
        public NativeCommon.IFabricAsyncOperationContext BeginCallSystemService(
            IntPtr action,
            IntPtr inputBlob,
            UInt32 timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.CallSystemServiceAsync(
                        NativeTypes.FromNativeString(action),
                        NativeTypes.FromNativeString(inputBlob),
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "FaultAnalysisServiceBroker.CallSystemService",
                    ThreadErrorMessageSetter);
        }

        public NativeCommon.IFabricStringResult EndCallSystemService(NativeCommon.IFabricAsyncOperationContext context)
        {
            var outputBlob = AsyncTaskCallInAdapter.End<String>(context);
            return new StringResult(outputBlob);
        }

        private Task<string> CallSystemServiceAsync(string action, string inputBlob, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.CallSystemService(action, inputBlob, timeout, cancellationToken);
        }

        #endregion
    }
}