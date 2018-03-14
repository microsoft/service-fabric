// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

Common::StringLiteral const TraceType("ProcessWaitTest");

using namespace std;

namespace Common
{
    //
    // Basic test for a thread pool based async wait
    //
    BOOST_AUTO_TEST_SUITE2(ProcessWaitTest)

    BOOST_AUTO_TEST_CASE(Basic)
    {
        ENTER;

        ProcessWait::Setup();
        ProcessWait::Test_Reset();

        auto childPid = fork();
        if (childPid == 0)
        {
            _exit(0);
        }

        Trace.WriteInfo(TraceType, "created child process {0}", childPid);

        auto childExit = make_shared<AutoResetEvent>(false);
        auto errorReported = make_shared<ErrorCode>();
        auto exitCodeReported = make_shared<DWORD>(0);
        auto pidReported = make_shared<pid_t>(0);

        auto pwait = ProcessWait::CreateAndStart(
            childPid,
            [=] (pid_t pid, ErrorCode const & err, DWORD exitCode)
            {
                Trace.WriteInfo(TraceType, "reported: process {0}, err = {1}, status = {2}", pid, err, exitCode);
                *pidReported = pid;
                *errorReported = err;
                *exitCodeReported = exitCode;
                childExit->Set();
            });

        auto success = childExit->WaitOne(TimeSpan::FromSeconds(30));
        if (!success)
        {
            string cmdline;
            cmdline.append(formatString("/bin/ps -f -q {0}", childPid));
            system(cmdline.c_str());
            BOOST_FAIL("wait failed");
        }
        Trace.WriteInfo(TraceType, "errorReported = {0}", *errorReported);

        VERIFY_ARE_EQUAL2(childPid, *pidReported);
        BOOST_REQUIRE(errorReported->IsSuccess());
        BOOST_REQUIRE(WIFEXITED(*exitCodeReported));
        VERIFY_ARE_EQUAL2(*exitCodeReported, 0);

        LEAVE;
    }
 
