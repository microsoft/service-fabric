// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#if KTL_USER_MODE

#include "FabricAsyncContextBase.h"

namespace Ktl
{
    namespace Com
    {
        using ::_delete;

        //
        // This class is used to invoke Begin/End based async operations from KAsyncContextBase based async operation.
        //
        // This class must be inherited. There must be one derivation for every Begin/End pair on an interface.
        //
        // Calling Begin:
        //
        // Derivations must implement a suitable StartMethod that invokes the Begin method using this as callback pointer. 
        // If Begin succeeds, OnComOperationStarted must be called.
        //
        // HRESULT StartMyOperation(
        //     __in_opt KAsyncContextBase* const parentAsyncContext,
        //     __in_opt KAsyncContextBase::CompletionCallback callbackPtr)
        // {
        //     ComPointer<IFabricAsyncOperationContext> context;
        //     HRESULT hr = _myInterface->BeginMyOperation(this, context.InitializationAddress());
        //     if (SUCCEEDED(hr))
        //     {
        //         OnComOperationStarted(context.GetRawPointer());
        //     }
        //
        //     return hr;
        // }
        //
        // Calling End:
        //
        // Derivations must implement OnEnd and invoke the corresponding End method.
        //
        // HRESULT OnEnd(IFabricAsyncOperationContext& Context)
        // {
        //     return _myInterface->EndMyOperation(Context);
        // }
        //
        template <class OperationToImplement>
        class AsyncCallOutAdapter abstract : public OperationToImplement,
                                             public IFabricAsyncOperationCallback
        {
            // OperationToImplement MUST be-a KAsyncContextBase
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncCallOutAdapter)

            K_BEGIN_COM_INTERFACE_LIST(AsyncCallOutAdapter<OperationToImplement>)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
                COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
            K_END_COM_INTERFACE_LIST()

        protected:
            // NOTE: Must be called by derivation Start*() methods ince the underlying COM op has been
            //       successfully started
            void OnComOperationStarted(
                __in IFabricAsyncOperationContext& Context,
                __in_opt KAsyncContextBase *const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            // Derivations must implement this method and call End
            virtual HRESULT OnEnd(__in IFabricAsyncOperationContext& Context) = 0;

            VOID OnReuse() override;

        private:
            void STDMETHODCALLTYPE Invoke(__in IFabricAsyncOperationContext *context) override;
            VOID OnStart() override;
            VOID OnCancel() override;

            void TryCompleteOperation(
                __in IFabricAsyncOperationContext& Context, 
                __in BOOLEAN ExpectCompletedSynchronously);

            void TryCompleteOperation(__in IFabricAsyncOperationContext& Context);

        private:
            Common::ComPointer<IFabricAsyncOperationContext>    _Context;
            ULONG                                               _State;
        };
    }
}


//*** AsyncCallOutAdapter Implementation ***
namespace Ktl
{
    namespace Com
    {      
        template <class OperationToImplement>
        AsyncCallOutAdapter<OperationToImplement>::AsyncCallOutAdapter() 
            :   _State(0)
        {
        }

        template <class OperationToImplement>
        AsyncCallOutAdapter<OperationToImplement>::~AsyncCallOutAdapter()
        {
        }

        template <class OperationToImplement>
        void STDMETHODCALLTYPE 
        AsyncCallOutAdapter<OperationToImplement>::Invoke(__in IFabricAsyncOperationContext* Context)
        {
            // Callback from COM operation. Attempt to complete the operation.
            TryCompleteOperation(*Context, FALSE);
        }

        template <class OperationToImplement>
        void 
        AsyncCallOutAdapter<OperationToImplement>::OnComOperationStarted(
            __in IFabricAsyncOperationContext& Context,
            __in_opt KAsyncContextBase *const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
        {
            _Context.SetAndAddRef(&Context);

            // Start the KAsyncContext state machine
            this->Start(ParentAsyncContext, CallbackPtr);

            // The COM operation may complete synchronously.
            TryCompleteOperation(Context, TRUE);
        }

        template <class OperationToImplement>
        VOID 
        AsyncCallOutAdapter<OperationToImplement>::OnReuse()
        {
            _Context.Release();
            _State = 0;

            __super::OnReuse();
        }

        template <class OperationToImplement>
        VOID 
        AsyncCallOutAdapter<OperationToImplement>::OnStart()
        {
            ASSERT_IF(nullptr == _Context.GetRawPointer(), "context not set");

            // The KAsyncContextBase state machine has started, attempt to complete the operation.
            TryCompleteOperation(*_Context.GetRawPointer());
        }

        template <class OperationToImplement>
        VOID 
        AsyncCallOutAdapter<OperationToImplement>::OnCancel()
        {
            _Context->Cancel();
        }

        template <class OperationToImplement>
        void 
        AsyncCallOutAdapter<OperationToImplement>::TryCompleteOperation(
            __in IFabricAsyncOperationContext& Context, 
            __in BOOLEAN ExpectCompletedSynchronously)
        {
            if (Context.CompletedSynchronously() != ExpectCompletedSynchronously)
            {
                return;
            }

            this->TryCompleteOperation(Context);
        }

        template <class OperationToImplement>
        void 
        AsyncCallOutAdapter<OperationToImplement>::TryCompleteOperation(__in IFabricAsyncOperationContext& Context)
        {
            // Two conditions must hold for the operation to be completed:
            //   - the COM operation has completed
            //   - the KAsyncContextBase has been started
            // In either case TryCompleteOperation is called exactly once. 
            // Thus, when the state reaches 2 the operation can complete.

            if (::InterlockedIncrement((volatile LONG*)&_State) != 2)
            {
                return;
            }

            this->Complete(OnEnd(Context));
        }
    }
}

#endif
