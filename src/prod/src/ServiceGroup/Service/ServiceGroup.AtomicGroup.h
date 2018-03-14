// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"
#include "ServiceGroup.IOperationBarrier.h"
#include "ServiceGroup.OperationExtendedBuffer.h"

namespace ServiceGroup
{
    class CReplicationAtomicGroupOperationCallback : 
        public IFabricAsyncOperationCallback, 
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
        DENY_COPY(CReplicationAtomicGroupOperationCallback)

    public:
        //
        // Constructor/Destructor.
        //
        CReplicationAtomicGroupOperationCallback(void);
        virtual ~CReplicationAtomicGroupOperationCallback(void);

        BEGIN_COM_INTERFACE_LIST(CReplicationAtomicGroupOperationCallback)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
            COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        END_COM_INTERFACE_LIST()
        //
        // IFabricAsyncOperationCallback methods.
        //
        void STDMETHODCALLTYPE Invoke(__in IFabricAsyncOperationContext* context);
        //
        // Initialization/Cleanup for this callback.
        //
        STDMETHOD(FinalConstruct)(__in IFabricAsyncOperationCallback* externalCallback, __in IFabricAsyncOperationContext* internalContext);
        STDMETHOD(FinalDestruct)(void) { return S_OK; }
    
    protected:
        IFabricAsyncOperationCallback* externalCallback_;
        IFabricAsyncOperationContext* internalContext_;
    };

    class CAtomicGroupAsyncOperationContext : 
        public IFabricAsyncOperationContext, 
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
        DENY_COPY(CAtomicGroupAsyncOperationContext)

    public:
        //
        // Constructor/Destructor.
        //
        CAtomicGroupAsyncOperationContext(void);
        virtual ~CAtomicGroupAsyncOperationContext(void);

        BEGIN_COM_INTERFACE_LIST(CAtomicGroupAsyncOperationContext)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationContext)
            COM_INTERFACE_ITEM(IID_IFabricAsyncOperationContext, IFabricAsyncOperationContext)
        END_COM_INTERFACE_LIST()
        //
        // IFabricAsyncOperationContext methods.
        //
        BOOLEAN STDMETHODCALLTYPE IsCompleted();
        BOOLEAN STDMETHODCALLTYPE CompletedSynchronously();
        STDMETHOD(get_Callback)(__out IFabricAsyncOperationCallback** state);
        STDMETHOD(Cancel)(void);
        //
        // Initialization/Cleanup for this context.
        //
        STDMETHOD(FinalConstruct)(__in FABRIC_OPERATION_TYPE operationType, __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, __in const Common::Guid& participant);
        STDMETHOD(FinalDestruct)(void) { return S_OK; }
        //
        // Additional methods.
        //
        FABRIC_ATOMIC_GROUP_ID get_AtomicGroupId(void) { return atomicGroupId_; } 
        FABRIC_OPERATION_TYPE get_OperationType(void) { return operationType_; } 
        const Common::Guid& get_Participant(void) { return participant_; } 
        HRESULT set_SystemContext(__in IFabricAsyncOperationContext* systemContext) 
        {
            Common::AcquireWriteLock lockContext(lock_);
            if (NULL != systemContext_)
            {
                WriteNoise(TraceSubComponentAtomicGroupAsyncOperationContext, "this={0} - System context set to {1}", this, systemContext_);

                ASSERT_IF(systemContext_ != systemContext, "Unexpected system context");

                WriteNoise(TraceSubComponentAtomicGroupAsyncOperationContext, "this={0} - Ignoring system context {1}", this, systemContext);
            }
            else
            {
                WriteNoise(TraceSubComponentAtomicGroupAsyncOperationContext, "this={0} - Setting context to system context {1}", this, systemContext);

                systemContext_ = systemContext; 
            }
            return S_OK;
        } 
        HRESULT get_SystemContext(__out IFabricAsyncOperationContext** context) 
        {
            ASSERT_IF(NULL == context, "Unexpected context");
            ASSERT_IF(NULL == systemContext_, "Unexpected system context");
            systemContext_->AddRef();
            *context = systemContext_; 
            return S_OK;
        } 
        FABRIC_SEQUENCE_NUMBER* get_SequenceNumber(void)
        {
            return &sequenceNumber_;
        }

    protected:
        IFabricAsyncOperationContext* systemContext_;
        FABRIC_ATOMIC_GROUP_ID atomicGroupId_;
        FABRIC_SEQUENCE_NUMBER sequenceNumber_;
        FABRIC_OPERATION_TYPE operationType_;
        Common::Guid participant_;
        Common::RwLock lock_;
    };

    class CAtomicGroupCommitRollback : 
        public IFabricOperation,
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
        DENY_COPY(CAtomicGroupCommitRollback)
    
    public:
        //
        // Constructor/Destructor.
        //
        CAtomicGroupCommitRollback(__in BOOLEAN isCommit);
        virtual ~CAtomicGroupCommitRollback(void);

        BEGIN_COM_INTERFACE_LIST(CAtomicGroupCommitRollback)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperation)
            COM_INTERFACE_ITEM(IID_IFabricOperation, IFabricOperation)
        END_COM_INTERFACE_LIST()
        //
        // IFabricOperation methods.
        //
        const FABRIC_OPERATION_METADATA * STDMETHODCALLTYPE get_Metadata(void);

        STDMETHOD(GetData)(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers);        
        
        STDMETHOD(Acknowledge)(void);
        //
        // Initialization/Cleanup for this operation.
        //
        STDMETHOD(FinalConstruct)(
            __in size_t count,
            __in FABRIC_OPERATION_TYPE atomicOperationType, 
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
            __in IFabricOperation* operation);
        STDMETHOD(FinalDestruct)(void) { return S_OK; }
    
    protected:
        IFabricOperation* innerOperation_;
        FABRIC_OPERATION_METADATA operationMetadata_;
        LONG count_;
        FABRIC_OPERATION_DATA_BUFFER dataBuffer_;
        BOOLEAN isCommit_;
    };

    class CUpdateEpochOperation : 
        public IOperationBarrier,
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
        DENY_COPY(CUpdateEpochOperation)
    
    public:
        //
        // Constructor/Destructor.
        //
        CUpdateEpochOperation(void);
        virtual ~CUpdateEpochOperation(void);
        
        BEGIN_COM_INTERFACE_LIST(CUpdateEpochOperation)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperation)
            COM_INTERFACE_ITEM(IID_IFabricOperation, IFabricOperation)
            COM_INTERFACE_ITEM(IID_IOperationBarrier, IOperationBarrier)
        END_COM_INTERFACE_LIST()
        //
        // IFabricOperation methods.
        //
        const FABRIC_OPERATION_METADATA * STDMETHODCALLTYPE get_Metadata(void);
        STDMETHOD(GetData)(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers);
        
        STDMETHOD(Acknowledge)(void);
        //
        // Initialization/Cleanup for this operation.
        //
        STDMETHOD(FinalConstruct)(void);
        STDMETHOD(FinalDestruct)(void) { return S_OK; }
        //
        // IOperationBarrier methods.
        //
        STDMETHOD(Barrier)(void);
        STDMETHOD(Cancel)(void);
    
    protected:
        Common::ManualResetEvent handle_;
        HRESULT errorCode_;
        Common::RwLock lock_;
        FABRIC_OPERATION_METADATA operationMetadata_;
    };
}
