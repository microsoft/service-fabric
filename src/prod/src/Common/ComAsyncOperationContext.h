// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ComAsyncOperationContext;

    typedef Common::ComPointer<ComAsyncOperationContext> ComAsyncOperationContextCPtr;

    class ComAsyncOperationContext
        : public IFabricAsyncOperationContext,
          protected ComUnknownBase
    {
        DENY_COPY(ComAsyncOperationContext)

        COM_INTERFACE_LIST1(
            ComAsyncOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext)

    public:
        ComAsyncOperationContext(bool skipCompleteOnCancel = false);
        virtual ~ComAsyncOperationContext();

        __declspec(property(get=get_Result)) HRESULT Result;

        HRESULT get_Result() const throw() { return result_.ToHResult(); }

        virtual BOOLEAN STDMETHODCALLTYPE IsCompleted();

        virtual BOOLEAN STDMETHODCALLTYPE CompletedSynchronously();

        virtual HRESULT STDMETHODCALLTYPE get_Callback(
            __out IFabricAsyncOperationCallback ** state);

        virtual HRESULT STDMETHODCALLTYPE Cancel();

        HRESULT STDMETHODCALLTYPE Initialize(
            __in ComponentRootSPtr const & rootSPtr,
            __in_opt IFabricAsyncOperationCallback * callback);

        HRESULT Start(
            ComAsyncOperationContextCPtr const & thisCPtr);

        HRESULT End();

        bool TryStartComplete();
        void FinishComplete(AsyncOperationSPtr const & proxySPtr, HRESULT result = S_OK);
        void FinishComplete(ComAsyncOperationContextCPtr const & thisCPtr, HRESULT result = S_OK);
        void FinishComplete(ComAsyncOperationContextCPtr const & thisCPtr, ErrorCode const &);
        bool TryComplete(AsyncOperationSPtr const & proxySPtr, HRESULT result = S_OK);
        bool TryComplete(AsyncOperationSPtr const & proxySPtr, ErrorCode const &);

        template <class TOperation>
        static HRESULT InitializeAndComplete(
            ErrorCode const & error,
            __in ComPointer<TOperation> && operationCPtr,
            __in_opt IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context)
        {
            ComponentRootSPtr rootSPtr;
            auto hr = operationCPtr->CreateComComponentRoot(rootSPtr);
            if (FAILED(hr)) { return hr; }

            ComAsyncOperationContextCPtr baseCPtr;
            baseCPtr.SetNoAddRef(operationCPtr.DetachNoRelease());

            hr = baseCPtr->Initialize(rootSPtr, callback);
            if (FAILED(hr)) { return hr; }

            baseCPtr->state_.TransitionStarted();

            auto proxySPtr = AsyncOperationRoot<ComAsyncOperationContextCPtr>::Create(baseCPtr);
            baseCPtr->TryComplete(proxySPtr, error);

            *context = baseCPtr.DetachNoRelease();
            return S_OK;
        }

        template <class TOperation>
        static HRESULT StartAndDetach(
            __in ComPointer<TOperation> && operation,
            __out IFabricAsyncOperationContext ** context)
        {
            if (context == NULL) { return E_POINTER; }

            ComAsyncOperationContextCPtr baseOperation;
            baseOperation.SetNoAddRef(operation.DetachNoRelease());
            HRESULT hr = baseOperation->Start(baseOperation);
            if (FAILED(hr)) { return hr; }

            *context = baseOperation.DetachNoRelease();
            return S_OK;
        }

    protected:
        virtual void OnStart(AsyncOperationSPtr const & proxySPtr) = 0;

        ComAsyncOperationContextCPtr const & GetCPtr(AsyncOperationSPtr const & proxySPtr);

    private:
        bool skipCompleteOnCancel_;
        AsyncOperationState state_;
        ComPointer<IFabricAsyncOperationCallback> callback_;
        Common::ErrorCode result_;
        Common::ExclusiveLock childLock_;
        std::weak_ptr<AsyncOperation> child_;
        ComponentRootSPtr rootSPtr_;

        void Cleanup();
        void CheckThisCPtr(ComAsyncOperationContextCPtr const & thisCPtr)
        {
            ASSERT_IF(
                thisCPtr.GetRawPointer() != this,
                "Incorrect thisCPtr used with ComAsyncOperationContext.");
        }
    };
}
