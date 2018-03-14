// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;

namespace Common
{
    BOOST_AUTO_TEST_SUITE2(MutexTest)

    void BasicTest(wstring const& mutexName)
    {
        auto mutex = MutexHandle::CreateUPtr(mutexName);
        BOOST_REQUIRE(mutex->WaitOne(TimeSpan::Zero).IsSuccess());

        AutoResetEvent completed(false);
        Threadpool::Post([&mutexName, &completed]
        {
            auto mutex = MutexHandle::CreateUPtr(mutexName);
            auto error = mutex->WaitOne(TimeSpan::FromMilliseconds(100));
            Trace.WriteInfo(TraceType, "acquire on a different thread: {0}", error); 
            BOOST_REQUIRE(!error.IsSuccess());
            completed.Set();
        });

        BOOST_REQUIRE(completed.WaitOne(TimeSpan::FromSeconds(3)));
        mutex->Release();

        Threadpool::Post([&mutexName, &completed]
        {
            auto mutex = MutexHandle::CreateUPtr(mutexName);
            auto error = mutex->WaitOne(TimeSpan::FromSeconds(3));
            Trace.WriteInfo(TraceType, "acquire on a different thread after release on main thread: {0}", error);
            BOOST_REQUIRE(error.IsSuccess());
            completed.Set();
        });

        BOOST_REQUIRE(completed.WaitOne(TimeSpan::FromSeconds(5)));
    }

    BOOST_AUTO_TEST_CASE(Basic)
    {
        ENTER;
        BasicTest(L"Global\\MutexTestBasic");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Basic2)
    {
        ENTER;
        BasicTest(L"Global/MutexTestBasic2");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Basic3)
    {
        ENTER;
        BasicTest(L"/Global/MutexTest/Basic3");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Basic4)
    {
        ENTER;
        BasicTest(L"Local\\Basic4`'~!@#$%^+_)(*&=-/.;[]|}{:>?<");
        LEAVE;
    }

#ifdef PLATFORM_UNIX

    BOOST_AUTO_TEST_CASE(MultiProcess)
    {
        ENTER;

        wstring mutexName(L"MutexMultiProcessTest." + Guid::NewGuid().ToString());

        int pipeToChild[2];
        ZeroRetValAssert(pipe(pipeToChild));
        int pipeToParent[2];
        ZeroRetValAssert(pipe(pipeToParent));

        auto pid = fork();
        if (pid == 0)
        {
            close(pipeToParent[0]);
            close(pipeToChild[1]);

            //Wait for parent process to lock first
            bool shouldStart = false;
            int retval = 0;
            RetryOnEIntr(retval, read(pipeToChild[0], &shouldStart, sizeof(shouldStart)));

            Trace.WriteInfo(TraceType, "child process read shouldStart: {0}", shouldStart);
            Invariant(shouldStart);

            auto mutex2 = MutexHandle::CreateUPtr(mutexName);
            bool locked = mutex2->WaitOne(TimeSpan::Zero).IsSuccess();
            RetryOnEIntr(retval, write(pipeToParent[1], &locked, sizeof(locked)));
            Invariant(retval == sizeof(locked));

            Trace.WriteInfo(TraceType, "child process exiting");
            _exit(0);
        }

        close(pipeToChild[0]);
        close(pipeToParent[1]);

        Trace.WriteInfo(TraceType, "parent acquiring mutex");
        auto mutex = MutexHandle::CreateUPtr(mutexName);
        BOOST_REQUIRE(mutex->WaitOne(TimeSpan::Zero).IsSuccess());
        Trace.WriteInfo(TraceType, "parent acquired mutex");

        bool shouldStart = true;
        int retval = 0;
        RetryOnEIntr(retval, write(pipeToChild[1], &shouldStart, sizeof(shouldStart)));
        VERIFY_ARE_EQUAL2(retval, sizeof(shouldStart));

        bool childLockSuccess = true;
        RetryOnEIntr(retval, read(pipeToParent[0], &childLockSuccess, sizeof(childLockSuccess)));
        VERIFY_ARE_EQUAL2(retval, 1);
        BOOST_REQUIRE(!childLockSuccess);

        Trace.WriteInfo(TraceType, "parent releasing mutex");
        mutex.reset();
        Trace.WriteInfo(TraceType, "parent released mutex");

        // Initialize pipes for the next fork 
        close(pipeToChild[1]);
        close(pipeToParent[0]);
        ZeroRetValAssert(pipe(pipeToChild));
        ZeroRetValAssert(pipe(pipeToParent));

        pid = fork();
        if (pid == 0)
        {
            close(pipeToParent[0]);
            close(pipeToChild[1]);

            auto mutex3 = MutexHandle::CreateUPtr(mutexName);
            bool locked = mutex3->WaitOne(TimeSpan::Zero).IsSuccess();
            RetryOnEIntr(retval, write(pipeToParent[1], &locked, sizeof(locked)));
            Invariant(retval == sizeof(locked));

            Trace.WriteInfo(TraceType, "child process exiting");
            mutex3.reset();
            _exit(0);
        }

        close(pipeToChild[0]);
        close(pipeToParent[1]);

        childLockSuccess = false;
        RetryOnEIntr(retval, read(pipeToParent[0], &childLockSuccess, sizeof(childLockSuccess)));
        VERIFY_ARE_EQUAL2(retval, 1);
        BOOST_REQUIRE(childLockSuccess);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(KillProcess)
    {
        ENTER;

        wstring mutexName(L"MutexTestKillProcess." + Guid::NewGuid().ToString());

        int pipeToChild[2];
        ZeroRetValAssert(pipe(pipeToChild));
        int pipeToParent[2];
        ZeroRetValAssert(pipe(pipeToParent));

        auto pid = fork();
        if (pid == 0)
        {
            close(pipeToParent[0]);
            close(pipeToChild[1]);

            auto mutex2 = MutexHandle::CreateUPtr(mutexName);
            bool locked = mutex2->WaitOne(TimeSpan::Zero).IsSuccess();
            Invariant(locked);

            bool shouldStart = true;
            int retval = 0;
            RetryOnEIntr(retval, write(pipeToParent[1], &shouldStart, sizeof(shouldStart)));
            VERIFY_ARE_EQUAL2(retval, sizeof(shouldStart));

            Trace.WriteInfo(TraceType, "child holding mutex, waiting to be killed");
            AutoResetEvent(false).WaitOne();
        }

        close(pipeToChild[0]);
        close(pipeToParent[1]);

        //Wait for child process to lock first
        bool shouldStart = false;
        int retval = 0;
        RetryOnEIntr(retval, read(pipeToParent[0], &shouldStart, sizeof(shouldStart)));
        BOOST_REQUIRE(shouldStart);

        Trace.WriteInfo(TraceType, "parent acquiring mutex");
        auto mutex = MutexHandle::CreateUPtr(mutexName);
        BOOST_REQUIRE(!mutex->WaitOne(TimeSpan::Zero).IsSuccess());

        // kill child process
        kill(pid, SIGKILL);

        Trace.WriteInfo(TraceType, "parent acquiring mutex after killing child process");
        mutex = MutexHandle::CreateUPtr(mutexName);
        BOOST_REQUIRE(mutex->WaitOne(TimeSpan::FromSeconds(3)).IsSuccess());

        LEAVE;
    }

#endif

    BOOST_AUTO_TEST_SUITE_END()
}
