// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"
#include "FabricRuntime_.h"

#include "ServiceGroup.AsyncOperations.h"
#include "ServiceGroup.OperationDataFactory.h"
#include "ServiceGroup.OperationStream.h"
#include "ServiceGroup.ServiceBaseUnit.h"

namespace ServiceGroup
{
    //
    // Common typedefs.
    //
    typedef std::list<IFabricOperation*>::iterator Operations_Iterator;
    typedef std::list<CGetOperationAsyncOperationContext*>::iterator Waiters_Iterator;
        
    //
    // Stateful service group unit hosting a user stateful service replica.
    //
    class CStatefulServiceUnit : 
        public IFabricStatefulServicePartition3,
        public IFabricStateProvider,
        public IFabricAtomicGroupStateProvider,
        public IFabricStateReplicator2,
        public IFabricInternalStateReplicator,
        public IFabricAtomicGroupStateReplicator,
        public IFabricStatefulServiceReplica, 
        public IOperationDataFactory,
        public IFabricServiceGroupPartition,
        public IFabricInternalManagedReplicator,
        public CBaseServiceUnit
    {
    
        DENY_COPY(CStatefulServiceUnit)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulServiceUnit(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_REPLICA_ID replicaId);
        virtual ~CStatefulServiceUnit(void);
    
        BEGIN_COM_INTERFACE_LIST(CStatefulServiceUnit)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServicePartition)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition, IFabricStatefulServicePartition)
            COM_INTERFACE_ITEM(IID_IFabricStateReplicator, IFabricStateReplicator)
            COM_INTERFACE_ITEM(IID_IFabricStateReplicator2, IFabricStateReplicator2)
            COM_INTERFACE_ITEM(IID_IFabricInternalStateReplicator, IFabricInternalStateReplicator)
            COM_INTERFACE_ITEM(IID_IFabricAtomicGroupStateReplicator, IFabricAtomicGroupStateReplicator)
            COM_INTERFACE_ITEM(IID_IOperationDataFactory, IOperationDataFactory)
            COM_INTERFACE_ITEM(IID_IFabricServiceGroupPartition, IFabricServiceGroupPartition)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition1, IFabricStatefulServicePartition1)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition2, IFabricStatefulServicePartition2)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition3, IFabricStatefulServicePartition3)
            COM_INTERFACE_ITEM(IID_IFabricInternalManagedReplicator, IFabricInternalManagedReplicator)
        END_COM_INTERFACE_LIST()
        //
        //  IFabricStatefulServiceReplica methods.
        //
        STDMETHOD(BeginOpen)(
            __in FABRIC_REPLICA_OPEN_MODE openMode,
            __in IFabricStatefulServicePartition* partition,
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndOpen)(
            __in IFabricAsyncOperationContext* context,
            __out IFabricReplicator** replicator
            );
        STDMETHOD(BeginChangeRole)(
            __in FABRIC_REPLICA_ROLE newRole,
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndChangeRole)(
            __in IFabricAsyncOperationContext* context,
            __out IFabricStringResult** serviceEndpoint
            );
        STDMETHOD(BeginClose)(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndClose)(__in IFabricAsyncOperationContext * context);
        STDMETHOD_(void, Abort)();
        //
        // IFabricStatefulServicePartition methods.
        //
        STDMETHOD(GetPartitionInfo)(__out const FABRIC_SERVICE_PARTITION_INFORMATION** bufferedValue);
        STDMETHOD(GetReadStatus)(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* readStatus);
        STDMETHOD(GetWriteStatus)(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* writeStatus);
        STDMETHOD(CreateReplicator)(
            __in IFabricStateProvider* stateProvider, 
            __in_opt FABRIC_REPLICATOR_SETTINGS const* replicatorSettings,
            __out IFabricReplicator ** replicator, 
            __out IFabricStateReplicator** stateReplicator
            );
        STDMETHOD(ReportLoad)(__in ULONG metricCount, __in const FABRIC_LOAD_METRIC* metrics);
        STDMETHOD(ReportFault)(FABRIC_FAULT_TYPE faultType);
        //
        // IFabricStatefulServicePartition1 methods.
        //
        STDMETHOD(ReportMoveCost)(FABRIC_MOVE_COST moveCost);
        //
        // IFabricStatefulServicePartition2 methods.
        //
        STDMETHOD(ReportReplicaHealth)(__in const FABRIC_HEALTH_INFORMATION* healthInformation);
        STDMETHOD(ReportPartitionHealth)(__in const FABRIC_HEALTH_INFORMATION* healthInformation);
        //
        // IFabricStatefulServicePartition3 methods.
        //
        STDMETHOD(ReportReplicaHealth2)(__in const FABRIC_HEALTH_INFORMATION* healthInformation, __in const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        STDMETHOD(ReportPartitionHealth2)(__in const FABRIC_HEALTH_INFORMATION* healthInformation, __in const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        //
        // IFabricServiceGroupPartition methods.
        //
        STDMETHOD(ResolveMember)(
            __in FABRIC_URI name,
            __in REFIID riid,
            __out void ** member);
        //
        // IFabricStateProvider methods.
        //
        STDMETHOD(BeginUpdateEpoch)(
            __in FABRIC_EPOCH const* epoch, 
            __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndUpdateEpoch)(
            __in IFabricAsyncOperationContext* context
            );
        STDMETHOD(GetLastCommittedSequenceNumber)(__out FABRIC_SEQUENCE_NUMBER* sequenceNumber);
        STDMETHOD(BeginOnDataLoss)(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndOnDataLoss)(
            __in IFabricAsyncOperationContext* context,
            __out BOOLEAN* isStateChanged
            );
        STDMETHOD(GetCopyContext)(__out IFabricOperationDataStream** copyContextEnumerator);
        STDMETHOD(GetCopyState)(
            __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            __in IFabricOperationDataStream* copyContextEnumerator,
            __out IFabricOperationDataStream** copyStateEnumerator
            );
        //
        // IFabricAtomicGroupStateProvider methods.
        //
        STDMETHOD(BeginAtomicGroupCommit)(
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            __in FABRIC_SEQUENCE_NUMBER commitSequenceNumber,
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndAtomicGroupCommit)(__in IFabricAsyncOperationContext* context);
        STDMETHOD(BeginAtomicGroupRollback)(
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            __in FABRIC_SEQUENCE_NUMBER rollbackSequenceNumber,
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndAtomicGroupRollback)(__in IFabricAsyncOperationContext* context);
        STDMETHOD(BeginUndoProgress)(
            __in FABRIC_SEQUENCE_NUMBER fromCommitSequenceNumber,
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndUndoProgress)(__in IFabricAsyncOperationContext* context);
        //
        // IFabricStateReplicator methods.
        //
        STDMETHOD(BeginReplicate)(
            __in IFabricOperationData* operationData,
            __in IFabricAsyncOperationCallback* callback,
            __out FABRIC_SEQUENCE_NUMBER* sequenceNumber,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndReplicate)(__in IFabricAsyncOperationContext* context, __out FABRIC_SEQUENCE_NUMBER* sequenceNumber);
        STDMETHOD(GetCopyStream)(__out IFabricOperationStream** stream);
        STDMETHOD(GetReplicationStream)(__out IFabricOperationStream** stream);
        STDMETHOD(UpdateReplicatorSettings)(__in FABRIC_REPLICATOR_SETTINGS const * replicatorSettings);
        //
        // IFabricStateReplicator2 methods.
        //
        STDMETHOD(GetReplicatorSettings)(__out IFabricReplicatorSettingsResult** replicatorSettings);
        //
        // IFabricInternalManagedReplicator methods
        //
        STDMETHOD(BeginReplicate2)(
            __in ULONG bufferCount,
            __in FABRIC_OPERATION_DATA_BUFFER_EX1 const * buffers,
            __in IFabricAsyncOperationCallback * callback,
            __out FABRIC_SEQUENCE_NUMBER * sequenceNumber,
            __out IFabricAsyncOperationContext ** context);
        //
        // IFabricInternalStateReplicator methods
        //
        STDMETHOD(ReserveSequenceNumber)(
            __in BOOLEAN alwaysReserveWhenPrimary,
            __out FABRIC_SEQUENCE_NUMBER * sequenceNumber
            );
        STDMETHOD(BeginReplicateBatch)(
            __in IFabricBatchOperationData* operationData,
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndReplicateBatch)(__in IFabricAsyncOperationContext* context);
        STDMETHOD(GetBatchReplicationStream)(__out IFabricBatchOperationStream** stream);
        STDMETHOD(GetReplicationQueueCounters)(__out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS * counters);
        //
        // IFabricAtomicGroupStateReplicator methods.
        //
        STDMETHOD(CreateAtomicGroup)(__out FABRIC_ATOMIC_GROUP_ID* atomicGroupId);
        STDMETHOD(BeginReplicateAtomicGroupOperation)(
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            __in IFabricOperationData* operationData,
            __in IFabricAsyncOperationCallback* callback,
            __out FABRIC_SEQUENCE_NUMBER* operationSequenceNumber,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndReplicateAtomicGroupOperation)(__in IFabricAsyncOperationContext* context, __out FABRIC_SEQUENCE_NUMBER* operationSequenceNumber);
        STDMETHOD(BeginReplicateAtomicGroupCommit)(
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            __in IFabricAsyncOperationCallback* callback,
            __out FABRIC_SEQUENCE_NUMBER* commitSequenceNumber,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndReplicateAtomicGroupCommit)(__in IFabricAsyncOperationContext* context, __out FABRIC_SEQUENCE_NUMBER* commitSequenceNumber);
        STDMETHOD(BeginReplicateAtomicGroupRollback)(
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            __in IFabricAsyncOperationCallback* callback,
            __out FABRIC_SEQUENCE_NUMBER* rollbackSequenceNumber,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndReplicateAtomicGroupRollback)(__in IFabricAsyncOperationContext* context, __out FABRIC_SEQUENCE_NUMBER* rollbackSequenceNumber);
        //
        // IOperationDataFactory methods.
        //
        STDMETHOD(CreateOperationData)(
            __in_ecount( segmentSizesCount ) ULONG * segmentSizes,
            __in ULONG segmentSizesCount,
            __out IFabricOperationData** operationData
            );
        STDMETHOD(CreateOperationData)(
            __in ULONG bufferCount,
            __in FABRIC_OPERATION_DATA_BUFFER_EX1 const * buffers,
            __out IFabricOperationData** operationData
            );
        //
        // Initialization/Cleanup the stateful service unit. Overrides from base class.
        //
        STDMETHOD(FinalConstruct)(
            __in ServiceModel::CServiceGroupMemberDescription* serviceActivationInfo,
            __in IFabricStatefulServiceFactory* factory
            );
        STDMETHOD(FinalDestruct)(void);
        //
        // Operation stream helper methods.
        //
        HRESULT StartOperationStreams(void);
        HRESULT EnqueueCopyOperation(__in_opt IFabricOperation* operation);
        HRESULT EnqueueReplicationOperation(__in_opt IFabricOperation* operation);
        HRESULT ForceDrainReplicationStream(__in BOOLEAN waitForNullDispatch);
        HRESULT UpdateOperationStreams(__in BOOLEAN isClosing, __in FABRIC_REPLICA_ROLE newReplicaRole);
        HRESULT ClearOperationStreams(void);
        HRESULT DrainUpdateEpoch(void);
        //
        // TRUE if the state provider is an atomic state provider.
        //
        bool HasInnerAtomicGroupStateProvider(void) { return (NULL != innerAtomicGroupStateProvider_); }
        //
        // Inner stateful service replica query interface.
        //
        HRESULT InnerQueryInterface(__in REFIID riid, __out void ** ppvObject);
        //
        // Called as part of faulting or aborting the replica after report fault has been called by the service.
        //
        void InternalTerminateOperationStreams(__in BOOLEAN forceDrainStream, __in BOOLEAN isReportFault);
        //
        // Used to release resources acquired during open.
        //
        void FixUpFailedOpen(void);
    
    private:
        //
        // Signals when the outer object have been destructed.
        //
        BOOLEAN outerStateReplicatorDestructed_;
        //
        // Outer stateful service partition. System provided.
        //
        IFabricStatefulServicePartition3* outerStatefulServicePartition_;
        //
        // Outer service group partition.
        //
        IFabricServiceGroupPartition* outerServiceGroupPartition_;
        //
        // Replica id of this stateful service unit. System provided.
        //
        FABRIC_REPLICA_ID replicaId_;
        //
        // Service replica contained by the service group unit. User service provided.
        //
        IFabricStatefulServiceReplica* innerStatefulServiceReplica_;
        //
        // State provider from the service replica. User service provided.
        //
        IFabricStateProvider* innerStateProvider_;
        //
        // State provider from the service replica. User service provided.
        //
        IFabricAtomicGroupStateProvider* innerAtomicGroupStateProvider_;
        //
        // Outer state replicator. System provided.
        //
        IFabricStateReplicator2* outerStateReplicator_;
        //
        // Atomic set outer state replicator. System provided.
        //
        IFabricAtomicGroupStateReplicator* outerAtomicGroupStateReplicator_;
        //
        // Cached service endpoint. Service replica provided.
        //
        IFabricStringResult* serviceEndpoint_;
        //
        // Current replica role of this stateful service unit.
        //
        FABRIC_REPLICA_ROLE currentReplicaRole_;
        //
        // New proposed replica role of this stateful service unit. Has same value as currentReplicaRole_ except during a change role operation.
        //
        FABRIC_REPLICA_ROLE proposedReplicaRole_;
        //
        // Copy operation stream.
        //
        COperationStream* copyOperationStream_;
        BOOLEAN copyStreamDrainingStarted_;
        //
        // Replication operation stream.
        //
        COperationStream* replicationOperationStream_;
        BOOLEAN replicationStreamDrainingStarted_;
        //
        // Signals that this replica is closing.
        //
        BOOLEAN isClosing_;
    };
}
