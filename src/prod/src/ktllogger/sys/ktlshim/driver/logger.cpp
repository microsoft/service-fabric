// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "logger.h"

NTSTATUS
KtlLogInitializeKtl()
{
    NTSTATUS status;
    
    KtlSystem* underlyingSystem;
    status = KtlSystem::Initialize(FALSE,             // Do not initialize VNetworking
                                   &underlyingSystem);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Turn on strict allocation tracking
    //
    underlyingSystem->SetStrictAllocationChecks(TRUE);

    EventRegisterMicrosoft_Windows_KTL();

    //
    // By default, enable debug trace mirroring.
    //
    KDbgMirrorToDebugger = FALSE;
    
    return(STATUS_SUCCESS);
}


VOID
KtlLogDeinitializeKtl()
{
    EventUnregisterMicrosoft_Windows_KTL();

    //
    // Reenable printfs in case there is some memory leaks.
    //
    KDbgMirrorToDebugger = TRUE;    
    KtlSystem::Shutdown();
}
