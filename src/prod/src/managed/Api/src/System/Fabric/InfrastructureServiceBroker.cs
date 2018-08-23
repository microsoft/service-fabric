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
    internal sealed class InfrastructureServiceBroker : NativeInfrastructureService.IFabricInfrastructureService
    {
        private readonly IInfrastructureService service;

        internal InfrastructureServiceBroker(IInfrastructureService service)
        {
            this.service = service;
        }

        internal IInfrastructureService Service
        {
            get
            {
                return this.service;
            }
        }

        NativeCommon.IFabricAsyncOperationContext NativeInfrastructureService.IFabricInfrastructureService.BeginRunCommand(SByte isAdminCommand, IntPtr command, uint timeoutInMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            string managedCommand = NativeTypes.FromNativeString(command);
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.RunCommandAsync(isAdminCommand != 0, managedCommand, managedTimeout, cancellationToken), callback, "InfrastructureServiceBroker.RunCommandAsync");
        }

        NativeCommon.IFabricStringResult NativeInfrastructureService.IFabricInfrastructureService.EndRunCommand(NativeCommon.IFabricAsyncOperationContext context)
        {
            string result = AsyncTaskCallInAdapter.End<string>(context);

            return new StringResult(result);
        }

        NativeCommon.IFabricAsyncOperationContext NativeInfrastructureService.IFabricInfrastructureService.BeginReportStartTaskSuccess(IntPtr taskId, long instanceId, uint timeoutInMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            string managedTaskId = NativeTypes.FromNativeString(taskId);
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.ReportStartTaskSuccessAsync(managedTaskId, instanceId, managedTimeout, cancellationToken), callback, "InfrastructureServiceBroker.ReportStartTaskSuccessAsync");
        }

        void NativeInfrastructureService.IFabricInfrastructureService.EndReportStartTaskSuccess(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }
        
        NativeCommon.IFabricAsyncOperationContext NativeInfrastructureService.IFabricInfrastructureService.BeginReportFinishTaskSuccess(IntPtr taskId, long instanceId, uint timeoutInMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            string managedTaskId = NativeTypes.FromNativeString(taskId);
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.ReportFinishTaskSuccessAsync(managedTaskId, instanceId, managedTimeout, cancellationToken), callback, "InfrastructureServiceBroker.ReportFinishTaskSuccessAsync");
        }

        void NativeInfrastructureService.IFabricInfrastructureService.EndReportFinishTaskSuccess(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        NativeCommon.IFabricAsyncOperationContext NativeInfrastructureService.IFabricInfrastructureService.BeginReportTaskFailure(IntPtr taskId, long instanceId, uint timeoutInMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            string managedTaskId = NativeTypes.FromNativeString(taskId);
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.ReportTaskFailureAsync(managedTaskId, instanceId, managedTimeout, cancellationToken), callback, "InfrastructureServiceBroker.ReportTaskFailureAsync");
        }

        void NativeInfrastructureService.IFabricInfrastructureService.EndReportTaskFailure(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        private Task<string> RunCommandAsync(bool isAdminCommand, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.RunCommandAsync(isAdminCommand, command, timeout, cancellationToken);
        }

        private Task ReportStartTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.ReportStartTaskSuccessAsync(taskId, instanceId, timeout, cancellationToken);
        }

        private Task ReportFinishTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.ReportFinishTaskSuccessAsync(taskId, instanceId, timeout, cancellationToken);
        }

        private Task ReportTaskFailureAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.ReportTaskFailureAsync(taskId, instanceId, timeout, cancellationToken);
        }
    }
}