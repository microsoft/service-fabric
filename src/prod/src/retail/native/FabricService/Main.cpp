// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Transport/Throttle.h"
#include "Hosting2/LeaseMonitor.h"

using namespace Common;
using namespace ServiceModel;
using namespace Fabric;
using namespace std;
#if defined(PLATFORM_UNIX)
#include <signal.h>
#include <sys/stat.h>

struct sigaction act;

void sighandler(int signum, siginfo_t *info, void *ptr)
{
    printf(">> Signal: %d\n", signum);
    printf(">> From process: %lu\n", (unsigned long)info->si_pid);
}

#endif
StringLiteral const TraceTag("FabricExe");

const unsigned int ExitCodeOpenTimeout = ErrorCodeValue::Timeout & 0x0000FFFF;
const unsigned int ExitCodeNodeFailed = ErrorCodeValue::OperationFailed & 0x0000FFFF;
const unsigned int ExitCodeUnhandledException = ERROR_UNHANDLED_EXCEPTION;
const unsigned int ExitCodeMissingConfigurationFile = ErrorCodeValue::InvalidConfiguration & 0x0000FFFF;
const unsigned int ExitCodeUsageError = ERROR_INVALID_PARAMETER;
const unsigned int ExitCodeSynchronizeAccessFailed = ERROR_ACCESS_DENIED;
const unsigned int ExitCodeVersionNotMatch = ERROR_CLUSTER_INCOMPATIBLE_VERSIONS;

FabricNodeSPtr fabricNodeSPtr;
RealConsole console;

void TraceErrorAndExitProcess(string const error, UINT exitCode)
{
    console.WriteLine(error);

    FabricNodeConfigSPtr config = std::make_shared<FabricNodeConfig>();
    if (config->AssertOnNodeFailure)
    {
        Assert::CodingError("Node failed with error {0}", error);
    }
    else
    {
        Trace.WriteError(TraceTag, "{0}", error);
        ExitProcess(exitCode);
    }
}

ErrorCode OnOpenComplete(AsyncOperationSPtr const & operation)
{
    ErrorCode e = FabricNode::EndCreateAndOpen(operation, fabricNodeSPtr);
    if (e.IsSuccess())
    {
        Trace.WriteInfo(TraceTag, "Fabric Node opened");
    }
    else
    {
        TraceErrorAndExitProcess(formatString("Fabric Node open failed with error code = {0}", e), ExitCodeOpenTimeout);
    }
    return e;
}

void CreateAndOpen(AutoResetEvent & openEvent, FabricNodeConfigSPtr config)
{  
    FabricNode::BeginCreateAndOpen(
        config,
        [&openEvent] (AsyncOperationSPtr const& operation) 
        {
            OnOpenComplete(operation); 
            openEvent.Set(); 
        },
        [] (EventArgs const & ) { TraceErrorAndExitProcess("Fabric Node has failed", ExitCodeNodeFailed); },
        [] (EventArgs const & ) { console.WriteLine(FabricServiceResource::GetResources().FMReady); Trace.WriteInfo(TraceTag, "FailoverManager is ready!"); }
    );
}

unsigned int OpenFabricNode()
{
    console.WriteLine(FabricServiceResource::GetResources().Opening);

    FabricNodeConfigSPtr config = make_shared<FabricNodeConfig>();
    FabricCodeVersion codeVersion = config->NodeVersion.Version.CodeVersion;

#if !defined(PLATFORM_UNIX)
    unsigned __int64  execVersion = Common::FileVersion::GetCurrentExeVersionNumber();         
    Trace.WriteInfo(TraceTag, "requested version: {0}", codeVersion);
    Trace.WriteInfo(TraceTag, "executable version: {0}", Common::FileVersion::NumberVersionToString(execVersion));
    
    if ( codeVersion != *(FabricCodeVersion::Default)    //don't assert default request version.
       && 
         !( HIWORD(HIDWORD(execVersion)) == codeVersion.MajorVersion
         && LOWORD(HIDWORD(execVersion)) == codeVersion.MinorVersion
         && HIWORD(LODWORD(execVersion)) == codeVersion.BuildVersion
         && LOWORD(LODWORD(execVersion)) == codeVersion.HotfixVersion) )
    {
        console.WriteLine("{0} {1}", FabricServiceResource::GetResources().RequestedVersion, codeVersion);
        console.WriteLine("{0} {1}", FabricServiceResource::GetResources().ExecutableVersion, Common::FileVersion::NumberVersionToString(execVersion));
        Trace.WriteError(TraceTag, "Version doesn't match. Requested: {0}, Executable version: {1}", codeVersion, Common::FileVersion::NumberVersionToString(execVersion));
        Common::Assert::TestAssert("Version doesn't match. Requested: {0}, Executable version: {1}", codeVersion, Common::FileVersion::NumberVersionToString(execVersion));   
        return ExitCodeVersionNotMatch;
    }
#else
    wstring currentVersion = L"";
    auto error = FabricEnvironment::GetFabricVersion(currentVersion);
    if (!error.IsSuccess())
    {
        Trace.WriteError(
            TraceTag,
            "Error {0} when trying to read cluster version",
            error);

        return ExitCodeVersionNotMatch;
    }

    if (codeVersion != *(FabricCodeVersion::Default)//don't assert default request version
        && !StringUtility::AreEqualCaseInsensitive(codeVersion.ToString(), currentVersion))
    {
        console.WriteLine("{0} '{1}'", FabricServiceResource::GetResources().RequestedVersion, codeVersion.ToString());
        console.WriteLine("{0} '{1}'", FabricServiceResource::GetResources().ExecutableVersion, currentVersion);
        Trace.WriteError(TraceTag, "Version doesn't match. Requested: {0}, Executable version: {1}", codeVersion, currentVersion);
        Common::Assert::TestAssert("Version doesn't match. Requested: {0}, Executable version: {1}", codeVersion, currentVersion);
        return ExitCodeVersionNotMatch;
    }

    CommonConfig::GetConfig().PosixTimerLimit = CommonConfig::GetConfig().PosixTimerLimit_Fabric;
#endif

    Transport::Throttle::Enable();
    AutoResetEvent openEvent;

    Threadpool::Post([&openEvent, config]()
    {
        CreateAndOpen(openEvent, config);
    });

    openEvent.WaitOne();
    console.WriteLine(FabricServiceResource::GetResources().Opened);
#if !defined(PLATFORM_UNIX)
    console.WriteLine(FabricServiceResource::GetResources().ExitPrompt);
#endif
    return 0;
}

