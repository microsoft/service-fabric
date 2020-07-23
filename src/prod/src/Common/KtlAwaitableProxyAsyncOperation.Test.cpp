// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/AssertWF.h"

#include <KTpl.h>

using namespace std;

namespace Common
{
    class FabricKtlAwaitableServerWrapper;

    class KtlAwaitableProxyTest
    {
    };

    class KtlAwaitableTestServer : public KAsyncContextBase // KTL Async Operation
    {
        friend FabricKtlAwaitableServerWrapper;
        K_FORCE_SHARED(KtlAwaitableTestServer)

    public:

        KtlAwaitableTestServer(__in bool shouldThrow);

        ktl::Awaitable<NTSTATUS>
        ExecuteServerAsync(__in KAsyncContextBase* const ParentAsync = nullptr)
        {
            if (shouldThrow_)
            {
                throw ktl::Exception(STATUS_NOT_IMPLEMENTED);
            }
            else
            {
                ktl::kasync::StartAwaiter::SPtr startAwaiter;
                NTSTATUS status = ktl::kasync::StartAwaiter::Create(GetThisAllocator(), GetThisAllocationTag(), *this, startAwaiter, ParentAsync);

                if (!NT_SUCCESS(status))
                {
                    co_return status;
                }

                status = co_await *startAwaiter;
                co_return status;
            }
        }

    private:

        void OnStart()
        {
            Complete(STATUS_INTERNAL_ERROR);
        }

        bool shouldThrow_;
    };

    KtlAwaitableTestServer::KtlAwaitableTestServer(__in bool shouldThrow)
        : shouldThrow_(shouldThrow)
    {
    }

    KtlAwaitableTestServer::~KtlAwaitableTestServer() {}

    class FabricKtlAwaitableServerWrapper  // Fabric operation corresponding to the KTL operation
    {

    public:

        FabricKtlAwaitableServerWrapper(__in KAllocator &allocator, __in bool shouldThrow)
        {
            KtlAwaitableServer_ = _new(KTL_TAG_TEST, allocator) KtlAwaitableTestServer(shouldThrow);
        }

        AsyncOperationSPtr BeginStart(
            __in AsyncCallback const & callback,
            __in AsyncOperationSPtr const & parent)
        {
            return AllocableAsyncOperation::CreateAndStart<FabricKtlAwaitableServerOp>(
                KtlAwaitableServer_->GetThisAllocator(),
                *KtlAwaitableServer_,
                callback,
                parent);
        }

        ErrorCode EndStart(
            __in Common::AsyncOperationSPtr const& operation)
        {
            return FabricKtlAwaitableServerOp::End(operation);
        }

    private:

        KtlAwaitableTestServer::SPtr KtlAwaitableServer_;

        class FabricKtlAwaitableServerOp : public KtlAwaitableProxyAsyncOperation
        {
        public:
            FabricKtlAwaitableServerOp(
                KtlAwaitableTestServer& server,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent)
                : KtlAwaitableProxyAsyncOperation(callback, parent)
                , server_(&server)
            {
            }

            static ErrorCode End(AsyncOperationSPtr const& operation)
            {
                auto op = AsyncOperation::End<FabricKtlAwaitableServerOp>(operation);
                return op->Error;
            }

            ktl::Awaitable<NTSTATUS>
            ExecuteOperationAsync()
            {
                NTSTATUS status = co_await server_->ExecuteServerAsync();
                co_return status;
            }
            
            KtlAwaitableTestServer::SPtr server_;
        };
    };

    BOOST_FIXTURE_TEST_SUITE(KtlAwaitableProxyTestSuite, KtlAwaitableProxyTest)

    BOOST_AUTO_TEST_CASE(TestOperation_NoThrow)
    {
        ManualResetEvent event;
        ErrorCode err;
        shared_ptr<FabricKtlAwaitableServerWrapper> fabricServerSPtr =
            make_shared<FabricKtlAwaitableServerWrapper>(Common::GetSFDefaultKtlSystem().PagedAllocator(), false);

        VERIFY_IS_TRUE(fabricServerSPtr);

        //
        // Not storing the AsyncOperationSPtr on stack, this makes sure that the fabric async operation manages
        // its own reference till the internal KTL async operation completes.
        //
        fabricServerSPtr->BeginStart
            ([&fabricServerSPtr, &event, &err](AsyncOperationSPtr const& operation)
            {
                err = fabricServerSPtr->EndStart(operation);
                event.Set();
            },
            AsyncOperationSPtr());

        event.WaitOne();

        VERIFY_IS_TRUE(err.ToHResult() == HRESULT_FROM_NT(STATUS_INTERNAL_ERROR));
    }

    BOOST_AUTO_TEST_CASE(TestOperation_Throw)
    {
        ManualResetEvent event;
        ErrorCode err;
        shared_ptr<FabricKtlAwaitableServerWrapper> fabricServerSPtr =
            make_shared<FabricKtlAwaitableServerWrapper>(Common::GetSFDefaultKtlSystem().PagedAllocator(), true);

        VERIFY_IS_TRUE(fabricServerSPtr);

        //
        // Not storing the AsyncOperationSPtr on stack, this makes sure that the fabric async operation manages
        // its own reference till the internal KTL async operation completes.
        //
        fabricServerSPtr->BeginStart
            ([&fabricServerSPtr, &event, &err](AsyncOperationSPtr const& operation)
            {
                err = fabricServerSPtr->EndStart(operation);
                event.Set();
            },
            AsyncOperationSPtr());

        event.WaitOne();

        VERIFY_IS_TRUE(err.ToHResult() == HRESULT_FROM_NT(STATUS_NOT_IMPLEMENTED));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
