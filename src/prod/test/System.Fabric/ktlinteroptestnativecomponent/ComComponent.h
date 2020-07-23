// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace KtlInteropTest
{
    using ::_delete;

    class ComComponent : public KShared<ComComponent>,
                         public KObject<ComComponent>,
                         public IKtlInteropTestComponent
    {
        K_FORCE_SHARED(ComComponent)

        K_BEGIN_COM_INTERFACE_LIST(ComComponent)
            COM_INTERFACE_ITEM(IID_IUnknown, IKtlInteropTestComponent)
            COM_INTERFACE_ITEM(IID_IKtlInteropTestComponent, IKtlInteropTestComponent)
        K_END_COM_INTERFACE_LIST()

    public:

        static HRESULT Create(
            __in KAllocator & allocator,
            __out ComPointer<IKtlInteropTestComponent> & component);

        STDMETHODIMP BeginOperation( 
            __in HRESULT beginResult,
            __in HRESULT endResult,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        STDMETHODIMP EndOperation( 
            __in IFabricAsyncOperationContext *context);

    private:

        class AsyncOperationContext : public Ktl::Com::FabricAsyncContextBase
        {
            friend ComComponent;

            K_FORCE_SHARED(AsyncOperationContext)

        public:

            ErrorCode StartOperation(
                __in HRESULT beginResult,
                __in HRESULT endResult,
                __in_opt KAsyncContextBase* const parent,
                __in_opt CompletionCallback callback);

        protected:

            void OnStart();

            void OnReuse();

        private:

            ComComponent::SPtr _owner;

            HRESULT _endResult;
        };
    };
};
