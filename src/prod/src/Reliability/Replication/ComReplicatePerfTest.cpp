// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TestHealthClient.h"
#include "ComTestStatefulServicePartition.h"
#include "ComTestOperation.h"
#include "ComTestStateProvider.h"
#include "ComProxyTestReplicator.h"
#include "ComReplicator.h"
#include "TestChangeRole.h"
#include "testFabricAsyncCallbackHelper.h"

#include <ktl.h>
#include <KTpl.h> 
#include <KComAdapter.h>

namespace ReplicationUnitTest
{
    using ::_delete;
}


namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;

    static Common::StringLiteral const PerfTestSource("PerfTest");

    class OperationData 
        : public IFabricOperationData
        , Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(OperationData)
            COM_INTERFACE_LIST1(
                FabricOperationData,
                IID_IFabricOperationData,
                IFabricOperationData)
    public:

        OperationData()
            : value_()
        {
        }

        OperationData(int valueSize)
            : value_(valueSize)
        {
            for (int i = 0; i < value_; i++)
            {
                buffer_.push_back(static_cast<byte>(0));
            }
        }

        HRESULT STDMETHODCALLTYPE GetData(
            /*[out]*/ ULONG * count,
            /*[out, retval]*/ FABRIC_OPERATION_DATA_BUFFER const ** buffers)
        {
            *count = 1;

            replicaBuffer_.BufferSize = static_cast<ULONG>(value_);
            replicaBuffer_.Buffer = buffer_.data();

            *buffers = &replicaBuffer_;

            return S_OK;
        }

    private:
        int value_;
        std::vector<byte> buffer_;
        FABRIC_OPERATION_DATA_BUFFER replicaBuffer_;
    };

    class ComProxyStateReplicator final
        : public KObject<ComProxyStateReplicator>
        , public KShared<ComProxyStateReplicator>
    {
        K_FORCE_SHARED(ComProxyStateReplicator)

    public:
         
        static NTSTATUS Create(
            __in Common::ComPointer<IFabricStateReplicator> & v1StateReplicator,
            __in KAllocator & allocator,
            __out ComProxyStateReplicator::SPtr & comProxyStateReplicator);
         
        ktl::Awaitable<void> ReplicateAsync(
            LONG size);
    private:

        ComProxyStateReplicator(
            __in Common::ComPointer<IFabricStateReplicator> & v1StateReplicator);

        class AsyncReplicateContextBase : 
            public Ktl::Com::FabricAsyncContextBase
        {
        };
 
        //
        // Replicate async operation
        //
        class AsyncReplicateContext
            : public Ktl::Com::AsyncCallOutAdapter<AsyncReplicateContextBase>
        {
            friend ComProxyStateReplicator;

            K_FORCE_SHARED(AsyncReplicateContext)

        public:

            ktl::Awaitable<void> ReplicateAsync(
                Common::ComPointer<IFabricOperationData> comOperationData);

        protected:

            HRESULT OnEnd(__in IFabricAsyncOperationContext & context);

        private:

            FABRIC_SEQUENCE_NUMBER resultSequenceNumber_;
            ComProxyStateReplicator::SPtr parent_;
            ktl::AwaitableCompletionSource<void>::SPtr task_;
        };

        Common::ComPointer<IFabricStateReplicator> comStateReplicator_;
    };

    ComProxyStateReplicator::ComProxyStateReplicator(
        __in ComPointer<IFabricStateReplicator> & v1StateReplicator)
        : KObject()
        , KShared()
        , comStateReplicator_(v1StateReplicator)
    {
    }

    ComProxyStateReplicator::~ComProxyStateReplicator()
    { 
    }

    NTSTATUS ComProxyStateReplicator::Create(
        __in ComPointer<IFabricStateReplicator> & v1StateReplicator,
        __in KAllocator & allocator,
        __out ComProxyStateReplicator::SPtr & comProxyStateReplicator)
    {
        ComProxyStateReplicator * pointer = _new('ascd', allocator) ComProxyStateReplicator(v1StateReplicator);

        if (pointer == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        comProxyStateReplicator = pointer;

        return STATUS_SUCCESS;
    }
     
    ktl::Awaitable<void> ComProxyStateReplicator::ReplicateAsync(
        __in LONG size)
    {
        AsyncReplicateContext::SPtr context = _new('abcd', GetThisAllocator()) AsyncReplicateContext();
        context->parent_ = this;

        ComPointer<IFabricOperationData> operationData = make_com<OperationData, IFabricOperationData>(size);

        co_await context->ReplicateAsync(
            operationData);

        co_return;
    }

    //
    // Callout adapter to invoke replicate API on v1 replicator
    //
    ComProxyStateReplicator::AsyncReplicateContext::AsyncReplicateContext()
        : resultSequenceNumber_(FABRIC_INVALID_SEQUENCE_NUMBER)
        , parent_()
        , task_()
    {
    }

    ComProxyStateReplicator::AsyncReplicateContext::~AsyncReplicateContext()
    {
    }

    ktl::Awaitable<void> ComProxyStateReplicator::AsyncReplicateContext::ReplicateAsync(
        __in Common::ComPointer<IFabricOperationData> comOperationData)
    {
        FABRIC_SEQUENCE_NUMBER logicalSequenceNumber = FABRIC_INVALID_SEQUENCE_NUMBER;

        //
        // The following implementation ensures that when the replication operations fails synchronously with
        // an error, this error is returned immediatly, instead of starting the underlying callout adapter.
        //
        // - begin fails: returns error, async context is not started
        // - begin succeeds:
        //    - completed synchronously:
        //       - end fails: returns error, async context is not started
        //       - end succeeds: returns success, async context not started
        //    - not completed synchronously:
        //       - when the Invoke is seen and the OnStart is seen, call end and complete the async context
        //
        ComPointer<IFabricAsyncOperationContext> context;

        HRESULT hr = parent_->comStateReplicator_->BeginReplicate(
            comOperationData.GetRawPointer(),
            this,
            &logicalSequenceNumber,
            context.InitializationAddress());

        if (context->CompletedSynchronously())
        {
            hr = parent_->comStateReplicator_->EndReplicate(
                context.GetRawPointer(),
                &resultSequenceNumber_); // Use the reference input variable only on beginreplicate

            co_return;
        }
        else
        {
            OnComOperationStarted(
                *context.GetRawPointer(),
                nullptr,
                nullptr);
        }

        ktl::AwaitableCompletionSource<void>::Create(GetThisAllocator(), 'abcf', task_);
        co_await task_->GetAwaitable();
        co_return;
    }
                
    HRESULT ComProxyStateReplicator::AsyncReplicateContext::OnEnd(__in IFabricAsyncOperationContext & context)
    {
        ASSERT_IFNOT(
            context.CompletedSynchronously() == FALSE,
            "Replicate Operation callback must only be invoked when not completed synchronously");

        HRESULT hr = parent_->comStateReplicator_->EndReplicate(
            &context,
            &resultSequenceNumber_);

        task_->Set();
        return hr;
    }

    class ComAsyncOperationCallbackHelper
        : public IFabricAsyncOperationCallback
        , private Common::ComUnknownBase
    {
        COM_INTERFACE_LIST1(ComAsyncOperationCallbackHelper, IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback);

    public:
        ComAsyncOperationCallbackHelper(std::function<void(IFabricAsyncOperationContext *)> const & callback)
            : callback_(callback)
        {
        }

        void STDMETHODCALLTYPE Invoke(IFabricAsyncOperationContext * aoc)
        {
            callback_(aoc);
        }

        static BOOLEAN GetCompletedSynchronously(IFabricAsyncOperationContext * context)
        {
            return context->CompletedSynchronously();
        }
    private:
        std::function<void(IFabricAsyncOperationContext *)> callback_;
    };

    class ServiceEntry
    {
    public:
        ServiceEntry(__int64 key)
            : key_(key)
        {
        }

        __declspec(property(get = get_Key)) __int64 Key;
        __int64 get_Key() const
        {
            return key_;
        }

        HRESULT BeginReplicate(
            Common::ComPointer<IFabricStateReplicator> & replicator,
            Common::ComPointer<ComAsyncOperationCallbackHelper> & callback,
            Common::ComPointer<IFabricAsyncOperationContext> & context,
            FABRIC_SEQUENCE_NUMBER & newVersion,
            int newValueSize)
        {
            ComPointer<OperationData> operationData = make_com<OperationData>(newValueSize);

            HRESULT hr = replicator->BeginReplicate(
                operationData.GetRawPointer(),
                callback.GetRawPointer(),
                &newVersion,
                context.InitializationAddress());

            return hr;
        }

    private:
        __int64 const key_;
        static FABRIC_SEQUENCE_NUMBER const InvalidVersion = -1;
    };

    class TestReplicatePerf
    {
    protected:
        TestReplicatePerf() { }

        class DummyRoot : public Common::ComponentRoot
        {
        };
    public:
        static Common::ComponentRoot const & GetRoot();
    };

    namespace {
        class ReplicatorTestWrapper
        {
        public:
            ReplicatorTestWrapper()
                : factory_(TestReplicatePerf::GetRoot())
                , primary_()
            {
                VERIFY_IS_TRUE(factory_.Open(L"0").IsSuccess(), L"ComReplicatorFactory opened");

                auto partition = new ComTestStatefulServicePartition(Common::Guid::NewGuid());
                IReplicatorHealthClientSPtr temp;
                auto hr = factory_.CreateReplicator(
                    0LL,
                    partition,
                    new ComTestStateProvider(0, -1, TestReplicatePerf::GetRoot()),
                    nullptr,
                    false,
                    move(temp),
                    stateReplicator_.InitializationAddress());

                ASSERT_IFNOT(
                    SUCCEEDED(hr),
                    "Create replicator failed with {0:x}",
                    hr);

                IFabricPrimaryReplicator *replicatorObj;
                hr = stateReplicator_->QueryInterface(
                    IID_IFabricPrimaryReplicator,
                    reinterpret_cast<void**>(&replicatorObj));

                ASSERT_IFNOT(
                    SUCCEEDED(hr),
                    "Query for IID_IFabricPrimaryReplicator failed with {0:x}",
                    hr);

                primary_.SetAndAddRef(replicatorObj);

                Common::AutoResetEvent event;
                ComPointer<ComAsyncOperationCallbackHelper> openCallback = make_com<ComAsyncOperationCallbackHelper>(
                    [&](IFabricAsyncOperationContext * context)
                {
                    ComPointer<IFabricStringResult> stringResult;
                    replicatorObj->EndOpen(context, stringResult.InitializationAddress());
                    event.Set();
                });

                ComPointer<IFabricAsyncOperationContext> openContext;
                hr = replicatorObj->BeginOpen(openCallback.GetRawPointer(), openContext.InitializationAddress());
                event.Wait();

                ComPointer<ComAsyncOperationCallbackHelper> crCallback = make_com<ComAsyncOperationCallbackHelper>(
                    [&](IFabricAsyncOperationContext * context)
                {
                    replicatorObj->EndChangeRole(context);
                    event.Set();
                });

                ComPointer<IFabricAsyncOperationContext> crContext;
                FABRIC_EPOCH epoch;
                epoch.DataLossNumber = 1;
                epoch.ConfigurationNumber = 122;
                epoch.Reserved = NULL;
                hr = replicatorObj->BeginChangeRole(&epoch, FABRIC_REPLICA_ROLE_PRIMARY, crCallback.GetRawPointer(), crContext.InitializationAddress());
                event.Wait();
            }

            ~ReplicatorTestWrapper()
            {
                VERIFY_IS_TRUE(factory_.Close().IsSuccess(), L"ComReplicatorFactory closed");
            }

            Common::ComPointer<IFabricStateReplicator> GetReplicator() const
            {
                return stateReplicator_;
            }

        private:

            ComReplicatorFactory factory_;
            ComPointer<IFabricStateReplicator> stateReplicator_;
            ComPointer<IFabricPrimaryReplicator> primary_;
        };
    }

    class Test
    {
    public:
        static void Execute(
            LONG maxOutstanding,
            LONG dataSize,
            LONG numberOfOperations,
            Common::ComPointer<IFabricStateReplicator> const & replicator)
        {
            Test t(maxOutstanding, dataSize, numberOfOperations, replicator);
            return t.Run();
        }

        static ktl::Awaitable<void> ExecuteAsync(
            LONG maxOutstanding,
            LONG dataSize,
            LONG numberOfOperations,
            KAllocator & allocator,
            Common::ComPointer<IFabricStateReplicator> const & replicator,
            ComProxyStateReplicator & proxyReplicator)
        {
            Test t(maxOutstanding, dataSize, numberOfOperations, replicator);
            co_await t.RunAsync(allocator, proxyReplicator);
            co_return;
        }

    private:
        Test(
            LONG maxOutstanding,
            LONG dataSize,
            LONG totalOperations,
            Common::ComPointer<IFabricStateReplicator> const & replicator)
            : maxOutstanding_(maxOutstanding)
            , dataSize_(dataSize)
            , totalOperations_(totalOperations)
            , numberOfOperations_(0)
            , inflightOperationCount_(0)
            , replicator_(replicator)
            , data_()
        {
            for (LONG i = 0; i < maxOutstanding; i++)
            {
                data_.push_back(ServiceEntry(i));
            }
        }

        static void StartReplicatingIfNeeded(ServiceEntry & entry, Test & test)
        {
            auto opCount = test.numberOfOperations_.load();

            if (opCount < test.totalOperations_)
            {
                ReplicateServiceEntry(entry, test);
            }
            else
            {
                Trace.WriteInfo(
                    PerfTestSource,
                    "ServiceEntry {0} stopped replicating",
                    entry.Key);
            }
        }

        static ktl::Awaitable<void> ReplicateServiceEntryAsync(
            ServiceEntry & entry, 
            Test & test,
            ComProxyStateReplicator & replicator)
        {
            test.inflightOperationCount_ += 1;
            co_await replicator.ReplicateAsync(test.dataSize_);
            test.inflightOperationCount_ -= 1;

            test.numberOfOperations_ += 1;

            StartReplicatingIfNeeded(entry, test);
        }

        static ktl::Task StartReplicatingIfNeededAsync(
            ServiceEntry & entry, 
            Test & test,
            ComProxyStateReplicator & replicator)
        {
            auto opCount = test.numberOfOperations_.load();

            if (opCount < test.totalOperations_)
            {
                co_await ReplicateServiceEntryAsync(entry, test, replicator);
            }
            else
            {
                Trace.WriteInfo(
                    PerfTestSource,
                    "ServiceEntry {0} stopped replicating",
                    entry.Key);
            }
        }

        static void ReplicateServiceEntry(ServiceEntry & entry, Test & test)
        {
            test.inflightOperationCount_ += 1;

            BOOLEAN completedSync = false;

            ComPointer<ComAsyncOperationCallbackHelper> callback = make_com<ComAsyncOperationCallbackHelper>(
                [&entry, &test](IFabricAsyncOperationContext * context)
            {
                BOOLEAN inCallbackCompletedSynchronously = ComAsyncOperationCallbackHelper::GetCompletedSynchronously(context);
                if (!inCallbackCompletedSynchronously)
                {
                    test.inflightOperationCount_ -= 1;
                    FABRIC_SEQUENCE_NUMBER lsn;

                    auto hr = test.replicator_->EndReplicate(context, &lsn);
                    if (hr == S_OK)
                    {
                        test.numberOfOperations_ += 1;
                    }

                    StartReplicatingIfNeeded(entry, test);
                }
            });

            FABRIC_SEQUENCE_NUMBER newLsn;
            ComPointer<IFabricAsyncOperationContext> beginContext;
            HRESULT hr = entry.BeginReplicate(
                test.replicator_,
                callback,
                beginContext,
                newLsn,
                test.dataSize_);

            if (hr != S_OK)
            {
                test.inflightOperationCount_ -= 1;

                Threadpool::Post(
                    [&entry, &test]
                {
                    StartReplicatingIfNeeded(entry, test);
                });

                return;
            }

            completedSync = ComAsyncOperationCallbackHelper::GetCompletedSynchronously(beginContext.GetRawPointer());

            if (completedSync)
            {
                --test.inflightOperationCount_;

                FABRIC_SEQUENCE_NUMBER lsn;
                hr = test.replicator_->EndReplicate(beginContext.GetRawPointer(), &lsn);
                if (hr == S_OK)
                {
                    ++test.numberOfOperations_;
                }

                StartReplicatingIfNeeded(entry, test);
            }
        }

        ktl::Awaitable<void> RunAsync(
            KAllocator & allocator,
            ComProxyStateReplicator & replicator)
        {
            double opsPerMillisecond = 0;
            double opsPerSecond = 0;
            Stopwatch watch;
            watch.Start();
            ComProxyStateReplicator::SPtr re = &replicator;

            for (LONG i = 0; i < maxOutstanding_; i++)
            {
                auto ref = &data_[i];

                    Trace.WriteInfo(
                        PerfTestSource,
                        "ServiceEntry {0} starting to replicate",
                        ref->Key);

                    StartReplicatingIfNeededAsync(*ref, *this, *re);
            }

            for (;;)
            {
                KTimer::SPtr timer;
                KTimer::Create(timer, allocator, 'abcd');
                co_await timer->StartTimerAsync(1000, nullptr);

                auto opCount = numberOfOperations_.load();
                auto inflightOpCount = inflightOperationCount_.load();
                if (opCount >= totalOperations_ &&
                    inflightOpCount == 0)
                {
                    break;
                }

                opsPerMillisecond = (double)numberOfOperations_.load() / (double)watch.ElapsedMilliseconds;
                opsPerSecond = 1000 * opsPerMillisecond;

                Trace.WriteInfo(
                    PerfTestSource,
                    "Ops Completed = {0}. Inflight ops = {1}. Waiting until {2}. OpsPerSecond = {3}",
                    opCount,
                    inflightOpCount,
                    totalOperations_,
                    opsPerSecond);
            }

            watch.Stop();

            Trace.WriteInfo(
                PerfTestSource,
                "Time taken {0}; Operations Per Second = {1}; Operations per Millisecond = {2}; numberOfOperations_.load()  = {3}; ElapsedMilliseconds = {4}",
                watch.Elapsed.ToString(),
                opsPerSecond,
                opsPerMillisecond,
                numberOfOperations_.load(),
                watch.ElapsedMilliseconds);

            co_return;
        }

        void Run()
        {
            double opsPerMillisecond = 0;
            double opsPerSecond = 0;
            Stopwatch watch;
            watch.Start();

            for (LONG i = 0; i < maxOutstanding_; i++)
            {
                auto ref = &data_[i];

                Threadpool::Post(
                    [ref, this]
                {
                    Trace.WriteInfo(
                        PerfTestSource,
                        "ServiceEntry {0} starting to replicate",
                        ref->Key);

                    StartReplicatingIfNeeded(*ref, *this);
                });
            }

            for (;;)
            {
                Sleep(1000);

                auto opCount = numberOfOperations_.load();
                auto inflightOpCount = inflightOperationCount_.load();
                if (opCount >= totalOperations_ &&
                    inflightOpCount == 0)
                {
                    break;
                }

                opsPerMillisecond = (double)numberOfOperations_.load() / (double)watch.ElapsedMilliseconds;
                opsPerSecond = 1000 * opsPerMillisecond;

                Trace.WriteInfo(
                    PerfTestSource,
                    "Ops Completed = {0}. Inflight ops = {1}. Waiting until {2}. OpsPerSecond = {3}",
                    opCount,
                    inflightOpCount,
                    totalOperations_,
                    opsPerSecond);
            }

            watch.Stop();

            Trace.WriteInfo(
                PerfTestSource,
                "Time taken {0}; Operations Per Second = {1}; Operations per Millisecond = {2}; numberOfOperations_.load()  = {3}; ElapsedMilliseconds = {4}",
                watch.Elapsed.ToString(),
                opsPerSecond,
                opsPerMillisecond,
                numberOfOperations_.load(),
                watch.ElapsedMilliseconds);
        }

        LONG const maxOutstanding_;
        LONG const dataSize_;
        LONG const totalOperations_;

        Common::atomic_long numberOfOperations_;
        Common::atomic_long inflightOperationCount_;
        Common::ComPointer<IFabricStateReplicator> replicator_;
        std::vector<ServiceEntry> data_;
    };

    LONG outstanding = 1000;
    LONG size = 128;
    LONG max = 2000000;


    /***********************************
    * TestReplicatePerf methods
    **********************************/
    Common::ComponentRoot const & TestReplicatePerf::GetRoot()
    {
        static std::shared_ptr<DummyRoot> root = std::make_shared<DummyRoot>();
        return *root;
    }

    BOOST_FIXTURE_TEST_SUITE(TestReplicatePerfSuite,TestReplicatePerf)

    BOOST_AUTO_TEST_CASE(BeginReplicatePerf)
    {
        ReplicatorTestWrapper wrapper;

        Test::Execute(
            outstanding,
            size,
            max,
            wrapper.GetReplicator());
    }

    BOOST_AUTO_TEST_CASE(BeginReplicatePerfKtl)
    {
        ReplicatorTestWrapper wrapper;
        KtlSystem* underlyingSystem;
        NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem->SetStrictAllocationChecks(TRUE);
        KAllocator& allocator = underlyingSystem->NonPagedAllocator();
        underlyingSystem->SetDefaultSystemThreadPoolUsage(FALSE);

        ComProxyStateReplicator::SPtr proxy;
        ComPointer<IFabricStateReplicator> replicator = wrapper.GetReplicator();
        ComProxyStateReplicator::Create(replicator, allocator, proxy);
    
        SyncAwait(Test::ExecuteAsync(
            outstanding,
            size,
            max,
            allocator,
            replicator,
            *proxy));
    }
    BOOST_AUTO_TEST_SUITE_END()
}
