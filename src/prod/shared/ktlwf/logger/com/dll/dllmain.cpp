// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "KPhysicalLog.h"
#include "ktrace.h"
#include "windows.h"
#include "Common/KtlSF.Common.h"

BOOLEAN KPhysicalLogKInvariantCallout(
    __in LPCSTR Condition,
    __in LPCSTR File,
    __in ULONG Line
    )
{
    KDbgPrintf("KInvariant(%s) failure at file %s line %d\n",
              Condition,
              File,
              Line);
    
    //
    // Default behavior is to assert or crash system
    //
    return(TRUE);
}


BOOL APIENTRY DllMain(
    __in HMODULE module,
    __in DWORD reason,
    __in LPVOID)
{
    UNREFERENCED_PARAMETER(module);

    KInvariant(DisableThreadLibraryCalls(module));

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
            KDbgMirrorToDebugger = FALSE;
            EventRegisterMicrosoft_Windows_KTL();
            SetKInvariantCallout(KPhysicalLogKInvariantCallout);
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            EventUnregisterMicrosoft_Windows_KTL();
            break;
        }

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
        }
    return TRUE;
}

using namespace Ktl::Com;
using namespace Common;

HRESULT KCreateKBuffer( 
    __in ULONG Size,
    __out IKBuffer **const Result)
{
    if (Result == nullptr)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComKBuffer::SPtr sptr;

    NTSTATUS    status = ComKBuffer::Create(GetSFDefaultKtlSystem().NonPagedAllocator(), Size, sptr);
    if (!NT_SUCCESS(status))
    {
        *Result = nullptr;
        return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
    }

    *Result = sptr.Detach();
    return Common::ComUtility::OnPublicApiReturn(S_OK);
}


HRESULT KCreateKIoBufferElement( 
    __in ULONG Size,
    __out IKIoBufferElement **const Result)
{
    if (Result == nullptr)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComKIoBufferElement::SPtr sptr;

    NTSTATUS    status = ComKIoBufferElement::Create(GetSFDefaultKtlSystem().NonPagedAllocator(), Size, sptr);
    if (!NT_SUCCESS(status))
    {
        *Result = nullptr;
        return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
    }

    *Result = sptr.Detach();
    return Common::ComUtility::OnPublicApiReturn(S_OK);
}


HRESULT 
KCreateEmptyKIoBuffer(__out IKIoBuffer **const Result)
{
    ComKIoBuffer::SPtr  comIoBuffer;
    HRESULT     hr = ComKIoBuffer::CreateEmpty(GetSFDefaultKtlSystem().NonPagedAllocator(), comIoBuffer);
    if (FAILED(hr))
    {
        *Result = nullptr;
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    *Result = comIoBuffer.Detach();
    return Common::ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT 
KCreateSimpleKIoBuffer( 
    __in ULONG Size,
    __out IKIoBuffer **const Result)
{
    ComKIoBuffer::SPtr  comIoBuffer;
    HRESULT     hr = ComKIoBuffer::CreateSimple(GetSFDefaultKtlSystem().NonPagedAllocator(), Size, comIoBuffer);
    if (FAILED(hr))
    {
        *Result = nullptr;
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    *Result = comIoBuffer.Detach();
    return Common::ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT 
KCreateGuidIKArray( 
    __in ULONG InitialSize,
    __out IKArray **const Result)
{
    ComIKArray<GUID>::SPtr    sptr;

    HRESULT hr = ComIKArray<GUID>::Create(
        GetSFDefaultKtlSystem().NonPagedAllocator(),
        InitialSize, 
        sptr);

    if (FAILED(hr))
    {
        *Result = nullptr;
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    *Result = sptr.Detach();
    return Common::ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT 
KCreateLogRecordMetadataIKArray( 
    __in ULONG InitialSize,
    __out IKArray **const Result)
{
    ComIKArray<KPHYSICAL_LOG_STREAM_RecordMetadata>::SPtr    sptr;

    HRESULT hr = ComIKArray<KPHYSICAL_LOG_STREAM_RecordMetadata>::Create(
        GetSFDefaultKtlSystem().NonPagedAllocator(),
        InitialSize, 
        sptr);

    if (FAILED(hr))
    {
        *Result = nullptr;
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    *Result = sptr.Detach();
    return Common::ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT 
KCreatePhysicalLogManager(__out IKPhysicalLogManager **const Result)
{
    ComIKPhysicalLogManager::SPtr   sptr;

    HRESULT hr = ComIKPhysicalLogManager::Create(
        GetSFDefaultKtlSystem().NonPagedAllocator(),
        sptr);

    if (FAILED(hr))
    {
        *Result = nullptr;
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    *Result = sptr.Detach();
    return Common::ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT 
KCreatePhysicalLogManagerInproc(__out IKPhysicalLogManager **const Result)
{
    ComIKPhysicalLogManager::SPtr   sptr;

    HRESULT hr = ComIKPhysicalLogManager::CreateInproc(
        GetSFDefaultKtlSystem().NonPagedAllocator(),
        sptr);

    if (FAILED(hr))
    {
        *Result = nullptr;
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    *Result = sptr.Detach();
    return Common::ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT 
KCopyMemory(
    __in ULONGLONG SourcePtr,
    __in ULONGLONG DestPtr,
    __in ULONG Size)
{
    KMemCpySafe((void*)DestPtr, Size, (void*)SourcePtr, Size);      // The load is on the c# layer to limit dest size
    return Common::ComUtility::OnPublicApiReturn(S_OK);
}

ULONGLONG
KCrc64(
    __in_bcount(Size) const VOID* Source,
    __in ULONG Size,
    __in ULONGLONG InitialCrc)
{
    return KChecksum::Crc64(Source, Size, InitialCrc);
}

//
// Interface version 1 adds MultiRecordRead apis
//
const USHORT InterfaceVersion = 1;

HRESULT 
QueryUserBuildInformation(
    __out ULONG* const BuildNumber,
    __out byte* const IsFreeBuild)

{
    *BuildNumber = (InterfaceVersion << 16) + VER_PRODUCTBUILD;
#if DBG
    *IsFreeBuild = 0;
#else
    *IsFreeBuild = 1;
#endif

    return Common::ComUtility::OnPublicApiReturn(S_OK);
}
