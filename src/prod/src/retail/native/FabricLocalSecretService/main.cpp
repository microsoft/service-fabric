//+---------------------------------------------------------------------------
//  Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management::LocalSecretService;

StringLiteral const TraceComponent("LocalSecretServiceMain");

int main(int argc, char** argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    Trace.WriteInfo(TraceComponent, "Constructing factory ...");

    ComPointer<IFabricStatelessServiceFactory> factory = make_com<LocalSecretServiceFactory, IFabricStatelessServiceFactory>();

    Trace.WriteInfo(TraceComponent, "Constructing runtime ...");

    IFabricRuntime* pRuntime;
    HRESULT hr = FabricCreateRuntime(IID_IFabricRuntime, /*out*/(void**)(&pRuntime));
    if (hr != S_OK)
    {
        return -1;
    }

    ComPointer<IFabricRuntime> spFabricRuntime;
    spFabricRuntime.SetNoAddRef(pRuntime);

    Trace.WriteInfo(TraceComponent, "Registering svc ...");
    hr = spFabricRuntime->RegisterStatelessServiceFactory(L"LocalSecretServiceType", factory.GetRawPointer());
    if (hr != S_OK)
    {
        return -1;
    }

    Trace.WriteInfo(TraceComponent, "Bootstrap completed. Waiting for exit...");

    for (;;)
    {
        Sleep(1000);
    }

    return 0;
}
