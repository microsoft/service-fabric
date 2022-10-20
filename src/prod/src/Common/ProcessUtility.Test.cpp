// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Common
{
    using namespace std;

    const StringLiteral TestSource = "ProcessUtilityTest";

    class ProcessUtilityTest
    {
        //TestRoot root_;
    };

    // tests end to end activation using the process utility functions
    // creates a job, creates process, assigns process to the job, 
    // monitors and kills the process by closing the job object.
    BOOST_FIXTURE_TEST_SUITE(ProcessUtilityTestSuite,ProcessUtilityTest)

    BOOST_AUTO_TEST_CASE(BasicTest)
    {
        HandleUPtr jobHandle;
        HandleUPtr processHandle;
        HandleUPtr threadHandle;
        Common::ManualResetEvent waitEvent(false);
        ThreadpoolWaitSPtr processMonitor;
        
        // create job object
        {
            Trace.WriteInfo(TestSource, "Creating annonymous job.");

            auto success = ProcessUtility::CreateAnnonymousJob(true, jobHandle);
            VERIFY_IS_TRUE(success.IsSuccess(), L"CreateAnnonymousJob");
        }

        // create process
        {
            Trace.WriteInfo(TestSource, "Creating process");
            
            vector<wchar_t> envBlock(0);
            auto notepadExe = Environment::Expand(L"%windir%\\System32\\notepad.exe");
            Trace.WriteError(TestSource, "{0}", notepadExe);
            auto success = ProcessUtility::CreateProcessW(
                notepadExe,
                L"",
                envBlock,
                ProcessUtility::CreationFlags_SuspendedProcessWithJobBreakaway,
                processHandle,
                threadHandle);
            VERIFY_IS_TRUE(success.IsSuccess(), wformatString("CreateProcess {0}", notepadExe).c_str());
        }

        // associate job to the process
        {
             Trace.WriteInfo(TestSource, "Associating job to the process");
             
             auto success = ProcessUtility::AssociateProcessToJob(
                 jobHandle,
                 processHandle);

             VERIFY_IS_TRUE(success.IsSuccess(), L"AssociateProcessToJob");
        }

        // start process monitoring
        {
             processMonitor = Common::ThreadpoolWait::Create(
                Handle(*processHandle, Handle::DUPLICATE()),
                [&waitEvent](Handle const &, ErrorCode const &) { waitEvent.Set(); });
        }

        // start the process
        {
            Trace.WriteInfo(TestSource, "Resuming the process");
            auto success = ProcessUtility::ResumeProcess(processHandle, threadHandle);

            VERIFY_IS_TRUE(success.IsSuccess(), L"ResumeProcess");
        }

        // close the job handle, the process should close and wait event should fire.
        jobHandle.reset();
        VERIFY_IS_TRUE(waitEvent.WaitOne(5000), L"Job handle close should terminate the process.");
    }

    BOOST_AUTO_TEST_SUITE_END()
}





