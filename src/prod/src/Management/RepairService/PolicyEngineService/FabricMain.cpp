// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace RepairPolicyEngine;

ManualResetEvent exitServiceHostEvent_;
int exitCode_ = 2; // default exit code in case the exit is called by RepairPolicyEngineService replica as result of failure recovery

HRESULT FabricMain(
    IFabricRuntime * runtime, 
    IFabricCodePackageActivationContext * activationContext)
{
    if (runtime == nullptr || activationContext == nullptr)
    {
        Trace.WriteError(ComponentName, "FabricMain(): One of the arguments is null.");
        return E_POINTER;
    }

    ComPointer<ServiceFactory> serviceFactoryCptr = make_com<ServiceFactory>();

    serviceFactoryCptr->Initialize(activationContext, &exitServiceHostEvent_);

    ComPointer<IFabricStatefulServiceFactory> statefulServiceFactoryCPtr(serviceFactoryCptr, IID_IFabricStatefulServiceFactory);

    if (statefulServiceFactoryCPtr.GetRawPointer() == NULL)
    {
        Trace.WriteError(ComponentName, "FabricMain(): Failed to create IFabricStatefulServiceFactory.");
        return E_POINTER;
    }

    // Register service factory with the fabric runtime
    return runtime->RegisterStatefulServiceFactory(L"RepairPolicyEngineService", statefulServiceFactoryCPtr.DetachNoRelease());
}

BOOL CtrlHandler(DWORD fdwCtrlType)
{ 
    switch(fdwCtrlType)
    { 
    case CTRL_C_EVENT: 
        // Handle the CTRL-C signal. 
        Trace.WriteInfo(ComponentName, "Ctrl+C event received ...");
        exitCode_ = 0;
        exitServiceHostEvent_.Set();
        return true;
    case CTRL_BREAK_EVENT: 
        Trace.WriteInfo(ComponentName, "Ctrl+Break event received ...");
        exitCode_ = 0;
        exitServiceHostEvent_.Set();
        return true;

    default: 
        // Pass other signals to the next handler.
        return FALSE; 
    } 
} 

void WaitForDebugger(int argc, __in_ecount(argc) wchar_t* argv[])
{
    bool waitForDebugger = false;

    for (int i = 0; i < argc; i++)
    {
        if (Common::StringUtility::AreEqualCaseInsensitive(L"-WaitForDebugger", argv[i]))
        {
            waitForDebugger = true;
        }
    }

    if (!waitForDebugger)
    {
        return;
    }

    Trace.WriteInfo(ComponentName, "Waiting for debugger to be attached ...");

    while(!IsDebuggerPresent())
    {
        Sleep(1000);
    }
}

int __cdecl wmain(int argc, __in_ecount(argc) wchar_t* argv[])
{
    DllConfig::GetConfig();

    try 
    {
        Trace.WriteInfo(ComponentName, "wmain(): Starting...");

        WaitForDebugger(argc, argv);

        ComPointer<IFabricRuntime> fabricRuntimeCPtr;
        ComPointer<IFabricCodePackageActivationContext> activationContextCPtr;

        HRESULT hrRuntime = ::FabricCreateRuntime(IID_IFabricRuntime, fabricRuntimeCPtr.VoidInitializationAddress());
        HRESULT hrContext = ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContextCPtr.VoidInitializationAddress());

        if (FAILED(hrRuntime))
        {
            Trace.WriteError(ComponentName, "wmain(): Failed to create IFabricRuntime. Error:{0}", hrRuntime);
            return hrRuntime;
        }

        if (FAILED(hrContext))
        {
            Trace.WriteError(ComponentName, "wmain(): Failed to create IFabricCodePackageActivationContext. Error:{0}", hrContext);
            return hrContext;
        }

        HRESULT hrFabricMain = FabricMain(fabricRuntimeCPtr.GetRawPointer(), activationContextCPtr.GetRawPointer());

        if (FAILED(hrFabricMain))
        {
            Trace.WriteError(ComponentName, "wmain(): FabricMain() returned Error:{0}", hrFabricMain);
            return hrFabricMain;
        }

        SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, true);

        Trace.WriteInfo(ComponentName, "wmain(): Main thread waiting for exit event (e.g. Ctrl+C) ...");

        // Keep the main thread of the service host alive.
        exitServiceHostEvent_.WaitOne();

        Trace.WriteInfo(ComponentName, "wmain(): Exiting...");

        // For now the exitServiceHostEvent_ is set only in error scenarios. Therefore non-zero exit code.
        return exitCode_;
    }
    catch (exception const& e)
    {
        Trace.WriteError(ComponentName, "wmain(): Exiting... Unhandled exception:{0}", e.what());
        return 3;
    }
}
