// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

BOOLEAN DNS::CompareKString(
    __in const KString::SPtr& spKeyOne,
    __in const KString::SPtr& spKeyTwo
)
{
    if (*spKeyOne == *spKeyTwo)
    {
        return TRUE;
    }

    return FALSE;
}

#if defined(PLATFORM_UNIX)
int WSAAPI DNS::closesocket(
    __in SOCKET s
    )
{
    close(s);
    return 0;
}
#endif
