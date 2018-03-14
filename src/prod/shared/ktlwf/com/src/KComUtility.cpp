// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ktl.h"
#include "KComUtility.h"

using namespace Ktl::Com;

HRESULT 
KComUtility::ToHRESULT(NTSTATUS FromNtStatus)
{
    if (FromNtStatus == STATUS_SUCCESS)
    {
        return S_OK;
    }

    if (!NT_SUCCESS(FromNtStatus))
    {
        switch (FromNtStatus)
        {
            case B_STATUS_RECORD_NOT_EXIST:
                return FABRIC_E_KEY_NOT_FOUND;
            case B_STATUS_RECORD_EXISTS:
                return E_INVALIDARG;
            case B_STATUS_IDEMPOTENCE:
                return FABRIC_E_INVALID_OPERATION;
            case K_STATUS_SHUTDOWN_PENDING:
                return FABRIC_E_OBJECT_CLOSED;
            case STATUS_TIMEOUT:
                return FABRIC_E_TIMEOUT;
            case STATUS_CANCELLED:
                return E_ABORT;
            case STATUS_INSUFFICIENT_RESOURCES:
                return E_OUTOFMEMORY;
            case STATUS_INVALID_PARAMETER:
                return E_INVALIDARG;
        }
		
        DWORD error = RtlNtStatusToDosError(FromNtStatus);    // Try to go from Ntstatus to Win32

        if (error != ERROR_MR_MID_NOT_FOUND) 
        {
            // Switch over from Win32 to the corresponding HRESULT.
           return HRESULT_FROM_WIN32(error);
        }
    }

    return HRESULT_FROM_NT(FromNtStatus);
}
