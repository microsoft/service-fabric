// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/ExclusiveFile.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;

namespace Common
{
    class TestExclusiveFile
    {
    };    

    static atomic_long counter(0);
    static atomic_long runningThreads(0);
    static shared_ptr<AutoResetEvent> allThreadsCompleted;

    DWORD _stdcall TestAccessThreadCallback(void *)
    {
        const int iterationCount = 10;
        wstring path = L"ExclusiveFile.lock";

        for (int i=0; i<iterationCount; i++)
        {
            ExclusiveFile lock(path);
            lock.Acquire(TimeSpan::MaxValue);

            ++counter;
            CODING_ERROR_ASSERT(counter.load() == 1);
            Sleep(200);
            CODING_ERROR_ASSERT(counter.load() == 1);
            --counter;
        }

        if (--runningThreads == 0) allThreadsCompleted->Set();

        return 0;
    }

    void TestAccessForThreadCount(int threadCount)
    {
        runningThreads.store(threadCount);
        allThreadsCompleted = make_shared<AutoResetEvent>(false);

        vector<HANDLE> threads;
        for (int i=0; i<threadCount; i++)
        {
            threads.push_back(CreateThread(NULL, 0, TestAccessThreadCallback, NULL, 0, NULL));
        }

        BOOST_REQUIRE(allThreadsCompleted->WaitOne(TimeSpan::FromSeconds(60)));
    }

    BOOST_FIXTURE_TEST_SUITE(ExclusiveFileTest,TestExclusiveFile)

    BOOST_AUTO_TEST_CASE(TestAccess)
    {
        TestAccessForThreadCount(1);
        TestAccessForThreadCount(5);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
