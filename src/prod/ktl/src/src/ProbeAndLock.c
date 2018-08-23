/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ProbeAndLock.c

Abstract:


Environment:

    Kernel mode only.

--*/

//
// This routine is in its own separate .c file since the underlying
// MmProbeAndLockPages api requires the use of exceptions to return
// errors. This worker routine will handle the exception and return the
// error code.
//
#if KTL_USER_MODE
#else
#include <ntddk.h>

NTSTATUS KtlMmProbeAndLockSelectedPagesNoException (
  _Inout_  PMDLX MemoryDescriptorList,
  _In_     PFILE_SEGMENT_ELEMENT SegmentArray,
  _In_     KPROCESSOR_MODE AccessMode,
  _In_     LOCK_OPERATION Operation
)
{
    NTSTATUS status = STATUS_SUCCESS;
    
    try
    {
        MmProbeAndLockSelectedPages(MemoryDescriptorList,
                                    SegmentArray,
                                    AccessMode,
                                    Operation);
    }
    except(EXCEPTION_EXECUTE_HANDLER) 
    {
        status = GetExceptionCode();
    }
    
    return(status);
}

#endif
