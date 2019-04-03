// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace NativeServiceGroupMember
{
    class DummyAsyncContext :
        public IFabricAsyncOperationContext,
        private ComUnknownBase
    {
        DENY_COPY(DummyAsyncContext)

            BEGIN_COM_INTERFACE_LIST(DummyAsyncContext)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationContext)
            COM_INTERFACE_ITEM(IID_IFabricAsyncOperationContext, IFabricAsyncOperationContext)
            END_COM_INTERFACE_LIST()

    public:

        DummyAsyncContext()
        {
        }

        ~DummyAsyncContext()
        {
        }

        void Initialize(IFabricAsyncOperationCallback * callback)
        {
            callback_.SetAndAddRef(callback);
        }

        BOOLEAN STDMETHODCALLTYPE IsCompleted()
        {
            return TRUE;
        }

        BOOLEAN STDMETHODCALLTYPE CompletedSynchronously() 
        {
            return TRUE;
        }

        STDMETHODIMP get_Callback(__out IFabricAsyncOperationCallback **state)
        {
            if (state == nullptr) { return E_POINTER; }
            return callback_->QueryInterface(state);
        }

        STDMETHODIMP Cancel() 
        {
            return S_OK;
        }

    private:

        ComPointer<IFabricAsyncOperationCallback> callback_;

        template <class ComImplementation>
        friend Common::ComPointer<ComImplementation> Common::make_com();
    };

    class StringResult :
        public IFabricStringResult,
        private ComUnknownBase
    {
        DENY_COPY(StringResult)

        BEGIN_COM_INTERFACE_LIST(StringResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStringResult)
            COM_INTERFACE_ITEM(IID_IFabricStringResult, IFabricStringResult)
        END_COM_INTERFACE_LIST()

    public:

        StringResult()
        {
        }

        ~StringResult()
        {
        }

        void Initialize(wstring const & result)
        {
            result_ = result;
        }

        LPCWSTR STDMETHODCALLTYPE get_String(void)
        {
            return result_.c_str();
        }

    private:

        wstring result_;
    };

    class NullOperationDataStream :
        public IFabricOperationDataStream,
        private ComUnknownBase
    {
        DENY_COPY(NullOperationDataStream)

        BEGIN_COM_INTERFACE_LIST(NullOperationDataStream)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationDataStream)
            COM_INTERFACE_ITEM(IID_IFabricOperationDataStream, IFabricOperationDataStream)
        END_COM_INTERFACE_LIST()

    public:

        NullOperationDataStream()
        {
        }

        ~NullOperationDataStream()
        {
        }

        STDMETHODIMP BeginGetNext( 
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context)
        {
            if (callback == nullptr || context == nullptr)
            {
                return E_POINTER;
            }

            try
            {
                ComPointer<IFabricAsyncOperationContext> dummy = make_com<DummyAsyncContext, IFabricAsyncOperationContext>();

                HRESULT hr = dummy->QueryInterface(context);
                if (FAILED(hr)) { return hr; }

                callback->Invoke(dummy.GetRawPointer());
                return hr;
            }
            catch (bad_alloc const &) { return E_OUTOFMEMORY; }
            catch (...) { return E_FAIL; }
        }

        STDMETHODIMP EndGetNext( 
            __in IFabricAsyncOperationContext *context,
            __out IFabricOperationData **operationData)
        {
            if (operationData == nullptr || context == nullptr)
            {
                return E_POINTER;
            }

            *operationData = NULL;
            return S_OK;
        }
    };

    class DrainOperationStreams :
        public IFabricAsyncOperationCallback,
        private ComUnknownBase
    {
        DENY_COPY(DrainOperationStreams)

            BEGIN_COM_INTERFACE_LIST(DrainOperationStreams)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
            COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
            END_COM_INTERFACE_LIST()

    public:
        DrainOperationStreams(IFabricStateReplicator * stateReplicator) :
        isCopying_(true)
        {
            stateReplicator_.SetAndAddRef(stateReplicator);
        }

        ~DrainOperationStreams()
        {
        }

        void Start()
        {
            done_.Reset();

            HRESULT hr = stateReplicator_->GetCopyStream(copyStream_.InitializationAddress());
            if (FAILED(hr))
            {
                done_.Set();
                return;
            }
            hr = stateReplicator_->GetReplicationStream(replicationStream_.InitializationAddress());
            if (FAILED(hr))
            {
                done_.Set();
                return;
            }

            StartOperationProcessing();
        }

        void StartOperationProcessing()
        {
            bool getNext = true;
            while (getNext)
            {
                getNext = ProcessNextOperation();
            }
        }

        bool ProcessNextOperation()
        {
            IFabricOperationStream* stream = NULL;
            if (isCopying_)
            {
                stream = copyStream_.GetRawPointer();
            }
            else
            {
                stream = replicationStream_.GetRawPointer();
            }

            ComPointer<IFabricAsyncOperationContext> context;
            HRESULT hr = stream->BeginGetOperation(this, context.InitializationAddress());

            if (FAILED(hr))
            {
                done_.Set();
                return false;
            }

            return FinishGetOperation(context.GetRawPointer(), TRUE);
        }

        bool FinishGetOperation(IFabricAsyncOperationContext* context, BOOLEAN expectCompletedSynchronously)
        {
            if (context->CompletedSynchronously() != expectCompletedSynchronously)
            {
                return false;
            }

            IFabricOperationStream* stream = NULL;
            if (isCopying_)
            {
                stream = copyStream_.GetRawPointer();
            }
            else
            {
                stream = replicationStream_.GetRawPointer();
            }

            ComPointer<IFabricOperation> operation;
            HRESULT hr = stream->EndGetOperation(context, operation.InitializationAddress());
            if (FAILED(hr))
            {
                done_.Set();
                return false;
            }

            if (NULL == operation.GetRawPointer())
            {
                if (isCopying_)
                {
                    isCopying_ = false;
                }
                else
                {
                    done_.Set();
                    return false;
                }
            }

            if (FALSE == expectCompletedSynchronously)
            {
                StartOperationProcessing();
            }

            return (TRUE == expectCompletedSynchronously);
        }

        void STDMETHODCALLTYPE Invoke(IFabricAsyncOperationContext *context)
        {
            FinishGetOperation(context, FALSE);
        }

        void Drain()
        {
            done_.WaitOne();
        }

    private:
        ComPointer<IFabricStateReplicator> stateReplicator_;
        ComPointer<IFabricOperationStream> copyStream_;
        ComPointer<IFabricOperationStream> replicationStream_;

        Common::ManualResetEvent done_;

        bool isCopying_;
    };
}
