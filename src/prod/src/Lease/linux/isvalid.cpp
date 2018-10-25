// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

BOOLEAN
IsValidProcessHandle(
    __in HANDLE UserModeProcessHandle
    )
    
/*++

Routine Description:

    Validates a user mode process handle.

Arguments:

    UserModeProcessHandle - user mode process handle to check.

Return Value:

    FALSE if the process handle is invalid, TRUE otherwise.

--*/

{
    UserModeProcessHandle;
    return TRUE;
//    PEPROCESS ProcessHandle = NULL;
//    PEPROCESS CurrentProcessHandle = NULL;
//
//    //
//    // Check user mode handle.
//    //
//    NTSTATUS Status = ObReferenceObjectByHandle(
//        UserModeProcessHandle,
//        STANDARD_RIGHTS_READ | STANDARD_RIGHTS_WRITE,
//        *PsProcessType,
//        UserMode,
//        &ProcessHandle,
//        NULL
//        );
//
//    if (NT_SUCCESS(Status)) {
//
//        //
//        // Accept only current process.
//        //
//        CurrentProcessHandle = PsGetCurrentProcess();
//
//        if (CurrentProcessHandle != ProcessHandle) {
//
//            Status = STATUS_INVALID_PARAMETER;
//        }
//
//        ObDereferenceObject(ProcessHandle);
//    }
//
//    return NT_SUCCESS(Status);
}

BOOLEAN
IsValidString(
    PWCHAR String,
    __in ULONG StringWcharLengthIncludingNullTerminator
    )

/*++

Routine Description:

    Validates a string.

Arguments:

    String - string to validate.

    StringWcharLengthIncludingNullTerminator - string length 
        including the null terminator.

Return Value:

    FALSE if the string is found incorrect, TRUE otherwise.

--*/

{
    //
    // Check string.
    //
    return wcsnlen((LPCWSTR)String, StringWcharLengthIncludingNullTerminator) < StringWcharLengthIncludingNullTerminator;
}

BOOLEAN
IsValidListenEndpoint(
    __in PTRANSPORT_LISTEN_ENDPOINT listenEndpoint
    )
{
    ULONG terminatorIndex;

    if (listenEndpoint->Port == 0)
    {
        return FALSE;
    }

    if ((listenEndpoint->ResolveType != AF_UNSPEC) &&
        (listenEndpoint->ResolveType != AF_INET) &&
        (listenEndpoint->ResolveType != AF_INET6))
    {
        return FALSE;
    }

    if (listenEndpoint->Address[0] == L'\0')
    {
        // Empty address string
        return FALSE;
    }

    // Look for the null terminator in the address string
    for (terminatorIndex = 1; terminatorIndex < ENDPOINT_ADDR_CCH_MAX; ++ terminatorIndex)
    {
        if (listenEndpoint->Address[terminatorIndex] == L'\0')
        {
            break;
        }
    }

    if (terminatorIndex == ENDPOINT_ADDR_CCH_MAX)
    {
        return FALSE;
    }

    for (terminatorIndex = 0; terminatorIndex < ENDPOINT_ADDR_CCH_MAX; ++terminatorIndex)
    {
        if (listenEndpoint->SspiTarget[terminatorIndex] == L'\0')
        {
            break;
        }
    }

    return terminatorIndex != ENDPOINT_ADDR_CCH_MAX;
}

BOOLEAN
IsValidDuration(
    __in LONG Duration
    )

/*++

Routine Description:

    Validates the lease TTL.

Arguments:

    Duration - lease TTL.

Return Value:

    FALSE if the duration is found incorrect, TRUE otherwise.

--*/

{
    if (0 >= Duration) {

        return FALSE;
    }

    if (Duration == DURATION_MAX_VALUE) {

        //
        // We use this for terminate messages.
        //
        return TRUE;
    }

    //
    // Need to figure out exactly the upper bound so we do not overflow
    // when we compute timer expiration times.
    //
    if (Duration > DURATION_MAX_VALUE / 2) {

        return FALSE;
    }

    return TRUE;
}

