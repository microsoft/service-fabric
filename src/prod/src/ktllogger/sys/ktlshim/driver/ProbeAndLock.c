// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <ntddk.h>

//
// This routine is in its own separate .c file since the underlying
// MmProbeAndLockPages api requires the use of exceptions to return
// errors. This worker routine will handle the exception and return the
// error code.
//
NTSTATUS
KtlLogMmProbeAndLockPagesNoException(
    __in PMDL Mdl,
    __in KPROCESSOR_MODE AccessMode,
    __in LOCK_OPERATION Operation
    )
{
    NTSTATUS status;
    
    try 
    {            
        //
        // Probe and lock the pages of this buffer in physical memory.
        // You can specify IoReadAccess, IoWriteAccess or IoModifyAccess
        // Always perform this operation in a try except block. 
        // MmProbeAndLockPages will raise an exception if it fails.
        //
        MmProbeAndLockPages(Mdl, AccessMode, Operation);
        status = STATUS_SUCCESS;
    }
    except(EXCEPTION_EXECUTE_HANDLER) 
    {
        status = GetExceptionCode();
    }
    
    return(status);
}