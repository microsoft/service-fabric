// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "WexTestClass.h"
#include "KComAdapter.h"

using namespace Ktl::Com;

namespace ApiTest
{
using ::_delete;

    class AsyncCallOutAdapterTest : public WEX::TestClass<AsyncCallOutAdapterTest>
    {
    public:
        TEST_CLASS(AsyncCallOutAdapterTest)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(Simple)
        TEST_METHOD(SimpleKAsync)

    private:

        static KAllocator* _allocator;
        static KtlSystem* _system;
    };

    KAllocator* AsyncCallOutAdapterTest::_allocator = nullptr;
    KtlSystem* AsyncCallOutAdapterTest::_system = nullptr;

    class ComOperation : public KShared<ComOperation>,
                         public KObject<ComOperation>,
                         public IFabricAsyncOperationContext,
                         public KThreadPool::WorkItem
    {
        K_FORCE_SHARED(ComOperation)

        K_BEGIN_COM_INTERFACE_LIST(ComOperation)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationContext)
            COM_INTERFACE_ITEM(IID_IFabricAsyncOperationContext, IFabricAsyncOperationContext)
        K_END_COM_INTERFACE_LIST()

    public:

        static ComOperation::SPtr CreateAndStart(
            __in KAllocator & allocator,
            __in IFabricAsyncOperationCallback * callback,
            __in BOOLEAN completedSynchronously);

        VOID Execute() override;

        //
        // IFabricAsyncOperationContext implementation
        //
        BOOLEAN STDMETHODCALLTYPE 
        IFabricAsyncOperationContext::IsCompleted();

        BOOLEAN STDMETHODCALLTYPE 
        IFabricAsyncOperationContext::CompletedSynchronously();

        HRESULT STDMETHODCALLTYPE 
        IFabricAsyncOperationContext::get_Callback( 
            __out IFabricAsyncOperationCallback **callback);

        HRESULT STDMETHODCALLTYPE 
        IFabricAsyncOperationContext::Cancel();

    private:

        Common::ComPointer<IFabricAsyncOperationCallback> _callback;

