// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Diagnostics.CodeAnalysis;
using System.Fabric.BackupRestore.DataStructures;
using System.Fabric.BackupRestore.Interop;
using System.Fabric.Common;
using System.Fabric.Interop;

namespace System.Fabric.BackupRestore
{
    [SuppressMessage(FxCop.Category.Design, FxCop.Rule.TypesThatOwnDisposableFieldsShouldBeDisposable, Justification = "// TODO: Implement an IDisposable, or ensure cleanup.")]
    internal sealed class BackupRestoreBroker : NativeBackupRestoreRuntime.IFabricBackupRestoreHandler
    {
        private const string TraceType = "BackupRestoreBroker";

        private static readonly InteropApi ThreadErrorMessageSetter = new InteropApi
        {
            CopyExceptionDetailsToThreadErrorMessage = true,
            ShouldIncludeStackTraceInThreadErrorMessage = false
        };

        private readonly IBackupRestoreHandler handler;

        internal BackupRestoreBroker(IBackupRestoreHandler handler)
        {
            this.handler = handler;
        }

        internal IBackupRestoreHandler Handler
        {
            get
            {
                return this.handler;
            }
        }

        NativeCommon.IFabricAsyncOperationContext NativeBackupRestoreRuntime.IFabricBackupRestoreHandler.BeginUpdateBackupSchedulePolicy(IntPtr backupPolicy, uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedTimout = TimeSpan.FromMilliseconds(timeoutMilliseconds);
            BackupPolicy managedPolicy = null;
            if (backupPolicy != IntPtr.Zero)
            {
                managedPolicy = BackupPolicy.FromNative(backupPolicy);
            }
            
            return Utility.WrapNativeAsyncMethodImplementation(
            cancellationToken =>
                this.Handler.UpdateBackupSchedulingPolicyAsync(
                    managedPolicy,
                    managedTimout,
                    cancellationToken),
                callback,
                "BackupRestoreBroker.BeginUpdateBackupSchedulePolicy",
                ThreadErrorMessageSetter);
        }

        void NativeBackupRestoreRuntime.IFabricBackupRestoreHandler.EndUpdateBackupSchedulePolicy(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        NativeCommon.IFabricAsyncOperationContext NativeBackupRestoreRuntime.IFabricBackupRestoreHandler.BeginPartitionBackupOperation(Guid operationId, IntPtr backupConfiguration,
            uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedTimout = TimeSpan.FromMilliseconds(timeoutMilliseconds);
            BackupNowConfiguration managedConfiguration = null;
            if (backupConfiguration != IntPtr.Zero)
            {
                managedConfiguration = BackupNowConfiguration.FromNative(backupConfiguration);
            }

            return Utility.WrapNativeAsyncMethodImplementation(
            cancellationToken =>
                this.Handler.BackupPartitionAsync(
                    operationId,
                    managedConfiguration,
                    managedTimout,
                    cancellationToken),
                callback,
                "BackupRestoreBroker.BeginPartitionBackupOperation",
                ThreadErrorMessageSetter);
        }

        void NativeBackupRestoreRuntime.IFabricBackupRestoreHandler.EndPartitionBackupOperation(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }
    }
}