// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.Common;
using System.Threading;
using System.Threading.Tasks;
using System.Diagnostics.CodeAnalysis;
using System.Fabric.Interop;
using System.Fabric.BackupRestore.DataStructures;
using System.Fabric.BackupRestore.Service.Interop;

namespace System.Fabric.BackupRestore.Service
{
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Preserve order of public members from V1.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1204:StaticElementsMustAppearBeforeInstanceElements", Justification = "Current grouping improves readability.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1202:ElementsMustBeOrderedByAccess", Justification = "Current grouping improves readability.")]
    internal sealed class FabricBackupRestoreServiceAgent
    {
        #region Fields & Properties

        internal static readonly TimeSpan DefaultTimeout = TimeSpan.FromMinutes(1);

        /// <summary>
        /// This should only be constructed once per process, so making it static.  Without this, there is a situation where it could be constructed more than once.  For example, if 
        /// the primary moves somewhere else then comes back to its former position without a host deactivation. 
        /// </summary>
        private static readonly Lazy<FabricBackupRestoreServiceAgent> Agent = new Lazy<FabricBackupRestoreServiceAgent>(() => new FabricBackupRestoreServiceAgent());

        private NativeBackupRestoreService.IFabricBackupRestoreServiceAgent _nativeAgent;

        /// <summary>
        /// Trace type string used by traces of this service.
        /// </summary>
        private const string TraceType = "FabricBackupRestoreServiceAgent";

        #endregion

        #region Constructors & Initialization

        public static FabricBackupRestoreServiceAgent Instance
        {
            get { return Agent.Value; }
        }

        private FabricBackupRestoreServiceAgent()
        {
            Utility.WrapNativeSyncInvokeInMTA(this.CreateNativeAgent, "BackupRestoreServiceAgent.CreateNativeAgent");
        }

        private void CreateNativeAgent()
        {
            Guid iid = typeof(NativeBackupRestoreService.IFabricBackupRestoreServiceAgent).GUID;
            // PInvoke call
            this._nativeAgent = NativeBackupRestoreService.CreateFabricBackupRestoreServiceAgent(ref iid);
        }

        #endregion

        #region API

        public string RegisterBackupRestoreService(Guid partitionId, long replicaId, IBackupRestoreService service)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNullOrEmpty();
            Requires.Argument<long>("replicaId", replicaId).NotNullOrEmpty();
            Requires.Argument<IBackupRestoreService>("service", service).NotNull();

            return Utility.WrapNativeSyncInvokeInMTA(() => this.RegisterBackupRestoreServiceHelper(partitionId, replicaId, service), "BackupRestoreServiceAgent.RegisterBackupRestoreService");
        }

        public void UnregisterBackupRestoreService(Guid partitionId, long replicaId)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNullOrEmpty();
            Requires.Argument<long>("replicaId", replicaId).NotNullOrEmpty();

