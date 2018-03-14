// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_parameters.hpp>
#include "Common/boost-taef.h"
#include "Hosting2/FabricNodeHost.Test.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Transport;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;

const StringLiteral TraceType("ProcessActivatorTest");

class ProcessActivatorTest
{
public:
    bool Setup();
    bool Cleanup();

    //void ProcessRedirectionFileRetention();
    //void ProcessRedirectionFileSize();
    //Disabling this test till we resolve 1226706
    //TEST_METHOD(ProcessRedirectionFileSizeLarge);
    //void ProcessDeactivationCtrlC();
    //void ProcessDeactivationForce();

public:
    ProcessActivatorTest();
    static wstring GetUnitTestFolder();
    static bool FileStartsWith(File & file, wstring const & searchString);
//    static void DeactivateProcessTest(shared_ptr<TestFabricNodeHost> const & fabricNodeHost, bool forceTerminateProcess);
    static void DeactivateProcessTest(bool forceTerminateProcess);
    static ErrorCode CreateProcessSyncEvent(__out HandleUPtr & eventHandle);
    static void OpenFileWithRetry(int retryMaxCount, std::wstring const & fileName, File & fileHandle);
    void ProcessRedirectionFileSizeTest(int maxSize);    
    static wstring ProcessSyncEventName;

public:
//    shared_ptr<TestFabricNodeHost> fabricNodeHost_;
};

wstring ProcessActivatorTest::ProcessSyncEventName = L"HostingProcessSync";
ProcessActivatorTest::ProcessActivatorTest()
{
}

bool ProcessActivatorTest::Setup()
{
//    return fabricNodeHost_->Open();
}

bool ProcessActivatorTest::Cleanup()
{
//    return fabricNodeHost_->Close();
}

wstring ProcessActivatorTest::GetUnitTestFolder()
{
    wstring unitTestsFolder;
    Environment::ExpandEnvironmentStringsW(L"%_NTTREE%/test", unitTestsFolder);
    return unitTestsFolder;
}

void ProcessActivatorTest::OpenFileWithRetry(int retryMaxCount, std::wstring const & fileName, File & fileHandle)
{
    int retryCount = 0;
    while (++retryCount < retryMaxCount && !fileHandle.TryOpen(fileName, FileMode::Open, FileAccess::Read, FileShare::Read, FileAttributes::None).IsSuccess())
    {
    }
    VERIFY_IS_TRUE(fileHandle.GetHandle() != INVALID_HANDLE_VALUE);
}

bool ProcessActivatorTest::FileStartsWith(File & file, wstring const & searchString)
{
    char buffer[1000];
    DWORD bytesRead;
    wchar_t wideBuffer[1000];
    file.TryRead2(buffer, 1000, bytesRead);
    ::MultiByteToWideChar(CP_ACP, 0, buffer, bytesRead, wideBuffer, 1000);
    return StringUtility::Contains<wstring>(wstring(wideBuffer), searchString);
}

void ProcessActivatorTest::DeactivateProcessTest(bool forceTerminateProcess)
{
    HostingConfig::GetConfig().EnableActivateNoWindow = true;

    wstring unitTestsFolder = ProcessActivatorTest::GetUnitTestFolder();
    wstring processName = Path::Combine(unitTestsFolder, L"Hosting2.TestProcess.exe");
    wstring argName = forceTerminateProcess ? L"CtrlCBlock" : L"CtrlCExit";
    wstring redirectionFileName = wformatString("{0}_{1}", L"Hosting2TestCtrlC", argName);

    auto pRoot = make_shared<Common::ComponentRoot>();
    wstring FabricBinFolder = GetUnitTestFolder();

    // Create process description
    Hosting2::ProcessActivator pa(*pRoot);

    wstring logFolder = Directory::GetCurrentDirectoryW();

    Common::EnvironmentMap emap;
    Hosting2::ProcessDescription pd(
            processName,
            argName,
            unitTestsFolder,
            emap,
            logFolder,
            logFolder,
            logFolder,
            logFolder,
            false,
            false,
            false,
            false);

    // Create Event for syncing the process.
    // This is needed to ensure that we do not terminate the process before we set the
    // CtrlC handler in the process.
    //HandleUPtr eventHandle;
    //VERIFY_IS_TRUE(ProcessActivatorTest::CreateProcessSyncEvent(eventHandle).IsSuccess());

    volatile int procExited = 0;
    // Create process
    AsyncOperationWaiterSPtr activateWaiter = make_shared<AsyncOperationWaiter>();
    IProcessActivationContextSPtr processActivationContext;
    auto async = pa.BeginActivate(
            nullptr,
            pd,
            FabricBinFolder,
            [&procExited](pid_t, Common::ErrorCode const &, DWORD) { InterlockedIncrement(&procExited); printf("Child Process exited.\n"); },
            TimeSpan::MaxValue,
            [activateWaiter, &pa, &processActivationContext](AsyncOperationSPtr const & operation)
            { activateWaiter->SetError(pa.EndActivate(operation, processActivationContext)); activateWaiter->Set(); },
            pRoot->CreateAsyncOperationRoot());
    activateWaiter->WaitOne();
    VERIFY_IS_TRUE(activateWaiter->GetError().IsSuccess());

    usleep(500000);

    // Deactivate the process
    AsyncOperationWaiterSPtr deactivateWaiter = make_shared<AsyncOperationWaiter>();
    auto asyncOp = pa.BeginDeactivate(
            processActivationContext,
            TimeSpan::FromSeconds(8),
            [deactivateWaiter, &pa](AsyncOperationSPtr const & operation)
            { deactivateWaiter->SetError(pa.EndDeactivate(operation)); deactivateWaiter->Set(); },
            pRoot->CreateAsyncOperationRoot());
    deactivateWaiter->WaitOne();
    VERIFY_IS_TRUE(deactivateWaiter->GetError().IsSuccess());

    AutoResetEvent().WaitOne(TimeSpan::FromSeconds(3));

    VERIFY_IS_TRUE(procExited == 1, wformatString("Process did not shut down. ").c_str());
}

