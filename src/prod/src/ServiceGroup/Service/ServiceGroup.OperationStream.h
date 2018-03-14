// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"

#include "ServiceGroup.Constants.h"
#include "ServiceGroup.AsyncOperations.h"
#include "ServiceGroup.AtomicGroup.h"

namespace ServiceGroup
{
    class COperationStream : 
        public IFabricOperationStream2, 
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
            DENY_COPY(COperationStream)
        
        public:
            //
            // Constructor/Destructor.
            //
            COperationStream(__in const Common::Guid& partitionId, __in BOOLEAN isCopyStream);
            virtual ~COperationStream(void);
            
            BEGIN_COM_INTERFACE_LIST(COperationStream)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationStream)
                COM_INTERFACE_ITEM(IID_IFabricOperationStream, IFabricOperationStream)
                COM_INTERFACE_ITEM(IID_IFabricOperationStream2, IFabricOperationStream2)
            END_COM_INTERFACE_LIST()
            
            //
            // IFabricOperationStream methods.
            //
            STDMETHOD(BeginGetOperation)(__in IFabricAsyncOperationCallback* callback, __out IFabricAsyncOperationContext ** context);
            STDMETHOD(EndGetOperation)(__in IFabricAsyncOperationContext* context, __out IFabricOperation** operation);
            //
            // IFabricOperationStream2 methods.
            //
            STDMETHOD(ReportFault)(__in FABRIC_FAULT_TYPE faultType);
            //
            // Initialization.
            //
            HRESULT FinalConstruct(void);
            //
            // Additional methods.
            //
            HRESULT EnqueueOperation(__in_opt IFabricOperation* operation);
            HRESULT EnqueueUpdateEpochOperation(__in IOperationBarrier* operation);
            HRESULT ForceDrain(__in HRESULT errorCode, __in BOOLEAN waitForNullDispatch);
            HRESULT Drain(void);
            static void CompleteWaiter(__in CGetOperationAsyncOperationContext* contextCompleted, __in IFabricOperation* operationCompleted);
            static void CompleteWaiterOnThreadPool(__in CGetOperationAsyncOperationContext* contextCompleted, __in IFabricOperation* operationCompleted);
            inline LPCWSTR GetStreamName(void) { return isCopyStream_ ? COPY_STREAM_NAME : REPLICATION_STREAM_NAME; };

        private:
            //
            // The service partition id..
            //
            Common::Guid partitionId_;
            //
            // Concurrency control for the operation queue.
            //
            Common::RwLock operationLock_;
            //
            // Operation queue.
            //
            std::list<IFabricOperation*> operationQueue_;
            //
            // Operation waiters.
            //
            std::list<CGetOperationAsyncOperationContext*> operationWaiters_;
            //
            // The last operation has been seen.
            //
            BOOLEAN lastOperationSeen_;
            //
            // The last operation has been dispatched.
            //
            BOOLEAN lastOperationDispatched_;
            //
            // Event set when the last operation is dispatched.
            //
            Common::ManualResetEvent waitLastOperationDispatched_;
            //
            // Error code set by force drain operations.
            //
            HRESULT errorCode_;
            //
            // Set during a call to update epoch.
            //
            IOperationBarrier* operationUpdateEpoch_;
            //
            // Specifies if this operation stream is copy or replication.
            //
            BOOLEAN isCopyStream_;
    };
}

