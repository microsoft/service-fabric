// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if !PLATFORM_UNIX
#include <sal.h>
#endif
#include <ktl.h>
#include <KTpl.h>
#include <ktrace.h>
#include "../../ktllogger/ktlloggerX.h"
#include "../../ktllogger/sys/inc/KLogicalLog.h"
#include "../../ktllogger/sys/inc/ktllogger.h"
#include "../utilities/Data.Utilities.Public.h"

namespace Data
{
    namespace Log
    {
        using ::_delete;
    }
}

namespace LogTests 
{ 
    class LogTestBase; // forward declaration for making the impl friends with the tests
    class LogicalLogTest; // forward declaration for making the impl friends with the tests
    using ::_delete;
    using namespace Data::Log;
}

#include "KtlLoggerMode.h"
#include "LogCreationFlags.h"
#include "ILogicalLogReadStream.h"
#include "ILogicalLog.h"
#include "IPhysicalLogHandle.h"
#include "ILogManagerHandle.h"
#include "LogManager.h"
#include "FileLogicalLogReadStream.h"
#include "FileLogicalLog.h"
