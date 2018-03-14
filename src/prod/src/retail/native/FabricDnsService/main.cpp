// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricTracer.h"
#include "FabricDnsService.h"
#include "FabricDnsServiceFactory.h"
#include "Constants.h"

using namespace DNS;

IDnsTracer* g_pTracer = nullptr;

BOOLEAN FailFastHandler(
    __in LPCSTR condition,
    __in LPCSTR file,
    __in ULONG line
)
{
    UNREFERENCED_PARAMETER(condition);

    g_pTracer->Trace(DnsTraceLevel_Error, 
        "DNS FailFastHandler condition {0} line {1}:{2}",
        KMemRef(static_cast<ULONG>(0), (PVOID)condition),
        KMemRef(static_cast<ULONG>(0), (PVOID)file),
        line);

    // Terminate the process if the control is given back to us
    //
    TerminateProcess(GetCurrentProcess(), 1);

    // TerminateProcess is asynchronous. So wait here until our thread is killed by process teardown.
    Sleep(INFINITE);

    return FALSE;
}

int main(int argc, char** argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    KtlSystem* pKtlSystem = &Common::GetSFDefaultKtlSystem();
    if (pKtlSystem == nullptr)
    {
        return 1;
    }

    KAllocator& allocator = pKtlSystem->NonPagedAllocator();

    FabricTracer::SPtr spTracer;
    FabricTracer::Create(/*out*/spTracer, allocator);

    g_pTracer = spTracer.RawPtr();

    KInvariantCalloutType callout = FailFastHandler;
    SetKInvariantCallout(callout);

    FabricDnsServiceFactory::SPtr spFactory;
    FabricDnsServiceFactory::Create(/*out*/spFactory, allocator, *spTracer);

    IFabricRuntime* pRuntime;
    HRESULT hr = FabricCreateRuntime(IID_IFabricRuntime, /*out*/(void**)(&pRuntime));
    if (hr != S_OK)
    {
        g_pTracer->Trace(DnsTraceLevel_Error, "Failed to create FabricRuntime, error 0x{0}", hr);
        KInvariant(false);
    }

    DNS::ComPointer<IFabricRuntime> spFabricRuntime;
    spFabricRuntime.Attach(pRuntime);

    hr = spFabricRuntime->RegisterStatelessServiceFactory(DnsServiceTypeName, spFactory.RawPtr());
    if (hr != S_OK)
    {
        g_pTracer->Trace(DnsTraceLevel_Error, "Failed to register DnsService with FabricRuntime, error 0x{0}", hr);
        KInvariant(false);
    }

    for (;;)
    {
        Sleep(1000);
    }

    return 0;
}
