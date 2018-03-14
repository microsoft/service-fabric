// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ComOperationStream : 
            public IFabricOperationStream2, 
            public IFabricBatchOperationStream, 
            private Common::ComUnknownBase
        {
            DENY_COPY(ComOperationStream)

            BEGIN_COM_INTERFACE_LIST(ComOperationStream)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationStream)
            COM_INTERFACE_ITEM(IID_IFabricOperationStream, IFabricOperationStream)
            COM_INTERFACE_ITEM(IID_IFabricOperationStream2, IFabricOperationStream2)
            COM_INTERFACE_ITEM(IID_IFabricBatchOperationStream, IFabricBatchOperationStream)
            END_COM_INTERFACE_LIST()

        public:
            explicit ComOperationStream(
                OperationStreamSPtr && stream);

            virtual ~ComOperationStream() {}
                        
            virtual HRESULT STDMETHODCALLTYPE BeginGetOperation(
                /* [in] */ __in IFabricAsyncOperationCallback * callback,
                /* [out, retval] */ __out IFabricAsyncOperationContext ** context);

            virtual HRESULT STDMETHODCALLTYPE EndGetOperation(
                /* [in] */ __in IFabricAsyncOperationContext * context,
                /* [out, retval] */ __out IFabricOperation ** operation);

            virtual HRESULT STDMETHODCALLTYPE BeginGetBatchOperation(
                /* [in] */ __in IFabricAsyncOperationCallback * callback,
                /* [out, retval] */ __out IFabricAsyncOperationContext ** context);

            virtual HRESULT STDMETHODCALLTYPE EndGetBatchOperation(
                /* [in] */ __in IFabricAsyncOperationContext * context,
                /* [out, retval] */ __out IFabricBatchOperation ** batchOperation);
            
            virtual HRESULT STDMETHODCALLTYPE ReportFault(
                /* [in] */ __in FABRIC_FAULT_TYPE faultType);
        private:
            class GetOperationOperation;

            OperationStreamSPtr operationStream_;
            Common::ComponentRootSPtr root_;
        };

    } // end namespace ReplicationComponent
} // end namespace Reliability
