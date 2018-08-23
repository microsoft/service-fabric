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

    internal sealed class StatefulServiceReplicaBroker 
        : NativeRuntime.IFabricStatefulServiceReplica
        , NativeRuntimeInternal.IFabricInternalStatefulServiceReplica
        , NativeRuntimeInternal.IFabricInternalBrokeredService
    {
        private static readonly InteropApi OpenAsyncApi = new InteropApi {CopyExceptionDetailsToThreadErrorMessage = true};
        private static readonly InteropApi CloseAsyncApi = new InteropApi {CopyExceptionDetailsToThreadErrorMessage = true};
        private static readonly InteropApi ChangeRoleAsyncApi = new InteropApi {CopyExceptionDetailsToThreadErrorMessage = true};
        private static readonly InteropApi AbortApi = new InteropApi {CopyExceptionDetailsToThreadErrorMessage = true};

        private readonly ServiceInitializationParameters initializationParameters;
        private readonly long replicaId;

        private readonly IStatefulServiceReplica statefulServiceReplica;
        private StatefulServicePartition statefulPartition;

        private readonly NativeRuntimeInternal.IFabricInternalStatefulServiceReplica internalReplica;

        public StatefulServiceReplicaBroker(IStatefulServiceReplica statefulServiceReplica, ServiceInitializationParameters initializationParameters, long replicaId)
        {
            Requires.Argument("statefulServiceReplica", statefulServiceReplica).NotNull();
            Requires.Argument("initializationParameters", initializationParameters).NotNull();
            
            this.statefulServiceReplica = statefulServiceReplica;
            this.initializationParameters = initializationParameters;
            this.replicaId = replicaId;

            // KVS is currently the only service that internally supports returning additional 
            // DeployedStatefulServiceReplicaDetail information. If we ever support custom
            // information from user services, then GetQueryResult() can be exposed via IStatefulServiceReplica.
            //
            var casted = this.statefulServiceReplica as KeyValueStoreReplica;
            if (casted != null)
            {
                this.internalReplica = casted.InternalStore;
            }
        }

        public IStatefulServiceReplica Service
        {
            get
            {
                return this.statefulServiceReplica;
            }
        }

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotCatchGeneralExceptionTypes, Justification = "Implementation of std async pattern.")]
        NativeCommon.IFabricAsyncOperationContext NativeRuntime.IFabricStatefulServiceReplica.BeginOpen(
            NativeTypes.FABRIC_REPLICA_OPEN_MODE openMode,
            NativeRuntime.IFabricStatefulServicePartition partition, 
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.OpenAsync(openMode, partition, cancellationToken), callback, "StatefulServiceReplicaBroker.Open", OpenAsyncApi);
        }

        NativeRuntime.IFabricReplicator NativeRuntime.IFabricStatefulServiceReplica.EndOpen(NativeCommon.IFabricAsyncOperationContext context)
        {
            IReplicator replicator = AsyncTaskCallInAdapter.End<IReplicator>(context);

            // If it is the fabric replicator then return the actual native interface implementation
            // This will prevent going through a broker to translate native and managed code
            FabricReplicator fabricReplicator = replicator as FabricReplicator;
            if (fabricReplicator != null)
            {
                return fabricReplicator.NativeReplicator;
            }

            // Return the broker that implements the correct native interfaces
            // If IReplicatorCatchupSpecificQuorum is supported return ReplicatorBroker2 (which derives from ReplicatorBroker and also implements IFabricReplicatorCatchupSpecificQuorum)
            // Else return ReplicatorBroker (which implements only IFabricReplicator and IFabricPrimaryReplicator)
            if (replicator is IReplicatorCatchupSpecificQuorum)
            {
                return new ReplicatorBroker2(replicator);
            }
            else
            {
                return new ReplicatorBroker(replicator);
            }
        }

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotCatchGeneralExceptionTypes, Justification = "Implementation of std async pattern.")]
        NativeCommon.IFabricAsyncOperationContext NativeRuntime.IFabricStatefulServiceReplica.BeginChangeRole(NativeTypes.FABRIC_REPLICA_ROLE newRole, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.ReplicaRoleChangeAsync((ReplicaRole)newRole, cancellationToken), callback, "StatefulServiceBroker.ReplicaChangeRole", ChangeRoleAsyncApi);
        }

        NativeCommon.IFabricStringResult NativeRuntime.IFabricStatefulServiceReplica.EndChangeRole(NativeCommon.IFabricAsyncOperationContext context)
        {
            string result = AsyncTaskCallInAdapter.End<string>(context);

            AppTrace.TraceSource.WriteNoise(
                "StatefulServiceReplicaBroker.ReplicaChangeRole",
                "ChangeRole result {0} for ServiceName {1}. Uri {2}. ReplicaId {3}. PartitionId {4}",
                result,
                this.initializationParameters.ServiceTypeName,
                this.initializationParameters.ServiceName,
                this.replicaId,
                this.initializationParameters.PartitionId);

            return new StringResult(result);            
        }

        NativeCommon.IFabricAsyncOperationContext NativeRuntime.IFabricStatefulServiceReplica.BeginClose(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.CloseAsync(cancellationToken), callback, "StatefulServiceReplicaBroker.CloseAsync", CloseAsyncApi);
        }

        void NativeRuntime.IFabricStatefulServiceReplica.EndClose(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        void NativeRuntime.IFabricStatefulServiceReplica.Abort()
        {
            Utility.WrapNativeSyncMethodImplementation(() => this.statefulServiceReplica.Abort(), "StatefulServiceReplicaBroker.Abort", AbortApi);
        }

        object NativeRuntimeInternal.IFabricInternalBrokeredService.GetBrokeredService()
        {
            return this.Service;
        }

        NativeRuntime.IFabricStatefulServiceReplicaStatusResult NativeRuntimeInternal.IFabricInternalStatefulServiceReplica.GetStatus()
        {
            if (this.internalReplica != null)
            {
                return this.internalReplica.GetStatus();
            }
            else
            {
                return null;
            }
        }

        private Task<IReplicator> OpenAsync(
            NativeTypes.FABRIC_REPLICA_OPEN_MODE openMode,
            NativeRuntime.IFabricStatefulServicePartition partition,
            CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise(
                "StatefulServiceReplicaBroker.OpenAsync",
                "OpenAsync for ServiceName {0}. Uri {1}. ReplicaId {2}. PartitionId {3}, OpenMode {4}",
                this.initializationParameters.ServiceTypeName,
                this.initializationParameters.ServiceName,
                this.replicaId,
                this.initializationParameters.PartitionId,
                openMode);

            this.statefulPartition = this.CreateStatefulPartition(partition);

            return this.statefulServiceReplica.OpenAsync((ReplicaOpenMode)openMode, this.statefulPartition, cancellationToken);
        }

        private Task<string> ReplicaRoleChangeAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise(
                "StatefulServiceReplicaBroker.ReplicaChangeRole",
                "ChangeRole for ServiceName {0}. Uri {1}. ReplicaId {2}. PartitionId {3}. NewRole: {4}",
                this.initializationParameters.ServiceTypeName,
                this.initializationParameters.ServiceName,
                this.replicaId,
                this.initializationParameters.PartitionId,
                newRole);

            return this.statefulServiceReplica.ChangeRoleAsync(newRole, cancellationToken);
        }

        private StatefulServicePartition CreateStatefulPartition(NativeRuntime.IFabricStatefulServicePartition nativeStatefulPartition)
        {
            //// Calls native code, requires UnmanagedCode permission
            Requires.Argument("nativeStatefulPartition", nativeStatefulPartition).NotNull();

            // Initialize partition information
            ServicePartitionInformation partitionInfo = ServicePartitionInformation.FromNative(nativeStatefulPartition.GetPartitionInfo());

            NativeRuntime.IFabricServiceGroupPartition nativeServiceGroupPartition = nativeStatefulPartition as NativeRuntime.IFabricServiceGroupPartition;

            if (nativeServiceGroupPartition != null)
            {
                return new AtomicGroupStatefulServicePartition(nativeStatefulPartition, nativeServiceGroupPartition, partitionInfo);
            }

            return new StatefulServicePartition(nativeStatefulPartition, partitionInfo);
        }

        private Task CloseAsync(CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise(
                "StatefulServiceReplicaBroker.CloseAsync",
                "CloseAsync for ServiceName {0}. Uri {1}. ReplicaId {2}. PartitionId {3}",
                this.initializationParameters.ServiceTypeName,
                this.initializationParameters.ServiceName,
                this.replicaId,
                this.statefulPartition.PartitionInfo.Id);

            return this.statefulServiceReplica.CloseAsync(cancellationToken);
        }
    }
}