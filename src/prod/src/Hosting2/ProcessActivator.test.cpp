// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "FabricRuntime.h"
#include "Common/Common.h"
#include "Hosting2/FabricNodeHost.Test.h"


using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Transport;
using namespace Fabric;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;

const StringLiteral TraceType("ProcessActivatorTest");

class ProcessActivatorTest
{
protected:
    // This test should be run with elevated Admin access
    // Setup checks that the process runs elevated; otherwise, it fails;
	ProcessActivatorTest()
		: fabricNodeHost_(make_shared<TestFabricNodeHost>())
		, maxOpenRetryCount_(3)
		, openRetryDelayInMilliSec_(2000)
	{ 
		
		BOOST_REQUIRE(Setup()); 
	}

    TEST_CLASS_SETUP(Setup);
    ~ProcessActivatorTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup);

    //Disabling this test till we resolve 1226706
    //TEST_METHOD(ProcessRedirectionFileSizeLarge);
    static wstring GetUnitTestFolder();
    static bool FileStartsWith(File & file, wstring const & searchString);
    static void DeactivateProcessTest(shared_ptr<TestFabricNodeHost> const & fabricNodeHost, bool forceTerminateProcess);
    static ErrorCode CreateProcessSyncEvent(__out HandleUPtr & eventHandle);
    static void OpenFileWithRetry(int retryMaxCount, std::wstring const & fileName, File & fileHandle);
    static void ProcessRedirectionFileSizeTest(shared_ptr<TestFabricNodeHost> const & fabricNodeHost, int maxSize);
    static wstring ProcessSyncEventName;

	int maxOpenRetryCount_;
	int openRetryDelayInMilliSec_;
	shared_ptr<TestFabricNodeHost> fabricNodeHost_;
};

wstring ProcessActivatorTest::ProcessSyncEventName = L"HostingProcessSync";


BOOST_FIXTURE_TEST_SUITE(ProcessActivatorTestSuite,ProcessActivatorTest)

BOOST_AUTO_TEST_CASE(ProcessRedirectionFileRetention)
{    
	Trace.WriteInfo(TraceType, "BEGIN TEST: ProcessRedirectionFileRetention");

    int retentionCount = 3;
    LONG maxSize = 20480;
    HostingConfig::GetConfig().EnableActivateNoWindow = true;

    wstring unitTestsFolder = ProcessActivatorTest::GetUnitTestFolder();
    wstring processName = Path::Combine(unitTestsFolder, L"Hosting2.TestProcess.exe");
    wstring redirectionFileName = L"Hosting2TestProcessLog";
    
    Hosting2::ProcessActivator pa(
        *fabricNodeHost_->GetFabricNode()->CreateComponentRoot());
    
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
    for (int i = 0 ; i<5; i++)
    {
        AsyncOperationWaiterSPtr activateWaiter = make_shared<AsyncOperationWaiter>();
        IProcessActivationContextSPtr processActivationContext;
        auto async = pa.BeginActivate(
            nullptr, 
            pd, 
            fabricNodeHost_->GetHosting().FabricBinFolder,
            [](pid_t, Common::ErrorCode const &, DWORD) {},
            TimeSpan::MaxValue,
            [activateWaiter, &pa, &processActivationContext](AsyncOperationSPtr const & operation)
            { activateWaiter->SetError(pa.EndActivate(operation, processActivationContext)); activateWaiter->Set(); },
            fabricNodeHost_->GetFabricNode()->CreateAsyncOperationRoot());
        
        activateWaiter->WaitOne();
        VERIFY_IS_TRUE(activateWaiter->GetError().IsSuccess());
        processActivationContext->TerminateAndCleanup(false, 0);
    }

    // Look for files
    wstring pattern = redirectionFileName;
    pattern.append(L"*.out");

    // Verify that the files generated are not more than the retention count.
    //
    auto files = Common::Directory::Find(logFolder, pattern, 5, false);
    Trace.WriteInfo(TraceType, "Found {0} files. Expected {1}", files.size(), retentionCount);
    for (auto const & file : files)
    {
        Trace.WriteInfo(TraceType, "Found {0}", file);
    }
    VERIFY_IS_TRUE((files.size() == static_cast<size_t>(retentionCount)), L"Found different number files than expected");

	Trace.WriteInfo(TraceType, "END TEST: ProcessRedirectionFileRetention");
}

