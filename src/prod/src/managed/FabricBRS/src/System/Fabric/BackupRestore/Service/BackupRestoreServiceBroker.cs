// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Service
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.BackupRestore.Service.Interop;
    using System.Fabric.BackupRestore.DataStructures;
    using System.Fabric.BackupRestore.Interop;

    [SuppressMessage(FxCop.Category.Design, FxCop.Rule.TypesThatOwnDisposableFieldsShouldBeDisposable, Justification = "// TODO: Implement an IDisposable, or ensure cleanup.")]
    internal sealed class BackupRestoreServiceBroker : NativeBackupRestoreService.IFabricBackupRestoreService
    {
        private const string TraceType = "BackupRestoreServiceBroker";

        private static readonly InteropApi ThreadErrorMessageSetter = new InteropApi
        {
            CopyExceptionDetailsToThreadErrorMessage = true,
            ShouldIncludeStackTraceInThreadErrorMessage = false
        };

        private readonly IBackupRestoreService service;

        internal BackupRestoreServiceBroker(IBackupRestoreService service)
        {
            this.service = service;
        }

        internal IBackupRestoreService Service
        {
            get
            {
                return this.service;
            }
        }

        NativeCommon.IFabricAsyncOperationContext NativeBackupRestoreService.IFabricBackupRestoreService.BeginGetBackupSchedulePolicy(IntPtr partitionInfo, uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedBackupPartitionInfo = BackupPartitionInfo.FromNative(partitionInfo);
            var managedTimout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                cancellationToken => 
                    this.Service.GetBackupSchedulingPolicyAsync(
                        managedBackupPartitionInfo,
                        managedTimout,
                        cancellationToken),
                    callback,
                    "BackupRestoreServiceBroker.GetBackupSchedulingPolicyAsync",
                    ThreadErrorMessageSetter);
        }

        NativeBackupRestoreRuntime.IFabricGetBackupSchedulePolicyResult NativeBackupRestoreService.IFabricBackupRestoreService.EndGetBackupSchedulePolicy(NativeCommon.IFabricAsyncOperationContext context)
        {
            var backupSchedulingPolicy = AsyncTaskCallInAdapter.End<BackupPolicy>(context);
            return new GetBackupSchedulePolicyResult(backupSchedulingPolicy);
        }

        NativeCommon.IFabricAsyncOperationContext NativeBackupRestoreService.IFabricBackupRestoreService.BeginGetRestorePointDetails(IntPtr partitionInfo, uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedBackupPartitionInfo = BackupPartitionInfo.FromNative(partitionInfo);
            var managedTimout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                cancellationToken =>
                    this.Service.GetRestorePointDetailsAsync(
                        managedBackupPartitionInfo,
                        managedTimout,
                        cancellationToken),
                    callback,
                    "BackupRestoreServiceBroker.GetRestorePointDetailsAsync",
                    ThreadErrorMessageSetter);
        }

        NativeBackupRestoreRuntime.IFabricGetRestorePointDetailsResult NativeBackupRestoreService.IFabricBackupRestoreService.EndGetRestorePointDetails(NativeCommon.IFabricAsyncOperationContext context)
        {
            var restorePoint = AsyncTaskCallInAdapter.End<RestorePointDetails>(context);
            return new GetRestorePointDetailsResult(restorePoint);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginReportBackupOperationResult(IntPtr operationResult, uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedOperationResult = BackupOperationResult.FromNative(operationResult);
            var managedTimout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                cancellationToken =>
                    this.Service.BackupOperationResultAsync(
                        managedOperationResult,
                        managedTimout,
                        cancellationToken),
                    callback,
                    "BackupRestoreServiceBroker.BackupOperationResultAsync",
                    ThreadErrorMessageSetter);
        }

        public void EndReportBackupOperationResult(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        NativeCommon.IFabricAsyncOperationContext NativeBackupRestoreService.IFabricBackupRestoreService.BeginReportRestoreOperationResult(IntPtr operationResult, uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedOperationResult = RestoreOperationResult.FromNative(operationResult);
            var managedTimout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                cancellationToken =>
                    this.Service.RestoreOperationResultAsync(
                        managedOperationResult,
                        managedTimout,
                        cancellationToken),
                    callback,
                    "BackupRestoreServiceBroker.RestoreOperationResultAsync",
                    ThreadErrorMessageSetter);
        }

        void NativeBackupRestoreService.IFabricBackupRestoreService.EndReportRestoreOperationResult(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }
    }
}