            Utility.WrapNativeSyncInvokeInMTA(() => this.UnregisterBackupRestoreServiceHelper(partitionId, replicaId), "BackupRestoreServiceAgent.UnregisterBackupRestoreService");
        }

        public Task EnableProtectionAsync(Uri serviceName, Guid partitionId, 
            DataStructures.BackupPolicy policy, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Requires.Argument<Uri>("serviceName", serviceName).NotNull();
            Requires.Argument<Guid>("partitionId", partitionId).NotNullOrEmpty();
            Requires.Argument<DataStructures.BackupPolicy>("policy", policy).NotNull();

            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Enabling protection for partition {0} with policy id {1}", partitionId, policy.Name);

            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.UpdateBackupSchedulePolicyBeginWrapper(serviceName, partitionId, policy, timeout, callback),
                this.UpdateBackupSchedulePolicyEndWrapper,
                cancellationToken,
                "FabricBackupRestoreServiceAgent.EnableProtectionAsync");
        }

        public Task DisableProtectionAsync(Uri serviceName, Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Requires.Argument<Uri>("serviceName", serviceName).NotNull();
            Requires.Argument<Guid>("partitionId", partitionId).NotNullOrEmpty();

            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Disabling protection for partition {0}", partitionId);

            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.UpdateBackupSchedulePolicyBeginWrapper(serviceName, partitionId, null, timeout, callback),
                this.UpdateBackupSchedulePolicyEndWrapper,
                cancellationToken,
                "FabricBackupRestoreServiceAgent.DisableProtectionAsync");
        }

        public Task BackupPartitionAsync(Uri serviceName, Guid partitionId, Guid operationId, BackupNowConfiguration configuration, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Requires.Argument<Uri>("serviceName", serviceName).NotNull();
            Requires.Argument<Guid>("partitionId", partitionId).NotNullOrEmpty();
            Requires.Argument<Guid>("operationId", operationId).NotNullOrEmpty();
            Requires.Argument<BackupNowConfiguration>("configuration", configuration).NotNull();

            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Backup Now partition {0}", partitionId);

            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.PartitionBackupOperationBeginWrapper(serviceName, partitionId, operationId, configuration, timeout, callback),
                this.PartitionBackupOperationEndWrapper,
                cancellationToken,
                "FabricBackupRestoreServiceAgent.BackupPartitionAsync");
        }

        #endregion

        #region Register/Unregister BackupRestore Service

        private string RegisterBackupRestoreServiceHelper(Guid partitionId, long replicaId, IBackupRestoreService service)
        {
            BackupRestoreServiceBroker broker = new BackupRestoreServiceBroker(service);

            NativeCommon.IFabricStringResult nativeString = this._nativeAgent.RegisterBackupRestoreService(
                partitionId,
                replicaId,
                broker);

            return StringResult.FromNative(nativeString);
        }

        private void UnregisterBackupRestoreServiceHelper(Guid partitionId, long replicaId)
        {
            this._nativeAgent.UnregisterBackupRestoreService(
                partitionId,
                replicaId);
        }

        private NativeCommon.IFabricAsyncOperationContext UpdateBackupSchedulePolicyBeginWrapper(
            Uri serviceName,
            Guid partitionId,
            BackupPolicy policy,
            TimeSpan timeout,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var timeoutMilliseconds = Utility.ToMilliseconds(timeout, "timeout");
            var partitionInfo = new BackupPartitionInfo
            {
                PartitionId = partitionId,
                ServiceName = serviceName.ToString(),
            };

            using (var pin = new PinCollection())
            {
                return this._nativeAgent.BeginUpdateBackupSchedulePolicy(
                    partitionInfo.ToNative(pin),
                    policy == null ? IntPtr.Zero : policy.ToNative(pin),
                    timeoutMilliseconds, 
                    callback);
            }
        }

        private void UpdateBackupSchedulePolicyEndWrapper(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            this._nativeAgent.EndUpdateBackupSchedulePolicy(context);
        }

        private NativeCommon.IFabricAsyncOperationContext PartitionBackupOperationBeginWrapper(
            Uri serviceName,
            Guid partitionId,
            Guid operationId,
            BackupNowConfiguration configuration,
            TimeSpan timeout,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var timeoutMilliseconds = Utility.ToMilliseconds(timeout, "timeout");
            var partitionInfo = new BackupPartitionInfo
            {
                PartitionId = partitionId,
                ServiceName = serviceName.ToString(),
            };

            using (var pin = new PinCollection())
            {
                return this._nativeAgent.BeginPartitionBackupOperation(
                    partitionInfo.ToNative(pin),
                    operationId,
                    configuration.ToNative(pin),
                    timeoutMilliseconds,
                    callback);
            }
        }

        private void PartitionBackupOperationEndWrapper(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            this._nativeAgent.EndPartitionBackupOperation(context);
        }

        #endregion
    }
}