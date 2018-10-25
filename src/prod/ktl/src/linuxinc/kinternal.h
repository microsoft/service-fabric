/*++

    (c) 2017 by Microsoft Corp. All Rights Reserved.

    kinternal.h

    Description:
        Internal KTL stuff for Linux

    History:

--*/

#pragma once

HANDLE
WINAPI
TlsGetKxmHandle();

BOOL
WINAPI
TlsSetupKxmHandle();

