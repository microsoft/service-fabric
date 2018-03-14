// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Utilities;

// Wrap HRESULT in NTSTATUS by setting the FACILITY_NT_BIT
#define NT_FROM_HRESULT(x) ((NTSTATUS) ((x) | FACILITY_NT_BIT))
// Get the HRESULT in NTSTATUS by unsetting the FACILITY_NT_BIT
#define HRESULT_IN_NT(x) ((HRESULT) ((x) & (~FACILITY_NT_BIT)))
// Get the NTSTATUS wrapped in HRESULT x by unsetting the FACILITY_NT_BIT
#define NT_IN_HRESULT(x) ((NTSTATUS) ((x) & (~FACILITY_NT_BIT)))
// Return true if FACILITY_NT_BIT is set
#define NT_BIT_SET(x) ((x) & FACILITY_NT_BIT)

bool StatusConverter::IsSFHResult(__in HRESULT hResult) noexcept
{
    // Check if the S, R, C, N, r, Facility and highest order error code bits are SF.
    if ((hResult & HighMask) != HighMask)
    {
        return false;
    }

    // Check if the remaining error code is within the SF reserved range.
    HRESULT errorCode = (hResult & LowMask);
    if (errorCode < LowestSFErrorCode || errorCode > HighestSFErrorCode)
    {
        return false;
    }

    return true;
}

bool StatusConverter::TryConvertToSFSTATUS(
    __in HRESULT hResult, 
    __out NTSTATUS & status) noexcept
{
    // Input validation: Check if the input HResult is an SF HResult.
    if (IsSFHResult(hResult) == false)
    {
        // This method only supports conversion from SF HRESULT to SF STATUS.
        return false;
    }

    // Zero out the high order bits.
    status = hResult & SFLowMask;

    // Reset them with the SF status high order bits.
    status |= SFStatusHighMask;
    return true;
}

NTSTATUS StatusConverter::ConvertToSFSTATUS(__in HRESULT hResult) noexcept
{
    NTSTATUS status = STATUS_SUCCESS;
    bool result = TryConvertToSFSTATUS(hResult, status);

    ASSERT_IFNOT(
        result,
        "Failed to Convert HRESULT {0} to SF NTSTATUS",
        hResult);

    return status;
}

bool StatusConverter::TryConvert(
    __in HRESULT hResult,
    __out NTSTATUS & status) noexcept
{
    NTSTATUS result;
    bool isConverted;

    isConverted = TryConvertToSFSTATUS(hResult, result);
    if (isConverted)
    {
        status = result;
        return true;
    }

    isConverted = MapToNTStatus(hResult, result);
    if (isConverted)
    {
        status = result;
        return true;
    }

    isConverted = TryConvertWin32ErrorCode(hResult, result);
    if (isConverted)
    {
        status = result;
        return true;
    }

    // If FACILITY_NT_BIT is set, then this HRESULT is actually a NTSTATUS.
    // Unsetting the bit will give us the original NTSTATUS. If FACILITY_NT_BIT
    // is not set, this is a real HRESULT. We convert this HRESULT to NTSTATUS
    // by setting the FACILITY_NT_BIT.
    if (NT_BIT_SET(hResult))
    {
        status = NT_IN_HRESULT(hResult);
    }
    else
    {
        status = NT_FROM_HRESULT(hResult);
    }
    return true;
}

NTSTATUS StatusConverter::Convert(__in HRESULT hResult) noexcept
{
    NTSTATUS status = STATUS_SUCCESS;
    bool result = TryConvert(hResult, status);

    ASSERT_IFNOT(
        result,
        "Failed to Convert HRESULT {0} to NTSTATUS",
        hResult);

    return status;
}

//
// This function checks whether an NTSTATUS is an SF STATUS.
// Assumptions: 
// 1. All SF NTSTATUS and HRESULT are errors.
// 2. SF NTSTATUS and HRESULT have only one facility code.
//
bool StatusConverter::IsSFStatus(__in NTSTATUS status) noexcept
{
    if (status < LowestSFStatus || status > HighestSFStatus)
    {
        return false;
    }

    return true;
}

//
// This function converts an SF STATUS to HRESULT.
// Assumptions: 
// 1. All SF STATUS and HRESULT are errors.
// 2. SF STATUS and HRESULT have only one facility code.
//
bool StatusConverter::ToSFHResult(
    __in NTSTATUS status, 
    __out HRESULT & hResult) noexcept
{
    // Input validation: Check if the input HResult is an SF HResult.
    if (IsSFStatus(status) == false)
    {
        // This method only supports conversion from SF HRESULT to SF STATUS.
        return false;
    }

    // Zero out the high order bits.
    hResult = status & SFLowMask;

    // Reset them with the SF HResult high order bits.
    hResult |= SFHResultHighMask;
    return true;
}

