// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "rpcnterr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef GUID UUID;

typedef void __RPC_FAR * RPC_IF_HANDLE;

RPCRTAPI
RPC_STATUS
RPC_ENTRY
UuidCreate (
    UUID __RPC_FAR * Uuid
    );

#ifdef __cplusplus
}
#endif
