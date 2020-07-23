// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_parameters.hpp>
#include "Common/boost-taef.h"
#include "Hosting2/FabricNodeHost.Test.h"
#include <sys/eventfd.h>

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
private:
    class SyncEventHandle
    {
    public:
        SyncEventHandle()
        {
            syncEventFD_ = eventfd (0, 0);
            VERIFY_IS_TRUE(syncEventFD_ != -1);
        }

        ~SyncEventHandle()
        {
            int res = close(syncEventFD_);
            VERIFY_IS_TRUE(res != -1);
        }

        void WaitForEvent()
        {
            uint64 value;
            int res = read(syncEventFD_, &value, sizeof(value));
            VERIFY_IS_TRUE(res != -1);
        }

        int GetEventFD() const
        {
            return syncEventFD_;
        }

    private:
        int syncEventFD_;
    };

    static void ActivateTestProcess(
        Hosting2::ProcessActivator & pa,
        IProcessActivationContextSPtr & processActivationContext,
        bool processBlocksOnCtrlC,
        ProcessWait::WaitCallback const & processExitCallback,
        Common::EnvironmentMap const & emap = Common::EnvironmentMap());
    static void DeactivateTestProcess(Hosting2::ProcessActivator & pa, IProcessActivationContextSPtr processActivationContext);
    static void GetProcessEnvironment(pid_t pid, Common::EnvironmentMap & emapOut);
    static void ProcessEnvironmentTestHelper(Common::EnvironmentMap const & emapIn, Common::EnvironmentMap & emapOut);

public:
    ProcessActivatorTest();
    bool Setup();
    bool Cleanup();

    //void ProcessRedirectionFileRetention();
    //void ProcessRedirectionFileSize();
    //Disabling this test till we resolve 1226706
    //TEST_METHOD(ProcessRedirectionFileSizeLarge);

    static wstring GetUnitTestFolder();
    static bool FileStartsWith(File & file, wstring const & searchString);
    static void DeactivateProcessTest(bool forceTerminateProcess);
    static void ProcessEnvironmentTest();
    static void OpenFileWithRetry(int retryMaxCount, std::wstring const & fileName, File & fileHandle);
    void ProcessRedirectionFileSizeTest(int maxSize);
};

ProcessActivatorTest::ProcessActivatorTest()
{
}

bool ProcessActivatorTest::Setup()
{
}

