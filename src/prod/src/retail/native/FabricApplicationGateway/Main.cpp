// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Transport/Throttle.h"

using namespace Common;
using namespace HttpApplicationGateway;
using namespace std;

StringLiteral const TraceTag("FabricApplicationGateway");

const unsigned int ExitCodeOpenTimeout = ErrorCodeValue::Timeout & 0x0000FFFF;
const unsigned int ExitCodeUnhandledException = ERROR_UNHANDLED_EXCEPTION;
const unsigned int ExitCodeSynchronizeAccessFailed = ERROR_ACCESS_DENIED;

HttpApplicationGatewaySPtr gatewaySPtr;
RealConsole console;

void TraceErrorAndExitProcess(string const error, UINT exitCode)
{
    console.WriteLine(error);

    Trace.WriteError(TraceTag, "{0}", error);
    ExitProcess(exitCode);
}

ErrorCode OnOpenComplete(AsyncOperationSPtr const & operation)
{
    ErrorCode e = HttpApplicationGatewayImpl::EndCreateAndOpen(operation, gatewaySPtr);
    if (e.IsSuccess())
    {
        Trace.WriteInfo(TraceTag, "FabricApplicationGateway started");
    }
    else
    {
        TraceErrorAndExitProcess(formatString("FabricApplicationGateway failed with error code = {0}", e), ExitCodeOpenTimeout);
    }
    return e;
}

void CreateAndOpen(AutoResetEvent & openEvent)
{
    FabricNodeConfigSPtr fabricNodeConfig = make_shared<FabricNodeConfig>();

    TimeSpan timeout = TimeSpan::FromMinutes(4);

    HttpApplicationGatewayImpl::BeginCreateAndOpen(
        fabricNodeConfig,
        timeout,
        [&openEvent] (AsyncOperationSPtr const& operation) 
        {
            OnOpenComplete(operation); 
            openEvent.Set(); 
        });
}

void OpenGateway()
{
    console.WriteLine(FabricApplicationGatewayResource::GetResources().Starting);

    Transport::Throttle::Enable();
    AutoResetEvent openEvent;

    Threadpool::Post([&openEvent]()
    {
        CreateAndOpen(openEvent);
    });

    openEvent.WaitOne();
    console.WriteLine(FabricApplicationGatewayResource::GetResources().Started);
    console.WriteLine(FabricApplicationGatewayResource::GetResources().ExitPrompt);
}

void OnCloseCompleted(AsyncOperationSPtr const& operation)
{
    HttpApplicationGatewaySPtr & server = AsyncOperationRoot<HttpApplicationGatewaySPtr>::Get(operation->Parent);
    ErrorCode e = server->EndClose(operation);
    if (e.IsSuccess())
    {
        Trace.WriteNoise(TraceTag, "FabricApplicationGateway close completed successfully");
    }
    else
    {
        Trace.WriteWarning(TraceTag, "FabricApplicationGateway close failed with error {0}", e);
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
        console.WriteLine(FabricApplicationGatewayResource::GetResources().WarnShutdown);
        Trace.WriteWarning(TraceTag, "fabricApplicationgatewaySPtr is not assigned, so open has not completed yet");
        return;
    }

    console.WriteLine(FabricApplicationGatewayResource::GetResources().Closing);

    AutoResetEvent closeEvent;
    Threadpool::Post([&closeEvent]()
    {
        Close(closeEvent);
    });

    closeEvent.WaitOne();
    gatewaySPtr.reset();
    console.WriteLine(FabricApplicationGatewayResource::GetResources().Closed);
}

BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 
    switch( fdwCtrlType ) 
    { 
        // Handle the CTRL-C signal. 
    case CTRL_C_EVENT: 
        console.WriteLine(FabricApplicationGatewayResource::GetResources().CtrlC);
        CloseGateway();
#if defined(PLATFORM_UNIX)
        ExitProcess(ERROR_SUCCESS);
#else
        ExitProcess(ERROR_CONTROL_C_EXIT);
#endif

        // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT: 
        console.WriteLine(FabricApplicationGatewayResource::GetResources().CtrlBreak);
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
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    HeapSetInformation(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);
    CommonConfig::GetConfig(); // load configuration

    try
    {
        OpenGateway();

        SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, true);
        wchar_t singleChar;
        wcin.getline(&singleChar, 1);

        CloseGateway();
    }
    catch (exception const& e)
    {
        Trace.WriteError(TraceTag, "Unhandled exception: {0}", e.what());
        return ExitCodeUnhandledException;
    }
}
