// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define LOGMANAGER_TAG 'gMgL'
#define LOGMANAGER_HANDLE_TAG 'TMgL'
#define FILELOG_TAG 'goLF'

// Internal logger dependencies for UDRIVER/UPASSTHROUGH configurations
#if defined(UDRIVER) || defined(UPASSTHROUGH)
#include "../../ktllogger/sys/ktlshim/KtlLogShimKernel.h"
#endif

// Public headers and their dependencies.
#include "LogicalLog.Public.h"

// Internal headers.
#include "LogManagerHandle.h"
#include "LogicalLogKIoBufferStream.h"
#include "LogicalLogBuffer.h"
#include "LogicalLogOperationResults.h"
#include "LogicalLogReadTask.h"
#include "LogicalLogReadContext.h"
#include "ReadAsyncResults.h"
#include "LogicalLogStream.h"
#include "LogicalLog.h"
#include "LogicalLogInfo.h"
#include "PhysicalLog.h"
#include "PhysicalLogHandle.h"
