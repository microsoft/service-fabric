// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"

#include "ServiceGroup.AsyncOperations.h"
#include "ServiceGroup.OperationDataStream.h"
#include "ServiceGroup.ReplicatorSettings.h"
#include "ServiceGroup.AtomicGroup.h"
#include "ServiceGroup.ServiceBase.h"
#include "ServiceGroup.StatefulServiceUnit.h"

namespace ServiceGroup
{
    //
    // The set of states in which an atomic group can exist.
    //
    namespace AtomicGroupStatus
    {
        enum Enum
        {
            IN_FLIGHT = 0,
            ROLLING_BACK = 1,
            COMMITTING = 2,
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum val);
    }

    #define ATOMIC_GROUP_STATUS_TEXT(x) \
        (AtomicGroupStatus::IN_FLIGHT == x)    ? TraceAtomicGroupStatusInFlight    : ( \
        (AtomicGroupStatus::ROLLING_BACK == x) ? TraceAtomicGroupStatusRollingBack : ( \
        (AtomicGroupStatus::COMMITTING == x)   ? TraceAtomicGroupStatusCommitting  : ( \
                              TraceAtomicGroupStatusInvalid       ))) \

    enum UpdateOperationStreamsReason
    {
        CHANGE_ROLE,
        CLOSE,
        ABORT
    };
    
    //
    // This represents the state of each atomic group participant.
    //
    class ParticipantState : public Serialization::FabricSerializable
    {
    public:
        ParticipantState()
        {
            replicating = replicated = 0;
            createdSequenceNumber = _I64_MAX;
        }

    public:
        //
        // Replicated operations that are still in flight for this participant.
        //
        LONG replicating;
        //
        // Replicated operations that have been acked for this participant.
        //
        LONG replicated;
        //
        // Replication LSN that created this participant.
        //
        LONGLONG createdSequenceNumber;
        //
        // Serialized fields.
        //
        FABRIC_FIELDS_01(createdSequenceNumber);
    };

    //
    // Atomic group state maintained by the service group. It is copied when replicas are being built.
    //
    class AtomicGroupState : public Serialization::FabricSerializable
    {
    public:
        AtomicGroupState()
        {
            status = AtomicGroupStatus::IN_FLIGHT;
            replicating = replicated = 0;
            terminator.assign(Common::Guid::Empty());
            createdSequenceNumber = _I64_MAX;
            terminationFailed = FALSE;
        }

    public:
        //
        // Current status of the atomic group.
        //
        AtomicGroupStatus::Enum status;
        //
        // Current participants in the atomic group.
        //
        std::map<Common::Guid, ParticipantState> participants;
        //
        // Replicated operations that are still in flight across all participants.
        //
        LONG replicating;
        //
        // Replicated operations that have beena acked across all participants.
        //
        LONG replicated;
        //
        // Participant that terminates the atomic group by calling commit or rollback.
        //
        Common::Guid terminator;
        //
        // Replication LSN that created this atomic group.
        //
        LONGLONG createdSequenceNumber;
        //
        // Set to TRUE if commit/rollback operation has failed.
        //
        BOOLEAN terminationFailed;
        //
        // Serialized fields.
        //
        FABRIC_FIELDS_04(
            status,
            participants,
            terminator,
            createdSequenceNumber);
    };

    //
    // Common typedefs.
    //
    typedef std::map<FABRIC_ATOMIC_GROUP_ID, AtomicGroupState>::iterator AtomicGroup_AtomicState_Iterator;
    typedef std::pair<FABRIC_ATOMIC_GROUP_ID, AtomicGroupState> AtomicGroup_AtomicState_Pair;
    typedef std::pair<std::map<FABRIC_ATOMIC_GROUP_ID, AtomicGroupState>::iterator, bool> AtomicGroup_AtomicState_Insert;
    typedef std::map<Common::Guid, std::set<FABRIC_ATOMIC_GROUP_ID>>::iterator Participant_AtomicGroup_Iterator;
    typedef std::pair<Common::Guid, std::set<FABRIC_ATOMIC_GROUP_ID>> Participant_AtomicGroup_Pair;
    typedef std::pair<std::set<FABRIC_ATOMIC_GROUP_ID>::iterator, bool> Participant_AtomicGroup_Insert;
    typedef std::map<Common::Guid, ParticipantState>::iterator Participant_ParticipantState_Iterator;
    typedef std::pair<std::map<Common::Guid, ParticipantState>::iterator, bool> Participant_ParticipantState_Insert;
    typedef std::pair<Common::Guid, ParticipantState> Participant_ParticipantState_Pair;
    typedef std::map<Common::Guid, ParticipantState>::const_iterator Participant_ParticipantState_Iterator_Const;
    typedef std::map<Common::Guid, ParticipantState>::const_iterator Participant_ParticipantState_Iterator_Const;

