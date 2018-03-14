// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"

namespace ServiceGroup
{
    //
    // Base class for COM-based async operations.
    //
    class COperationContext : 
        public IFabricAsyncOperationContext, 
        public IFabricAsyncOperationCallback,
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupCommon>
    {
        DENY_COPY(COperationContext)
    
    public:
        //
        // Constructor/Destructor.
        //
        COperationContext(void);
        virtual ~COperationContext(void);
    
        BEGIN_COM_INTERFACE_LIST(COperationContext)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationContext)
            COM_INTERFACE_ITEM(IID_IFabricAsyncOperationContext, IFabricAsyncOperationContext)
            COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        END_COM_INTERFACE_LIST()
        //
        // IFabricAsyncOperationContext methods.
        //
        BOOLEAN STDMETHODCALLTYPE IsCompleted(void);
        BOOLEAN STDMETHODCALLTYPE CompletedSynchronously(void);
        STDMETHOD(get_Callback)(__out IFabricAsyncOperationCallback** state);
        STDMETHOD(Cancel)(void);
        //
        // IFabricAsyncOperationCallback methods.
        //
        void STDMETHODCALLTYPE Invoke(__in IFabricAsyncOperationContext* context);
        //
        // Extra routines.
        //
        STDMETHOD(set_IsCompleted)(void);
        STDMETHOD(set_CompletedSynchronously)(__in BOOLEAN completedSynchronously);
        STDMETHOD(set_Callback)(__in IFabricAsyncOperationCallback* state);
        STDMETHOD(Wait)(__in DWORD dwMilliseconds);
        //
        // Initialize/Cleanup of the operation context.
        //
        STDMETHOD(FinalConstruct)(void);
        STDMETHOD(FinalDestruct)(void);
    