    BOOST_AUTO_TEST_CASE(Kill)
    {
        ENTER;

        auto childPid = fork();
        if (childPid == 0)
        {
            sleep(300);
        }

        Trace.WriteInfo(TraceType, "created child process {0}", childPid);

        auto childExit = make_shared<AutoResetEvent>(false);
        auto errorReported = make_shared<ErrorCode>();
        auto exitCodeReported = make_shared<DWORD>(0);
        auto pidReported = make_shared<pid_t>(0);

        auto pwait = ProcessWait::CreateAndStart(
            childPid,
            [=] (pid_t pid, ErrorCode const & err, DWORD exitCode)
            {
                Trace.WriteInfo(TraceType, "reported: process {0}, err = {1}, status = {2}", pid, err, exitCode);
                *pidReported = pid;
                *errorReported = err;
                *exitCodeReported = exitCode;
                childExit->Set();
            });

        BOOST_REQUIRE_EQUAL(kill(childPid, SIGKILL), 0);

        BOOST_REQUIRE(childExit->WaitOne(TimeSpan::FromSeconds(3)));
        Trace.WriteInfo(TraceType, "errorReported = {0}", *errorReported);
        VERIFY_ARE_EQUAL2(childPid, *pidReported);
        BOOST_REQUIRE(errorReported->IsSuccess());
        BOOST_REQUIRE(WIFSIGNALED(*exitCodeReported));
        VERIFY_ARE_EQUAL2(WTERMSIG(*exitCodeReported), SIGKILL);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(System)
    {
        ENTER;

        auto childPid = fork();
        if (childPid == 0)
        {
            sleep(300);
        }

        Trace.WriteInfo(TraceType, "created child process {0}", childPid);

        auto childExit = make_shared<AutoResetEvent>(false);
        auto errorReported = make_shared<ErrorCode>();
        auto exitCodeReported = make_shared<DWORD>(0);
        auto pidReported = make_shared<pid_t>(0);

        auto pwait = ProcessWait::CreateAndStart(
            childPid,
            [=] (pid_t pid, ErrorCode const & err, DWORD exitCode)
            {
                Trace.WriteInfo(TraceType, "reported: process {0}, err = {1}, status = {2}", pid, err, exitCode);
                *pidReported = pid;
                *errorReported = err;
                *exitCodeReported = exitCode;
                childExit->Set();
            });

        Trace.WriteInfo(TraceType, "verify system() still works, system calls waitpid()");
        BOOST_REQUIRE_EQUAL(system("/bin/true"), 0);

        BOOST_REQUIRE_EQUAL(kill(childPid, SIGKILL), 0);

        BOOST_REQUIRE(childExit->WaitOne(TimeSpan::FromSeconds(3)));
        Trace.WriteInfo(TraceType, "errorReported = {0}", *errorReported);
        VERIFY_ARE_EQUAL2(childPid, *pidReported);
        BOOST_REQUIRE(errorReported->IsSuccess());
        BOOST_REQUIRE(WIFSIGNALED(*exitCodeReported));
        VERIFY_ARE_EQUAL2(WTERMSIG(*exitCodeReported), SIGKILL);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SPtr)
    {
        ENTER;

        auto childPid = fork();
        int const exitCode = 1;
        if (childPid == 0)
        {
            _exit(exitCode);
        }

        Trace.WriteInfo(TraceType, "created child process {0}", childPid);

        auto childExit = make_shared<AutoResetEvent>(false);
        auto errorReported = make_shared<ErrorCode>();
        auto exitCodeReported = make_shared<DWORD>(0);
        auto pidReported = make_shared<pid_t>(0);

        auto pwait = ProcessWait::CreateAndStart(
            childPid,
            [=] (pid_t pid, ErrorCode const & err, DWORD exitCode)
            {
                Trace.WriteInfo(TraceType, "reported: process {0}, err = {1}, status = {2}", pid, err, exitCode);
                *pidReported = pid;
                *errorReported = err;
                *exitCodeReported = exitCode;
                childExit->Set();
            });

        BOOST_REQUIRE(childExit->WaitOne(TimeSpan::FromSeconds(3)));
        Trace.WriteInfo(TraceType, "errorReported = {0}", *errorReported);
        VERIFY_ARE_EQUAL2(childPid, *pidReported);
        BOOST_REQUIRE(errorReported->IsSuccess());
        BOOST_REQUIRE(WIFEXITED(*exitCodeReported));
        VERIFY_ARE_EQUAL2(WEXITSTATUS(*exitCodeReported), exitCode);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ExplicitStart)
    {
        ENTER;

        auto childPid = fork();
        if (childPid == 0)
        {
            _exit(0);
        }

        Trace.WriteInfo(TraceType, "created child process {0}", childPid);

        auto childExit = make_shared<AutoResetEvent>(false);
        auto error = make_shared<ErrorCode>();
        auto pwait = make_shared<ProcessWait>();
        pwait->StartWait(
            childPid,
            [pwait, childExit, error] (pid_t, ErrorCode const & err, DWORD)
            {
                *error = err;
                childExit->Set();
            });

        BOOST_REQUIRE(childExit->WaitOne(TimeSpan::FromSeconds(3)));
        Trace.WriteInfo(TraceType, "error = {0}", *error);
        BOOST_REQUIRE(error->IsSuccess());

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TimeoutTest)
    {
        ENTER;

        auto childExit = make_shared<AutoResetEvent>(false);
        auto error = make_shared<ErrorCode>();
        auto pwait = make_shared<ProcessWait>();
        pwait->StartWait(
            getpid(),
            [=] (pid_t, ErrorCode const & err, DWORD)
            {
                *error = err;
                childExit->Set();
            });

        BOOST_REQUIRE(!childExit->WaitOne(TimeSpan::FromSeconds(0.1)));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Cancel)
    {
        ENTER;

        auto childExit = make_shared<AutoResetEvent>(false);
        auto error = make_shared<ErrorCode>();

        auto pwait = ProcessWait::CreateAndStart(
            getpid(),
            [=] (pid_t, ErrorCode const & err, DWORD)
            {
                *error = err;
                childExit->Set();
            });

        pwait->Cancel();

        BOOST_REQUIRE(childExit->WaitOne(TimeSpan::FromSeconds(1)));
        Trace.WriteInfo(TraceType, "error = {0}", *error);
        BOOST_REQUIRE(error->IsError(ErrorCodeValue::OperationCanceled));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(AutoCancel)
    {
        ENTER;

        auto childExit = make_shared<AutoResetEvent>(false);
        auto error = make_shared<ErrorCode>();
        {
            ProcessWait pwait(
                getpid(),
                [childExit, error] (pid_t, ErrorCode const & err, DWORD)
                {
                    *error = err;
                    childExit->Set();
                });

            pwait.StartWait();
        }

        BOOST_REQUIRE(childExit->WaitOne(TimeSpan::FromSeconds(1)));
        Trace.WriteInfo(TraceType, "error = {0}", *error);
        BOOST_REQUIRE(error->IsError(ErrorCodeValue::OperationCanceled));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Repeat)
    {
        ENTER;

        vector<ProcessWaitSPtr> waiters;
        for(uint i = 0; i < 3; ++i)
        {
            auto childPid = fork();
            if (childPid == 0)
            {
                _exit(0);
            }

            Trace.WriteInfo(TraceType, "created child process {0}", childPid);

            auto childExit = make_shared<AutoResetEvent>(false);
            auto error = make_shared<ErrorCode>();

            auto pwait = ProcessWait::CreateAndStart(
                childPid,
                [childExit, error] (pid_t, ErrorCode const & err, DWORD)
                {
                    *error = err;
                    childExit->Set();
                });

            waiters.emplace_back(move(pwait));

            BOOST_REQUIRE(childExit->WaitOne(TimeSpan::FromSeconds(3)));
            Trace.WriteInfo(TraceType, "error = {0}", *error);
            BOOST_REQUIRE(error->IsSuccess());
        }

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Concurrent)
    {
        ENTER;

        vector<pid_t> children;
        const uint total = 3;
        for(uint i = 0; i < total; ++i)
        {
            auto childPid = fork();
            if (childPid == 0)
            {
                _exit(0);
            }

            Trace.WriteInfo(TraceType, "created child process {0}", childPid);
            children.push_back(childPid);
        }

        auto completeCount = make_shared<atomic_uint64>(0);
        auto allCompleted = make_shared<AutoResetEvent>(false);
        auto waiters = make_shared<vector<ProcessWaitSPtr>>();

        for(auto const childPid : children)
        {
            Threadpool::Post([childPid, completeCount, allCompleted, waiters]
            {
                auto childExit = make_shared<AutoResetEvent>(false);
                auto error = make_shared<ErrorCode>();
                auto pwait = ProcessWait::CreateAndStart(
                    childPid,
                    [childExit, error] (pid_t, ErrorCode const & err, DWORD)
                    {
                        *error = err;
                        childExit->Set();
                    });

                waiters->emplace_back(move(pwait));

                BOOST_REQUIRE(childExit->WaitOne(TimeSpan::FromSeconds(10)));
                Trace.WriteInfo(TraceType, "error = {0}", *error);
                BOOST_REQUIRE(error->IsSuccess());

                if(++(*completeCount) == total)
                {
                    allCompleted->Set();
                }
            });
        }

        BOOST_REQUIRE(allCompleted->WaitOne(TimeSpan::FromSeconds(30)));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(LateWait)
    {
        ENTER;

        vector<pid_t> children;
        for(uint i = 0; i < 3; ++i)
        {
            auto childPid = fork();
            if (childPid == 0)
            {
                _exit(0);
            }

            children.push_back(childPid);
            Trace.WriteInfo(TraceType, "created child process {0}", childPid);
        }

        for(auto const childPid : children)
        {
            auto childExit = make_shared<AutoResetEvent>(false);
            auto error = make_shared<ErrorCode>();

            auto pwait = ProcessWait::CreateAndStart(
                childPid,
                [=] (pid_t, ErrorCode const & err, DWORD)
                {
                    *error = err;
                    childExit->Set();
                });

            BOOST_REQUIRE(childExit->WaitOne(TimeSpan::FromSeconds(3)));
            Trace.WriteInfo(TraceType, "error = {0}", *error);
            BOOST_REQUIRE(error->IsSuccess());
        }

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(DuplicateWait)
    {
        ENTER;

        auto childPid = fork();
        const int exitCode = 2; 
        if (childPid == 0)
        {
            _exit(exitCode);
        }

        for (int i = 0; i < 2; ++i)
        {
            Trace.WriteInfo(TraceType, "created child process {0}", childPid);

            auto childExit = make_shared<AutoResetEvent>(false);
            auto errorReported = make_shared<ErrorCode>();
            auto exitCodeReported = make_shared<DWORD>(0);
            auto pidReported = make_shared<pid_t>(0);

            auto pwait = ProcessWait::CreateAndStart(
                childPid,
                [=] (pid_t pid, ErrorCode const & err, DWORD exitCode)
                {
                    *pidReported = pid;
                    *errorReported = err;
                    *exitCodeReported = exitCode;
                    childExit->Set();
                });

            BOOST_REQUIRE(childExit->WaitOne(TimeSpan::FromSeconds(3)));
            Trace.WriteInfo(TraceType, "errorReported = {0}", *errorReported);
            VERIFY_ARE_EQUAL2(childPid, *pidReported);
            BOOST_REQUIRE(errorReported->IsSuccess());
            BOOST_REQUIRE(WIFEXITED(*exitCodeReported));
            VERIFY_ARE_EQUAL2(WEXITSTATUS(*exitCodeReported), exitCode);
        }

        LEAVE;

    }

    BOOST_AUTO_TEST_CASE(SizeLimit)
    {
        ENTER;
        
        ProcessWait::Test_Reset();

        auto saved = CommonConfig::GetConfig().ProcessExitCacheSizeLimit;
        KFinally([=] { CommonConfig::GetConfig().ProcessExitCacheSizeLimit = saved; });
        const uint sizeLimit = 3;
        CommonConfig::GetConfig().ProcessExitCacheSizeLimit = sizeLimit;

        vector<pid_t> children;
        const int delta = 1;
        for(uint i = 0; i < sizeLimit + delta; ++i) 
        {
            auto childPid = fork();
            if (childPid == 0)
            {
                _exit(0);
            }

            children.push_back(childPid);
            Trace.WriteInfo(TraceType, "created child process {0}", childPid);
        }

        auto successCount = make_shared<atomic_uint>(0);
        for(auto const childPid : children)
        {
            auto childExit = make_shared<AutoResetEvent>(false);
            auto waitResult = make_shared<ErrorCode>();

            auto pwait = ProcessWait::CreateAndStart(
                childPid,
                [=] (pid_t, ErrorCode const & err, DWORD)
                {
                    *waitResult = err;
                    childExit->Set();
                });

            if (childExit->WaitOne(TimeSpan::FromSeconds(2)) && waitResult->IsSuccess())
            {
                ++*successCount;
            }
        }
        
        Trace.WriteInfo(TraceType, "first round wait should all succeed");
        VERIFY_ARE_EQUAL2(successCount->load(), children.size());

        auto successCount2 = make_shared<atomic_uint>(0);
        for(auto const childPid : children)
        {
            auto childExit = make_shared<AutoResetEvent>(false);
            auto waitResult = make_shared<ErrorCode>();

            auto pwait = ProcessWait::CreateAndStart(
                childPid,
                [=] (pid_t, ErrorCode const & err, DWORD)
                {
                    Trace.WriteInfo(TraceType, "second round wait: {0}", err);
                    *waitResult = err;
                    childExit->Set();
                });

            if (childExit->WaitOne(TimeSpan::FromSeconds(2)) && waitResult->IsSuccess())
            {
                Trace.WriteInfo(TraceType, "second round success"); 
                ++(*successCount2);
            }
        }
 
        Trace.WriteInfo(TraceType, "second round wait should see {0} failure", delta);
        VERIFY_ARE_EQUAL2(successCount2->load(), children.size() - delta);
        VERIFY_ARE_EQUAL2(ProcessWait::Test_CacheSize(), sizeLimit);

        LEAVE;
    }

    void AgeLimitTest(TimeSpan ageLimit)
    {
        ProcessWait::Test_Reset();

        auto saved = CommonConfig::GetConfig().ProcessExitCacheAgeLimit;
        KFinally([=] { CommonConfig::GetConfig().ProcessExitCacheAgeLimit = saved; });
        CommonConfig::GetConfig().ProcessExitCacheAgeLimit = ageLimit;

        vector<pid_t> children;
        for(uint i = 0; i < 5; ++i) 
        {
            auto childPid = fork();
            if (childPid == 0)
            {
                _exit(0);
            }

            children.push_back(childPid);
            Trace.WriteInfo(TraceType, "created child process {0}", childPid);
        }

        auto successCount = make_shared<atomic_uint>(0);
        for(auto const childPid : children)
        {
            auto childExit = make_shared<AutoResetEvent>(false);
            auto waitResult = make_shared<ErrorCode>();
            auto pwait = ProcessWait::CreateAndStart(
                childPid,
                [=] (pid_t, ErrorCode const & err, DWORD)
                {
                    *waitResult = err;
                    childExit->Set();
                });

            if (childExit->WaitOne(TimeSpan::FromSeconds(1)) && waitResult->IsSuccess())
            {
                ++*successCount;
            }
        }

        Trace.WriteInfo(TraceType, "first round wait should all succeed");
        VERIFY_ARE_EQUAL2(successCount->load(), children.size()); 

        AutoResetEvent(false).WaitOne(ageLimit + TimeSpan::FromSeconds(1));

        successCount->store(0);
        for(auto const childPid : children)
        {
            auto childExit = make_shared<AutoResetEvent>(false);
            auto waitResult = make_shared<ErrorCode>();
            auto pwait = ProcessWait::CreateAndStart(
                childPid,
                [=] (pid_t, ErrorCode const & err, DWORD)
                {
                    *waitResult = err;
                    childExit->Set();
                });

            if (childExit->WaitOne(TimeSpan::FromMilliseconds(10)) && waitResult->IsSuccess())
            {
                ++*successCount;
            }
        }

        Trace.WriteInfo(TraceType, "second round wait should all fail due to event aging");
        VERIFY_ARE_EQUAL2(successCount->load(), 0);
        VERIFY_ARE_EQUAL2(ProcessWait::Test_CacheSize(), 0);
    }

    BOOST_AUTO_TEST_CASE(AgeLimit)
    {
        ENTER;
        AgeLimitTest(TimeSpan::Zero);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(AgeLimit2)
    {
        ENTER;
        AgeLimitTest(TimeSpan::FromMilliseconds(1));
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(AgeLimit3)
    {
        ENTER;
        AgeLimitTest(TimeSpan::FromSeconds(1));
        LEAVE;
    }

    BOOST_AUTO_TEST_SUITE_END()
}
