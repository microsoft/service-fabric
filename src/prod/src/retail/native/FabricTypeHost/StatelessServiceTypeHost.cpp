// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace FabricTypeHost;

StringLiteral const TraceType("StatelessServiceTypeHost");

StatelessServiceTypeHost::StatelessServiceTypeHost(vector<wstring> && typesToHost)
    : typesToHost_(move(typesToHost))
{
    traceId_ = wformatString("{0}", static_cast<void*>(this));
    WriteNoise(TraceType, traceId_, "ctor");
}

StatelessServiceTypeHost::~StatelessServiceTypeHost()
{
    WriteNoise(TraceType, traceId_, "dtor");
}

ErrorCode StatelessServiceTypeHost::OnOpen()
{
    HRESULT hr = ::FabricCreateRuntime(IID_IFabricRuntime, runtime_.VoidInitializationAddress());
    if (FAILED(hr))
    {
        WriteWarning(
            TraceType, 
            traceId_,
            "Failed to create FabricRuntime. HRESULT={0}",
            hr);
        return ErrorCode::FromHResult(hr);
    }

    WriteNoise(TraceType, traceId_, "FabricRuntime created.");

    // create a service factory and register it for all the types
    ComPointer<IFabricStatelessServiceFactory> serviceFactory = make_com<ComStatelessServiceFactory, IFabricStatelessServiceFactory>();

    for(auto iter = typesToHost_.begin(); iter != typesToHost_.end(); ++ iter)
    {
        hr = runtime_->RegisterStatelessServiceFactory(iter->c_str(), serviceFactory.GetRawPointer());
        if (FAILED(hr))
        {
            WriteWarning(
            TraceType, 
            traceId_,
            "Failed to register stateless service factory for {1}. HRESULT={0}",
            hr,
            *iter);
            return ErrorCode::FromHResult(hr);
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode StatelessServiceTypeHost::OnClose()
{
    OnAbort();
    return ErrorCode(ErrorCodeValue::Success);
}

void StatelessServiceTypeHost::OnAbort()
{
    ComPointer<IFabricRuntime> cleanup = move(runtime_);
    if (cleanup.GetRawPointer() != NULL)
    {
        cleanup.Release();
    }
}
