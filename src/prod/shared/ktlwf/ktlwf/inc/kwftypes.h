// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if !KTL_USER_MODE

//
// Turn on these flags to exclude headers that cannot be compiled in kernel mode.
// These headers are included in RPC headers.
//

#ifndef COM_NO_WINDOWS_H
#define COM_NO_WINDOWS_H
#endif

#ifndef _KRPCENV_
#define _KRPCENV_
#endif

#ifndef RPC_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#endif

//
// Defeat wtypes.h included in fabrictypes.h.
// wtypes.h has many dependencies that cannot be compiled in kernel mode.
//

#ifndef __wtypes_h__
#define __wtypes_h__
#endif

//
// Define missing types used by fabrictypes.h. 
// These types are defined in wtypes.h but wtypes.h is excluded in kernel mode compilation.
//

typedef unsigned char       BYTE;
typedef unsigned char       byte;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;

#endif

#include <FabricTypes.h>

