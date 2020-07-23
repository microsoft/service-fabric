// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'pmBS'

namespace TStoreTests
{
#define TRACE_MEMORY_DIFF(scenario) TRACE_MEMORY_DIFF_1(K_MAKE_UNIQUE_NAME(start), scenario)

#define TRACE_MEMORY_DIFF_1(start_name, scenario) \
    auto start_name = Diagnostics::GetProcessMemoryUsageBytes(); \
    KFinally([&] { Trace.WriteInfo("Test", scenario": {0} bytes. Size={1}", Diagnostics::GetProcessMemoryUsageBytes() - start_name, this->Store->Size);});

    using namespace ktl;
    using namespace Data::Utilities;

    class StringBufferMemoryPerfTest : public TStorePerfTestBase<KString::SPtr, KBuffer::SPtr, KStringComparer, StringStateSerializer, KBufferSerializer>
    {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        StringBufferMemoryPerfTest()
        {
            Setup(1);
        }

        ~StringBufferMemoryPerfTest()
        {
            Cleanup();
        }

        KString::SPtr CreateKey(__in ULONG32 index) override
        {
            KString::SPtr key;

            wstring zeroString = wstring(L"0");
            wstring seedString = to_wstring(index);
            while (seedString.length() < 15) // zero pad the number to make it easier to compare
            {
                seedString = zeroString + seedString;
            }

            wstring str = wstring(L"test_key_") + seedString;
            auto status = KString::Create(key, GetAllocator(), str.c_str());
            Diagnostics::Validate(status);
            return key;
        }

        KBuffer::SPtr CreateValue(__in ULONG32 index) override
        {
            return CreateBuffer(500, index);
        }

        Awaitable<void> MemoryUsageTestAsync(__in ULONG32 numKeys)
        {
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            cout << endl;

            // Add first key to ignore static costs
            KString::SPtr key0 = CreateKey(0);
            KBuffer::SPtr value0 = CreateBuffer(128, 's');

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key0, value0, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }


            {
                TRACE_MEMORY_DIFF("Delta Differential State");

                {
                    TRACE_MEMORY_DIFF("    Differential State 1")
                        for (ULONG32 i = 1; i < numKeys; i++)
                        {
                            KString::SPtr key = CreateKey(i);
                            KBuffer::SPtr value = CreateBuffer(128, 's');

                            {
                                auto txn = CreateWriteTransaction();
                                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                                co_await txn->CommitAsync();
                            }

                            key = nullptr;
                            value = nullptr;
                        }
                }

                {
                    TRACE_MEMORY_DIFF("            Checkpoint 1");
                    co_await CheckpointAsync();
                }
            }

            {
                TRACE_MEMORY_DIFF("      Consolidated State");

                {
                    TRACE_MEMORY_DIFF("    Differential State 2")
                        for (ULONG32 i = numKeys; i < 3 * numKeys; i++)
                        {
                            KString::SPtr key = CreateKey(i);
                            KBuffer::SPtr value = CreateBuffer(128, 's');

                            {
                                auto txn = CreateWriteTransaction();
                                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                                co_await txn->CommitAsync();
                            }

                            key = nullptr;
                            value = nullptr;
                        }
                }

                {
                    TRACE_MEMORY_DIFF("            Checkpoint 2");
                    co_await CheckpointAsync();
                }
            }

            {
                TRACE_MEMORY_DIFF("                 Sweep 1");
                Store->ConsolidationManagerSPtr->SweepConsolidatedState(CancellationToken::None);
            }

            {
                TRACE_MEMORY_DIFF("                 Sweep 2");
                Store->ConsolidationManagerSPtr->SweepConsolidatedState(CancellationToken::None);
            }
        }

    };

    BOOST_FIXTURE_TEST_SUITE(StringBufferMemoryPerfTestSuite, StringBufferMemoryPerfTest)

    BOOST_AUTO_TEST_CASE(MemoryUsageTest)
    {
        SyncAwait(MemoryUsageTestAsync(10'000));

            {
                TRACE_MEMORY_DIFF("  Close and Reopen Store");
                CloseAndReOpenStore();
            }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
