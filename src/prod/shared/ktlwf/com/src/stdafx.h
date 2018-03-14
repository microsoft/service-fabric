// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if !defined(PLATFORM_UNIX)
#include <ktl.h>
#include "Common/Common.h"
#else
#include "Common/Common.h"
#include <ktl.h>
#endif

#include "KUnknown.h"

#include "FabricRuntime_.h"

using Common::ErrorCode;
using Common::ComPointer;

#define COMMON_ALLOCATION_TAG 'FCMN'

