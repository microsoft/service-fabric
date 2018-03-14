// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if defined(PLATFORM_UNIX)
#include <sys/stat.h>
#endif

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management::FileStoreService;


uint exitCode_ = 2;
shared_ptr<ManualResetEvent> exitServiceHostEvent_ = make_shared<ManualResetEvent>();

StringLiteral const TraceComponent("FileStoreServiceMain");

BOOL CtrlHandler(DWORD fdwCtrlType)
{ 
    switch(fdwCtrlType)
    { 
    case CTRL_C_EVENT: 
        // Handle the CTRL-C signal. 
        Trace.WriteInfo(TraceComponent, "Ctrl+C event received ...");
        exitCode_ = 0;
        exitServiceHostEvent_->Set();
        return true;
    case CTRL_BREAK_EVENT: 
        Trace.WriteInfo(TraceComponent, "Ctrl+Break event received ...");
        exitCode_ = 0;
        exitServiceHostEvent_->Set();
        return true;

    default: 
        // Pass other signals to the next handler.
        return FALSE; 
    } 
}

#if !defined(PLATFORM_UNIX)
void WaitForDebugger(int argc, __in_ecount(argc) wchar_t* argv[])
#else
void WaitForDebugger(int argc, __in_ecount(argc) char* argv[])
#endif
{
    bool waitForDebugger = false;

    for (int i = 0; i < argc; i++)
    {
#if !defined(PLATFORM_UNIX)
        if (Common::StringUtility::AreEqualCaseInsensitive(L"-WaitForDebugger", argv[i]))
#else
        if (Common::StringUtility::AreEqualCaseInsensitive("-WaitForDebugger", argv[i]))
#endif
        {
            waitForDebugger = true;
        }
    }

    if (!waitForDebugger)
    {
        return;
    }

    Trace.WriteInfo(TraceComponent, "Waiting for debugger to be attached ...");

    while(!IsDebuggerPresent())
    {
        Sleep(1000);
    }
}

#if !defined(PLATFORM_UNIX)
int __cdecl wmain(int argc, __in_ecount(argc) wchar_t* argv[])
{
#else
int main(int argc, char* argv[])
{
    umask(0002);
#endif
    try
    {
        Trace.WriteInfo(TraceComponent, "wmain(): Starting...");

        WaitForDebugger(argc, argv);

        ComPointer<IFabricRuntime> fabricRuntimeCPtr;
        ComPointer<IFabricCodePackageActivationContext> activationContextCPtr;

        // Get FabricRuntime and CodePackageActivationContext
        HRESULT hrRuntime = Hosting2::ApplicationHostContainer::FabricCreateRuntime(IID_IFabricRuntime, fabricRuntimeCPtr.VoidInitializationAddress());
        if (FAILED(hrRuntime))
        {
            Trace.WriteError(TraceComponent, "wmain(): Failed to create IFabricRuntime. Error:{0}", hrRuntime);
            return hrRuntime;
        }

        HRESULT hrContext = Hosting2::ApplicationHostContainer::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContextCPtr.VoidInitializationAddress());
        if (FAILED(hrContext))
        {
            Trace.WriteError(TraceComponent, "wmain(): Failed to create IFabricCodePackageActivationContext. Error:{0}", hrContext);
            return hrContext;
        }

        ComPointer<IFabricNodeContextResult2> nodeContextCPtr;
        hrContext = Hosting2::ApplicationHostContainer::FabricGetNodeContext(nodeContextCPtr.VoidInitializationAddress());
        if (FAILED(hrContext))
        {
            Trace.WriteWarning(TraceComponent, "wmain(): Failed to create IFabricNodeContextResult2. Error:{0}", hrContext);
            return hrContext;
        }

        // Create FileStoreServiceFactory and Initialize the factory
        FabricNodeConfigSPtr fabricNodeConfig = make_shared<FabricNodeConfig>();
        auto factorySPtr  = make_shared<FileStoreServiceFactory>(
            fabricRuntimeCPtr,
            fabricNodeConfig,
            wstring(activationContextCPtr->get_WorkDirectory()),
            nodeContextCPtr->get_NodeContext()->IPAddressOrFQDN,
            exitServiceHostEvent_);
        auto error = factorySPtr->Initialize();
        if(!error.IsSuccess())
        {
            Trace.WriteError(TraceComponent, "wmain(): Failed to create FileStoreServiceFactory. Error:{0}", error);
            return error.ToHResult();
        }

        SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, true);

        Trace.WriteInfo(TraceComponent, "wmain(): Main thread waiting for exit event (e.g. Ctrl+C) ...");

        // Keep the main thread of the service host alive.
        exitServiceHostEvent_->WaitOne();

        Trace.WriteInfo(TraceComponent, "wmain(): Exiting with '{0}'", exitCode_);

        return exitCode_;
    }
    catch (exception const& e)
    {
        Trace.WriteError(TraceComponent, "wmain(): Exiting... Unhandled exception:{0}", e.what());
        return 3;
    }
}