//
// This function converts an NTSTATUS to HRESULT.
// First it tries to convert into SF HRESULT which is a lossless conversion.
// If it fails, uses the set of well known map to convert in to a HRESULT.
// If it fails, uses Windows macro to wrap the NTSTATUS in an HRESULT.
// TODO: If we need round-trip conversion, we may have to provide our own HRESULT_FROM_NT 
// that we can convert back and forth without loss.
//
HRESULT StatusConverter::ToHResult(__in NTSTATUS status) noexcept
{
    HRESULT result;
    bool isConverted;

    isConverted = ToSFHResult(status, result);
    if (isConverted)
    {
        return result;
    }

    isConverted = MapToHResult(status, result);
    if (isConverted)
    {
        return result;
    }

    // If FACILITY_NT_BIT is set, then this NTSTATUS is actually a HRESULT.
    // Unsetting the bit will give us the original HRESULT. If FACILITY_NT_BIT
    // is not set, this is a real NTSTATUS(a real NTSTATUS should never have
    // this bit set according to https://msdn.microsoft.com/en-us/library/cc231200.aspx.)
    // We convert this NTSTATUS to HRESULT by setting the FACILITY_NT_BIT.
    if (NT_BIT_SET(status))
    {
        return HRESULT_IN_NT(status);
    }
    else
    {
        return HRESULT_FROM_NT(status);
    }
}

bool StatusConverter::MapToNTStatus(
    __in HRESULT hResult, 
    __out NTSTATUS & status) noexcept
{
    switch(hResult)
    {
    case S_OK:
        status = STATUS_SUCCESS;
        return true;
    case E_INVALIDARG:
        status = STATUS_INVALID_PARAMETER;
        return true;
    case E_ACCESSDENIED:
        status = STATUS_ACCESS_DENIED;
        return true;
    case E_POINTER:
        status = Common::K_STATUS_NULL_REF_POINTER;
        return true;
    case E_ABORT:
        status = STATUS_CANCELLED;
        return true;
    case E_FAIL:
        status = STATUS_UNSUCCESSFUL;
        return true;
    case E_OUTOFMEMORY:
        status = STATUS_INSUFFICIENT_RESOURCES;
        return true;
    case E_NOTIMPL:
        status = STATUS_NOT_IMPLEMENTED;
        return true;
    }

    return false;
}

//
// This function map a set of well known NTSTATUS in to HRESULT.
// Note that this map must be opposite of TryConvertWellKnown.
//
bool StatusConverter::MapToHResult(
    __in NTSTATUS status, 
    __out HRESULT & hResult) noexcept
{
    switch (status)
    {
    case STATUS_SUCCESS:
        hResult = S_OK;
        return true;
    case STATUS_INVALID_PARAMETER:
    case STATUS_INVALID_PARAMETER_1:
    case STATUS_INVALID_PARAMETER_2:
    case STATUS_INVALID_PARAMETER_3:
    case STATUS_INVALID_PARAMETER_4:
    case STATUS_INVALID_PARAMETER_5:
    case STATUS_INVALID_PARAMETER_6:
    case STATUS_INVALID_PARAMETER_7:
    case STATUS_INVALID_PARAMETER_8:
    case STATUS_INVALID_PARAMETER_9:
    case STATUS_INVALID_PARAMETER_10:
    case STATUS_INVALID_PARAMETER_11:
    case STATUS_INVALID_PARAMETER_12:
        hResult = E_INVALIDARG;
        return true;
    case STATUS_ACCESS_DENIED:
        hResult = E_ACCESSDENIED;
        return true;
    case Common::K_STATUS_NULL_REF_POINTER:
        hResult = E_POINTER;
        return true;
    case STATUS_CANCELLED:
        hResult = E_ABORT;
        return true;
    case STATUS_UNSUCCESSFUL:
        hResult = E_FAIL;
        return true;
    case STATUS_INSUFFICIENT_RESOURCES:
        hResult = E_OUTOFMEMORY;
        return true;
    case STATUS_NOT_IMPLEMENTED:
        hResult = E_NOTIMPL;
        return true;
    }

    return false;
}

bool StatusConverter::TryConvertWin32ErrorCode(
    __in HRESULT hResult,
    __out NTSTATUS & status) noexcept
{
    if ((hResult & HighBytesMask) != Win32HighBytesMask)
    {
        return false;
    }
    
    DWORD errorCode = (hResult & LowBytesMask);

    switch (errorCode)
    {
        // Convert to NTStatus 
    case ERROR_DISK_FULL:
        status = STATUS_DISK_FULL;
        return true;
    case ERROR_ACCESS_DENIED:
        status = STATUS_ACCESS_DENIED;
        return true;
    case ERROR_OUTOFMEMORY:
    case ERROR_NOT_ENOUGH_MEMORY:
        status = STATUS_INSUFFICIENT_RESOURCES;
        return true;
    case ERROR_OPEN_FAILED:
        status = STATUS_OPEN_FAILED;
        return true;
        // Convert to SFStatus 
    case ERROR_FILE_NOT_FOUND:
        status = SF_STATUS_FILE_NOT_FOUND;
        return true;
    case ERROR_PATH_NOT_FOUND:
        status = SF_STATUS_DIRECTORY_NOT_FOUND;
        return true;
    }

    return false;
}