BOOST_FIXTURE_TEST_SUITE(ProcessActivatorTestSuite,ProcessActivatorTest)

/* TODO: Removed console-redirection related test cases for now, 
 * as ProcessActivator does not support yet. 
BOOST_AUTO_TEST_CASE(ProcessRedirectionFileRetention)
{    
    int retentionCount = 5;
    LONG maxSize = 20480;
    HostingConfig::GetConfig().EnableActivateNoWindow = true;

    wstring unitTestsFolder = ProcessActivatorTest::GetUnitTestFolder();
    wstring processName = Path::Combine(unitTestsFolder, L"Hosting2.TestProcess.exe");
    wstring redirectionFileName = L"Hosting2TestProcessLog";

    auto pRoot = make_shared<Common::ComponentRoot>();
    wstring FabricBinFolder = GetUnitTestFolder();

    Hosting2::ProcessActivator pa(*pRoot);

    // Process Description
    wstring logFolder = Directory::GetCurrentDirectoryW();
    
    Common::EnvironmentMap emap;
    Hosting2::ProcessDescription pd(
        processName,
        L"consoleWrite", 
        unitTestsFolder, 
        emap, 
        logFolder,
        logFolder, 
        logFolder,
        logFolder, 
        true,
        true,
        false,
        false,
        redirectionFileName, 
        retentionCount,
        maxSize);
    
    // Activate multiple processes
    volatile int procNum = 1;
    for (int i = 0 ; i < 5; i++)
    {
        AsyncOperationWaiterSPtr activateWaiter = make_shared<AsyncOperationWaiter>();
        IProcessActivationContextSPtr processActivationContext;
        auto async = pa.BeginActivate(
            nullptr,
            pd, 
            FabricBinFolder,
            [&procNum](Common::Handle const & , Common::ErrorCode const & ) { InterlockedDecrement(&procNum); printf("Child Process exited.\n"); },
            TimeSpan::MaxValue,
            [activateWaiter, &pa, &processActivationContext](AsyncOperationSPtr const & operation)
            { activateWaiter->SetError(pa.EndActivate(operation, processActivationContext)); activateWaiter->Set(); },
            pRoot->CreateAsyncOperationRoot());
        
        activateWaiter->WaitOne();
        VERIFY_IS_TRUE(activateWaiter->GetError().IsSuccess());
        while(procNum != 0)
        {
            usleep(100000);
        }
        InterlockedIncrement(&procNum);
        processActivationContext->TerminateAndCleanup(false, 0);
    }

    // Look for files
    wstring outPattern = redirectionFileName;
    outPattern.append(L"*.out*");
    wstring errPattern = redirectionFileName;
    errPattern.append(L"*.err*");

    vector<wstring> outFiles = Common::Directory::Find(logFolder, outPattern, retentionCount, false);
    Trace.WriteInfo(TraceType, "Found {0} files. Expected {1}", outFiles.size(), retentionCount);

    for(wstring outfile : outFiles)
        Common::File::Delete(outfile, false);

    vector<wstring> errFiles = Common::Directory::Find(logFolder, errPattern, retentionCount, false);
    Trace.WriteInfo(TraceType, "Found {0} files. Expected {1}", errFiles.size(), retentionCount);

    for(wstring errfile : errFiles)
        Common::File::Delete(errfile, false);

    // Verify that the files generated are not more than the retention count.
    VERIFY_IS_TRUE((outFiles.size() == static_cast<size_t>(retentionCount)), L"Found different number files than expected");
}

BOOST_AUTO_TEST_CASE(ProcessRedirectionFileSize)
{
    ProcessRedirectionFileSizeTest(5);
}*/

BOOST_AUTO_TEST_CASE(ProcessDeactivationForce)
{
    DeactivateProcessTest(true);
}

BOOST_AUTO_TEST_CASE(ProcessDeactivationCtrlC)
{
    DeactivateProcessTest(false);
}

BOOST_AUTO_TEST_SUITE_END()


//Disabling this test till we resolve 1226706
//void ProcessActivatorTest::ProcessRedirectionFileSizeLarge()
//{
//    ProcessRedirectionFileSizeTest(1024);
//}

