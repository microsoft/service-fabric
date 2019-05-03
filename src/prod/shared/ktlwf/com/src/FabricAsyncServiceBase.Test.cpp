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

    class FabricAsyncServiceBaseTest : public WEX::TestClass<FabricAsyncServiceBaseTest>
    {
    public:
        TEST_CLASS(FabricAsyncServiceBaseTest)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(Simple)
        TEST_METHOD(SimpleAsyncService)

    private:
        template <typename ServiceBaseType>
        static void CommonServiceTest();

    private:
        static KAllocator*  _Allocator;
        static KtlSystem*   _System;
    };

    template <typename BaseClass = FabricAsyncServiceBase>
    class TestAsyncService : public BaseClass
    {
        friend FabricAsyncServiceBaseTest;

        K_FORCE_SHARED(TestAsyncService)

        HRESULT StartTestOpen(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncServiceBase::OpenCompletionCallback CallbackPtr);

        HRESULT StartTestClose(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncServiceBase::CloseCompletionCallback CallbackPtr);

        void OnServiceOpen() override;
        void OnServiceClose() override;
    };

    template <typename BaseClass>
    HRESULT TestAsyncService<BaseClass>::StartTestOpen(
        __in_opt KAsyncContextBase* const parentAsyncContext,
        __in_opt KAsyncServiceBase::OpenCompletionCallback callbackPtr)
    {
        NTSTATUS status = StartOpen(parentAsyncContext, callbackPtr);
        return KComUtility::ToHRESULT(status);
    }

    template <typename BaseClass>
    HRESULT TestAsyncService<BaseClass>::StartTestClose(
        __in_opt KAsyncContextBase* const parentAsyncContext,
        __in_opt KAsyncServiceBase::OpenCompletionCallback callbackPtr)
    {
        NTSTATUS status = StartClose(parentAsyncContext, callbackPtr);
        return KComUtility::ToHRESULT(status);
    }

    template <typename BaseClass>
    TestAsyncService<BaseClass>::TestAsyncService()
    {
    }

    template <typename BaseClass>
    TestAsyncService<BaseClass>::~TestAsyncService()
    {
    }

    template <>
    void TestAsyncService<KAsyncServiceBase>::OnServiceOpen()
    {
        CompleteOpen(STATUS_SUCCESS);
    }

    template <typename BaseClass>
    void TestAsyncService<BaseClass>::OnServiceOpen()
    {
        CompleteOpen(S_OK);
    }

    template <>
    void TestAsyncService<KAsyncServiceBase>::OnServiceClose()
    {
        CompleteClose(STATUS_SUCCESS);
    }

    template <typename BaseClass>
    void TestAsyncService<BaseClass>::OnServiceClose()
    {
        CompleteClose(S_OK);
    }




    class TestServiceCallback : public KShared<TestServiceCallback>,
                                public KObject<TestServiceCallback>,
                                public IFabricAsyncOperationCallback
    {
        friend FabricAsyncServiceBaseTest;

        K_FORCE_SHARED(TestServiceCallback)

        K_BEGIN_COM_INTERFACE_LIST(TestServiceCallback)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
            COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        K_END_COM_INTERFACE_LIST()

    public:
        void STDMETHODCALLTYPE Invoke(__in IFabricAsyncOperationContext * context);

    private:
        BOOLEAN _invoked;
    };

    TestServiceCallback::TestServiceCallback() :
        _invoked(FALSE)
    {
    }

    TestServiceCallback::~TestServiceCallback()
    {
    }

    void STDMETHODCALLTYPE TestServiceCallback::Invoke(__in IFabricAsyncOperationContext * context)
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


    KAllocator* FabricAsyncServiceBaseTest::_Allocator = nullptr;
    KtlSystem* FabricAsyncServiceBaseTest::_System = nullptr;

    bool FabricAsyncServiceBaseTest::Setup()
    {
        NTSTATUS status = STATUS_SUCCESS;

        status = KtlSystem::Initialize(&_System);
        _System->SetStrictAllocationChecks(TRUE);

        if (!NT_SUCCESS(status))
        {
            return false;
        }

        _Allocator = &KtlSystem::GlobalPagedAllocator();

        return true;
    }

    bool FabricAsyncServiceBaseTest::Cleanup()
    {
        KtlSystem::Shutdown();

        return true;
    } 

    template <typename ServiceBaseType>
    void FabricAsyncServiceBaseTest::CommonServiceTest()
    {
        TestAsyncService<ServiceBaseType>::SPtr service;
        service = _new(COMMON_ALLOCATION_TAG, *_Allocator) TestAsyncService<ServiceBaseType>();
        VERIFY_IS_NOT_NULL(service.RawPtr());
        VERIFY_IS_TRUE(TRUE == NT_SUCCESS(service->Status()));

        // Test Service Open 
        {
            IFabricAsyncOperationCallback* callbackRaw = _new (COMMON_ALLOCATION_TAG, *_Allocator) TestServiceCallback();

            Common::ComPointer<IFabricAsyncOperationCallback> callback;
            callback.SetAndAddRef(callbackRaw);

            Common::ComPointer<IFabricAsyncOperationContext> context;

            HRESULT hr = ServiceOpenCloseAdapter::StartOpen(
                *_Allocator,
                service,
                callback.GetRawPointer(),
                context.InitializationAddress(),
                [&] (TestAsyncService<ServiceBaseType>& s, KAsyncServiceBase::OpenCompletionCallback c) -> HRESULT
                {
                    return s.StartTestOpen(nullptr, c);
                });

            VERIFY_SUCCEEDED(hr, L"Create and start open");
            VERIFY_IS_NOT_NULL(context.GetRawPointer());

            // Prove EndOpen call
            TestAsyncService<ServiceBaseType>::SPtr openEnd;

            hr = ServiceOpenCloseAdapter::EndOpen(context.GetRawPointer(), openEnd);
            VERIFY_SUCCEEDED(hr, L"EndOpen");
        }

        // Test Service Close
        {
            IFabricAsyncOperationCallback* callbackRaw = _new (COMMON_ALLOCATION_TAG, *_Allocator) TestServiceCallback();

            Common::ComPointer<IFabricAsyncOperationCallback> callback;
            callback.SetAndAddRef(callbackRaw);

            Common::ComPointer<IFabricAsyncOperationContext> context;

            HRESULT hr = ServiceOpenCloseAdapter::StartClose(
                *_Allocator,
                service,
                callback.GetRawPointer(),
                context.InitializationAddress(),
                [&] (TestAsyncService<ServiceBaseType>& s, KAsyncServiceBase::OpenCompletionCallback c) -> HRESULT
                {
                    return s.StartTestClose(nullptr, c);
                });

            VERIFY_SUCCEEDED(hr, L"Create and start close");
            VERIFY_IS_NOT_NULL(context.GetRawPointer());

            // Prove EndOpen
            TestAsyncService<ServiceBaseType>::SPtr closeEnd;

            hr = ServiceOpenCloseAdapter::EndClose(context.GetRawPointer(), closeEnd);
            VERIFY_SUCCEEDED(hr, L"EndClose");
        }

        // Test Service Reopen
        service->Reuse();
        {
            IFabricAsyncOperationCallback* callbackRaw = _new (COMMON_ALLOCATION_TAG, *_Allocator) TestServiceCallback();

            Common::ComPointer<IFabricAsyncOperationCallback> callback;
            callback.SetAndAddRef(callbackRaw);

            Common::ComPointer<IFabricAsyncOperationContext> context;

            HRESULT hr = ServiceOpenCloseAdapter::StartOpen(
                *_Allocator,
                service,
                callback.GetRawPointer(),
                context.InitializationAddress(),
                [&] (TestAsyncService<ServiceBaseType>& s, KAsyncServiceBase::OpenCompletionCallback c) -> HRESULT
                {
                    return s.StartTestOpen(nullptr, c);
                });

            VERIFY_SUCCEEDED(hr, L"Create and start open");
            VERIFY_IS_NOT_NULL(context.GetRawPointer());

            // Prove EndOpen call
            TestAsyncService<ServiceBaseType>::SPtr openEnd;

            hr = ServiceOpenCloseAdapter::EndOpen(context.GetRawPointer(), openEnd);
            VERIFY_SUCCEEDED(hr, L"EndOpen");
        }

        // Test Service Close after reopen
        {
            IFabricAsyncOperationCallback* callbackRaw = _new (COMMON_ALLOCATION_TAG, *_Allocator) TestServiceCallback();

            Common::ComPointer<IFabricAsyncOperationCallback> callback;
            callback.SetAndAddRef(callbackRaw);

            Common::ComPointer<IFabricAsyncOperationContext> context;

            HRESULT hr = ServiceOpenCloseAdapter::StartClose(
                *_Allocator,
                service,
                callback.GetRawPointer(),
                context.InitializationAddress(),
                [&] (TestAsyncService<ServiceBaseType>& s, KAsyncServiceBase::OpenCompletionCallback c) -> HRESULT
                {
                    return s.StartTestClose(nullptr, c);
                });

            VERIFY_SUCCEEDED(hr, L"Create and start close");
            VERIFY_IS_NOT_NULL(context.GetRawPointer());

            // Prove EndOpen
            TestAsyncService<ServiceBaseType>::SPtr closeEnd;

            hr = ServiceOpenCloseAdapter::EndClose(context.GetRawPointer(), closeEnd);
            VERIFY_SUCCEEDED(hr, L"EndClose");
        }
    }

    void FabricAsyncServiceBaseTest::Simple()
    {
        FabricAsyncServiceBaseTest::CommonServiceTest<FabricAsyncServiceBase>();
    }


    void FabricAsyncServiceBaseTest::SimpleAsyncService()
    {
        FabricAsyncServiceBaseTest::CommonServiceTest<KAsyncServiceBase>();
    }
}

