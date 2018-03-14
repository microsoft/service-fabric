// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "RvdLoggerTests.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include <ktllogger.h>
#include <InternalKtlLogger.h>
#include "RvdLogTestHelpers.h"

#include "VerifyQuiet.h"

#include "RvdLoggerParallelTests.h"
#include "RvdLoggerTests.cpp"
#include "RvdLoggerParallelTests.cpp"
#include "RvdLoggerVerifyTests.cpp"
#include "RvdLogStreamAsyncIOTests.cpp"
#include "RvdLogTestHelpers.cpp"
#include "RvdLoggerRecoveryTests.cpp"
#include "RvdLoggerReservationTests.cpp"
#include "RvdLoggerAliasTests.cpp"
