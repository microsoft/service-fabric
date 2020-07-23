#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'psLL'

namespace TStoreTests {
    using namespace ktl;

    class ThreadPoolTests {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        ThreadPoolTests() 
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~ThreadPoolTests()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->PagedAllocator();
        }

        Awaitable<void> DoWorkAsync(__in CancellationToken token)
        {
            co_await CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());

            while (!token.IsCancellationRequested)
            {
                lock_.Acquire();
                asyncCounter_++;
                lock_.Release();

                DoWork();
            }
        }

        void DoWork()
        {
            lock_.Acquire();
            workCounter_++;

            if (workCounter_ % 1000 == 0)
            {
                Task backgroundWorkTask = DoBackgroundWork();
                CODING_ERROR_ASSERT(backgroundWorkTask.IsTaskStarted());
            }
            
            lock_.Release();
        }


        Task DoBackgroundWork()
        {
            InterlockedIncrement64(&scheduleCounter_);

            co_await CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());

            lock_.Acquire();
            backgroundCounter_++;
            lock_.Release();
        }

        Awaitable<void> TestAsync()
        {
            CancellationTokenSource::SPtr tokenSourceSPtr = nullptr;
            CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, tokenSourceSPtr);

            KSharedArray<Awaitable<void>>::SPtr tasks = 
                _new(ALLOC_TAG, this->GetAllocator()) KSharedArray<Awaitable<void>>();
            
            for (int i = 0; i < 3; i++)
            {
                tasks->Append(DoWorkAsync(tokenSourceSPtr->Token));
            }

            for (ULONG32 i = 0; i < 5; i++)
            {
                Trace.WriteInfo(
                    "Test", 
                    "{0}: asyncCounter={1}, workCounter={2} scheduleCounter={3} backgroundCounter={4}", 
                    i, asyncCounter_, workCounter_, scheduleCounter_, backgroundCounter_);
                co_await KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, 5'000, nullptr);
            }

            tokenSourceSPtr->Cancel();
            co_await StoreUtilities::WhenAll(*tasks, GetAllocator());

            Trace.WriteWarning("Test", "Sleeping for 20 seconds");
            co_await KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, 20'000, nullptr);

            Trace.WriteInfo(
                "Test",
                "{0}: asyncCounter={1}, workCounter={2} scheduleCounter={3} backgroundCounter={4}",
                -1, asyncCounter_, workCounter_, scheduleCounter_, backgroundCounter_);
        }

    private:
        LONG64 asyncCounter_ = 0;
        LONG64 workCounter_ = 0;

        LONG64 scheduleCounter_ = 0;
        LONG64 backgroundCounter_ = 0;

        KtlSystem* ktlSystem_;
        KSpinLock lock_;
    };

    BOOST_FIXTURE_TEST_SUITE(ThreadPoolTestsSuite, ThreadPoolTests)

    BOOST_AUTO_TEST_CASE(ThreadPoolTest_ShouldNotDeadlock)
    {
        // Tests that CorHelper::ThreadPoolThread always suspends immediately. If it it does not, then this test will deadlock
        SyncAwait(TestAsync());
    }

    BOOST_AUTO_TEST_SUITE_END()
}