    class COperationStreamCallback;

    //
    // Class implementing a service group stateful partition hosting a collection of stateful services.
    //
    class CStatefulServiceGroup : 
        public IFabricStatefulServicePartition3,
        public IFabricStatefulServiceReplica, 
        public IFabricStateProvider, 
        public IFabricStateReplicator2,
        public IFabricAtomicGroupStateReplicator,
        public IFabricServiceGroupPartition,
        public IFabricConfigurationPackageChangeHandler,
        public CBaseServiceGroup
    {
        DENY_COPY(CStatefulServiceGroup)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulServiceGroup(FABRIC_REPLICA_ID replicaId);
        virtual ~CStatefulServiceGroup(void);
    
        BEGIN_COM_INTERFACE_LIST(CStatefulServiceGroup)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServicePartition)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition, IFabricStatefulServicePartition)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceReplica, IFabricStatefulServiceReplica)
            COM_INTERFACE_ITEM(IID_IFabricStateProvider, IFabricStateProvider)
            COM_INTERFACE_ITEM(IID_IFabricStateReplicator, IFabricStateReplicator)
            COM_INTERFACE_ITEM(IID_IFabricStateReplicator2, IFabricStateReplicator2)
            COM_INTERFACE_ITEM(IID_IFabricAtomicGroupStateReplicator, IFabricAtomicGroupStateReplicator)
            COM_INTERFACE_ITEM(IID_IFabricServiceGroupPartition, IFabricServiceGroupPartition)
            COM_INTERFACE_ITEM(IID_IServiceGroupReport, IServiceGroupReport)
            COM_INTERFACE_ITEM(IID_IFabricConfigurationPackageChangeHandler, IFabricConfigurationPackageChangeHandler)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition1, IFabricStatefulServicePartition1)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition2, IFabricStatefulServicePartition2)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition3, IFabricStatefulServicePartition3)
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
        // IFabricStateReplicator methods.
        //
        STDMETHOD(BeginReplicate)(
            __in IFabricOperationData* operationData,
            __in IFabricAsyncOperationCallback* callback,
            __out FABRIC_SEQUENCE_NUMBER* sequenceNumber,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndReplicate)(__in IFabricAsyncOperationContext* context,  __out FABRIC_SEQUENCE_NUMBER* sequenceNumber);
        STDMETHOD(GetCopyStream)(__out IFabricOperationStream** stream);
        STDMETHOD(GetReplicationStream)(__out IFabricOperationStream** stream);
        STDMETHOD(UpdateReplicatorSettings)(__in FABRIC_REPLICATOR_SETTINGS const * replicatorSettings);
        STDMETHOD(GetReplicatorSettings)(__out IFabricReplicatorSettingsResult** replicatorSettingsResult);
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
        STDMETHOD(ReportFault)(__in FABRIC_FAULT_TYPE faultType);
        //
        // IFabricStatefulServicePartition1 methods.
        //
        STDMETHOD(ReportMoveCost)(__in FABRIC_MOVE_COST moveCost);
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
        // IServiceGroupReport methods.
        //
        STDMETHOD(ReportLoad)(__in FABRIC_PARTITION_ID partitionId, __in ULONG metricCount, __in const FABRIC_LOAD_METRIC* metrics);
        STDMETHOD(ReportFault)(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_FAULT_TYPE faultType);
        STDMETHOD(ReportMoveCost)(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_MOVE_COST moveCost);
        //
        // IFabricConfigurationPackageChangeHandler
        //
        void STDMETHODCALLTYPE OnPackageAdded( 
            __in IFabricCodePackageActivationContext *source,
            __in IFabricConfigurationPackage *configPackage);
        void STDMETHODCALLTYPE OnPackageRemoved( 
            __in IFabricCodePackageActivationContext *source,
            __in IFabricConfigurationPackage *configPackage);
        void STDMETHODCALLTYPE OnPackageModified( 
            __in IFabricCodePackageActivationContext *source,
            __in IFabricConfigurationPackage *previousConfigPackage,
            __in IFabricConfigurationPackage *configPackage);
        //
        // Initialize/Cleanup of the service group. Overriden from base class.
        //
        STDMETHOD(FinalConstruct)(
            __in ServiceModel::CServiceGroupDescription* serviceGroupActivationInfo,
            __in const std::map<std::wstring, IFabricStatefulServiceFactory*>& statefulServiceFactories,
            __in const std::map<std::wstring, IFabricStatelessServiceFactory*>& statelessServiceFactories,
            __in IFabricCodePackageActivationContext* activationContext,
            __in LPCWSTR serviceTypeName
            );
        STDMETHOD(FinalDestruct)(void);
    
    protected:
        //
        // Helper methods.
        //
        HRESULT DoOpen(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        void FixUpFailedOpen(void);
        HRESULT DoClose(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        HRESULT DoChangeRole(
            __in FABRIC_REPLICA_ROLE newReplicaRole,
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        HRESULT DoUpdateEpoch(
            __in FABRIC_EPOCH const & epoch, 
            __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        HRESULT DoOnDataLoss(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        HRESULT DoGetCopyContext(__out IFabricOperationDataStream** copyContextEnumerator);
        HRESULT DoGetCopyState(
            __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            __in IFabricOperationDataStream* copyContextEnumerator,
            __out IFabricOperationDataStream** copyStateEnumerator
            );
        HRESULT DoAtomicGroupCommitRollback(
            __in BOOLEAN isCommit, 
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
            __in FABRIC_SEQUENCE_NUMBER sequenceNumber
            );
        HRESULT CreateAtomicGroupCommitRollbackOperation(
            __in BOOLEAN isCommit, 
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
            __in FABRIC_SEQUENCE_NUMBER sequenceNumber,
            __in_opt CBaseAsyncOperation* parent,
            __out CCompositeAsyncOperationAsync** operation);
        //
        // Retrieve enumerators for copy context.
        //
        HRESULT GetCopyContextEnumerators(__out std::map<Common::Guid, IFabricOperationDataStream*>& copyContextEnumerators);
        //
        // Retrieve enumerators for copy state.
        //
        HRESULT GetCopyStateEnumerators(
            __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            __in std::map<Common::Guid, COperationDataStream*>& serviceCopyContextEnumerators,
            __out std::map<Common::Guid, IFabricOperationDataStream*>& serviceCopyStateEnumerators
            );
        //
        // Operation stream related routines.
        //
        HRESULT StartInnerOperationStreams(void);
        HRESULT StartOperationStreams(void);
        HRESULT DrainOperationStream(__in BOOLEAN isCopy);
        HRESULT CompleteStreamOperation(
            __in IFabricAsyncOperationContext* context, 
            __in BOOLEAN isCopy
            );
        HRESULT CompleteCopyOperation(
            __in IFabricAsyncOperationContext* context
            );
        HRESULT CompleteSingleCopyOperation(
            __in const Common::Guid& serviceGroupUnitPartition,
            __in FABRIC_OPERATION_TYPE operationType,
            __in CStatefulServiceUnit* statefulServiceUnit,
            __in IFabricOperation* operation
            );
        HRESULT CompleteReplicationOperation(
            __in IFabricAsyncOperationContext* context
            );
        HRESULT CompleteSingleReplicationOperation(
            __in const Common::Guid& serviceGroupUnitPartition,
            __in FABRIC_OPERATION_TYPE operationType,
            __in CStatefulServiceUnit* statefulServiceUnit,
            __in IFabricOperation* operation
            );
        HRESULT CompleteAtomicGroupReplicationOperation(
            __in const Common::Guid& serviceGroupUnitParticipantPartition,
            __in FABRIC_OPERATION_TYPE operationType,
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            __in LONGLONG sequenceNumber,
            __in CStatefulServiceUnit* statefulServiceUnit,
            __in IFabricOperation* operation
            );
        HRESULT UpdateOperationStreams(__in UpdateOperationStreamsReason reason, __in FABRIC_REPLICA_ROLE newReplicaRole);
        //
        // Replicator settings related methods
        //
        HRESULT GetReplicatorSettingsFromActivationContext(__in ReplicatorSettings & replicatorSettings);
        HRESULT RegisterConfigurationChangeHandler();
        HRESULT UnregisterConfigurationChangeHandler();
        //
        // The callback used for the operation stream requires access to routines in this class.
        //
        friend class COperationStreamCallback;
        //
        // The operation for rollback requires access to routines in this class
        //
        friend class CStatefulCompositeRollbackUpdateEpochOperationAsync;
        friend class CStatefulCompositeRollbackCloseOperationAsync;
        friend class CStatefulCompositeRollbackChangeRoleOperationAsync;
        //
        // The operation for completing committed or rolled back atomic groups requires access to routines in this class
        //
        friend class CCompositeAtomicGroupCommitRollbackOperationAsync;
        //
        // Helper functions.
        //
        bool IsServiceGroupRelatedOperation(FABRIC_PARTITION_ID const & partitionId)
        {
            return (partitionId == partitionId_.AsGUID()) || (partitionId == Common::Guid::Empty().AsGUID());
        }

    protected:
        enum OperationStatus
        {
            REPLICATING,
            REPLICATED,
            COPYING
        };
        //
        // Atomic group related helper routines.
        //
        HRESULT CreateRollbackAtomicGroupsOperation(
            __in FABRIC_SEQUENCE_NUMBER sequenceNumber, 
            __in_opt CBaseAsyncOperation* parent,
            __out CCompositeAsyncOperationAsync** operation);
        HRESULT RefreshAtomicGroup(
            __in FABRIC_OPERATION_TYPE operationType,
            __in OperationStatus operationStatus,
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
            __in const Common::Guid& participant,
            __in const std::wstring & participantName
            );
        HRESULT ErrorFixupAtomicGroup(
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
            __in const Common::Guid& participant,
            __in const std::wstring & participantName
            );
        HRESULT RemoveAtomicGroup(__in FABRIC_ATOMIC_GROUP_ID atomicGroupId, __in OperationStatus operationStatus);
        HRESULT RemoveAtomicGroupOnCommitRollback(__in FABRIC_ATOMIC_GROUP_ID atomicGroupId);
        void UpdateLastCommittedLSN(__in LONGLONG lsn);
        void SetAtomicGroupSequenceNumber(
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
            __in const Common::Guid& participant,
            __in LONGLONG sequenceNumber
            );
        void SetAtomicGroupsReplicationDone(void);
        void DrainAtomicGroupsReplication(void);
        void RemoveAtomicGroupsOnUpdateEpoch(FABRIC_EPOCH const & epoch, FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber);
        void RemoveAllAtomicGroups();
        //
        // Gets the member with the specified id.
        //
        HRESULT GetStatefulServiceUnit(__in const Common::Guid & id, __out CStatefulServiceUnit** unit);
        //
        // Called as part of faulting the replica after report fault has been called by the service.
        //
        void InternalAbort(__in FABRIC_PARTITION_ID partitionId, __in BOOLEAN isReportFault);

    private:
        //
        // Signals when the outer object have been destructed.
        //
        BOOLEAN outerStateReplicatorDestructed_;
        //
        // Replica id of this stateful service group. System provided.
        //
        FABRIC_REPLICA_ID replicaId_;
        //
        // Simulated partition to stateful service unit map.
        //
        std::map<Common::Guid, CStatefulServiceUnit*> innerStatefulServiceUnits_;
        std::map<Common::Guid, BOOLEAN> innerStatefulServiceUnitsCopyCompletion_;
        std::map<Common::Guid, BOOLEAN>::iterator copyCompletionIterator_;
        //
        // Common typedefs.
        //
        typedef std::pair<Common::Guid, CStatefulServiceUnit*> PartitionId_StatefulServiceUnit_Pair;
        typedef std::map<Common::Guid, CStatefulServiceUnit*>::iterator PartitionId_StatefulServiceUnit_Iterator;
        typedef std::pair<PartitionId_StatefulServiceUnit_Iterator, bool> PartitionId_StatefulServiceUnit_Insert;
        typedef std::pair<Common::Guid, BOOLEAN> PartitionId_CopyCompletion_Pair;
        //
        // The stateful service partition. System provided.
        //
        IFabricStatefulServicePartition3* outerStatefulServicePartition_;
        //
        // Outer shared state replicator. System provided.
        //
        IFabricStateReplicator2* outerStateReplicator_;
        //
        // Outer shared replicator. System provided.
        //
        IFabricReplicator* outerReplicator_;
        //
        // Current replica role of this stateful service unit.
        //
        FABRIC_REPLICA_ROLE currentReplicaRole_;
        //
        // Specifies if the service group is persisted. This is populated as part of the service group initialization.
        //
        BOOLEAN hasPersistedState_;
        //
        // Operation stream callbacks.
        //
        COperationStreamCallback* replicationOperationStreamCallback_;
        COperationStreamCallback* copyOperationStreamCallback_;
        //
        // Operation stream events. They signal operation stream termination.
        //
        Common::ManualResetEvent lastReplicationOperationDispatched_;
        Common::ManualResetEvent lastCopyOperationDispatched_;
        size_t lastCopyOperationCount_;
        //
        // Replication and copy operation streams. System provided.
        //
        IFabricOperationStream* outerReplicationOperationStream_;
        IFabricOperationStream* outerCopyOperationStream_;
        //
        // Flags maintaining state around system operation streams.
        //
        BOOLEAN replicationStreamDrainingStarted_;
        BOOLEAN copyStreamDrainingStarted_;
        BOOLEAN streamsEnding_;
        //
        // Mode with which the replica was opened
        //
        FABRIC_REPLICA_OPEN_MODE openMode_;

    private:
        //
        // Atomic set id sequence.
        //
        LONGLONG sequenceAtomicGroupId_;
        //
        // Atomic set volatile state.
        //
        std::map<FABRIC_ATOMIC_GROUP_ID, AtomicGroupState> atomicGroups_;
        //
        // LSN that was last committed during an atomic group. It is used to avoid phantoms
        // during copy operations that race with replication operations.
        //
        LONGLONG lastCommittedLSN_;
        //
        // The current epoch of this replica. Copied duing replica build.
        //
        FABRIC_EPOCH currentEpoch_;
        //
        // Sequnce number received in update epoch calls.
        //
        LONGLONG previousEpochLsn_;
        //
        // TRUE after the atomic group copy state has been seen by a replica being built.
        //
        BOOLEAN isAtomicGroupCopied_;
        //
        // Atomic set state concurrency control.
        //
        Common::RwLock lockAtomicGroups_;
        //
        // Event signaled when the atomic group state has been copied.
        //
        Common::ManualResetEvent atomicGroupCopyDoneSignal_;
        //
        // Used to synchronize update epoch with still ongoing end replicate operations.
        //
        std::unique_ptr<Common::ManualResetEvent> atomicGroupReplicationDoneSignal_;
        //
        // Coordinates access to atomicGroupReplicationDoneSignal_
        //
        Common::RwLock lockAtomicGroupReplication_;

        AtomicGroupStateReplicatorCountersSPtr atomicGroupStateReplicatorCounters_;

    private:
        //
        // Activation context of the replica. 
        // If set the set service group registers for configuration changes and updates the replicator settings 
        //
        IFabricCodePackageActivationContext * activationContext_;
        //
        // ServiceTypeName used to locate the replicator settings in the service manifest
        //
        std::wstring serviceTypeName_;
        //
        // IFabricConfigurationPackageChangeHandler callback handle
        //
        LONGLONG configurationChangedCallbackHandle_;
        //
        // Name of the config package containing replicator settings
        //
        std::wstring replicatorSettingsConfigPackageName_;
        //
        // Indicates whether replicator settings are specified in a config package
        //
        BOOLEAN useReplicatorSettingsFromConfigPackage_;
        //
        // Used to coordinate the execution of the operation pump with report fault.
        //
        Common::RwLock lockFaultReplica_;
        //
        // TRUE if report fault has been observed.
        //
        BOOLEAN isReplicaFaulted_;
    };

    //
    // Operation stream callback. Same class is used for both copy and replication streams.
    //
    class COperationStreamCallback : 
        public IFabricAsyncOperationCallback, 
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
        DENY_COPY(COperationStreamCallback)

    public:
        //
        // Constructor/Destructor.
        //
        COperationStreamCallback(__in CStatefulServiceGroup* const statefulServiceGroup, __in BOOLEAN isCopyStream) : statefulServiceGroup_(statefulServiceGroup)
        {
            WriteNoise(TraceSubComponentStatefulServiceGroupOperationStreams, "this={0} - ctor", this);

            ASSERT_IF(NULL == statefulServiceGroup, "Unexpected");
            
            statefulServiceGroup->AddRef();
            isCopyStream_ = isCopyStream;
        }
        virtual ~COperationStreamCallback(void)
        {
            WriteNoise(TraceSubComponentStatefulServiceGroupOperationStreams, "this={0} - dtor", this);

            statefulServiceGroup_->Release();
        }

        BEGIN_COM_INTERFACE_LIST(COperationStreamCallback)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
            COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        END_COM_INTERFACE_LIST()
        //
        // IFabricAsyncOperationCallback methods.
        //
        void STDMETHODCALLTYPE Invoke(__in IFabricAsyncOperationContext* context)
        {
            ASSERT_IF(NULL == context, "Unexpected context");
            ASSERT_IF(NULL == statefulServiceGroup_, "Unexpected service group");
            
            if (!context->CompletedSynchronously())
            {
                //
                // Based on the type of stream operating on, complete copy or replication operation.
                //
                HRESULT hr = statefulServiceGroup_->CompleteStreamOperation(context, isCopyStream_);

                //
                // Check if the stream completed (last operation in the stream was seen). If so, exit.
                //
                if (HR_STOP_OPERATION_PUMP == hr)
                {
                    return;
                }

                //
                // Continue draining the stream (pick up the next operation).
                //
                statefulServiceGroup_->DrainOperationStream(isCopyStream_);
            }
        }
    
    protected:
        //
        // Service group operating on.
        //
        CStatefulServiceGroup* const statefulServiceGroup_;
        //
        // Distinguishes between copy and replication streams.
        //
        BOOLEAN isCopyStream_;
    };

    //
    // Serialization class used to copy current atomic groups to newly built replicas.
    //
    class AtomicGroupCopyState : public Serialization::FabricSerializable
    {
    public:
        AtomicGroupCopyState()
        {
            lastCommittedLSN = -1;
            epochDataLossNumber = -1;
            epochConfigurationNumber = -1;
        }

    public:
        //
        // This is the service group's last committed LSN.
        //
        LONGLONG lastCommittedLSN;
        //
        // The current epoch of the primary has to be copied to the newly built replica. If that is not done,
        // one cannot distinguish between UpdateEpoch calls that should rollback atomic groups and the 
        // ones that should not.
        //
        LONGLONG epochDataLossNumber;
        LONGLONG epochConfigurationNumber;
        //
        // The atomic groups.
        //
        std::map<FABRIC_ATOMIC_GROUP_ID, AtomicGroupState> atomicGroups;
        
        FABRIC_FIELDS_04(lastCommittedLSN, epochDataLossNumber, epochConfigurationNumber, atomicGroups);
    };

    //
    // Operation data implementation used by the atomic group state enumerator.
    //
    class CAtomicGroupStateOperationData : 
        public IFabricOperationData, 
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
        DENY_COPY(CAtomicGroupStateOperationData)
    
    public:
        //
        // Constructor/Destructor.
        //
        CAtomicGroupStateOperationData(void);
        virtual ~CAtomicGroupStateOperationData(void);
    
        BEGIN_COM_INTERFACE_LIST(CAtomicGroupStateOperationData)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationData)
            COM_INTERFACE_ITEM(IID_IFabricOperationData, IFabricOperationData)
        END_COM_INTERFACE_LIST()
        //
        // IFabricOperationData methods.
        //
        STDMETHOD(GetData)(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers);
        
        //
        // Initialization for this operation data.
        //
        STDMETHOD(FinalConstruct)(__in const AtomicGroupCopyState& atomicGroupCopyState);
        
    protected:
        std::vector<byte> buffer_;
        FABRIC_OPERATION_DATA_BUFFER replicaBuffer_;
    };

    //
    // Atomic group state enumerator.
    //
    class CAtomicGroupStateOperationDataAsyncEnumerator : 
        public IFabricOperationDataStream, 
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
        DENY_COPY(CAtomicGroupStateOperationDataAsyncEnumerator)

    public:
        //
        // Constructor/Destructor.
        //
        CAtomicGroupStateOperationDataAsyncEnumerator();
        virtual ~CAtomicGroupStateOperationDataAsyncEnumerator(void);
        
        BEGIN_COM_INTERFACE_LIST(CAtomicGroupStateOperationDataAsyncEnumerator)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationDataStream)
            COM_INTERFACE_ITEM(IID_IFabricOperationDataStream, IFabricOperationDataStream)
        END_COM_INTERFACE_LIST()
        //
        // IFabricOperationDataStream methods.
        //
        STDMETHOD(BeginGetNext)(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndGetNext)( 
            __in IFabricAsyncOperationContext* context,
            __out IFabricOperationData** operationData
            );
        //
        // Initialization/Cleanup.
        //
        STDMETHOD(FinalConstruct)(__in const AtomicGroupCopyState& atomicGroupCopyState);
        STDMETHOD(FinalDestruct)(void) { return S_OK; }

    private: 
        CAtomicGroupStateOperationData* operationData_;
        BOOLEAN done_;
    };

    //
    // This is the copy context that dispatches copy context operations to the primary
    // in the case of persisted services false progress detection.
    //
    class CCopyContextStreamDispatcher :
        public IFabricAsyncOperationCallback,
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
        DENY_COPY(CCopyContextStreamDispatcher)

    public:
        //
        // Constructor/Destructor.
        //
        CCopyContextStreamDispatcher(
            __in Common::Guid const & serviceGroupGuid,
            __in IFabricOperationDataStream* systemCopyContextEnumerator,
            __in std::map<Common::Guid, COperationDataStream*> & serviceCopyContextEnumerators);
        virtual ~CCopyContextStreamDispatcher();

        BEGIN_COM_INTERFACE_LIST(CCopyContextStreamDispatcher)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
            COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        END_COM_INTERFACE_LIST()

        //
        // IFabricAsyncOperationCallback method.
        //
        void STDMETHODCALLTYPE Invoke(
            __in IFabricAsyncOperationContext* context
            );

        //
        // Additional methods
        //
        HRESULT Dispatch();

        HRESULT FinishDispatch(HRESULT errorCode);

    protected:
        HRESULT DispatchOperationData(
            __in IFabricAsyncOperationContext* context,
            __out BOOLEAN* isLast);
        
    private:
        Common::Guid serviceGroupGuid_;
        IFabricOperationDataStream* systemCopyContextEnumerator_;
        std::map<Common::Guid, COperationDataStream*> serviceCopyContextEnumerators_;
    };

    class CStatefulCompositeUndoProgressOperation : public CCompositeAsyncOperationAsync
    {
        DENY_COPY(CStatefulCompositeUndoProgressOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulCompositeUndoProgressOperation( __in_opt CBaseAsyncOperation* parent, __in FABRIC_PARTITION_ID partitionId);
        virtual ~CStatefulCompositeUndoProgressOperation(void);
        //
        // Overrides from CCompositeAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        //
        // Additional methods.
        //
        HRESULT Register(__in CStatefulServiceUnit* statefulServiceUnit);

    private:
        std::vector<CStatefulServiceUnit*> statefulServiceUnits_;
    };

    class COperationDataAsyncEnumeratorWrapper :
        public IFabricOperationDataStream, 
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>

    {
        DENY_COPY(COperationDataAsyncEnumeratorWrapper);

    public:
        //
        // Constructor/Destructor.
        //
        COperationDataAsyncEnumeratorWrapper(IFabricOperationDataStream* enumerator);

        virtual ~COperationDataAsyncEnumeratorWrapper(void);

        BEGIN_COM_INTERFACE_LIST(COperationDataAsyncEnumeratorWrapper)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationDataStream)
            COM_INTERFACE_ITEM(IID_IFabricOperationDataStream, IFabricOperationDataStream)
        END_COM_INTERFACE_LIST()

        //
        // IFabricOperationDataStream methods.
        //
        STDMETHOD(BeginGetNext)(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );

        STDMETHOD(EndGetNext)( 
            __in IFabricAsyncOperationContext* context,
            __out IFabricOperationData** operationData
            );

    private:
        IFabricOperationDataStream* inner_;
    };
}

DEFINE_USER_MAP_UTILITY(Common::Guid, ServiceGroup::ParticipantState);
DEFINE_USER_MAP_UTILITY(FABRIC_ATOMIC_GROUP_ID, ServiceGroup::AtomicGroupState);