void OnCloseCompleted(AsyncOperationSPtr const& operation)
{
    FabricNodeSPtr & node = AsyncOperationRoot<FabricNodeSPtr>::Get(operation->Parent);
    ErrorCode e = node->EndClose(operation);
    if (e.IsSuccess())
    {
        Trace.WriteNoise(TraceTag, "FabricNode close completed successfully");
    }
    else
    {
        Trace.WriteWarning(TraceTag, "FabricNode close failed with error {0}", e);
    }
}

void Close(AutoResetEvent & closeEvent)
{
    fabricNodeSPtr->BeginClose(
        [&closeEvent] (AsyncOperationSPtr const& operation) 
    {
        OnCloseCompleted(operation); 
        closeEvent.Set();
    },
        fabricNodeSPtr->CreateAsyncOperationRoot());
}

void CloseFabricNode()
{
    if (!fabricNodeSPtr)
    {
        console.WriteLine(FabricServiceResource::GetResources().WarnShutdown);
        Trace.WriteWarning(TraceTag, "fabricNodeSPtr is not assigned, so open has not completed yet");
        return;
    }

    console.WriteLine(FabricServiceResource::GetResources().Closing);

    AutoResetEvent closeEvent;
    Threadpool::Post([&closeEvent]()
    {
        Close(closeEvent);
    });

    closeEvent.WaitOne();
    fabricNodeSPtr.reset();
    console.WriteLine(FabricServiceResource::GetResources().Closed);
}

// enable synchronize access for everyone so that Fabric process can be watched by 
// FabricRuntime DLL from within process running as any user
void EnableSynchronizeAccessToEveryone()
{
#if !defined(PLATFORM_UNIX)
    auto error = ProcessUtility::EnableSynchronizeAccess();
    if (!error.IsSuccess())
    {    
        TraceErrorAndExitProcess(
            formatString("Failed to EnableSynchronizeAccess (because {0}) for Everyone on Fabric.exe", error),
            ExitCodeSynchronizeAccessFailed);
    }
#endif
}

BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 
    switch( fdwCtrlType ) 
    { 
        // Handle the CTRL-C signal. 
    case CTRL_C_EVENT: 
        console.WriteLine(FabricServiceResource::GetResources().CtrlC);
        CloseFabricNode();
#if defined(PLATFORM_UNIX)
        ExitProcess(ERROR_SUCCESS);
#else
        ExitProcess(ERROR_CONTROL_C_EXIT);
#endif

        // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT: 
        console.WriteLine(FabricServiceResource::GetResources().CtrlBreak);
        CloseFabricNode();
#if defined(PLATFORM_UNIX)
        ExitProcess(ERROR_SUCCESS);
#else
        ExitProcess(ERROR_CONTROL_C_EXIT);
#endif

    default: 
        return FALSE; 
    } 
} 

#if defined(PLATFORM_UNIX)
void terminatewithtrace()
{
    StackTrace stackTrace;
    stackTrace.CaptureCurrentPosition();
    Trace.WriteError(TraceTag, "Unhandled exception: {0}", stackTrace.ToString());
    abort();    
}
#endif

#if !defined(PLATFORM_UNIX)
int __cdecl wmain(int argc, __in_ecount(argc) wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    HeapSetInformation(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);
    CommonConfig::GetConfig(); // load configuration

    Hosting2::LeaseMonitor::DisableIpcLeasePolling();

    try
    {
#if defined(PLATFORM_UNIX)
        memset(&act, 0, sizeof(act));
        act.sa_sigaction = sighandler;
        act.sa_flags = SA_SIGINFO;
        sigaction(SIGHUP, &act, NULL);

        umask(0002);
#endif
        EnableSynchronizeAccessToEveryone();

        unsigned int ret = OpenFabricNode();
        if ( ret!=0 )
        {
            return ret;
        }

        SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, true);
#if !defined(PLATFORM_UNIX)
        wchar_t singleChar;
        wcin.getline(&singleChar, 1);
#else
        //std::set_terminate(terminatewithtrace);
        do
        {
            pause();
        } while(errno == EINTR);
#endif

        CloseFabricNode();
    }
    catch (exception const& e)
    {
        Trace.WriteError(TraceTag, "Unhandled exception: {0}", e.what());
        return ExitCodeUnhandledException;
    }
}