BOOST_AUTO_TEST_CASE(ProcessRedirectionFileSize)
{
	Trace.WriteInfo(TraceType, "BEGIN TEST: ProcessRedirectionFileSize");

    ProcessRedirectionFileSizeTest(fabricNodeHost_, 5);

	Trace.WriteInfo(TraceType, "END TEST: ProcessRedirectionFileSize");
}

//Disabling this test till we resolve 1226706
//void ProcessActivatorTest::ProcessRedirectionFileSizeLarge()
//{
//    ProcessRedirectionFileSizeTest(1024);
//}

BOOST_AUTO_TEST_CASE(ProcessDeactivationCtrlC)
{
	Trace.WriteInfo(TraceType, "BEGIN TEST: ProcessDeactivationCtrlC");

    DeactivateProcessTest(fabricNodeHost_, false);

	Trace.WriteInfo(TraceType, "END TEST: ProcessDeactivationCtrlC");
}

BOOST_AUTO_TEST_CASE(ProcessDeactivationForce)
{
	Trace.WriteInfo(TraceType, "BEGIN TEST: ProcessDeactivationForce");

    DeactivateProcessTest(fabricNodeHost_, true);

	Trace.WriteInfo(TraceType, "END TEST: ProcessDeactivationForce");
}

BOOST_AUTO_TEST_SUITE_END()

bool ProcessActivatorTest::Setup()
{
	for (int retryCount = 1; retryCount <= maxOpenRetryCount_; retryCount++)
	{
		if (fabricNodeHost_->Open())
		{
			return true;
		}

		Sleep(openRetryDelayInMilliSec_);
	}
	
	return false;
}

bool ProcessActivatorTest::Cleanup()
{
   return fabricNodeHost_->Close();
}

wstring ProcessActivatorTest::GetUnitTestFolder()
{
    wstring unitTestsFolder;
#if !defined(PLATFORM_UNIX)
    VERIFY_IS_TRUE(Environment::ExpandEnvironmentStringsW(L"%_NTTREE%\\FabricUnitTests", unitTestsFolder));
#else
    VERIFY_IS_TRUE(Environment::ExpandEnvironmentStringsW(L"%_NTTREE%/test", unitTestsFolder));
#endif
	return unitTestsFolder;
}

