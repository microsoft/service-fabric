// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TracePoints;

TracePointModule::TracePointModule(wstring const & name)
    : handle_(TracePointModule::Load(name))
{
}

TracePointModule::~TracePointModule()
{
    if (handle_ != NULL)
    {
        FreeLibrary(handle_);
    }
}

void TracePointModule::Invoke(LPCSTR functionName, ProviderMap & providerMap, LPCWSTR userData)
{
    FARPROC function = GetProcAddress(handle_, functionName);
    if (function == NULL)
    {
        throw Error::LastError("Failed to retrieve the function.");
    }

    TracePointModuleFunctionPtr moduleFunction = reinterpret_cast<TracePointModuleFunctionPtr>(function);
    moduleFunction(static_cast<ProviderMapHandle>(&providerMap), userData);
}

HMODULE TracePointModule::Load(wstring const & name)
{
    HMODULE handle = LoadLibrary(name.c_str());
    if (handle == NULL)
    {
        throw Error::LastError("Failed to load module.");
    }

    return handle;
}
