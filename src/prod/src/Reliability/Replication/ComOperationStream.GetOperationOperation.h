// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // {B0FA9C90-92AC-4964-A9D3-00243A6D2CC7}
        static const GUID CLSID_ComOperationStream_GetOperationOperation = 
            { 0xb0fa9c90, 0x92ac, 0x4964, { 0xa9, 0xd3, 0x0, 0x24, 0x3a, 0x6d, 0x2c, 0xc7 } };
                

        class ComOperationStream::GetOperationOperation : 
            public Common::ComAsyncOperationContext
        {
            DENY_COPY(GetOperationOperation)

            COM_INTERFACE_AND_DELEGATE_LIST(
                GetOperationOperation,
                CLSID_ComOperationStream_GetOperationOperation,
                GetOperationOperation,
                ComAsyncOperationContext)

        public:
            explicit GetOperationOperation(__in OperationStream & operationStream);

            virtual ~GetOperationOperation() { }

            HRESULT STDMETHODCALLTYPE Initialize(
                Common::ComponentRootSPtr const & rootSPtr,
                __in_opt IFabricAsyncOperationCallback * callback);

            static HRESULT End(
                __in IFabricAsyncOperationContext * context, 
                __out IFabricOperation ** operation);

        protected:
            virtual void OnStart(
                Common::AsyncOperationSPtr const & proxySPtr);

        private:
            void GetOperationCallback(Common::AsyncOperationSPtr const & asyncOperation);

            void FinishGetOperation(Common::AsyncOperationSPtr const & asyncOperation);

            OperationStream & operationStream_;
            IFabricOperation * operation_;
        };

     } // end namespace ReplicationComponent
} // end namespace Reliability
