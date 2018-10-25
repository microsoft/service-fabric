// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef 
enum NET_FW_PROFILE_TYPE2_
    {	NET_FW_PROFILE2_DOMAIN	= 0x1,
	NET_FW_PROFILE2_PRIVATE	= 0x2,
	NET_FW_PROFILE2_PUBLIC	= 0x4,
	NET_FW_PROFILE2_ALL	= 0x7fffffff
    } 	NET_FW_PROFILE_TYPE2;

typedef 
enum NET_FW_IP_PROTOCOL_
    {	NET_FW_IP_PROTOCOL_TCP	= 6,
	NET_FW_IP_PROTOCOL_UDP	= 17,
	NET_FW_IP_PROTOCOL_ANY	= 256
    } 	NET_FW_IP_PROTOCOL;

#ifdef __cplusplus
}
#endif
