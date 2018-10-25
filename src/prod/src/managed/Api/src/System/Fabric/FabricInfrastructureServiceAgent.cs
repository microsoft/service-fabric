// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Reflection;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Preserve order of public members from V1.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1204:StaticElementsMustAppearBeforeInstanceElements", Justification = "Current grouping improves readability.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1202:ElementsMustBeOrderedByAccess", Justification = "Current grouping improves readability.")]
    internal sealed class FabricInfrastructureServiceAgent
    {
        #region Fields & Properties

        internal static readonly TimeSpan DefaultTimeout = TimeSpan.FromMinutes(1);

        private NativeInfrastructureService.IFabricInfrastructureServiceAgent nativeAgent;

        #endregion

        #region Constructors & Initialization

        public FabricInfrastructureServiceAgent()
        {
            Utility.WrapNativeSyncInvokeInMTA(() => this.CreateNativeAgent(), "InfrastructureServiceAgent.CreateNativeAgent");
        }

        private void CreateNativeAgent()
        {
            Guid iid = typeof(NativeInfrastructureService.IFabricInfrastructureServiceAgent).GetTypeInfo().GUID;

            //// PInvoke call
            this.nativeAgent = NativeInfrastructureService.CreateFabricInfrastructureServiceAgent(ref iid);
        }

        #endregion

        #region API

        public void RegisterInfrastructureServiceFactory(IStatefulServiceFactory factory)
        {
            var activationContext = FabricRuntime.GetActivationContext();
            var broker = new ServiceFactoryBroker(factory, activationContext);

            Utility.WrapNativeSyncInvokeInMTA(() => this.nativeAgent.RegisterInfrastructureServiceFactory(broker), "InfrastructureServiceAgent.RegisterInfrastructureServiceFactory");
        }

        public string RegisterInfrastructureService(Guid partitionId, long replicaId, IInfrastructureService service)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNull();

            return Utility.WrapNativeSyncInvokeInMTA(() => this.RegisterInfrastructureServiceHelper(partitionId, replicaId, service), "InfrastructureServiceAgent.RegisterInfrastructureService");
        }

        public void UnregisterInfrastructureService(Guid partitionId, long replicaId)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNull();

            Utility.WrapNativeSyncInvokeInMTA(() => this.UnregisterInfrastructureServiceHelper(partitionId, replicaId), "InfrastructureServiceAgent.UnregisterInfrastructureService");
        }

        public Task StartInfrastructureTask(InfrastructureTaskDescription description)
        {
            return this.StartInfrastructureTaskAsync(description, FabricInfrastructureServiceAgent.DefaultTimeout, CancellationToken.None);
        }

        public Task StartInfrastructureTaskAsync(InfrastructureTaskDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Requires.Argument<InfrastructureTaskDescription>("description", description).NotNull();

            return this.StartInfrastructureTaskAsyncHelper(description, timeout, cancellationToken);
        }

        public Task FinishInfrastructureTask(string taskId, long instanceId)
        {
            return this.FinishInfrastructureTaskAsync(taskId, instanceId, FabricInfrastructureServiceAgent.DefaultTimeout, CancellationToken.None);
        }

        public Task FinishInfrastructureTaskAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FinishInfrastructureTaskAsyncHelper(taskId, instanceId, timeout, cancellationToken);
        }

        public Task<InfrastructureTaskQueryResult> QueryInfrastructureTask()
        {
            return this.QueryInfrastructureTaskAsync(FabricInfrastructureServiceAgent.DefaultTimeout, CancellationToken.None);
        }

        public Task<InfrastructureTaskQueryResult> QueryInfrastructureTaskAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.QueryInfrastructureTaskAsyncHelper(timeout, cancellationToken);
        }

        #endregion

        #region Register/Unregister Infrastructure Service

        private string RegisterInfrastructureServiceHelper(Guid partitionId, long replicaId, IInfrastructureService service)
        {
            InfrastructureServiceBroker broker = new InfrastructureServiceBroker(service);

            NativeCommon.IFabricStringResult nativeString = this.nativeAgent.RegisterInfrastructureService(
                partitionId,
                replicaId,
                broker);

            return StringResult.FromNative(nativeString);
        }

        private void UnregisterInfrastructureServiceHelper(Guid partitionId, long replicaId)
        {
            this.nativeAgent.UnregisterInfrastructureService(
                partitionId,
                replicaId);
        }

        #endregion

        #region Start Infrastructure Task

        private Task StartInfrastructureTaskAsyncHelper(InfrastructureTaskDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.StartInfrastructureTaskBeginWrapper(description, timeout, callback),
                this.StartInfrastructureTaskEndWrapper,
                cancellationToken,
                "InfrastructureServiceAgent.StartInfrastructureTaskAsync");
        }

        private NativeCommon.IFabricAsyncOperationContext StartInfrastructureTaskBeginWrapper(InfrastructureTaskDescription description, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            using (var pin = new PinCollection())
            {
                return this.nativeAgent.BeginStartInfrastructureTask(
                    description.ToNative(pin),
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }
        }

        private void StartInfrastructureTaskEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.nativeAgent.EndStartInfrastructureTask(context);
        }

        #endregion

        #region Finish Infrastructure Task
        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        private Task FinishInfrastructureTaskAsyncHelper(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.FinishInfrastructureTaskBeginWrapper(taskId, instanceId, timeout, callback),
                this.FinishInfrastructureTaskEndWrapper,
                cancellationToken,
                "InfrastructureServiceAgent.FinishInfrastructureTaskAsync");
        }

        private NativeCommon.IFabricAsyncOperationContext FinishInfrastructureTaskBeginWrapper(string taskId, long instanceId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            using (var pin = new PinCollection())
            {
                return this.nativeAgent.BeginFinishInfrastructureTask(
                    pin.AddBlittable(taskId),
                    instanceId,
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }
        }

        private void FinishInfrastructureTaskEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.nativeAgent.EndFinishInfrastructureTask(context);
        }

        #endregion

        #region Query Infrastructure Task

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        private Task<InfrastructureTaskQueryResult> QueryInfrastructureTaskAsyncHelper(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<InfrastructureTaskQueryResult>(
                (callback) => this.QueryInfrastructureTaskBeginWrapper(timeout, callback),
                this.QueryInfrastructureTaskEndWrapper,
                cancellationToken,
                "InfrastructureServiceAgent.QueryInfrastructureTaskAsync");
        }

        private NativeCommon.IFabricAsyncOperationContext QueryInfrastructureTaskBeginWrapper(TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return this.nativeAgent.BeginQueryInfrastructureTask(
                Utility.ToMilliseconds(timeout, "timeout"),
                callback);
        }

        private InfrastructureTaskQueryResult QueryInfrastructureTaskEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            return InfrastructureTaskQueryResult.CreateFromNative(this.nativeAgent.EndQueryInfrastructureTask(context));
        }

        #endregion
    }
}