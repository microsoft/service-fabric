// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// *************
// For CITs only
// *************

namespace Naming
{
    class MockReplicator :
        public IFabricStateReplicator,
        public IFabricInternalStateReplicator,
        public IFabricReplicator,
        public Common::ComUnknownBase
    {
        BEGIN_COM_INTERFACE_LIST(MockReplicator)
        COM_INTERFACE_ITEM(IID_IUnknown,IFabricStateReplicator)
        COM_INTERFACE_ITEM(IID_IFabricStateReplicator,IFabricStateReplicator)
        COM_INTERFACE_ITEM(IID_IFabricInternalStateReplicator,IFabricInternalStateReplicator)
        COM_INTERFACE_ITEM(IID_IFabricReplicator,IFabricReplicator)
        END_COM_INTERFACE_LIST()

    public:
        MockReplicator() : sequenceNumber_(0) { }

        void InitializeSequenceNumber(::FABRIC_SEQUENCE_NUMBER sequenceNumber)
        {
            sequenceNumber_ = sequenceNumber;
        }

        // IFabricReplicator
        
        HRESULT STDMETHODCALLTYPE BeginOpen(
            __in IFabricAsyncOperationCallback *,
            __out IFabricAsyncOperationContext **) 
        { 
            return E_NOTIMPL; 
        }

        HRESULT STDMETHODCALLTYPE EndOpen(
            __in IFabricAsyncOperationContext *,
            __out IFabricStringResult **) 
        { 
            return E_NOTIMPL; 
        }

        HRESULT STDMETHODCALLTYPE BeginChangeRole(
            ::FABRIC_EPOCH const *,
            ::FABRIC_REPLICA_ROLE,
            __in IFabricAsyncOperationCallback *,
            __out IFabricAsyncOperationContext **) 
        { 
            return E_NOTIMPL; 
        }

        HRESULT STDMETHODCALLTYPE EndChangeRole(
            __in IFabricAsyncOperationContext *)
        { 
            return E_NOTIMPL; 
        }

        HRESULT STDMETHODCALLTYPE BeginUpdateEpoch(
            ::FABRIC_EPOCH const *,
            __in IFabricAsyncOperationCallback *,
            __out IFabricAsyncOperationContext **) 
        { 
            return E_NOTIMPL; 
        }

        HRESULT STDMETHODCALLTYPE EndUpdateEpoch(
            __in IFabricAsyncOperationContext *)
        { 
            return E_NOTIMPL; 
        }

        HRESULT STDMETHODCALLTYPE BeginClose(
            __in IFabricAsyncOperationCallback *,
            __out IFabricAsyncOperationContext **) 
        { 
            return E_NOTIMPL; 
        }

        HRESULT STDMETHODCALLTYPE EndClose(
            __in IFabricAsyncOperationContext *) 
        { 
            return E_NOTIMPL; 
        }

        void STDMETHODCALLTYPE Abort() 
        { 
        }

        HRESULT STDMETHODCALLTYPE GetCurrentProgress(__out ::FABRIC_SEQUENCE_NUMBER *)
        {
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetCatchUpCapability(__out ::FABRIC_SEQUENCE_NUMBER *)
        {
            return E_NOTIMPL;
        }

        // IFabricStateReplicator
        
        HRESULT STDMETHODCALLTYPE BeginReplicate(
            __in IFabricOperationData *,
            __in IFabricAsyncOperationCallback * callback,
            __out ::FABRIC_SEQUENCE_NUMBER * sequenceNumber,
            __out IFabricAsyncOperationContext ** context)
        {
            if (callback == NULL || sequenceNumber == NULL || context == NULL)
            {
                return E_FAIL;
            }

            *sequenceNumber = ++sequenceNumber_;
            Common::ComPointer<MockOperationContext> operationContext = Common::make_com<MockOperationContext>();
            HRESULT hr = operationContext->QueryInterface(IID_IFabricAsyncOperationContext, reinterpret_cast<void**>(context));
            ASSERT_IFNOT(SUCCEEDED(hr), "MockOperationContext does not support IFabricAsyncOperationContext: hr = {0}", hr);

            callback->Invoke(operationContext.GetRawPointer());

            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE EndReplicate(
            __in IFabricAsyncOperationContext *,
            __out ::FABRIC_SEQUENCE_NUMBER *)
        {
            return S_OK;
        }
        
        HRESULT STDMETHODCALLTYPE GetReplicationStream(
            __out IFabricOperationStream **)
        {
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetCopyStream(
            __out IFabricOperationStream **)
        {
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE UpdateReplicatorSettings(
            __in FABRIC_REPLICATOR_SETTINGS const * replicatorSettings)
        {
            UNREFERENCED_PARAMETER(replicatorSettings);
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE BeginReplicateBatch(
            /* [in] */ IFabricBatchOperationData * operationData,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context)
        {
            UNREFERENCED_PARAMETER(operationData);
            UNREFERENCED_PARAMETER(callback);
            UNREFERENCED_PARAMETER(context);
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE EndReplicateBatch(
            /* [in] */ IFabricAsyncOperationContext * context)
        {
            UNREFERENCED_PARAMETER(context);
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE ReserveSequenceNumber(
            /* [in] */ BOOLEAN alwaysReserveWhenPrimary,
            /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * sequenceNumber)
        {
            UNREFERENCED_PARAMETER(alwaysReserveWhenPrimary);
            UNREFERENCED_PARAMETER(sequenceNumber);
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetBatchReplicationStream(
            /* [out, retval] */ IFabricBatchOperationStream **batchStream)
        {
            UNREFERENCED_PARAMETER(batchStream);
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetReplicationQueueCounters(
            /* [out] */ FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS * counters)
        {
            counters->operationCount = 0;
            counters->queueSizeBytes = 0;
            return S_OK;
        }

    private:
        ::FABRIC_SEQUENCE_NUMBER sequenceNumber_;
    };
}