        BOOLEAN _completeSynchronously;
        BOOLEAN _completed;
    };

    ComOperation::ComOperation() :
        _completed(FALSE),
        _completeSynchronously(FALSE)
    {
    }

    ComOperation::~ComOperation()
    {
    }

    ComOperation::SPtr ComOperation::CreateAndStart(
        __in KAllocator & allocator,
        __in IFabricAsyncOperationCallback * callback, 
        __in BOOLEAN completedSynchronously)
    {
        ComOperation::SPtr operation = _new(COMMON_ALLOCATION_TAG, allocator) ComOperation();
        VERIFY_IS_NOT_NULL(operation.RawPtr(), L"ComOperation::ComOperation");
        VERIFY_IS_TRUE(TRUE == NT_SUCCESS(operation->Status()), L"ComOperation::Status");

        operation->_completeSynchronously = completedSynchronously;
        operation->_callback.SetAndAddRef(callback);

        operation->AddRef();

        if (completedSynchronously)
        {
            operation->Execute();
        }
        else
        {
            operation->GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*operation);
        }

        return operation;
    }

    VOID ComOperation::Execute()
    {
        _completed = TRUE;
         
        _callback->Invoke(this);

        this->Release();
    }

    BOOLEAN STDMETHODCALLTYPE ComOperation::IsCompleted()
    {
        return _completed;
    }

    BOOLEAN STDMETHODCALLTYPE ComOperation::CompletedSynchronously()
    {
        return _completed && _completeSynchronously;
    }

    HRESULT STDMETHODCALLTYPE ComOperation::get_Callback( 
        __out IFabricAsyncOperationCallback **callback)
    {
        *callback = _callback.GetRawPointer();
        _callback->AddRef();

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ComOperation::Cancel()
    {
        return S_FALSE;
    }


    template<class BaseType>
    class OperationDefinition abstract : public BaseType
    {
        K_FORCE_SHARED_WITH_INHERITANCE(OperationDefinition);

    public: 

        virtual HRESULT StartOperation(
            __in BOOLEAN completeSynchronously, 
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
    };

    template<class BaseType>
    OperationDefinition<BaseType>::OperationDefinition()
    {
    }

    template<class BaseType>
    OperationDefinition<BaseType>::~OperationDefinition()
    {
    }


    template<class BaseType>
    class ComProxyOperation : public AsyncCallOutAdapter<OperationDefinition<BaseType>>
    {
        friend AsyncCallOutAdapterTest;

        K_FORCE_SHARED(ComProxyOperation);

    public:
        HRESULT StartOperation(
            __in BOOLEAN completeSynchronously, 
            __in_opt KAsyncContextBase* const parentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback callbackPtr) override;

        HRESULT OnEnd(__in IFabricAsyncOperationContext& context) override;

    protected:

        VOID OnReuse();
    };

    template<class BaseType>
    ComProxyOperation<BaseType>::ComProxyOperation()
    {
    }

    template<class BaseType>
    ComProxyOperation<BaseType>::~ComProxyOperation()
    {
    }

    template<class BaseType>
    HRESULT ComProxyOperation<BaseType>::StartOperation(
        __in BOOLEAN completeSynchronously, 
        __in_opt KAsyncContextBase* const parentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback callbackPtr)
    {
        ComOperation::SPtr operation = ComOperation::CreateAndStart(GetThisAllocator(), this, completeSynchronously);

        OnComOperationStarted(*operation, parentAsyncContext, callbackPtr);

        return S_OK;
    }

    template<class BaseType>
    HRESULT ComProxyOperation<BaseType>::OnEnd(__in IFabricAsyncOperationContext& context)
    {
        VERIFY_IS_TRUE(TRUE == context.IsCompleted());

        return S_OK;
    }

    template<class BaseType>
    VOID ComProxyOperation<BaseType>::OnReuse()
    {
        AsyncCallOutAdapter<OperationDefinition>::OnReuse();
    }

    bool AsyncCallOutAdapterTest::Setup()
    {
        NTSTATUS status = STATUS_SUCCESS;

        status = KtlSystem::Initialize(&_system);
        _system->SetStrictAllocationChecks(TRUE);

        if (!NT_SUCCESS(status))
        {
            return false;
        }

        _allocator = &KtlSystem::GlobalPagedAllocator();

        return true;
    }

    bool AsyncCallOutAdapterTest::Cleanup()
    {
        KtlSystem::Shutdown();

        return true;
    } 

    void AsyncCallOutAdapterTest::Simple()
    {
        ComProxyOperation<FabricAsyncContextBase>::SPtr operation;
        operation = _new(COMMON_ALLOCATION_TAG, *_allocator) ComProxyOperation<FabricAsyncContextBase>();
        VERIFY_IS_NOT_NULL(operation.RawPtr(), L"ComProxyOperation::ComProxyOperation");
        VERIFY_IS_TRUE(TRUE == NT_SUCCESS(operation->Status()), L"ComProxyOperation::Status");

        KSynchronizer sync;

        HRESULT hr = operation->StartOperation(TRUE, nullptr, sync);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        NTSTATUS status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(TRUE == NT_SUCCESS(status));

        VERIFY_IS_TRUE(SUCCEEDED(operation->Result()));

        operation->Reuse();

        // Prove Async completion works
        hr = operation->StartOperation(TRUE, nullptr, sync);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(TRUE == NT_SUCCESS(status));

        VERIFY_IS_TRUE(SUCCEEDED(operation->Result()));

        operation->Reuse();
    }

    // prove direct deriavtion from KAsyncContextBase works
    void AsyncCallOutAdapterTest::SimpleKAsync()
    {
        ComProxyOperation<KAsyncContextBase>::SPtr operation;
        operation = _new(COMMON_ALLOCATION_TAG, *_allocator) ComProxyOperation<KAsyncContextBase>();
        VERIFY_IS_NOT_NULL(operation.RawPtr(), L"ComProxyOperation::ComProxyOperation");
        VERIFY_IS_TRUE(TRUE == NT_SUCCESS(operation->Status()), L"ComProxyOperation::Status");

        KSynchronizer sync;

        HRESULT hr = operation->StartOperation(TRUE, nullptr, sync);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        NTSTATUS status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(TRUE == NT_SUCCESS(status));

        VERIFY_IS_TRUE(NT_SUCCESS(operation->Status()));

        operation->Reuse();

        // Prove async completion
        hr = operation->StartOperation(FALSE, nullptr, sync);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(TRUE == NT_SUCCESS(status));

        VERIFY_IS_TRUE(NT_SUCCESS(operation->Status()));

        operation->Reuse();
    }
}