bool ProcessActivatorTest::Cleanup()
{
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

void ProcessActivatorTest::ActivateTestProcess(
    Hosting2::ProcessActivator & pa,
    IProcessActivationContextSPtr & processActivationContext,
    bool processBlocksOnCtrlC,
    ProcessWait::WaitCallback const & processExitCallback,
    Common::EnvironmentMap const & emap)
{
    HostingConfig::GetConfig().EnableActivateNoWindow = true;

    wstring unitTestsFolder = ProcessActivatorTest::GetUnitTestFolder();
    wstring processName = Path::Combine(unitTestsFolder, L"Hosting2.TestProcess.exe");
    wstring args = processBlocksOnCtrlC ? L"CtrlCBlock" : L"CtrlCExit";
    wstring redirectionFileName = wformatString("{0}_{1}", L"Hosting2TestCtrlC", args);
    wstring FabricBinFolder = GetUnitTestFolder();
    wstring logFolder = Directory::GetCurrentDirectoryW();

    // Create Event for syncing with the child process.
    // This is needed to ensure that we do not terminate the process before we set the
    // CtrlC handler in the process.
    SyncEventHandle eventHandle;

    // Pass the sync event file descriptor to the
    // child process so that it can signal readiness.
    args += L"\n" + std::to_wstring(eventHandle.GetEventFD());

    // Create process description
    Hosting2::ProcessDescription pd(
            processName,
            args,
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

    // Create process
    AsyncOperationWaiterSPtr activateWaiter = make_shared<AsyncOperationWaiter>();
    auto async = pa.BeginActivate(
            nullptr,
            pd,
            FabricBinFolder,
            processExitCallback,
            TimeSpan::MaxValue,
            [activateWaiter, &pa, &processActivationContext](AsyncOperationSPtr const & operation)
            { activateWaiter->SetError(pa.EndActivate(operation, processActivationContext)); activateWaiter->Set(); },
            pa.Root.CreateAsyncOperationRoot());
    activateWaiter->WaitOne();
    VERIFY_IS_TRUE(activateWaiter->GetError().IsSuccess());

    // Wait for signal from child process indicating readiness.
    eventHandle.WaitForEvent();
}

void ProcessActivatorTest::DeactivateTestProcess(Hosting2::ProcessActivator & pa, IProcessActivationContextSPtr processActivationContext)
{
    AsyncOperationWaiterSPtr deactivateWaiter = make_shared<AsyncOperationWaiter>();
    auto asyncOp = pa.BeginDeactivate(
            processActivationContext,
            TimeSpan::FromSeconds(8),
            [deactivateWaiter, &pa](AsyncOperationSPtr const & operation)
            { deactivateWaiter->SetError(pa.EndDeactivate(operation)); deactivateWaiter->Set(); },
            pa.Root.CreateAsyncOperationRoot());
    deactivateWaiter->WaitOne();
    VERIFY_IS_TRUE(deactivateWaiter->GetError().IsSuccess());

    AutoResetEvent().WaitOne(TimeSpan::FromSeconds(3));
}

void ProcessActivatorTest::DeactivateProcessTest(bool forceTerminateProcess)
{
    ComponentRootSPtr pRoot = make_shared<Common::ComponentRoot>();
    Hosting2::ProcessActivator pa(*pRoot);
    IProcessActivationContextSPtr processActivationContext;

    volatile int procExited = 0;
    ProcessWait::WaitCallback const processExitCallback =
        [&procExited, forceTerminateProcess](pid_t pid, Common::ErrorCode const & errorCode, DWORD exitCode)
        {
            // Verify Process Wait Exit Code (ProcessWaitImpl TimedAsyncOperation).
            VERIFY_IS_TRUE(errorCode.IsSuccess());

            // Verify Process Exit Code.
            DWORD expectedExitCode = forceTerminateProcess ? ProcessActivator::ProcessDeactivateExitCode : 0;
            VERIFY_IS_TRUE(exitCode == expectedExitCode);

            InterlockedIncrement(&procExited);
        };

    // Activate the test process.
    ActivateTestProcess(pa, processActivationContext, forceTerminateProcess, processExitCallback);

    // Deactivate the test process.
    DeactivateTestProcess(pa, processActivationContext);

    // Verify process exit callback operation.
    VERIFY_IS_TRUE(procExited == 1);
}

void ProcessActivatorTest::GetProcessEnvironment(pid_t pid, Common::EnvironmentMap & envMap)
{
    // Open /proc/pid/environ for reading.
    std::string envFilePath = "/proc/" + std::to_string(pid) + "/environ";
    std::ifstream envFile(envFilePath.c_str());
    bool fOpen = envFile.is_open();
    VERIFY_IS_TRUE(fOpen);

    // read environment variables.
    char delim = '\0';
    std::string envLine;
    while(std::getline(envFile, envLine, delim))
    {
        // Add environment variable to the map.
        std::wstring envLineW;
        StringUtility::Utf8ToUtf16(envLine, envLineW);
        size_t pos = envLineW.find(L'=');
        if (pos != string::npos)
        {
            wstring key = envLineW.substr(0, pos);
            wstring value = envLineW.substr(pos + 1);
            envMap.insert(make_pair(key, value));
        }
    }
    envFile.close();
}

void ProcessActivatorTest::ProcessEnvironmentTestHelper(Common::EnvironmentMap const & emapIn, Common::EnvironmentMap & emapOut)
{
    ComponentRootSPtr pRoot = make_shared<Common::ComponentRoot>();
    Hosting2::ProcessActivator pa(*pRoot);
    IProcessActivationContextSPtr processActivationContext;

    volatile int procExited = 0;
    ProcessWait::WaitCallback const processExitCallback =
        [&procExited](pid_t pid, Common::ErrorCode const & errorCode, DWORD exitCode)
        {
            // Verify Process Wait Exit Code (ProcessWaitImpl TimedAsyncOperation).
            VERIFY_IS_TRUE(errorCode.IsSuccess());

            // Verify Process Exit Code.
            VERIFY_IS_TRUE(exitCode == 0);

            InterlockedIncrement(&procExited);
        };

    // Activate the test process.
    ActivateTestProcess(pa, processActivationContext, false, processExitCallback, emapIn);

    // Get the Process' environment.
    GetProcessEnvironment(processActivationContext->ProcessId, emapOut);

    // Deactivate the test process.
    DeactivateTestProcess(pa, processActivationContext);

    // Verify process exit callback operation.
    VERIFY_IS_TRUE(procExited == 1);
}

void ProcessActivatorTest::ProcessEnvironmentTest()
{
    // Dummy environment variable.
    std::wstring dummyEnvVarKey     = L"DUMMY_ENV_VAR_KEY";
    std::wstring dummyEnvVarValue   = L"DUMMY_ENV_VAR_VALUE";

    // LD_LIBRARY_PATH specific setup.
    std::wstring ldLibraryPathKey        = L"LD_LIBRARY_PATH";
    std::wstring ldLibraryPathValueGuest = L"/nonexistant/guestenvironment";
    std::wstring ldLibraryPathValueHost  = L"/nonexistant/hostenvironment";

    // Save and clear old value of LD_LIBRARY_PATH before testing
    // in case Hosting2.Test.exe is run with LD_LIBRARY_PATH set.
    std::wstring oldLdLibraryPathValue;
    bool oldLdLibraryPath = Environment::GetEnvironmentVariable(ldLibraryPathKey, oldLdLibraryPathValue, NOTHROW());
    if (oldLdLibraryPath)
    {
        // Clear the existing value.
        VERIFY_IS_TRUE(Environment::SetEnvironmentVariable(ldLibraryPathKey, std::wstring()));
    }

    // 1. Test process activation with empty environment.
    Common::EnvironmentMap emapInLD1;
    Common::EnvironmentMap emapOutLD1;
    ProcessEnvironmentTestHelper(emapInLD1, emapOutLD1);
    VERIFY_IS_TRUE(emapOutLD1.empty());

    // 2. Test process activation with LD_LIBRARY_PATH specified
    // for the guest (i.e. the process we will launch).
    Common::EnvironmentMap emapInLD2;
    Common::EnvironmentMap emapOutLD2;
    emapInLD2[ldLibraryPathKey] = ldLibraryPathValueGuest;
    ProcessEnvironmentTestHelper(emapInLD2, emapOutLD2);
    VERIFY_IS_TRUE(emapOutLD2.size()==1);
    VERIFY_IS_TRUE(emapOutLD2[ldLibraryPathKey].compare(ldLibraryPathValueGuest) == 0);

    // For tests 3 and 4 set LD_LIBRARY_PATH in the host evnironment (i.e. current process).
    VERIFY_IS_TRUE(Environment::SetEnvironmentVariable(ldLibraryPathKey, ldLibraryPathValueHost));

    // 3. Test process activation with LD_LIBRARY_PATH specified in the
    // in the host environment and a non empty guest environment.
    Common::EnvironmentMap emapInLD3;
    Common::EnvironmentMap emapOutLD3;
    emapInLD3[dummyEnvVarKey] = dummyEnvVarValue;
    ProcessEnvironmentTestHelper(emapInLD3, emapOutLD3);
    // We should only have two entries.
    VERIFY_IS_TRUE(emapOutLD3.size()==2);
    VERIFY_IS_TRUE(emapOutLD3[dummyEnvVarKey].compare(dummyEnvVarValue) == 0);
    VERIFY_IS_TRUE(emapOutLD3[ldLibraryPathKey].compare(ldLibraryPathValueHost) == 0);

    // 4. Test process activation with LD_LIBRARY_PATH specified for the guest
    // and set in the host environment as well.
    Common::EnvironmentMap emapInLD4;
    Common::EnvironmentMap emapOutLD4;
    emapInLD4[ldLibraryPathKey] = ldLibraryPathValueGuest;
    ProcessEnvironmentTestHelper(emapInLD4, emapOutLD4);
    // We should only have one entry of LD_LIBRARY_PATH.
    VERIFY_IS_TRUE(emapOutLD4.size()==1);
    std::wstring expectedValue = ldLibraryPathValueGuest + L":" + ldLibraryPathValueHost;
    VERIFY_IS_TRUE(emapOutLD4[ldLibraryPathKey].compare(expectedValue) == 0);

    // Unset LD_LIBRARY_PATH in the host evnironment (i.e. current process).
    VERIFY_IS_TRUE(Environment::SetEnvironmentVariable(ldLibraryPathKey, std::wstring()));

    // Restore old LD_LIBRARY_PATH value if needed.
    if (oldLdLibraryPath)
    {
        VERIFY_IS_TRUE(Environment::SetEnvironmentVariable(ldLibraryPathKey, oldLdLibraryPathValue));
    }
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

BOOST_AUTO_TEST_CASE(ProcessEnvironment)
{
    ProcessEnvironmentTest();
}

BOOST_AUTO_TEST_SUITE_END()


//Disabling this test till we resolve 1226706
//void ProcessActivatorTest::ProcessRedirectionFileSizeLarge()
//{
//    ProcessRedirectionFileSizeTest(1024);
//}

