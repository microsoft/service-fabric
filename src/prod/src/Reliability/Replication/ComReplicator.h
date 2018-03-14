// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ComReplicator : 
            public IFabricStateReplicator2, 
            public IFabricPrimaryReplicator, 
            public IFabricInternalStateReplicator, 
            public IFabricInternalReplicator, 
            public IOperationDataFactory,
            public IFabricInternalManagedReplicator,
            public IFabricReplicatorCatchupSpecificQuorum,
            private Common::ComUnknownBase
        {
            DENY_COPY(ComReplicator)

            BEGIN_COM_INTERFACE_LIST(ComReplicator)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricStateReplicator)
                COM_INTERFACE_ITEM(IID_IFabricStateReplicator, IFabricStateReplicator)
                COM_INTERFACE_ITEM(IID_IFabricStateReplicator2, IFabricStateReplicator2)
                COM_INTERFACE_ITEM(IID_IFabricReplicator, IFabricPrimaryReplicator)
                COM_INTERFACE_ITEM(IID_IFabricPrimaryReplicator, IFabricPrimaryReplicator)
                COM_INTERFACE_ITEM(IID_IFabricInternalStateReplicator, IFabricInternalStateReplicator)
                COM_INTERFACE_ITEM(IID_IFabricInternalReplicator, IFabricInternalReplicator)
                COM_INTERFACE_ITEM(IID_IOperationDataFactory, IOperationDataFactory)
                COM_INTERFACE_ITEM(IID_IFabricInternalManagedReplicator, IFabricInternalManagedReplicator)
                COM_INTERFACE_ITEM(IID_IFabricReplicatorCatchupSpecificQuorum, IFabricReplicatorCatchupSpecificQuorum)
            END_COM_INTERFACE_LIST()

        public:
            virtual ~ComReplicator() {}

            // TODO: make this private
            explicit ComReplicator(
                Replicator & replicator);

            static HRESULT CreateReplicator( 
                FABRIC_REPLICA_ID replicaId,
                Common::Guid const & partitionId,
                __in IFabricStatefulServicePartition * partition,
                __in IFabricStateProvider * stateProvider,
                BOOLEAN hasPersistedState,
                REInternalSettingsSPtr && config, 
                ReplicationTransportSPtr && transport,
                IReplicatorHealthClientSPtr && healthClient,
                /* [retval][out] */ IFabricStateReplicator **stateReplicator);

            static HRESULT CreateReplicatorV1Plus( 
                FABRIC_REPLICA_ID replicaId,
                Common::Guid const & partitionId,
                __in IFabricStatefulServicePartition * partition,
                __in IFabricStateProvider * stateProvider,
                BOOLEAN hasPersistedState,
                REInternalSettingsSPtr && config, 
                BOOLEAN batchEnabled,
                ReplicationTransportSPtr && transport,
                IReplicatorHealthClientSPtr && healthClient,
                /* [retval][out] */ IFabricStateReplicator **stateReplicator);

            // *****************************
            // IFabricStateReplicator methods
            // *****************************
            virtual HRESULT STDMETHODCALLTYPE BeginReplicate(
                /* [in] */ IFabricOperationData * operationData,
                /* [in] */ IFabricAsyncOperationCallback * callback,
                /* [out] */ FABRIC_SEQUENCE_NUMBER * sequenceNumber,
                /* [out, retval] */ IFabricAsyncOperationContext ** context);

            virtual HRESULT STDMETHODCALLTYPE EndReplicate(
                /* [in] */ IFabricAsyncOperationContext * context,
                /* [out] */ FABRIC_SEQUENCE_NUMBER * sequenceNumber);

            virtual HRESULT STDMETHODCALLTYPE GetReplicationStream(
                /* [out, retval] */ IFabricOperationStream **stream);

            virtual HRESULT STDMETHODCALLTYPE GetCopyStream(
                /* [out, retval] */ IFabricOperationStream **stream);

            virtual HRESULT STDMETHODCALLTYPE UpdateReplicatorSettings(
                /* [in] */ FABRIC_REPLICATOR_SETTINGS const * replicatorSettings);

            // *****************************
            // IFabricStateReplicator2 methods
            // *****************************
            virtual HRESULT STDMETHODCALLTYPE GetReplicatorSettings(
                /* [out, retval] */ IFabricReplicatorSettingsResult ** replicatorSettings);

            // *****************************
            // IFabricInternalManagedReplicator methods
            // *****************************
            virtual HRESULT STDMETHODCALLTYPE BeginReplicate2(
                /* [in] */ ULONG bufferCount,
                /* [in] */ FABRIC_OPERATION_DATA_BUFFER_EX1 const * buffers,
                /* [in] */ IFabricAsyncOperationCallback * callback,
                /* [out] */ FABRIC_SEQUENCE_NUMBER * sequenceNumber,
                /* [out, retval] */ IFabricAsyncOperationContext ** context);

            // *****************************
            // IFabricReplicator methods
            // *****************************
            virtual HRESULT STDMETHODCALLTYPE BeginOpen( 
                /* [in] */ IFabricAsyncOperationCallback *callback,
                /* [retval][out] */ IFabricAsyncOperationContext **context);
        
            virtual HRESULT STDMETHODCALLTYPE EndOpen( 
                /* [in] */ IFabricAsyncOperationContext *context,
                /* [retval][out] */ IFabricStringResult **replicationEndpoint);

            virtual HRESULT STDMETHODCALLTYPE BeginChangeRole( 
                /* [in] */ FABRIC_EPOCH const * epoch,
                /* [in] */ FABRIC_REPLICA_ROLE role,
                /* [in] */ IFabricAsyncOperationCallback *callback,
                /* [retval][out] */ IFabricAsyncOperationContext **context);
        
            virtual HRESULT STDMETHODCALLTYPE EndChangeRole( 
                /* [in] */ IFabricAsyncOperationContext *context);

            virtual HRESULT STDMETHODCALLTYPE BeginUpdateEpoch( 
                /* [in] */ FABRIC_EPOCH const * epoch,
                /* [in] */ IFabricAsyncOperationCallback *callback,
                /* [retval][out] */ IFabricAsyncOperationContext **context);

            virtual HRESULT STDMETHODCALLTYPE EndUpdateEpoch( 
                /* [in] */ IFabricAsyncOperationContext *context);

            virtual HRESULT STDMETHODCALLTYPE BeginClose( 
                /* [in] */ IFabricAsyncOperationCallback *callback,
                /* [retval][out] */ IFabricAsyncOperationContext **context);
        
            virtual HRESULT STDMETHODCALLTYPE EndClose( 
                /* [in] */ IFabricAsyncOperationContext *context);

            virtual void STDMETHODCALLTYPE Abort();

            virtual HRESULT STDMETHODCALLTYPE GetCurrentProgress( 
                /* [out] */ FABRIC_SEQUENCE_NUMBER *lastSequenceNumber);

            virtual HRESULT STDMETHODCALLTYPE GetCatchUpCapability( 
                /* [out] */ FABRIC_SEQUENCE_NUMBER *firstSequenceNumber);

            // *****************************
            // IFabricPrimaryReplicator methods
            // *****************************
            virtual HRESULT STDMETHODCALLTYPE BeginOnDataLoss(
                /* [in] */ IFabricAsyncOperationCallback * callback,
                /* [out, retval] */ IFabricAsyncOperationContext ** context);
    
            virtual HRESULT STDMETHODCALLTYPE EndOnDataLoss(
                /* [in] */ IFabricAsyncOperationContext * context,
                /* [out, retval] */ BOOLEAN * isStateChanged);

            virtual HRESULT STDMETHODCALLTYPE UpdateCatchUpReplicaSetConfiguration( 
                /* [in] */ FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration,
                /* [in] */ FABRIC_REPLICA_SET_CONFIGURATION const * previousConfiguration);

            virtual HRESULT STDMETHODCALLTYPE BeginWaitForCatchUpQuorum(
                /* [in] */ FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
                /* [in] */ IFabricAsyncOperationCallback *callback,
                /* [retval][out] */ IFabricAsyncOperationContext **context);
        
            virtual HRESULT STDMETHODCALLTYPE EndWaitForCatchUpQuorum( 
                /* [in] */ IFabricAsyncOperationContext *context);

            virtual HRESULT STDMETHODCALLTYPE UpdateCurrentReplicaSetConfiguration( 
                /* [in] */ FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration);

            virtual HRESULT STDMETHODCALLTYPE BeginBuildReplica( 
                /* [in] */ FABRIC_REPLICA_INFORMATION const *replica,
                /* [in] */ IFabricAsyncOperationCallback *callback,
                /* [retval][out] */ IFabricAsyncOperationContext **context);
        
            virtual HRESULT STDMETHODCALLTYPE EndBuildReplica( 
                /* [in] */ IFabricAsyncOperationContext *context);
        
            virtual HRESULT STDMETHODCALLTYPE RemoveReplica( 
                /* [in] */ FABRIC_REPLICA_ID replicaId);

            // *****************************
            // IFabricInternalStateReplicator methods
            // *****************************
            virtual HRESULT STDMETHODCALLTYPE BeginReplicateBatch(
                /* [in] */ IFabricBatchOperationData * operationData,
                /* [in] */ IFabricAsyncOperationCallback * callback,
                /* [out, retval] */ IFabricAsyncOperationContext ** context);

            virtual HRESULT STDMETHODCALLTYPE EndReplicateBatch(
                /* [in] */ IFabricAsyncOperationContext * context);

            virtual HRESULT STDMETHODCALLTYPE ReserveSequenceNumber(
                /* [in] */ BOOLEAN alwaysReserveWhenPrimary,
                /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * sequenceNumber);

            virtual HRESULT STDMETHODCALLTYPE GetBatchReplicationStream(
                /* [out, retval] */ IFabricBatchOperationStream **batchStream);

            virtual HRESULT STDMETHODCALLTYPE GetReplicationQueueCounters(
                /* [out] */ FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS * counters);

            // *****************************
            // IFabricInternalReplicator methods
            // *****************************
            virtual HRESULT STDMETHODCALLTYPE GetReplicatorStatus(
                /* [out, retval] */ IFabricGetReplicatorStatusResult **replicatorStatus);

            // *****************************
            // IOperationDataFactory members
            // *****************************
            virtual HRESULT STDMETHODCALLTYPE CreateOperationData(
                __in ULONG * segments,
                __in ULONG segmentCount,
                __out ::IFabricOperationData ** operationData);

            // *****************************
            // Inner types that implement async operations
            // *****************************
            class BaseOperation;

            // IFabricStateReplicator
            class ReplicateOperation;
            class ReplicateBatchOperation;
            class GetOperationOperation;
            
            // IFabricReplicator
            class OpenOperation;
            class CloseOperation;
            class ChangeRoleOperation;
            class UpdateEpochOperation;

            // IFabricPrimaryReplicator
            class BuildIdleReplicaOperation;
            class CatchupReplicaSetOperation;
            class OnDataLossOperation;

            __declspec(property(get=get_Engine)) Replicator & ReplicatorEngine;
            Replicator & get_Engine() const { return replicator_; }

        private:

            Replicator & replicator_;
            Common::ComponentRootSPtr root_;
        };

    } // end namespace ReplicationComponent
} // end namespace Reliability
