/*++

Copyright (c) 2012 Microsoft Corporation

Module Name:

    RvdLogger.cpp

Abstract:

    This file contains test case implementations.
--*/
#include "RvdLoggerTests.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
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
