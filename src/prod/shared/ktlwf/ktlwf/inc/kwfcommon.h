// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <kwftypes.h>

//
// Facility codes.  Define KTL WinFab wrapper to be Facility 0xEF.  Other components will use other facility codes.
//

const USHORT FacilityKtlWf = 0xEF;

#if KTL_USER_MODE

inline
NTSTATUS
KWfResultToNtStatus(
    __in HRESULT Hr
    )
{
    switch (Hr)
    {
        case FABRIC_E_INVALID_NAME_URI: return STATUS_OBJECT_NAME_INVALID;
        case FABRIC_E_NAME_ALREADY_EXISTS: return STATUS_OBJECT_NAME_EXISTS;
        case FABRIC_E_SERVICE_ALREADY_EXISTS: return STATUS_OBJECT_NAME_EXISTS;
        case FABRIC_E_TIMEOUT: return STATUS_IO_TIMEOUT;
        case FABRIC_E_COMMUNICATION_ERROR: return STATUS_UNEXPECTED_NETWORK_ERROR;
        case FABRIC_E_INVALID_ADDRESS: return STATUS_INVALID_ADDRESS;
        case FABRIC_E_NAME_DOES_NOT_EXIST: return STATUS_OBJECT_NAME_NOT_FOUND;
        case FABRIC_E_PROPERTY_DOES_NOT_EXIST: return STATUS_RESOURCE_NAME_NOT_FOUND;
        case FABRIC_E_VALUE_TOO_LARGE: return STATUS_FILE_TOO_LARGE;

        default:
            if (SUCCEEDED(Hr))
            {
                return KtlStatusCode(STATUS_SEVERITY_SUCCESS, FacilityKtlWf, SCODE_CODE(Hr));
            }
            else
            {
                return KtlStatusCode(STATUS_SEVERITY_ERROR, FacilityKtlWf, SCODE_CODE(Hr));
            }
    }
}

#endif

//
// Often we have the following pattern:
//
// AsyncOperation(
//      ...
//      __in KAsyncContextBase::CompletionCallback Completion,
//      __in_opt KAsyncContextBase* const ParentAsync = nullptr,
//      __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
//    );
//
// This pattern is similar to WinFab IFabricAsyncOperationContext pattern,
// where the caller supplies a pointer-to-pointer to an async context.
// The difference is that AsyncOperation() allows the caller supplies an existing async context.
//
// BeginAsyncOperation(
//      ...
//      __in IFabricAsyncOperationCallback* Completion,
//      __inout_opt IFabricAsyncOperationContext** Context
//    );
//
// This helper function gets an async context from the given ChildAsync, if there is one,
// or allocates a new one.
//

typedef KDelegate<NTSTATUS(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag
    )> KWfContextAllocator;

template <class ChildAsyncType>
inline NTSTATUS
KWfGetContext(
    __in KWfContextAllocator ContextAllocator,
    __inout_opt KAsyncContextBase::SPtr *ChildAsync,
    __out KSharedPtr<ChildAsyncType>& RealChildAsync
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KAsyncContextBase::SPtr baseAsync;

    if (ChildAsync == nullptr || *ChildAsync == nullptr)
    {
        status = ContextAllocator(baseAsync, KTL_TAG_WINFAB);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if (ChildAsync)
        {
            *ChildAsync = baseAsync.RawPtr();
        }
    }
    else
    {
        baseAsync = *ChildAsync;
    }

    KAssert(baseAsync);
    RealChildAsync = baseAsync.DownCast<ChildAsyncType>();

    return STATUS_SUCCESS;
}

