// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Async enumerator that gives copy context operations
        // to the state provider if it has persisted state
        class ComOperationDataAsyncEnumerator : 
            public IFabricOperationDataStream,
            private Common::ComUnknownBase
        {
            DENY_COPY(ComOperationDataAsyncEnumerator)

            COM_INTERFACE_LIST1(
                ComOperationDataAsyncEnumerator,
                IID_IFabricOperationDataStream,
                IFabricOperationDataStream)

        public:
            virtual ~ComOperationDataAsyncEnumerator();

            virtual HRESULT STDMETHODCALLTYPE BeginGetNext(
                /*[in]*/ IFabricAsyncOperationCallback *callback,
                /*[out, retval]*/ IFabricAsyncOperationContext **context);

            virtual HRESULT STDMETHODCALLTYPE EndGetNext(
                /*[in]*/ IFabricAsyncOperationContext *context,
                /*[out, retval]*/ IFabricOperationData ** operationData);

            void Close(bool errorsEncountered);

        private:
            class GetNextOperation;
            
            ComOperationDataAsyncEnumerator(
                Common::ComponentRoot const & root,
                DispatchQueueSPtr const & dispatchQueue);
                        
            template <class ComImplementation,class T0,class T1>
            friend Common::ComPointer<ComImplementation> Common::make_com(T0 && a0, T1 && a1);

            template <class ComImplementation,class ComInterface,class T0,class T1>
            friend Common::ComPointer<ComInterface> Common::make_com(T0 && a0, T1 && a1);

            Common::ComponentRootSPtr rootSPtr_;
            // DispatchQueue is kept alive by the root,
            // so we can keep it by ref
            DispatchQueueSPtr const & dispatchQueue_;
            volatile bool errorsEncountered_;
            volatile bool closeCalled_;
            
            MUTABLE_RWLOCK(REComOperationDataAsyncEnumerator, lock_);

        }; // end ComOperationDataAsyncEnumerator 

    } // end namespace ReplicationComponent
} // end namespace Reliability
