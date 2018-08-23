// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Api;
using namespace Transport;
using namespace ResourceMonitor;

LPCWSTR ResourceMonitorTypeName = L"ResourceMonitorServiceType";

int main()
{
    ComPointer<IFabricRuntime> runtimeCPtr;

    HRESULT hr = ::FabricCreateRuntime(
        IID_IFabricRuntime,
        runtimeCPtr.VoidInitializationAddress());
    if (FAILED(hr)) { return hr; }

    auto serviceFactory = make_com<ResourceMonitorServiceFactory, IFabricStatelessServiceFactory>(runtimeCPtr);

    hr = runtimeCPtr->RegisterStatelessServiceFactory(ResourceMonitorTypeName, serviceFactory.GetRawPointer());
    if (FAILED(hr)) { return hr; }

    while (1)
    {
        Sleep(10000);
    }

}
