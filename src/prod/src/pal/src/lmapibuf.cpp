// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "lmapibuf.h"

NET_API_STATUS NET_API_FUNCTION
NetApiBufferFree (
    IN LPVOID Buffer
    )
{
    delete[] Buffer;
    return 0;
}

