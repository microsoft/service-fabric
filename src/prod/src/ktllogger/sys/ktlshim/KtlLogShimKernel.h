// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <ktl.h>
#include <ktrace.h>
#include "minmax.h"
#include <kinstrumentop.h>

#include "KtlPhysicalLog.h"
#include "../inc/ktllogger.h"
#include "../inc/KLogicalLog.h"

#include "DevIoUmKm.h"
#include "fileio.h"

#include "KtlLogMarshal.h"

#include "MBInfoAccess.h"
#include "servicewrapper.h"
#include "GlobalObj.h"

#include "shimkm.h"
