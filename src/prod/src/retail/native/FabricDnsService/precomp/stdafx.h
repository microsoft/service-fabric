// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "ktl.h"
#include "Common/KtlSF.Common.h"

#include "FabricClient_.h"
#include "FabricCommon_.h"
#include "FabricRuntime_.h"

#include "ComUnknown.h"
#include "ComPointer.h"
#include "IDnsTracer.h"
#include "IDnsParser.h"
#include "INetIoManager.h"
#include "IFabricResolve.h"
#include "IDnsService.h"

#include "FabricAsyncOperationContext.h"
#include "FabricStringResult.h"
#include "FabricAsyncOperationCallback.h"

#if !defined(PLATFORM_UNIX)
static ULONG TAG = 'SSND';
#else
#define TAG 'SSND'
#endif

using namespace DNS;