void ProcessActivatorTest::ProcessRedirectionFileSizeTest(shared_ptr<TestFabricNodeHost> const & fabricNodeHost, int maxSize)
{
    int retentionCount = 1;
    HostingConfig::GetConfig().EnableActivateNoWindow = true;

    wstring unitTestsFolder = ProcessActivatorTest::GetUnitTestFolder();
    wstring processName = Path::Combine(unitTestsFolder, L"Hosting2.TestProcess.exe");
    wstring redirectionFileName = L"Hosting2TestProcessLogFileSize";
    
    Hosting2::ProcessActivator pa(
        *fabricNodeHost->GetFabricNode()->CreateComponentRoot());    

    // ProcessDescription
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
    
    HandleUPtr eventHandle;
    {
        // Create event to sync the host process.
        VERIFY_IS_TRUE(ProcessActivatorTest::CreateProcessSyncEvent(eventHandle).IsSuccess());

        // Activate process
        AsyncOperationWaiterSPtr activateWaiter = make_shared<AsyncOperationWaiter>();
        IProcessActivationContextSPtr processActivationContext;
        auto async = pa.BeginActivate(
            nullptr, 
            pd, 
            fabricNodeHost->GetHosting().FabricBinFolder,
            [](pid_t, Common::ErrorCode const &, DWORD) {},
            TimeSpan::MaxValue,
            [activateWaiter, &pa, &processActivationContext](AsyncOperationSPtr const & operation)
            { activateWaiter->SetError(pa.EndActivate(operation, processActivationContext)); activateWaiter->Set(); },
            fabricNodeHost->GetFabricNode()->CreateAsyncOperationRoot());        
        activateWaiter->WaitOne();
        VERIFY_IS_TRUE(activateWaiter->GetError().IsSuccess());

        // Wait for the process to start-up and write to console
        auto waitResult = ::WaitForSingleObject(eventHandle->Value, 10000);
        VERIFY_IS_TRUE(
            WAIT_OBJECT_0 == waitResult, 
            wformatString("Actual wait result = {0}", waitResult).c_str());
    }
    
    // Look for the out files
    wstring pattern = redirectionFileName;
    pattern.append(L"*.out");

    vector<wstring> files = Common::Directory::Find(logFolder, pattern, 1, false);
    Trace.WriteInfo(TraceType, "Found {0} files. ", files.size());

    // for small size, the file will be overwritten. Hence there is a possiblity that
    // we have the file deleted or 0 bytes
    if (maxSize < 50)
    {
        // There is a possiblity that we closed just after deleting. In that case this 0 is acceptable
        VERIFY_IS_TRUE((files.size() <= 1), wformatString("Found {0} files", files.size()).c_str());
        if (files.size() == 0) { return; }
    
        File file;
        OpenFileWithRetry(5, files[0], file);

        // Verify that the file size is within the limit
        // It can be zero if we closed just after creating a new file
        DWORD size = ::GetFileSize(file.GetHandle(), nullptr);
        VERIFY_IS_TRUE(
            (size < (DWORD)(maxSize * 1024)), 
            wformatString("File size is = {0}", size).c_str());        
    }
    else
    {
        VERIFY_IS_TRUE((files.size() == 1), wformatString("Found {0} files", files.size()).c_str());

        DWORD size = 0;
        int retryCount = 0;
        File file;
        OpenFileWithRetry(5, files[0], file);
        
        while (++retryCount < 5)
        {
            // At times the file is not flushed. So the size is 0. Retry in that case.        
            size = ::GetFileSize(file.GetHandle(), nullptr);
            if (size > 0) break;
            Sleep(500);
        }

        VERIFY_IS_TRUE(
            (size < (DWORD)(maxSize * 1024) && size > 0), 
            wformatString("File size is = {0}", size).c_str());

        // Verify the content
        VERIFY_IS_TRUE(ProcessActivatorTest::FileStartsWith(file, L"Writing test data to console"), L"Does not contain string");
    }

    VERIFY_IS_TRUE(::ResetEvent(eventHandle->Value) == TRUE ? true : false);
}

