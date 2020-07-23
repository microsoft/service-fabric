// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/ComPointer.h"
#include "Common/AssertWF.h"

using namespace std;

namespace Common
{
    class FabricKtlServerWrapper;

    class KtlProxyTest
    {
    };

    class KtlTestServer 
        : public KAsyncContextBase // KTL Async Operation
    {
        friend FabricKtlServerWrapper;
        K_FORCE_SHARED(KtlTestServer)

    public:
        NTSTATUS KtlStartServer(
        __in KAsyncContextBase* const ParentAsync,
        __in KAsyncContextBase::CompletionCallback Completion)
        {
            Start(ParentAsync, Completion);
            return STATUS_SUCCESS;
        }

    private:
        void OnStart()
        {
            Complete(STATUS_INTERNAL_ERROR);
        }

        void OnReuse()
        {
        }
    };

    KtlTestServer::KtlTestServer() {}
    KtlTestServer::~KtlTestServer() {}

    class FabricKtlServerWrapper  // Fabric operation corresponding to the KTL operation
    {
    public:
        FabricKtlServerWrapper(KAllocator &allocator)
        {
            ktlServer_ = _new(0, allocator) KtlTestServer();
        }

        AsyncOperationSPtr BeginStart(
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent)
        {
            return AllocableAsyncOperation::CreateAndStart<FabricKtlServerOp>(
                ktlServer_->GetThisAllocator(),
                ktlServer_,
                callback,
                parent);
        }

        ErrorCode EndStart(
            __in Common::AsyncOperationSPtr const& operation)
        {
            return FabricKtlServerOp::End(operation);
        }

    private:
        KtlTestServer::SPtr ktlServer_;

        class FabricKtlServerOp : public KtlProxyAsyncOperation
        {
        public:
            FabricKtlServerOp(
                KtlTestServer::SPtr server,
                AsyncCallback const& callback,
                AsyncOperationSPtr const& parent,
                KAsyncContextBase * const ParentAsyncContext = nullptr)
                : KtlProxyAsyncOperation(server.RawPtr(), ParentAsyncContext, callback, parent)
            {
            }

            static ErrorCode End(AsyncOperationSPtr const& operation)
            {
                auto op = AsyncOperation::End<FabricKtlServerOp>(operation);
                return op->Error;
            }

            NTSTATUS StartKtlAsyncOperation(
                __in KAsyncContextBase* const parentAsyncContext, 
                __in KAsyncContextBase::CompletionCallback callback)
            {
                KtlTestServer* server = thisKtlOperation_.DownCast<KtlTestServer>();
                return server->KtlStartServer(
                    parentAsyncContext,
                    callback);
            }
        };
    };

    BOOST_FIXTURE_TEST_SUITE(KtlProxyTestSuite,KtlProxyTest)

    BOOST_AUTO_TEST_CASE(TestOperation)
    {
        ManualResetEvent event;
        ErrorCode err;
        shared_ptr<FabricKtlServerWrapper> fabricServerSPtr = 
            make_shared<FabricKtlServerWrapper>(Common::GetSFDefaultKtlSystem().PagedAllocator());

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

    BOOST_AUTO_TEST_SUITE_END()

}
