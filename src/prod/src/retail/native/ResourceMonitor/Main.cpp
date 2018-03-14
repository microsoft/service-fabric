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
	IFabricRuntime* pRuntime;
	HRESULT hr = FabricCreateRuntime(IID_IFabricRuntime, (void**)(&pRuntime));

	auto serviceFactory = make_com<ResourceMonitorServiceFactory, IFabricStatelessServiceFactory>(pRuntime);
	auto &ipc = (dynamic_cast<ComFabricRuntime*>(pRuntime))->Runtime->Host.Client;

	ComPointer<IFabricRuntime> spFabricRuntime;
	spFabricRuntime.SetNoAddRef(pRuntime);
	serviceFactory.GetRawPointer();

	hr = spFabricRuntime->RegisterStatelessServiceFactory(ResourceMonitorTypeName, serviceFactory.GetRawPointer());
	UNREFERENCED_PARAMETER(hr); UNREFERENCED_PARAMETER(ipc);
    while (1)
    {
        Sleep(10000);
    }

}