    protected:
        Common::ManualResetEvent wait_;
        BOOLEAN completedSynchronously_;
        IFabricAsyncOperationCallback* state_;
        HRESULT errorCode_;
    };
    
    class CGetOperationAsyncOperationContext : public COperationContext
    {
        DENY_COPY(CGetOperationAsyncOperationContext)
    
    public:
        //
        // Constructor/Destructor.
        //
        CGetOperationAsyncOperationContext(void);
        virtual ~CGetOperationAsyncOperationContext(void);
        //
        // Overrides.
        //
        STDMETHOD(set_Callback)(__in IFabricAsyncOperationCallback* state);
        STDMETHOD(get_Callback)(__out IFabricAsyncOperationCallback** state);
        //
        // Additional methods.
        //
        STDMETHOD(set_Operation)(__in HRESULT errorCode, __in_opt IFabricOperation* operation);
        STDMETHOD(get_Operation)(__out IFabricOperation** operation);
        //
        // Cleanup of the operation context.
        //
        STDMETHOD(FinalDestruct)(void);

    protected:
        IFabricOperation* operation_;
    };
    
    class CGetOperationDataAsyncOperationContext : public COperationContext
    {
        DENY_COPY(CGetOperationDataAsyncOperationContext)
    
    public:
        //
        // Constructor/Destructor.
        //
        CGetOperationDataAsyncOperationContext(void);
        virtual ~CGetOperationDataAsyncOperationContext(void);
        //
        // Overrides.
        //
        STDMETHOD(set_Callback)(__in IFabricAsyncOperationCallback* state);
        STDMETHOD(get_Callback)(__out IFabricAsyncOperationCallback** state);
        //
        // Additional methods.
        //
        STDMETHOD(set_OperationData)(__in HRESULT errorCode, __in_opt IFabricOperationData* operationData);
        STDMETHOD(get_OperationData)(__out IFabricOperationData** operationData);
        //
        // Cleanup of the operation context.
        //
        STDMETHOD(FinalDestruct)(void);

    protected:
        IFabricOperationData* operationData_;
    };
    
    class CBaseAsyncOperation : public COperationContext
    {
        DENY_COPY(CBaseAsyncOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CBaseAsyncOperation(__in_opt CBaseAsyncOperation* parent);
        virtual ~CBaseAsyncOperation(void);
        //
        // Overrides from CAsyncOperationContext.
        //
        STDMETHOD(set_IsCompleted)(void);
        //
        // Additional methods.
        //
        STDMETHOD(Begin)(void) = 0;
        STDMETHOD(End)(void);
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
        BOOLEAN HasStartedSuccessfully(void);
    
    protected:
        HRESULT errorCode_;
        CBaseAsyncOperation* parentOperation_;
        BOOLEAN startedSuccessfully_;
    };
    
    //
    // Common typedefs.
    //
    typedef std::list<CBaseAsyncOperation*>::iterator BaseAsyncOperation_Iterator;
    
    class CCompositeAsyncOperation : public CBaseAsyncOperation 
    {
        DENY_COPY(CCompositeAsyncOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CCompositeAsyncOperation(__in_opt CBaseAsyncOperation* parent);
        virtual ~CCompositeAsyncOperation(void);
        //
        // Overrides from CBaseAsyncOperation/CAsyncOperationContext.
        //
        STDMETHOD(set_IsCompleted)(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        STDMETHOD(End)(void);
        //
        // Additional methods.
        //
        STDMETHOD(Compose)(__in CBaseAsyncOperation* operation);
    
    protected:
        LONGLONG count_;
        std::list<CBaseAsyncOperation*> asyncOperations_;
    };
    
    class CCompositeAsyncOperationAsync : public CCompositeAsyncOperation 
    {
        DENY_COPY(CCompositeAsyncOperationAsync)
    
    public:
        //
        // Constructor/Destructor.
        //
        CCompositeAsyncOperationAsync(__in_opt CBaseAsyncOperation* parent, __in FABRIC_PARTITION_ID partitionId);
        virtual ~CCompositeAsyncOperationAsync(void);
        //
        // Overrides from CCompositeAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        STDMETHOD(End)(void);
        //
        // Overrides from CCompositeAsyncOperation.
        //
        STDMETHOD(set_IsCompleted)(void);

    protected:
        Common::Guid partitionId_;
    };

    class CSingleAsyncOperation : public CBaseAsyncOperation
    {
        DENY_COPY(CSingleAsyncOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CSingleAsyncOperation(__in_opt CBaseAsyncOperation* parent);
        virtual ~CSingleAsyncOperation(void);
        //
        // Overrides from CAsyncOperationContext.
        //
        void STDMETHODCALLTYPE Invoke(__in IFabricAsyncOperationContext* context);
    };
    
    class CStatefulAsyncOperation : public CSingleAsyncOperation
    {
        DENY_COPY(CStatefulAsyncOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulAsyncOperation(
            __in_opt CBaseAsyncOperation* parent,
            __in IFabricStatefulServiceReplica* serviceReplica
            );
        virtual ~CStatefulAsyncOperation(void);
    
    protected:
        IFabricStatefulServiceReplica* serviceReplica_; // __in
    };

    class CStatefulAsyncOperationAsync : public CStatefulAsyncOperation
    {
        DENY_COPY(CStatefulAsyncOperationAsync)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulAsyncOperationAsync(
            __in_opt CBaseAsyncOperation* parent,
            __in IFabricStatefulServiceReplica* serviceReplica
            );
        virtual ~CStatefulAsyncOperationAsync(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
    };
    
    class CStatefulOpenOperation : public CStatefulAsyncOperationAsync
    {
        DENY_COPY(CStatefulOpenOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulOpenOperation(
            __in_opt CBaseAsyncOperation* parent,
            __in FABRIC_REPLICA_OPEN_MODE openMode,
            __in IFabricStatefulServiceReplica* serviceReplica,
            __in IFabricStatefulServicePartition* servicePartition
            );
        virtual ~CStatefulOpenOperation(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        //
        // Overrides from CSingleAsyncOperation.
        //
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
        //
        // Additional methods.
        //
        STDMETHOD(get_Replicator)(__out IFabricReplicator** replicator);
    
    protected:
        FABRIC_REPLICA_OPEN_MODE openMode_; // __in
        IFabricStatefulServicePartition* servicePartition_; // __in
        IFabricReplicator* replicator_; // __out
    };
    
    class CStatefulCloseOperation : public CStatefulAsyncOperationAsync
    {
        DENY_COPY(CStatefulCloseOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulCloseOperation(
            __in_opt CBaseAsyncOperation* parent,
            __in IFabricStatefulServiceReplica* serviceReplica
            );
        virtual ~CStatefulCloseOperation(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        //
        // Overrides from CSingleAsyncOperation.
        //
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
    };
    
    class CStatefulChangeRoleOperation : public CStatefulAsyncOperationAsync
    {
        DENY_COPY(CStatefulChangeRoleOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulChangeRoleOperation(
            __in_opt CBaseAsyncOperation* parent,
            __in std::wstring const & serviceName,
            __in IFabricStatefulServiceReplica* serviceReplica,
            __in FABRIC_REPLICA_ROLE newReplicaRole
            );
        virtual ~CStatefulChangeRoleOperation(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        //
        // Overrides from CSingleAsyncOperation.
        //
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
        //
        // Additional methods.
        //
        STDMETHOD(get_ServiceEndpoint)(__out IFabricStringResult** serviceEndpoint);
        //
        // Properties.
        //
        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        inline std::wstring const & get_ServiceName() { return serviceName_; }

    protected:
        std::wstring const & serviceName_; // __in
        FABRIC_REPLICA_ROLE newReplicaRole_; // __in
        IFabricStringResult* serviceEndpoint_; // __out
    };
    
    class CStatelessAsyncOperation : public CSingleAsyncOperation
    {
        DENY_COPY(CStatelessAsyncOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatelessAsyncOperation(
            __in_opt CBaseAsyncOperation* parent,
            __in IFabricStatelessServiceInstance* serviceInstance
            );
        virtual ~CStatelessAsyncOperation(void);
    
    protected:
        IFabricStatelessServiceInstance* serviceInstance_; // __in
    };

    class CStatelessAsyncOperationAsync : public CStatelessAsyncOperation
    {
        DENY_COPY(CStatelessAsyncOperationAsync)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatelessAsyncOperationAsync(
            __in_opt CBaseAsyncOperation* parent,
            __in IFabricStatelessServiceInstance* serviceInstance
            );
        virtual ~CStatelessAsyncOperationAsync(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
    };
    
    class CStatelessOpenOperation : public CStatelessAsyncOperationAsync
    {
        DENY_COPY(CStatelessOpenOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatelessOpenOperation(
            __in_opt CBaseAsyncOperation* parent,
            __in std::wstring const & serviceName,
            __in IFabricStatelessServiceInstance* serviceInstance,
            __in IFabricStatelessServicePartition* servicePartition
            );
        virtual ~CStatelessOpenOperation(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        //
        // Overrides from CSingleAsyncOperation.
        //
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
        //
        // Additional methods.
        //
        STDMETHOD(get_ServiceEndpoint)(__out IFabricStringResult** serviceEndpoint);
        //
        // Properties.
        //
        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        inline std::wstring const & get_ServiceName() { return serviceName_; }
    
    protected:
        std::wstring const & serviceName_; // __in
        IFabricStatelessServicePartition* servicePartition_; // __in
        IFabricStringResult* serviceEndpoint_; // __out
    };

    class CStatelessCloseOperation : public CStatelessAsyncOperationAsync
    {
        DENY_COPY(CStatelessCloseOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatelessCloseOperation(
            __in_opt CBaseAsyncOperation* parent,
            __in IFabricStatelessServiceInstance* serviceInstance
            );
        virtual ~CStatelessCloseOperation(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        //
        // Overrides from CSingleAsyncOperation.
        //
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
    };
    
    class CStatefulCompositeChangeRoleOperation : public CCompositeAsyncOperationAsync
    {
        DENY_COPY(CStatefulCompositeChangeRoleOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulCompositeChangeRoleOperation(__in_opt CBaseAsyncOperation* parent, __in FABRIC_PARTITION_ID partitionId, __in FABRIC_REPLICA_ROLE newReplicaRole);
        virtual ~CStatefulCompositeChangeRoleOperation(void);
        //
        // Additional methods.
        //
        STDMETHOD(get_ServiceEndpoint)(__out IFabricStringResult** serviceEndpoint);
        STDMETHOD(get_ReplicaRole)(__out FABRIC_REPLICA_ROLE* newReplicaRole);
    private:
        FABRIC_REPLICA_ROLE newReplicaRole_;
    };
    
    class CStatelessCompositeOpenOperation : public CCompositeAsyncOperationAsync
    {
        DENY_COPY(CStatelessCompositeOpenOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatelessCompositeOpenOperation(__in_opt CBaseAsyncOperation* parent, __in FABRIC_PARTITION_ID partitionId);
        virtual ~CStatelessCompositeOpenOperation(void);
        //
        // Additional methods.
        //
        STDMETHOD(get_ServiceEndpoint)(__out IFabricStringResult** serviceEndpoint);
    };
    
    class CCompositeServiceEndpoint : 
        public IFabricStringResult, 
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupCommon>
    {
        DENY_COPY(CCompositeServiceEndpoint)
    
    public:
        //
        // Constructor/Destructor.
        //
        CCompositeServiceEndpoint(void);
        virtual ~CCompositeServiceEndpoint(void);
    
        BEGIN_COM_INTERFACE_LIST(CCompositeServiceEndpoint)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStringResult)
            COM_INTERFACE_ITEM(IID_IFabricStringResult, IFabricStringResult)
        END_COM_INTERFACE_LIST()
        //
        // IFabricStringResult methods.
        //
        LPCWSTR STDMETHODCALLTYPE get_String(void);
        //
        // Additional methods.
        //
        STDMETHOD(Compose)(__in IFabricStringResult* serviceEndpoint, __in std::wstring const & serviceName);
    
    protected:
        std::wstring serviceEndpoint_;
    };
    
    class CStateProviderAsyncOperationAsync : public CSingleAsyncOperation
    {
        DENY_COPY(CStateProviderAsyncOperationAsync)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStateProviderAsyncOperationAsync(
            __in_opt CBaseAsyncOperation* parent,
            __in IFabricStateProvider* stateProvider
            );
        virtual ~CStateProviderAsyncOperationAsync(void);
        //
        // Overrides from CSingleAsyncOperation.
        //
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
    
    protected:
        IFabricStateProvider* stateProvider_; // __in
    };
    
    class CStatefulOnDataLossOperation : public CStateProviderAsyncOperationAsync
    {
        DENY_COPY(CStatefulOnDataLossOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulOnDataLossOperation(
            __in_opt CBaseAsyncOperation* parent,
            __in IFabricStateProvider* stateProvider
            );
        virtual ~CStatefulOnDataLossOperation(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        //
        // Overrides from CSingleAsyncOperation.
        //
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
        //
        // Additional methods.
        //
        STDMETHOD(get_IsStateChanged)(__out BOOLEAN* isStateChanged);
    
    protected:
        BOOLEAN isStateChanged_; // __out
    };
    
    class CStatefulCompositeOnDataLossOperation : public CCompositeAsyncOperationAsync
    {
        DENY_COPY(CStatefulCompositeOnDataLossOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulCompositeOnDataLossOperation( __in_opt CBaseAsyncOperation* parent, __in FABRIC_PARTITION_ID partitionId);
        virtual ~CStatefulCompositeOnDataLossOperation(void);
        //
        // Additional methods.
        //
        STDMETHOD(get_IsStateChanged)(__out BOOLEAN* isStateChanged);
    };

    class CEmptyOperationDataAsyncEnumerator : 
        public IFabricOperationDataStream, 
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
        DENY_COPY(CEmptyOperationDataAsyncEnumerator)

    public:
        //
        // Constructor/Destructor.
        //
        CEmptyOperationDataAsyncEnumerator(void);
        virtual ~CEmptyOperationDataAsyncEnumerator(void);
    
        BEGIN_COM_INTERFACE_LIST(CEmptyOperationDataAsyncEnumerator)
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
    };

    class CCompositeAtomicGroupCommitRollbackOperation : public CCompositeAsyncOperation 
    {
        DENY_COPY(CCompositeAtomicGroupCommitRollbackOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CCompositeAtomicGroupCommitRollbackOperation(void);
        virtual ~CCompositeAtomicGroupCommitRollbackOperation(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Begin)(void);
    };

    class CAtomicGroupCommitRollbackOperation : public CSingleAsyncOperation
    {
        DENY_COPY(CAtomicGroupCommitRollbackOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CAtomicGroupCommitRollbackOperation(
            __in_opt CBaseAsyncOperation* parent,
            __in IFabricAtomicGroupStateProvider* atomicGroupStateProvider,
            __in BOOLEAN isCommit,
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            __in FABRIC_SEQUENCE_NUMBER sequenceNumber
            );
        virtual ~CAtomicGroupCommitRollbackOperation(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        //
        // Overrides from CSingleAsyncOperation.
        //
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
        
    private:
        IFabricAtomicGroupStateProvider* atomicGroupStateProvider_;
        BOOLEAN isCommit_;
        FABRIC_ATOMIC_GROUP_ID atomicGroupId_;
        FABRIC_SEQUENCE_NUMBER sequenceNumber_;
    };

    class CAtomicGroupCommitRollbackOperationAsync : public CStateProviderAsyncOperationAsync
    {
        DENY_COPY(CAtomicGroupCommitRollbackOperationAsync)
    
    public:
        //
        // Constructor/Destructor.
        //
        CAtomicGroupCommitRollbackOperationAsync(
            __in_opt CBaseAsyncOperation* parent,
            __in IFabricAtomicGroupStateProvider* atomicGroupStateProvider,
            __in BOOLEAN isCommit,
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            __in FABRIC_SEQUENCE_NUMBER sequenceNumber
            );
        virtual ~CAtomicGroupCommitRollbackOperationAsync(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        //
        // Overrides from CSingleAsyncOperation.
        //
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
        
    private:
        IFabricAtomicGroupStateProvider* atomicGroupStateProvider_;
        BOOLEAN isCommit_;
        FABRIC_ATOMIC_GROUP_ID atomicGroupId_;
        FABRIC_SEQUENCE_NUMBER sequenceNumber_;
    };

    class CAtomicGroupUndoProgressOperation : public CStateProviderAsyncOperationAsync
    {
        DENY_COPY(CAtomicGroupUndoProgressOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CAtomicGroupUndoProgressOperation(
            __in_opt CBaseAsyncOperation* parent,
            __in IFabricAtomicGroupStateProvider* atomicGroupStateProvider,
            __in FABRIC_SEQUENCE_NUMBER sequenceNumber
            );
        virtual ~CAtomicGroupUndoProgressOperation(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        //
        // Overrides from CSingleAsyncOperation.
        //
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
        
    private:
        IFabricAtomicGroupStateProvider* atomicGroupStateProvider_;
        FABRIC_SEQUENCE_NUMBER fromCommitSequenceNumber_;
    };

    class CUpdateEpochOperationAsync : public CStateProviderAsyncOperationAsync
    {
        DENY_COPY(CUpdateEpochOperationAsync)
    
    public:
        //
        // Constructor/Destructor.
        //
        CUpdateEpochOperationAsync(
            __in_opt CBaseAsyncOperation* parent,
            __in IFabricStateProvider* stateProvider,
            __in FABRIC_EPOCH const & epoch,
            __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber
            );
        virtual ~CUpdateEpochOperationAsync(void);
        //
        // Overrides from CBaseAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        //
        // Overrides from CSingleAsyncOperation.
        //
        STDMETHOD(Complete)(__in_opt IFabricAsyncOperationContext* context);
        
    private:
        
        FABRIC_EPOCH epoch_;
        FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber_;
    };

    class CCopyOperationCallbackContext : public COperationContext
    {
        DENY_COPY(CCopyOperationCallbackContext)
    
    public:
    //
    // Constructor/Destructor.
    //
        CCopyOperationCallbackContext();
        virtual ~CCopyOperationCallbackContext(void);

        //
        // Override from COperationContext.
        //
        void STDMETHODCALLTYPE Invoke(__in IFabricAsyncOperationContext* context);

        //
        // Additional methods.
        //
        HRESULT set_OperationContext(__in IFabricAsyncOperationContext* context)
        {
            if (NULL != context)
            {
                ASSERT_IF(NULL != serviceContext_, "non-NULL context");
                context->AddRef();
            }
            serviceContext_ = context;
            return S_OK;
        }
        HRESULT get_OperationContext(__out IFabricAsyncOperationContext** context)
        {
            if (NULL == context)
            {
                return E_POINTER;
            }
            if (NULL != serviceContext_)
            {
                serviceContext_->AddRef();
            }
            *context = serviceContext_;
            return S_OK;
        }
        HRESULT remove_OperationContext(void)
        {
            if (NULL != serviceContext_)
            {
                serviceContext_->Release();
                serviceContext_ = NULL;
            }
            return S_OK;
        }
    
    private:
         IFabricAsyncOperationContext* serviceContext_;
    };
    
    class CCopyOperationDataAsyncEnumerator : 
        public IFabricOperationDataStream,
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
        DENY_COPY(CCopyOperationDataAsyncEnumerator)
    
    public:
        //
        // Constructor/Destructor.
        //
        CCopyOperationDataAsyncEnumerator();
        virtual ~CCopyOperationDataAsyncEnumerator();
        //
        // Initialization.
        //
        HRESULT FinalConstruct(
            __in Common::Guid const & serviceGroupGuid,
            __in std::map<Common::Guid, IFabricOperationDataStream*> & copyOperationEnumerators
            );
                
        BEGIN_COM_INTERFACE_LIST(CCopyOperationDataAsyncEnumerator)
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

    protected:
        //
        // Additional methods
        //    
        BOOLEAN IsEmptyOperationData(
            __in IFabricOperationData* operationData
            );
        
        HRESULT GetExtendedOperationDataBuffer(
            __in IFabricOperationData* operationData,
            __in Common::Guid const & partitionId,
            __out IFabricOperationData** extendedOperationData
            );
    
    protected:
        Common::Guid serviceGroupGuid_;
        std::map<Common::Guid, IFabricOperationDataStream*> copyOperationEnumerators_;
        std::map<Common::Guid, IFabricOperationDataStream*>::iterator enumeratorIterator_;
    };
    
    class CCopyContextOperationData :
        public IFabricOperationData,
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
        DENY_COPY(CCopyContextOperationData)
        
    public:
        //
        // Constructor/Destructor.
        //
        CCopyContextOperationData(__in IFabricOperationData* operationData);
        virtual ~CCopyContextOperationData();        
    
        BEGIN_COM_INTERFACE_LIST(CCopyContextOperationData)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationData)
            COM_INTERFACE_ITEM(IID_IFabricOperationData, IFabricOperationData)
        END_COM_INTERFACE_LIST()
        //
        // IFabricOperationData methods.
        //
        STDMETHOD(GetData)(
            __out ULONG * count, 
            __out const FABRIC_OPERATION_DATA_BUFFER ** buffers
            );            

    private:
        IFabricOperationData* operationData_;
        
        FABRIC_OPERATION_DATA_BUFFER replicaBuffer_;
    };

    class CStatefulCompositeOpenUndoCloseAsyncOperationAsync : public CCompositeAsyncOperationAsync 
    {
        DENY_COPY(CStatefulCompositeOpenUndoCloseAsyncOperationAsync)

        enum Phase
        {
            Open,
            Undo,
            Close,
            Done
        };
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulCompositeOpenUndoCloseAsyncOperationAsync(
            __in_opt CBaseAsyncOperation* parent,
            __in FABRIC_PARTITION_ID partitionId
            );
        virtual ~CStatefulCompositeOpenUndoCloseAsyncOperationAsync(void);
        //
        // Overrides from CCompositeAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        STDMETHOD(End)(void);
        STDMETHOD(set_IsCompleted)(void);
        //
        // Addtional methods.
        //
        HRESULT Register(
            __in CCompositeAsyncOperationAsync* openOperationAsync,
            __in_opt CCompositeAsyncOperationAsync* undoOperationAsync,
            __in CCompositeAsyncOperationAsync* closeOperationAsync
            );

    private:
        CCompositeAsyncOperationAsync* openOperationAsync_;
        CCompositeAsyncOperationAsync* undoOperationAsync_;
        CCompositeAsyncOperationAsync* closeOperationAsync_;
        Phase phase_;
    };

    class CStatefulCompositeOnDataLossUndoAsyncOperationAsync : public CCompositeAsyncOperationAsync 
    {
        DENY_COPY(CStatefulCompositeOnDataLossUndoAsyncOperationAsync)

        enum Phase
        {
            DataLoss,
            Undo,
            Done
        };
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulCompositeOnDataLossUndoAsyncOperationAsync(
            __in_opt CBaseAsyncOperation* parent,
            __in FABRIC_PARTITION_ID partitionId
            );
        virtual ~CStatefulCompositeOnDataLossUndoAsyncOperationAsync(void);
        //
        // Overrides from CCompositeAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        STDMETHOD(End)(void);
        STDMETHOD(set_IsCompleted)(void);
        //
        // Addtional methods.
        //
        HRESULT Register(
            __in CStatefulCompositeOnDataLossOperation* dataLossOperationAsync,
            __in_opt CCompositeAsyncOperationAsync* undoOperationAsync
            );
        STDMETHOD(get_IsStateChanged)(__out BOOLEAN* isStateChanged);

    private:
        CStatefulCompositeOnDataLossOperation* dataLossOperationAsync_;
        CCompositeAsyncOperationAsync* undoOperationAsync_;
        Phase phase_;
    };

    //
    // forward declaration
    //
    class CStatefulServiceGroup;

    class CStatefulCompositeRollbackWithPostProcessingOperationAsync : public CCompositeAsyncOperationAsync
    {
        DENY_COPY(CStatefulCompositeRollbackWithPostProcessingOperationAsync)

        enum Phase
        {
            Rollback,
            PostProcessing,
            Done
        };

    public:
        //
        // Constructor/Destructor.
        //
        CStatefulCompositeRollbackWithPostProcessingOperationAsync(
            __in CStatefulServiceGroup* owner,
            __in_opt CBaseAsyncOperation* parent,
            __in FABRIC_PARTITION_ID partitionId,
            __in Common::StringLiteral const & traceType, // must be passed const reference to global constant
            __in std::wstring const & operationName       // must be passed const reference to global constant
            );
        virtual ~CStatefulCompositeRollbackWithPostProcessingOperationAsync(void);
        //
        // Overrides from CCompositeAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        STDMETHOD(End)(void);
        STDMETHOD(set_IsCompleted)(void);
        //
        // Addtional methods.
        //
        HRESULT Register(
            __in CCompositeAsyncOperationAsync* rollbackOperationAsync,
            __in CCompositeAsyncOperationAsync* postProcessingOperationAsync
            );

    protected:

        virtual void OnRollbackCompleted() = 0;

        CStatefulServiceGroup* owner_;
        CCompositeAsyncOperationAsync* postProcessingOperationAsync_;

    private:

        HRESULT BeginRollback(void);
        HRESULT BeginPostProcessing(void);

        CCompositeAsyncOperationAsync* rollbackOperationAsync_;
        Phase phase_;

        Common::StringLiteral const & traceType_;
        std::wstring const & operationName_;
    };

    class CStatefulCompositeRollbackUpdateEpochOperationAsync : public CStatefulCompositeRollbackWithPostProcessingOperationAsync
    {
        DENY_COPY(CStatefulCompositeRollbackUpdateEpochOperationAsync)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulCompositeRollbackUpdateEpochOperationAsync(
            __in CStatefulServiceGroup* owner,
            __in_opt CBaseAsyncOperation* parent,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_EPOCH const & epoch,
            __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber
            );
        virtual ~CStatefulCompositeRollbackUpdateEpochOperationAsync(void);
        
        
        FABRIC_EPOCH const & get_Epoch() { return epoch_; }
        FABRIC_SEQUENCE_NUMBER get_PreviousEpochLastSequenceNumber() { return previousEpochLastSequenceNumber_; }

    protected:
        //
        // Cleanup
        //
        void OnRollbackCompleted();

    private:

        FABRIC_EPOCH epoch_;
        FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber_;
    };

    class CStatefulCompositeRollbackCloseOperationAsync : public CStatefulCompositeRollbackWithPostProcessingOperationAsync
    {
        DENY_COPY(CStatefulCompositeRollbackCloseOperationAsync)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulCompositeRollbackCloseOperationAsync(
            __in CStatefulServiceGroup* owner,
            __in_opt CBaseAsyncOperation* parent,
            __in FABRIC_PARTITION_ID partitionId
            );
        virtual ~CStatefulCompositeRollbackCloseOperationAsync(void);

    protected:
        //
        // Cleanup
        //
        void OnRollbackCompleted();
    };

    class CStatefulCompositeRollbackChangeRoleOperationAsync : public CStatefulCompositeRollbackWithPostProcessingOperationAsync
    {
        DENY_COPY(CStatefulCompositeRollbackChangeRoleOperationAsync)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatefulCompositeRollbackChangeRoleOperationAsync(
            __in CStatefulServiceGroup* owner,
            __in_opt CBaseAsyncOperation* parent,
            __in FABRIC_PARTITION_ID partitionId
            );
        virtual ~CStatefulCompositeRollbackChangeRoleOperationAsync(void);

        //
        // Additional methods.
        //
        STDMETHOD(get_ServiceEndpoint)(__out IFabricStringResult** serviceEndpoint);
        STDMETHOD(get_ReplicaRole)(__out FABRIC_REPLICA_ROLE* newReplicaRole);

    protected:
        //
        // Cleanup
        //
        void OnRollbackCompleted();
    };

    class CStatelessCompositeOpenCloseAsyncOperationAsync : public CCompositeAsyncOperationAsync 
    {
        DENY_COPY(CStatelessCompositeOpenCloseAsyncOperationAsync)

        enum Phase
        {
            Open,
            Close,
            Done
        };
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatelessCompositeOpenCloseAsyncOperationAsync(
            __in_opt CBaseAsyncOperation* parent,
            __in FABRIC_PARTITION_ID partitionId
            );
        virtual ~CStatelessCompositeOpenCloseAsyncOperationAsync(void);
        //
        // Overrides from CCompositeAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        STDMETHOD(End)(void);
        STDMETHOD(set_IsCompleted)(void);
        //
        // Addtional methods.
        //
        HRESULT get_ServiceEndpoint(__out IFabricStringResult** serviceEndpoint);
        HRESULT Register(
            __in CStatelessCompositeOpenOperation* openOperationAsync,
            __in CCompositeAsyncOperationAsync* closeOperationAsync
            );

    private:
        CStatelessCompositeOpenOperation* openOperationAsync_;
        CCompositeAsyncOperationAsync* closeOperationAsync_;
        Phase phase_;
    };

    class CCompositeAtomicGroupCommitRollbackOperationAsync : public CCompositeAsyncOperationAsync 
    {
        DENY_COPY(CCompositeAtomicGroupCommitRollbackOperationAsync)
    
    public:
        //
        // Constructor/Destructor.
        //
        CCompositeAtomicGroupCommitRollbackOperationAsync(
            __in CStatefulServiceGroup* owner,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            __in BOOLEAN isCommit
            );
        virtual ~CCompositeAtomicGroupCommitRollbackOperationAsync(void);
        //
        // Overrides from CCompositeAsyncOperation.
        //
        STDMETHOD(Begin)(void);
        STDMETHOD(set_IsCompleted)(void);
    
    private:
        FABRIC_ATOMIC_GROUP_ID atomicGroupId_;
        BOOLEAN isCommit_;
        CStatefulServiceGroup* owner_;
    };
}

