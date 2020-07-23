/*++

Copyright (c) 2012  Microsoft Corporation

Module Name:

    VHDUtil.cpp

Abstract:

    This file implements a simple utility class to create and delete VHDs for testing. 

Author:

    Peng Li (pengli)    24-June-2012

Environment:

    User Mode only

Notes:

Revision History:

--*/

#include <ktl.h>
#include <windows.h>
#include <stdio.h>

#include "VHDUtil.h"

BOOLEAN
VHDUtil::CreateVHD(
    __in_z LPCWSTR VHDFilePath,
    __in ULONG SizeInMB,
    __in_z LPCWSTR VolumeLabel,
    __in BOOLEAN KeepAttached    
    )
{    
    WCHAR scriptFile[MAX_PATH] = {0};
    swprintf_s(scriptFile, MAX_PATH, L"createvhdscript-%I64x.txt", KNt::GetPerformanceTime());        

    //
    // Build a diskpart script file.
    //

    FILE* fp = _wfopen(scriptFile, L"wt");
    if (!fp)
    {
        return FALSE;
    }

    fwprintf(fp, L"create vdisk file=\"%s\" maximum=%d type=expandable\n", VHDFilePath, SizeInMB);
    fwprintf(fp, L"select vdisk file=\"%s\"\n", VHDFilePath);    
    fwprintf(fp, L"attach vdisk\n");        
    fwprintf(fp, L"create partition primary\n");            
    fwprintf(fp, L"format fs=ntfs quick label=\"%s\"\n", VolumeLabel);   
    fwprintf(fp, L"assign\n");
    if (!KeepAttached)
    {
        fwprintf(fp, L"detach vdisk\n");    
    }

    fclose(fp);
    fp = nullptr;

    DWORD error = RunDiskPart(scriptFile);
    if (error == ERROR_SUCCESS)
    {
        ::DeleteFileW(scriptFile);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOLEAN
VHDUtil::DeleteVHD(
    __in_z LPCWSTR VHDFilePath
    )
{
    //
    // Try to deatch the VHD. If the VHD is not attached, detach will fail. 
    //
    DetachVHD(VHDFilePath);

    //
    // Always delete the VHD file. Diskpart might have failed because the VHD isn't attached.
    // But we still want to delete the VHD file.
    //
    BOOL b = ::DeleteFileW(VHDFilePath);
    return b ? TRUE : FALSE;
}

BOOLEAN
VHDUtil::AttachVHD(
    __in_z LPCWSTR VHDFilePath    
    )
{
    WCHAR scriptFile[MAX_PATH] = {0};
    swprintf_s(scriptFile, MAX_PATH, L"attachvhdscript-%I64x.txt", KNt::GetPerformanceTime());            

    //
    // Build a diskpart script file.
    //

    FILE* fp = _wfopen(scriptFile, L"wt");
    if (!fp)
    {
        return FALSE;
    }

    fwprintf(fp, L"select vdisk file=\"%s\"\n", VHDFilePath);    
    fwprintf(fp, L"attach vdisk\n");        

    fclose(fp);
    fp = nullptr;

    DWORD error = RunDiskPart(scriptFile);
    if (error == ERROR_SUCCESS)
    {
        ::DeleteFileW(scriptFile);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOLEAN
VHDUtil::DetachVHD(
    __in_z LPCWSTR VHDFilePath
    )
{
    WCHAR scriptFile[MAX_PATH] = {0};    
    swprintf_s(scriptFile, MAX_PATH, L"detachvhdscript-%I64x.txt", KNt::GetPerformanceTime());            
    
    //
    // Build a diskpart script file.
    //

    FILE* fp = _wfopen(scriptFile, L"wt");
    if (!fp)
    {
        return FALSE;
    }    

    fwprintf(fp, L"select vdisk file=\"%s\"\n", VHDFilePath);    
    fwprintf(fp, L"detach vdisk\n");        

    fclose(fp);
    fp = nullptr;

    DWORD error = RunDiskPart(scriptFile);

    //
    // Always delete the script file. Detach can fail because the VHD is not attached at all, 
    // which is not a real failure. It creates a lot of garbage on disk.
    //
    ::DeleteFileW(scriptFile);    
    
    if (error == ERROR_SUCCESS)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

DWORD
VHDUtil::RunDiskPart(
    __in_z LPCWSTR ScriptFilePath
    )
{
    DWORD error = ERROR_SUCCESS;    
    
    WCHAR commandLine[MAX_PATH] = {0};
    swprintf_s(commandLine, MAX_PATH, L"diskpart.exe /s %s", ScriptFilePath);

    STARTUPINFO si;
    RtlZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;    
    RtlZeroMemory(&pi, sizeof(pi));

    KFinally([&]()
    {  
        if (pi.hProcess != INVALID_HANDLE_VALUE && pi.hProcess != NULL)
        {
            ::CloseHandle(pi.hProcess);
        }

        if (pi.hThread != INVALID_HANDLE_VALUE && pi.hThread != NULL)
        {
            ::CloseHandle(pi.hThread);
        }
    });
    
    BOOL b = ::CreateProcessW(
        nullptr,        // lpApplicationName
        commandLine,    // lpCommandLine
        nullptr,        // lpProcessAttributes
        nullptr,        // lpThreadAttributes  
        FALSE,          // bInheritHandles
        0,              // dwCreationFlags
        nullptr,        // lpEnvironment 
        nullptr,        // lpCurrentDirectory 
        &si,
        &pi
        );
    if (!b)
    {
        error = GetLastError();
        return error;
    }    

    //
    // Wait until diskpart.exe finishes.
    //
    if (::WaitForSingleObject(pi.hProcess, INFINITE) != WAIT_OBJECT_0)
    {
        error = GetLastError();
        return error;
    }

    ::GetExitCodeProcess(pi.hProcess, &error);
    return error;
}

NTSTATUS
VHDDisk::Create(
    __out VHDDisk::SPtr& Result,
    __in GUID& DiskId, 
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in_z LPCWSTR VHDFilePath,
    __in ULONG SizeInMB,
    __in_z LPCWSTR VolumeLabel
    )
{
    Result = _new(AllocationTag, Allocator) VHDDisk(DiskId, VHDFilePath, SizeInMB, VolumeLabel);
    if (!Result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result = nullptr;
        return status;
    }

    return STATUS_SUCCESS;
}

VHDDisk::VHDDisk(
    __in GUID& DiskId, 
    __in_z LPCWSTR VHDFileName,
    __in ULONG SizeInMB,
    __in_z LPCWSTR VolumeLabel    
    ) : 
    _DiskId(DiskId),
    _FullyQualifiedFilePath(GetThisAllocator()),
    _IsAttached(FALSE)
{
    NTSTATUS status = STATUS_SUCCESS;

    WCHAR dir[MAX_PATH] = {0};
    DWORD n = ::GetCurrentDirectoryW(MAX_PATH, dir);
    if (!n || n >= MAX_PATH)
    {
        SetConstructorStatus(STATUS_NAME_TOO_LONG);
        return;
    }

    _FullyQualifiedFilePath = dir;
    _FullyQualifiedFilePath += L"\\";
    _FullyQualifiedFilePath += VHDFileName;

    status = _FullyQualifiedFilePath.Status();
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;        
    }    

    //
    // Clean up any existing VHD.
    //

    VHDUtil::DeleteVHD((WCHAR*)_FullyQualifiedFilePath);

    //
    // Create a new one.
    //

    for (ULONG retryCount = 0; retryCount < 8; retryCount++)
    {
        BOOLEAN b = VHDUtil::CreateVHD(_FullyQualifiedFilePath, SizeInMB, VolumeLabel, TRUE);
        if (b)
        {
            _IsAttached = TRUE;
            return;
        }

        //
        // This error is most likely because the deletion is pending on the VHD file.
        // Wait a little while and try again.
        //
        
        KNt::Sleep(2000);        
    }

    SetConstructorStatus(STATUS_INTERNAL_ERROR);
    return;
}

VHDDisk::~VHDDisk()
{
    VHDUtil::DeleteVHD(_FullyQualifiedFilePath);    
}

BOOLEAN
VHDDisk::Attach()
{
    if (!_IsAttached)
    {
        return VHDUtil::AttachVHD(_FullyQualifiedFilePath);        
    }
    else
    {
        return TRUE;
    }
}

BOOLEAN
VHDDisk::Detach()
{
    if (_IsAttached)
    {
        return VHDUtil::DetachVHD(_FullyQualifiedFilePath);
    }
    else
    {
        return TRUE;
    }
}

GUID const&
VHDDisk::DiskId()
{
    return _DiskId;
}

LPCWSTR 
VHDDisk::FilePath()
{
    return (LPCWSTR)_FullyQualifiedFilePath;
}

