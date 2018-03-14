// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"

#include "ServiceGroup.AsyncOperations.h"

namespace ServiceGroup
{
    class COperationDataStream : 
        public IFabricOperationDataStream,
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
            DENY_COPY(COperationDataStream)
        
        public:
            //
            // Constructor/Destructor.
            //
            COperationDataStream(const Common::Guid& partitionId);
            virtual ~COperationDataStream(void);
            
            BEGIN_COM_INTERFACE_LIST(COperationDataStream)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationDataStream)
                COM_INTERFACE_ITEM(IID_IFabricOperationDataStream, IFabricOperationDataStream)
            END_COM_INTERFACE_LIST()
            
            //
            // IFabricOperationDataStream methods.
            //
            STDMETHOD(BeginGetNext)(__in IFabricAsyncOperationCallback* callback, __out IFabricAsyncOperationContext ** context);
            STDMETHOD(EndGetNext)(__in IFabricAsyncOperationContext* context, __out IFabricOperationData** operationData);
            //
            // Initialization.
            //
            HRESULT FinalConstruct(void);
            //
            // Additional methods.
            //
            HRESULT EnqueueOperationData(__in HRESULT errorCode, __in_opt IFabricOperationData* operationData);
            HRESULT ForceDrain(__in HRESULT errorCode);
            static void CompleteWaiter(
                __in HRESULT errorCode, 
                __in CGetOperationDataAsyncOperationContext* contextCompleted, 
                __in IFabricOperationData* operationCompleted
                );
            static void CompleteWaiterOnThreadPool(
                __in HRESULT errorCode, 
                __in CGetOperationDataAsyncOperationContext* contextCompleted, 
                __in IFabricOperationData* operationCompleted
                );

        private:
            //
            // The service partition id..
            //
            Common::Guid partitionId_;
            //
            // Concurrency control for the operationData queue.
            //
            Common::RwLock operationDataLock_;
            //
            // Operation Data queue.
            //
            std::list<IFabricOperationData*> operationDataQueue_;
            //
            // Operation Data waiters.
            //
            std::list<CGetOperationDataAsyncOperationContext*> operationDataWaiters_;
            //
            // The last operationData has been seen.
            //
            BOOLEAN lastOperationDataSeen_;
            //
            // The last operationData has been dispatched.
            //
            BOOLEAN lastOperationDataDispatched_;
            //
            // Event set when the last operationData is dispatched.
            //
            Common::ManualResetEvent waitLastOperationDataDispatched_;
            //
            // Failure occured.
            //
            HRESULT errorCode_;
    };
}

