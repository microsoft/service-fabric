// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "rpc.h"
#include "rpcdce.h"
#include "ntstatus.h"
#include <uuid/uuid.h>

RPCRTAPI
RPC_STATUS
RPC_ENTRY
UuidCreate (
    UUID __RPC_FAR * Uuid
    )
{
    uuid_generate((unsigned char*)Uuid);
    return RPC_S_OK;
}
