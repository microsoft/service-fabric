// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <ktl.h>
#include <ktrace.h>
#include <minmax.h>
#include <KtlPhysicalLog.h>
#include <kinstrumentop.h>
#include "ktllogger.h"

#include "DevIoUmKm.h"
#include "fileio.h"

#include "KtlLogMarshal.h"

#include "MBInfoAccess.h"
#include "KLogicalLog.h"
#include "GlobalObj.h"

#include "shimkm.h"
