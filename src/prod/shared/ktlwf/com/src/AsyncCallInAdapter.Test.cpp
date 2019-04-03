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

#define VERIFY_REF_COUNT(object, expected) \
    object->AddRef(); \
    VERIFY_ARE_EQUAL((LONG)expected, (LONG)(object->Release()), L""  L## #object L"(" L ## #expected L")");

    class AsyncCallInAdapterTest : public WEX::TestClass<AsyncCallInAdapterTest>
    {
    public:
        TEST_CLASS(AsyncCallInAdapterTest)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(Simple)
        TEST_METHOD(SimpleKAsyncBaseTest)
        TEST_METHOD(AdaptorExtensionTest)

    private:

        static KAllocator* _allocator;
        static KtlSystem* _system;
    };

    class TestAsyncOperation : public FabricAsyncContextBase
    {
        friend AsyncCallInAdapterTest;

        K_FORCE_SHARED(TestAsyncOperation)

         HRESULT StartOperation(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    private:

        VOID OnStart();

        VOID OnReuse();
    };

    HRESULT TestAsyncOperation::StartOperation(
        __in_opt KAsyncContextBase* const parentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback callbackPtr)
    {
        Start(parentAsyncContext, callbackPtr);

        return S_OK;
    }

    TestAsyncOperation::TestAsyncOperation()
    {
    }

    TestAsyncOperation::~TestAsyncOperation()
    {
    }

    void TestAsyncOperation::OnStart()
    {
        Complete(STATUS_SUCCESS);
    }

    void TestAsyncOperation::OnReuse()
    {
    }

    class TestAsyncCallback : public KShared<TestAsyncCallback>,
                              public KObject<TestAsyncCallback>,
                              public IFabricAsyncOperationCallback
    {
        friend AsyncCallInAdapterTest;

        K_FORCE_SHARED(TestAsyncCallback)

        K_BEGIN_COM_INTERFACE_LIST(TestAsyncCallback)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
            COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        K_END_COM_INTERFACE_LIST()

    public:

        void STDMETHODCALLTYPE Invoke(__in IFabricAsyncOperationContext * context);

    private:
        BOOLEAN _invoked;
    };

    TestAsyncCallback::TestAsyncCallback() :
        _invoked(FALSE)
    {
    }

    TestAsyncCallback::~TestAsyncCallback()
    {
    }

    void STDMETHODCALLTYPE TestAsyncCallback::Invoke(__in IFabricAsyncOperationContext * context)
    {
        ASSERT_IF(nullptr == context, "context is null");
        ComPointer<IFabricAsyncOperationContext> qi;
        HRESULT hr = context->QueryInterface(IID_IFabricAsyncOperationContext, qi.VoidInitializationAddress());

        IFabricAsyncOperationContext * c2 = qi.GetRawPointer();

        ASSERT_IFNOT(c2 == context, "context is not is IID_IFabricAsyncOperationContext");

        ASSERT_IF(TRUE == FAILED(hr), "QI failed");

        _invoked = TRUE;
        context->CompletedSynchronously();
    }

    KAllocator* AsyncCallInAdapterTest::_allocator = nullptr;
    KtlSystem* AsyncCallInAdapterTest::_system = nullptr;

    bool AsyncCallInAdapterTest::Setup()
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

    bool AsyncCallInAdapterTest::Cleanup()
    {
        KtlSystem::Shutdown();

        return true;
    } 

    void AsyncCallInAdapterTest::Simple()
    {
        TestAsyncOperation::SPtr operation = _new(COMMON_ALLOCATION_TAG, *_allocator) TestAsyncOperation();
        VERIFY_IS_NOT_NULL(operation.RawPtr());
        VERIFY_IS_TRUE(TRUE == NT_SUCCESS(operation->Status()));

        TestAsyncOperation::SPtr operationBegin = operation;

        IFabricAsyncOperationCallback * callbackRaw = _new (COMMON_ALLOCATION_TAG, *_allocator) TestAsyncCallback();

        //
        // Begin call
        //
        Common::ComPointer<IFabricAsyncOperationCallback> callback;
        callback.SetAndAddRef(callbackRaw);

        Common::ComPointer<IFabricAsyncOperationContext> context;

        HRESULT hr = AsyncCallInAdapter::CreateAndStart(
            *_allocator,
            Ktl::Move(operation),
            callback.GetRawPointer(),
            context.InitializationAddress(),
            [&] (AsyncCallInAdapter&, TestAsyncOperation& o, KAsyncContextBase::CompletionCallback c) -> HRESULT
            {
                return o.StartOperation(nullptr, c);
            });

        VERIFY_SUCCEEDED(hr, L"Create and start");
        VERIFY_IS_NOT_NULL(context.GetRawPointer());

        //
        // End call
        //
        TestAsyncOperation::SPtr operationEnd;

        hr = AsyncCallInAdapter::End(context.GetRawPointer(), operationEnd);
        VERIFY_SUCCEEDED(hr, L"End");
    }

    //** Test for direct derivation of KAsyncContextBase
    class TestKAsyncOperation : public KAsyncContextBase
    {
        friend AsyncCallInAdapterTest;

        K_FORCE_SHARED(TestKAsyncOperation)

         NTSTATUS StartOperation(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
        {
            Start(ParentAsyncContext, CallbackPtr);
            return STATUS_SUCCESS;
        }
    };

    TestKAsyncOperation::TestKAsyncOperation()
    {
    }

    TestKAsyncOperation::~TestKAsyncOperation()
    {
    }


    void AsyncCallInAdapterTest::SimpleKAsyncBaseTest()
    {
        TestKAsyncOperation::SPtr operation = _new(COMMON_ALLOCATION_TAG, *_allocator) TestKAsyncOperation();
        VERIFY_IS_NOT_NULL(operation.RawPtr());
        VERIFY_IS_TRUE(TRUE == NT_SUCCESS(operation->Status()));

        TestKAsyncOperation::SPtr operationBegin = operation;

        IFabricAsyncOperationCallback * callbackRaw = _new (COMMON_ALLOCATION_TAG, *_allocator) TestAsyncCallback();

        //
        // Begin call
        //
        Common::ComPointer<IFabricAsyncOperationCallback> callback;
        callback.SetAndAddRef(callbackRaw);

        Common::ComPointer<IFabricAsyncOperationContext> context;

        HRESULT hr = AsyncCallInAdapter::CreateAndStart(
            *_allocator,
            Ktl::Move(operation),
            callback.GetRawPointer(),
            context.InitializationAddress(),
            [&] (AsyncCallInAdapter&, TestKAsyncOperation& o, KAsyncContextBase::CompletionCallback c) -> NTSTATUS
            {
                return o.StartOperation(nullptr, c);
            });

        VERIFY_SUCCEEDED(hr, L"Create and start");
        VERIFY_IS_NOT_NULL(context.GetRawPointer());

        // End call
        TestKAsyncOperation::SPtr operationEnd;

        hr = AsyncCallInAdapter::End(context.GetRawPointer(), operationEnd);
        VERIFY_SUCCEEDED(hr, L"End");
    }

    void AsyncCallInAdapterTest::AdaptorExtensionTest()
    {
        TestKAsyncOperation::SPtr operation = _new(COMMON_ALLOCATION_TAG, *_allocator) TestKAsyncOperation();
        VERIFY_IS_NOT_NULL(operation.RawPtr());
        VERIFY_IS_TRUE(TRUE == NT_SUCCESS(operation->Status()));

        TestKAsyncOperation::SPtr operationBegin = operation;

        //
        // Begin call
        //
        Common::ComPointer<IFabricAsyncOperationContext>    context;
        LONGLONG                                            timestamp = 0;
        AsyncCallInAdapter::Extended<LONGLONG>::SPtr        myAdaptorExt;

        HRESULT hr = AsyncCallInAdapter::CreateAndStart(
            *_allocator,
            Ktl::Move(operation),
            nullptr,
            context.InitializationAddress(),
            [&] (AsyncCallInAdapter& a, TestKAsyncOperation& o, KAsyncContextBase::CompletionCallback c) -> NTSTATUS
            {
                VERIFY_IS_TRUE(&a == myAdaptorExt.RawPtr());
                return o.StartOperation(nullptr, c);
            },
            [&] (__in KAllocator& Allocator, __out AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
            {
                NTSTATUS status = AsyncCallInAdapter::Extended<LONGLONG>::Create(Allocator, Adapter);
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                myAdaptorExt = (AsyncCallInAdapter::Extended<LONGLONG>*)Adapter.RawPtr();
                timestamp = KNt::GetPerformanceTime();
                myAdaptorExt->Extension = timestamp;
                return STATUS_SUCCESS;
            });

        VERIFY_SUCCEEDED(hr, L"Create and start");
        VERIFY_IS_NOT_NULL(context.GetRawPointer());

        // End call
        TestKAsyncOperation::SPtr operationEnd;

        hr = AsyncCallInAdapter::End(context.GetRawPointer(), operationEnd);
        VERIFY_SUCCEEDED(hr, L"End");
        VERIFY_IS_TRUE(((AsyncCallInAdapter::Extended<LONGLONG>*)(context.GetRawPointer()))->Extension == timestamp);
    }
}
