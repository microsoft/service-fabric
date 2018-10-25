// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Transport/Throttle.h"

using namespace Common;
using namespace FabricGateway;
using namespace std;

StringLiteral const TraceTag("FabricGateway");

const unsigned int ExitCodeOpenTimeout = ErrorCodeValue::Timeout & 0x0000FFFF;
const unsigned int ExitCodeUnhandledException = ERROR_UNHANDLED_EXCEPTION;
const unsigned int ExitCodeSynchronizeAccessFailed = ERROR_ACCESS_DENIED;

GatewaySPtr gatewaySPtr;
RealConsole console;

void TraceErrorAndExitProcess(string const error, UINT exitCode)
{
    console.WriteLine(error);

    Trace.WriteError(TraceTag, "{0}", error);
    ExitProcess(exitCode);
}

ErrorCode OnOpenComplete(AsyncOperationSPtr const & operation)
{
    ErrorCode e = Gateway::EndCreateAndOpen(operation, gatewaySPtr);
    if (e.IsSuccess())
    {
        Trace.WriteInfo(TraceTag, "FabricGateway started");
    }
    else
    {
        TraceErrorAndExitProcess(formatString("FabricGateway failed with error code = {0}", e), ExitCodeOpenTimeout);
    }
    return e;
}

void CreateAndOpen(AutoResetEvent & openEvent, wstring const &ipcListenAddress)
{
    Gateway::BeginCreateAndOpen(
        ipcListenAddress,
        [](ErrorCode const & error) { TraceErrorAndExitProcess(formatString("FabricGateway failed with error code = {0}", error), error.ToHResult()); },
        [&openEvent] (AsyncOperationSPtr const& operation) 
        {
            OnOpenComplete(operation); 
            openEvent.Set(); 
        });
}

void OpenGateway(wstring const &ipcListenAddress)
{
    console.WriteLine(FabricGatewayResource::GetResources().Starting);

    Transport::Throttle::Enable();
    AutoResetEvent openEvent;

    Threadpool::Post([&openEvent, ipcListenAddress]()
    {
        CreateAndOpen(openEvent, ipcListenAddress);
    });

    openEvent.WaitOne();
    console.WriteLine(FabricGatewayResource::GetResources().Started);
#if !defined(PLATFORM_UNIX)
    console.WriteLine(FabricGatewayResource::GetResources().ExitPrompt);
#endif
}

void OnCloseCompleted(AsyncOperationSPtr const& operation)
{
    GatewaySPtr & server = AsyncOperationRoot<GatewaySPtr>::Get(operation->Parent);
    ErrorCode e = server->EndClose(operation);
    if (e.IsSuccess())
    {
        Trace.WriteNoise(TraceTag, "FabricGateway close completed successfully");
    }
    else
    {
        Trace.WriteWarning(TraceTag, "FabricGateway close failed with error {0}", e);
    }
}

void Close(AutoResetEvent & closeEvent)
{
    gatewaySPtr->BeginClose(
        TimeSpan::MaxValue,
        [&closeEvent] (AsyncOperationSPtr const& operation) 
    {
        OnCloseCompleted(operation); 
        closeEvent.Set();
    },
    gatewaySPtr->CreateAsyncOperationRoot());
}

void CloseGateway()
{
    if (!gatewaySPtr)
    {
        console.WriteLine(FabricGatewayResource::GetResources().WarnShutdown);
        Trace.WriteWarning(TraceTag, "fabricgatewaySPtr is not assigned, so open has not completed yet");
        return;
    }

    console.WriteLine(FabricGatewayResource::GetResources().Closing);

    AutoResetEvent closeEvent;
    Threadpool::Post([&closeEvent]()
    {
        Close(closeEvent);
    });

    closeEvent.WaitOne();
    gatewaySPtr.reset();
    console.WriteLine(FabricGatewayResource::GetResources().Closed);
}

BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 
    switch( fdwCtrlType ) 
    { 
        // Handle the CTRL-C signal. 
    case CTRL_C_EVENT: 
        console.WriteLine(FabricGatewayResource::GetResources().CtrlC);
        CloseGateway();
#if defined(PLATFORM_UNIX)
        ExitProcess(ERROR_SUCCESS);
#else
        ExitProcess(ERROR_CONTROL_C_EXIT);
#endif

        // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT: 
        console.WriteLine(FabricGatewayResource::GetResources().CtrlBreak);
        CloseGateway();
#if defined(PLATFORM_UNIX)
        ExitProcess(ERROR_SUCCESS);
#else
        ExitProcess(ERROR_CONTROL_C_EXIT);
#endif

    default: 
        return FALSE; 
    } 
} 

int main(int argc, __in_ecount(argc) char* argv[])
{
    HeapSetInformation(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);
    CommonConfig::GetConfig(); // load configuration

    try
    {
        if (argc < 2)
        {
            Trace.WriteError(TraceTag, "Port argument missing");
            return 1;

        }

        StringCollection args;
        for (int i = 0; i < argc; i++)
        {
            args.push_back(wstring(argv[i], argv[i] + strlen(argv[i])));
        }
        
        OpenGateway(args[1]);

        SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, true);
#if !defined(PLATFORM_UNIX)
        wchar_t singleChar;
        wcin.getline(&singleChar, 1);
#else
        do
        {
            pause();
        } while(errno == EINTR);
#endif

        CloseGateway();
    }
    catch (exception const& e)
    {
        Trace.WriteError(TraceTag, "Unhandled exception: {0}", e.what());
        return ExitCodeUnhandledException;
    }
}