void ProcessActivatorTest::OpenFileWithRetry(int retryMaxCount, std::wstring const & fileName, File & fileHandle)
{
    int retryCount = 0;
    while (++retryCount < retryMaxCount)
    {
        auto error = fileHandle.TryOpen(fileName, FileMode::Open, FileAccess::Read, FileShare::Read);
        if (error.IsSuccess())
        {
            Trace.WriteInfo(TraceType, "Opened {0}: {1}", fileName, error);
            break;
        }
        else
        {
            Trace.WriteWarning(TraceType, "Failed to open {0}: {1}", fileName, error);
            Sleep(1000);
            continue;
        }
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


void ProcessActivatorTest::DeactivateProcessTest(shared_ptr<TestFabricNodeHost> const & fabricNodeHost, bool forceTerminateProcess)
{
    HostingConfig::GetConfig().EnableActivateNoWindow = true;

    wstring unitTestsFolder = ProcessActivatorTest::GetUnitTestFolder();
    wstring processName = Path::Combine(unitTestsFolder, L"Hosting2.TestProcess.exe");
    wstring argName = forceTerminateProcess ? L"CtrlCBlock" : L"CtrlCExit";
    wstring redirectionFileName = wformatString("{0}_{1}", L"Hosting2TestCtrlC", argName);
 
    // Create process description
    Hosting2::ProcessActivator pa(
        *fabricNodeHost->GetFabricNode()->CreateComponentRoot());
    
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
    HandleUPtr eventHandle;
    VERIFY_IS_TRUE(ProcessActivatorTest::CreateProcessSyncEvent(eventHandle).IsSuccess());

    // Create process
    AsyncOperationWaiterSPtr activateWaiter = make_shared<AsyncOperationWaiter>();
    IProcessActivationContextSPtr processActivationContext;
    auto async = pa.BeginActivate(
        nullptr, 
        pd, 
        fabricNodeHost->GetHosting().FabricBinFolder,
        [](pid_t, Common::ErrorCode const &, DWORD) {},
        TimeSpan::MaxValue,
        [activateWaiter, &pa, &processActivationContext](AsyncOperationSPtr const & operation)
        { activateWaiter->SetError(pa.EndActivate(operation, processActivationContext)); activateWaiter->Set(); },
        fabricNodeHost->GetFabricNode()->CreateAsyncOperationRoot());        
    activateWaiter->WaitOne();
    VERIFY_IS_TRUE(activateWaiter->GetError().IsSuccess());

    // Wait for the event that was created previously. Hosting2.TestProcess.exe will set
    // the event to indicate that it has registerd the ctrl C handler
    auto waitResult = ::WaitForSingleObject(eventHandle->Value, 10000);
    VERIFY_IS_TRUE(
        WAIT_OBJECT_0 == waitResult, 
        wformatString("Actual wait result = {0}", waitResult).c_str());

    // Deactivate the process
    AsyncOperationWaiterSPtr deactivateWaiter = make_shared<AsyncOperationWaiter>();
    auto asyncOp = pa.BeginDeactivate(
        processActivationContext,         
        TimeSpan::FromSeconds(10), 
        [deactivateWaiter, &pa](AsyncOperationSPtr const & operation) 
        { deactivateWaiter->SetError(pa.EndDeactivate(operation)); deactivateWaiter->Set(); },
        fabricNodeHost->GetFabricNode()->CreateAsyncOperationRoot());
    deactivateWaiter->WaitOne();
    VERIFY_IS_TRUE(deactivateWaiter->GetError().IsSuccess());
    
    // Wait for the process to shutdown
    waitResult = ::WaitForSingleObject(static_pointer_cast<ProcessActivationContext>(processActivationContext)->ProcessHandle->Value, 10000);
    VERIFY_IS_TRUE(
        WAIT_OBJECT_0 == waitResult, 
        wformatString("Process did not shut down. Actual wait result = {0}", waitResult).c_str());

    // Verify that the deactivation code is expected
    DWORD processExitCode = 0;    
    VERIFY_IS_TRUE(
        ::GetExitCodeProcess(static_pointer_cast<ProcessActivationContext>(processActivationContext)->ProcessHandle->Value, &processExitCode) == TRUE ? true : false,
        Common::wformatString("Could not retrieve the exit code for process. Error = {0}", ErrorCode::FromWin32Error()).c_str());
    
    DWORD expectedExitCode = (forceTerminateProcess ?  ProcessActivator::ProcessDeactivateExitCode : STATUS_CONTROL_C_EXIT);
    VERIFY_IS_TRUE(
        processExitCode == expectedExitCode, 
        Common::wformatString("Process did not exit with expected exit code. Expected = {0}, Actual = {1}", expectedExitCode, processExitCode).c_str());

    VERIFY_IS_TRUE(deactivateWaiter->WaitOne(15000), L"Deactivate did not finsih in 15 sec");
    VERIFY_IS_TRUE(deactivateWaiter->GetError().IsSuccess(), L"Process did not shutdown");    
}

// Create an event here. It will be opened by the child process.
// Using CreateEvent instead of Common::ManualResetEvent as ManualResetEvent creates
// event with SecurityAttribute as null restricting the event to be inherited.
ErrorCode ProcessActivatorTest::CreateProcessSyncEvent(__out HandleUPtr & eventHandle)
{
    SECURITY_ATTRIBUTES saAttr; 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = true;
    saAttr.lpSecurityDescriptor = NULL; 
    
    eventHandle = make_unique<Handle>(
        ::CreateEventW(
        &saAttr,
        true,
        false,
        ProcessSyncEventName.c_str()));
    return (eventHandle->Value == INVALID_HANDLE_VALUE) ? ErrorCode::FromWin32Error() : ErrorCode::Success();
}
