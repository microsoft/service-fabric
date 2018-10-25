// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef long RPC_STATUS;

#define __RPC_STUB
#define __RPC_USER
#define __RPC_FAR

#if !defined(__RPC_MAC__) && ( (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) )
#	define __RPC_API  __stdcall
#	define __RPC_USER __stdcall
#	define __RPC_STUB __stdcall
#	define  RPC_ENTRY __stdcall
#else // Not Win32/Win64
#	define __RPC_API
#	define __RPC_USER
#	define __RPC_STUB
#	define RPC_ENTRY
#endif

#if !defined(_RPCRT4_) && !defined(_KRPCENV_)
#define RPCRTAPI DECLSPEC_IMPORT
#else
#define RPCRTAPI
#endif

#ifdef __cplusplus
}
#endif

#include "rpcdce.h"
