// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "windows.h"
#include "rpc.h"

#include "WexTestClass.h"
#include "LogController.h"

#include "ControllerHost.h"

using namespace WEX::Logging;
using namespace WEX::Common;
using namespace WEX::TestExecution;


const PWCHAR TestIPC::TestIPCController::IPCControllerName = L"IPCController";
const PWCHAR TestIPC::TestIPCController::IPCControllerOffsetName = L"IPCControllerOffset";

//
// Implementation of the controller side of the IPC
//
TestIPC::TestIPCController::TestIPCController(
    ) :
     _FileMapping(NULL),
     _Pointer(NULL),
     _SizeInBytes(0)
{
   _ConnectionString[0] = 0;
}

VOID
TestIPC::TestIPCController::Reset(
    )
{
    if (_Pointer != NULL)
    {
        UnmapViewOfFile(_Pointer);
        _Pointer = NULL;        
    }

    if (_FileMapping != NULL)
    {
        CloseHandle(_FileMapping);
        _FileMapping = NULL;
    }

    _SizeInBytes = 0;
}

TestIPC::TestIPCController::~TestIPCController(
    ) 
{
	Reset();
}

ULONG
TestIPC::TestIPCController::Initialize(
    __in PWCHAR Name,
    __in SIZE_T SizeInBytes
    )
{
    ULONG error;
    HRESULT hr;
    GUID guid;

    //
    // Round up to 8KB boundary
    //
    _SizeInBytes = (SizeInBytes+0x1FFF) & ~0x1FFF;
    
    error = UuidCreate(&guid);
    if (error != RPC_S_OK)
    {
        return(error);
    }

    //
    // Build the connection string
    //
    hr = StringCbPrintf(_ConnectionString, sizeof(_ConnectionString),
                        L"%ws_0x%08x_%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                        Name,
                        _SizeInBytes,
                        guid.Data1,
                        guid.Data2,
                        guid.Data3,
                        guid.Data4[0],
                        guid.Data4[1],
                        guid.Data4[2],
                        guid.Data4[3],
                        guid.Data4[4],
                        guid.Data4[5],
                        guid.Data4[6],
                        guid.Data4[7]);
    if (FAILED(hr))
    {
        return(hr);
    }
                    

    //
    // Setup the file mapping object
    //
    _FileMapping = CreateFileMapping(NULL,
                                     NULL,
                                     PAGE_READWRITE | SEC_COMMIT,
                                     0,
                                     (ULONG)_SizeInBytes,
                                     _ConnectionString);
    if (_FileMapping == NULL)
    {
        error = GetLastError();
        return(error);
    }
                                     
    _Pointer = MapViewOfFile(_FileMapping,
                             FILE_MAP_ALL_ACCESS,
                             0,
                             0,
                             0);
    if (_Pointer == NULL)
    {
        error = GetLastError();
		Reset();
		
        return(error);
    }

    return(ERROR_SUCCESS);
}


//
// Implementation of the Host side of the IPC
//
TestIPC::TestIPCHost::TestIPCHost() :
   _Pointer(NULL),
   _SizeInBytes(0),
   _FileMapping(NULL)
{
}

VOID
TestIPC::TestIPCHost::Reset(
    )
{
    if (_Pointer != NULL)
    {
        UnmapViewOfFile(_Pointer);
        _Pointer = NULL;        
    }

    if (_FileMapping != NULL)
    {
        CloseHandle(_FileMapping);
        _FileMapping = NULL;
    }

    _SizeInBytes = 0;
}

TestIPC::TestIPCHost::~TestIPCHost()
{
    Reset();
}

ULONG
TestIPC::TestIPCHost::Connect(
    __in LPCWSTR ConnectionString
    )
{
    ULONG error;
    
    _FileMapping = OpenFileMapping(GENERIC_ALL,
                                   FALSE,
                                   ConnectionString);

    if (_FileMapping == NULL)
    {
        error = GetLastError();
        return(error);
    }

    _Pointer = MapViewOfFile(_FileMapping,
                             FILE_MAP_ALL_ACCESS,
                             0,
                             0,
                             0);
    if (_Pointer == NULL)
    {
        error = GetLastError();
		Reset();
		
        return(error);
    }

    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T size;

    size = VirtualQuery(_Pointer,
                        &mbi,
                        sizeof(MEMORY_BASIC_INFORMATION));
    
    if (size != sizeof(MEMORY_BASIC_INFORMATION))
    {
        error = GetLastError();
		Reset();
		
        return(error);      
    }

    _SizeInBytes = mbi.RegionSize;
    
    return(ERROR_SUCCESS);
}

VOID
TestIPC::TestIPCReturnError(
    TestIPC::TestIPCResult* Result,
    ULONG CompletionStatus,
    PCHAR ErrorFileName,
    ULONG ErrorLineNumber,
    PWCHAR ErrorMessage
)
{
    Result->CompletionStatus = CompletionStatus;
    StringCbCopyA(Result->ErrorFileName, sizeof(Result->ErrorFileName), ErrorFileName);
    StringCbCopy(Result->ErrorMessage, sizeof(Result->ErrorMessage), ErrorMessage);
    Result->ErrorLineNumber = ErrorLineNumber;
}
