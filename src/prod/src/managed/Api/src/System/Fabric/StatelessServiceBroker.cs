// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    [SuppressMessage(FxCop.Category.Design, FxCop.Rule.TypesThatOwnDisposableFieldsShouldBeDisposable, Justification = "// TODO: Implement an IDisposable, or ensure cleanup.")]
    internal sealed class StatelessServiceBroker : NativeRuntime.IFabricStatelessServiceInstance, NativeRuntimeInternal.IFabricInternalBrokeredService
    {
        private static readonly InteropApi OpenAsyncApi = new InteropApi { CopyExceptionDetailsToThreadErrorMessage = true };
        private static readonly InteropApi CloseAsyncApi = new InteropApi { CopyExceptionDetailsToThreadErrorMessage = true };
        private static readonly InteropApi AbortApi = new InteropApi { CopyExceptionDetailsToThreadErrorMessage = true };

        private readonly ServiceInitializationParameters initializationParameters;
        private readonly long instanceId;

        private readonly IStatelessServiceInstance statelessService;
        private StatelessServicePartition statelessServicePartition;

        internal StatelessServiceBroker(IStatelessServiceInstance statelessServiceInstance, ServiceInitializationParameters initializationParameters, long instanceId)
        {
            Requires.Argument("statelessServiceInstance", statelessServiceInstance).NotNull();
            Requires.Argument("initializationParameters", initializationParameters).NotNull();

            this.statelessService = statelessServiceInstance;
            this.initializationParameters = initializationParameters;
            this.instanceId = instanceId;
        }

        internal IStatelessServiceInstance Service
        {
            get
            {
                return this.statelessService;
            }
        }

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotCatchGeneralExceptionTypes, Justification = "Implementation of std async pattern.")]
        NativeCommon.IFabricAsyncOperationContext NativeRuntime.IFabricStatelessServiceInstance.BeginOpen(NativeRuntime.IFabricStatelessServicePartition partition, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.OpenAsync(partition, cancellationToken), callback, "StatelessServiceBroker.OpenAsync", OpenAsyncApi);    
        }

        NativeCommon.IFabricStringResult NativeRuntime.IFabricStatelessServiceInstance.EndOpen(NativeCommon.IFabricAsyncOperationContext context)
        {
            string result = AsyncTaskCallInAdapter.End<string>(context);

            AppTrace.TraceSource.WriteNoise(
                "StatelessServiceBroker.OpenAsync",
                "OpenAsync result {0} for ServiceName {1}. Uri {2}. InstanceId {3}. PartitionId {4}",
                result,
                this.initializationParameters.ServiceTypeName,
                this.initializationParameters.ServiceName,
                this.instanceId,
                this.initializationParameters.PartitionId);

            return new StringResult(result);
        }

        NativeCommon.IFabricAsyncOperationContext NativeRuntime.IFabricStatelessServiceInstance.BeginClose(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.CloseAsync(cancellationToken), callback, "StatelessServiceBroker.CloseAsync", CloseAsyncApi);
        }

        void NativeRuntime.IFabricStatelessServiceInstance.EndClose(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        void NativeRuntime.IFabricStatelessServiceInstance.Abort()
        {
            Utility.WrapNativeSyncMethodImplementation(() => this.statelessService.Abort(), "StatelessServiceBroker.Abort", AbortApi);
        }

        object NativeRuntimeInternal.IFabricInternalBrokeredService.GetBrokeredService()
        {
            return this.Service;
        }

        private Task<string> OpenAsync(NativeRuntime.IFabricStatelessServicePartition partition, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise(
                "StatelessServiceBroker.OpenAsync",
                "OpenAsync for ServiceName {0}. Uri {1}. InstanceId {2}. PartitionId {3}",
                this.initializationParameters.ServiceTypeName,
                this.initializationParameters.ServiceName,
                this.instanceId,
                this.initializationParameters.PartitionId);

            this.statelessServicePartition = this.CreatePartition(partition);

            return this.statelessService.OpenAsync(this.statelessServicePartition, cancellationToken);
        }

        private StatelessServicePartition CreatePartition(NativeRuntime.IFabricStatelessServicePartition nativePartition)
        {
            NativeRuntime.IFabricServiceGroupPartition nativeServiceGroupPartition = nativePartition as NativeRuntime.IFabricServiceGroupPartition;
            if (nativeServiceGroupPartition != null)
            {
                return new ServiceGroupStatelessPartition(this, nativePartition, nativeServiceGroupPartition);
            }
            else
            {
                return new StatelessServicePartition(this, nativePartition);
            }
        }

        private Task CloseAsync(CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise(
                "StatelessServiceBroker.CloseAsync",
                "CloseAsync for ServiceName {0}. Uri {1}. InstanceId {2}. PartitionId {3}",
                this.initializationParameters.ServiceTypeName,
                this.initializationParameters.ServiceName,
                this.instanceId,
                this.initializationParameters.PartitionId);

            return this.statelessService.CloseAsync(cancellationToken);
        }
    }
}