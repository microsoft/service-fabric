// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

NTSTATUS WdfRequestRetrieveInputBuffer(
    _In_ WDFREQUEST & Request,
    _In_ size_t MinimumRequiredSize,
    _Outptr_result_bytebuffer_(*Length) PVOID* Buffer,
    _Out_opt_ size_t* Length)
{
    if (Request->inputLength < MinimumRequiredSize)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *Buffer = Request->input;
    if(Length)
    {
        *Length = MinimumRequiredSize;
    }

    return STATUS_SUCCESS;
}

NTSTATUS WdfRequestRetrieveOutputBuffer(
    _In_ WDFREQUEST & Request,
    _In_ size_t MinimumRequiredSize,
    _Outptr_result_bytebuffer_(*Length) PVOID* Buffer,
    _Out_opt_ size_t* Length)
{
    if (Request->outputCapacity < MinimumRequiredSize)
    {
        return STATUS_INVALID_PARAMETER;
    }
    
    *Buffer = Request->output;
    if(Length)
    {
        *Length = MinimumRequiredSize;
    }

    return STATUS_SUCCESS;
}

NTSTATUS DeviceIoControl(
  _In_        DWORD        dwIoControlCode,
  _In_opt_    LPVOID       lpInBuffer,
  _In_        DWORD        nInBufferSize,
  _Out_opt_   LPVOID       lpOutBuffer,
  _In_        DWORD        nOutBufferSize,
  _Out_opt_   LPDWORD      lpBytesReturned)
{
    auto wdfRequest = make_shared<WdfRequest>(
        lpInBuffer,
        nInBufferSize,
        lpOutBuffer,
        nOutBufferSize);

    auto status =  LeaseLayerEvtIoDeviceControl(
        wdfRequest,
        nOutBufferSize,
        nInBufferSize,
        dwIoControlCode);

    *lpBytesReturned = wdfRequest->outputLength;

    if (!NT_SUCCESS(status))
    {
        switch (status)
        {
        case STATUS_INVALID_PARAMETER:
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        case (NTSTATUS)STATUS_INVALID_HANDLE:
            SetLastError(ERROR_INVALID_HANDLE);
            break;
        }
    }

    return status;
}

BOOL IsLeaseLayerInitialized()
/*++
 
Routine Description:
 
    Check to see if the user mode lease layer has been successfully initialized.

Return Value:
    
    TRUE if the handle to the device is not NULL.

--*/
{
    return true;
}

VOID InitializeLeaseLayer()
{
    //LINUXTODO
}

HRESULT StringCchCopyW(
    _Out_writes_(cchDest) _Always_(_Post_z_) LPWSTR pszDest,
    _In_ size_t cchDest,
    _In_ LPCWSTR pszSrc)
{
    auto srcLength = wcslen(pszSrc) + 1;
    if (srcLength > cchDest) return E_INVALIDARG;

    wcsncpy(pszDest, pszSrc, cchDest);
    return S_OK;
}
